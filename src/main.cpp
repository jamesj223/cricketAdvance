// Cricket Advance.
//
// main() starts Butano, loads the save, then alternates between the menu and a batting
// scene. Each scene lives in its own { } block, so its destructor frees its sprites and
// backgrounds (the GBA's limited video memory) before the next scene creates its own.
// All game code lives in the ca_* files; see docs/learnings.md for a tour.

#include "bn_core.h"

#include "ca_batting_scene.h"
#include "ca_menu_scene.h"
#include "ca_save.h"

int main()
{
    bn::core::init();

    ca::save_data save = ca::load_save();
    ca::game_mode mode = ca::game_mode::QUICK_PLAY;

    while(true)
    {
        {
            ca::menu_scene menu(save, mode);

            while(menu.update())
            {
                // Waits for VBlank and commits sprite/background changes to the hardware.
                bn::core::update();
            }

            mode = menu.mode();
        }

        bn::core::update();

        {
            ca::batting_scene scene(mode, save);

            while(scene.update())
            {
                bn::core::update();
            }
        }

        bn::core::update();
    }
}
