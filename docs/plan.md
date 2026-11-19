# Cricket Advance — Plan

End of every phase: update `docs/learnings.md` (with exercises) and save the ROM as
`roms/phase-N.gba`.

Rapid prototyping: get something playable early, play it, iterate. Each phase ends with
a ROM that runs in mGBA and on the RG SP.

## Phase 0 — Hello GBA
- [x] Install devkitPro (devkitARM) and mGBA
- [x] Get Butano (submodule, 21.10.0)
- [x] Our own project: a sprite moving with the D-pad
- [ ] Copy the ROM to the RG SP and run it (deferred — tested in mGBA only)

Learn: build pipeline, sprites, input, the main loop / VBlank.

## Phase 1 — A ball is bowled
- [x] Perspective pitch background (placeholder)
- [x] Ball with 3D position (x, z, h) in fixed point, projected to screen
- [x] Shadow sprite + raised ball; bounce at a length; scaling with distance
- [x] Deliveries generated from a bowler profile (data), randomised line/length
- [x] Press a button to bowl again
- [x] Debug overlay: line, length, speed

Learn: fixed-point maths, affine sprites, backgrounds.

Extras done: B replays the last delivery; L/R switch bowler; pitch marker where the
ball bounced; "hitting the stumps" detection.

## Phase 2 — Hitting it (the main iteration loop)
- [x] Restructure: main.cpp → practice scene + ball/batter views + debug HUD
- [x] Zones (line × length) and shot-vs-ball rating table (`src/ca_shots.cpp`)
- [x] Control schemes (now Basic and Pro), switchable with SELECT
- [x] Playtest round 1 → two schemes: Basic (8-way + R loft) and Pro (shoulders + A+B loft)
- [x] Live timing bar (optional, toggle with B)
- [x] Outcomes: miss, edge, block, 1–6, bowled, caught (no LBW yet)
- [x] Feedback: timing (frames early/late) and shot rating after each ball (text for now)
- [x] Visual timing bar after each shot
- [x] Broken-stumps frame when bowled (batter's stumps are now a sprite)
- [x] Loft for Pro Controls (A+B)
- [x] Playtest on real hardware (Miyoo Mini Plus — Basic feels great; Pro needs rethinking)
- [ ] **Play it a lot. Tune. Pick a control scheme.**

### Phase 2 — where we are / next up (WIP)
Status: playable practice loop with four control schemes (Basic V2 and Pro V3 are the front-runners); real-hardware testing under way.
Full notes and reasoning in `docs/design.md` ("Controls — playtest notes",
"Timing feedback"). Candidate next steps, roughly in order:
- [x] Basic V2 controls: D-pad = direction, Up = block, A ground, B loft; game picks
      the foot. 16 rated shots (direction x foot)
- [x] Pro V2: Basic V2 + hold L/R for front/back foot (neither = stuck on the crease)
- [x] Removed the live timing bar and old Pro (A+B chord)
- [ ] Playtest V2 schemes; decide whether Basic V1 goes
- [x] Fix caught-behind / caught: aim catches at keeper/slips/fielder and stop the ball
      there; only uncaught edges run away
- [x] Pro V3 prototype (B front / A back, shoulder loft)
- [x] Input cooldown after each ball
- [ ] Basic V2 difficulty: tighten ratings / timing windows
- [ ] Bad balls: hittable kinds (half-volley, long hop, width, on the pads), not random junk
- [ ] Block risk by line (wide = miss, 5th stump = edge risk) and by timing grade
      (early = leading edge / return catch, late = thin edge to keeper)
- [ ] Banner between balls still says "PRESS A TO BOWL" — should say A/B (help line already does)
- [ ] Shot ratings: user has notes on FF flick and leg glance (from hardware testing)
- [ ] Show what you pressed in the after-ball summary
- [ ] Continuous hit angle from timing error (and ball line)
- [ ] Phase-end: write the Phase 2 section of `docs/learnings.md` with exercises,
      save `roms/phase-2.gba`

## Phase 2.5 — Sound
- [ ] Set up Butano audio (Makefile `AUDIO`, Direct Sound samples)
- [ ] Bounce: pitch/volume from impact speed, panning from line
- [ ] Bat on ball (clean vs edge), stumps breaking, crowd on 4 / 6

## Phase 3 — Quick Play loop
- [ ] Bottom scoreboard: runs, balls faced; out → game over → restart
- [ ] After-contact presentation: follow ball off screen → field view (test transitions)
- [ ] Simple fielders (squares) that slide to the ball; auto running; caught
- [ ] Event banners: 4, 6, wicket, end of over
- [ ] High score saved to SRAM

## Phase 4 — Depth
- [ ] Placeholder bowler sprite: run-up, delivery stride, ball visible before release
- [ ] Pace vs spin, swing, seam, turn; bowler tells
- [ ] Bowler variety: swing/seam, in/outswing, off/leg spin, left/right arm
- [ ] Per-game attack of ~5 bowlers rotating in spells
- [ ] Difficulty levels (pace, movement, fewer bad balls)
- [ ] Confidence and fatigue meters (top of screen)
- [ ] Difficulty curve over a run (bowlers get harder)
- [ ] Default field per bowler; field changes at start of over based on scoring;
      show field view at the start of each over

## Phase 5 — Polish
- [ ] Pixel art (batter, bowler, pitch, fielders)
- [ ] Title screen, menus
- [ ] Options: ball colour (red / white / pink)
- [ ] Fancier event animations (full-screen wicket, milestones)
- [ ] Sound and music
