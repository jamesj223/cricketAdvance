#ifndef CA_FIELD_VIEW_H
#define CA_FIELD_VIEW_H

// The top-down view of the whole ground, shown after a shot that goes into the field.
//
// For now this *acts out* a result that was already decided at the moment of the hit
// (ca_shots.cpp): the nearest fielder to the ball's path cuts it off, chases it to the
// rope or gets under the catch, and the batters run the runs. Later, fielders could decide
// the result instead (see docs/design.md).
//
// 1 pixel = 1 metre. The batter's end is at the top (like the TV view), off side on the
// left. The ground itself is a background (graphics/field.bmp, from
// tools/make_placeholder_graphics.py); this class owns the sprites on it.

#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "ca_shots.h"

namespace ca
{
    // Does this result get the field view? (Dots, blocks and edges to the keeper don't.)
    [[nodiscard]] bool goes_into_the_field(const shot_outcome& outcome, bool defensive);

    class field_view
    {

    public:
        enum class status
        {
            RUNNING,
            EVENT,      // this frame: the moment it's decided (rope, catch, runs completed)
            DONE,       // finished (after a short hold)
        };

        // keeper_back: the keeper stands back to pace bowling, up to the stumps for spin.
        field_view(const shot_outcome& outcome, const vec3& hit_position, bool keeper_back);

        [[nodiscard]] status update();

        // Jump to the end (the event still fires on the next update if it hasn't yet).
        void skip();

    private:
        struct point
        {
            bn::fixed x;
            bn::fixed z;
        };

        enum class kind
        {
            FOUR,       // along the ground, over the rope
            SIX,        // over the rope in the air
            CATCH,      // in the air to a fielder
            RUNS,       // cut off by a fielder and thrown in while the batters run
        };

        static constexpr int FIELDERS = 11;

        kind _kind;
        int _runs;
        point _start;               // where the bat met the ball
        point _dir;                 // unit vector along the ball's path
        bn::fixed _ball_distance;   // how far along the path the ball goes
        bn::fixed _peak_height;     // SIX / CATCH: top of the arc (metres)
        int _ball_frames;           // frames for the ball to get there
        int _throw_start = 0;       // RUNS: when the fielder throws it back...
        int _throw_frames = 0;      // ...and how long the throw takes
        int _event_frame;
        int _end_frame;
        int _frame = 0;
        bool _event_sent = false;

        point _fielders[FIELDERS];
        point _keeper_stumps;       // where the keeper takes a throw (up at the stumps)
        bool _keeper_comes_up = false;  // standing back, so runs up for the throw
        int _chaser = -1;           // the fielder going after the ball (-1 = none)
        point _chaser_target;
        int _chaser_frames = 1;

        bn::vector<bn::sprite_ptr, FIELDERS> _fielder_sprites;
        bn::vector<bn::sprite_ptr, 2> _batter_sprites;
        bn::sprite_ptr _ball;
        bn::sprite_ptr _shadow;

        [[nodiscard]] point _along(bn::fixed distance) const;
        [[nodiscard]] point _batter_position(int batter) const;
        [[nodiscard]] point _keeper_position() const;
        void _draw();
    };
}

#endif
