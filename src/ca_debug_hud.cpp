#include "ca_debug_hud.h"

#include "common_variable_8x8_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"

#include "bn_blending.h"
#include "bn_sprite_items_banner_band.h"

#include "ca_draw_order.h"

namespace ca
{

namespace
{
    constexpr int DEFAULT_BANNER_Y = -18;   // first line; between the batter and the info lines
    constexpr int BANNER_START_OFFSET[2] = { -256, 256 };   // slide in from left / right
    constexpr bn::fixed BAND_ALPHA = 0.75;          // 1 = solid, 0 = invisible
}

debug_hud::debug_hud() :
    _small(common::variable_8x8_sprite_font),
    _big(common::variable_8x16_sprite_font),
    _banner_y(DEFAULT_BANNER_Y)
{
    _small.set_z_order(draw_order::TEXT);
    _big.set_z_order(draw_order::TEXT);
    _big.set_center_alignment();

    // One global setting for every blended sprite/background (it's a hardware register).
    bn::blending::set_transparency_alpha(BAND_ALPHA);
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

    _set_banner(0, "");
    _set_banner(1, "");
    _dirty = true;
}

void debug_hud::set_lines_visible(bool visible)
{
    _lines_visible = visible;
    _dirty = true;
}

void debug_hud::set_banner(const bn::string_view& text)
{
    _set_banner(0, text);
}

void debug_hud::set_sub_banner(const bn::string_view& text)
{
    _set_banner(1, text);
}

void debug_hud::_set_banner(int index, const bn::string_view& text)
{
    banner& line = _banners[index];

    if(line.text == text)
    {
        return;     // already showing (don't slide it in again)
    }

    line.text = text;
    line.sprites.clear();
    line.base_x.clear();

    if(text.empty())
    {
        return;
    }

    // The band: eight 32-pixel pieces across the screen, semi-transparent, behind the text.
    for(int piece = 0; piece < 8; ++piece)
    {
        bn::sprite_ptr band = bn::sprite_items::banner_band.create_sprite(-112 + piece * 32, _banner_y + index * BANNER_LINE_HEIGHT);
        band.set_z_order(draw_order::BANNER_BAND);
        band.set_blending_enabled(true);
        line.sprites.push_back(bn::move(band));
    }

    _big.generate(0, _banner_y + index * BANNER_LINE_HEIGHT, text, line.sprites);

    for(const bn::sprite_ptr& sprite : line.sprites)
    {
        line.base_x.push_back(sprite.x());
    }

    line.offset = BANNER_START_OFFSET[index];
}

int debug_hud::default_banner_y()
{
    return DEFAULT_BANNER_Y;
}

void debug_hud::set_banner_y(int y)
{
    int delta = y - _banner_y;
    _banner_y = y;

    for(banner& line : _banners)
    {
        for(bn::sprite_ptr& sprite : line.sprites)
        {
            sprite.set_y(sprite.y() + delta);
        }
    }
}

void debug_hud::reset_banner_y()
{
    set_banner_y(DEFAULT_BANNER_Y);
}

void debug_hud::set_help(const bn::string_view& text)
{
    _help = text;
    _dirty = true;
}

void debug_hud::update()
{
    // Slide the banners in: each frame, cover 1/4 of the remaining distance (ease-out).
    for(banner& line : _banners)
    {
        if(line.offset != 0)
        {
            line.offset = line.offset * 3 / 4;

            for(int index = 0, count = line.sprites.size(); index < count; ++index)
            {
                line.sprites[index].set_x(line.base_x[index] + line.offset);
            }
        }
    }

    if(! _dirty)
    {
        return;
    }

    _dirty = false;
    _sprites.clear();

    // Line positions keep clear of the batter (top centre) and the ball's path.
    // Line 0: top-left. Line 1: under it (Quick Play's shot name). Lines 2-4:
    // lower-left, above the timing bar (Practice's aids).
    struct position
    {
        int x;
        int y;
        bool right_aligned;
    };

    constexpr position positions[LINES] = {
        { -116, -74, false },
        { -116, -64, false },
        { -116, 8, false },
        { -116, 18, false },
        { -116, 28, false },
    };

    for(int index = 0; index < LINES && _lines_visible; ++index)
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
    _small.generate(0, 58, _help, _sprites);     // just above the scoreboard strip
}

}
