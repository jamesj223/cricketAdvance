#ifndef CA_SCOREBOARD_H
#define CA_SCOREBOARD_H

// The TV-style scoreboard along the bottom of the screen.
//
// The strip itself is a second background layer (graphics/scoreboard.bmp: transparent
// except the bottom 14 rows). Background layers and sprites are drawn in order of their
// *priority* (0-3, lower = in front). The pitch and the game's sprites use the default, 3.
// The strip uses 2, so the ball and its shadow pass *under* it. Its text sprites use 1, so
// they're drawn on top of the strip. (At the same priority, sprites beat backgrounds.)

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"

namespace ca
{
    class scoreboard
    {

    public:
        scoreboard();

        // Quick Play: "23* (17)" (* = not out), the best score so far (best_runs < 0 =
        // none yet) and the strike rate.
        void set_quick_play(int runs, int balls, bool out, int best_runs);

        // Practice: "23/2 (17)" (runs/wickets, balls) and the strike rate.
        void set_practice(int runs, int wickets, int balls);

    private:
        bn::regular_bg_ptr _strip;
        bn::sprite_text_generator _text;
        bn::vector<bn::sprite_ptr, 16> _sprites;

        void _show(const bn::string_view& left, const bn::string_view& centre, int runs, int balls);
    };
}

#endif
