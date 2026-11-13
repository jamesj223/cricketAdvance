// Phase 1: a ball is bowled.
//
// Controls:
//   A      bowl a new delivery
//   B      replay the last delivery
//   L / R  change bowler

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_vector.h"
#include "bn_optional.h"
#include "bn_blending.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_text_generator.h"

#include "bn_regular_bg_items_pitch.h"
#include "bn_sprite_items_ball.h"
#include "bn_sprite_items_shadow.h"
#include "bn_sprite_items_marker.h"
#include "common_variable_8x8_sprite_font.h"

#include "ca_camera.h"
#include "ca_delivery.h"

namespace
{
    constexpr int RUN_UP_FRAMES = 40;

    enum class state
    {
        READY,      // waiting for the player to bowl
        RUN_UP,     // short pause before release
        IN_FLIGHT,
        DEAD,       // ball finished; results shown
    };

    // Appends a fixed-point value with one decimal place, e.g. "5.2".
    void append_1dp(bn::ostringstream& stream, bn::fixed value)
    {
        int tenths = (value * 10).round_integer();

        if(tenths < 0)
        {
            stream << '-';
            tenths = -tenths;
        }

        stream << tenths / 10 << '.' << tenths % 10;
    }

    // A block of left-aligned debug text lines at the top of the screen, plus a help line.
    // Text is made of sprites, so it is only regenerated when it changes.
    class debug_text
    {

    public:
        debug_text() :
            _generator(common::variable_8x8_sprite_font)
        {
        }

        void set_line(int index, const bn::string_view& text)
        {
            _lines[index] = text;
            _dirty = true;
        }

        void clear()
        {
            for(bn::string<48>& line : _lines)
            {
                line.clear();
            }

            _dirty = true;
        }

        void update()
        {
            if(! _dirty)
            {
                return;
            }

            _dirty = false;
            _sprites.clear();

            for(int index = 0; index < LINES; ++index)
            {
                _generator.generate(-116, -74 + index * 10, _lines[index], _sprites);
            }

            _generator.set_center_alignment();
            _generator.generate(0, 74, "A: BOWL   B: REPLAY   L/R: BOWLER", _sprites);
            _generator.set_left_alignment();
        }

    private:
        static constexpr int LINES = 4;

        bn::sprite_text_generator _generator;
        bn::string<48> _lines[LINES];
        bn::vector<bn::sprite_ptr, 64> _sprites;    // ~6 letters per sprite; see docs/learnings.md
        bool _dirty = true;
    };
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr pitch = bn::regular_bg_items::pitch.create_bg(0, 0);

    // Sprites. Lower z order draws on top: ball over shadow over pitch marker.
    bn::sprite_ptr ball = bn::sprite_items::ball.create_sprite(0, 0);
    bn::sprite_ptr shadow = bn::sprite_items::shadow.create_sprite(0, 0);
    bn::sprite_ptr marker = bn::sprite_items::marker.create_sprite(0, 0);
    ball.set_z_order(0);
    shadow.set_z_order(1);
    marker.set_z_order(2);
    ball.set_visible(false);
    shadow.set_visible(false);
    marker.set_visible(false);

    // The shadow is semi-transparent so the pitch shows through it.
    shadow.set_blending_enabled(true);
    bn::blending::set_transparency_alpha(0.5);

    bn::random random;
    debug_text text;

    int bowler_index = 0;
    state current_state = state::READY;
    int run_up_counter = 0;
    bn::optional<ca::delivery_plan> plan;
    bn::optional<ca::ball_flight> flight;

    auto show_bowler = [&]
    {
        text.clear();
        const ca::bowler_profile& bowler = ca::bowler_profiles[bowler_index];
        bn::string<48> line(bowler.name);
        line.append(bowler.side == ca::wicket_side::OVER ? " (OVER)" : " (ROUND)");
        text.set_line(0, line);
    };

    show_bowler();

    while(true)
    {
        // Advancing the generator every frame means the deliveries depend on *when*
        // the player presses A, so they differ between play sessions.
        random.update();

        if(current_state == state::READY || current_state == state::DEAD)
        {
            if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
            {
                int delta = bn::keypad::r_pressed() ? 1 : ca::bowler_profiles_count - 1;
                bowler_index = (bowler_index + delta) % ca::bowler_profiles_count;
                show_bowler();
            }

            bool bowl_new = bn::keypad::a_pressed();
            bool replay = bn::keypad::b_pressed() && plan;

            if(bowl_new || replay)
            {
                if(bowl_new)
                {
                    plan = ca::generate_delivery(ca::bowler_profiles[bowler_index], random);
                }

                show_bowler();
                ball.set_visible(false);
                shadow.set_visible(false);
                marker.set_visible(false);
                run_up_counter = RUN_UP_FRAMES;
                current_state = state::RUN_UP;
            }
        }
        else if(current_state == state::RUN_UP)
        {
            if(--run_up_counter <= 0)
            {
                flight.emplace(*plan);

                bn::string<48> line;
                bn::ostringstream stream(line);
                stream << plan->speed_kmh << " KM/H";
                text.set_line(1, line);

                current_state = state::IN_FLIGHT;
            }
        }
        else if(current_state == state::IN_FLIGHT)
        {
            if(! flight->update())
            {
                // Ball is dead: describe what it did.
                if(flight->bounced())
                {
                    bn::fixed length = ca::STUMPS_Z - flight->bounce_position().z;
                    bn::string<48> line;
                    bn::ostringstream stream(line);
                    stream << ca::length_name(length) << " (";
                    append_1dp(stream, length);
                    stream << " M)";
                    text.set_line(2, line);
                }

                if(flight->reached_crease())
                {
                    bn::string<48> line;
                    bn::ostringstream stream(line);
                    stream << ca::line_name(flight->crease_position().x) << ", AT ";
                    append_1dp(stream, flight->crease_position().h);
                    stream << " M HIGH";
                    text.set_line(3, line);
                }

                if(flight->hit_stumps())
                {
                    text.set_line(1, "HITTING THE STUMPS!");
                }

                current_state = state::DEAD;
            }
        }

        // Draw the ball, its shadow and the pitch marker from the 3D simulation.
        if(flight && (current_state == state::IN_FLIGHT || current_state == state::DEAD))
        {
            const ca::vec3& pos = flight->position();
            ca::camera::screen_point ball_screen = ca::camera::project(pos);
            ca::camera::screen_point shadow_screen = ca::camera::project({ pos.x, pos.z, 0 });

            ball.set_position(ball_screen.x, ball_screen.y);
            ball.set_scale(ball_screen.scale);
            ball.set_visible(true);

            shadow.set_position(shadow_screen.x, shadow_screen.y);
            shadow.set_scale(shadow_screen.scale);
            shadow.set_visible(true);

            if(flight->bounced())
            {
                ca::camera::screen_point marker_screen = ca::camera::project(flight->bounce_position());
                marker.set_position(marker_screen.x, marker_screen.y);
                marker.set_visible(true);
            }
        }

        text.update();
        bn::core::update();
    }
}
