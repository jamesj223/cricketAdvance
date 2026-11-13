#ifndef CA_WORLD_H
#define CA_WORLD_H

// World coordinates, in metres (bn::fixed: 20.12 fixed point; the GBA has no FPU).
//   x: across the pitch. +x = screen right. Off side of a right-handed batter = -x.
//   z: down the pitch. z = 0 at the bowler's stumps, z = STUMPS_Z at the batter's stumps.
//   h: height above the ground.

#include "bn_fixed.h"

namespace ca
{
    // Full-precision fixed-point multiply/divide.
    //
    // WARNING: bn::fixed's own `*` and `/` operators are fast but lossy: they drop each
    // operand to 6 fractional bits first, so anything below 1/64 (e.g. DT ~= 0.01)
    // becomes 0. The safe_ versions use a 64-bit intermediate instead. Use these for all
    // physics / camera maths; the plain operators are fine for fixed * int and fixed / int.
    [[nodiscard]] constexpr bn::fixed mul(bn::fixed a, bn::fixed b)
    {
        return a.safe_multiplication(b);
    }

    [[nodiscard]] constexpr bn::fixed div(bn::fixed a, bn::fixed b)
    {
        return a.safe_division(b);
    }

    struct vec3
    {
        bn::fixed x;
        bn::fixed z;
        bn::fixed h;
    };

    // Pitch geometry (real-world dimensions).
    constexpr bn::fixed STUMPS_Z = 20.12;            // batter's stumps
    constexpr bn::fixed POPPING_CREASE_Z = 18.90;    // batter's popping crease
    constexpr bn::fixed STUMP_HALF_WIDTH = 0.114;    // stumps are 22.86 cm wide
    constexpr bn::fixed STUMP_HEIGHT = 0.71;
    constexpr bn::fixed BALL_RADIUS = 0.036;

    constexpr bn::fixed RELEASE_Z = 1.0;             // just behind the bowler's popping crease
    constexpr bn::fixed DEAD_BALL_Z = 23.0;          // roughly where the keeper takes it

    constexpr bn::fixed GRAVITY = 9.8;

    // Game speed: < 1 slows everything down (real pace bowling gives ~0.45 s to react,
    // which is too fast for a handheld game). Trajectories keep the same shape.
    constexpr bn::fixed GAME_SPEED = 0.6;

    // Simulation time step per frame (60 fps), in game-time seconds.
    constexpr bn::fixed DT = GAME_SPEED / 60;
}

#endif
