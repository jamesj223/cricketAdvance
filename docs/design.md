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
- **Ball colour:** red for now. Later: a player-selectable option (red / white / pink),
  not tied to mode or format. Cheap to do (palette swap); check visibility against the
  pitch, sightscreen and fielders for each colour.

## Game mode

**Decided — Quick Play (single batter, endless):** one batter faces deliveries until out.
Score as many runs as possible. High score saved.

Later, maybe: chase-scenario ladder ("18 off 12, 2 wickets left"), T20/ODI/TEST innings.

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

Start with generic pace and spin. Later ideas:
- **More bowler types:** split pace into swing and seam bowlers; swing into
  inswing / outswing. Split spin into off spin / leg spin.
- **Left- and right-arm** bowlers, **over / round the wicket** (`arm` and `side` fields
  exist; release side is derived from them). Later: bowlers switching sides mid-spell.
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
  - *Caught-behind looks wrong:* the edge is sent ~20-25 degrees off straight-back (fine
    leg / third man) at full speed and keeps rolling, as there's no keeper to catch it, so
    it reads as "ran away to the side" while the banner says CAUGHT BEHIND. Fix: aim
    catches at the keeper / slips cordon and stop the ball there (a "caught" stop); leave
    wider angles for edges that are *not* caught (those should run away for runs).
    Same for lofted CAUGHT: end the flight at the catch, don't let it land and roll.
  - *Mistimed blocks too safe:* an early (yellow / GOOD) block can't get you out, because
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

### Timing feedback

- After the shot: a colour-coded timing bar (red miss, orange mistimed, yellow good,
  green perfect) using the actual windows for the shot played, with an arrow at your press.
- **Live timing bar** (toggle with B between balls): thin track along the bottom, ticks
  at the front-foot and back-foot contact moments, marker moving with the ball. Can't be
  shot-specific (no shot chosen yet). An assist: candidate for training / easy only.
- Playtest verdict on the live bar: not helpful — the game is too fast, and looking down
  at it makes you late. It can't be tracked in peripheral vision. Likely to be removed.
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
- **Training aid: colour-coded length zones on the pitch**, like TV pitch maps
  (full / good / short bands). Teaches reading length, which is what the shot choice
  really depends on, without telling you *when* to swing. Easy: pre-rendered pitch
  variant, toggled as an option.

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

## Out of scope (for now)

Fielding/bowling as the player, multiple batters, partnerships, Test matches, sound.
