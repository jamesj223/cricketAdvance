#ifndef CA_CONTROLS_H
#define CA_CONTROLS_H

// ARCHIVE BUILD: every control scheme tried during Phase 2, selectable with SELECT, for
// looking back and comparing. The main game keeps only Basic (V2) and Pro (V3).
// Each scheme ONLY maps buttons to a shot_intent; timing and outcomes don't know which
// scheme was used. Older schemes predate the 16-shot model, so they reach fewer shots.
//
//   1. SCHEME 1   Left/Right = off/leg side (no footwork: the game picks the better
//                 foot). A = hit, B = block. No loft. The first "simple" idea.
//   2. SCHEME 2   D-pad Up/Down = back/front foot (neither = front), L/R shoulders =
//                 off/leg side. A = hit, B = block. No loft.
//   3. BASIC V1   D-pad 8-way: Left/Right = side, Up = back foot, Down = front foot
//                 (neither = game picks). A = hit, B = block, hold R = loft.
//   4. PRO V1     Scheme 2 plus A+B together = lofted hit (waits up to 3 frames for
//                 the second button; the shot is timed from the first press).
//   5. PRO V2     D-pad aims (7 directions, Up = block). Hold L = front foot, R = back
//                 foot (neither/both = stuck on the crease: a notch worse). A = ground,
//                 B = lofted.
//   6. BASIC V2   D-pad aims, Up = block, A/B = hit, hold L/R = loft. The game picks the
//                 foot from the length. (Kept as "Basic".)
//   7. PRO V3     D-pad aims, Up = block. B = front foot, A = back foot, hold L/R =
//                 loft. (Kept as "Pro".)

#include "bn_optional.h"
#include "ca_shots.h"

namespace ca
{
    enum class control_scheme
    {
        SCHEME_1,
        SCHEME_2,
        BASIC_V1,
        PRO_V1,
        PRO_V2,
        BASIC_V2,
        PRO_V3,
    };

    constexpr int control_scheme_count = 7;

    [[nodiscard]] const char* scheme_name(control_scheme scheme);

    // One short line describing the controls, for the on-screen help.
    [[nodiscard]] const char* scheme_help(control_scheme scheme);

    struct shot_input
    {
        shot_intent intent;
        int frames_ago;     // how long ago the shot was actually pressed (Pro V1's chord)
    };

    // Reads shots from the keypad. Stateful only because of Pro V1's A+B chord: after the
    // first of A/B is pressed, it waits up to CHORD_FRAMES for the other one.
    class shot_reader
    {

    public:
        static constexpr int CHORD_FRAMES = 3;

        // Call once per frame while the ball is live.
        [[nodiscard]] bn::optional<shot_input> update(control_scheme scheme);

        // True while waiting to see whether a press becomes an A+B chord.
        [[nodiscard]] bool pending() const { return _pending; }

        void reset();

    private:
        shot_intent _intent;
        bool _pending = false;
        bool _first_was_attack = false;
        int _waited = 0;

        [[nodiscard]] bn::optional<shot_input> _update_pro_v1();
    };
}

#endif
