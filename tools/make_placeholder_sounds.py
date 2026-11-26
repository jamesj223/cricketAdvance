#!/usr/bin/env python3
"""Generates placeholder sound effects (audio/*.wav), synthesised from scratch.

Like the placeholder graphics, these are stand-ins until real samples replace them. They
are 8-bit mono 22050 Hz WAVs (the same format as Butano's examples); Maxmod converts them
at build time. No dependencies beyond the Python standard library.

  bounce.wav  the ball hitting the pitch: a short wooden "thock". Played at different
              speeds (pitch) by the game: deeper for harder, shorter-pitched balls.
  bat.wav     bat on ball, mistimed: a short, woody "tock" (mostly noise, so it doesn't
              ring like a xylophone). Also used, slower and quieter, for blocks.
  solid.wav   a well-timed hit: a sharper crack ("okay hit, not middled").
  Middled (perfect timing), by kind of shot:
    thwack.wav  along the ground, leg side: deep punchy body + bright snap.
    pock.wav    cuts and other square / behind off-side shots: a hard, crisp tock.
    clean.wav   drives: almost all snap ("so sweet it barely makes a noise").
    thock.wav   lofted leg side, the big six over cow corner: round, low and solid.
  mishit.wav  an attacking shot that's going to be caught: a double crack (bat flexing).
  edge.wav    an edge: a thin, ringing nick (played faster by the game).
  stumps.wav  stumps and bails clattering.
  crowd.wav   a crowd cheer for 4s, 6s and wickets.
  whoosh.wav  the bat swishing through the air on a play and miss.

Run from the repo root: python3 tools/make_placeholder_sounds.py
"""

import math
import random
import wave
from pathlib import Path

RATE = 22050
OUT = Path(__file__).resolve().parent.parent / "audio"

random.seed(4)  # the same sounds every run


def silence(seconds):
    return [0.0] * int(RATE * seconds)


def mix_into(buffer, samples, start_seconds=0.0, gain=1.0):
    start = int(start_seconds * RATE)
    for i, value in enumerate(samples):
        if start + i < len(buffer):
            buffer[start + i] += value * gain


def knock(freq, seconds, decay, noise=0.3, freq_end=None):
    """A struck object: a sine (optionally gliding in pitch) plus a noisy attack,
    with an exponential decay. decay = time constant in seconds."""
    out = []
    phase = 0.0
    freq_end = freq_end or freq
    for i in range(int(RATE * seconds)):
        t = i / RATE
        f = freq + (freq_end - freq) * min(1.0, t / seconds)
        phase += 2 * math.pi * f / RATE
        env = math.exp(-t / decay)
        click = random.uniform(-1, 1) * math.exp(-t / 0.004) * noise
        out.append((math.sin(phase) + click) * env)
    return out


def lowpass(samples, amount):
    """One-pole low-pass filter; amount in (0, 1], smaller = darker."""
    out, y = [], 0.0
    for x in samples:
        y += amount * (x - y)
        out.append(y)
    return out


def write(name, samples):
    peak = max(abs(s) for s in samples) or 1.0
    data = bytes(int(128 + 120 * s / peak) for s in samples)   # 8-bit unsigned
    with wave.open(str(OUT / name), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(1)
        w.setframerate(RATE)
        w.writeframes(data)
    print(f"wrote {OUT.name}/{name} ({len(samples) / RATE:.2f} s)")


def make_bounce():
    # Low, dull thud whose pitch drops slightly: leather on a hard pitch.
    s = knock(190, 0.14, 0.03, noise=0.6, freq_end=120)
    write("bounce.wav", lowpass(s, 0.35))


def noise_burst(seconds, decay, brightness):
    """Noise with a sharp attack and fast decay. brightness: low-pass amount (0-1)."""
    noise = [random.uniform(-1, 1) * math.exp(-(i / RATE) / decay)
             for i in range(int(RATE * seconds))]
    return lowpass(noise, brightness)


def highpass(samples, amount):
    dark = lowpass(samples, amount)
    return [a - b for a, b in zip(samples, dark)]


def make_edge():
    # The original (tonal, ringing) bat sound: as an edge, played fast, it's a thin nick.
    s = silence(0.16)
    mix_into(s, knock(950, 0.16, 0.025, noise=1.2))
    mix_into(s, knock(1550, 0.16, 0.015, noise=0.0), gain=0.5)
    mix_into(s, knock(420, 0.16, 0.04, noise=0.0), gain=0.4)
    write("edge.wav", s)


def make_bat():
    # Willow on leather: mostly a noise "tock" with a very short, low wooden body.
    # Short decays everywhere, so nothing rings.
    s = silence(0.1)
    mix_into(s, noise_burst(0.1, 0.006, 0.5))
    mix_into(s, knock(650, 0.1, 0.006, noise=0.0), gain=0.35)
    mix_into(s, knock(300, 0.1, 0.012, noise=0.0), gain=0.5)
    write("bat.wav", s)


def make_solid():
    # Well timed: a very sharp, bright double transient (a "crack"), then a fuller body.
    s = silence(0.14)
    bright = highpass(noise_burst(0.14, 0.004, 1.0), 0.25)
    mix_into(s, bright, 0.0, 1.0)
    mix_into(s, bright, 0.0025, 0.6)
    mix_into(s, noise_burst(0.14, 0.012, 0.35), 0.0, 0.7)
    mix_into(s, knock(240, 0.14, 0.02, noise=0.0), gain=0.55)
    write("solid.wav", s)


def make_stumps():
    # A few wooden knocks (stumps, then bails) at slightly random times and pitches.
    s = silence(0.45)
    hits = [(0.0, 760, 1.0), (0.025, 980, 0.8), (0.06, 640, 0.7),
            (0.13, 1700, 0.45), (0.2, 1900, 0.3), (0.29, 1600, 0.2)]
    for start, freq, gain in hits:
        f = freq * random.uniform(0.95, 1.05)
        mix_into(s, knock(f, 0.12, 0.022, noise=0.8), start, gain)
    write("stumps.wav", s)


def make_crowd():
    # Many voices: band-limited noise, each "voice" layer swelling at its own slow rate,
    # under an overall envelope (rise, hold, fade).
    seconds = 2.2
    n = int(RATE * seconds)
    total = [0.0] * n
    for _ in range(10):
        noise = [random.uniform(-1, 1) for _ in range(n)]
        band = lowpass(noise, random.uniform(0.08, 0.2))
        rate = random.uniform(2.5, 6.0)
        offset = random.uniform(0, math.tau)
        for i in range(n):
            t = i / RATE
            wobble = 0.6 + 0.4 * math.sin(math.tau * rate * t + offset)
            total[i] += band[i] * wobble
    # Take some rumble out (high-pass by subtracting a darker copy).
    dark = lowpass(total, 0.02)
    total = [a - b for a, b in zip(total, dark)]
    for i in range(n):
        t = i / RATE
        rise = min(1.0, t / 0.25)
        fade = min(1.0, max(0.0, (seconds - t) / 1.2))
        total[i] *= rise * fade
    write("crowd.wav", total)


def swept_lowpass(samples, start, peak, end, peak_at):
    """One-pole low-pass whose brightness glides start -> peak (at peak_at, 0-1) -> end:
    a sweep like air rushing past as the bat comes through."""
    out, y, n = [], 0.0, len(samples)
    for i, x in enumerate(samples):
        f = i / n
        if f < peak_at:
            amount = start + (peak - start) * f / peak_at
        else:
            amount = peak + (end - peak) * (f - peak_at) / (1 - peak_at)
        y += amount * (x - y)
        out.append(y)
    return out


def make_whoosh():
    random.seed(11)     # its own seed, so it never changes the sounds above
    seconds = 0.28
    n = int(RATE * seconds)
    noise = [random.uniform(-1, 1) for _ in range(n)]
    band = highpass(swept_lowpass(noise, 0.04, 0.35, 0.06, 0.55), 0.03)
    out = []
    for i, v in enumerate(band):
        f = i / n
        # Swell up to the moment the bat passes, then a quicker fade.
        env = (f / 0.55) ** 2 if f < 0.55 else ((1 - f) / 0.45) ** 1.5
        out.append(v * env)
    write("whoosh.wav", out)


# --- Middled hits ------------------------------------------------------------------------
# Auditioned as five candidates; each one suggested a different shot, so the game uses all
# of them (see ca_sounds.cpp). Each has its own seed, so they never change each other.

def make_middled():
    # A: "thwack" - a deep, punchy body with a pitch drop, under a short bright snap.
    random.seed(101)
    s = silence(0.16)
    mix_into(s, knock(210, 0.16, 0.03, noise=0.0, freq_end=110), gain=1.0)
    mix_into(s, highpass(noise_burst(0.16, 0.003, 1.0), 0.3), gain=0.9)
    mix_into(s, noise_burst(0.16, 0.015, 0.25), gain=0.5)
    write("thwack.wav", s)

    # B: "pock" - the normal bat tock, but harder: a crisper attack and a mid wooden
    # resonance (~1.1 kHz) that dies very fast.
    random.seed(102)
    s = silence(0.12)
    mix_into(s, noise_burst(0.12, 0.004, 0.7), gain=1.0)
    mix_into(s, knock(1100, 0.12, 0.008, noise=0.0), gain=0.6)
    mix_into(s, knock(420, 0.12, 0.018, noise=0.0), gain=0.7)
    write("pock.wav", s)

    # C: "clean crack" - almost all transient: one very sharp click with a tiny tail.
    random.seed(103)
    s = silence(0.1)
    mix_into(s, highpass(noise_burst(0.1, 0.002, 1.0), 0.15), gain=1.0)
    mix_into(s, noise_burst(0.1, 0.008, 0.4), gain=0.45)
    mix_into(s, knock(700, 0.1, 0.01, noise=0.0), gain=0.3)
    write("clean.wav", s)

    # D: "full thock" - like the bounce but bigger and brighter: a solid, low, round
    # hit (leather on a big, dense piece of willow).
    random.seed(104)
    s = silence(0.18)
    mix_into(s, knock(320, 0.18, 0.035, noise=0.9, freq_end=240), gain=1.0)
    mix_into(s, knock(160, 0.18, 0.05, noise=0.0), gain=0.6)
    mix_into(s, highpass(noise_burst(0.18, 0.003, 1.0), 0.25), gain=0.5)
    write("thock.wav", lowpass(s, 0.6))

    # E: "rifle" - two quick transients (the double crack of ball then bat flex),
    # bright, with a short mid body. Used for the mis-hit that goes up and is caught.
    random.seed(105)
    s = silence(0.14)
    snap = highpass(noise_burst(0.14, 0.0025, 1.0), 0.2)
    mix_into(s, snap, 0.0, 1.0)
    mix_into(s, snap, 0.006, 0.7)
    mix_into(s, knock(520, 0.14, 0.015, noise=0.0, freq_end=380), gain=0.6)
    write("mishit.wav", s)


def main():
    OUT.mkdir(exist_ok=True)
    # Order matters: everything shares one seeded random generator, so this order keeps
    # the existing sounds identical. Add new sounds at the END, or the ones after them
    # change. (edge.wav is the original bat sound, so it stays in the old bat's slot.)
    make_bounce()
    make_edge()
    make_stumps()
    make_crowd()
    make_bat()
    make_solid()
    make_whoosh()               # (each from here on sets its own seed)
    make_middled()


if __name__ == "__main__":
    main()
