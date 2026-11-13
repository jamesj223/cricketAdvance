#include "ca_timing_bar.h"

#include "bn_math.h"

#include "bn_sprite_items_timing_cell.h"
#include "bn_sprite_items_timing_marker.h"

#include "ca_draw_order.h"
#include "ca_shots.h"

namespace ca
{

namespace
{
    constexpr int BAR_Y = 44;
    constexpr int CELL_WIDTH = 8;

    // timing_cell frames: 0 red, 1 orange, 2 yellow, 3 green.
    int cell_colour(timing_grade grade)
    {
        switch(grade)
        {
            case timing_grade::PERFECT:
                return 3;
            case timing_grade::GOOD:
                return 2;
            case timing_grade::EARLY:
            case timing_grade::LATE:
                return 1;
            default:
                return 0;
        }
    }
}

void timing_bar::show(int error_frames, int rating, bool defensive)
{
    hide();

    // Colour each cell by asking the real timing rules what that frame would have scored,
    // so the bar can never disagree with the game.
    for(int offset = -RANGE; offset <= RANGE; ++offset)
    {
        int colour = cell_colour(grade_timing(offset, rating, defensive));
        bn::sprite_ptr cell = bn::sprite_items::timing_cell.create_sprite(
                    offset * CELL_WIDTH, BAR_Y, colour);
        cell.set_z_order(draw_order::TIMING_BAR);
        _cells.push_back(bn::move(cell));
    }

    int press = bn::clamp(error_frames, -RANGE, RANGE);
    bn::sprite_ptr arrow = bn::sprite_items::timing_marker.create_sprite(press * CELL_WIDTH, BAR_Y - 8, 0);
    bn::sprite_ptr tick = bn::sprite_items::timing_marker.create_sprite(0, BAR_Y + 7, 1);
    arrow.set_z_order(draw_order::TIMING_BAR);
    tick.set_z_order(draw_order::TIMING_BAR);
    _markers.push_back(bn::move(arrow));
    _markers.push_back(bn::move(tick));
}

void timing_bar::hide()
{
    _cells.clear();
    _markers.clear();
}

}
