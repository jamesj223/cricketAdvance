#include "ca_controls.h"

#include "bn_keypad.h"

namespace ca
{

namespace
{
    // --- Basic V1 -------------------------------------------------------------------------

    // The batter faces the camera, so stepping forward towards the bowler is DOWN the
    // screen and going back towards the stumps is UP. Empty if neither is held.
    bn::optional<footwork> v1_footwork()
    {
        if(bn::keypad::up_held())
        {
            return footwork::BACK;
        }

        if(bn::keypad::down_held())
        {
            return footwork::FRONT;
        }

        return bn::nullopt;
    }

    bn::optional<shot_intent> read_basic_v1()
    {
        bool attack = bn::keypad::a_pressed();
        bool defend = bn::keypad::b_pressed();

        if(! attack && ! defend)
        {
            return bn::nullopt;
        }

        // Screen left is the off side for a right-hander. Off side = cover drive off the
        // front foot, cut off the back; leg side = on drive / pull.
        bool off = bn::keypad::left_held();
        bool leg = bn::keypad::right_held();

        shot_intent intent;
        intent.attacking = attack;      // A wins if both are pressed on the same frame

        if(off != leg)
        {
            intent.front_direction = off ? shot_direction::COVER : shot_direction::MIDWICKET;
            intent.back_direction = off ? shot_direction::POINT : shot_direction::SQUARE_LEG;
        }

        intent.foot = v1_footwork();    // neither held = the game picks
        intent.lofted = attack && bn::keypad::r_held();
        return intent;
    }

    // --- V2 (Basic and Pro) -----------------------------------------------------------------

    // The D-pad as a direction on the field. Empty for pure Up (= block).
    bn::optional<shot_direction> v2_direction()
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

    // Basic V2: A and B both play the shot; hold L or R = lofted.
    // Pro V2: A = along the ground, B = lofted (L/R are the feet).
    bn::optional<shot_intent> read_v2(bool manual_footwork)
    {
        bool a = bn::keypad::a_pressed();
        bool b = bn::keypad::b_pressed();

        if(! a && ! b)
        {
            return bn::nullopt;
        }

        bool lofted = manual_footwork ? b && ! a :      // both on the same frame = ground
                                        bn::keypad::l_held() || bn::keypad::r_held();

        shot_intent intent;
        bn::optional<shot_direction> direction = v2_direction();

        if(direction)
        {
            intent.front_direction = *direction;
            intent.back_direction = *direction;
            intent.lofted = lofted;
        }
        else
        {
            intent.attacking = false;   // Up = block (A or B)
        }

        if(manual_footwork)
        {
            bool front = bn::keypad::l_held();
            bool back = bn::keypad::r_held();

            if(front != back)
            {
                intent.foot = front ? footwork::FRONT : footwork::BACK;
            }
            else
            {
                intent.on_crease = true;    // the game picks, but it costs you
            }
        }

        return intent;
    }

    // --- Pro V3 -------------------------------------------------------------------------

    bn::optional<shot_intent> read_pro_v3()
    {
        bool front = bn::keypad::b_pressed();
        bool back = bn::keypad::a_pressed();

        if(! front && ! back)
        {
            return bn::nullopt;
        }

        shot_intent intent;
        bn::optional<shot_direction> direction = v2_direction();

        if(direction)
        {
            intent.front_direction = *direction;
            intent.back_direction = *direction;
            intent.lofted = bn::keypad::l_held() || bn::keypad::r_held();
        }
        else
        {
            intent.attacking = false;   // Up = block, off the foot you pressed
        }

        if(front != back)
        {
            intent.foot = front ? footwork::FRONT : footwork::BACK;
        }
        else
        {
            intent.on_crease = true;    // both at once
        }

        return intent;
    }
}

const char* scheme_name(control_scheme scheme)
{
    constexpr const char* names[] = {
        "BASIC V1 (OLD)",
        "BASIC V2",
        "PRO V2",
        "PRO V3",
    };
    static_assert(sizeof(names) / sizeof(names[0]) == control_scheme_count);
    return names[int(scheme)];
}

const char* scheme_help(control_scheme scheme)
{
    constexpr const char* help[] = {
        // Keep these under ~230 px: the font averages ~6 px per character (spaces too).
        "PAD:FOOT+SIDE A:HIT B:BLOCK R:LOFT",
        "PAD:AIM UP:BLOCK A/B:HIT L/R:LOFT",
        "PAD:AIM UP:BLOCK L:FWD R:BACK B:LOFT",
        "PAD:AIM UP:BLK B:FWD A:BACK L/R:LOFT",
    };
    static_assert(sizeof(help) / sizeof(help[0]) == control_scheme_count);
    return help[int(scheme)];
}

bn::optional<shot_intent> read_shot(control_scheme scheme)
{
    switch(scheme)
    {
        case control_scheme::BASIC_V1:
            return read_basic_v1();

        case control_scheme::PRO_V2:
            return read_v2(true);

        case control_scheme::PRO_V3:
            return read_pro_v3();

        default:
            return read_v2(false);
    }
}

}
