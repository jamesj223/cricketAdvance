#ifndef CA_PRACTICE_SCENE_H
#define CA_PRACTICE_SCENE_H

// Practice nets: an endless stream of deliveries to bat against, with debug feedback.
// This is the Phase 2 playground for tuning controls and timing.

#include "bn_optional.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"

#include "ca_ball_view.h"
#include "ca_batter_view.h"
#include "ca_controls.h"
#include "ca_debug_hud.h"
#include "ca_delivery.h"
#include "ca_shots.h"
#include "ca_stumps_view.h"
#include "ca_timing_bar.h"
#include "ca_live_timing_bar.h"

namespace ca
{
    class practice_scene
    {

    public:
        practice_scene();

        // Call once per frame.
        void update();

    private:
        enum class state
        {
            WAITING,    // between balls
            RUN_UP,     // short pause before release
            LIVE,       // ball in play
        };

        bn::regular_bg_ptr _pitch;
        ball_view _ball_view;
        batter_view _batter;
        stumps_view _stumps;
        timing_bar _timing_bar;
        live_timing_bar _live_bar;
        shot_reader _reader;
        bool _live_bar_enabled = false;
        debug_hud _hud;
        bn::random _random;

        state _state = state::WAITING;
        int _run_up_counter = 0;
        int _bowler_index = 0;
        control_scheme _scheme = control_scheme::BASIC;

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

        // Running tally (for playtesting; the real scoreboard comes in Phase 3).
        int _runs = 0;
        int _balls = 0;
        int _outs = 0;

        void _update_waiting();
        void _start_delivery();
        void _release();
        void _update_live();
        void _play_shot(const shot_intent& intent, int frame);
        void _finish_ball();

        [[nodiscard]] footwork _best_footwork(const shot_intent& intent) const;
        void _show_bowler();
        void _show_tally();
        void _show_help();
    };
}

#endif
