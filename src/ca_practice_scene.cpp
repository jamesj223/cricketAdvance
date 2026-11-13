#include "ca_practice_scene.h"

#include "bn_math.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_pitch.h"

namespace ca
{

namespace
{
    constexpr int RUN_UP_FRAMES = 40;

    // If no shot has been played this many frames after the ball reached the back-foot
    // contact point, the batter has left it. Longer than any late timing window.
    constexpr int LEAVE_AFTER_FRAMES = 10;
}

practice_scene::practice_scene() :
    _pitch(bn::regular_bg_items::pitch.create_bg(0, 0))
{
    _show_bowler();
    _show_tally();
    _show_help();
    _hud.set_banner("PRESS A TO BOWL");
}

void practice_scene::update()
{
    // Advancing the generator every frame means deliveries depend on *when* the player
    // presses buttons, so each session differs.
    _random.update();

    switch(_state)
    {
        case state::WAITING:
            _update_waiting();
            break;

        case state::RUN_UP:
            if(--_run_up_counter <= 0)
            {
                _release();
            }
            break;

        case state::LIVE:
            _update_live();
            break;

        default:
            break;
    }

    _hud.update();
}

void practice_scene::_update_waiting()
{
    if(bn::keypad::l_pressed() || bn::keypad::r_pressed())
    {
        int delta = bn::keypad::r_pressed() ? 1 : bowler_profiles_count - 1;
        _bowler_index = (_bowler_index + delta) % bowler_profiles_count;
        _show_bowler();
    }

    if(bn::keypad::select_pressed())
    {
        _scheme = control_scheme((int(_scheme) + 1) % control_scheme_count);
        _show_help();
    }

    if(bn::keypad::b_pressed())
    {
        _live_bar_enabled = ! _live_bar_enabled;
        _show_help();

        if(! _live_bar_enabled)
        {
            _live_bar.hide();
        }
    }

    if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
    {
        _start_delivery();
    }
}

void practice_scene::_start_delivery()
{
    _plan = generate_delivery(bowler_profiles[_bowler_index], _random);

    _hud.clear_lines();
    _show_bowler();
    _show_tally();
    _show_help();
    _ball_view.hide();
    _batter.reset();
    _stumps.set_broken(false);
    _timing_bar.hide();
    _live_bar.hide();
    _reader.reset();
    _run_up_counter = RUN_UP_FRAMES;
    _state = state::RUN_UP;
}

void practice_scene::_release()
{
    _flight.emplace(*_plan);

    // Work out where and when the ball will arrive, before the player sees it.
    _preview = preview_delivery(*_plan);
    _line = classify_line(_preview.x_at_crease);
    _length = classify_length(_preview.length);

    _shot_played = false;
    _shot = shot_type::LEAVE;
    _outcome = shot_outcome();
    _hit_frame = -1;

    if(_live_bar_enabled)
    {
        _live_bar.start(_preview);
    }

    bn::string<48> text(bowler_profiles[_bowler_index].name);
    bn::ostringstream stream(text);
    stream << "  " << _plan->speed_kmh << " KM/H";
    _hud.set_line(0, text);

    _state = state::LIVE;
}

void practice_scene::_update_live()
{
    _flight->update();
    int frame = _flight->frames();

    if(! _shot_played && ! _flight->dead())
    {
        if(bn::optional<shot_input> input = _reader.update(_scheme))
        {
            // Timed from when the button was first pressed (see shot_reader).
            _play_shot(input->intent, frame - input->frames_ago);
        }
        else if(_reader.pending())
        {
            // Waiting to see if it's an A+B chord: don't let the ball bowl the batter
            // while we decide.
            _flight->suspend_stumps();
        }
        else if(frame > _preview.back_contact_frame + LEAVE_AFTER_FRAMES)
        {
            // Ball is past the batter without a shot: they left it.
            _shot_played = true;
            _shot = shot_type::LEAVE;
            _batter.play_shot(shot_type::LEAVE);
            _hud.set_line(3, "LEAVE");
        }
    }

    // Early presses wait for the ball to arrive; on-time and late ones hit it now.
    if(_hit_frame >= 0 && frame >= _hit_frame && ! _flight->was_hit() && ! _flight->dead())
    {
        _flight->hit(_outcome.hit_velocity);
        _hud.set_banner(outcome_banner(_outcome));
    }

    // Decision made (and any hit applied): a remembered stumps hit now counts.
    if(! _reader.pending())
    {
        _flight->resume_stumps();
    }

    _ball_view.update(*_flight, _batter.z());

    if(_live_bar_enabled)
    {
        _live_bar.update(frame);
    }

    if(_flight->dead())
    {
        _finish_ball();
    }
}

void practice_scene::_play_shot(const shot_intent& intent, int frame)
{
    footwork foot = intent.foot ? *intent.foot : _best_footwork(intent);
    _shot = make_shot(intent.attacking, foot, intent.direction);
    _shot_played = true;

    int rating = shot_rating(_shot, _line, _length);
    int contact_frame = foot == footwork::FRONT ? _preview.front_contact_frame :
                                                  _preview.back_contact_frame;
    int error = frame - contact_frame;
    timing_grade grade = grade_timing(error, rating, is_defensive(_shot));

    _outcome = resolve_shot(_shot, intent.lofted, grade, rating, _random);

    if(makes_contact(grade))
    {
        _hit_frame = bn::max(frame, contact_frame);
    }

    _batter.play_shot(_shot);
    _timing_bar.show(error, rating, is_defensive(_shot));

    // Feedback: what you played, how good a choice it was, and your timing.
    {
        bn::string<48> text;
        bn::ostringstream stream(text);
        stream << (intent.lofted ? "LOFTED " : "") << shot_name(_shot) << ": " << rating_name(rating);
        _hud.set_line(3, text);
    }
    {
        bn::string<48> text;
        bn::ostringstream stream(text);
        stream << timing_name(grade) << " (" << (error > 0 ? "+" : "") << error << " FRAMES)";
        _hud.set_line(4, text);
    }
}

void practice_scene::_finish_ball()
{
    if(! _flight->was_hit())
    {
        _outcome.runs = 0;

        if(_flight->hit_stumps())
        {
            _outcome.type = outcome_type::BOWLED;
            _stumps.set_broken(true);
        }
        else if(! _shot_played || _shot == shot_type::LEAVE)
        {
            _outcome.type = outcome_type::LEFT;
        }
        else
        {
            _outcome.type = outcome_type::PLAYED_AND_MISSED;
        }

        _hud.set_banner(outcome_banner(_outcome));
    }

    ++_balls;
    _runs += _outcome.runs;

    if(is_out(_outcome.type))
    {
        ++_outs;
    }

    // Reveal what the ball was, now that it's too late to matter.
    bn::string<48> text(length_zone_name(_length));
    text.append(", ");
    text.append(line_zone_name(_line));
    _hud.set_line(2, text);

    _show_tally();
    _state = state::WAITING;
    _show_help();
}

footwork practice_scene::_best_footwork(const shot_intent& intent) const
{
    shot_type front = make_shot(intent.attacking, footwork::FRONT, intent.direction);
    shot_type back = make_shot(intent.attacking, footwork::BACK, intent.direction);
    bool back_better = shot_rating(back, _line, _length) > shot_rating(front, _line, _length);
    return back_better ? footwork::BACK : footwork::FRONT;
}

void practice_scene::_show_bowler()
{
    const bowler_profile& bowler = bowler_profiles[_bowler_index];
    bn::string<48> text(bowler.name);
    text.append(bowler.side == wicket_side::OVER ? " (OVER)" : " (ROUND)");
    _hud.set_line(0, text);
}

void practice_scene::_show_tally()
{
    bn::string<48> text;
    bn::ostringstream stream(text);
    stream << _runs << " RUNS  " << _balls << " BALLS  " << _outs << " OUT";
    _hud.set_line(1, text);
}

void practice_scene::_show_help()
{
    if(_state == state::WAITING)
    {
        bn::string<48> controls("SEL: ");
        controls.append(scheme_name(_scheme));
        // Shows the live bar's current state (B toggles it).
        _hud.set_help(controls, _live_bar_enabled ? "A:BOWL L/R:BOWLER B:LIVE BAR=ON" :
                                                    "A:BOWL L/R:BOWLER B:LIVE BAR=OFF");
    }
    else
    {
        _hud.set_help(scheme_name(_scheme), scheme_help(_scheme));
    }
}

}
