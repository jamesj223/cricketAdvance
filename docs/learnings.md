# Learnings

Notes on GBA development picked up while building Cricket Advance, phase by phase.
Each phase ends with some hands-on exercises.

---

## Phase 0 — Toolchain and project setup

### What `make` does

1. **Graphics conversion.** Butano reads each `graphics/*.bmp` + `.json` and converts it
   into GBA-native data (8×8 tiles + palette). It also generates a C++ header in `build/`,
   e.g. `build/bn_sprite_items_ball.h`. That's why `#include "bn_sprite_items_ball.h"`
   works even though the file isn't in the repo.
2. **Compile.** `arm-none-eabi-g++` (devkitARM) cross-compiles C++ for the GBA's ARM CPU.
3. **Link + fix.** Everything is linked into one binary, then `gbafix` writes the
   cartridge header (title, checksum) — the "ROM fixed!" line.

### Memory map (the parts that matter so far)

| Region | Size | Notes |
|---|---|---|
| ROM (cartridge) | up to 32 MB | Read-only. Code runs directly from it. 16-bit bus with wait states. |
| IWRAM | 32 KB | Fastest RAM, 32-bit bus. Stack lives here. |
| EWRAM | 256 KB | Slower, 16-bit bus. |
| VRAM / OAM / palette RAM | 96 KB / 1 KB / 1 KB | Graphics memory. Butano manages it for us. |

### Butano conventions

- **No runtime allocation.** Use `bn::vector<T, N>`, `bn::string<N>`, `bn::optional<T>`
  (fixed capacity, chosen at compile time) instead of `std::vector` / `std::string`.
  Memory use is predictable and you can't run out mid-game.
- **Centred coordinates.** (0, 0) is the middle of the 240×160 screen.
- **RAII handles.** A sprite/background exists on screen exactly as long as its
  `bn::sprite_ptr` / `bn::regular_bg_ptr` object exists.
- Butano is pinned as a git submodule (`external/butano`) so every machine builds against
  the same version.

### Exercises
- Open a generated header in `build/` and see what an image becomes.

---

## Phase 1 — A ball is bowled

### No floating point: fixed-point maths

The GBA's ARM7TDMI (16.78 MHz) has **no FPU** and **no hardware divide**.

`bn::fixed` stores a real number as a 32-bit integer scaled by 4096 (12 fractional bits):

| Real value | Stored integer |
|---|---|
| 1.0 | 4096 |
| 0.5 | 2048 |
| 0.71 | ≈ 2908 (really 0.70996…) |

- Decimal literals like `constexpr bn::fixed STUMP_HEIGHT = 0.71;` are converted **at
  compile time** on the Mac. The ROM only contains the integer.
- Add/subtract: plain integer add. Multiply: 64-bit integer multiply, shift right 12.
  Divide: shift left 12, integer divide (software routine — costs more, so keep them few).
- Precision 1/4096 ≈ 0.00024 (0.24 mm in our world). Range ≈ ±524,288.
- **Gotcha: small values lose precision.** `1 / depth` ≈ 0.022 is only ~92 steps of
  1/4096 — the ball would judder. `FOCAL / depth` ≈ 17 has ~70,000 steps. See
  `include/ca_camera.h`.
- **Bug we hit: Butano's `*` and `/` on two `bn::fixed` values are lossy.** For speed and
  overflow safety they first cut each operand to 6 fractional bits, so anything below
  1/64 becomes 0. Our time step `DT` ≈ 0.0098, so `velocity * DT` was always 0 and the
  ball never moved. Fix: `ca::mul` / `ca::div` in `include/ca_world.h`, which call
  `safe_multiplication` / `safe_division` (64-bit intermediate, full precision). On ARM a
  64-bit multiply is a single fast instruction; 64-bit division is slower, so keep those
  few. Lessons: know what your number type actually does, and a float-based mirror of
  the maths (our Python check) can't catch fixed-point bugs.
- `float` *does* compile, but every operation becomes a software-emulation call (tens to
  hundreds of cycles). Fine at startup, far too slow for per-frame physics.
- Constant trig (the camera tilt) was computed once in Python and pasted in. Changing
  angles use lookup tables (`bn::sin`).

### Memory speed vs CPU speed (compared with the N64)

- N64: fast CPU, slow RAM → a memory read can cost more than computing the value, so
  lookup tables can be *slower* than maths (Kaze's sine-table findings).
- GBA: slow CPU, comparatively fast memory → **lookup tables are a win**.
- But memory regions differ: IWRAM is fast; ROM/EWRAM have 16-bit buses and wait states.
- Two instruction sets: **Thumb** (16-bit, best when running from ROM — one bus read
  per instruction) and **ARM** (32-bit, best from IWRAM). Hot code can be put in IWRAM
  as ARM (`BN_CODE_IWRAM`).
- Lesson: the "normal" fastest way to do something depends on the hardware.

### Graphics hardware

- **Sprites (OAM):** 128 hardware sprites. Butano's default text is *also* made of
  sprites, so the debug text is only regenerated when it changes.
- **How sprite text works** (`bn::sprite_text_generator`, variable-width font): the font
  is a sprite sheet with one 8×8 glyph per character. For each string, Butano allocates
  fresh 32×8 "wide" sprites and copies each glyph's pixel columns into their tile memory,
  packing as many characters as fit (~5–6 at this font's widths) before starting the next
  sprite. So a 30-character line ≈ 6 sprites. Each new string costs new sprites + VRAM
  tiles, which is why identical letters aren't shared.
  - Limits: 128 sprites total, plus a per-scanline budget (the hardware can only draw so
    many sprite pixels on one line). Our debug text uses ~30 sprites; the container
    holding them is capped at 64 and Butano stops with an error screen if that's exceeded.
- **Text-heavy games** draw text into a **background layer** instead (e.g. Pokémon's
  text box renders font glyphs into BG tiles). Costs no sprites, can fill the screen.
- **Affine sprites:** scaling/rotation uses one of 32 affine matrices. Scaling above 1×
  needs "double size" mode or the sprite is clipped — hence the small ball drawn inside a
  16×16 canvas.
- **Backgrounds are tilemaps:** a grid of 8×8 tiles + a 16-colour (4bpp) palette.
  Identical tiles are stored once, so flat colours / no anti-aliasing keep it small.
- **Sub-pixel detail rounds badly.** The batter's stumps are ~4 px wide in total;
  rounding each stump to a pixel separately gave uneven gaps. Fix: snap the middle stump
  to a pixel and space the others by the same whole number of pixels. Pixel art at this
  scale often needs hand-placed pixels rather than exact geometry.
- **Palette index 0 is transparent.** Convention: put magenta there.
- **Alpha blending is global:** one transparency strength for everything blended
  (`bn::blending::set_transparency_alpha`). The shadow uses it at 50%.

### The frame loop and 60 fps

- `bn::core::update()` waits for **VBlank** (the gap between screen redraws, 60×/s) and
  commits sprite/BG changes then, so you never see half-updated frames.
- Game logic runs **once per frame** (fixed timestep). Budget: ~280,896 CPU cycles per
  frame. If a frame overran, the game would *slow down* (GoldenEye-style) rather than skip
  ahead — the normal GBA choice, and it keeps physics consistent.
- Target: **locked 60 fps.** `GAME_SPEED` is deliberate slow motion for playability,
  not lag.

### Randomness

- No clock, no entropy source → a **pseudo-random number generator**: a formula that
  turns a seed into a long, random-looking sequence (`bn::random`).
- Pokémon examples:
  - Ruby/Sapphire seed from the cartridge's real-time clock chip (there for
    berries/tides). Dead battery → same seed every time.
  - Emerald seeds with 0 every boot; it only diverges because the RNG advances every
    frame — why Emerald RNG manipulation is so well developed.
  - FireRed/LeafGreen seed from a timer when you leave the title screen.
- We do roughly what Emerald does: fixed seed, advanced every frame
  (`random.update()` in `src/main.cpp`), so the player's timing creates the variety.
  Later: seed from the frame count when Start is pressed on the title screen.
- Determinism is a feature: replays, reproducing bugs, "daily challenge" seeds.

### Code structure

Reading order: `ca_world.h` → `ca_bowler.h` → `ca_delivery.h/.cpp` → `ca_camera.h` →
`main.cpp`.

- **Simulation separate from presentation.** `ca_delivery` works in metres and seconds and
  knows nothing about sprites. `main.cpp` projects world positions to the screen through
  the camera. Changing the camera (top-down, behind the batter) means a new projection +
  background; the physics doesn't change. (Gameplay *feel* is view-dependent, though, so
  timing would need retuning.)
- **Data, not code, for variety.** `bowler_profile` is plain numbers (ranges,
  target + spread). New bowlers are new data.
- **Named fields in data (C++20 designated initialisers).** `bowler_profiles` uses
  `.speed_min_kmh = 132,` instead of a bare list of numbers, so every value is labelled
  and a field can't silently shift position. Fields must appear in declaration order.
- **The compiler checks the data.** With designated initialisers and
  `-Wmissing-field-initializers` (on in Butano's build), forgetting a field in a bowler
  gives a warning. `static_assert` checks things at compile time — e.g. that
  `release_x()` puts each arm/side combination on the correct side — and fails the build
  if they're wrong, at zero cost in the ROM.
- **Don't store what you can derive.** Bowlers store `arm`, `side` and a positive
  `release_width`; the signed release x is computed. Switching a bowler to round the
  wicket is one field, and the data can't contradict itself.
- **Butano asserts.** `BN_ASSERT` checks are on by default and stop the game on an error
  screen showing the file, line and message (e.g. `bn::random::get_fixed(0, 0)` →
  "Invalid range"). Annoying, but far better than silently wrong behaviour. Handle edge
  cases like "a variation of 0" in our own helpers (`plus_minus` in `ca_delivery.cpp`).
- **Plan vs flight.** `generate_delivery` decides what the bowler *intended*;
  `ball_flight` simulates it. Storing the plan made replay (B) free.
- **Solve backwards.** The release velocity is calculated to hit the planned bounce spot
  (design decides the result; physics delivers it).
- **Exact integration.** Moving by the *average* of old and new velocity is exact under
  constant acceleration (gravity), so the ball doesn't drift off its planned path.
- **Sub-frame bounce.** When the ball passes through the ground mid-frame, it's moved back
  to the exact contact point so the pitch marker is accurate.
- Known weak spots: `main.cpp` is one big prototype loop (to be split into scenes and a
  `ball_view` class in Phase 2); camera constants are duplicated in Python and C++.

### Exercises
1. **mGBA → Tools:** View Sprites, View Tiles, View Palette, View Map. See the ball,
   shadow, text sprites and pitch tiles as the hardware holds them.
2. **Add a third bowler** in `include/ca_bowler.h` — a leg spinner (negative `turn`) or a
   left-armer (positive `release_x`).
3. **Change `GAME_SPEED`** in `include/ca_world.h` and feel the difference.
4. **Move the camera:** edit the constants in both `tools/make_placeholder_graphics.py`
   and `include/ca_camera.h`, run `python3 tools/make_placeholder_graphics.py --preview`,
   look at `build/preview.png`, rebuild.
5. *(Optional)* Browse `external/butano/examples/` — one small project per feature.
   The Tonc tutorials explain the underlying hardware.

---

## Phase 2 — Hitting it

The longest phase so far, and the first one driven mostly by playtesting: build something,
play it (on a laptop, then real hardware), change it. Reading order: `ca_shots.h` →
`ca_controls.h` → `ca_practice_scene.cpp` (`_play_shot` is the heart of it).

### The batting pipeline

One delivery, start to finish:

1. **Release:** the whole delivery is simulated in advance (`preview_delivery`) to find
   where the ball pitches and the exact frames it reaches the bat on the front and back
   foot.
2. **Classify the ball** into a zone: line (wide / outside off / stumps / leg) ×
   length (yorker / full / good / short). Length bands differ for pace and spin.
3. **Read the input** → a `shot_intent` (direction, foot or "game picks", attack/block,
   loft). The control scheme only does this step.
4. **Intent → shot** (`make_shot`): direction × foot gives one of 16 shots.
5. **Rate the shot for the ball** (`rating_table`, 0 = wrong … 3 = ideal).
6. **Grade the timing:** press frame minus contact frame, against windows that grow with
   the rating. A good shot choice literally gives you more time.
7. **Outcome** from timing + rating (+ line for blocks), then pick a velocity that *shows*
   that outcome (`hit_velocity`). The result is decided first; the physics delivers it —
   the same "solve backwards" idea as the bowling in Phase 1.

### Looking into the future is cheap

`preview_delivery` runs the same `ball_flight` code ~60-100 times in one frame before the
ball is even visible. That's a few thousand integer operations — trivial for the CPU. It
works because the simulation is **deterministic**: same plan in, same flight out. (If the
physics used the random number generator mid-flight, the preview and the real ball could
disagree.) Games use this trick a lot: AI that predicts where a ball will land, trajectory
previews, replays.

### Time is measured in frames

- At 60 fps one frame ≈ 16.7 ms. All timing is in whole frames: an ideal shot's "perfect"
  window is ±2 frames (≈ 83 ms total). See `timing_windows` in `src/ca_shots.cpp`.
- Human reaction time is ~200 ms ≈ 12 frames — longer than most timing windows. You can't
  react to a cue *at* the right moment; you have to anticipate. That's why the live timing
  bar failed (and was removed): watching it took your eyes off the ball.
- The after-shot timing bar asks the real rules (`grade_timing`) for every cell, so it can
  never disagree with the game. "Draw the UI from the rules, not a copy of them."

### Reading the buttons

- The GBA has one 10-bit register (`KEYINPUT`) with a bit per button. Butano reads it once
  per frame. `a_held()` = the bit is set now; `a_pressed()` = set now but not last frame
  (an **edge**). Shots use `_pressed` so holding A doesn't play a shot every frame.
- Two buttons pressed within the same frame are **simultaneous** — there's no way to know
  which came first. Pro's A+B case has to have a rule for that.
- **Input buffering has knock-on effects.** The old A+B chord loft waited up to 3 frames
  for the second button. That needed "timed from the first press" bookkeeping *and*
  pausing the stumps check, or a well-timed shot could be ruled bowled while the game was
  still deciding. Lesson: every frame of input latency has gameplay consequences.
- After each ball, input is ignored for 45 frames: a very late press was bowling the next
  ball. Small "dead zones" like this are everywhere in real games.

### Sprites: frames, layers and text budget

- **Animation by swapping tiles:** the batter is one 32×32 sprite; each pose is a frame in
  the same sprite sheet and `set_tiles` swaps which frame's tiles it shows. Same for the
  standing / broken stumps. No new sprite needed.
- **Layering:** lower z order draws on top. The ball switches z order as it passes the
  batter, so it goes behind the pads and in front of the stumps.
- **Text is a budget:** with a variable-width sprite font, a line must fit the screen.
  We measure strings using the font's width table (that's how "GOOD CHOICE" became
  "GOOD SHOT" — it was 5 px too wide in the longest line).

### Design and code lessons

- **Tables are design.** The rating table, timing windows and blocking percentages are all
  tuning data in one place. Most playtest feedback turned into a changed number, not
  changed code.
- **Schemes are just input mappings.** Because every control scheme only produces a
  `shot_intent`, we could add, compare and delete schemes (seven in total!) without
  touching timing or outcomes.
- **Deleting is fine — git remembers.** The removed schemes live on in history and in the
  `phase-2-archive` branch (`roms/phase-2-controls.gba`).
- **Play it on the real thing.** Real buttons felt completely different to a keyboard, and
  the A+B chord and button layout questions only showed up in hands-on play.
- **Two ways to playtest a tuning table:** play normally and note where the game
  disagrees with your instinct; or play one fixed shot for 15-20 balls and judge each
  result. The second isolates one row of the table.
- **Realism often *is* the fun.** "Bad balls" became hittable half-volleys and long hops
  rather than random junk: more fun, and closer to how real bowlers miss.
- **Interpret, don't follow literally.** Pro controls went V1 → V2 → V3. The problem was
  never which buttons; it was how many separate decisions happen at the moment of the shot.
  V3 folds footwork into *which* button you press, so it's two inputs, not three.

### Exercises

1. **Frame-step a shot.** In mGBA, pause (**Ctrl+P**, **⌘P** on Mac) as the ball
   approaches, then advance one frame at a time (**Ctrl+N** / **⌘N**) and press A on a chosen frame (hold it, then advance).
   Try to land exactly on the perfect frame, then one either side. Watch the arrow on the
   timing bar move one cell per frame.
2. **Retune a shot.** In `rating_table` (`src/ca_shots.cpp`), make the pull ideal (3) to a
   wide short ball. Play 15 pulls to short wide balls before and after. Watch the timing
   bar's green section grow.
3. **Change the timing windows** (`timing_windows`) to make everything ±1 frame tighter.
   How much harder does it feel? (This is the "difficulty" knob we decided *not* to use
   for Basic alone.)
4. **Make blocking safer or riskier** with the `BLOCK_*` constants, e.g. set
   `BLOCK_WIDE_MISS_PERCENT` to 0 and try blocking wide balls.
5. **Add a leg spinner** to `bowler_profiles` (`include/ca_bowler.h`): negative
   `bounce_movement`, and a `bad_ball_mix` heavy on long hops (drag-downs).
6. **Add a bad-ball kind**, e.g. a yorker or a full toss: a new entry next to
   `half_volley` in `src/ca_delivery.cpp`, a weight in `bad_ball_weights`, and a case in
   `pick_bad_ball`. (The compiler will point out every bowler that needs the new weight.)
7. *(Optional)* Play the archive ROM (`roms/phase-2-controls.gba`) and try every scheme
   from this phase.
