#ifndef CA_CONTROLS_H
#define CA_CONTROLS_H

// Control schemes. Each one ONLY maps buttons to a shot_intent; the timing and outcome
// logic never knows which scheme was used. See docs/design.md ("Controls").
//
// Both aim with the D-pad, which maps onto the field as seen on screen (the batter faces
// the camera): down = straight, down-left = cover, left = point, up-left = third man,
// up-right = fine leg, right = square leg, down-right = midwicket. Up = block.
// Hold L or R = lofted.
//
//   BASIC (V2): A or B plays the shot. The game picks front or back foot from the length
//               (front to full balls, back to short), not always the best-rated one.
//   PRO (V3):   the button picks the foot AND times the shot: B = front foot, A = back
//               foot (B sits lower on the GBA = forward, down the screen). A and B on
//               the same frame: the game picks the foot, as in Basic.
//
// Older schemes tried in Phase 2 are in git history: Pro V1 (A+B chord loft) in 7f6cdff,
// Basic V1 and Pro V2 in 2900f36.

#include "bn_optional.h"
#include "ca_shots.h"

namespace ca
{
    enum class control_scheme
    {
        BASIC,
        PRO,
    };

    constexpr int control_scheme_count = 2;

    [[nodiscard]] const char* scheme_name(control_scheme scheme);

    // One short line describing the controls, for the on-screen help.
    [[nodiscard]] const char* scheme_help(control_scheme scheme);

    // Reads a shot from the keypad: call once per frame while the ball is live. Returns
    // the shot on the frame a shot button is pressed.
    [[nodiscard]] bn::optional<shot_intent> read_shot(control_scheme scheme);
}

#endif
