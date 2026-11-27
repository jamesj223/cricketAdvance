#ifndef CA_DEBUG_HUD_H
#define CA_DEBUG_HUD_H

// Prototype HUD: info lines, two event banners ("BOWLED!" / "GAME OVER") and a help line
// (bottom, above the scoreboard). Text is made of sprites, so it's only regenerated when
// something changes.
//
// The banners slide in, TV style (the first from the left, the second from the right),
// on a navy band that's drawn semi-transparent using the GBA's alpha blending hardware:
// each pixel of the band is mixed with whatever is behind it, for free.

#include "bn_fixed.h"
#include "bn_string.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"

namespace ca
{
    class debug_hud
    {

    public:
        static constexpr int LINES = 5;

        debug_hud();

        void set_line(int index, const bn::string_view& text);

        // Clears the info lines and both banners.
        void clear_lines();

        // Hides the info lines (e.g. over the field view); banners and help stay.
        void set_lines_visible(bool visible);

        void set_banner(const bn::string_view& text);

        // A second banner line under the first ("GAME OVER" under "BOWLED!").
        void set_sub_banner(const bn::string_view& text);

        void set_help(const bn::string_view& text);

        // Move both banner lines (first line's y; the second sits under it). Banners
        // already showing move too. reset_banner_y() puts them back in the default spot.
        void set_banner_y(int y);
        void reset_banner_y();
        [[nodiscard]] static int default_banner_y();
        static constexpr int BANNER_LINE_HEIGHT = 16;

        // Call once per frame.
        void update();

    private:
        struct banner
        {
            bn::string<24> text;
            bn::vector<bn::sprite_ptr, 16> sprites;     // the band, then the text
            bn::vector<bn::fixed, 16> base_x;           // where each sprite rests
            int offset = 0;                             // slide-in offset (pixels)
        };

        bn::sprite_text_generator _small;
        bn::sprite_text_generator _big;
        bn::string<48> _lines[LINES];
        bn::string<48> _help;
        bn::vector<bn::sprite_ptr, 80> _sprites;    // lines and help; ~5 letters per sprite
        banner _banners[2];
        int _banner_y;
        bool _lines_visible = true;
        bool _dirty = true;

        void _set_banner(int index, const bn::string_view& text);
    };
}

#endif
