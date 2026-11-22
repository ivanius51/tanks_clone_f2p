"""Synthesizes the background music loops into Resources/music/ (OGG via ffmpeg, WAV without it).

Calm tracks: slow beat for normal play. Intense tracks: fast and urgent for bosses, the last wave and level ends.
Every note is made here from simple waves and noise, so the game ships no borrowed music.
Usage: python tools/gen_music.py
"""

import array
import math
import os
import random
import shutil
import subprocess
import wave

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, "Resources", "music")
RATE = 22050

# Semitones from the key root for each chord of a progression (minor-ish, military-march feel).
PROGRESSIONS = {
    "a": [0, -4, -7, -2],
    "b": [0, 3, -2, -5],
    "c": [0, -2, -4, -5],
    "d": [0, 5, 3, -2],
}

TRACKS = [
    # name, bpm, root hz, progression, bars per chord, intensity 0..1, seed
    ("calm_1", 84, 110.0, "a", 2, 0.15, 1),
    ("calm_2", 90, 98.0, "b", 2, 0.25, 2),
    ("calm_3", 78, 123.5, "c", 2, 0.1, 3),
    ("calm_4", 96, 103.8, "d", 2, 0.3, 4),
    ("calm_5", 88, 116.5, "b", 2, 0.2, 5),
    ("calm_6", 92, 92.5, "a", 2, 0.35, 6),
    ("intense_1", 148, 110.0, "a", 1, 0.85, 11),
    ("intense_2", 156, 123.5, "c", 1, 0.95, 12),
    ("intense_3", 140, 98.0, "d", 1, 0.8, 13),
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
    for name, bpm, root, progression, bars_per_chord, intensity, seed in TRACKS:
        write(name, render(bpm, root, progression, bars_per_chord, intensity, seed))
    print("music written to " + OUTPUT)


if __name__ == "__main__":
    main()
