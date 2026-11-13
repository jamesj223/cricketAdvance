#ifndef CA_DRAW_ORDER_H
#define CA_DRAW_ORDER_H

// Sprite z orders in one place. In Butano, LOWER z order = drawn ON TOP.
// (Butano groups sprites by z order internally, so keep the number of distinct values low.)
//
// From the camera, front to back: ball in front of the batter, the batter, ball between
// batter and stumps, the stumps, ball behind the stumps, pitch marker.

namespace ca::draw_order
{
    constexpr int TEXT = -10;
    constexpr int TIMING_BAR = -9;
    constexpr int BALL_IN_FRONT = 0;
    constexpr int SHADOW_IN_FRONT = 1;
    constexpr int BATTER = 2;
    constexpr int BALL_MIDDLE = 3;
    constexpr int SHADOW_MIDDLE = 4;
    constexpr int STUMPS = 5;
    constexpr int BALL_BEHIND = 6;
    constexpr int SHADOW_BEHIND = 7;
    constexpr int MARKER = 8;
}

#endif
