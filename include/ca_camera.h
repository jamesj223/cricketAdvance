#ifndef CA_CAMERA_H
#define CA_CAMERA_H

// Fake-3D camera: a pinhole camera behind the bowler, tilted down at the pitch.
// The pitch background is pre-rendered with the same maths by
// tools/make_placeholder_graphics.py — keep the constants here in sync with it.

#include "ca_world.h"

namespace ca::camera
{
    constexpr bn::fixed BACK = 24.0;            // metres behind the bowler's stumps
    constexpr bn::fixed HEIGHT = 6.5;           // metres above the ground
    constexpr bn::fixed FOCAL = 760.0;          // pixels
    constexpr bn::fixed SCREEN_Y_OFFSET = -20.0;

    // Camera tilt: atan2(HEIGHT, BACK + 17.0 [look-at z]).
    constexpr bn::fixed SIN_T = 0.156581;
    constexpr bn::fixed COS_T = 0.987665;

    // Depth at which sprites are drawn at scale 1 (the batter's stumps).
    constexpr bn::fixed REFERENCE_DEPTH = 44.6;
    constexpr bn::fixed SCALE_PER_PIXEL = div(REFERENCE_DEPTH, FOCAL);

    struct screen_point
    {
        bn::fixed x;        // centred screen coords (0, 0 = middle of the screen)
        bn::fixed y;
        bn::fixed scale;    // sprite scale for this depth (1 at the batter's end)
    };

    [[nodiscard]] inline screen_point project(const vec3& p)
    {
        bn::fixed dz = p.z + BACK;
        bn::fixed dy = p.h - HEIGHT;
        bn::fixed depth = mul(dz, COS_T) - mul(dy, SIN_T);
        bn::fixed up = mul(dy, COS_T) + mul(dz, SIN_T);
        // One division, reused. (FOCAL / depth rather than 1 / depth: with only 12
        // fractional bits, 1 / depth would be too coarse and the ball would judder.)
        bn::fixed f = div(FOCAL, depth);
        return { mul(p.x, f), SCREEN_Y_OFFSET - mul(up, f), mul(f, SCALE_PER_PIXEL) };
    }
}

#endif
