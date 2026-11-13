#include "ca_ball_view.h"

#include "bn_math.h"
#include "bn_blending.h"

#include "bn_sprite_items_ball.h"
#include "bn_sprite_items_shadow.h"
#include "bn_sprite_items_marker.h"

#include "ca_camera.h"
#include "ca_delivery.h"
#include "ca_draw_order.h"

namespace ca
{

namespace
{
    // Affine sprites in double-size mode can only show up to 2x without clipping.
    constexpr bn::fixed MAX_SCALE = 2;
}

ball_view::ball_view() :
    _ball(bn::sprite_items::ball.create_sprite(0, 0)),
    _shadow(bn::sprite_items::shadow.create_sprite(0, 0)),
    _marker(bn::sprite_items::marker.create_sprite(0, 0))
{
    _marker.set_z_order(draw_order::MARKER);

    // The shadow is semi-transparent so the pitch shows through it.
    _shadow.set_blending_enabled(true);
    bn::blending::set_transparency_alpha(0.5);

    hide();
}

void ball_view::update(const ball_flight& flight, bn::fixed batter_z)
{
    const vec3& pos = flight.position();
    camera::screen_point ball_screen = camera::project(pos);
    camera::screen_point shadow_screen = camera::project({ pos.x, pos.z, 0 });

    // Layering: in front of the batter, between batter and stumps, or behind the stumps.
    if(pos.z < batter_z)
    {
        _ball.set_z_order(draw_order::BALL_IN_FRONT);
        _shadow.set_z_order(draw_order::SHADOW_IN_FRONT);
    }
    else if(pos.z < STUMPS_Z)
    {
        _ball.set_z_order(draw_order::BALL_MIDDLE);
        _shadow.set_z_order(draw_order::SHADOW_MIDDLE);
    }
    else
    {
        _ball.set_z_order(draw_order::BALL_BEHIND);
        _shadow.set_z_order(draw_order::SHADOW_BEHIND);
    }

    _ball.set_position(ball_screen.x, ball_screen.y);
    _ball.set_scale(bn::min(ball_screen.scale, MAX_SCALE));
    _ball.set_visible(true);

    _shadow.set_position(shadow_screen.x, shadow_screen.y);
    _shadow.set_scale(bn::min(shadow_screen.scale, MAX_SCALE));
    _shadow.set_visible(true);

    if(flight.bounced() && ! flight.was_hit())
    {
        camera::screen_point marker_screen = camera::project(flight.bounce_position());
        _marker.set_position(marker_screen.x, marker_screen.y);
        _marker.set_visible(true);
    }
}

void ball_view::hide()
{
    _ball.set_visible(false);
    _shadow.set_visible(false);
    _marker.set_visible(false);
}

}
