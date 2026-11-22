"""Synthesizes the background music loops into Resources/music/ (OGG via ffmpeg, WAV without it).

Calm tracks: slow beat for normal play. Intense tracks: fast and urgent for bosses, the last wave and level ends.
Every note is made here from simple waves and noise, so the game ships no borrowed music.
Usage: python tools/gen_music.py [track names, all by default]
"""

import array
import math
import os
import random
import shutil
import subprocess
import sys
import wave

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, "Resources", "music")
RATE = 44100

# Semitones from the key root for each chord of a progression (minor-ish, military-march feel).
PROGRESSIONS = {
    "a": [0, -4, -7, -2],
    "b": [0, 3, -2, -5],
    "c": [0, -2, -4, -5],
    "d": [0, 5, 3, -2],
    "e": [0, -4, 3, -2],
    "f": [0, 0, -4, -5],
}

TRACKS = [
    # name, bpm, root hz, progression, bars per chord, intensity 0..1, seed, style ("synth" default, "menu" has no drums, "chip")
    ("menu_1", 80, 98.0, "e", 2, 0.2, 21, "menu"),
    ("menu_2", 72, 110.0, "f", 2, 0.15, 22, "menu"),
    ("calm_1", 90, 110.0, "a", 2, 0.2, 1, "synth"),
    ("calm_2", 96, 98.0, "e", 2, 0.25, 2, "synth"),
    ("calm_3", 88, 123.5, "c", 2, 0.2, 3, "synth"),
    ("calm_4", 100, 103.8, "f", 2, 0.3, 4, "synth"),
    ("calm_5", 94, 116.5, "b", 2, 0.25, 5, "synth"),
    ("calm_6", 92, 92.5, "e", 2, 0.35, 6, "synth"),
    ("calm_7", 100, 110.0, "f", 2, 0.3, 7, "synth"),
    ("intense_1", 132, 110.0, "a", 1, 0.85, 11, "synth"),
    ("intense_2", 128, 123.5, "e", 1, 0.9, 12, "synth"),
    ("intense_3", 136, 98.0, "d", 1, 0.8, 13, "synth"),
]


def hz(root, semitones):
    return root * 2 ** (semitones / 12.0)


def add(buffer, start, samples, volume):
    for i, value in enumerate(samples):
        j = start + i
        if j >= len(buffer):
            j -= len(buffer)
        buffer[j] += value * volume


def voice(freq, duration, shape, decay):
    count = int(duration * RATE)
    out = []
    phase = 0.0
    step = freq / RATE
    for i in range(count):
        phase += step
        p = phase % 1.0
        if shape == "square":
            v = 1.0 if p < 0.5 else -1.0
        elif shape == "pulse":
            v = 1.0 if p < 0.25 else -1.0
        elif shape == "saw":
            v = 2 * p - 1
        else:
            v = 4 * p - 1 if p < 0.5 else 3 - 4 * p
        attack = min(1.0, i / (0.004 * RATE))
        out.append(v * attack * math.exp(-decay * i / RATE))
    return out


def drum(kind, rng):
    if kind == "kick":
        out, phase = [], 0.0
        for i in range(int(0.18 * RATE)):
            t = i / RATE
            phase += (40 + 110 * math.exp(-t * 30)) / RATE
            out.append(math.sin(2 * math.pi * phase) * math.exp(-t * 14))
        return out
    length = 0.14 if kind == "snare" else 0.04
    decay = 22 if kind == "snare" else 70
    out = []
    for i in range(int(length * RATE)):
        t = i / RATE
        n = rng.uniform(-1, 1)
        body = math.sin(2 * math.pi * 190 * t) * 0.5 if kind == "snare" else 0.0
        out.append((n + body) * math.exp(-t * decay))
    return out


def render(bpm, root, progression, bars_per_chord, intensity, seed):
    rng = random.Random(seed)
    beat = 60.0 / bpm
    sixteenth = beat / 4
    chords = PROGRESSIONS[progression]
    bars = len(chords) * bars_per_chord * (2 if intensity < 0.5 else 4)
    total = int(bars * 4 * beat * RATE)
    buffer = [0.0] * total
    kick, snare, hat = drum("kick", rng), drum("snare", rng), drum("hat", rng)
    lead_shape = "pulse" if intensity > 0.5 else "triangle"
    motif = [rng.choice([0, 3, 7, 10, 12, 7, 5]) for _ in range(8)]

    for bar in range(bars):
        chord = chords[(bar // bars_per_chord) % len(chords)]
        bar_start = bar * 4 * beat
        for step in range(16):
            at = int((bar_start + step * sixteenth) * RATE)
            # Drums: kick on beats (every 8th when intense), snare on 2 and 4, hats fill in.
            if step % 4 == 0 or (intensity > 0.5 and step % 2 == 0 and rng.random() < 0.5):
                add(buffer, at, kick, 0.55)
            if step in (4, 12):
                add(buffer, at, snare, 0.25 + 0.15 * intensity)
            if intensity > 0.5 or step % 2 == 0:
                add(buffer, at, hat, 0.06 + 0.06 * intensity)
            # Bass: root on beats, octave bounce when intense.
            if step % (2 if intensity > 0.5 else 4) == 0:
                octave = 12 if (intensity > 0.5 and step % 4 == 2) else 0
                add(buffer, at, voice(hz(root / 2, chord + octave), sixteenth * 1.8, "triangle", 6), 0.35)
        # Pad: chord tones held through the bar, quiet.
        for tone in (0, 3, 7):
            add(buffer, int(bar_start * RATE), voice(hz(root, chord + tone), 4 * beat, "triangle", 0.6), 0.05)
        # Lead: a short motif every other bar; every bar and faster when intense.
        if intensity > 0.5 or bar % 2 == 1:
            notes = 8 if intensity > 0.5 else 4
            length = 2 * beat / notes * (2 if intensity > 0.5 else 1)
            for n in range(notes):
                at = int((bar_start + n * length) * RATE)
                add(buffer, at, voice(hz(root * 2, chord + motif[n % len(motif)]), length * 0.9, lead_shape, 5), 0.07 + 0.05 * intensity)
    return buffer


def blep(p, dt):
    """PolyBLEP correction: smooths the saw's jump so high harmonics don't fold back as hiss and crackle."""
    if p < dt:
        x = p / dt
        return x + x - x * x - 1.0
    if p > 1.0 - dt:
        x = (p - 1.0) / dt
        return x * x + x + x + 1.0
    return 0.0


def detuned_saw(freq, duration, cutoff_from, cutoff_to, attack, release):
    """Three slightly detuned saws through a low-pass whose cutoff sweeps: a wide analog-style pad."""
    count = int(duration * RATE)
    phases = [0.0, 0.33, 0.66]
    steps = [freq * d / RATE for d in (0.993, 1.0, 1.007)]
    out, low = [], 0.0
    for i in range(count):
        v = 0.0
        for k in range(3):
            phases[k] += steps[k]
            p = phases[k] % 1.0
            v += 2 * p - 1 - blep(p, steps[k])
        cutoff = cutoff_from + (cutoff_to - cutoff_from) * i / count
        low += (v / 3 - low) * min(1.0, 2 * math.pi * cutoff / RATE)
        t = i / RATE
        env = min(1.0, t / attack) * min(1.0, (duration - t) / release)
        out.append(low * max(0.0, env))
    return out


def echo(buffer, delay, feedback, mix):
    """A looping delay line: wraps around so the loop seam keeps its tail."""
    shift = int(delay * RATE)
    wet = [0.0] * len(buffer)
    for _ in range(2):
        for i in range(len(buffer)):
            wet[i] = buffer[i - shift] + wet[i - shift] * feedback
    return [d + w * mix for d, w in zip(buffer, wet)]


def ambience(total, root, rng):
    """Cyberpunk backdrop: a low drone, a slowly breathing filtered noise wash (city hum, rain) and a high shimmer."""
    out, low = [0.0] * total, 0.0
    drone, shimmer = 0.0, 0.0
    for i in range(total):
        t = i / RATE
        breath = 0.5 + 0.5 * math.sin(2 * math.pi * t / 9.0)
        n = rng.uniform(-1, 1)
        low += (n - low) * 0.004
        drone += root / 4 / RATE
        shimmer += root * 4 / RATE
        out[i] = (low * 0.25 * breath
                  + math.sin(2 * math.pi * drone) * 0.12
                  + math.sin(2 * math.pi * shimmer) * 0.015 * (0.5 + 0.5 * math.sin(2 * math.pi * t / 5.3)))
    # Fade the noise toward the loop seam so it wraps without a click.
    edge = int(0.05 * RATE)
    for i in range(edge):
        k = i / edge
        out[i] *= k
        out[-1 - i] *= k
    return out


def soft_drum(kind, rng):
    """Gentle drums for the synth style: no sharp click, filtered noise, slow fade-in so nothing stabs the ears."""
    out, low = [], 0.0
    if kind == "kick":
        phase = 0.0
        for i in range(int(0.35 * RATE)):
            t = i / RATE
            phase += (45 + 35 * math.exp(-t * 18)) / RATE
            out.append(math.sin(2 * math.pi * phase) * math.exp(-t * 7) * min(1.0, t / 0.008))
        return out
    if kind == "hat":
        return [math.sin(2 * math.pi * 6000 * i / RATE) * math.exp(-i / RATE * 60) * min(1.0, i / (0.003 * RATE)) for i in range(int(0.06 * RATE))]
    length, decay, cutoff = 0.45, 7, 0.06
    for i in range(int(length * RATE)):
        t = i / RATE
        low += (rng.uniform(-1, 1) - low) * cutoff
        out.append(low * math.exp(-t * decay) * min(1.0, t / 0.012))
    return out


def render_synth(bpm, root, progression, bars_per_chord, intensity, seed, with_drums=True, bars=None):
    """Dark electronic style: pulsing 8th-note bass ducked by the kick, wide swelling pads,
    a big gated snare, a 16th arpeggio and a slow lead, all with echo."""
    rng = random.Random(seed)
    beat = 60.0 / bpm
    sixteenth = beat / 4
    chords = PROGRESSIONS[progression]
    bars = bars or len(chords) * bars_per_chord * (2 if intensity < 0.5 else 4)
    total = int(bars * 4 * beat * RATE)
    music = [0.0] * total
    drums = [0.0] * total
    kick, snare, hat = soft_drum("kick", rng), soft_drum("snare", rng), soft_drum("hat", rng)
    arp_shape = [0, 7, 12, 15, 12, 7, 3, 7]
    lead = [rng.choice([12, 15, 19, 17, 14]) for _ in range(4)]
    swell = 900 + 1800 * intensity

    for bar in range(bars):
        chord = chords[(bar // bars_per_chord) % len(chords)]
        bar_start = bar * 4 * beat
        section = bar / bars
        for step in range(16):
            at = int((bar_start + step * sixteenth) * RATE)
            if with_drums and step % 4 == 0 and (section > 0.12 or intensity > 0.5):
                add(drums, at, kick, 0.7)
            if with_drums and step in (4, 12) and section > 0.25:
                add(drums, at, snare, 0.35)
            if with_drums and step % 2 == 1 and section > 0.25:
                add(drums, at, hat, 0.05 + 0.05 * intensity)
            if step % 2 == 0 and (with_drums or section > 0.25):
                add(music, at, detuned_saw(hz(root / 2, chord), sixteenth * 1.6, 300, 160, 0.003, 0.03), 0.32)
            if section > 0.5 or intensity > 0.5:
                note = arp_shape[step % len(arp_shape)]
                add(music, at, voice(hz(root * 2, chord + note), sixteenth * 0.8, "pulse", 14), 0.04)
        sweep_from = 250 + swell * (bar % (2 * bars_per_chord)) / (2 * bars_per_chord)
        for tone in (0, 3, 7, 12):
            add(music, int(bar_start * RATE), detuned_saw(hz(root, chord + tone), 4 * beat, sweep_from, sweep_from + swell / 2, 0.6, 0.4), 0.07)
        if section >= 0.5 and bar % 2 == 0:
            for n, tone in enumerate(lead):
                at = int((bar_start + n * 2 * beat) * RATE)
                add(music, at, detuned_saw(hz(root * 2, chord + tone), 2 * beat, 1400, 2200, 0.08, 0.5), 0.06)
    # The kick ducks the synths (sidechain pump), then everything gets the dotted-8th echo.
    duck = [1.0] * total
    for bar in range(bars if with_drums else 0):
        for b in range(4):
            start = int((bar * 4 + b) * beat * RATE)
            for i in range(int(beat * 0.6 * RATE)):
                if start + i < total:
                    duck[start + i] = 0.45 + 0.55 * min(1.0, i / (beat * 0.6 * RATE)) ** 0.7
    music = echo([m * d for m, d in zip(music, duck)], beat * 0.75, 0.35, 0.3)
    # Drums sit under the music: calm tracks get them softer than intense ones.
    drum_level = 0.18 + 0.22 * intensity
    return [m + d * drum_level + a * 0.6 for m, d, a in zip(music, drums, ambience(total, root, rng))]


# A level's own track: (bars, intensity, drums) per section. At 110 bpm a bar is ~2.2 s:
# calm intro, a build from ~40 s, full speed from ~70 s, a calm verse near 80-90 s, then full again.
LEVEL_TRACKS = {
    "level_1": (110, 110.0, "f", 2, 31, [(18, 0.2, False), (14, 0.55, True), (6, 0.95, True), (8, 0.25, True), (16, 0.95, True), (8, 0.3, False),
                                         (16, 0.55, True), (16, 0.95, True), (8, 0.2, False)]),
}


def arrangement(intensity, with_drums):
    """Sections (bars, intensity, drums) that build up, break down and build again."""
    low, high = max(0.1, intensity - 0.15), min(1.0, intensity + 0.1)
    plan = [(8, low, False), (16, intensity, True), (8, low, True), (16, high, True), (8, low, False),
            (16, intensity, True), (16, high, True), (8, low, False)]
    if intensity > 0.5:
        plan = plan + plan[1:4]
    return [(bars, level, drums and with_drums) for bars, level, drums in plan]


def render_level(bpm, root, progression, bars_per_chord, seed, sections):
    out = []
    fade = int(0.4 * RATE)
    for index, (bars, intensity, with_drums) in enumerate(sections):
        part = render_synth(bpm, root, progression, bars_per_chord, intensity, seed + index, with_drums, bars)
        if out:
            for i in range(fade):
                k = i / fade
                out[-fade + i] = out[-fade + i] * (1 - k) + part[i] * k
            part = part[fade:]
        out.extend(part)
    return out


def write(name, samples):
    os.makedirs(OUTPUT, exist_ok=True)
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = 0.85 / peak
    data = array.array("h", (int(s * scale * 32767) for s in samples))
    path = os.path.join(OUTPUT, name + ".wav")
    with wave.open(path, "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(data.tobytes())
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg:
        subprocess.run([ffmpeg, "-loglevel", "error", "-y", "-i", path, "-c:a", "libvorbis", "-q:a", "3", os.path.join(OUTPUT, name + ".ogg")], check=True)
        os.remove(path)


def main():
    only = set(sys.argv[1:])
    for name, bpm, root, progression, bars_per_chord, intensity, seed, *style in TRACKS:
        if only and name not in only:
            continue
        if style == ["chip"]:
            samples = render(bpm, root, progression, bars_per_chord, intensity, seed)
        else:
            samples = render_level(bpm, root, progression, bars_per_chord, seed, arrangement(intensity, style != ["menu"]))
        write(name, samples)
        print("  " + name)
    for name, (bpm, root, progression, bars_per_chord, seed, sections) in LEVEL_TRACKS.items():
        if only and name not in only:
            continue
        write(name, render_level(bpm, root, progression, bars_per_chord, seed, sections))
        print("  " + name)
    print("music written to " + OUTPUT)


if __name__ == "__main__":
    main()
