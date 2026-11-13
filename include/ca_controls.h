#ifndef CA_CONTROLS_H
#define CA_CONTROLS_H

// Control schemes. Each one ONLY maps buttons to a shot_intent; the timing and outcome
// logic never knows which scheme was used. Adding or removing a scheme touches nothing
// else. See docs/design.md ("Controls").
//
//   BASIC: D-pad 8-way. Left/Right = off/leg side, Up = back foot, Down = front foot
//          (neither = the game picks the footwork). A = hit, B = block, hold R = loft.
//   PRO:   Up/Down = back/front foot (neither = front), L/R = off/leg side
//          (neither or both = straight). A = hit, B = block, A+B together = lofted hit.

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

    struct shot_input
    {
        shot_intent intent;
        int frames_ago;     // how long ago the shot was actually pressed (see below)
    };

    // Reads shots from the keypad. Stateful because of PRO's A+B chord: after the first
    // of A/B is pressed, it waits up to CHORD_FRAMES for the other one before deciding.
    // The shot is still timed from the *first* press (frames_ago says how long ago).
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

        [[nodiscard]] bn::optional<shot_input> _update_basic();
        [[nodiscard]] bn::optional<shot_input> _update_pro();
    };
}

#endif
