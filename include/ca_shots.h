#ifndef CA_SHOTS_H
#define CA_SHOTS_H

// Batting: shot types, ball zones, the shot-vs-ball rating table, timing and outcomes.
//
// Pipeline for one delivery:
//   1. When the ball is released, it's classified into a zone (line x length).
//   2. The control scheme turns button presses into a shot_intent.
//   3. The intent becomes a shot_type; the rating table says how suitable that shot is
//      for this zone (0 = wrong .. 3 = ideal). The rating sets the timing windows.
//   4. Timing error (frames between the press and the ball reaching the bat) is graded.
//   5. Grade + rating -> outcome (runs, edge, caught, missed ...).
//
// Everything here is pure logic (no sprites), and all tuning numbers live in
// src/ca_shots.cpp.

#include "bn_optional.h"
#include "ca_world.h"

namespace bn
{
    class random;
}

namespace ca
{
    // --- Shots ------------------------------------------------------------------------
    enum class footwork
    {
        FRONT,
        BACK,
    };

    // Where the batter aims, as seen on screen. The batter faces the camera, so the D-pad
    // maps straight onto the field: down = back past the bowler, left = off side, etc.
    // (Up, straight behind, has no shot: Basic/Pro V2 use it for blocking.)
    enum class shot_direction
    {
        STRAIGHT,       // down (or no direction)
        COVER,          // down-left
        POINT,          // left
        THIRD_MAN,      // up-left
        FINE_LEG,       // up-right
        SQUARE_LEG,     // right
        MIDWICKET,      // down-right
    };

    constexpr int shot_direction_count = 7;

    // Every shot is a direction played off the front or back foot (plus the two blocks).
    // The first eight are the original Phase 2 shots; their order matches older tables.
    enum class shot_type
    {
        FORWARD_DEFENCE,
        BACK_DEFENCE,
        COVER_DRIVE,        // front foot, cover
        STRAIGHT_DRIVE,     // front foot, straight
        ON_DRIVE,           // front foot, midwicket
        CUT,                // back foot, point
        BACK_FOOT_PUNCH,    // back foot, straight
        PULL,               // back foot, square leg
        SQUARE_DRIVE,       // front foot, point
        GUIDE,              // front foot, third man
        LEG_GLANCE,         // front foot, fine leg
        FLICK,              // front foot, square leg
        BACK_FOOT_DRIVE,    // back foot, cover
        LATE_CUT,           // back foot, third man
        HOOK,               // back foot, fine leg
        SLOG,               // back foot, midwicket
        LEAVE,
    };

    constexpr int RATED_SHOT_COUNT = 16;    // every shot except LEAVE

    // What the player asked for.
    //  - direction can differ by foot: Basic V1's "off side" means a cover drive off the
    //    front foot but a cut off the back foot. V2 schemes set both the same.
    //  - foot is empty when the game picks the footwork (whichever suits the ball better).
    //  - on_crease: Pro V2 with neither (or both) feet chosen. The game picks the better
    //    foot, but the shot is a notch worse: you were stuck on the crease.
    struct shot_intent
    {
        bool attacking = true;
        shot_direction front_direction = shot_direction::STRAIGHT;
        shot_direction back_direction = shot_direction::STRAIGHT;
        bn::optional<footwork> foot;
        bool lofted = false;
        bool on_crease = false;

        [[nodiscard]] shot_direction direction(footwork for_foot) const
        {
            return for_foot == footwork::FRONT ? front_direction : back_direction;
        }
    };

    [[nodiscard]] shot_type make_shot(bool attacking, footwork foot, shot_direction direction);
    [[nodiscard]] footwork shot_footwork(shot_type shot);
    [[nodiscard]] bool is_defensive(shot_type shot);
    [[nodiscard]] const char* shot_name(shot_type shot);

    // --- Zones ------------------------------------------------------------------------
    enum class line_zone
    {
        WIDE_OFF,
        OFF,
        STUMPS,
        LEG,
    };

    enum class length_zone
    {
        YORKER,
        FULL,
        GOOD,
        SHORT,
    };

    [[nodiscard]] line_zone classify_line(bn::fixed x_at_crease);
    [[nodiscard]] length_zone classify_length(bn::fixed length_from_stumps);
    [[nodiscard]] const char* line_zone_name(line_zone line);
    [[nodiscard]] const char* length_zone_name(length_zone length);

    // --- Rating -----------------------------------------------------------------------
    // 0 = wrong shot, 1 = risky, 2 = good choice, 3 = ideal.
    [[nodiscard]] int shot_rating(shot_type shot, line_zone line, length_zone length);
    [[nodiscard]] const char* rating_name(int rating);

    // --- Timing -----------------------------------------------------------------------
    enum class timing_grade
    {
        TOO_EARLY,  // swing and miss
        EARLY,      // made contact, mistimed
        GOOD,
        PERFECT,
        LATE,       // made contact, mistimed
        TOO_LATE,   // swing and miss
    };

    // error_frames: press frame - frame the ball reaches the bat (negative = early).
    // (Being late on a ball that's hitting the stumps is handled by the ball itself: if it
    // reaches the stumps before you press, you're bowled.)
    [[nodiscard]] timing_grade grade_timing(int error_frames, int rating, bool defensive);
    [[nodiscard]] const char* timing_name(timing_grade grade);
    [[nodiscard]] bool makes_contact(timing_grade grade);

    // --- Outcome ----------------------------------------------------------------------
    enum class outcome_type
    {
        RUNS,           // 0-3 runs off the bat
        FOUR,
        SIX,
        BLOCKED,
        EDGED,          // edge that falls safe (0-1 runs)
        CAUGHT,         // skied / lofted catch
        CAUGHT_BEHIND,  // edge carried to the keeper / slips
        PLAYED_AND_MISSED,
        LEFT,
        BOWLED,
    };

    struct shot_outcome
    {
        outcome_type type;
        int runs = 0;
        vec3 hit_velocity;      // m/s; only meaningful if the bat made contact
    };

    // Decides the outcome of a shot that made contact (or missed: TOO_EARLY/TOO_LATE).
    [[nodiscard]] shot_outcome resolve_shot(shot_type shot, bool lofted, timing_grade grade,
                                            int rating, bn::random& random);

    [[nodiscard]] bool is_out(outcome_type type);
    [[nodiscard]] const char* outcome_banner(const shot_outcome& outcome);
}

#endif
