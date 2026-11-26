#ifndef CA_SAVE_H
#define CA_SAVE_H

// Save data, kept in the cartridge's battery-backed SRAM (32 KB; it survives power off).
//
// SRAM is just bytes: on a brand new cart (or emulator) it's full of garbage, usually
// 0xFF. So the saved block starts with a "magic" tag; if that doesn't match, the save is
// treated as empty. Changing the layout of save_data? Bump the tag, so an old save isn't
// misread as the new layout. (Butano also puts the text "SRAM_V113" in the ROM, which is
// how emulators and flash carts recognise that the game uses SRAM.)

#include "ca_controls.h"

namespace ca
{
    struct best_score
    {
        int runs = -1;      // -1 = no innings yet
        int balls = 0;
    };

    struct save_data
    {
        best_score best[control_scheme_count];  // Quick Play, per control scheme
        control_scheme scheme = control_scheme::BASIC;  // last used
    };

    [[nodiscard]] save_data load_save();

    void store_save(const save_data& data);

    // True (and updates data) if this innings beats the best for that scheme: more runs,
    // or the same runs off fewer balls. Ducks don't count.
    bool record_innings(save_data& data, control_scheme scheme, int runs, int balls);
}

#endif
