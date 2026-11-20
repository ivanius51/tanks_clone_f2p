"""Synthesizes the placeholder sound effects into Resources/sounds/ (16-bit mono WAV).

Every sound is made here from simple waves and noise, so the game ships no borrowed audio.
"""

import math
import os
import random
import struct
import wave

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, "Resources", "sounds")
RATE = 22050


def square(phase):
    return 1.0 if (phase % 1.0) < 0.5 else -1.0


def triangle(phase):
    p = phase % 1.0
    return 4 * p - 1 if p < 0.5 else 3 - 4 * p


def tone(duration, start_hz, end_hz=None, shape=square, volume=0.5, attack=0.005, decay=None):
    end_hz = start_hz if end_hz is None else end_hz
    count = int(duration * RATE)
    samples = []
    phase = 0.0
    for i in range(count):
        t = i / count
        phase += (start_hz + (end_hz - start_hz) * t) / RATE
        envelope = min(1.0, i / max(1, attack * RATE))
        envelope *= (1 - t) ** (decay if decay else 1.0)
        samples.append(shape(phase) * volume * envelope)
    return samples


def noise(duration, volume=0.6, decay=2.0, low_pass=0.5, seed=1):
    rng = random.Random(seed)
    count = int(duration * RATE)
    samples = []
    value = 0.0
    hold = 0
    for i in range(count):
        if hold <= 0:
            target = rng.uniform(-1, 1)
            hold = 1 + int(low_pass * 6)
            value = target
        hold -= 1
        t = i / count
        samples.append(value * volume * (1 - t) ** decay)
    return samples


def mix(*tracks):
    length = max(len(track) for track in tracks)
    return [sum(track[i] for track in tracks if i < len(track)) for i in range(length)]


def sequence(*parts):
    result = []
    for part in parts:
        result += part
    return result


def silence(duration):
    return [0.0] * int(duration * RATE)


def write(name, samples):
    os.makedirs(OUTPUT, exist_ok=True)
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = min(1.0, 0.9 / peak)
    with wave.open(os.path.join(OUTPUT, name + ".wav"), "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(2)
        file.setframerate(RATE)
        file.writeframes(b"".join(struct.pack("<h", int(s * scale * 32767)) for s in samples))


def notes(frequencies, length, shape=square, volume=0.35):
    return sequence(*[tone(length, f, shape=shape, volume=volume, decay=0.6) for f in frequencies])


def main():
    write("player_shot", mix(tone(0.12, 880, 220, volume=0.35, decay=1.5), noise(0.08, 0.3, 3.0, 0.2, 2)))
    write("hit_wall", mix(noise(0.1, 0.6, 2.5, 0.6, 3), tone(0.08, 180, 90, volume=0.3)))
    write("hit_armor", mix(tone(0.15, 1400, 1100, triangle, 0.4, decay=2.0), tone(0.15, 2100, 1700, triangle, 0.2, decay=2.0)))
    write("enemy_explode", mix(noise(0.45, 0.8, 1.6, 0.8, 4), tone(0.4, 160, 40, volume=0.35, decay=1.2)))
    write("player_explode", mix(noise(0.9, 0.8, 1.2, 1.0, 5), tone(0.8, 220, 30, volume=0.4, decay=1.0)))
    write("boss_explode", mix(noise(1.6, 0.9, 0.9, 1.0, 6), tone(1.4, 120, 25, volume=0.5, decay=0.8),
                              sequence(silence(0.25), noise(1.0, 0.6, 1.4, 0.9, 7))))
    write("wall_break", mix(noise(0.35, 0.7, 1.8, 0.7, 8), tone(0.25, 300, 80, volume=0.25, decay=1.5)))
    write("bonus_pickup", notes([660, 880, 1320], 0.07, volume=0.3))
    write("level_start", sequence(notes([440], 0.12), silence(0.18), notes([440], 0.12), silence(0.18), notes([880], 0.3)))
    write("level_complete", notes([523, 659, 784, 1047, 784, 1047], 0.11, triangle, 0.45))
    write("game_over", notes([392, 330, 262, 196], 0.22, triangle, 0.45))
    write("pause", sequence(notes([988], 0.06, volume=0.25), silence(0.05), notes([988], 0.06, volume=0.25)))
    print("sounds written to", OUTPUT)


if __name__ == "__main__":
    main()
