#ifndef CA_STUMPS_VIEW_H
#define CA_STUMPS_VIEW_H

// The batter's stumps: a sprite (not part of the pitch background) so they can be shown
// broken, and so the ball can pass behind them.

#include "bn_sprite_ptr.h"

namespace ca
{
    class stumps_view
    {

    public:
        stumps_view();

        void set_broken(bool broken);

    private:
        bn::sprite_ptr _sprite;
    };
}

#endif
