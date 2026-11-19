#ifndef CA_CONTROLS_H
#define CA_CONTROLS_H

// Control schemes. Each one ONLY maps buttons to a shot_intent; the timing and outcome
// logic never knows which scheme was used. Adding or removing a scheme touches nothing
// else. See docs/design.md ("Controls").
//
//   BASIC V1: D-pad 8-way. Left/Right = off/leg side, Up = back foot, Down = front foot
//             (neither = the game picks the footwork). A = hit, B = block, hold R = loft.
//   BASIC V2: D-pad = where to hit it, as seen on screen (down = straight, down-left =
//             cover, left = point, up-left = third man, up-right = fine leg, right =
//             square leg, down-right = midwicket). Up = block. A or B = play the shot,
//             hold L or R = lofted. The game picks front or back foot (the better one).
//             (Loft on the shoulders matches Pro V3.)
//   PRO V2:   Basic V2's aim, but you pick the footwork: hold L = front, R = back foot.
//             A = along the ground, B = lofted.
//             Neither or both = stuck on the crease (game picks, shot a notch worse).
//   PRO V3:   D-pad aim as V2 (Up = block). The button picks the foot AND times the shot:
//             B = front foot, A = back foot (B sits lower on the GBA = forward, down the
//             screen). Hold L or R = lofted. A+B together = stuck on the crease.

#include "bn_optional.h"
#include "ca_shots.h"

namespace ca
{
    enum class control_scheme
    {
        BASIC_V1,
        BASIC_V2,
        PRO_V2,
        PRO_V3,
    };

    constexpr int control_scheme_count = 4;

    [[nodiscard]] const char* scheme_name(control_scheme scheme);

    // One short line describing the controls, for the on-screen help.
    [[nodiscard]] const char* scheme_help(control_scheme scheme);

    // Reads a shot from the keypad: call once per frame while the ball is live. Returns
    // the shot on the frame a shot button is pressed.
    [[nodiscard]] bn::optional<shot_intent> read_shot(control_scheme scheme);
}

#endif
