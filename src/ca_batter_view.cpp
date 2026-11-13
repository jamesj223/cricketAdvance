#include "ca_batter_view.h"

#include "bn_sprite_items_batter.h"

#include "ca_camera.h"
#include "ca_draw_order.h"

namespace ca
{

namespace
{
    // The sprite is 32 px tall and the batter ~1.8 m, so the sprite's centre is ~0.9 m up.
    constexpr bn::fixed SPRITE_CENTRE_HEIGHT = 0.9;

    batter_pose pose_for(shot_type shot)
    {
        switch(shot)
        {
            case shot_type::FORWARD_DEFENCE:
            case shot_type::BACK_DEFENCE:
                return batter_pose::BLOCK;
            case shot_type::CUT:
                return batter_pose::CUT;
            case shot_type::PULL:
                return batter_pose::PULL;
            case shot_type::LEAVE:
                return batter_pose::LEAVE;
            default:
                return batter_pose::DRIVE;
        }
    }
}

batter_view::batter_view() :
    _sprite(bn::sprite_items::batter.create_sprite(0, 0)),
    _z(BATTER_Z)
{
    _sprite.set_z_order(draw_order::BATTER);
    reset();
}

void batter_view::reset()
{
    _set(batter_pose::STANCE, BATTER_Z);
}

void batter_view::play_shot(shot_type shot)
{
    bn::fixed z = BATTER_Z;

    if(shot != shot_type::LEAVE)
    {
        z = shot_footwork(shot) == footwork::FRONT ? BATTER_FRONT_FOOT_Z : BATTER_BACK_FOOT_Z;
    }

    _set(pose_for(shot), z);
}

void batter_view::_set(batter_pose pose, bn::fixed z)
{
    _z = z;
    _sprite.set_tiles(bn::sprite_items::batter.tiles_item(), int(pose));

    camera::screen_point screen = camera::project({ BATTER_X, z, SPRITE_CENTRE_HEIGHT });
    _sprite.set_position(screen.x, screen.y);
}

}
