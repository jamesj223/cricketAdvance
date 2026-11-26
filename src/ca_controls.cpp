#include "ca_controls.h"

#include "bn_keypad.h"

namespace ca
{

namespace
{
    // --- Old 3-way side (schemes 1, 2, Basic V1, Pro V1) ----------------------------------

    // Off side = cover drive off the front foot, cut off the back; leg side = on drive /
    // pull; neither (or both) = straight.
    void set_side(shot_intent& intent, bool off, bool leg)
    {
        if(off == leg)
        {
            return;     // straight (the default)
        }

        intent.direction = off ? shot_direction::COVER : shot_direction::MIDWICKET;
        intent.back_direction = off ? shot_direction::POINT : shot_direction::SQUARE_LEG;
    }

    // Up = back foot, Down = front foot (the batter faces the camera). Empty if neither.
    bn::optional<footwork> dpad_footwork()
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

    bn::optional<shot_intent> read_scheme_1()
    {
        bool attack = bn::keypad::a_pressed();

        if(! attack && ! bn::keypad::b_pressed())
        {
            return bn::nullopt;
        }

        shot_intent intent;
        intent.attacking = attack;
        set_side(intent, bn::keypad::left_held(), bn::keypad::right_held());
        intent.best_footwork = true;
        return intent;
    }

    bn::optional<shot_intent> read_scheme_2()
    {
        bool attack = bn::keypad::a_pressed();

        if(! attack && ! bn::keypad::b_pressed())
        {
            return bn::nullopt;
        }

        shot_intent intent;
        intent.attacking = attack;
        set_side(intent, bn::keypad::l_held(), bn::keypad::r_held());
        intent.foot = dpad_footwork().value_or(footwork::FRONT);
        return intent;
    }

    bn::optional<shot_intent> read_basic_v1()
    {
        bool attack = bn::keypad::a_pressed();

        if(! attack && ! bn::keypad::b_pressed())
        {
            return bn::nullopt;
        }

        shot_intent intent;
        intent.attacking = attack;      // A wins if both are pressed on the same frame
        set_side(intent, bn::keypad::left_held(), bn::keypad::right_held());
        intent.foot = dpad_footwork();  // neither held = the game picks (the better foot)
        intent.best_footwork = true;
        intent.lofted = attack && bn::keypad::r_held();
        return intent;
    }

    // --- Aiming (Pro V2, Basic V2, Pro V3) -------------------------------------------------

    // The D-pad as a direction on the field. Empty for pure Up (= block).
    bn::optional<shot_direction> aim()
    {
        int x = int(bn::keypad::right_held()) - int(bn::keypad::left_held());
        int y = int(bn::keypad::down_held()) - int(bn::keypad::up_held());

        if(y < 0)
        {
            if(x == 0)
            {
                return bn::nullopt;     // straight up: block
            }

            return x < 0 ? shot_direction::THIRD_MAN : shot_direction::FINE_LEG;
        }

        if(y > 0 && x != 0)
        {
            return x < 0 ? shot_direction::COVER : shot_direction::MIDWICKET;
        }

        if(x != 0)
        {
            return x < 0 ? shot_direction::POINT : shot_direction::SQUARE_LEG;
        }

        return shot_direction::STRAIGHT;
    }

    // Fills in aim (or block, for Up) and loft.
    void apply_aim(shot_intent& intent, bool lofted)
    {
        if(bn::optional<shot_direction> direction = aim())
        {
            intent.direction = *direction;
            intent.lofted = lofted;
        }
        else
        {
            intent.attacking = false;
        }
    }

    bn::optional<shot_intent> read_pro_v2()
    {
        bool a = bn::keypad::a_pressed();
        bool b = bn::keypad::b_pressed();

        if(! a && ! b)
        {
            return bn::nullopt;
        }

        shot_intent intent;
        apply_aim(intent, b && ! a);    // B = lofted (both on one frame = ground)

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

        return intent;
    }

    bn::optional<shot_intent> read_basic_v2()
    {
        if(! bn::keypad::a_pressed() && ! bn::keypad::b_pressed())
        {
            return bn::nullopt;
        }

        shot_intent intent;
        apply_aim(intent, bn::keypad::l_held() || bn::keypad::r_held());
        return intent;
    }

    bn::optional<shot_intent> read_pro_v3()
    {
        bool front = bn::keypad::b_pressed();
        bool back = bn::keypad::a_pressed();

        if(! front && ! back)
        {
            return bn::nullopt;
        }

        shot_intent intent;
        apply_aim(intent, bn::keypad::l_held() || bn::keypad::r_held());

        if(front != back)
        {
            intent.foot = front ? footwork::FRONT : footwork::BACK;
        }

        return intent;
    }

    bn::optional<shot_input> now(bn::optional<shot_intent> intent)
    {
        if(! intent)
        {
            return bn::nullopt;
        }

        return shot_input{ *intent, 0 };
    }
}

const char* scheme_name(control_scheme scheme)
{
    constexpr const char* names[] = {
        "1: SCHEME 1 (SIMPLE)",
        "2: SCHEME 2",
        "3: BASIC V1",
        "4: PRO V1 (A+B LOFT)",
        "5: PRO V2",
        "6: BASIC V2 (KEPT)",
        "7: PRO V3 (KEPT)",
    };
    static_assert(sizeof(names) / sizeof(names[0]) == control_scheme_count);
    return names[int(scheme)];
}

const char* scheme_help(control_scheme scheme)
{
    constexpr const char* help[] = {
        // Keep these under ~230 px: the font averages ~6 px per character (spaces too).
        "L/R:SIDE A:HIT B:BLOCK (AUTO FEET)",
        "UP:BACK DN:FWD L/R:SIDE A:HIT B:BLK",
        "PAD:FOOT+SIDE A:HIT B:BLOCK R:LOFT",
        "UP:BACK DN:FWD L/R:SIDE A+B:LOFT",
        "PAD:AIM UP:BLK L:FWD R:BACK B:LOFT",
        "PAD:AIM UP:BLOCK A/B:HIT L/R:LOFT",
        "PAD:AIM UP:BLK B:FWD A:BACK L/R:LOFT",
    };
    static_assert(sizeof(help) / sizeof(help[0]) == control_scheme_count);
    return help[int(scheme)];
}

bn::optional<shot_input> shot_reader::update(control_scheme scheme)
{
    switch(scheme)
    {
        case control_scheme::SCHEME_1:
            return now(read_scheme_1());
        case control_scheme::SCHEME_2:
            return now(read_scheme_2());
        case control_scheme::BASIC_V1:
            return now(read_basic_v1());
        case control_scheme::PRO_V1:
            return _update_pro_v1();
        case control_scheme::PRO_V2:
            return now(read_pro_v2());
        case control_scheme::BASIC_V2:
            return now(read_basic_v2());
        default:
            return now(read_pro_v3());
    }
}

void shot_reader::reset()
{
    _pending = false;
    _waited = 0;
}

bn::optional<shot_input> shot_reader::_update_pro_v1()
{
    bool a = bn::keypad::a_pressed();
    bool b = bn::keypad::b_pressed();

    if(! _pending)
    {
        if(! a && ! b)
        {
            return bn::nullopt;
        }

        // Direction and footwork are read at the moment of the first press.
        _intent = shot_intent();
        set_side(_intent, bn::keypad::l_held(), bn::keypad::r_held());
        _intent.foot = dpad_footwork().value_or(footwork::FRONT);

        if(a && b)
        {
            _intent.lofted = true;
            return shot_input{ _intent, 0 };
        }

        _pending = true;
        _first_was_attack = a;
        _waited = 0;
        return bn::nullopt;
    }

    // Waiting to see if the other button follows.
    ++_waited;
    bool other = _first_was_attack ? b : a;

    if(other || _waited >= CHORD_FRAMES)
    {
        _pending = false;
        _intent.attacking = other || _first_was_attack;
        _intent.lofted = other;
        return shot_input{ _intent, _waited };
    }

    return bn::nullopt;
}

}
