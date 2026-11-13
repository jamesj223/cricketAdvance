#include "ca_live_timing_bar.h"

#include "bn_math.h"

#include "bn_sprite_items_live_track.h"
#include "bn_sprite_items_live_marks.h"

#include "ca_delivery.h"
#include "ca_draw_order.h"

namespace ca
{

namespace
{
    constexpr int BAR_Y = 76;
    constexpr int LEFT = -112;
    constexpr int WIDTH = 224;

    // The track runs from release to a little after the back-foot contact moment.
    constexpr int FRAMES_AFTER_BACK_CONTACT = 8;
}

void live_timing_bar::start(const delivery_preview& preview)
{
    hide();
    _end_frame = preview.back_contact_frame + FRAMES_AFTER_BACK_CONTACT;

    for(int index = 0; index < TRACK_PIECES; ++index)
    {
        bn::sprite_ptr piece = bn::sprite_items::live_track.create_sprite(LEFT + 16 + index * 32, BAR_Y);
        piece.set_z_order(draw_order::TIMING_BAR);
        _track.push_back(bn::move(piece));
    }

    for(int frame : { preview.front_contact_frame, preview.back_contact_frame })
    {
        bn::sprite_ptr tick = bn::sprite_items::live_marks.create_sprite(_x(frame), BAR_Y, 0);
        tick.set_z_order(draw_order::TIMING_BAR);
        _marks.push_back(bn::move(tick));
    }

    bn::sprite_ptr mover = bn::sprite_items::live_marks.create_sprite(_x(0), BAR_Y, 1);
    mover.set_z_order(draw_order::TEXT);   // on top of the ticks
    _marks.push_back(bn::move(mover));
}

void live_timing_bar::update(int frame)
{
    if(_marks.size() == 3)
    {
        _marks[2].set_x(_x(frame));
    }
}

void live_timing_bar::hide()
{
    _track.clear();
    _marks.clear();
}

int live_timing_bar::_x(int frame) const
{
    int clamped = bn::clamp(frame, 0, _end_frame);
    return LEFT + (WIDTH * clamped) / _end_frame;
}

}
