"""Generate placeholder sprite BMPs (indexed, 16 colours) for early prototyping.

Usage: python3 tools/make_placeholder_sprites.py
Palette index 0 is the transparent colour (magenta).
"""
from pathlib import Path
from PIL import Image

OUT = Path(__file__).resolve().parent.parent / "graphics"

def save(name, size, palette, pixels):
    img = Image.new("P", (size, size), 0)
    flat = [c for rgb in palette for c in rgb]
    flat += [0] * (16 * 3 - len(flat))
    img.putpalette(flat)
    img.putdata(pixels)
    img.save(OUT / f"{name}.bmp")

def circle(size, r, inner, outer, edge):
    c = (size - 1) / 2
    px = []
    for y in range(size):
        for x in range(size):
            d2 = (x - c) ** 2 + (y - c) ** 2
            px.append(inner if d2 <= (r - 1.2) ** 2 else edge if d2 <= r ** 2 else outer)
    return px

MAGENTA = (255, 0, 255)

# Red cricket ball: transparent bg, red body, dark red rim.
save("ball", 16, [MAGENTA, (200, 30, 30), (110, 10, 10)], circle(16, 4.5, 1, 0, 2))
