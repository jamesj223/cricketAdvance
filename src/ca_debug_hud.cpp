#include "ca_debug_hud.h"

#include "common_variable_8x8_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"

#include "ca_draw_order.h"

namespace ca
{

debug_hud::debug_hud() :
    _small(common::variable_8x8_sprite_font),
    _big(common::variable_8x16_sprite_font)
{
    _small.set_z_order(draw_order::TEXT);
    _big.set_z_order(draw_order::TEXT);
    _big.set_center_alignment();
}

void debug_hud::set_line(int index, const bn::string_view& text)
{
    _lines[index] = text;
    _dirty = true;
}

void debug_hud::clear_lines()
{
    for(bn::string<48>& line : _lines)
    {
        line.clear();
    }

    _banner.clear();
    _dirty = true;
}

void debug_hud::set_banner(const bn::string_view& text)
{
    _banner = text;
    _dirty = true;
}

void debug_hud::set_help(const bn::string_view& line_1, const bn::string_view& line_2)
{
    _help[0] = line_1;
    _help[1] = line_2;
    _dirty = true;
}

void debug_hud::update()
{
    if(! _dirty)
    {
        return;
    }

    _dirty = false;
    _sprites.clear();

    // Line positions keep clear of the batter (top centre) and the ball's path.
    // Line 0: top-left. Line 1: top-right. Lines 2-4: lower-left, above the timing bar.
    struct position
    {
        int x;
        int y;
        bool right_aligned;
    };

    constexpr position positions[LINES] = {
        { -116, -74, false },
        { 116, -64, true },
        { -116, 8, false },
        { -116, 18, false },
        { -116, 28, false },
    };

    for(int index = 0; index < LINES; ++index)
    {
        const position& pos = positions[index];

        if(pos.right_aligned)
        {
            _small.set_right_alignment();
        }
        else
        {
            _small.set_left_alignment();
        }

        _small.generate(pos.x, pos.y, _lines[index], _sprites);
    }

    _small.set_center_alignment();
    _small.generate(0, 58, _help[0], _sprites);
    _small.generate(0, 67, _help[1], _sprites);

    if(! _banner.empty())
    {
        _big.generate(0, -14, _banner, _sprites);
    }
}

}
