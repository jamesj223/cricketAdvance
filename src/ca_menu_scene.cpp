#include "ca_menu_scene.h"

#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_pitch.h"
#include "common_variable_8x8_sprite_font.h"
#include "common_variable_8x16_sprite_font.h"

#include "ca_draw_order.h"

namespace ca
{

namespace
{
    // Menu rows: the two modes (their row number = the game_mode value), then controls.
    constexpr int ROWS = 3;
    constexpr int CONTROLS_ROW = 2;
    constexpr int FIRST_ROW_Y = -22;
    constexpr int ROW_SPACING = 18;
}

menu_scene::menu_scene(save_data& save, game_mode mode) :
    _save(save),
    _pitch(bn::regular_bg_items::pitch.create_bg(0, 0)),
    _big(common::variable_8x16_sprite_font),
    _small(common::variable_8x8_sprite_font),
    _row(int(mode))
{
    _big.set_center_alignment();
    _small.set_center_alignment();
    _big.set_z_order(draw_order::TEXT);
    _small.set_z_order(draw_order::TEXT);
    _redraw();
}

bool menu_scene::update()
{
    if(bn::keypad::up_pressed())
    {
        _row = (_row + ROWS - 1) % ROWS;
        _redraw();
    }
    else if(bn::keypad::down_pressed())
    {
        _row = (_row + 1) % ROWS;
        _redraw();
    }

    if(bn::keypad::left_pressed())
    {
        _change_scheme(-1);
    }
    else if(bn::keypad::right_pressed())
    {
        _change_scheme(1);
    }

    if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        if(_row == CONTROLS_ROW)
        {
            _change_scheme(1);
        }
        else
        {
            // Only write SRAM when something changed (it's slow-ish, and pointless).
            if(_scheme_changed)
            {
                store_save(_save);
            }

            return false;
        }
    }

    return true;
}

game_mode menu_scene::mode() const
{
    return game_mode(_row);
}

void menu_scene::_change_scheme(int delta)
{
    int scheme = (int(_save.scheme) + delta + control_scheme_count) % control_scheme_count;
    _save.scheme = control_scheme(scheme);
    _scheme_changed = true;
    _redraw();
}

void menu_scene::_redraw()
{
    _sprites.clear();
    _big.generate(0, -60, "CRICKET ADVANCE", _sprites);

    const char* labels[] = { "QUICK PLAY", "PRACTICE" };

    for(int row = 0; row < ROWS; ++row)
    {
        bn::string<32> text(row == _row ? "> " : "");

        if(row == CONTROLS_ROW)
        {
            text.append("CONTROLS: ");
            text.append(scheme_name(_save.scheme));
        }
        else
        {
            text.append(labels[row]);
        }

        text.append(row == _row ? " <" : "");
        _big.generate(0, FIRST_ROW_Y + row * ROW_SPACING, text, _sprites);
    }

    // The best Quick Play innings with these controls.
    bn::string<48> best("QUICK PLAY BEST (");
    best.append(scheme_name(_save.scheme));
    best.append("): ");
    {
        const best_score& score = _save.best[int(_save.scheme)];
        bn::ostringstream stream(best);

        if(score.runs >= 0)
        {
            stream << score.runs << " (" << score.balls << ")";
        }
        else
        {
            stream << "-";
        }
    }
    _small.generate(0, 42, best, _sprites);
    _small.generate(0, 66, "UP/DOWN:CHOOSE  L/R:CONTROLS  A:GO", _sprites);
}

}
