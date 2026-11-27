#include "ca_batting_scene.h"

#include "bn_bg_palettes.h"
#include "bn_math.h"
#include "bn_keypad.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_regular_bg_items_field.h"
#include "bn_regular_bg_items_pitch.h"
#include "bn_regular_bg_items_pitch_zones_pace.h"
#include "bn_regular_bg_items_pitch_zones_spin.h"
#include "bn_sprite_palettes.h"

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

    // After a shot into the field: follow the ball this long (time to take in the shot),
    // then fade (out, swap views, back in) to the top-down field view, and back again.
    constexpr int FOLLOW_BALL_FRAMES = 45;
    constexpr int FADE_FRAMES = 10;

    // The result banner waits a moment after the ball is dead, so there's time to see
    // what happened first. In the field view it follows the moment (rope, catch, runs).
    constexpr int BANNER_DELAY_FRAMES = 30;
    constexpr int FIELD_BANNER_DELAY_FRAMES = 15;

    // After the last ball of an over, how long its banner shows before the next over's
    // card. Input waits for the card, so you always see it before you can bowl.
    constexpr int OVER_CARD_DELAY_FRAMES = 40;

    // After a dismissal, how long its banner ("CAUGHT!") shows before "GAME OVER".
    constexpr int GAME_OVER_BANNER_DELAY_FRAMES = 60;

    // The timing bar slides up a moment after the shot (or, for a shot into the field,
    // once we're back from the field view), so you watch the shot first.
    constexpr int TIMING_BAR_DELAY_FRAMES = 20;
    constexpr int TIMING_BAR_AFTER_FIELD_FRAMES = 8;

    // Banners move up or down to keep clear of the pitch marker: this much space around it.
    constexpr int MARKER_CLEARANCE = 6;

    // Quick Play has no help line during the innings, so the timing bar sits lower.
    constexpr int TIMING_BAR_Y_PRACTICE = 44;
    constexpr int TIMING_BAR_Y_QUICK_PLAY = 52;

    constexpr int BALLS_PER_OVER = 6;   // no wides or no balls yet, so every ball counts

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
    _timing_bar.set_y(_aids() ? TIMING_BAR_Y_PRACTICE : TIMING_BAR_Y_QUICK_PLAY);
    _pitch.set_item(_pitch_item());
    _show_bowler();
    _show_score();
    _show_help();
    _hud.set_banner(mode == game_mode::QUICK_PLAY ? "PRESS A/B TO START" : "PRESS A/B TO BOWL");
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

        case state::TO_FIELD:
        case state::TO_PITCH:
            _update_transition();
            break;

        case state::FIELD:
            _update_field();
            break;

        default:
            break;
    }

    if(_banner_delay > 0 && --_banner_delay == 0)
    {
        _show_pending_banner();
    }

    if(_over_card_delay > 0 && --_over_card_delay == 0)
    {
        _show_over_card();
    }

    // Out: the dismissal banner ("BOWLED!") stays up for a moment first.
    if(_game_over_delay > 0 && --_game_over_delay == 0)
    {
        _hud.set_sub_banner(_new_best ? "NEW HIGH SCORE!" : "GAME OVER");
    }

    _timing_bar.update();
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

    // Wait for the result banner and the next over's card, so neither is missed.
    if(_banner_delay > 0 || _over_card_delay > 0)
    {
        return;
    }

    bool quick_play = _mode == game_mode::QUICK_PLAY;

    // Practice: SELECT cycles the bowlers (L/R are batting buttons, too easy to hit).
    if(! quick_play && bn::keypad::select_pressed())
    {
        _bowler_index = (_bowler_index + 1) % bowler_profiles_count;
        _show_bowler();
        _pitch.set_item(_pitch_item());     // pace and spin have different length zones
    }

    // Back to the menu: any time in Practice; in Quick Play only between innings, so a
    // stray press can't throw away a big score.
    if(bn::keypad::start_pressed() && (! quick_play || _innings_over || ! _innings_started))
    {
        _exit = true;
        return;
    }

    if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
    {
        if(quick_play && (_innings_over || ! _innings_started))
        {
            _new_innings();     // shows who's bowling the first over; the next press bowls
            return;
        }

        _start_delivery();
    }
}

void batting_scene::_start_delivery()
{
    _plan = generate_delivery(bowler_profiles[_bowler_index], _random);
    _over_card_delay = 0;

    _hud.clear_lines();
    _hud.reset_banner_y();
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
    _banner_queued = false;

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
            if(_aids())
            {
                _hud.set_line(3, "LEAVE");
            }
        }
    }

    // Early presses wait for the ball to arrive; on-time and late ones hit it now.
    if(_hit_frame >= 0 && frame >= _hit_frame && ! _flight->was_hit() && ! _flight->dead())
    {
        _hit_position = _flight->position();
        _flight->hit(_outcome.hit_velocity);
        sounds::bat(_contact);

        if(goes_into_the_field(_outcome, is_defensive(_shot)))
        {
            // Watch it go for a moment, then cut to the field to see what happens (the
            // banner and the crowd wait until then).
            _field_delay = FOLLOW_BALL_FRAMES;
        }
    }

    _check_catch();

    _ball_view.update(*_flight, _batter.z());

    if(_field_delay > 0)
    {
        // Following the ball (even if it's come to rest), then over to the field.
        if(--_field_delay == 0)
        {
            _transition_frame = 0;
            _state = state::TO_FIELD;
        }
    }
    else if(_flight->dead())
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
    _timing_bar.prepare(error, rating, is_defensive(_shot));

    if(! goes_into_the_field(_outcome, is_defensive(_shot)))
    {
        _timing_bar.reveal(TIMING_BAR_DELAY_FRAMES);
    }

    // Feedback (Practice only): what you played, how good a choice it was, your timing.
    if(_aids())
    {
        bn::string<48> text;
        bn::ostringstream stream(text);
        stream << (intent.lofted ? "LOFTED " : "") << shot_name(_shot) << ": " << rating_name(rating);
        _hud.set_line(3, text);

        bn::string<48> timing;
        bn::ostringstream timing_stream(timing);
        timing_stream << timing_name(grade) << " (" << (error > 0 ? "+" : "") << error << " FRAMES)";
        _hud.set_line(4, timing);
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

void batting_scene::_update_transition()
{
    // Fade to black, swap views at the darkest point, fade back in. Butano's palette fade
    // recolours every palette towards black (a cheap trick: nothing is redrawn).
    ++_transition_frame;
    bool to_field = _state == state::TO_FIELD;
    bn::fixed intensity;

    if(_transition_frame < FADE_FRAMES)
    {
        intensity = div(_transition_frame, FADE_FRAMES);

        if(to_field)
        {
            // Keep following the ball while the picture fades.
            _flight->update();
            _ball_view.update(*_flight, _batter.z());
        }
    }
    else if(_transition_frame == FADE_FRAMES)
    {
        intensity = 1;
        _show_field_view(to_field);
    }
    else if(_transition_frame < FADE_FRAMES * 2)
    {
        intensity = div(FADE_FRAMES * 2 - _transition_frame, FADE_FRAMES);
    }
    else
    {
        intensity = 0;

        if(to_field)
        {
            _state = state::FIELD;
        }
        else
        {
            _finish_ball();
        }
    }

    bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
}

void batting_scene::_show_field_view(bool field)
{
    _batter.set_visible(! field);
    _stumps.set_visible(! field);
    _hud.set_lines_visible(! field);
    _ball_view.set_marker_only(! field);    // the pitch marker comes back with the pitch

    if(field)
    {
        _pitch.set_item(bn::regular_bg_items::field);
        bool keeper_back = _plan->speed_kmh >= KEEPER_BACK_MIN_KMH;
        _field.emplace(_outcome, _hit_position, keeper_back);
        _hud.reset_banner_y();
        _hud.set_help(_aids() ? "A/B:SKIP" : "");
    }
    else
    {
        _field.reset();
        _pitch.set_item(_pitch_item());
        _place_banners();       // the field view's banner, moved off the marker
    }
}

void batting_scene::_update_field()
{
    if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
    {
        _field->skip();
    }

    switch(_field->update())
    {
        case field_view::status::EVENT:
            _queue_banner(FIELD_BANNER_DELAY_FRAMES);

            if(_outcome.type == outcome_type::FOUR || _outcome.type == outcome_type::SIX ||
                    _outcome.type == outcome_type::CAUGHT)
            {
                sounds::crowd(_outcome.type);
            }
            break;

        case field_view::status::DONE:
            _show_pending_banner();     // skipped: don't lose it
            _transition_frame = 0;
            _state = state::TO_PITCH;
            break;

        default:
            break;
    }
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
    }

    if(! _banner_queued)
    {
        _queue_banner(BANNER_DELAY_FRAMES);
    }

    _timing_bar.reveal(TIMING_BAR_AFTER_FIELD_FRAMES);    // (no-op if it's already on its way)

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

    // Reveal what the ball was, now that it's too late to matter (Practice only).
    if(_aids())
    {
        bn::string<48> text(length_zone_name(_length));
        text.append(", ");
        text.append(line_zone_name(_line));
        _hud.set_line(2, text);
    }

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

    // A fresh start on screen, then the first over's card.
    _hud.clear_lines();
    _batter.reset();
    _stumps.set_broken(false);
    _timing_bar.hide();
    _ball_view.hide();
    _show_score();
    _show_over_card();
    _show_help();
}

void batting_scene::_end_of_over()
{
    _over_bowler = 1 - _over_bowler;
    _bowler_index = _bowlers[_over_bowler];
    _over_card_delay = _banner_delay + OVER_CARD_DELAY_FRAMES;  // the last ball's banner first
}

void batting_scene::_show_over_card()
{
    // "OVER 2" / "LEFT-ARM INSWING": who's bowling this over (also the end-of-over cue).
    bn::string<24> over("OVER ");
    {
        bn::ostringstream stream(over);
        stream << _balls / BALLS_PER_OVER + 1;
    }

    _hud.set_banner(over);
    _hud.set_sub_banner(bowler_profiles[_bowler_index].name);
    _show_bowler();
}

void batting_scene::_queue_banner(int delay)
{
    _banner_queued = true;
    _banner_delay = bn::max(delay, 1);
}

void batting_scene::_show_pending_banner()
{
    if(_banner_queued)
    {
        _banner_delay = 0;

        if(_state != state::FIELD)
        {
            _place_banners();
        }

        _hud.set_banner(outcome_banner(_outcome));
    }
}

const bn::regular_bg_item& batting_scene::_pitch_item() const
{
    // Practice paints the TV-style length zones on the pitch (they differ for spin).
    if(! _aids())
    {
        return bn::regular_bg_items::pitch;
    }

    return bowler_profiles[_bowler_index].type == bowler_type::SPIN ?
                bn::regular_bg_items::pitch_zones_spin : bn::regular_bg_items::pitch_zones_pace;
}

void batting_scene::_place_banners()
{
    // The banners normally sit across the middle of the pitch, which is right where most
    // balls pitch. If the marker's under them, move them: below it for full balls (up the
    // screen, near the batter), above it for shorter ones.
    bn::optional<bn::fixed> marker = _ball_view.marker_y();
    int y = debug_hud::default_banner_y();

    if(marker)
    {
        int marker_y = marker->round_integer();
        int half = debug_hud::BANNER_LINE_HEIGHT / 2;
        int top = y - half;                                     // of the first line
        int bottom = y + debug_hud::BANNER_LINE_HEIGHT + half;  // of the second

        if(marker_y + MARKER_CLEARANCE > top && marker_y - MARKER_CLEARANCE < bottom)
        {
            if(marker_y <= y + half)
            {
                y = marker_y + MARKER_CLEARANCE + half;     // below the marker
            }
            else
            {
                y = marker_y - MARKER_CLEARANCE - half - debug_hud::BANNER_LINE_HEIGHT;   // above
            }
        }
    }

    _hud.set_banner_y(y);
}

void batting_scene::_innings_ended()
{
    _innings_over = true;
    _game_over_delay = _banner_delay + GAME_OVER_BANNER_DELAY_FRAMES;
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
        _hud.set_help(_aids() ? scheme_help(_scheme) : "");
    }
    else if(_mode == game_mode::PRACTICE)
    {
        _hud.set_help("A/B:BOWL  SELECT:BOWLER  START:MENU");
    }
    else if(_innings_over || ! _innings_started)
    {
        _hud.set_help(_innings_over ? "A/B:NEW INNINGS  START:MENU" : "A/B:PLAY  START:MENU");
    }
    else
    {
        _hud.set_help("");      // Quick Play: no hints mid-innings
    }
}

}
