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
Status: playable practice loop, both schemes work; real-hardware testing under way.
Full notes and reasoning in `docs/design.md` ("Controls — playtest notes",
"Timing feedback"). Candidate next steps, roughly in order:
- [ ] Fix caught-behind / caught: aim catches at keeper/slips/fielder and stop the ball
      there; only uncaught edges run away
- [ ] Mistimed blocks carry real risk (early = leading edge / return catch,
      late = thin edge to keeper)
- [ ] Sound: bounce (pitch from impact speed), bat-on-ball, edge, stumps, crowd on 4/6
- [ ] Pro Controls v2 — prototype "scheme 4" (D-pad aims, A/R/B/L = front/back
      foot attack/block)
- [ ] Show what you pressed in the after-ball summary
- [ ] Continuous hit angle from timing error (and ball line)
- [ ] Decide the fate of the live timing bar; try subtler timing cues
      (length-zone pitch colours, bounce sound)
- [ ] Phase-end: write the Phase 2 section of `docs/learnings.md` with exercises,
      save `roms/phase-2.gba`

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
