#ifndef CA_LIVE_TIMING_BAR_H
#define CA_LIVE_TIMING_BAR_H

// Real-time timing assist: a thin track along the bottom of the screen, with ticks at the
// moments the ball reaches the bat on the front and back foot, and a marker that moves
// as the ball travels. Shot-agnostic (it shows before you've chosen a shot).
// Optional: toggled between balls. Candidate for a training / easy-difficulty option.

#include "bn_vector.h"
#include "bn_sprite_ptr.h"

namespace ca
{
    struct delivery_preview;

    class live_timing_bar
    {

    public:
        void start(const delivery_preview& preview);
        void update(int frame);
        void hide();

    private:
        static constexpr int TRACK_PIECES = 7;

        bn::vector<bn::sprite_ptr, TRACK_PIECES> _track;
        bn::vector<bn::sprite_ptr, 3> _marks;     // front tick, back tick, mover
        int _end_frame = 1;

        [[nodiscard]] int _x(int frame) const;
    };
}

#endif
