#include "ca_batting_scene.h"

#include "bn_math.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_pitch.h"

#include "ca_sounds.h"

namespace ca
{

namespace
{
    constexpr int RUN_UP_FRAMES = 40;

    // If no shot has been played this many frames after the ball reached the back-foot
    // contact point, the batter has left it. Longer than any late timing window.
    constexpr int LEAVE_AFTER_FRAMES = 10;

    // After each ball, input is ignored for this long, so a very late shot press can't
    // bowl the next ball.
    constexpr int COOLDOWN_FRAMES = 45;

    // How long after the hit the crowd cheers a boundary (60 frames = 1 s). A six hangs
    // in the air, so the cheer builds a little later.
    constexpr int BOUNDARY_CHEER_DELAY_FRAMES = 30;
    constexpr int SIX_CHEER_DELAY_FRAMES = 40;

    // After a dismissal, how long its banner ("CAUGHT!") shows before "GAME OVER".
    constexpr int GAME_OVER_BANNER_DELAY_FRAMES = 75;

    constexpr int BALLS_PER_OVER = 6;   // no wides or no balls yet, so every ball counts

    // The ball is removed 90 frames after the hit, then input is ignored for
    // COOLDOWN_FRAMES; an outfield catch cheer must land before the next ball can start.
    constexpr int MAX_CATCH_CHEER_DELAY_FRAMES = 120;

    // How far behind the bat an edge is caught: the keeper stands back to pace and up to
    // the stumps for spin (the slips are alongside the keeper).
    constexpr int KEEPER_BACK_MIN_KMH = 100;
    constexpr bn::fixed KEEPER_BACK_DISTANCE = 14;
    constexpr bn::fixed KEEPER_UP_DISTANCE = 2.5;

    // A skied catch is taken as the ball comes down to about this height.
    constexpr bn::fixed CATCH_HEIGHT = 1.8;

}

batting_scene::batting_scene(game_mode mode, save_data& save) :
    _mode(mode),
    _save(save),
    _pitch(bn::regular_bg_items::pitch.create_bg(0, 0)),
    _scheme(save.scheme)
{
    _show_bowler();
    _show_score();
    _show_help();
    _hud.set_banner("PRESS A/B TO BOWL");
}

bool batting_scene::update()
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

    // Ticks in every state: the ball may already be dead when the cheer is due.
    if(_crowd_delay > 0 && --_crowd_delay == 0)
    {
        sounds::crowd(_outcome.type);
    }

    // Out: the dismissal banner ("BOWLED!") stays up for a moment first.
    if(_game_over_delay > 0 && --_game_over_delay == 0)
    {
        _hud.set_sub_banner(_new_best ? "NEW HIGH SCORE!" : "GAME OVER");
    }

    _hud.update();
    return ! _exit;
}

void batting_scene::_update_waiting()
{
    if(_cooldown > 0)
    {
        --_cooldown;
        return;
    }

    bool quick_play = _mode == game_mode::QUICK_PLAY;

    if(! quick_play && (bn::keypad::l_pressed() || bn::keypad::r_pressed()))
    {
        int delta = bn::keypad::r_pressed() ? 1 : bowler_profiles_count - 1;
        _bowler_index = (_bowler_index + delta) % bowler_profiles_count;
        _show_bowler();
    }

    // Back to the menu: any time in Practice; in Quick Play only between innings, so a
    // stray press can't throw away a big score.
    if(bn::keypad::select_pressed() && (! quick_play || _innings_over || ! _innings_started))
    {
        _exit = true;
        return;
    }

    if(bn::keypad::a_pressed() || bn::keypad::b_pressed() || bn::keypad::start_pressed())
    {
        if(quick_play && (_innings_over || ! _innings_started))
        {
            _new_innings();
        }

        _start_delivery();
    }
}

void batting_scene::_start_delivery()
{
    _plan = generate_delivery(bowler_profiles[_bowler_index], _random);

    _hud.clear_lines();
    _show_bowler();
    _show_score();
    _show_help();
    _ball_view.hide();
    _batter.reset();
    _stumps.set_broken(false);
    _timing_bar.hide();
    _run_up_counter = RUN_UP_FRAMES;
    _state = state::RUN_UP;
}

void batting_scene::_release()
{
    _flight.emplace(*_plan);

    // Work out where and when the ball will arrive, before the player sees it.
    _preview = preview_delivery(*_plan);
    _line = classify_line(_preview.x_at_crease);
    _length = classify_length(_preview.length, bowler_profiles[_bowler_index].type);

    _shot_played = false;
    _shot = shot_type::LEAVE;
    _outcome = shot_outcome();
    _hit_frame = -1;
    _bounces_heard = 0;

    bn::string<48> text(bowler_profiles[_bowler_index].name);
    bn::ostringstream stream(text);
    stream << "  " << _plan->speed_kmh << " KM/H";
    _hud.set_line(0, text);

    _state = state::LIVE;
}

void batting_scene::_update_live()
{
    _flight->update();
    int frame = _flight->frames();
    _play_bounce_sounds();

    if(! _shot_played && ! _flight->dead())
    {
        if(bn::optional<shot_intent> intent = read_shot(_scheme))
        {
            _play_shot(*intent, frame);
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
        _hit_position = _flight->position();
        _flight->hit(_outcome.hit_velocity);
        _hud.set_banner(outcome_banner(_outcome));
        sounds::bat(_contact);

        // The crowd reacts once they see where it's going, not at the moment of the hit.
        if(_outcome.type == outcome_type::FOUR)
        {
            _crowd_delay = BOUNDARY_CHEER_DELAY_FRAMES;
        }
        else if(_outcome.type == outcome_type::SIX)
        {
            _crowd_delay = SIX_CHEER_DELAY_FRAMES;
        }
        else if(_outcome.type == outcome_type::CAUGHT && ! is_defensive(_shot))
        {
            // Outfield catches are taken after the ball has flown out of view (and been
            // removed), so _check_catch never sees them: cheer when the catch would be.
            _crowd_delay = _frames_until_catch();
        }
    }

    _check_catch();

    _ball_view.update(*_flight, _batter.z());

    if(_flight->dead())
    {
        _finish_ball();
    }
}

void batting_scene::_play_shot(const shot_intent& intent, int frame)
{
    // Basic: the game reads the length, like a batter's instinct (not always the best
    // shot; Pro players can beat it by choosing the foot themselves).
    footwork foot = intent.foot ? *intent.foot : _natural_footwork();
    _shot = make_shot(intent.attacking, foot, intent.direction);
    _shot_played = true;

    int rating = shot_rating(_shot, _line, _length);
    int contact_frame = foot == footwork::FRONT ? _preview.front_contact_frame :
                                                  _preview.back_contact_frame;
    int error = frame - contact_frame;
    timing_grade grade = grade_timing(error, rating, is_defensive(_shot));

    _outcome = resolve_shot(_shot, intent.lofted, grade, rating, _line, _random);
    _contact = { _outcome.type, grade, intent.direction, is_defensive(_shot), intent.lofted };

    if(makes_contact(grade) && _outcome.type != outcome_type::PLAYED_AND_MISSED)
    {
        _hit_frame = bn::max(frame, contact_frame);
    }
    else if(! is_defensive(_shot))
    {
        sounds::whoosh();   // swinging at thin air
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

void batting_scene::_play_bounce_sounds()
{
    if(_flight->bounces() == _bounces_heard)
    {
        return;
    }

    _bounces_heard = _flight->bounces();
    bn::fixed impact = _flight->last_impact_speed();
    bn::fixed x = _flight->position().x;

    if(! _flight->was_hit() && _bounces_heard == 1)
    {
        sounds::pitch_bounce(impact, _preview.length, x);
    }
    else
    {
        sounds::outfield_bounce(impact, x);
    }
}

void batting_scene::_check_catch()
{
    if(! _flight->was_hit() || _flight->dead())
    {
        return;
    }

    const vec3& position = _flight->position();

    if(_outcome.type == outcome_type::CAUGHT_BEHIND)
    {
        bn::fixed distance = _plan->speed_kmh >= KEEPER_BACK_MIN_KMH ? KEEPER_BACK_DISTANCE :
                                                                       KEEPER_UP_DISTANCE;
        bn::fixed dx = position.x - _hit_position.x;
        bn::fixed dz = position.z - _hit_position.z;

        if(mul(dx, dx) + mul(dz, dz) >= mul(distance, distance))
        {
            _flight->stop();
            sounds::crowd(_outcome.type);
        }
    }
    else if(_outcome.type == outcome_type::CAUGHT || _outcome.type == outcome_type::CAUGHT_AND_BOWLED)
    {
        if(_flight->velocity().h < 0 && position.h <= CATCH_HEIGHT)
        {
            _flight->stop();
            sounds::crowd(_outcome.type);
        }
    }
}

int batting_scene::_frames_until_catch() const
{
    // Solve h(t) = h0 + up*t - g/2 t^2 = CATCH_HEIGHT for the later (coming down) root:
    //   t = (up + sqrt(up^2 + 2g (h0 - CATCH_HEIGHT))) / g
    bn::fixed up = _outcome.hit_velocity.h;
    bn::fixed h0 = _hit_position.h;
    bn::fixed discriminant = mul(up, up) + mul(2 * GRAVITY, h0 - CATCH_HEIGHT);
    // (If it never gets above catching height, it's caught at the top of its flight.)
    bn::fixed root = discriminant > 0 ? bn::fixed(bn::sqrt(discriminant)) : bn::fixed(0);
    bn::fixed t = div(up + root, GRAVITY);

    // Never later than the next ball could start, or it would cheer over the next delivery.
    return bn::clamp((t * 60).round_integer(), 1, MAX_CATCH_CHEER_DELAY_FRAMES);
}

void batting_scene::_finish_ball()
{

    if(! _flight->was_hit())
    {
        _outcome.runs = 0;

        if(_flight->hit_stumps())
        {
            _outcome.type = outcome_type::BOWLED;
            _stumps.set_broken(true);
            sounds::stumps();
            sounds::crowd(_outcome.type);
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
        if(_mode == game_mode::QUICK_PLAY)
        {
            _innings_ended();
        }
        else
        {
            ++_wickets;
        }
    }
    else if(_mode == game_mode::QUICK_PLAY && _balls % BALLS_PER_OVER == 0)
    {
        _end_of_over();
    }

    // Reveal what the ball was, now that it's too late to matter.
    bn::string<48> text(length_zone_name(_length));
    text.append(", ");
    text.append(line_zone_name(_line));
    _hud.set_line(2, text);

    _show_score();
    _cooldown = COOLDOWN_FRAMES;
    _state = state::WAITING;
    _show_help();
}

footwork batting_scene::_natural_footwork() const
{
    bn::fixed back_from = back_foot_from_length(bowler_profiles[_bowler_index].type);
    return _preview.length >= back_from ? footwork::BACK : footwork::FRONT;
}

void batting_scene::_show_bowler()
{
    if(_mode == game_mode::QUICK_PLAY && ! _innings_started)
    {
        _hud.set_line(0, "QUICK PLAY");    // the bowlers are picked when you start
        return;
    }

    const bowler_profile& bowler = bowler_profiles[_bowler_index];
    bn::string<48> text(bowler.name);
    text.append(bowler.side == wicket_side::OVER ? " (OVER)" : " (ROUND)");
    _hud.set_line(0, text);
}

void batting_scene::_show_score()
{
    if(_mode == game_mode::QUICK_PLAY)
    {
        _scoreboard.set_quick_play(_runs, _balls, _innings_over, _save.best[int(_scheme)].runs);
    }
    else
    {
        _scoreboard.set_practice(_runs, _wickets, _balls);
    }
}

void batting_scene::_new_innings()
{
    _runs = 0;
    _balls = 0;
    _wickets = 0;
    _innings_started = true;
    _innings_over = false;
    _new_best = false;
    _game_over_delay = 0;

    // Two different bowlers, one from each end. Picked now (when A is pressed), not in
    // the constructor: the random generator has been ticking every frame since the scene
    // started, so the pair depends on when you pressed it.
    _bowlers[0] = _random.get_int(bowler_profiles_count);
    _bowlers[1] = _random.get_int(bowler_profiles_count - 1);

    if(_bowlers[1] >= _bowlers[0])
    {
        ++_bowlers[1];      // skip over the first bowler, so they're never the same
    }

    _over_bowler = 0;
    _bowler_index = _bowlers[0];
}

void batting_scene::_end_of_over()
{
    _hud.set_sub_banner("END OF OVER");
    _over_bowler = 1 - _over_bowler;
    _bowler_index = _bowlers[_over_bowler];
    _show_bowler();     // shows who's bowling the next over
}

void batting_scene::_innings_ended()
{
    _innings_over = true;
    _game_over_delay = GAME_OVER_BANNER_DELAY_FRAMES;
    _new_best = record_innings(_save, _scheme, _runs, _balls);

    if(_new_best)
    {
        store_save(_save);
    }
}

void batting_scene::_show_help()
{
    // One line, just above the scoreboard. Keep it under ~230 px (~6 px per character).
    if(_state != state::WAITING)
    {
        _hud.set_help(scheme_help(_scheme));
    }
    else if(_mode == game_mode::PRACTICE)
    {
        _hud.set_help("A/B:BOWL  L/R:BOWLER  SEL:MENU");
    }
    else if(_innings_over || ! _innings_started)
    {
        _hud.set_help(_innings_over ? "A/B:NEW INNINGS  SEL:MENU" : "A/B:BOWL  SEL:MENU");
    }
    else
    {
        _hud.set_help("A/B:BOWL");
    }
}

}
