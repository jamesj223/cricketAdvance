#ifndef CA_BATTING_SCENE_H
#define CA_BATTING_SCENE_H

// Batting: deliveries to face, in either mode.
//   Quick Play: two bowlers swap every over; out = game over; best score saved.
//   Practice:   the Phase 2 nets: pick the bowler with L/R, endless, wickets just counted.
// Both keep the batting aids (timing bar, shot rating, what the ball was) for now.

#include "bn_optional.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"

#include "ca_ball_view.h"
#include "ca_batter_view.h"
#include "ca_controls.h"
#include "ca_debug_hud.h"
#include "ca_delivery.h"
#include "ca_game_mode.h"
#include "ca_save.h"
#include "ca_scoreboard.h"
#include "ca_shots.h"
#include "ca_sounds.h"
#include "ca_stumps_view.h"
#include "ca_timing_bar.h"

namespace ca
{
    class batting_scene
    {

    public:
        batting_scene(game_mode mode, save_data& save);

        // Call once per frame. Returns false when the player goes back to the menu.
        [[nodiscard]] bool update();

    private:
        enum class state
        {
            WAITING,    // between balls
            RUN_UP,     // short pause before release
            LIVE,       // ball in play
        };

        game_mode _mode;
        save_data& _save;
        bn::regular_bg_ptr _pitch;
        ball_view _ball_view;
        batter_view _batter;
        stumps_view _stumps;
        timing_bar _timing_bar;
        debug_hud _hud;
        bn::random _random;

        state _state = state::WAITING;
        int _run_up_counter = 0;
        int _cooldown = 0;      // frames left ignoring input after a ball
        int _bowler_index = 0;
        control_scheme _scheme;
        bool _exit = false;

        // The current delivery.
        bn::optional<delivery_plan> _plan;
        bn::optional<ball_flight> _flight;
        delivery_preview _preview;
        line_zone _line = line_zone::STUMPS;
        length_zone _length = length_zone::GOOD;

        // The shot played to it.
        bool _shot_played = false;
        shot_type _shot = shot_type::LEAVE;
        shot_outcome _outcome;
        int _hit_frame = -1;    // frame the bat meets the ball; -1 = it won't
        sounds::bat_contact _contact;   // how the hit will sound
        vec3 _hit_position;     // where the bat met it
        int _bounces_heard = 0; // bounces we've already played a sound for
        int _crowd_delay = 0;   // frames until the boundary cheer (0 = none pending)

        // The innings.
        scoreboard _scoreboard;
        int _runs = 0;
        int _balls = 0;
        int _wickets = 0;               // Practice only (Quick Play ends at the first)
        bool _innings_started = false;  // Quick Play: the first ball starts the innings
        bool _innings_over = false;
        bool _new_best = false;
        int _game_over_delay = 0;   // frames until the "GAME OVER" line (0 = none pending)
        int _bowlers[2] = { 0, 1 }; // Quick Play: the two bowlers, one from each end
        int _over_bowler = 0;       // which of them has this over

        void _update_waiting();
        void _start_delivery();
        void _release();
        void _update_live();
        void _play_shot(const shot_intent& intent, int frame);
        void _play_bounce_sounds();
        void _check_catch();
        void _finish_ball();
        [[nodiscard]] int _frames_until_catch() const;

        [[nodiscard]] footwork _natural_footwork() const;
        void _show_bowler();
        void _show_score();
        void _new_innings();
        void _show_help();
        void _end_of_over();
        void _innings_ended();
    };
}

#endif
