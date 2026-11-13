"""Generate placeholder graphics (indexed BMPs) for early prototyping.

Usage:
    python3 tools/make_placeholder_graphics.py            # write graphics/*.bmp
    python3 tools/make_placeholder_graphics.py --preview  # also write build/preview.png

The pitch background is rendered with the SAME camera/projection as the game.
The camera constants below MUST match include/ca_camera.h.

World coordinates (metres):
    x: across the pitch. +x = screen right. Off side of a right-handed batter = -x.
    z: down the pitch. z = 0 at the bowler's stumps, z = 20.12 at the batter's stumps.
    h: height above the ground.
"""
import math
import sys
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "graphics"

# --- Camera (keep in sync with include/ca_camera.h) -------------------------------------
CAM_BACK = 24.0     # metres behind the bowler's stumps
CAM_HEIGHT = 6.5    # metres above the ground
CAM_TARGET_Z = 17.0 # point on the pitch the camera looks at
FOCAL = 760.0       # pixels
SCREEN_Y_OFFSET = -20.0  # screen y (centred coords) of the look-at point

PITCH_THETA = math.atan2(CAM_HEIGHT, CAM_BACK + CAM_TARGET_Z)
SIN_T = math.sin(PITCH_THETA)
COS_T = math.cos(PITCH_THETA)

# --- Pitch dimensions --------------------------------------------------------------------
STUMPS_Z = (0.0, 20.12)
POPPING_OFFSET = 1.22
PITCH_HALF_W = 1.52
RETURN_CREASE_X = 1.32
STUMP_HALF_W = 0.114
STUMP_HEIGHT = 0.71


def project(x, z, h=0.0):
    """World -> centred screen coords. Returns (sx, sy, depth)."""
    dz = z + CAM_BACK
    dy = h - CAM_HEIGHT
    depth = dz * COS_T - dy * SIN_T
    up = dy * COS_T + dz * SIN_T
    return x * FOCAL / depth, SCREEN_Y_OFFSET - up * FOCAL / depth, depth


def unproject_ground(sx, sy):
    """Centred screen coords -> ground point (x, z), or None if above the horizon."""
    # Ray direction in camera space: (sx/F, -(sy - offset)/F, 1)
    rx = sx / FOCAL
    ry = -(sy - SCREEN_Y_OFFSET) / FOCAL
    # Camera space -> world: forward = (0, -sin, cos), up = (0, cos, sin)
    dir_h = -SIN_T + ry * COS_T
    dir_z = COS_T + ry * SIN_T
    if dir_h >= 0:
        return None
    t = CAM_HEIGHT / -dir_h
    return rx * t, -CAM_BACK + dir_z * t


# --- Palette helpers ---------------------------------------------------------------------
MAGENTA = (255, 0, 255)


def indexed(size, palette):
    w, h = size if isinstance(size, tuple) else (size, size)
    img = Image.new("P", (w, h), 0)
    flat = [c for rgb in palette for c in rgb]
    flat += [0] * (256 * 3 - len(flat))
    img.putpalette(flat)
    return img


def circle_pixels(size, r, fill, edge=None, cx=None, cy=None, ry=None):
    cx = (size - 1) / 2 if cx is None else cx
    cy = (size - 1) / 2 if cy is None else cy
    ry = r if ry is None else ry
    px = []
    for y in range(size):
        for x in range(size):
            d = ((x - cx) / r) ** 2 + ((y - cy) / ry) ** 2
            if d <= 1.0:
                px.append(edge if (edge is not None and d > 0.55) else fill)
            else:
                px.append(0)
    return px


# --- Sprites -----------------------------------------------------------------------------
def make_sprites():
    # Ball: small red ball centred in a 16x16 canvas (room to scale up with affine).
    img = indexed(16, [MAGENTA, (225, 40, 40), (120, 15, 15)])
    img.putdata(circle_pixels(16, 2.6, 1, 2, cx=7.5, cy=7.5))
    img.save(OUT / "ball.bmp")

    # Shadow: flat dark ellipse (drawn with blending in game).
    img = indexed(16, [MAGENTA, (10, 25, 10)])
    img.putdata(circle_pixels(16, 3.2, 1, cx=7.5, cy=7.5, ry=1.6))
    img.save(OUT / "shadow.bmp")

    # Pitch marker: small light-blue oval, where the ball bounced.
    img = indexed(8, [MAGENTA, (120, 200, 255), (40, 110, 200)])
    img.putdata(circle_pixels(8, 2.6, 1, 2, cx=3.5, cy=3.5, ry=1.4))
    img.save(OUT / "marker.bmp")


# --- Batter ---------------------------------------------------------------------------
# Right-handed batter seen from the bowler's end, one 32x32 frame per pose, stacked
# vertically (Butano reads a tall image + "height": 32 as an animation strip).
# The frame order MUST match ca::batter_pose in include/ca_batter_view.h.
BATTER_POSES = ["stance", "block", "drive", "cut", "pull", "leave"]
BATTER_PALETTE = [
    MAGENTA,
    (245, 245, 240),  # 1 whites / pads
    (25, 40, 110),    # 2 helmet
    (215, 170, 130),  # 3 skin
    (200, 160, 90),   # 4 bat
    (60, 60, 60),     # 5 outline / shoes
    (190, 190, 185),  # 6 shading on whites
]


def draw_batter(draw, ox, oy, pose):
    def rect(x0, y0, x1, y1, c):
        draw.rectangle([ox + x0, oy + y0, ox + x1, oy + y1], fill=c)

    def bat(x0, y0, x1, y1):
        draw.line([(ox + x0, oy + y0), (ox + x1, oy + y1)], fill=4, width=2)

    crouch = 2 if pose == "block" else 0
    # Legs + pads (front leg forward/left for the block).
    lx = -2 if pose == "block" else 0
    rect(13 + lx, 19, 15 + lx, 29, 1)
    rect(17, 19, 19, 29, 1)
    rect(13 + lx, 30, 15 + lx, 31, 5)
    rect(17, 30, 19, 31, 5)
    # Body.
    rect(13, 9 + crouch, 19, 19, 1)
    rect(18, 10 + crouch, 19, 19, 6)
    # Head: helmet with a face peeking out.
    rect(14, 3 + crouch, 18, 8 + crouch, 2)
    rect(15, 6 + crouch, 17, 7 + crouch, 3)

    # Bat + hands per pose.
    if pose == "stance":
        bat(11, 17, 11, 29)
        rect(11, 15, 12, 16, 3)
    elif pose == "block":
        bat(13, 17, 12, 29)
        rect(12, 15, 13, 16, 3)
    elif pose == "drive":
        bat(20, 9, 27, 1)
        rect(19, 9, 20, 10, 3)
    elif pose == "cut":
        bat(12, 13, 1, 11)
        rect(12, 12, 13, 13, 3)
    elif pose == "pull":
        bat(20, 12, 31, 9)
        rect(19, 12, 20, 13, 3)
    elif pose == "leave":
        bat(20, 9, 22, 0)      # bat held up, well out of the way
        rect(19, 9, 20, 10, 3)


def make_batter():
    img = indexed((32, 32 * len(BATTER_POSES)), BATTER_PALETTE)
    draw = ImageDraw.Draw(img)
    for i, pose in enumerate(BATTER_POSES):
        draw_batter(draw, 0, i * 32, pose)
    img.save(OUT / "batter.bmp")


# --- Batter's stumps (sprite) ----------------------------------------------------------
# 16x16, two frames: standing, broken. The base of the stumps is on row STUMPS_BASE_ROW,
# and the middle stump on column 8. Keep STUMPS_BASE_ROW in sync with
# include/ca_stumps_view.h.
STUMPS_BASE_ROW = 13


def make_stumps():
    _, base_y, depth = project(0.0, STUMPS_Z[1], 0.0)
    gap = max(1, round(STUMP_HALF_W * FOCAL / depth))
    height = round(STUMP_HEIGHT * FOCAL / depth)
    top = STUMPS_BASE_ROW - height + 1
    img = indexed((16, 32), [MAGENTA, (235, 220, 170), (150, 120, 70)])
    draw = ImageDraw.Draw(img)
    b = STUMPS_BASE_ROW

    # Frame 0: standing, bails on top.
    for col in (8 - gap, 8, 8 + gap):
        draw.line([(col, top), (col, b)], fill=1)
    draw.line([(8 - gap, top - 1), (8 + gap, top - 1)], fill=2)

    # Frame 1: broken. Off and middle stumps knocked over, bails flying.
    o = 16
    draw.line([(8 - gap, o + b), (8 - gap - 4, o + top + 3)], fill=1)
    draw.line([(8, o + b), (9, o + top + 6)], fill=1)     # knocked back: looks shorter
    draw.line([(8 + gap, o + top), (8 + gap, o + b)], fill=1)
    draw.line([(1, o + top - 1), (2, o + top - 1)], fill=2)
    draw.line([(13, o + top - 2), (14, o + top - 3)], fill=2)
    img.save(OUT / "stumps.bmp")


# --- Timing bar -------------------------------------------------------------------------
# Cells: 8x8, frames = red (miss), orange (mistimed), yellow (good), green (perfect).
# Marker: 8x8, frames = down arrow (where you pressed), small tick (the ideal moment).
def make_timing_bar():
    colours = [(220, 40, 40), (240, 140, 30), (240, 220, 40), (60, 200, 70)]
    img = indexed((8, 8 * len(colours)), [MAGENTA] + colours)
    draw = ImageDraw.Draw(img)
    for i in range(len(colours)):
        draw.rectangle([0, i * 8 + 1, 7, i * 8 + 6], fill=i + 1)
    img.save(OUT / "timing_cell.bmp")

    img = indexed((8, 16), [MAGENTA, (255, 255, 255), (40, 40, 40)])
    draw = ImageDraw.Draw(img)
    draw.polygon([(1, 2), (7, 2), (4, 7)], fill=1, outline=2)     # down arrow
    draw.polygon([(4, 8 + 1), (2, 8 + 4), (6, 8 + 4)], fill=1)       # up tick
    img.save(OUT / "timing_marker.bmp")


# --- Live timing bar ---------------------------------------------------------------------
# Track: 32x8 piece of a thin line (tiled across the bottom of the screen).
# Marks: 8x8 frames = 0 tick (ideal contact moment), 1 mover (the ball's progress).
def make_live_bar():
    img = indexed((32, 8), [MAGENTA, (70, 70, 70), (130, 130, 130)])
    draw = ImageDraw.Draw(img)
    draw.line([(0, 3), (31, 3)], fill=2)
    draw.line([(0, 4), (31, 4)], fill=1)
    img.save(OUT / "live_track.bmp")

    img = indexed((8, 16), [MAGENTA, (60, 200, 70), (255, 255, 255), (40, 40, 40)])
    draw = ImageDraw.Draw(img)
    draw.rectangle([3, 1, 4, 6], fill=1)                    # tick
    draw.rectangle([2, 8 + 1, 5, 8 + 6], fill=2, outline=3)  # mover
    img.save(OUT / "live_marks.bmp")


# --- Background --------------------------------------------------------------------------
BG_PALETTE = [
    MAGENTA,          # 0 unused/transparent
    (40, 110, 40),    # 1 grass light
    (34, 96, 34),     # 2 grass dark
    (200, 180, 120),  # 3 pitch
    (240, 240, 230),  # 4 crease lines
    (235, 220, 170),  # 5 stumps
    (20, 50, 90),     # 6 sky / stands
    (182, 162, 104),  # 7 pitch worn
]


def make_background(preview=False):
    size = 256
    img = indexed(size, BG_PALETTE)
    px = img.load()
    for py in range(size):
        for pxx in range(size):
            sx, sy = pxx - 128 + 0.5, py - 128 + 0.5
            g = unproject_ground(sx, sy)
            if g is None or g[1] > 80:
                px[pxx, py] = 6
                continue
            x, z = g
            if abs(x) <= PITCH_HALF_W and -1.4 <= z <= 21.5:
                # Slightly worn area where bowlers land / batter stands.
                worn = abs(x) < 0.5 and (z < 2.4 or z > 17.7)
                px[pxx, py] = 7 if worn else 3
            else:
                # Mowing stripes across the ground, every 4 m.
                px[pxx, py] = 1 if int(math.floor(z / 4.0)) % 2 == 0 else 2

    draw = ImageDraw.Draw(img)

    def line(p0, p1, colour):
        a = project(*p0)
        b = project(*p1)
        draw.line([(a[0] + 128, a[1] + 128), (b[0] + 128, b[1] + 128)], fill=colour)

    for sz, sign in ((STUMPS_Z[0], 1), (STUMPS_Z[1], -1)):
        pz = sz + sign * POPPING_OFFSET
        line((-PITCH_HALF_W, sz), (PITCH_HALF_W, sz), 4)                 # bowling crease
        line((-PITCH_HALF_W - 0.4, pz), (PITCH_HALF_W + 0.4, pz), 4)     # popping crease
        for rx in (-RETURN_CREASE_X, RETURN_CREASE_X):                   # return creases
            line((rx, sz - sign * 1.0), (rx, pz), 4)

    # Stumps: only a few pixels wide, so rounding each stump separately gives uneven
    # gaps. Snap the middle stump to a pixel column and space the others symmetrically.
    # Only the bowler's end: the batter's stumps are a sprite (so they can be broken).
    for sz in STUMPS_Z[:1]:
        base_x, base_y, depth = project(0.0, sz, 0.0)
        _, top_y, _ = project(0.0, sz, STUMP_HEIGHT)
        mid = math.floor(base_x + 128)
        gap = max(1, round(STUMP_HALF_W * FOCAL / depth))
        for col in (mid - gap, mid, mid + gap):
            draw.line([(col, math.floor(base_y + 128)), (col, math.floor(top_y + 128))], fill=5)

    img.save(OUT / "pitch.bmp")

    if preview:
        (ROOT / "build").mkdir(exist_ok=True)
        strip = Image.open(OUT / "batter.bmp").convert("RGB")
        strip.resize((strip.width * 6, strip.height * 6), Image.NEAREST).save(ROOT / "build" / "batter_preview.png")
        # Screen area of the 256x256 bg (centred): x 8..248, y 48..208. Scale up 3x.
        shot = img.convert("RGB").crop((8, 48, 248, 208)).resize((720, 480), Image.NEAREST)
        shot.save(ROOT / "build" / "preview.png")


if __name__ == "__main__":
    make_sprites()
    make_batter()
    make_stumps()
    make_timing_bar()
    make_live_bar()
    make_background(preview="--preview" in sys.argv)
    for name, (x, z, h) in {
        "bowler stumps base": (0, 0, 0),
        "batter stumps base": (0, 20.12, 0),
        "batter stumps top": (0, 20.12, 0.71),
        "release pace": (-0.4, 1.0, 2.2),
        "pitch near-left": (-1.52, -1.4, 0),
    }.items():
        sx, sy, d = project(x, z, h)
        print(f"{name:20s} sx={sx:7.1f} sy={sy:7.1f} depth={d:5.1f}")
