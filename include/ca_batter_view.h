#ifndef CA_BATTER_VIEW_H
#define CA_BATTER_VIEW_H

// Draws the (placeholder) batter: one sprite, a pose per shot, and a step forward/back
// for footwork.

#include "bn_sprite_ptr.h"
#include "ca_shots.h"

namespace ca
{
    // Frame order MUST match BATTER_POSES in tools/make_placeholder_graphics.py.
    enum class batter_pose
    {
        STANCE,
        BLOCK,
        DRIVE,
        CUT,
        PULL,
        LEAVE,
    };

    class batter_view
    {

    public:
        batter_view();

        // Back to the stance, in the middle of the crease.
        void reset();

        // Shows the pose for this shot and steps forward/back.
        void play_shot(shot_type shot);

        [[nodiscard]] bn::fixed z() const { return _z; }

    private:
        bn::sprite_ptr _sprite;
        bn::fixed _z;

        void _set(batter_pose pose, bn::fixed z);
    };
}

#endif
