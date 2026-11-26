#!/usr/bin/env python3
"""Generates the game's logo and a cart label for the slot CFW (RG SP).

Outputs (in art/):
  logo.png                 transparent logo: pixel title + the batter (drive pose)
  label/cricketAdvance.png cart label, 1280x661 (GBA label shape 1.94:1). Copy to the
                           SD card as Labels/GBA/<ROM file name>.png

Everything is drawn at native pixel size and scaled up with nearest neighbour, so it
stays crisp pixel art. Run from the repo root: python3 tools/make_cart_art.py
"""

from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "art"

WHITE = (255, 255, 255, 255)
GREY = (140, 140, 150, 255)
BLACK = (0, 0, 0, 255)
CLEAR = (0, 0, 0, 0)

# --- Title font ------------------------------------------------------------------------
# A chunky 2-pixel-stroke font, just the letters we need. 9 rows: rows 0-1 are ascender
# space, lowercase sits on rows 2-8.
GLYPHS = {
    "c": ["      ", "      ", " #### ", "##  ##", "##    ", "##    ", "##    ", "##  ##", " #### "],
    "r": ["      ", "      ", "## ###", "####  ", "###   ", "##    ", "##    ", "##    ", "##    "],
    "i": ["##", "  ", "##", "##", "##", "##", "##", "##", "##"],
    "k": ["##    ", "##    ", "##  ##", "## ## ", "####  ", "####  ", "## ## ", "##  ##", "##  ##"],
    "e": ["      ", "      ", " #### ", "##  ##", "##  ##", "######", "##    ", "##  ##", " #### "],
    "t": [" ##  ", " ##  ", "#####", " ##  ", " ##  ", " ##  ", " ##  ", " ##  ", "  ###"],
    "A": ["  ###  ", " ## ## ", "##   ##", "##   ##", "##   ##", "#######", "##   ##", "##   ##", "##   ##"],
    "d": ["    ##", "    ##", " #####", "##  ##", "##  ##", "##  ##", "##  ##", "##  ##", " #####"],
    "v": ["      ", "      ", "##  ##", "##  ##", "##  ##", "##  ##", " #### ", " #### ", "  ##  "],
    "a": ["      ", "      ", " #### ", "    ##", " #####", "##  ##", "##  ##", "##  ##", " #####"],
    "n": ["      ", "      ", "##### ", "##  ##", "##  ##", "##  ##", "##  ##", "##  ##", "##  ##"],
}
LETTER_GAP = 1


def text_mask(text):
    width = sum(len(GLYPHS[c][0]) for c in text) + LETTER_GAP * (len(text) - 1)
    img = Image.new("RGBA", (width, 9), CLEAR)
    x = 0
    for c in text:
        rows = GLYPHS[c]
        for y, row in enumerate(rows):
            for dx, cell in enumerate(row):
                if cell == "#":
                    img.putpixel((x + dx, y), WHITE)
        x += len(rows[0]) + LETTER_GAP
    return img


def _exterior(img):
    """Transparent pixels reachable from the edge (i.e. not holes inside letters)."""
    seen = set()
    stack = [(x, y) for x in range(img.width) for y in (0, img.height - 1)]
    stack += [(x, y) for y in range(img.height) for x in (0, img.width - 1)]
    while stack:
        x, y = stack.pop()
        if (x, y) in seen or not (0 <= x < img.width and 0 <= y < img.height):
            continue
        if img.getpixel((x, y))[3]:
            continue
        seen.add((x, y))
        stack += [(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)]
    return seen


def outlined(img, layers):
    """Adds 1-pixel outline rings around the opaque pixels, one per colour, inside out.
    Small holes inside the shape (letter counters) are filled with the first colour,
    so the outer outline never speckles the inside of letters."""
    pad = len(layers)
    out = Image.new("RGBA", (img.width + pad * 2, img.height + pad * 2), CLEAR)
    out.paste(img, (pad, pad))
    for index, colour in enumerate(layers):
        src = out.copy()
        outside = _exterior(src)
        for y in range(out.height):
            for x in range(out.width):
                if src.getpixel((x, y))[3]:
                    continue
                if (x, y) not in outside:
                    out.putpixel((x, y), layers[0])     # a hole: fill it
                    continue
                touching = any(
                    0 <= x + dx < out.width and 0 <= y + dy < out.height and
                    src.getpixel((x + dx, y + dy))[3]
                    for dx in (-1, 0, 1) for dy in (-1, 0, 1))
                if touching:
                    out.putpixel((x, y), colour)
    return out


def batter_sprite(pose=2):
    """The in-game batter (graphics/batter.bmp), magenta = transparent, cropped tight."""
    sheet = Image.open(ROOT / "graphics" / "batter.bmp").convert("RGBA")
    frame = sheet.crop((0, pose * 32, 32, pose * 32 + 32))
    for y in range(32):
        for x in range(32):
            if frame.getpixel((x, y))[:3] == (255, 0, 255):
                frame.putpixel((x, y), CLEAR)
    return frame.crop(frame.getbbox())


def ball_with_trail():
    """The ball flying away off the bat: a red ball and a fading trail, 2px tall."""
    img = Image.new("RGBA", (12, 6), CLEAR)
    # Trail rising to the right (faint -> solid), then a 2x2 red ball with a black outline.
    trail = [((0, 5), 70), ((1, 5), 70), ((2, 4), 120), ((3, 4), 120), ((4, 3), 180), ((5, 3), 180)]
    for (x, y), alpha in trail:
        img.putpixel((x, y), (255, 255, 255, alpha))
    for x in range(6, 10):
        for y in range(0, 4):
            img.putpixel((x, y), BLACK)
    for x, y in [(7, 1), (8, 1), (7, 2), (8, 2)]:
        img.putpixel((x, y), (210, 30, 30, 255))
    return img


def make_logo():
    """Two lines of title ("cricket" / "Advance") with the batter to the left, and the ball
    flying away off the end of the shorter first line."""
    line1 = text_mask("cricket")
    line2 = text_mask("Advance")
    line_gap = 3
    text = Image.new("RGBA", (max(line1.width, line2.width), line1.height * 2 + line_gap), CLEAR)
    text.alpha_composite(line1, (0, 0))
    text.alpha_composite(line2, (0, line1.height + line_gap))
    title = outlined(text, [GREY, BLACK])

    batter = outlined(batter_sprite(), [GREY, BLACK])
    ball = ball_with_trail()

    gap = 2
    width = batter.width + gap + title.width
    height = max(batter.height, title.height)
    logo = Image.new("RGBA", (width, height), CLEAR)
    logo.alpha_composite(batter, (0, height - batter.height))
    title_x = batter.width + gap
    title_y = height - title.height
    logo.alpha_composite(title, (title_x, title_y))
    # Ball flying away to the top right, in the space above the title.
    logo.alpha_composite(ball, (width - ball.width - 2, 2))
    return logo


def make_label(logo):
    """GBA label: 1280x661 (shape 1.94:1). Native canvas 80x42, scaled x16, cropped."""
    scale = 16
    w, h = 80, 42
    bg = Image.new("RGBA", (w, h), CLEAR)

    # Outfield with diagonal mowing stripes.
    stripes = [(46, 125, 50, 255), (60, 145, 64, 255)]
    band = 5
    for y in range(h):
        for x in range(w):
            bg.putpixel((x, y), stripes[((x + y) // band) % 2])

    label = bg.copy()
    x = (w - logo.width) // 2
    y = (h - logo.height) // 2
    label.alpha_composite(logo, (x, y))
    label = label.resize((w * scale, h * scale), Image.NEAREST)
    top = (label.height - 661) // 2
    return label.crop((0, top, 1280, top + 661)).convert("RGB")


def main():
    (OUT / "label").mkdir(parents=True, exist_ok=True)
    logo = make_logo()
    logo.resize((logo.width * 16, logo.height * 16), Image.NEAREST).save(OUT / "logo.png")
    make_label(logo).save(OUT / "label" / "cricketAdvance.png")
    print("wrote", OUT / "logo.png", "and", OUT / "label" / "cricketAdvance.png")


if __name__ == "__main__":
    main()
