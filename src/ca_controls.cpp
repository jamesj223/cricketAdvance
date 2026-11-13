#include "ca_controls.h"

#include "bn_keypad.h"

namespace ca
{

namespace
{
    // The batter faces the camera, so stepping forward towards the bowler is DOWN the
    // screen and going back towards the stumps is UP. Empty if neither is held.
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

    // Screen left is the off side for a right-hander.
    shot_direction side(bool off, bool leg)
    {
        if(off == leg)
        {
            return shot_direction::STRAIGHT;
        }

        return off ? shot_direction::OFF : shot_direction::LEG;
    }
}

const char* scheme_name(control_scheme scheme)
{
    constexpr const char* names[] = {
        "BASIC CONTROLS",
        "PRO CONTROLS",
    };
    return names[int(scheme)];
}

const char* scheme_help(control_scheme scheme)
{
    constexpr const char* help[] = {
        // Keep these under ~230 px: the font averages ~6 px per character (spaces too).
        "PAD:FOOT+SIDE A:HIT B:BLOCK R:LOFT",
        "UP:BACK DN:FWD L/R:SIDE A+B:LOFT",
    };
    return help[int(scheme)];
}

bn::optional<shot_input> shot_reader::update(control_scheme scheme)
{
    return scheme == control_scheme::PRO ? _update_pro() : _update_basic();
}

void shot_reader::reset()
{
    _pending = false;
    _waited = 0;
}

bn::optional<shot_input> shot_reader::_update_basic()
{
    bool attack = bn::keypad::a_pressed();
    bool defend = bn::keypad::b_pressed();

    if(! attack && ! defend)
    {
        return bn::nullopt;
    }

    shot_intent intent;
    intent.attacking = attack;      // A wins if both are pressed on the same frame
    intent.direction = side(bn::keypad::left_held(), bn::keypad::right_held());
    intent.foot = dpad_footwork();  // neither held = the game picks
    intent.lofted = attack && bn::keypad::r_held();
    return shot_input{ intent, 0 };
}

bn::optional<shot_input> shot_reader::_update_pro()
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
        _intent.direction = side(bn::keypad::l_held(), bn::keypad::r_held());
        _intent.foot = dpad_footwork().value_or(footwork::FRONT);

        if(a && b)
        {
            _intent.attacking = true;
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
