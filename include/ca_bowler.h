#ifndef CA_BOWLER_H
#define CA_BOWLER_H

// Bowler profiles: pure data. Every delivery is generated from one of these, so adding
// bowler variety (swing vs seam, left-arm, leg spin, difficulty) is just more data.
//
// Conventions (see include/ca_world.h):
//   - Distances in metres, speeds in km/h (real-world values; GAME_SPEED slows play down).
//   - x is across the pitch as seen on screen. All bowlers currently bowl to a
//     RIGHT-HANDED batter, so:   -x = off side,   +x = leg side.
//   - Sideways movement is signed the same way: + moves the ball towards the leg side
//     (into a right-hander: inswing, off-break), - moves it towards the off side
//     (away from a right-hander: outswing, leg-break).
//   - Randomness: each "..._variation" / "..._spread" value is the most a delivery can
//     differ from the base value, either way. A value of 0 means "always exactly the base".

#include "bn_fixed.h"

namespace ca
{
    enum class bowler_type
    {
        PACE,
        SPIN,
    };

    enum class bowling_arm
    {
        RIGHT,
        LEFT,
    };

    // Which side of the stumps the bowler runs past. Over the wicket = bowling arm
    // next to the stumps; round the wicket = the other side, wider, angling in more.
    enum class wicket_side
    {
        OVER,
        ROUND,
    };

    struct bowler_profile
    {
        const char* name;       // display only; doesn't affect how they bowl
        bowler_type type;
        bowling_arm arm;
        wicket_side side;

        // --- Speed ----------------------------------------------------------------------
        // Release speed, picked uniformly in [min, max]. Rough real-world guide:
        // express 145+, fast 135-145, fast-medium 125-135, medium 110-125, spin 75-95.
        // Width of the range (max - min): a consistent pace bowler's normal deliveries
        // vary by ~6-10 km/h; a spinner who varies their pace on purpose ~8-15 km/h.
        // Keep it narrower for a metronomic bowler. (Deliberate slower balls / bouncers
        // will be separate variations later, not part of this range.)
        int speed_min_kmh;
        int speed_max_kmh;

        // --- Release point --------------------------------------------------------------
        // Distance of the hand from the middle stump at release (always positive).
        // Which side it's on comes from `arm` and `side` (see release_x() below).
        // Guide: over the wicket, spinners 0.3-0.4, pace 0.4-0.5.
        // KNOWN LIMITATION: round the wicket the hand is about a shoulder-width (~0.25 m)
        // further out (0.55-0.7), because the body is between the arm and the stumps.
        // That is NOT derived: if you change `side`, change this too. Fine while each
        // bowler's side is fixed; revisit if bowlers can switch sides mid-spell (e.g.
        // store the over-the-wicket width and add the shoulder offset in release_x()).
        bn::fixed release_width;
        // Height of the ball at release: roughly the bowler's height + arm, 2.0-2.4 m.
        // Taller = steeper angle into the pitch = more bounce.
        bn::fixed release_h;

        // --- Accuracy -------------------------------------------------------------------
        // Length: how far in front of the batter's stumps the ball bounces, metres.
        //   pace: yorker 0-2   full 2-4      good 4-6.5    short 6.5+ (long hop 8+)
        //   spin: yorker 0-1.25 full 1.25-3  good 3-4.75   short 4.75+
        // (These are the zones the shot ratings use: classify_length in ca_shots.cpp.)
        // Swing bowlers pitch it up (full) to give the ball time to swing; spinners
        // bowl fuller than seamers because the ball is slower.
        bn::fixed target_length;
        // Most lengths land within about half of this either side; never beyond it.
        bn::fixed length_spread;
        // Line: x where the ball bounces, metres. Right-handed batter: -x = off side.
        //   wide outside off  < -0.6   (beyond about -0.9 would be a wide)
        //   outside off       -0.6 to -0.15
        //     "corridor of uncertainty" (4th-5th stump) about -0.2 to -0.35
        //   off stump  -0.114   middle  0   leg stump  +0.114   (stumps span -0.15..+0.15
        //   counting the ball's radius: anything in there is "on the stumps")
        //   down leg          > +0.15  (beyond about +0.9 would be a wide)
        // These are the zones the shot ratings use (classify_line, which judges where the
        // ball passes the batter). Movement after the bounce (seam/turn) shifts it: an
        // off-break pitched outside off turns in to the stumps; a leg-break pitched on
        // leg stump turns away to outside off.
        bn::fixed target_line;
        bn::fixed line_spread;
        // Chance (0-100) of a loose ball that ignores the above. Higher = easier bowler.
        // Real bowlers rarely bowl junk; a bad ball just misses line or length enough to
        // be put away. Which kind comes from bad_ball_mix.
        int bad_ball_percent;
        // Relative weights of the kinds of bad ball (see pick_bad_ball in ca_delivery.cpp):
        //   half-volley (over-pitched, around off/middle), long hop (dragged down short),
        //   width (good-ish length, too wide outside off), pads (full, drifting onto the legs).
        // e.g. leg spinners drag it down (long hops); inswingers stray onto the pads.
        struct bad_ball_weights
        {
            int half_volley;
            int long_hop;
            int width;
            int pads;
        };
        bad_ball_weights bad_ball_mix;

        // --- Movement in the air (swing for pace, drift for spin) -----------------------
        // Sideways acceleration while the ball is in the air, m/s^2 (sign as above).
        // Rough guide for a full ball at ~120-140 km/h: 2 = gentle (~0.2 m of curve),
        // 4 = big (~0.4 m), 6+ = cartoonish. The aim is adjusted so the ball still lands on its
        // target, so swing makes it *start* wider and curve in to the line.
        // Swing stops at the bounce.
        bn::fixed air_movement;
        bn::fixed air_movement_variation;

        // --- Movement off the pitch (seam for pace, turn for spin) ----------------------
        // Sideways speed added at the bounce, m/s (sign as above). After the bounce a
        // spinner reaches the batter in ~0.1-0.15 s, so 1.5 m/s ≈ 0.2 m of turn by the
        // crease; pace gets there in ~0.05-0.1 s, so 0.5 m/s ≈ 0.05 m of seam.
        //   seam bowler: bounce_movement 0, variation 0.3-0.6 (either way, unpredictable)
        //   off-break: +1 to +2   leg-break: -1 to -2.5 (wrist spin turns more)
        bn::fixed bounce_movement;
        bn::fixed bounce_movement_variation;

        // --- Bounce ---------------------------------------------------------------------
        // Fraction of the downward speed returned upwards at the bounce (0-1).
        // Higher = bouncier: hard pitches, "hit the deck" bowlers. 0.45-0.6 is sensible.
        bn::fixed bounce_restitution;
        // Fraction of the forward speed kept after the bounce (0-1). Lower = the ball
        // "grips" and slows off the pitch (spinners ~0.8, pace ~0.85-0.9).
        bn::fixed bounce_friction;
    };

    constexpr bowler_profile bowler_profiles[] = {
        {
            .name = "RIGHT-ARM FAST",
            .type = bowler_type::PACE,
            .arm = bowling_arm::RIGHT,
            .side = wicket_side::OVER,
            .speed_min_kmh = 132,
            .speed_max_kmh = 145,
            .release_width = 0.45,
            .release_h = 2.25,
            .target_length = 5.5,
            .length_spread = 2.0,
            .target_line = -0.25,
            .line_spread = 0.35,
            .bad_ball_percent = 10,
            .bad_ball_mix = { .half_volley = 3, .long_hop = 3, .width = 3, .pads = 2 },
            .air_movement = 0,              // no swing: a pure seam bowler
            .air_movement_variation = 0,
            .bounce_movement = 0,
            .bounce_movement_variation = 0.5,
            .bounce_restitution = 0.55,
            .bounce_friction = 0.88,
        },
        {
            .name = "RIGHT-ARM OFF SPIN",
            .type = bowler_type::SPIN,
            .arm = bowling_arm::RIGHT,
            .side = wicket_side::OVER,
            .speed_min_kmh = 78,
            .speed_max_kmh = 90,
            .release_width = 0.35,
            .release_h = 2.0,
            .target_length = 3.5,
            .length_spread = 1.5,
            .target_line = -0.35,           // pitch outside off, turn back in to the stumps
            .line_spread = 0.3,
            .bad_ball_percent = 10,
            .bad_ball_mix = { .half_volley = 3, .long_hop = 2, .width = 1, .pads = 3 },
            .air_movement = 0,              // drift, either way
            .air_movement_variation = 1.0,
            .bounce_movement = 1.4,         // off-break: turns into a right-hander
            .bounce_movement_variation = 0.6,
            .bounce_restitution = 0.45,
            .bounce_friction = 0.80,
        },
        {
            .name = "RIGHT-ARM LEG SPIN",
            .type = bowler_type::SPIN,
            .arm = bowling_arm::RIGHT,
            .side = wicket_side::OVER,
            .speed_min_kmh = 70,
            .speed_max_kmh = 85,
            .release_width = 0.40,
            .release_h = 2.0,
            .target_length = 3.5,
            .length_spread = 2.0,
            .target_line = 0.11,           // pitch on or around leg stump, turn towards off
            .line_spread = 1.0,
            .bad_ball_percent = 10,
            .bad_ball_mix = { .half_volley = 3, .long_hop = 4, .width = 1, .pads = 1 },
            .air_movement = 0,              // drift, either way
            .air_movement_variation = 1.0,
            .bounce_movement = -2.4,         // leg-break: turns into a right-hander
            .bounce_movement_variation = 0.6,
            .bounce_restitution = 0.45,
            .bounce_friction = 0.80,
        },
        {
            .name = "LEFT-ARM INSWING",
            .type = bowler_type::PACE,
            .arm = bowling_arm::LEFT,
            .side = wicket_side::OVER,
            .speed_min_kmh = 115,
            .speed_max_kmh = 128,
            .release_width = 0.35,
            .release_h = 2.1,
            .target_length = 3.0,
            .length_spread = 1.75,
            .target_line = -0.12,           // at off stump: swing in, threaten the stumps
            .line_spread = 0.4,
            .bad_ball_percent = 10,
            .bad_ball_mix = { .half_volley = 3, .long_hop = 1, .width = 2, .pads = 4 },
            .air_movement = 3.5,            // always swings in to a right-hander
            .air_movement_variation = 1.0,  // so 2.5 to 4.5
            .bounce_movement = 0,
            .bounce_movement_variation = 0.5,
            .bounce_restitution = 0.55,
            .bounce_friction = 0.88,
        },
        {
            .name = "RIGHT-ARM OUTSWING",
            .type = bowler_type::PACE,
            .arm = bowling_arm::RIGHT,
            .side = wicket_side::OVER,
            .speed_min_kmh = 122,
            .speed_max_kmh = 135,
            .release_width = 0.45,
            .release_h = 2.2,
            .target_length = 3.5,           // full: tempting the drive
            .length_spread = 1.75,
            .target_line = -0.08,           // pitch middle-and-off, carry away past off stump
            .line_spread = 0.35,
            .bad_ball_percent = 10,
            .bad_ball_mix = { .half_volley = 3, .long_hop = 1, .width = 4, .pads = 2 },
            .air_movement = -3.0,           // always swings away from a right-hander
            .air_movement_variation = 1.0,  // so -2 to -4
            .bounce_movement = 0,
            .bounce_movement_variation = 0.5,
            .bounce_restitution = 0.55,
            .bounce_friction = 0.88,
        },
    };

    // Sideways release position (signed x). Right-arm over and left-arm round release
    // from the off side of the stumps as seen on screen (-x); the other two from +x.
    [[nodiscard]] constexpr bn::fixed release_x(bowling_arm arm, wicket_side side, bn::fixed width)
    {
        bool off_side = (arm == bowling_arm::RIGHT) == (side == wicket_side::OVER);
        return off_side ? -width : width;
    }

    [[nodiscard]] constexpr bn::fixed release_x(const bowler_profile& bowler)
    {
        return release_x(bowler.arm, bowler.side, bowler.release_width);
    }

    // Compile-time checks: these run inside the compiler, cost nothing in the ROM, and
    // fail the build if the release-side logic is ever broken.
    static_assert(release_x(bowling_arm::RIGHT, wicket_side::OVER, 1) < 0);
    static_assert(release_x(bowling_arm::RIGHT, wicket_side::ROUND, 1) > 0);
    static_assert(release_x(bowling_arm::LEFT, wicket_side::OVER, 1) > 0);
    static_assert(release_x(bowling_arm::LEFT, wicket_side::ROUND, 1) < 0);

    constexpr int bowler_profiles_count = sizeof(bowler_profiles) / sizeof(bowler_profiles[0]);
}

#endif
