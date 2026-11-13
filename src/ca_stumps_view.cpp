#include "ca_stumps_view.h"

#include "bn_sprite_items_stumps.h"

#include "ca_camera.h"
#include "ca_draw_order.h"

namespace ca
{

namespace
{
    // Row of the sprite the base of the stumps is drawn on. Keep in sync with
    // STUMPS_BASE_ROW in tools/make_placeholder_graphics.py.
    constexpr int STUMPS_BASE_ROW = 13;
}

stumps_view::stumps_view() :
    _sprite(bn::sprite_items::stumps.create_sprite(0, 0))
{
    // A 16x16 sprite at (x, y) covers screen rows y-8 .. y+7. Line the base row up with
    // the pixel row the crease is drawn on, so the stumps sit exactly on it.
    int base_y = camera::project({ 0, STUMPS_Z, 0 }).y.floor_integer();
    _sprite.set_position(0, base_y + 8 - STUMPS_BASE_ROW);
    _sprite.set_z_order(draw_order::STUMPS);
}

void stumps_view::set_broken(bool broken)
{
    _sprite.set_tiles(bn::sprite_items::stumps.tiles_item(), broken ? 1 : 0);
}

}
