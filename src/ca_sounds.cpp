#include "ca_sounds.h"

#include "bn_math.h"
#include "bn_sound_items.h"

#include "ca_world.h"

namespace ca::sounds
{

namespace
{
    // Panning: the ball is only ever ~1 m either side of the stumps on the pitch, so
    // scale that up (but never hard left/right, which sounds odd on a handheld).
    bn::fixed pan(bn::fixed x, bn::fixed metres_for_full)
    {
        return bn::clamp(div(x, metres_for_full), bn::fixed(-0.7), bn::fixed(0.7));
    }
}

void pitch_bounce(bn::fixed impact_speed, bn::fixed length, bn::fixed x)
{
    // Pace balls hit the pitch at ~7 m/s (full) to ~10 m/s (long hop); spinners at
    // ~6.3 m/s whatever the length (they come down at a similar angle). So the pitch
    // also takes a little from the length, so short spin balls still sound deeper.
    bn::fixed speed = bn::fixed(1.55) - mul(impact_speed, bn::fixed(0.08)) -
                      mul(length - 4, bn::fixed(0.03));
    speed = bn::clamp(speed, bn::fixed(0.6), bn::fixed(1.4));

    bn::fixed volume = bn::clamp(bn::fixed(0.4) + mul(impact_speed, bn::fixed(0.06)),
                                 bn::fixed(0.4), bn::fixed(1));

    bn::sound_items::bounce.play(volume, speed, pan(x, 1));
}

void outfield_bounce(bn::fixed impact_speed, bn::fixed x)
{
    if(impact_speed < 2)
    {
        return;     // rolling / tiny hops: silent
    }

    bn::fixed volume = bn::min(div(impact_speed, 20), bn::fixed(0.5));
    bn::sound_items::bounce.play(volume, bn::fixed(1.2), pan(x, 20));
}

namespace
{
    bool leg_side(shot_direction direction)
    {
        return direction == shot_direction::MIDWICKET || direction == shot_direction::SQUARE_LEG ||
               direction == shot_direction::FINE_LEG;
    }

    // A middled shot: each sound was matched by ear to the shot it suggested.
    void middled(const bat_contact& contact)
    {
        shot_direction direction = contact.direction;

        if(leg_side(direction))
        {
            if(contact.lofted)
            {
                bn::sound_items::thock.play(1);     // the big one over cow corner
            }
            else
            {
                bn::sound_items::thwack.play(1);    // punched along the ground
            }
        }
        else if(direction == shot_direction::POINT || direction == shot_direction::THIRD_MAN)
        {
            if(contact.lofted)
            {
                bn::sound_items::clean.play(1);
            }
            else
            {
                bn::sound_items::pock.play(1);      // cuts, guides
            }
        }
        else
        {
            bn::sound_items::clean.play(1);         // drives: so sweet it's barely there
        }
    }
}

void bat(const bat_contact& contact)
{
    switch(contact.outcome)
    {
        case outcome_type::EDGED:
        case outcome_type::CAUGHT_BEHIND:
            bn::sound_items::edge.play(bn::fixed(0.6), bn::fixed(1.8), 0);   // thin nick
            return;

        case outcome_type::BLOCKED:
        case outcome_type::CAUGHT_AND_BOWLED:
            bn::sound_items::bat.play(bn::fixed(0.6), bn::fixed(0.75), 0);   // dead bat
            return;

        default:
            break;
    }

    if(contact.defensive)
    {
        bn::sound_items::bat.play(bn::fixed(0.6), bn::fixed(0.75), 0);
        return;
    }

    if(contact.outcome == outcome_type::CAUGHT)
    {
        bn::sound_items::mishit.play(1);    // you can hear it's gone up off the wrong bit
        return;
    }

    switch(contact.grade)
    {
        case timing_grade::PERFECT:
            middled(contact);
            break;

        case timing_grade::GOOD:
            bn::sound_items::solid.play(bn::fixed(0.9));
            break;

        default:
            bn::sound_items::bat.play(bn::fixed(0.85));     // mistimed
            break;
    }
}

void whoosh()
{
    bn::sound_items::whoosh.play(bn::fixed(0.7));
}

void stumps()
{
    bn::sound_items::stumps.play(1);
}

void crowd(outcome_type outcome)
{
    switch(outcome)
    {
        // Boundaries get a polite cheer; wickets are the big moment.
        case outcome_type::SIX:
            bn::sound_items::crowd.play(bn::fixed(0.65));
            break;

        case outcome_type::FOUR:
            bn::sound_items::crowd.play(bn::fixed(0.45));
            break;

        case outcome_type::BOWLED:
        case outcome_type::CAUGHT:
        case outcome_type::CAUGHT_BEHIND:
        case outcome_type::CAUGHT_AND_BOWLED:
            // Slightly slower = a lower, more "ooh" roar for a wicket.
            bn::sound_items::crowd.play(1, bn::fixed(0.9), 0);
            break;

        default:
            break;
    }
}

}
