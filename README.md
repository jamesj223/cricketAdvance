# Cricket Advance

A cricket game for the Game Boy Advance, built with [Butano](https://github.com/GValiente/butano).

See [docs/design.md](docs/design.md) and [docs/plan.md](docs/plan.md).

## Setup (once per machine)

1. Install devkitPro: the macOS `.pkg` from https://github.com/devkitPro/pacman/releases/latest,
   then `sudo dkp-pacman -Sy && sudo dkp-pacman -S gba-dev`.
2. Add to `~/.zshrc`:
   ```sh
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   export PATH=$DEVKITPRO/tools/bin:$PATH
   ```
3. Install mGBA: `brew install --cask mgba`.
4. Python 3 with Pillow (only needed to regenerate placeholder sprites).
5. Clone with submodules: `git clone --recursive …`, or in an existing clone
   `git submodule update --init`.

## Build and run

```sh
make -j8                          # builds cricketAdvance.gba
open -a mGBA cricketAdvance.gba   # run in the emulator
make clean                        # remove build output
```

To play on the RG SP, copy `cricketAdvance.gba` to the GBA ROMs folder on its SD card.

## Layout

- `src/`, `include/` — game code
- `graphics/` — sprite/background BMPs + JSON config (processed by Butano at build time)
- `tools/` — helper scripts (e.g. placeholder sprite generator)
- `external/butano/` — Butano, pinned as a git submodule (21.10.0)
- `docs/` — design doc and plan
