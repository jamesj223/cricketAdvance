#ifndef CA_TIMING_BAR_H
#define CA_TIMING_BAR_H

// After-the-fact timing feedback: a row of coloured cells, one per frame either side of
// the ideal moment, coloured by what pressing on that frame would have given for the
// shot you played (green perfect, yellow good, orange mistimed, red miss). An arrow
// marks where you actually pressed; a tick marks the ideal moment.
//
// It slides up from behind the scoreboard strip. No masking needed: the strip is a
// background at priority 2 and the bar's sprites are at priority 3, so the strip is
// drawn in front of them (and the bar in front of the pitch, also at 3: at equal
// priority, sprites beat backgrounds).

#include "bn_vector.h"
#include "bn_sprite_ptr.h"

namespace ca
{
    class timing_bar
    {

    public:
        static constexpr int RANGE = 12;    // frames shown either side of the ideal

        // Where the bar sits (screen y of the cells). Quick Play has no help line, so it
        // can sit lower, out of the way.
        void set_y(int y) { _y = y; }

        // Build the bar for this shot, but keep it hidden (below the screen) until reveal().
        void prepare(int error_frames, int rating, bool defensive);

        // Slide the prepared bar up after this many frames.
        void reveal(int delay_frames = 0);

        void hide();
        void set_visible(bool visible);

        // Call once per frame.
        void update();

    private:
        bn::vector<bn::sprite_ptr, RANGE * 2 + 1> _cells;
        bn::vector<bn::sprite_ptr, 2> _markers;
        int _y = 44;
        int _delay = -1;        // frames until it starts sliding (-1 = not revealed yet)
        int _offset = 0;        // pixels below where it rests

        void _place();
    };
}

#endif
