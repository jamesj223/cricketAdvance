#ifndef CA_DELIVERY_H
#define CA_DELIVERY_H

#include "ca_world.h"
#include "ca_bowler.h"

namespace bn
{
    class random;
}

namespace ca
{
    // What the bowler is trying to bowl (decided at release).
    struct delivery_plan
    {
        const bowler_profile* bowler;
        int speed_kmh;
        vec3 release;
        bn::fixed bounce_x;         // planned line (x at the bounce)
        bn::fixed bounce_z;         // planned length (z of the bounce)
        bn::fixed air_movement;     // sideways acceleration in the air, m/s^2
        bn::fixed deviation;        // sideways velocity change at the bounce, m/s
    };

    [[nodiscard]] delivery_plan generate_delivery(const bowler_profile& bowler, bn::random& random);

    // The ball in flight, simulated one frame at a time.
    class ball_flight
    {

    public:
        explicit ball_flight(const delivery_plan& plan);

        // Advances one frame. Returns false once the ball is dead.
        bool update();

        // The bat hits the ball: it now travels with this velocity (m/s), and can no
        // longer hit the stumps.
        void hit(const vec3& velocity);

        // While suspended, reaching the stumps is only *remembered*, not acted on. Used
        // while the game waits a few frames to decide what shot was pressed (A+B chord),
        // so a shot pressed in time isn't ruled bowled just because the decision was late.
        // Resuming applies a remembered stumps hit, unless the bat has hit the ball.
        void suspend_stumps();
        void resume_stumps();

        [[nodiscard]] const vec3& position() const { return _pos; }
        [[nodiscard]] bool dead() const { return _dead; }
        [[nodiscard]] int frames() const { return _frames; }
        [[nodiscard]] bool was_hit() const { return _hit; }

        // Recorded as the ball travels (valid once the corresponding flag is set).
        [[nodiscard]] bool bounced() const { return _bounced; }
        [[nodiscard]] const vec3& bounce_position() const { return _bounce_pos; }
        [[nodiscard]] bool reached_crease() const { return _reached_crease; }
        [[nodiscard]] const vec3& crease_position() const { return _crease_pos; }
        [[nodiscard]] bool reached_stumps() const { return _reached_stumps; }
        [[nodiscard]] bool hit_stumps() const { return _hit_stumps; }

    private:
        vec3 _pos;
        vec3 _vel;
        bn::fixed _air_movement;
        bn::fixed _deviation;
        bn::fixed _restitution;
        bn::fixed _friction;
        vec3 _bounce_pos;
        vec3 _crease_pos;
        int _frames = 0;
        int _hit_frame = 0;
        bool _hit = false;
        bool _stumps_live = true;
        bool _stumps_hit_pending = false;
        vec3 _stumps_hit_pos;
        bool _bounced = false;
        bool _reached_crease = false;
        bool _reached_stumps = false;
        bool _hit_stumps = false;
        bool _dead = false;
    };

    // Everything worth knowing about a delivery before it's bowled, found by simulating a
    // copy of its flight to the end (cheap: ~60 steps). Frame numbers count from release,
    // matching ball_flight::frames().
    struct delivery_preview
    {
        bn::fixed length;           // bounce distance in front of the stumps
        bn::fixed x_at_crease;      // line, where it passes the popping crease
        int front_contact_frame;    // when it reaches the bat on the front foot ...
        int back_contact_frame;     // ... and on the back foot
        int stumps_frame;           // when it reaches the stumps
    };

    [[nodiscard]] delivery_preview preview_delivery(const delivery_plan& plan);
}

#endif
