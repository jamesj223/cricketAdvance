#ifndef CA_SOUNDS_H
#define CA_SOUNDS_H

// Sound effects. The game logic says *what happened*; this decides how it sounds.
// The samples are placeholders made by tools/make_placeholder_sounds.py (audio/*.wav).
//
// Butano plays a sample with a volume (0..1), a speed (1 = normal; higher = higher pitch
// and shorter) and a panning (-1 left .. +1 right). Varying those lets one sample cover
// many situations, e.g. every bounce uses bounce.wav at a different speed.

#include "bn_fixed.h"
#include "ca_shots.h"

namespace ca::sounds
{
    // The ball hitting the ground. impact_speed: how fast it was coming down (m/s).
    // On the pitch (delivery), harder and shorter = deeper and louder; length is metres
    // in front of the stumps. x is the ball's sideways position, for panning.
    void pitch_bounce(bn::fixed impact_speed, bn::fixed length, bn::fixed x);

    // A bounce in the outfield after a shot (quieter; ignored if it's barely moving).
    void outfield_bounce(bn::fixed impact_speed, bn::fixed x);

    // Everything about a bat-on-ball contact that changes how it sounds.
    struct bat_contact
    {
        outcome_type outcome = outcome_type::BLOCKED;
        timing_grade grade = timing_grade::GOOD;
        shot_direction direction = shot_direction::STRAIGHT;
        bool defensive = false;
        bool lofted = false;
    };

    // Bat meets ball: a thin edge, a dull block, a mistimed tock, a solid crack, a mis-hit
    // that's going up to be caught, or (middled) a sound for the kind of shot.
    void bat(const bat_contact& contact);

    // The bat swishing through thin air: an attacking shot that missed.
    void whoosh();

    void stumps();

    // The crowd reacting: a cheer for 4s, 6s and wickets (wickets loudest).
    void crowd(outcome_type outcome);
}

#endif
