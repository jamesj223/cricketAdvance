#ifndef CA_MENU_SCENE_H
#define CA_MENU_SCENE_H

// The title menu: pick a mode (Quick Play / Practice) and the control scheme, and see the
// best Quick Play score for that scheme.

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "ca_game_mode.h"
#include "ca_save.h"

namespace ca
{
    class menu_scene
    {

    public:
        menu_scene(save_data& save, game_mode mode);

        // Call once per frame. Returns false once a mode has been chosen.
        [[nodiscard]] bool update();

        [[nodiscard]] game_mode mode() const;

    private:
        save_data& _save;
        bn::regular_bg_ptr _pitch;
        bn::sprite_text_generator _big;
        bn::sprite_text_generator _small;
        bn::vector<bn::sprite_ptr, 48> _sprites;
        int _row;
        bool _scheme_changed = false;

        void _redraw();
        void _change_scheme(int delta);
    };
}

#endif
