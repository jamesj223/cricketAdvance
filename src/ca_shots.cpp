#include "ca_shots.h"

#include "bn_math.h"
#include "bn_random.h"

namespace ca
{

namespace
{
    // =====================================================================================
    // Shot-vs-ball rating table. THE main tuning knob for batting.
    //
    // For each shot: a 4x4 grid. Rows = length (yorker, full, good, short),
    // columns = line (wide outside off, outside off, on the stumps, leg side).
    // 0 = wrong shot, 1 = risky, 2 = good choice, 3 = ideal.
    // =====================================================================================
    constexpr int rating_table[RATED_SHOT_COUNT][4][4] = {
        //          WIDE  OFF  STMP  LEG
        // FORWARD_DEFENCE
        {
            /* YORKER */ { 1,   2,   3,   2 },
            /* FULL   */ { 1,   3,   3,   2 },
            /* GOOD   */ { 1,   3,   3,   2 },
            /* SHORT  */ { 0,   1,   1,   0 },
        },
        // BACK_DEFENCE
        {
            /* YORKER */ { 0,   1,   1,   1 },
            /* FULL   */ { 0,   1,   1,   1 },
            /* GOOD   */ { 1,   2,   3,   2 },
            /* SHORT  */ { 1,   3,   3,   2 },
        },
        // COVER_DRIVE
        {
            /* YORKER */ { 1,   1,   0,   0 },
            /* FULL   */ { 3,   3,   1,   0 },
            /* GOOD   */ { 1,   2,   1,   0 },
            /* SHORT  */ { 0,   0,   0,   0 },
        },
        // STRAIGHT_DRIVE
        {
            /* YORKER */ { 0,   1,   2,   1 },
            /* FULL   */ { 0,   2,   3,   1 },
            /* GOOD   */ { 0,   1,   2,   1 },
            /* SHORT  */ { 0,   0,   0,   0 },
        },
        // ON_DRIVE
        {
            /* YORKER */ { 0,   0,   1,   2 },
            /* FULL   */ { 0,   0,   2,   3 },
            /* GOOD   */ { 0,   0,   1,   2 },
            /* SHORT  */ { 0,   0,   0,   1 },
        },
        // CUT
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 1,   0,   0,   0 },
            /* GOOD   */ { 2,   1,   0,   0 },
            /* SHORT  */ { 3,   3,   0,   0 },
        },
        // BACK_FOOT_PUNCH
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 0,   0,   0,   0 },
            /* GOOD   */ { 1,   1,   2,   1 },
            /* SHORT  */ { 1,   2,   2,   1 },
        },
        // PULL
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 0,   0,   0,   1 },
            /* GOOD   */ { 0,   0,   1,   2 },
            /* SHORT  */ { 0,   3,   3,   2 },   // too wide: cut it; down leg: hook it
        },
        // SQUARE_DRIVE: front foot to point; full and wide
        {
            /* YORKER */ { 1,   0,   0,   0 },
            /* FULL   */ { 3,   2,   0,   0 },
            /* GOOD   */ { 2,   1,   0,   0 },
            /* SHORT  */ { 0,   0,   0,   0 },
        },
        // GUIDE: front foot, run down to third man; outside off
        {
            /* YORKER */ { 0,   1,   0,   0 },
            /* FULL   */ { 1,   2,   0,   0 },
            /* GOOD   */ { 2,   2,   0,   0 },
            /* SHORT  */ { 1,   1,   0,   0 },
        },
        // LEG_GLANCE: front foot to fine leg; best on the pads / down leg, risky on the
        // stumps (playing across the line), useless outside off
        {
            /* YORKER */ { 0,   0,   1,   2 },
            /* FULL   */ { 0,   0,   1,   3 },
            /* GOOD   */ { 0,   0,   1,   3 },
            /* SHORT  */ { 0,   0,   0,   1 },
        },
        // CLIP: front foot to square leg; best on the pads, risky on the stumps,
        // useless outside off
        {
            /* YORKER */ { 0,   0,   1,   2 },
            /* FULL   */ { 0,   0,   1,   3 },
            /* GOOD   */ { 0,   0,   1,   2 },
            /* SHORT  */ { 0,   0,   0,   0 },
        },
        // BACK_FOOT_DRIVE: back foot through cover; short of a length outside off
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 0,   0,   0,   0 },
            /* GOOD   */ { 1,   2,   1,   0 },
            /* SHORT  */ { 2,   3,   0,   0 },
        },
        // LATE_CUT: back foot to third man; short and wide
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 0,   0,   0,   0 },
            /* GOOD   */ { 2,   1,   0,   0 },
            /* SHORT  */ { 3,   2,   0,   0 },
        },
        // HOOK: back foot to fine leg; short, at the body or down leg
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 0,   0,   0,   0 },
            /* GOOD   */ { 0,   0,   0,   1 },
            /* SHORT  */ { 0,   1,   2,   3 },
        },
        // SLOG: back foot, across the line to midwicket; agricultural but it works
        {
            /* YORKER */ { 0,   0,   0,   0 },
            /* FULL   */ { 0,   0,   1,   1 },
            /* GOOD   */ { 0,   0,   1,   1 },
            /* SHORT  */ { 0,   1,   2,   2 },
        },
    };

    // =====================================================================================
    // Timing windows, in frames either side of the ball reaching the bat, by rating.
    // |error| <= perfect -> PERFECT, <= good -> GOOD, <= ok -> EARLY/LATE (mistimed),
    // beyond -> swing and miss. -1 = that grade is impossible for this rating.
    // At 60 fps and GAME_SPEED 0.6, a pace ball travels ~0.33 m per frame.
    // =====================================================================================
    struct timing_window
    {
        int perfect;
        int good;
        int ok;
    };

    constexpr timing_window timing_windows[4] = {
        { -1, 1, 4 },   // 0 wrong shot
        {  0, 2, 5 },   // 1 risky
        {  1, 3, 6 },   // 2 good choice
        {  2, 4, 7 },   // 3 ideal
    };

    // Defensive shots are easier to time, especially early: getting the bat down too
    // soon rarely matters, being late does. Their windows are widened by this many frames.
    constexpr int DEFENCE_EARLY_BONUS = 6;
    constexpr int DEFENCE_LATE_BONUS = 2;

    // =====================================================================================
    // Where each shot sends the ball: angle in degrees from "straight back past the
    // bowler" (0). Negative = off side, positive = leg side, +/-180 = behind the batter.
    // =====================================================================================
    constexpr int shot_angles[RATED_SHOT_COUNT] = {
        0,      // FORWARD_DEFENCE
        0,      // BACK_DEFENCE
        -55,    // COVER_DRIVE
        0,      // STRAIGHT_DRIVE
        30,     // ON_DRIVE
        -100,   // CUT
        -10,    // BACK_FOOT_PUNCH
        95,     // PULL
        -80,    // SQUARE_DRIVE
        -140,   // GUIDE
        140,    // LEG_GLANCE
        75,     // CLIP
        -50,    // BACK_FOOT_DRIVE
        -130,   // LATE_CUT
        125,    // HOOK
        50,     // SLOG
    };

    constexpr int ANGLE_RANDOM = 12;        // +/- degrees of variety
    // Mistiming steers the ball: early = bat through too soon = squarer on the leg side;
    // late = finer on the off side.
    constexpr int EARLY_ANGLE_SHIFT = 25;
    constexpr int LATE_ANGLE_SHIFT = -25;
    // Edges that fall safe run away fine: third man / fine leg.
    constexpr int OFF_EDGE_ANGLE = -160;    // outside edge
    constexpr int LEG_EDGE_ANGLE = 155;     // inside / top edge
    // Edges that are caught go (almost) straight back to the keeper / slips.
    constexpr int OFF_CAUGHT_EDGE_ANGLE = -172;
    constexpr int LEG_CAUGHT_EDGE_ANGLE = 174;
    constexpr int CAUGHT_EDGE_RANDOM = 5;
    constexpr int BAT_PAD_ANGLE = 60;

    // =====================================================================================
    // Blocking risk. A block is safe on the stumps with good timing; the risk comes from
    // the line (reaching for wide ones) and from mistiming.
    // =====================================================================================
    // Blocking a wide ball: usually beaten outright; otherwise a thick edge.
    constexpr int BLOCK_WIDE_MISS_PERCENT = 50;
    constexpr int BLOCK_WIDE_EDGE_CAUGHT_PERCENT = 40;
    // Blocking outside off (the "5th stump" channel): chance of an edge, by timing grade
    // (TOO_EARLY, EARLY, GOOD, PERFECT, LATE, TOO_LATE).
    constexpr int BLOCK_OFF_EDGE_PERCENT[6] = { 0, 25, 10, 5, 25, 0 };
    constexpr int BLOCK_EDGE_CAUGHT_PERCENT = 50;
    // The wrong block for the ball (rating 0, e.g. forward to a short one): fending at it.
    constexpr int BLOCK_WRONG_SHOT_EDGE_PERCENT = 20;
    // Mistimed blocks: early = leading edge (return catch or bat-pad), late = thin edge.
    constexpr int BLOCK_EARLY_CAUGHT_PERCENT = 20;
    constexpr int BLOCK_LATE_CAUGHT_PERCENT = 20;

    bool chance(bn::random& random, int percent)
    {
        return random.get_int(100) < percent;
    }

    int timing_points(timing_grade grade)
    {
        switch(grade)
        {
            case timing_grade::PERFECT:
                return 3;
            case timing_grade::GOOD:
                return 2;
            case timing_grade::EARLY:
            case timing_grade::LATE:
                return 1;
            default:
                return 0;
        }
    }

    // Ball speed off the bat (horizontal) and upward speed, m/s, for each kind of hit.
    struct hit_profile
    {
        bn::fixed speed;
        bn::fixed up;
    };

    hit_profile hit_profile_for(shot_type shot, const shot_outcome& outcome)
    {
        switch(outcome.type)
        {
            case outcome_type::CAUGHT_AND_BOWLED:
                return { 9, bn::fixed(5.5) };      // lobs back ~7 m to the bowler
            case outcome_type::CAUGHT:
                if(is_defensive(shot))
                {
                    return { 5, 4 };                // bat-pad: pops up to a close fielder
                }
                return { 20, 11 };
            case outcome_type::FOUR:
                return { 32, 1 };
            case outcome_type::SIX:
                return { 30, 13 };
            case outcome_type::BLOCKED:
                return { 4, bn::fixed(0.5) };
            case outcome_type::EDGED:
                return { 18, 2 };
            case outcome_type::CAUGHT_BEHIND:
                // Fast and flat enough to carry ~14 m to a keeper standing back, still
                // around waist height (at 20 m/s it would land just short).
                return { 26, bn::fixed(3.5) };
            default:
                break;
        }

        // Plain runs: harder hit for more runs.
        switch(outcome.runs)
        {
            case 0:
                return { 8, bn::fixed(0.5) };
            case 1:
                return { 14, 1 };
            case 2:
                return { 20, 1 };
            default:
                return { 25, 1 };
        }
    }

    vec3 hit_velocity(shot_type shot, timing_grade grade, const shot_outcome& outcome,
                      bn::random& random)
    {
        int angle = shot_angles[int(shot)];

        if(grade == timing_grade::EARLY)
        {
            angle += EARLY_ANGLE_SHIFT;
        }
        else if(grade == timing_grade::LATE)
        {
            angle += LATE_ANGLE_SHIFT;
        }

        bool leg_side_shot = shot_angles[int(shot)] > 0;
        int spread = ANGLE_RANDOM;

        if(outcome.type == outcome_type::EDGED)
        {
            angle = leg_side_shot ? LEG_EDGE_ANGLE : OFF_EDGE_ANGLE;
        }
        else if(outcome.type == outcome_type::CAUGHT_BEHIND)
        {
            angle = leg_side_shot ? LEG_CAUGHT_EDGE_ANGLE : OFF_CAUGHT_EDGE_ANGLE;
            spread = CAUGHT_EDGE_RANDOM;
        }
        else if(outcome.type == outcome_type::CAUGHT_AND_BOWLED)
        {
            angle = 0;
            spread = CAUGHT_EDGE_RANDOM;
        }
        else if(outcome.type == outcome_type::CAUGHT && is_defensive(shot))
        {
            angle = random.get_int(2) ? BAT_PAD_ANGLE : -BAT_PAD_ANGLE;   // short leg / silly point
        }

        angle += random.get_int(-spread, spread + 1);

        hit_profile profile = hit_profile_for(shot, outcome);
        bn::fixed sin = bn::degrees_lut_sin_safe(angle);
        bn::fixed cos = bn::degrees_lut_sin_safe(angle + 90);

        // Angle 0 = back towards the bowler, i.e. -z.
        return { mul(profile.speed, sin), -mul(profile.speed, cos), profile.up };
    }
}

shot_type make_shot(bool attacking, footwork foot, shot_direction direction)
{
    if(! attacking)
    {
        return foot == footwork::FRONT ? shot_type::FORWARD_DEFENCE : shot_type::BACK_DEFENCE;
    }

    // [direction][front, back], in shot_direction order.
    constexpr shot_type shots[shot_direction_count][2] = {
        { shot_type::STRAIGHT_DRIVE, shot_type::BACK_FOOT_PUNCH },   // STRAIGHT
        { shot_type::COVER_DRIVE,    shot_type::BACK_FOOT_DRIVE },   // COVER
        { shot_type::SQUARE_DRIVE,   shot_type::CUT },               // POINT
        { shot_type::GUIDE,          shot_type::LATE_CUT },          // THIRD_MAN
        { shot_type::LEG_GLANCE,     shot_type::HOOK },              // FINE_LEG
        { shot_type::CLIP,           shot_type::PULL },              // SQUARE_LEG
        { shot_type::ON_DRIVE,       shot_type::SLOG },              // MIDWICKET
    };
    return shots[int(direction)][foot == footwork::FRONT ? 0 : 1];
}

footwork shot_footwork(shot_type shot)
{
    switch(shot)
    {
        case shot_type::BACK_DEFENCE:
        case shot_type::CUT:
        case shot_type::BACK_FOOT_PUNCH:
        case shot_type::PULL:
        case shot_type::BACK_FOOT_DRIVE:
        case shot_type::LATE_CUT:
        case shot_type::HOOK:
        case shot_type::SLOG:
            return footwork::BACK;
        default:
            return footwork::FRONT;
    }
}

bool is_defensive(shot_type shot)
{
    return shot == shot_type::FORWARD_DEFENCE || shot == shot_type::BACK_DEFENCE;
}

const char* shot_name(shot_type shot)
{
    constexpr const char* names[] = {
        "FORWARD DEFENCE", "BACK FOOT DEFENCE", "COVER DRIVE", "STRAIGHT DRIVE", "ON DRIVE",
        "CUT", "BACK FOOT PUNCH", "PULL", "SQUARE DRIVE", "GUIDE", "LEG GLANCE", "CLIP",
        "BACK FOOT DRIVE", "LATE CUT", "HOOK", "SLOG", "LEAVE",
    };
    return names[int(shot)];
}

line_zone classify_line(bn::fixed x)
{
    // Right-handed batter: off side is -x.
    if(x < bn::fixed(-0.6))
    {
        return line_zone::WIDE_OFF;
    }

    if(x < -(STUMP_HALF_WIDTH + BALL_RADIUS))
    {
        return line_zone::OFF;
    }

    if(x <= STUMP_HALF_WIDTH + BALL_RADIUS)
    {
        return line_zone::STUMPS;
    }

    return line_zone::LEG;
}

namespace
{
    // Length zone boundaries, metres in front of the stumps: [yorker | full | good | short].
    struct length_bands
    {
        bn::fixed full_from;
        bn::fixed good_from;
        bn::fixed short_from;
        bn::fixed back_foot_from;   // tie-break for auto footwork (see header)
    };

    constexpr length_bands pace_bands = { 2, 4, 6.5, 5.5 };
    constexpr length_bands spin_bands = { 1.25, 3, 4.75, 4 };

    const length_bands& bands_for(bowler_type type)
    {
        return type == bowler_type::SPIN ? spin_bands : pace_bands;
    }
}

length_zone classify_length(bn::fixed length, bowler_type type)
{
    const length_bands& bands = bands_for(type);

    if(length < bands.full_from)
    {
        return length_zone::YORKER;
    }

    if(length < bands.good_from)
    {
        return length_zone::FULL;
    }

    if(length < bands.short_from)
    {
        return length_zone::GOOD;
    }

    return length_zone::SHORT;
}

bn::fixed back_foot_from_length(bowler_type type)
{
    return bands_for(type).back_foot_from;
}

const char* line_zone_name(line_zone line)
{
    constexpr const char* names[] = { "WIDE OUTSIDE OFF", "OUTSIDE OFF", "ON THE STUMPS", "LEG SIDE" };
    return names[int(line)];
}

const char* length_zone_name(length_zone length)
{
    constexpr const char* names[] = { "YORKER", "FULL", "GOOD LENGTH", "SHORT" };
    return names[int(length)];
}

int shot_rating(shot_type shot, line_zone line, length_zone length)
{
    if(shot == shot_type::LEAVE)
    {
        return 0;
    }

    return rating_table[int(shot)][int(length)][int(line)];
}

const char* rating_name(int rating)
{
    constexpr const char* names[] = { "WRONG SHOT", "RISKY SHOT", "GOOD SHOT", "IDEAL SHOT" };
    return names[rating];
}

timing_grade grade_timing(int error, int rating, bool defensive)
{
    timing_window window = timing_windows[rating];

    if(defensive)
    {
        int bonus = error < 0 ? DEFENCE_EARLY_BONUS : DEFENCE_LATE_BONUS;
        window.perfect += bonus;
        window.good += bonus;
        window.ok += bonus;
    }

    int distance = bn::abs(error);

    if(distance <= window.perfect)
    {
        return timing_grade::PERFECT;
    }

    if(distance <= window.good)
    {
        return timing_grade::GOOD;
    }

    if(distance <= window.ok)
    {
        return error < 0 ? timing_grade::EARLY : timing_grade::LATE;
    }

    return error < 0 ? timing_grade::TOO_EARLY : timing_grade::TOO_LATE;
}

const char* timing_name(timing_grade grade)
{
    constexpr const char* names[] = { "WAY TOO EARLY", "EARLY", "GOOD TIMING", "PERFECT TIMING",
                                      "LATE", "WAY TOO LATE" };
    return names[int(grade)];
}

bool makes_contact(timing_grade grade)
{
    return grade != timing_grade::TOO_EARLY && grade != timing_grade::TOO_LATE;
}

shot_outcome resolve_shot(shot_type shot, bool lofted, timing_grade grade, int rating,
                          line_zone line, bn::random& random)
{
    shot_outcome outcome;

    if(! makes_contact(grade))
    {
        outcome.type = outcome_type::PLAYED_AND_MISSED;
        return outcome;
    }

    // Score 1..6: timing (1-3) + shot choice (0-3). The tables below turn it into a result.
    int score = timing_points(grade) + rating;

    auto runs = [&](int value)
    {
        outcome.type = value == 4 ? outcome_type::FOUR : outcome_type::RUNS;
        outcome.runs = value;
    };

    auto edge = [&](int caught_percent)
    {
        if(chance(random, caught_percent))
        {
            outcome.type = lofted ? outcome_type::CAUGHT : outcome_type::CAUGHT_BEHIND;
        }
        else
        {
            outcome.type = outcome_type::EDGED;
            outcome.runs = random.get_int(2);
        }
    };

    if(is_defensive(shot))
    {
        // Defence: safe on the stumps if timed well. Line and timing bring the risk.
        if(line == line_zone::WIDE_OFF)
        {
            if(chance(random, BLOCK_WIDE_MISS_PERCENT))
            {
                outcome.type = outcome_type::PLAYED_AND_MISSED;     // beaten: no contact
                return outcome;
            }

            edge(BLOCK_WIDE_EDGE_CAUGHT_PERCENT);
        }
        else
        {
            int edge_percent = line == line_zone::OFF ? BLOCK_OFF_EDGE_PERCENT[int(grade)] : 0;

            if(rating == 0)
            {
                edge_percent += BLOCK_WRONG_SHOT_EDGE_PERCENT;
            }

            if(chance(random, edge_percent))
            {
                edge(BLOCK_EDGE_CAUGHT_PERCENT);
            }
            else if(grade == timing_grade::EARLY && chance(random, BLOCK_EARLY_CAUGHT_PERCENT))
            {
                // Bat comes down too soon: leading edge, back to the bowler or bat-pad.
                outcome.type = chance(random, 50) ? outcome_type::CAUGHT_AND_BOWLED :
                                                    outcome_type::CAUGHT;
            }
            else if(grade == timing_grade::LATE && chance(random, BLOCK_LATE_CAUGHT_PERCENT))
            {
                outcome.type = outcome_type::CAUGHT_BEHIND;     // thin edge
            }
            else
            {
                outcome.type = outcome_type::BLOCKED;
            }
        }
    }
    else if(lofted)
    {
        // Lofted: bigger reward, bigger risk.
        if(score >= 6)
        {
            outcome.type = outcome_type::SIX;
            outcome.runs = 6;
        }
        else if(score == 5)
        {
            if(chance(random, 50))
            {
                outcome.type = outcome_type::SIX;
                outcome.runs = 6;
            }
            else
            {
                runs(4);
            }
        }
        else
        {
            // Score 4: 25% caught, else 2. Score 3: 50% caught, else 1. Lower: 75%, else 0.
            int caught_percent = score == 4 ? 25 : score == 3 ? 50 : 75;
            int runs_if_not_caught = score == 4 ? 2 : score == 3 ? 1 : 0;

            if(chance(random, caught_percent))
            {
                outcome.type = outcome_type::CAUGHT;
            }
            else
            {
                runs(runs_if_not_caught);
            }
        }
    }
    else
    {
        // Along the ground.
        if(score >= 6)
        {
            runs(4);
        }
        else if(score == 5)
        {
            runs(chance(random, 50) ? 4 : 2);
        }
        else if(score == 4)
        {
            runs(chance(random, 50) ? 2 : 1);
        }
        else if(score == 3)
        {
            runs(chance(random, 50) ? 1 : 0);
        }
        else if(score == 2)
        {
            edge(25);
        }
        else
        {
            edge(50);
        }
    }

    outcome.hit_velocity = hit_velocity(shot, grade, outcome, random);
    return outcome;
}

bool is_out(outcome_type type)
{
    return type == outcome_type::CAUGHT || type == outcome_type::CAUGHT_BEHIND ||
            type == outcome_type::CAUGHT_AND_BOWLED || type == outcome_type::BOWLED;
}

const char* outcome_banner(const shot_outcome& outcome)
{
    switch(outcome.type)
    {
        case outcome_type::FOUR:
            return "FOUR!";
        case outcome_type::SIX:
            return "SIX!";
        case outcome_type::BLOCKED:
            return "BLOCKED";
        case outcome_type::EDGED:
            return outcome.runs ? "EDGED - 1 RUN" : "EDGED";
        case outcome_type::CAUGHT:
            return "CAUGHT!";
        case outcome_type::CAUGHT_BEHIND:
            return "CAUGHT BEHIND!";
        case outcome_type::CAUGHT_AND_BOWLED:
            return "CAUGHT & BOWLED!";
        case outcome_type::PLAYED_AND_MISSED:
            return "PLAYED AND MISSED";
        case outcome_type::LEFT:
            return "LEFT ALONE";
        case outcome_type::BOWLED:
            return "BOWLED!";
        default:
            break;
    }

    constexpr const char* run_names[] = { "NO RUN", "1 RUN", "2 RUNS", "3 RUNS" };
    return run_names[bn::min(outcome.runs, 3)];
}

}
