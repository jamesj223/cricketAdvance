#include "ca_field_view.h"

#include "bn_math.h"

#include "bn_sprite_items_field_dots.h"

namespace ca
{

namespace
{
    // --- The ground (keep in sync with make_field() in make_placeholder_graphics.py) ----
    constexpr int CENTRE_Y = -8;                    // screen y of the middle of the pitch
    constexpr bn::fixed GROUND_CENTRE_Z = STUMPS_Z / 2;
    constexpr bn::fixed BOUNDARY = 65;              // metres from the middle

    // --- The field: a 5-4 (five on the off side, four on the leg side) ------------------
    // Angle in degrees from straight back down the pitch (0 = past the bowler, -90 =
    // square on the off side, 180 = behind the keeper), and distance from the batter's
    // stumps in metres. The same convention as the shot angles in ca_shots.cpp.
    struct spot
    {
        int angle;
        int distance;
    };

    constexpr int KEEPER = 0;
    constexpr int BOWLER = 1;
    constexpr spot FIELD[] = {
        { 180, 0 },     // keeper (distance depends on the bowler, see below)
        { 0, 17 },      // bowler, in the follow-through
        { -168, 18 },   // slip
        { -135, 21 },   // gully
        { -95, 24 },    // point
        { -55, 27 },    // cover
        { -15, 27 },    // mid off
        { 15, 27 },     // mid on
        { 55, 27 },     // midwicket
        { 95, 24 },     // square leg
        { 150, 52 },    // fine leg (out on the rope)
    };
    constexpr int KEEPER_BACK = 14;
    constexpr int KEEPER_UP = 2;

    // --- Timings (frames) ---------------------------------------------------------------
    constexpr int HOLD_FRAMES = 45;         // after the event, before fading back
    constexpr int RUN_START_FRAME = 10;     // batters set off
    constexpr int RUN_FRAMES = 45;          // one run (22 yards)
    constexpr int FIELD_STOP_FRAMES = 8;    // pick up before throwing
    constexpr bn::fixed CHASE_SPEED = 7;    // m/s for a fielder chasing a lost cause
    constexpr int KEEPER_SET_OFF_FRAME = 15;    // keeper standing back sets off for the stumps
    constexpr bn::fixed KEEPER_SPEED = 6;       // m/s

    // How high things are drawn: a ball h metres up is drawn this many pixels per metre
    // above its shadow (a top-down view can't show height, so fake it).
    constexpr bn::fixed HEIGHT_PIXELS_PER_METRE = 0.6;

    // Sprite frames in field_dots.bmp.
    constexpr int FIELDER_FRAME = 0;
    constexpr int BATTER_FRAME = 1;
    constexpr int BALL_FRAME = 2;
    constexpr int SHADOW_FRAME = 3;

    bn::fixed lerp(bn::fixed a, bn::fixed b, bn::fixed t)
    {
        return a + mul(b - a, t);
    }

    bn::fixed fraction(int frame, int frames)
    {
        if(frame <= 0)
        {
            return 0;
        }

        if(frame >= frames)
        {
            return 1;
        }

        return div(frame, frames);
    }

    bn::fixed length(bn::fixed x, bn::fixed z)
    {
        return bn::fixed(bn::sqrt(mul(x, x) + mul(z, z)));
    }

    bn::sprite_ptr dot(int frame, int z_order)
    {
        bn::sprite_ptr sprite = bn::sprite_items::field_dots.create_sprite(0, 0, frame);
        sprite.set_z_order(z_order);
        return sprite;
    }
}

bool goes_into_the_field(const shot_outcome& outcome, bool defensive)
{
    switch(outcome.type)
    {
        case outcome_type::FOUR:
        case outcome_type::SIX:
            return true;

        case outcome_type::CAUGHT:
            return ! defensive;     // bat-pad catches stay in the batting view

        case outcome_type::RUNS:
        case outcome_type::EDGED:
            return outcome.runs > 0;

        default:
            return false;
    }
}

field_view::field_view(const shot_outcome& outcome, const vec3& hit_position, bool keeper_back) :
    _runs(outcome.runs),
    _start{ hit_position.x, hit_position.z },
    _ball(dot(BALL_FRAME, 0)),
    _shadow(dot(SHADOW_FRAME, 3))
{
    // The ball's path along the ground: straight out from the bat.
    bn::fixed speed = length(outcome.hit_velocity.x, outcome.hit_velocity.z);
    _dir = { div(outcome.hit_velocity.x, speed), div(outcome.hit_velocity.z, speed) };

    // Where everyone stands.
    for(int index = 0; index < FIELDERS; ++index)
    {
        spot place = FIELD[index];

        if(index == KEEPER)
        {
            place.distance = keeper_back ? KEEPER_BACK : KEEPER_UP;
        }

        bn::fixed sin = bn::degrees_lut_sin_safe(place.angle);
        bn::fixed cos = bn::degrees_lut_sin_safe(place.angle + 90);
        _fielders[index] = { sin * place.distance, STUMPS_Z - cos * place.distance };
        _fielder_sprites.push_back(dot(FIELDER_FRAME, 2));
    }

    // Throws come in to the keeper over the stumps: one standing back runs up for it.
    _keeper_stumps = { 0, STUMPS_Z + KEEPER_UP };
    _keeper_comes_up = keeper_back && outcome.type != outcome_type::FOUR &&
                       outcome.type != outcome_type::SIX && outcome.type != outcome_type::CAUGHT;

    for(int batter = 0; batter < 2; ++batter)
    {
        _batter_sprites.push_back(dot(BATTER_FRAME, 2));
    }

    // The fielder nearest the ball's path (and in front of the bat) goes after it. Not the
    // keeper or the bowler: a straight hit goes past the bowler to mid off / mid on.
    // For each fielder: how far along the path they are (projection, a dot product) and
    // how far off it (perpendicular distance, a 2D cross product).
    bn::fixed best_off_path = 1000;
    bn::fixed best_along = 0;

    for(int index = 0; index < FIELDERS; ++index)
    {
        if(index == KEEPER || index == BOWLER)
        {
            continue;
        }

        bn::fixed rel_x = _fielders[index].x - _start.x;
        bn::fixed rel_z = _fielders[index].z - _start.z;
        bn::fixed along = mul(rel_x, _dir.x) + mul(rel_z, _dir.z);
        bn::fixed off_path = bn::abs(mul(rel_x, _dir.z) - mul(rel_z, _dir.x));

        if(along > 8 && off_path < best_off_path)
        {
            best_off_path = off_path;
            best_along = along;
            _chaser = index;
        }
    }

    // How far along the path the rope is: solve |start + dir * s - centre| = BOUNDARY,
    // a quadratic in s (the same maths as a ray hitting a circle).
    bn::fixed from_centre_x = _start.x;
    bn::fixed from_centre_z = _start.z - GROUND_CENTRE_Z;
    bn::fixed b = mul(from_centre_x, _dir.x) + mul(from_centre_z, _dir.z);
    bn::fixed c = mul(from_centre_x, from_centre_x) + mul(from_centre_z, from_centre_z) -
                  mul(BOUNDARY, BOUNDARY);
    bn::fixed to_rope = -b + bn::fixed(bn::sqrt(mul(b, b) - c));

    switch(outcome.type)
    {
        case outcome_type::FOUR:
        {
            _kind = kind::FOUR;
            _ball_distance = to_rope + 4;
            _ball_frames = 100;
            // The ball slows down (ease-out: distance = end * (1 - (1 - t)^2)); solve for
            // the moment it crosses the rope: t = 1 - sqrt(1 - rope / end).
            bn::fixed t = 1 - bn::fixed(bn::sqrt(1 - div(to_rope, _ball_distance)));
            _event_frame = (t * _ball_frames).round_integer();
            break;
        }

        case outcome_type::SIX:
            _kind = kind::SIX;
            _ball_distance = to_rope + 12;
            _peak_height = 16;
            _ball_frames = 110;
            _event_frame = _ball_frames;
            break;

        case outcome_type::CAUGHT:
            _kind = kind::CATCH;
            _ball_distance = bn::max(best_along, bn::fixed(12));
            _peak_height = 8 + _ball_distance / 4;
            _ball_frames = 95;
            _event_frame = _ball_frames;
            break;

        default:
        {
            _kind = kind::RUNS;
            // More runs: the ball got further past the infield before it was cut off.
            _ball_distance = bn::min(best_along + 11 * (_runs - 1), to_rope - 6);
            _ball_frames = 20 + (_ball_distance * 5 / 2).round_integer();

            // Throw it back to the keeper so it arrives just after the batters complete
            // their runs (no run outs yet!).
            int runs_done = RUN_START_FRAME + _runs * RUN_FRAMES;
            point fielded = _along(_ball_distance);
            bn::fixed throw_distance = length(_keeper_stumps.x - fielded.x,
                                              _keeper_stumps.z - fielded.z);
            _throw_frames = 15 + (throw_distance / 2).round_integer();
            _throw_start = bn::max(_ball_frames + FIELD_STOP_FRAMES, runs_done - _throw_frames + 6);
            _event_frame = bn::max(_throw_start + _throw_frames, runs_done);
            break;
        }
    }

    // Where the chaser heads: the catch / pick-up point, or the rope (and no further) for
    // a boundary, at a believable running speed.
    if(_chaser >= 0)
    {
        point from = _fielders[_chaser];

        if(_kind == kind::CATCH || _kind == kind::RUNS)
        {
            _chaser_target = _along(_ball_distance);
            _chaser_frames = _ball_frames;
        }
        else
        {
            point rope = _along(to_rope - 2);
            bn::fixed dx = rope.x - from.x;
            bn::fixed dz = rope.z - from.z;
            bn::fixed distance = length(dx, dz);
            bn::fixed reach = div(CHASE_SPEED * _ball_frames, 60);

            if(distance > reach)
            {
                bn::fixed t = div(reach, distance);
                rope = { from.x + mul(dx, t), from.z + mul(dz, t) };
            }

            _chaser_target = rope;
            _chaser_frames = _ball_frames;
        }
    }

    _end_frame = _event_frame + HOLD_FRAMES;
    _draw();
}

field_view::status field_view::update()
{
    ++_frame;
    _draw();

    if(! _event_sent && _frame >= _event_frame)
    {
        _event_sent = true;
        return status::EVENT;
    }

    return _frame >= _end_frame ? status::DONE : status::RUNNING;
}

void field_view::skip()
{
    _frame = bn::max(_frame, _end_frame - 1);
}

field_view::point field_view::_along(bn::fixed distance) const
{
    return { _start.x + mul(_dir.x, distance), _start.z + mul(_dir.z, distance) };
}

field_view::point field_view::_keeper_position() const
{
    point back = _fielders[KEEPER];

    if(! _keeper_comes_up)
    {
        return back;
    }

    // Jog up at a steady speed: how far along (0..1) after this many frames?
    bn::fixed distance = length(_keeper_stumps.x - back.x, _keeper_stumps.z - back.z);
    bn::fixed covered = div(KEEPER_SPEED * bn::max(_frame - KEEPER_SET_OFF_FRAME, 0), 60);
    bn::fixed t = bn::min(div(covered, distance), bn::fixed(1));
    return { lerp(back.x, _keeper_stumps.x, t), lerp(back.z, _keeper_stumps.z, t) };
}

field_view::point field_view::_batter_position(int batter) const
{
    // Two batters: the striker starts at the top (batter's end), the non-striker at the
    // bottom; each run swaps them, along their own side of the pitch.
    constexpr bn::fixed TOP_Z = 19;
    constexpr bn::fixed BOTTOM_Z = 1.2;
    bn::fixed x = batter == 0 ? bn::fixed(1) : bn::fixed(-1);
    bool starts_top = batter == 0;

    if(_kind != kind::RUNS)
    {
        return { x, starts_top ? TOP_Z : BOTTOM_Z };
    }

    int running = bn::clamp(_frame - RUN_START_FRAME, 0, _runs * RUN_FRAMES);
    int runs_done = running / RUN_FRAMES;
    bn::fixed t = fraction(running % RUN_FRAMES, RUN_FRAMES);

    if(runs_done == _runs)
    {
        t = 0;
    }

    bool at_top = starts_top == (runs_done % 2 == 0);
    bn::fixed from = at_top ? TOP_Z : BOTTOM_Z;
    bn::fixed to = at_top ? BOTTOM_Z : TOP_Z;
    return { x, lerp(from, to, t) };
}

void field_view::_draw()
{
    auto place = [](bn::sprite_ptr& sprite, bn::fixed x, bn::fixed z, bn::fixed height)
    {
        sprite.set_position(x, CENTRE_Y - (z - GROUND_CENTRE_Z) - mul(height, HEIGHT_PIXELS_PER_METRE));
    };

    // Fielders: everyone stands still except the chaser.
    for(int index = 0; index < FIELDERS; ++index)
    {
        point at = _fielders[index];

        if(index == KEEPER)
        {
            at = _keeper_position();
        }
        else if(index == _chaser)
        {
            bn::fixed t = fraction(_frame, _chaser_frames);
            at = { lerp(at.x, _chaser_target.x, t), lerp(at.z, _chaser_target.z, t) };
        }

        place(_fielder_sprites[index], at.x, at.z, 0);
    }

    for(int batter = 0; batter < 2; ++batter)
    {
        point at = _batter_position(batter);
        place(_batter_sprites[batter], at.x, at.z, 0);
    }

    // The ball.
    point ball = _start;
    bn::fixed height = 0;
    bn::fixed t = fraction(_frame, _ball_frames);

    switch(_kind)
    {
        case kind::FOUR:
        case kind::RUNS:
        {
            // Along the ground, slowing down: ease-out.
            bn::fixed remaining = 1 - t;
            ball = _along(mul(_ball_distance, 1 - mul(remaining, remaining)));

            if(_kind == kind::RUNS && _frame > _throw_start)
            {
                // Thrown back to the keeper, in a flat arc.
                point keeper = _keeper_position();
                bn::fixed u = fraction(_frame - _throw_start, _throw_frames);
                ball = { lerp(ball.x, keeper.x, u), lerp(ball.z, keeper.z, u) };
                height = mul(4 * u, 1 - u) * 3;
            }
            break;
        }

        case kind::SIX:
        case kind::CATCH:
            // In the air: steady along the ground, a parabola for the height.
            ball = _along(mul(_ball_distance, t));
            height = mul(4 * t, 1 - t) * _peak_height.round_integer();
            break;

        default:
            break;
    }

    place(_ball, ball.x, ball.z, height);
    place(_shadow, ball.x, ball.z, 0);
    _shadow.set_visible(height > 0);

    // Caught: it's in the fielder's hands.
    bool held = _kind == kind::CATCH && _frame >= _ball_frames;
    _ball.set_visible(! held);
    _shadow.set_visible(! held && height > 0);
}

}
