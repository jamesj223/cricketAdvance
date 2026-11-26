#include "ca_scoreboard.h"

#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_scoreboard.h"
#include "common_variable_8x8_sprite_font.h"

#include "ca_draw_order.h"

namespace ca
{

namespace
{
    constexpr int STRIP_PRIORITY = 2;
    constexpr int TEXT_PRIORITY = 1;
    constexpr int TEXT_Y = 73;      // middle of the strip (screen y 66..80)
    constexpr int TEXT_X = 112;     // inset from the screen edges (120)
}

scoreboard::scoreboard() :
    _strip(bn::regular_bg_items::scoreboard.create_bg(0, 0)),
    _text(common::variable_8x8_sprite_font)
{
    _strip.set_priority(STRIP_PRIORITY);
    _text.set_bg_priority(TEXT_PRIORITY);
    _text.set_z_order(draw_order::TEXT);
}

void scoreboard::set_quick_play(int runs, int balls, bool out, int best_runs)
{
    // Runs, a * while not out (as on a real scorecard), then balls faced.
    bn::string<32> score;
    {
        bn::ostringstream stream(score);
        stream << runs << (out ? "" : "*") << " (" << balls << ")";
    }

    bn::string<32> best;

    if(best_runs >= 0)
    {
        bn::ostringstream stream(best);
        stream << "BEST " << best_runs;
    }

    _show(score, best, runs, balls);
}

void scoreboard::set_practice(int runs, int wickets, int balls)
{
    bn::string<32> score;
    {
        bn::ostringstream stream(score);
        stream << runs << "/" << wickets << " (" << balls << ")";
    }

    _show(score, "PRACTICE", runs, balls);
}

void scoreboard::_show(const bn::string_view& left, const bn::string_view& centre, int runs, int balls)
{
    _sprites.clear();

    // Strike rate = runs per 100 balls, to one decimal place. Integer maths:
    // tenths = runs * 1000 / balls, then split into whole and tenths for printing.
    bn::string<32> rate("SR ");
    {
        bn::ostringstream stream(rate);

        if(balls > 0)
        {
            int tenths = runs * 1000 / balls;
            stream << tenths / 10 << "." << tenths % 10;
        }
        else
        {
            stream << "-";
        }
    }

    _text.set_left_alignment();
    _text.generate(-TEXT_X, TEXT_Y, left, _sprites);
    _text.set_center_alignment();
    _text.generate(0, TEXT_Y, centre, _sprites);
    _text.set_right_alignment();
    _text.generate(TEXT_X, TEXT_Y, rate, _sprites);
}

}
