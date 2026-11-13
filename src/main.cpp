// Cricket Advance.
//
// main() just starts Butano and runs the current scene. All game code lives in the
// ca_* files; see docs/learnings.md for a tour.

#include "bn_core.h"

#include "ca_practice_scene.h"

int main()
{
    bn::core::init();

    ca::practice_scene scene;

    while(true)
    {
        scene.update();

        // Waits for VBlank and commits sprite/background changes to the hardware.
        bn::core::update();
    }
}
