#ifndef CA_BALL_VIEW_H
#define CA_BALL_VIEW_H

// Draws a ball_flight: the ball, its shadow, and a marker where it pitched.

#include "bn_optional.h"
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

        // Over the field view: hide everything; afterwards, bring back only the marker (the
        // ball itself is long gone).
        void set_marker_only(bool visible);

        // Screen y of the pitch marker, if it's showing (so banners can keep clear of it).
        [[nodiscard]] bn::optional<bn::fixed> marker_y() const;

    private:
        bn::sprite_ptr _ball;
        bn::sprite_ptr _shadow;
        bn::sprite_ptr _marker;
        bool _has_marker = false;
    };
}

#endif
