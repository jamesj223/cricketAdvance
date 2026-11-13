#include "ca_delivery.h"

#include "bn_math.h"
#include "bn_random.h"

namespace ca
{

namespace
{
    // Roughly bell-shaped random value in [-spread, spread) (average of two uniforms).
    bn::fixed spread_random(bn::random& random, bn::fixed spread)
    {
        bn::fixed a = random.get_fixed(-1, 1);
        bn::fixed b = random.get_fixed(-1, 1);
        return mul((a + b) / 2, spread);
    }

    // Random value in [-amount, amount). Returns 0 when amount is 0 (bn::random
    // asserts on an empty range, so this must be checked).
    bn::fixed plus_minus(bn::random& random, bn::fixed amount)
    {
        if(amount <= 0)
        {
            return 0;
        }

        return random.get_fixed(-amount, amount);
    }

    // Linear interpolation of the ball between two frames, at z = target_z.
    vec3 interpolate_at_z(const vec3& from, const vec3& to, bn::fixed target_z)
    {
        bn::fixed t = div(target_z - from.z, to.z - from.z);
        return { from.x + mul(to.x - from.x, t), target_z, from.h + mul(to.h - from.h, t) };
    }
}

delivery_plan generate_delivery(const bowler_profile& bowler, bn::random& random)
{
    delivery_plan plan;
    plan.bowler = &bowler;
    plan.speed_kmh = random.get_int(bowler.speed_min_kmh, bowler.speed_max_kmh + 1);
    plan.release = { release_x(bowler), RELEASE_Z, bowler.release_h };

    bn::fixed length;
    bn::fixed line;

    if(random.get_int(100) < bowler.bad_ball_percent)
    {
        // Loose ball: anything from a full toss to a long hop, wide either side.
        length = random.get_fixed(-0.5, 11);
        line = random.get_fixed(-1.2, 0.8);
    }
    else
    {
        length = bowler.target_length + spread_random(random, bowler.length_spread);
        line = bowler.target_line + spread_random(random, bowler.line_spread);
    }

    if(length < bn::fixed(0.3))
    {
        length = 0.3;   // TODO: proper full tosses (bounce behind the batter)
    }

    plan.bounce_x = line;
    plan.bounce_z = STUMPS_Z - length;
    plan.air_movement = bowler.air_movement + plus_minus(random, bowler.air_movement_variation);
    plan.deviation = bowler.bounce_movement + plus_minus(random, bowler.bounce_movement_variation);
    return plan;
}

ball_flight::ball_flight(const delivery_plan& plan) :
    _pos(plan.release),
    _air_movement(plan.air_movement),
    _deviation(plan.deviation),
    _restitution(plan.bowler->bounce_restitution),
    _friction(plan.bowler->bounce_friction)
{
    // Solve for the release velocity that lands the ball on the planned spot.
    bn::fixed speed = div(plan.speed_kmh, 3.6);   // m/s
    bn::fixed t = div(plan.bounce_z - plan.release.z, speed);       // time to bounce
    bn::fixed half_t2 = mul(t, t) / 2;

    _vel.z = speed;
    // h(t) = h0 + vh*t - g/2 t^2 = 0
    _vel.h = div(mul(GRAVITY, half_t2) - plan.release.h, t);
    // x(t) = x0 + vx*t + a/2 t^2 = bounce_x  (so swing curves the ball *into* the target)
    _vel.x = div(plan.bounce_x - plan.release.x - mul(_air_movement, half_t2), t);
}

bool ball_flight::update()
{
    if(_dead)
    {
        return false;
    }

    ++_frames;
    vec3 prev = _pos;

    // Constant acceleration within a frame: moving by the average of the old and new
    // velocity is exact, so the ball lands where the plan says (no Euler drift).
    vec3 old_vel = _vel;
    _vel.x += mul(_air_movement, DT);
    _vel.h -= mul(GRAVITY, DT);
    _pos.x += mul((old_vel.x + _vel.x) / 2, DT);
    _pos.z += mul((old_vel.z + _vel.z) / 2, DT);
    _pos.h += mul((old_vel.h + _vel.h) / 2, DT);

    if(_pos.h <= 0 && _vel.h < 0)
    {
        // The ball went through the ground during this frame: move it back to the
        // exact point of contact so the bounce (and pitch marker) is where it should be.
        if(prev.h > 0)
        {
            bn::fixed t = div(prev.h, prev.h - _pos.h);
            _pos.x = prev.x + mul(_pos.x - prev.x, t);
            _pos.z = prev.z + mul(_pos.z - prev.z, t);
        }

        _pos.h = 0;
        _vel.h = -mul(_vel.h, _restitution);
        _vel.z = mul(_vel.z, _friction);

        if(! _bounced)
        {
            _bounced = true;
            _bounce_pos = _pos;
            _vel.x += _deviation;   // seam / turn off the pitch
            _air_movement = 0;      // no more swing once it has pitched
        }
    }

    if(! _reached_crease && _pos.z >= POPPING_CREASE_Z)
    {
        _reached_crease = true;
        _crease_pos = interpolate_at_z(prev, _pos, POPPING_CREASE_Z);
    }

    if(! _hit && ! _reached_stumps && _pos.z >= STUMPS_Z)
    {
        _reached_stumps = true;
        vec3 at_stumps = interpolate_at_z(prev, _pos, STUMPS_Z);

        if(bn::abs(at_stumps.x) <= STUMP_HALF_WIDTH + BALL_RADIUS &&
                at_stumps.h <= STUMP_HEIGHT + BALL_RADIUS)
        {
            if(_stumps_live)
            {
                _hit_stumps = true;
                _pos = at_stumps;
                _dead = true;
            }
            else
            {
                _stumps_hit_pending = true;
                _stumps_hit_pos = at_stumps;
            }
        }
    }

    if(_hit)
    {
        // Off the bat: dead once it's well away (or has stopped). Stopping before the
        // camera (z ~ -24) also keeps the projection maths in a safe range.
        if(_pos.z < -12 || _pos.z > 40 || bn::abs(_pos.x) > 30 || _frames - _hit_frame > 90)
        {
            _dead = true;
        }
    }
    else if(_pos.z >= DEAD_BALL_Z || _frames > 60 * 5)
    {
        _dead = true;
    }

    return ! _dead;
}

void ball_flight::hit(const vec3& velocity)
{
    _vel = velocity;
    _air_movement = 0;
    _deviation = 0;
    _bounced = true;            // so no seam/turn is applied if it lands
    _restitution = 0.45;        // outfield bounce
    _friction = 0.85;
    _hit = true;
    _hit_frame = _frames;
    _stumps_hit_pending = false;
}

void ball_flight::suspend_stumps()
{
    _stumps_live = false;
}

void ball_flight::resume_stumps()
{
    if(_stumps_live)
    {
        return;
    }

    _stumps_live = true;

    if(_stumps_hit_pending && ! _hit)
    {
        _hit_stumps = true;
        _pos = _stumps_hit_pos;
        _dead = true;
    }

    _stumps_hit_pending = false;
}

delivery_preview preview_delivery(const delivery_plan& plan)
{
    ball_flight flight(plan);
    delivery_preview preview = {};
    bool front = false;
    bool back = false;
    bool stumps = false;

    while(true)
    {
        bool alive = flight.update();
        bn::fixed z = flight.position().z;

        if(! front && z >= FRONT_FOOT_CONTACT_Z)
        {
            front = true;
            preview.front_contact_frame = flight.frames();
        }

        if(! back && z >= BACK_FOOT_CONTACT_Z)
        {
            back = true;
            preview.back_contact_frame = flight.frames();
        }

        // Reaching the stumps (or the ball dying, e.g. by hitting them) ends the batter's
        // chance to play it.
        if(! stumps && (z >= STUMPS_Z || ! alive))
        {
            stumps = true;
            preview.stumps_frame = flight.frames();
        }

        if(! alive)
        {
            break;
        }
    }

    preview.length = STUMPS_Z - flight.bounce_position().z;
    preview.x_at_crease = flight.crease_position().x;
    return preview;
}

}
