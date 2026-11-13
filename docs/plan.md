# Cricket Advance — Plan

Rapid prototyping: get something playable early, play it, iterate. Each phase ends with
a ROM that runs in mGBA and on the RG SP.

## Phase 0 — Hello GBA
- [ ] Install devkitPro (devkitARM) and mGBA
- [ ] Get Butano, build one of its examples
- [ ] Our own project: a sprite moving with the D-pad
- [ ] Copy the ROM to the RG SP and run it

Learn: build pipeline, sprites, input, the main loop / VBlank.

## Phase 1 — A ball is bowled
- [ ] Perspective pitch background (placeholder)
- [ ] Ball with 3D position (x, z, h) in fixed point, projected to screen
- [ ] Shadow sprite + raised ball; bounce at a length; scaling with distance
- [ ] Deliveries generated from a bowler profile (data), randomised line/length
- [ ] Press a button to bowl again
- [ ] Debug overlay: line, length, speed

Learn: fixed-point maths, affine sprites, backgrounds.

## Phase 2 — Hitting it (the main iteration loop)
- [ ] Zones (line × length) and shot-vs-ball timing table
- [ ] Control scheme 1; then schemes 2 and 3, switchable in a debug menu
- [ ] Outcomes: miss, edge, block, 1–6, bowled, caught
- [ ] Feedback: timing indicator and shot rating after each ball
- [ ] **Play it a lot. Tune. Pick a control scheme.**

## Phase 3 — Quick Play loop
- [ ] Bottom scoreboard: runs, balls faced; out → game over → restart
- [ ] After-contact presentation: follow ball off screen → field view (test transitions)
- [ ] Simple fielders (squares) that slide to the ball; auto running; caught
- [ ] Event banners: 4, 6, wicket, end of over
- [ ] High score saved to SRAM

## Phase 4 — Depth
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
- [ ] Fancier event animations (full-screen wicket, milestones)
- [ ] Sound and music
