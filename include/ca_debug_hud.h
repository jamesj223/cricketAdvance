#ifndef CA_DEBUG_HUD_H
#define CA_DEBUG_HUD_H

// Prototype HUD: info lines (top-left), a big centred banner ("FOUR!") and help lines
// (bottom). Text is made of sprites, so it's only regenerated when something changes.

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
        void clear_lines();
        void set_banner(const bn::string_view& text);
        void set_help(const bn::string_view& line_1, const bn::string_view& line_2);

        // Call once per frame.
        void update();

    private:
        bn::sprite_text_generator _small;
        bn::sprite_text_generator _big;
        bn::string<48> _lines[LINES];
        bn::string<24> _banner;
        bn::string<48> _help[2];
        bn::vector<bn::sprite_ptr, 80> _sprites;    // ~5 letters per sprite
        bool _dirty = true;
    };
}

#endif
