// Phase 0: Hello GBA.
// A ball sprite you can move with the D-pad, plus some text.
// Hold A to move faster, press B to reset to the centre.

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_display.h"
#include "bn_fixed.h"
#include "bn_algorithm.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_text_generator.h"

#include "bn_sprite_items_ball.h"
#include "common_variable_8x8_sprite_font.h"

int main()
{
    bn::core::init();

    // Grass-green backdrop (the colour shown where no background is drawn).
    bn::bg_palettes::set_transparent_color(bn::color(4, 16, 4));

    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
    text_generator.set_center_alignment();

    // Static title text. Sprites stay on screen as long as their bn::sprite_ptr lives.
    bn::vector<bn::sprite_ptr, 16> title_sprites;
    text_generator.generate(0, -70, "CRICKET ADVANCE - PHASE 0", title_sprites);
    text_generator.generate(0, 70, "D-PAD: MOVE   A: FAST   B: RESET", title_sprites);

    // Butano uses a centred coordinate system: (0, 0) is the middle of the screen.
    bn::sprite_ptr ball = bn::sprite_items::ball.create_sprite(0, 0);

    constexpr bn::fixed half_w = bn::display::width() / 2 - 8;
    constexpr bn::fixed half_h = bn::display::height() / 2 - 8;

    bn::vector<bn::sprite_ptr, 8> coord_sprites;

    while(true)
    {
        // bn::fixed is fixed-point: the GBA has no FPU, so no floats.
        bn::fixed speed = bn::keypad::a_held() ? 3 : 1;

        if(bn::keypad::left_held())  { ball.set_x(ball.x() - speed); }
        if(bn::keypad::right_held()) { ball.set_x(ball.x() + speed); }
        if(bn::keypad::up_held())    { ball.set_y(ball.y() - speed); }
        if(bn::keypad::down_held())  { ball.set_y(ball.y() + speed); }

        if(bn::keypad::b_pressed())
        {
            ball.set_position(0, 0);
        }

        // Clamp to the screen.
        ball.set_x(bn::clamp(ball.x(), -half_w, half_w));
        ball.set_y(bn::clamp(ball.y(), -half_h, half_h));

        // Show the ball's position (regenerated every frame; cheap enough for a demo).
        coord_sprites.clear();
        bn::string<32> coords;
        bn::ostringstream stream(coords);
        stream << "X " << ball.x().right_shift_integer() << "  Y " << ball.y().right_shift_integer();
        text_generator.generate(0, 56, coords, coord_sprites);

        // Waits for VBlank and commits sprite/background changes to the hardware.
        bn::core::update();
    }
}
