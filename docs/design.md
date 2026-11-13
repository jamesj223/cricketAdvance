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
