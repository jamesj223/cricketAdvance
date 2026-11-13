#ifndef CA_TIMING_BAR_H
#define CA_TIMING_BAR_H

// After-the-fact timing feedback: a row of coloured cells, one per frame either side of
// the ideal moment, coloured by what pressing on that frame would have given for the
// shot you played (green perfect, yellow good, orange mistimed, red miss). An arrow
// marks where you actually pressed; a tick marks the ideal moment.

#include "bn_vector.h"
#include "bn_sprite_ptr.h"

namespace ca
{
    class timing_bar
    {

    public:
        static constexpr int RANGE = 12;    // frames shown either side of the ideal

        void show(int error_frames, int rating, bool defensive);
        void hide();

    private:
        bn::vector<bn::sprite_ptr, RANGE * 2 + 1> _cells;
        bn::vector<bn::sprite_ptr, 2> _markers;
    };
}

#endif
