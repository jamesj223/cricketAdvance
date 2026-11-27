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
    constexpr int CELL_WIDTH = 8;
    constexpr int ARROW_DY = -8;    // above the cells
    constexpr int TICK_DY = 7;      // below the cells
    constexpr int HIDDEN_OFFSET = 40;   // far enough down to be behind the strip / off screen

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

void timing_bar::prepare(int error_frames, int rating, bool defensive)
{
    hide();

    // Colour each cell by asking the real timing rules what that frame would have scored,
    // so the bar can never disagree with the game.
    for(int offset = -RANGE; offset <= RANGE; ++offset)
    {
        int colour = cell_colour(grade_timing(offset, rating, defensive));
        bn::sprite_ptr cell = bn::sprite_items::timing_cell.create_sprite(offset * CELL_WIDTH, 0, colour);
        cell.set_z_order(draw_order::TIMING_BAR);
        _cells.push_back(bn::move(cell));
    }

    int press = bn::clamp(error_frames, -RANGE, RANGE);
    bn::sprite_ptr arrow = bn::sprite_items::timing_marker.create_sprite(press * CELL_WIDTH, 0, 0);
    bn::sprite_ptr tick = bn::sprite_items::timing_marker.create_sprite(0, 0, 1);
    arrow.set_z_order(draw_order::TIMING_BAR);
    tick.set_z_order(draw_order::TIMING_BAR);
    _markers.push_back(bn::move(arrow));
    _markers.push_back(bn::move(tick));

    _offset = HIDDEN_OFFSET;
    _delay = -1;
    _place();
    set_visible(false);
}

void timing_bar::reveal(int delay_frames)
{
    if(! _cells.empty() && _delay < 0)
    {
        _delay = bn::max(delay_frames, 1);     // (1: it appears on the next update)
    }
}

void timing_bar::hide()
{
    _cells.clear();
    _markers.clear();
    _delay = -1;
}

void timing_bar::set_visible(bool visible)
{
    for(bn::sprite_ptr& cell : _cells)
    {
        cell.set_visible(visible);
    }

    for(bn::sprite_ptr& marker : _markers)
    {
        marker.set_visible(visible);
    }
}

void timing_bar::update()
{
    if(_delay < 0)
    {
        return;     // nothing to show, or waiting for reveal()
    }

    if(_delay > 0)
    {
        if(--_delay == 0)
        {
            set_visible(true);
        }

        return;
    }

    if(_offset > 0)
    {
        // Ease out, like the banners: cover a quarter of the remaining distance a frame.
        _offset = _offset * 3 / 4;
        _place();
    }
}

void timing_bar::_place()
{
    int y = _y + _offset;

    for(bn::sprite_ptr& cell : _cells)
    {
        cell.set_y(y);
    }

    if(_markers.size() == 2)
    {
        _markers[0].set_y(y + ARROW_DY);
        _markers[1].set_y(y + TICK_DY);
    }
}

}
