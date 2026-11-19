# Cricket Advance

A cricket game for the Game Boy Advance, built with [Butano](https://github.com/GValiente/butano).

See [docs/design.md](docs/design.md) and [docs/plan.md](docs/plan.md).

## Setup (once per machine)

All platforms: clone with submodules (`git clone --recursive …`, or in an existing clone
`git submodule update --init`). Butano's build tools need **Python 3** (they convert the
graphics at build time); Pillow is only needed to regenerate the placeholder sprites
(`pip install pillow`).

### macOS

1. Install devkitPro: the macOS `.pkg` from https://github.com/devkitPro/pacman/releases/latest,
   then `sudo dkp-pacman -Sy && sudo dkp-pacman -S gba-dev`.
2. Add to `~/.zshrc`:
   ```sh
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   export PATH=$DEVKITPRO/tools/bin:$PATH
   ```
3. Install mGBA: `brew install --cask mgba`.

### Windows (not yet tested on this project)

1. Install devkitPro with the graphical installer from
   https://github.com/devkitPro/installer/releases/latest — tick **GBA Development**.
   It installs to `C:\devkitPro` and sets the `DEVKITPRO` / `DEVKITARM` variables.
2. Install Python 3 from https://www.python.org (tick "Add python.exe to PATH").
   The Makefile uses `python` on Windows and `python3` elsewhere.
3. Install mGBA from https://mgba.io/downloads.html.
4. Build from the **devkitPro MSYS2** shell (Start menu → devkitPro → MSYS2), which has
   `make`. Windows drives are under `/c/…`, e.g. `cd /c/Users/you/Git/cricketAdvance`.

## Build and run

```sh
make -j8        # builds cricketAdvance.gba
make clean      # remove build output
```

Run it in the emulator:
- macOS: `open -a mGBA cricketAdvance.gba`
- Windows: open `cricketAdvance.gba` in mGBA (or drag it onto the window).

To play on a handheld (Anbernic RG SP, Miyoo Mini Plus), copy `cricketAdvance.gba` to the
GBA ROMs folder on its SD card.

## Phase snapshots

`roms/phase-N.gba` is the ROM at the end of each phase (committed, so you can look back at
progress). At the end of a phase: `cp cricketAdvance.gba roms/phase-N.gba`.

## Layout

- `src/`, `include/` — game code
- `graphics/` — sprite/background BMPs + JSON config (processed by Butano at build time)
- `tools/` — helper scripts (e.g. placeholder sprite generator)
- `external/butano/` — Butano, pinned as a git submodule (21.10.0)
- `docs/` — design doc, plan, learnings
- `roms/` — saved ROM from the end of each phase
