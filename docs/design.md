# Cricket Advance — Design Doc

A living document. Decisions are marked **Decided**; things still being explored are
marked **Open** and get settled by prototyping, not by debate.

## Pillars

1. **It has to feel good.** Controls and timing are the game. Everything else serves them.
2. **Skill, not luck.** When you get out it should be obvious *why*: wrong shot, bad timing,
   or a tired batter. Never arbitrary.
3. **Short bursts.** A run should take minutes, and end with "one more go".
4. **Arcade over sim.** Closer to Stick Cricket than to Big Ant, but with more depth than
   Stick Cricket (line, length, shot choice matter).

## Target

- Platform: GBA (`.gba` ROM), played on an Anbernic RG SP. Developed and tested in mGBA.
- Engine: **Butano** (C++). **Decided.**
- Art: placeholder shapes first; pixel art later.
- **Batter poses — later:** a separate pose per shot (16 + leave), even if some are
  close to others. Currently six poses are shared.
- **Ball sprite — later:** give it a seam; animate the seam spinning for spinners; for
  swing bowlers, make one side lighter (the shiny side), which shows which way it should
  swing — and gives a sneaky visual tell when an outswing bowler slips in a surprise
  inswinger.
- **Ball colour:** red for now. Later: a player-selectable option (red / white / pink),
  not tied to mode or format. Cheap to do (palette swap); check visibility against the
  pitch, sightscreen and fielders for each colour.

## Game mode

**Decided — Quick Play (single batter, endless):** one batter faces deliveries until out.
Score as many runs as possible. High score saved.

**Planned modes:**
- **Quick Play** — 1 wicket, bat until you're out, high score saved. (Phase 3.)
  Two random bowlers per innings, one from each end, swapping every over. High score
  saved per control scheme (Pro is harder, so it gets its own).
- **Over cards:** before each Quick Play over (including the first), a banner card
  "OVER n" / bowler name. After an over, the last ball's banner shows first.
- **High scores per control scheme — open.** The user isn't sure about separate bests.
  Alternative: one best (or a leaderboard) that records *which* scheme it was set with,
  shown off like Mario Kart Wii shows motion-control wins: an incentive to master Pro.
- **Field view (Phase 3, first pass):** after a shot into the field (runs, 4, 6, outfield
  catch), follow the ball briefly, fade to a top-down view of the whole ground and act
  the result out: a fixed 5-4 field (slip, gully, point, cover, mid off | mid on,
  midwicket, square leg, fine leg; plus keeper and bowler), the nearest fielder to the
  ball's path chases / cuts off / catches, the batters run. The result is decided at
  the hit as before; **later** the fielders could decide it (gaps, misfields, run outs),
  and field settings could vary per bowler / match situation.
- **Practice** — the Phase 2 nets as their own mode: pick the bowler (L/R), endless,
  score shown as runs/wickets. **Open:** which batting aids (timing bar, shot rating,
  "what the ball was") stay in the real modes. For now both modes have all of them.
- **Match: T20 / ODI / Test** — 10 wickets (a batting line-up), over limits of 20 / 50 /
  unlimited. Before the match, choose:
  - **Pitch / conditions:** Random (default), or a specific one (green, dry, flat,
    cloudy... see "Pitch and weather conditions").
  - **Bowlers:** Random (default: a generated attack), or a **famous bowling attack**.
- Later, maybe: chase-scenario ladder ("18 off 12, 2 wickets left").

**Famous bowling attacks** (one per nation ideally — strong *and* memorable; names in
Ricky Ponting Cricket-style misspellings). Pencilled in:
- **05 ENG:** Flintoff, Jones, Harmison, Hoggard, Giles(?)
- **07 AUS:** McGrath, Lee, Warne, Clark, Symonds
- **8X WI:** the famous quartet — Roberts, Holding, Garner, Marshall — plus Croft as the
  fifth. (Peak years overlap around 1980-83; the label can stay "8X".)
- **9X PAK:** Wasim, Waqar, Imran, Mushtaq, Aaqib. (Waqar missed the 1992 World Cup, so
  it's a 90s dream attack rather than the '92 side — you have to have Wasim *and* Waqar.)
- **96 SL** (World Cup): Vaas, Wickramasinghe, Muralitharan, Dharmasena, Jayasuriya
- **12 SA** (No. 1 Test side): Steyn, Philander, Morkel, Kallis, Tahir
- **21 NZ** (inaugural World Test Championship winners): Southee, Boult, Jamieson, Wagner,
  de Grandhomme. (A Hadlee-era side was considered, but Hadlee aside the attack is less
  memorable.)
- **24 IND:** Bumrah, Ashwin, Jadeja, Siraj, Kuldeep

### Mode rethink (user, Phase 3 overnight ideas — still open)

- **Quick Play:** settled. Single wicket, bat until out, random bowlers and conditions.
- **Custom:** choose the format and conditions. The famous attacks could live here, in
  Challenge, or both.
- **Challenge:** the famous attacks, each with a fixed format and conditions, chasing an
  iconic total. Spread the challenges across formats. Ideas so far:
  - **05 ENG:** Test, green pitch, overcast. Chase 282 or 420.
  - **07 AUS:** Test, hot, flat pitch. Chase 557 or 648.
  - **24 IND:** T20 (their T20 World Cup win, deliberately, despite the Test attack).
    Chase 176. Conditions TBD.
  - Leaning: AUS, ENG, NZ (WTC) as Test; SA Test (maybe T20); SL and PAK as ODI; WI
    Test or maybe ODI.
- **T20 / ODI / Test (Custom and Challenge) need two batters at the crease** (Quick Play
  stays single-batter):
  - rotating the strike
  - confidence and fatigue tracked per batter
  - a full batting card that pages or scrolls, given the small screen
- **Player-team batter names — open:** generic ("Batter 1"), Ponting-style misspellings,
  or player-editable. Name entry on a GBA needs an on-screen keyboard. That's doable,
  but it's a chunk of UI work.
- **Unlocks:** beating a team's Challenge unlocks their attack in Custom. Locked
  attacks still show when scrolling the options, greyed out with a lock symbol.

### Phase 3 playtest 1 (field view, banners, over cards)

The user tested in mGBA and on the RG SP.
- No crashes. Fielder speeds look fine. "Fast bowler, short ball, pull shot, FOUR —
  felt so good!" First 50 took about 30 minutes, and a milestone banner was missed.
- **Result banner came up too fast.** It now waits 30 frames after the ball is dead,
  or 15 frames after the moment in the field view. Bowling waits for it.
- **The cut to the field view was too quick.** It now follows the ball for 45 frames
  before fading, not 20. The fade back was good.
- **The keeper should come up to take throws when standing back.** He now jogs up at
  6 m/s once the ball's hit.
- **No 3s ever.** The outcome tables had no 3 at all, and any ball through the
  infield seemed to go for 4. 3s are now added (chased down short of the rope). Ground,
  score 5: 45% four, 25% three, 30% two. Score 4: 10% three, 45% two, 45% one.
  Lofted, score 4, not caught: 30% three.
- **Quick Play shows no aids:** no shot / timing / "what the ball was" lines and no
  button hints mid-innings. The timing bar stays, moved lower. Practice keeps them all.
- **START = menu, not SELECT.** In Quick Play that's only between innings, for now.
  Practice: SELECT cycles the bowler, since L/R are batting buttons and were changing
  the bowler by accident.
- **Sound too quiet on the RG SP; some shots seemed silent.** The bat sounds had half
  the loudness of the crowd (short clicks with low energy), and "clean" (middled drives)
  was the quietest. The bat sounds are now soft-clipped up to the crowd's loudness, and
  the playback volumes are raised.
- **Only 4 bowler types.** There had only ever been 4. A right-arm outswinger is now
  added (122-135 km/h, full, swinging away).

### Phase 3 playtest 2

- Transitions, banners, the keeper and 3s all feel better. Sound is louder in mGBA; still
  to test on the RG SP.
- Quick Play without the text feels empty, but it's the right call: the aids belong in
  Practice.
- **Timing bar:** it now slides up from behind the scoreboard strip. That needs no
  masking: the strip is bg priority 2 and the bar's sprites are priority 3. It appears 20
  frames after the shot, or for a shot into the field once the batting view is back.
- **Pitch marker:** it used to be lost over the field view and is now restored on the
  way back. Banners now move off the marker: below it for full balls, above it for
  shorter ones. This is an experiment, to see how it feels.
- **Practice length zones:** the pitch has TV-style length zones painted on (yorker
  blue-purple, full yellow, good green, short red, out to 10 m for pace and 8 m for spin).
  They're two pre-rendered pitch backgrounds, swapped by bowler type.
- **Outswinger:** feels fine and gets lots of caught behinds. **Open:** the game seems to
  want you to play him straighter than instinct says. The cause:
  - the line is classified where the ball passes the crease, not where it's heading
  - an outswinger pitched middle-and-off often still counts as "on the stumps"
  - the rating table gives a cover drive to a full ball on the stumps a 1 (risky, narrow
    timing window); a straight drive gets a 3 (ideal)

  That's defensible cricket ("don't play across the line"), but it may feel wrong
  against a ball you can see swinging away. Options if it still bugs after more play:
  - a cover drive to FULL/STUMPS gets a 2
  - judge the line a little past the stumps for swinging balls
  - a per-bowler "drive-able" bias
- **Reverse sweep feels easier than the sweep** (playtest 3, early impression; first ball,
  full outside off, reverse swept for 4). **Open; needs more play.** Likely the same root
  cause as the outswinger:
  - the only spinner is a leg spinner, which turns from leg to off for a right-hander
  - the line is classified at the crease, after the turn, so most balls count as "outside
    off" or "on the stumps"
  - the reverse sweep's best cells are outside off; the sweep's 3s need "leg side", which
    a leg spinner rarely gives you

  Options if it holds up: rate the sweep 3 on the stumps for full balls; judge spin's line
  where it pitches; or wait for an off spinner (turning into the pads), where the sweep
  should come into its own.

### Phase 3 playtest 3 (real hardware: GBA SP, GBA micro, RG SP)

- Timing bar slide: "perfect, very smooth". Length zones: good.
- The original GBA SP's screen is very dark (unlit). The game is fully playable on it;
  the only problem is seeing the ball in the field view. **Not designing for it**; it's
  fine on the GBA micro and RG SP.
- **Banners dodging the marker could cover Practice's info lines.** In Practice they now
  go above the marker instead, whenever going below would reach the lines.
- **Quick Play shows the shot played again** ("LOFTED COVER DRIVE"), under the bowler
  line top left. The little batter's poses can't show every shot clearly. There's no
  rating or timing; that stays in Practice.
- **Done — sweeps against spin:** front foot + up-right = SWEEP, up-left = REVERSE
  SWEEP, against spinners only (instead of the leg glance / guide). Back foot is unchanged
  (hook / late cut). In Basic the game picks the front foot for full and good-length spin,
  so the sweep comes out naturally there. Each has its own pose (down on one knee, bat flat
  to the leg / off side).
  - Ratings (rows yorker / full / good / short; columns wide off / off / stumps / leg):

    | Shot | Yorker | Full | Good | Short |
    |---|---|---|---|---|
    | Sweep | 0 0 1 1 | 0 1 2 3 | 0 1 2 3 | 0 0 1 1 |
    | Reverse sweep | 0 0 1 0 | 1 2 2 0 | 1 3 2 0 | 0 1 0 0 |

  - Directions: the sweep goes behind square on the leg side (110°), the reverse sweep
    behind point (−115°).
  - **Top edges:** 30% of mistimed sweeps (EARLY / LATE) are top-edged high to fine leg /
    short third man and caught. Cross-batted shots should carry their own risk.
  - To playtest: does it feel worth the risk? Is 30% too harsh?

## Camera / presentation

- **Decided — batting view:** behind the bowler, broadcast style. Pitch is a static
  perspective background; ball is tracked in 3D and projected.
  - Ball height shown with a **shadow sprite** on the ground + ball sprite raised by height.
  - Ball is scaled by distance (affine sprite). Note: from behind the bowler the ball
    travels *away* from the camera, so it shrinks slightly towards the batter. A long,
    telephoto-style camera keeps the change small and makes the batter's end bigger.
  - Camera constants live in `include/ca_camera.h`; the pitch background is pre-rendered
    with the same maths by `tools/make_placeholder_graphics.py`.
  - `GAME_SPEED` (in `include/ca_world.h`) slows everything down for playability:
    real pace gives ~0.45 s to react; currently 0.6 (~0.8 s for pace, ~1.3 s for spin).
- **Open — after contact:** how to show where the ball goes and the fielders.
  Must not feel jarring. Candidates (prototype and compare):
  - (a) Follow the ball in the batting view until it leaves the screen, *then*
    transition to a zoomed-out top-down field view (ball travels, fielder chases).
    Current favourite.
  - (b) Stay in batting view + a minimap / radar in a corner.
  - (c) Both: radar during play, top-down view for boundaries/catches.
  - Transition style also to test: hard cut vs quick fade vs scroll/zoom.

## Running

**Decided (for now) — auto run.** The game decides runs from where the ball goes and how
the fielders get to it. Fits the arcade feel. A manual "run / don't run" input could be
revisited later if outcomes feel too out of the player's hands.

## Fielders

- In the zoomed-out view, fielders are **plain squares or basic sprites** — no animation,
  they just slide towards the ball.
- **LATER — dynamic fields:**
  - Each bowler profile has a **default starting field**.
  - The field **changes only at the start of an over** (never ball-to-ball), based on
    recent scoring — e.g. lots of runs through the off side → move a fielder across.
  - At the start of each over, show the **zoomed-out field view** before going into the
    batting view, so the player sees the gaps. This also supports the fairness pillar:
    a catch in the deep is your fault if you saw the fielder there.

## Event presentation

Animations for events: boundary (4/6), wicket, end of over, change of bowler, milestones
(50, 100).

Suggested approach: **banners** (slide in over the play view, ~1 second) for most events,
since they keep the game moving; reserve **full-screen** for the big ones (wicket,
milestones). All skippable with A. **Open** — tune once there's something to look at.

## UI / HUD

- **Scoreboard along the bottom**, TV-broadcast style. Quick Play: runs and balls faced
  (plus current bowler). Later modes: full score (runs/wickets, overs, target).
- **Meters along the top** (confidence and fatigue), so they don't compete with the
  scoreboard. **Open** — revisit once both exist on screen.

## Ball model

The ball has a 3D position: `x` (across the pitch), `z` (down the pitch), `h` (height).
All maths in fixed point (`bn::fixed`).

Each delivery is defined by: speed, release point, **line** (x at the batter),
**length** (z where it bounces), and movement.

| Bowler | Speed | In the air | After bounce |
|---|---|---|---|
| Pace | fast, flat | swing (curves whole flight) | small seam deviation |
| Spin | slow, loopy (more height) | slight drift | big turn |

Tells for the player: release point, run-up, optional pitch-spot marker on easy difficulty.

**Bowler sprite needed:** without a bowler the ball appears from nowhere ~2 m up, which
projects to well up the screen and *feels* too far down the pitch. A bowler sprite with a
run-up and delivery stride (the ball visible in the hand before release) should fix it.

### Bowlers — later (Phase 4+)

- **Teams and kit — later:** let players choose their team / kit colours.
- **Pre-made bowling attacks — later:** themed by country and era (e.g. "2005 England",
  "2007 Australia", "1980s West Indies"): the right mix of pace, swing and spin for
  that side. Names: affectionate Ricky Ponting Cricket-style misspellings of the era's
  players (user's call). Could drive a **challenge mode** (e.g. "survive 2005 England").
- **The current `bowler_profiles` are temporary.** They become the *archetype templates*
  (fast seam, off spin, leg spin, left-arm swing, ...) that random bowlers are generated
  from; pre-made teams are hand-written profiles in the same format.
- **Pitch and weather conditions — later:** variety per game. Green pitch = more seam,
  cloudy = more swing, dry = more spin (and maybe bounce: hard/bouncy vs slow/low).
  Fits the profile model as multipliers on air_movement / bounce_movement / restitution.
- **Leg-side wides** (limited-overs rule, which suits our T20/ODI-style play): anything
  passing down the leg side, missing the batter, is a wide. Bowlers' bad balls on the
  pads should then stay just inside that line.
- **Wides and no balls — later (needs a scoreboard / extras, Phase 3+):** occasional real
  wides (past the wide line, +1 run, ball re-bowled) and a very rare no ball (~1 in 100)
  — possibly with a free hit, which suits the arcade feel.

Start with generic pace and spin. Later ideas:
- **More bowler types:** split pace into swing and seam bowlers; swing into
  inswing / outswing. Split spin into off spin / leg spin.
- **Left- and right-arm** bowlers, **over / round the wicket** (`arm` and `side` fields
  exist; release side is derived from them). Later: bowlers switching sides mid-spell.
  Note: `release_width` is *not* derived from the side — round the wicket should be
  ~0.25 m wider (the body sits between arm and stumps), so changing `side` means also
  changing the width. Fine while sides are fixed; fix it (derive the round-the-wicket
  offset in `release_x()`) if bowlers ever switch sides. Documented in `ca_bowler.h`.
- **Bowling attack per game:** at the start of each game, (semi-)randomly pick ~5
  bowlers and rotate them in realistic spells. Gives each run variety.
- **Names:** made-up, nostalgic-style player names (initial + surname, e.g. "M Hyder").
  The profile's `name` is display-only; arm/type/side are explicit fields.
- **Randomly generated bowlers:** take a base archetype profile, vary its stats randomly
  (variation size tied to difficulty), and generate a name from first-name / surname
  tables. Pairs well with the per-game attack.
- **Special / change-up balls:** each bowler has one or more signature variations used
  occasionally: slower ball, bouncer, yorker, wrong'un, arm ball, etc. Fits the profile
  model as a small list of variation profiles (overrides on speed/length/movement) plus
  a percent chance.
- **Difficulty / bowler skill:** higher difficulty → faster pace, more swing/seam/turn,
  fewer bad balls (half-volleys, long hops, wide ones). Could also ramp over a run.

Implication for now: a delivery should be generated from a **bowler profile** (data:
type, arm, speed range, movement amounts, accuracy) from day one, even if there are only
two profiles, so all of the above is just adding data.

## Batting

### Shot-vs-ball matrix (core idea)

Where the ball arrives at the batter is bucketed into zones (line × length), e.g.

- Line: wide outside off / outside off / on the stumps / leg side
- Length: full / good / short

For **every shot**, every zone has a **timing window** (how forgiving) and an
**outcome profile** (how good the result is when timed well, and how risky when not).

Example: full & outside off →
- front-foot cover drive: big window, good outcomes
- back-foot pull: tiny/no window, high edge/miss risk

So playing the right shot to the right ball is rewarded; the wrong shot is still
*possible*, just risky. This is the main skill lever and should live in a single,
easily-tweaked data table.

### Controls — **Open**, to be decided by playtesting

Inputs available: D-pad, A, B, L, R (Start/Select for menus).

Things the controls might express: shot direction, footwork (front/back), attack vs
defend, leave, power/loft.

Candidate schemes (build several, switch between them via a debug menu):

- **Scheme 1 — Direction only.** D-pad = shot direction (no footwork; footwork is
  implied by the zone). A = attack, B = block, nothing = leave. Simplest.
- **Scheme 2 — D-pad footwork, shoulders direction.** Up/Down = front/back foot,
  L/R = off/leg side, neither = straight. A = attack, B = block.
- **Scheme 3 — D-pad 8-way.** Up = front foot, Down = back foot, diagonals/left/right
  = direction (e.g. Up+Right = front-foot off drive). A = attack, B = block,
  R held = loft/power.

Current lean: **Scheme 2**, but try all three. Possibly ship more than one as a
player-selectable option (e.g. Scheme 1 as "Simple").

**Architecture (keeps multiple schemes cheap):** every scheme is only an *input mapping*
that produces the same thing — a **shot intent** (shot type, e.g. front-foot cover drive /
back-foot pull / block / leave, plus the frame the shot was played on). The timing table
and outcome logic only ever see shot intents, so they don't know or care which scheme
produced them. Adding or removing a scheme never touches the timing mechanics.

Scheme 1 has no footwork input, so it picks the footwork for you based on the ball —
effectively an assisted/easy mode.

### Batting model (Phase 2 implementation)

- Pressing A/B *is* the moment of contact. Timing error = press frame − frame the ball
  reaches the bat. Front-foot contact is at z 18.3, back-foot at 19.4, so the back foot
  gives ~3 more frames against pace (and less margin before the stumps).
- Timing windows by rating (frames, either side): ideal 2/4/7, good 1/3/6, risky 0/2/5,
  wrong –/1/4 (perfect/good/mistimed). Defence gets +6 early / +2 late (being early on a
  block rarely matters; being late gets you bowled or caught behind).
- Being late on a ball that's hitting the stumps gets you bowled (the ball gets there
  first). Late on anything else still makes contact — "playing late" off the back foot
  is fine.
- Mistiming steers the ball: early → squarer on the leg side, late → finer on the off side.
- Outcome score = timing (perfect 3, good 2, mistimed 1) + rating (0–3), mapped to
  runs / edges / catches; lofted shots have higher reward and catch risk.
- Early presses wait for the ball (hit at arrival); on-time/late presses hit it at once.
- Not yet: LBW, wides/no-balls, full tosses.
- **LBW (maybe later):** not required. If added, fake it: e.g. a leave/miss that would
  have hit the stumps, pitched in line (not outside leg), hitting the pads.
- **Run-outs:** no, unless manual running comes back.

### Controls — playtest notes (Phase 2)

- Up = back foot, Down = front foot (the batter faces the camera, so forward is *down*).
- Scheme 1 and 3 feel simple/intuitive; 3 feels most arcade and its loft (R) is liked.
  Scheme 2 feels the most precise. Likely keep more than one long term.
- **Decided — two schemes:**
  - **Basic Controls** (was scheme 3, default): D-pad 8-way. Left/Right = off/leg,
    Up/Down = back/front foot, neither = the game picks the footwork. A hit, B block,
    hold R = loft. (Absorbed scheme 1, which was removed.)
  - **Pro Controls** (was scheme 2): Up/Down = back/front foot (neither = front),
    L/R = off/leg (neither or both = straight). A hit, B block, **A+B = lofted hit**.
    The chord is detected by waiting up to 3 frames after the first press; the shot is
    still timed from the first press, and the stumps can't bowl you during the wait.
    Needs real-hardware testing.
- Loft alternatives considered: decide-before-the-ball (not loved), hold-and-release
  for power/loft (later experiment, could tie into fatigue).
- Replay button removed.
- Blocking may now be a little too easy — revisit.
- Needs testing on real hardware (RG SP).
- **Real-hardware playtest (Miyoo Mini Plus):** Basic feels great (33 off 24 first
  innings); real buttons feel far better than keyboard. Pro: more precision and decision
  making, but three separate inputs (D-pad foot + shoulder side + A timing) is "pat your
  head and rub your tummy". Good when it works, but when it doesn't you freeze — e.g. a
  short wide ball begging to be cut came out as a straight drive off the wrong foot.
  Middling one on Basic felt better than on Pro. **Verdict:** Basic is in a good place
  (can still improve). Keep two options, Basic and Pro, but Pro needs rethinking —
  probably fewer simultaneous inputs at the moment of the shot (scheme 4 puts the
  decision into *which* button you press, so it's two inputs, not three).
- **To fix (hardware playtest):**
  - *(Fixed)* Input ignored for 45 frames after each ball (a very late press bowled the
    next ball).
  - *(Fixed)* *Caught-behind looks wrong:* the edge is sent ~20-25 degrees off straight-back (fine
    leg / third man) at full speed and keeps rolling, as there's no keeper to catch it, so
    it reads as "ran away to the side" while the banner says CAUGHT BEHIND. Fix: aim
    catches at the keeper / slips cordon and stop the ball there (a "caught" stop); leave
    wider angles for edges that are *not* caught (those should run away for runs).
    Same for lofted CAUGHT: end the flight at the catch, don't let it land and roll.
  - *(Done)* *Bad balls too bad:* the loose ball is uniform over length -0.5..11 m and line
    -1.2..+0.8 m (before swing/seam), so it's usually a very short, very wide ball —
    unplayable rather than a gift. Fix: pick from a few *hittable* kinds instead —
    half-volley (1.5-3 m, around the stumps), long hop (8-10 m, around off), width
    (good-ish length, 0.4-0.8 m outside off), on the pads (full, just down leg).
    Real wides are a separate thing (below).
  - *(Done)* *Blocking should depend on the line* (user): blocking a wide ball = play and miss;
    blocking on 5th stump (OFF zone) = real edge risk; on the stumps = safest. Makes
    **leaving** the right answer to wide balls, which is cricket's core off-side decision.
    Combine with the timing-grade risk below.
  - *(Done)* *Mistimed blocks too safe:* an early (yellow / GOOD) block can't get you out, because
    GOOD timing + any non-zero shot rating already scores 3 = safe, and defence windows
    are already widened. Idea: base defence risk on the timing grade more than the score.
    Early = leading edge popping up (return catch / bat-pad); late = thin edge to the
    keeper. Ties into "blocking too easy".
- **Open questions (after night 1, no change yet — test on hardware first):**
  - Basic and Pro have drifted close together (main difference: L/R shoulders vs D-pad
    left/right for side). The original intent was one scheme with *auto* footwork and one
    where you choose it. Revisit which two we actually want.
  - Neither scheme can express footwork *and* a contradicting aim on one axis, e.g. a
    **back-foot cover drive** (back foot = Up, but cover is "forward-ish off side") or a
    **front-foot leg glance** (front = Down, but fine leg is "backward leg side"). Today
    direction is only off / straight / leg, so the shot table can't hold these yet anyway.
    Needs a finer direction model (forward/square/fine on each side) and a control
    mapping that separates footwork from aim.
  - **Scheme 4 idea:** D-pad = aim only (8-way); footwork on the buttons:
    A = front-foot attack, R = back-foot attack, B = front-foot block, L = back-foot block.
    Cleanly solves the axis clash above. Unsure about the blocks on B/L.
    *Later verdict (user): probably too complicated / unintuitive — same problem as Pro.*
- **Basic v2 proposal (user, after a few days' thought):** the player picks only a
  *direction* and plays the shot; the game picks front or back foot, whichever suits
  the ball better. Frees the D-pad's up/down axis for direction, which unlocks the
  missing shots (back-foot cover drive, front-foot leg glance...): ~8 directions x 2 feet.
  - Key insight: the batter faces the camera, so the D-pad maps *exactly* onto the field
    as seen on screen. Down = back past the bowler, down-left = cover, left = point,
    up-left = third man / late cut, up-right = fine leg / glance, right = square leg,
    down-right = midwicket. Up = behind the keeper (ramp — nothing sensible for now),
    so **Up could be block** (user idea), which frees B.
  - Freed B could become **loft** (A = along the ground, B = in the air), dropping R:
    a two-button arcade scheme. Or keep B as block. To decide in testing.
  - Skill shifts from "pick the right foot" to "pick the right *direction* for the
    line and length" (cutting a full straight ball is still wrong). Watch it isn't too easy.
  - Pro could then be Basic v2 plus an *optional* footwork override (e.g. L/R shoulders =
    back/front foot): precision when you want it, ignorable when you don't — so you
    can't tangle your thumbs.
- **Decided & built — three schemes for now** (SELECT between balls cycles them):
  - **Basic V2** (default): D-pad = direction on the field, Up = block, A or B = play
    the shot, hold L or R = lofted (changed from B = loft, to match Pro V3). The game
    picks front/back foot (better-rated one).
  - Between balls, A or B bowls the next ball (pressing B after a front-foot shot in
    Pro V3 kept not working).
  - **Pro V2**: Basic V2 plus hold **L = front foot, R = back foot**, then A/B to play and
    time it. **Neither or both held = "stuck on the crease":** the game picks the better
    foot, but the shot is rated one notch worse (shown as "ON CREASE" in the feedback).
    Chosen over a coin flip: still a shot, deterministic, cricket-flavoured, mildly
    punishing. Revisit in playtesting.
  - **Basic V1 (old)**: kept for comparison; will probably go.
  - Old Pro (A+B chord loft) removed, along with the chord wait / stumps suspension.
- **Playtest (laptop):** Basic V2 is the favourite; Pro V2 still doesn't feel as good.
  Basic V2 feels a lot easier (the game always takes the better of two shots) — may need
  tighter ratings / windows. It also chose back-foot shots for balls that looked full
  (good length): fixed by toning down back-foot ratings at good length and breaking
  ties by the real length (forward if fuller than 5.5 m).
- **"Stuck on the crease" removed:** with Pro V3, A and B on the very same frame is
  rare and can't be ordered, so the game just picks the foot from the length (as Basic).
  No penalty. (It was really a Pro V2 rule, for holding neither shoulder.)
- **Basic difficulty — decided (for now):** Basic is probably still slightly too easy,
  but leave it. Difficulty will rise for *both* schemes as mechanics arrive (fielders,
  confidence/fatigue meters, harder bowlers), and Pro will additionally reward correct
  foot selection on top of that. Don't artificially hamper Basic.
- **Basic difficulty — earlier thinking:** the game picking the foot makes Basic easier,
  but it's also what makes it do what the player *meant* most of the time. Plan: if both
  schemes feel too easy, that's global tuning / difficulty levels (Phase 4: bowler skill,
  windows), not a Basic handicap. Only add a Basic-specific knob if Basic alone is too
  easy relative to Pro. Fielders, meters and harder bowlers will also add difficulty.
- **Basic difficulty — decided to try:** instead of tighter timing windows (an artificial
  handicap), Basic's auto footwork now goes by **length only** (front if fuller than
  5.5 m pace / 4 m spin), not "whichever rates better". Usually the same; on in-between
  balls it can be the lesser shot, and Pro players can beat it by choosing. Pro's reward
  = it can do things Basic can't. Tighten windows only if Basic is still too easy.
- **Hardware playtest (RG SP, train):** happy with **Pro V3 and Basic V2** for now (minor
  tweaks later). Short balls have a big sweet spot on Pro V3 — feels rewarding and
  realistic (punish the bad ball). Lofted risk/reward feels a bit off, but wait for
  fielders and the meters before tuning it.
- **How the user tests ratings** (worth keeping): (1) bat normally, and note any ball where
  the shot felt right but the timing bar disagreed, with the reasoning; (2) play one fixed
  shot for 15-20 balls and judge each outcome, using the bar's colours and their widths.
- **Rating changes from those notes:**
  - *Pace vs spin:* spin now has its own, shorter length zones (yorker < 1.25 m, full
    < 3, good < 4.75, short beyond; pace is 2 / 4 / 6.5), and auto footwork goes back
    from 4 m (pace 5.5 m). The same spot pitched by a slow spinner is easier to go back
    to. One ratings table still serves both.
  - *Leg glance and clip:* best on the pads / down leg, risky (1) on the stumps,
    useless outside off. (Flick renamed to **Clip**.)
  - *Pull, short balls:* wide outside off = 0 (cut it instead; reaching for a pull is
    how you sky one), outside off = 3 (pull or cut), on the stumps = 3, down leg = 2
    (pull is fine but the hook is the shot).
- **Pro V3 (built):** D-pad aim, Up = block. **B = front foot, A = back foot** —
  the button picks the foot *and* times the shot, so it's two inputs, not three. Hold
  L or R = lofted (held in advance, not timed). A+B together = on the crease. B sits
  lower on the GBA, so lower button = forward = down the screen.
- **Shots — 16 rated + leave.** Direction x foot:

  | D-pad | Front foot | Back foot |
  |---|---|---|
  | down / none | Straight drive | Back-foot punch |
  | down-left (cover) | Cover drive | Back-foot drive |
  | left (point) | Square drive | Cut |
  | up-left (third man) | Guide | Late cut |
  | up-right (fine leg) | Leg glance | Hook |
  | right (square leg) | Clip | Pull |
  | down-right (midwicket) | On drive | Slog |
  | up | Forward defence | Back-foot defence |

### Timing feedback

- After the shot: a colour-coded timing bar (red miss, orange mistimed, yellow good,
  green perfect) using the actual windows for the shot played, with an arrow at your press.
- **Live timing bar** (toggle with B between balls): thin track along the bottom, ticks
  at the front-foot and back-foot contact moments, marker moving with the ball. Can't be
  shot-specific (no shot chosen yet). An assist: candidate for training / easy only.
- Playtest verdict on the live bar: not helpful — the game is too fast, and looking down
  at it makes you late. It can't be tracked in peripheral vision. **Removed.**
  The after-the-ball timing bar stays (maybe toggleable later).
- **Next to try — a cue on the ball itself**, so your eyes stay on the ball. The ideal
  moment is known in advance (from the delivery preview), but a cue *at* that moment is
  too late to react to (~12 frames of reaction time). So it must be anticipatory, e.g.
  an "approach ring" around the ball that shrinks and closes exactly at the ideal frame
  (rhythm-game style), or a flash a fixed number of frames before.
  Which moment: front- and back-foot contact differ (~3 frames for pace, ~6 for spin);
  could use the contact moment of the best-rated shot for that ball (which also hints at
  the right footwork — fine for a training / easy assist).
  User concern: an approach ring may be *too* helpful and make it feel like a rhythm game
  (watching the ring rather than the ball's trajectory). Not sold — keep looking for
  something subtler / more "cricket".
- **Sound — likely the best cue of all (later):** a "thock" when the ball bounces,
  plus bat-on-ball and stumps sounds. It's the cue real batters use, it's on every
  difficulty without being an "assist", and it keeps your eyes on the ball. Needs the
  Makefile `AUDIO` dir set up (Butano supports Direct Sound samples + Maxmod).
  **Bounce pitch by length:** play one bounce sample at a varied speed
  (`sound_item::play(volume, speed, panning)`): deeper thud for short, higher for full.
  Drive it from the ball's downward speed at the bounce, not length directly, so it falls
  out of the physics (short balls hit the pitch steeper and harder). Volume can scale too,
  and panning can follow the line. A sonic pitch map — teaches length by ear.
- **Sound playtest (Phase 3):** bounce pitch by impact works; stumps and crowd loved;
  whoosh on a play and miss "perfect". Crowd: boundaries get a delayed (0.5 s four,
  0.67 s six), quieter cheer; **wickets are the biggest cheer**.
  **Hit sounds by shot.** Five middled-crack candidates were auditioned; the user liked
  all of them and word-associated each with a shot, so each became one:
  | Contact | Sound | User's association |
  |---|---|---|
  | mistimed (early/late) | bat (tock) | — |
  | good timing | solid (old crack) | "okay hit, not an edge, not middled" |
  | middled, ground, leg side | thwack | "good hit along the ground, leg side" |
  | middled, ground, point / third man | pock | "cut shot, middled" |
  | middled, drives (and lofted off side) | clean | "lofted drive; so well hit it doesn't make as much noise as it should" |
  | middled, lofted, leg side | thock | "BIG 6, slow sweep over cow corner" |
  | attacking shot that's caught | mishit (rifle) | "big slog, mis-hit, probably caught" |
  Outfield catches cheer at the predicted catch time (the ball leaves view first).
  **Tried and rejected:** an 8-bit "ball in the air" whistle (PSG channel 3 triangle wave,
  pitch following height, NES-style): instant "Nope!" from the user. Sounds called done.
  The mis-hit sound gives the catch away a beat early, as in real cricket. Idea: per-shot
  sounds could later follow the actual shot (e.g. sweep) rather than just direction.
- **Training aid: colour-coded length zones on the pitch**, like TV pitch maps
  (full / good / short bands). Teaches reading length, which is what the shot choice
  really depends on, without telling you *when* to swing. Easy: pre-rendered pitch
  variant, toggled as an option.

### Later ideas from Phase 2 (not doing now)

- **Ramp and uppercut = "lofted block"** (user): Up + loft is currently just a block, and
  Up is the one direction with no shot (straight behind). Make it the cheeky shots:
  front foot = **ramp / scoop** over the keeper (full balls, on the stumps / legs),
  back foot = **uppercut** over the slips / third man (short and wide outside off).
  Always aerial, high risk / high reward (top edge, caught behind or at fine leg / third
  man). Needs 2 more shots in the table (18 rated), two rating rows, angles ~180 and
  ~-150, and fits the existing schemes with no new buttons (Up + L/R held).
- **Lofted shots that go along the ground:** a lofted shot that isn't a six or a catch
  gets the plain "runs" / "four" hit (1 m/s upwards), so it skids along the ground. The
  result is decided first and the flight is picked from the *result*, not from whether
  you lofted it. Real mistimed lofted shots do happen, but they'd look more like a
  one-bounce lob, a skier that drops short, or a flat hit that bounces before the rope.
  Fix when fielders arrive (aerial vs ground then matters for catches): give lofted
  results their own, higher hit profiles (e.g. lofted four: up ~7 m/s, lands then runs).

- **Show what you pressed** after the ball — decided the shot description is enough.
- **Continuous hit angle:** each frame early/late turns the ball a few more degrees
  (currently a fixed +/-25 for EARLY/LATE), plus a little from the ball's line. Revisit
  with fielders, when the angle starts to matter.

### Settings — later

A settings / options screen, whether or not these are also tied to difficulty:
after-ball timing bar on/off, pitch-map length colours (off / after the ball only / during
live play), ball colour, control scheme.

### Feedback (make outcomes feel fair)

After each ball, briefly show *why* it went the way it did. Ideas:
- Timing indicator: EARLY / GOOD / PERFECT / LATE (text or a small bar).
- Shot rating: "Good shot choice" vs "Wrong shot for that ball".
- Pitch map: where it bounced and where it arrived at the batter.
- Edge / play-and-miss animation clearly distinct from a clean hit.

## Meters — added after the core feels good

- **Confidence:** rises with time at the crease and good shots; falls on play-and-miss,
  edges. Effect: **widens timing windows**.
- **Fatigue:** rises when scoring — more for more runs (running a 3, swinging for a
  boundary). Falls when blocking/leaving. Effect (first idea): **reduces shot power**
  (fours become caught-in-the-deep), not timing.
- Interesting tension: a boundary raises both confidence (easier) and fatigue (weaker).
  Blocking is how you recover.
- **Open:** does fatigue need an explicit power input (so fatigue from big swings feels
  like your choice)? Does fatigue also affect running?
- Display: two small bars along the top of the screen (see UI / HUD).

### Achievements — Add these later. Phase 6

| Name | Achievement | Notes |
| --- | --- | --- |
| Start again! | Make a 50 in any game mode | Individual score, either in Quick play or for a single batter in Custom/Challenge |
| Raise the bat! | Make a 100 in any game mode | Individual score, either in Quick play or for a single batter in Custom/Challenge |
| Double Up! | Make a 200 in any game mode | Individual score, either in Quick play or for a single batter in Custom/Challenge |
| Triples is best | Make a 300 in any game mode | Individual score, either in Quick play or for a single batter in Custom/Challenge |
| Lara who? | Make a 400 in any game mode | Individual score, either in Quick play or for a single batter in Custom/Challenge |
| Quack! | Make a duck in any game mode | |
| Quack quack! | Make a golden duck in any game mode | Don't count a golden duck for both achievements |
| TBD | T20 High Score | 344 |
| TBD | ODI High Score | 498 |
| TBD | TEST High Score | 952 |
| Good start! | A 100-run opening partnership. | Custom or challenge mode |
| Get him up the order! | Score 100 with a number 11 | Custom or challenge mode |
| Innovative! | Hit a ramp or uppercut for six | Leave those shots undocumented so players have to discover them |
| World beater! | Beating every classic team in challenge mode | |

These need SRAM space for flags. A bitfield is enough.

## Out of scope (for now)

Fielding/bowling as the player. (Multiple batters, partnerships and Test matches are planned: see "Mode rethink".)
