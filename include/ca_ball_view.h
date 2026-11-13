#ifndef CA_BALL_VIEW_H
#define CA_BALL_VIEW_H

// Draws a ball_flight: the ball, its shadow, and a marker where it pitched.

#include "bn_sprite_ptr.h"
#include "ca_world.h"

namespace ca
{
    class ball_flight;

    class ball_view
    {

    public:
        ball_view();

        // batter_z: the ball is drawn behind the batter once it's further away than this.
        void update(const ball_flight& flight, bn::fixed batter_z);
        void hide();

    private:
        bn::sprite_ptr _ball;
        bn::sprite_ptr _shadow;
        bn::sprite_ptr _marker;
    };
}

#endif
