#include "ca_controls.h"

#include "bn_keypad.h"

namespace ca
{

namespace
{
    // --- Aiming (both schemes) -------------------------------------------------------------

    // The D-pad as a direction on the field. Empty for pure Up (= block).
    bn::optional<shot_direction> aim()
    {
        bool up = bn::keypad::up_held();
        bool down = bn::keypad::down_held();
        bool left = bn::keypad::left_held();
        bool right = bn::keypad::right_held();

        // Opposite directions cancel out.
        int x = int(right) - int(left);
        int y = int(down) - int(up);

        if(y < 0)
        {
            if(x < 0)
            {
                return shot_direction::THIRD_MAN;
            }

            if(x > 0)
            {
                return shot_direction::FINE_LEG;
            }

            return bn::nullopt;     // straight up: block
        }

        if(y > 0)
        {
            if(x < 0)
            {
                return shot_direction::COVER;
            }

            if(x > 0)
            {
                return shot_direction::MIDWICKET;
            }

            return shot_direction::STRAIGHT;
        }

        if(x < 0)
        {
            return shot_direction::POINT;
        }

        if(x > 0)
        {
            return shot_direction::SQUARE_LEG;
        }

        return shot_direction::STRAIGHT;   // no direction held
    }

    // --- Basic (V2 in Phase 2) ---------------------------------------------------------------------------

    // A or B plays the shot; hold L or R = lofted. The game picks the footwork.
    bn::optional<shot_intent> read_basic()
    {
        if(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed())
        {
            return bn::nullopt;
        }

        shot_intent intent;

        if(bn::optional<shot_direction> direction = aim())
        {
            intent.direction = *direction;
            intent.lofted = bn::keypad::l_held() || bn::keypad::r_held();
        }
        else
        {
            intent.attacking = false;   // Up = block
        }

        return intent;
    }

    // --- Pro (V3 in Phase 2) -------------------------------------------------------------------------

    bn::optional<shot_intent> read_pro()
    {
        bool front = bn::keypad::b_pressed();
        bool back = bn::keypad::a_pressed();

        if(! front && ! back)
        {
            return bn::nullopt;
        }

        shot_intent intent;
        bn::optional<shot_direction> direction = aim();

        if(direction)
        {
            intent.direction = *direction;
            intent.lofted = bn::keypad::l_held() || bn::keypad::r_held();
        }
        else
        {
            intent.attacking = false;   // Up = block, off the foot you pressed
        }

        // Both on the very same frame (rare): no way to tell which came first, so the
        // game picks the foot, as in Basic.
        if(front != back)
        {
            intent.foot = front ? footwork::FRONT : footwork::BACK;
        }

        return intent;
    }
}

const char* scheme_name(control_scheme scheme)
{
    constexpr const char* names[] = {
        "BASIC",
        "PRO",
    };
    static_assert(sizeof(names) / sizeof(names[0]) == control_scheme_count);
    return names[int(scheme)];
}

const char* scheme_help(control_scheme scheme)
{
    constexpr const char* help[] = {
        // Keep these under ~230 px: the font averages ~6 px per character (spaces too).
        "PAD:AIM UP:BLOCK A/B:HIT L/R:LOFT",
        "PAD:AIM UP:BLK B:FWD A:BACK L/R:LOFT",
    };
    static_assert(sizeof(help) / sizeof(help[0]) == control_scheme_count);
    return help[int(scheme)];
}

bn::optional<shot_intent> read_shot(control_scheme scheme)
{
    return scheme == control_scheme::PRO ? read_pro() : read_basic();
}

}
