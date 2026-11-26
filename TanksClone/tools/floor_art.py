"""Floor (ground modifier) tiles for every tileset: road, mud, forest and water. Seamless, so they tile on the map.

Used by gen_placeholder_art.py (32 px) and hd_art.py (64 px). The frame names are listed in
Resources/configs/levels/legend.json (`frame`) and exist in every tileset atlas.
"""
import math
import random

FLOOR_KINDS = ("road", "mud", "forest", "water")

# Material colours; each tileset pulls them a little towards its own ground so a road in the snow looks snowy.
MATERIALS = {
    "road": (122, 120, 118),
    "mud": (96, 70, 46),
    "forest": (44, 92, 44),
    "water": (44, 92, 168),
}
TILESET_PULL = 0.28


def _periodic_noise(seed, size, octaves=3):
    rng = random.Random(seed)
    waves = [(rng.randrange(1, 4) * (o + 1), rng.randrange(1, 4) * (o + 1), rng.random() * math.tau, 1.0 / (o + 1)) for o in range(octaves)]
    total = sum(w[3] for w in waves)

    def value(x, y):
        v = 0.0
        for fx, fy, phase, weight in waves:
            v += weight * math.sin(math.tau * (fx * x + fy * y) / size + phase)
        return 0.5 + 0.5 * v / total
    return value


def _blend(base, material, amount, rgb):
    return rgb(base[0] + (material[0] - base[0]) * amount, base[1] + (material[1] - base[1]) * amount, base[2] + (material[2] - base[2]) * amount)


def draw_floor(kind, variant, size, ground, seed, canvas_class, rgb, shade):
    """One floor tile of `size` px for a tileset whose plain ground colour is `ground`."""
    base = _blend(rgb(*MATERIALS[kind]), ground, TILESET_PULL, rgb)
    c = canvas_class(size, size)
    noise = _periodic_noise(seed * 31 + variant * 7 + len(kind), size)
    rng = random.Random(seed * 977 + variant * 13 + sum(map(ord, kind)))
    scale = size / 32.0
    for y in range(size):
        for x in range(size):
            c.set(x, y, shade(base, 0.86 + noise(x, y) * 0.22 + rng.random() * 0.03))
    {"road": _road, "mud": _mud, "forest": _forest, "water": _water}[kind](c, size, scale, base, rng, rgb, shade, variant)
    return c


def _wrapped(c, size, x, y, color):
    c.set(x % size, y % size, color)


def _road(c, size, scale, base, rng, rgb, shade, variant):
    # Worn cobbles: small stones with a light top edge, a few ruts running along the tile.
    for _ in range(int(26 * scale * scale)):
        x, y = rng.randrange(size), rng.randrange(size)
        stone = shade(base, 0.8 + rng.random() * 0.45)
        w, h = max(2, int(scale * (2 + rng.randrange(3)))), max(2, int(scale * (2 + rng.randrange(2))))
        for dy in range(h):
            for dx in range(w):
                _wrapped(c, size, x + dx, y + dy, stone)
        for dx in range(w):
            _wrapped(c, size, x + dx, y, shade(stone, 1.25))
            _wrapped(c, size, x + dx, y + h, shade(base, 0.7))
    rut = shade(base, 0.72)
    for lane in (size // 4, size * 3 // 4):
        for x in range(size):
            _wrapped(c, size, x, lane + int(math.sin(x / size * math.tau * 2 + variant) * scale), rut)


def _mud(c, size, scale, base, rng, rgb, shade, variant):
    # Dark wet puddles with a lighter rim and a few clods.
    for _ in range(2 + variant % 2):
        cx, cy = rng.randrange(size), rng.randrange(size)
        radius = (4 + rng.random() * 4) * scale
        for y in range(int(cy - radius - 2), int(cy + radius + 3)):
            for x in range(int(cx - radius - 2), int(cx + radius + 3)):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if d <= radius:
                    _wrapped(c, size, x, y, shade(base, 0.62 + 0.12 * d / radius))
                elif d <= radius + scale:
                    _wrapped(c, size, x, y, shade(base, 1.18))
    for _ in range(int(10 * scale)):
        x, y = rng.randrange(size), rng.randrange(size)
        for dx in range(int(scale) + 1):
            _wrapped(c, size, x + dx, y, shade(base, 1.3))


def _forest(c, size, scale, base, rng, rgb, shade, variant):
    # Overlapping tree crowns seen from above: dark shadow, body, a light highlight.
    crowns = [(rng.randrange(size), rng.randrange(size), (5 + rng.random() * 3) * scale) for _ in range(5 + variant % 3)]
    for shadow in (True, False):
        for cx, cy, radius in crowns:
            ox = 1.5 * scale if shadow else 0.0
            for y in range(int(cy - radius - 2), int(cy + radius + 3)):
                for x in range(int(cx - radius - 2), int(cx + radius + 3)):
                    d = math.hypot(x + 0.5 - cx - ox, y + 0.5 - cy - ox)
                    if d > radius:
                        continue
                    if shadow:
                        _wrapped(c, size, x, y, shade(base, 0.5))
                        continue
                    light = 1.0 + 0.45 * (1.0 - d / radius) - 0.25 * ((x - cx) + (y - cy)) / (2 * radius)
                    _wrapped(c, size, x, y, shade(base, max(0.7, light)))


def _water(c, size, scale, base, rng, rgb, shade, variant):
    # Ripples: short light streaks that wrap around the tile edge.
    for _ in range(int(9 * scale)):
        x, y = rng.randrange(size), rng.randrange(size)
        length = int((4 + rng.randrange(5)) * scale)
        for i in range(length):
            _wrapped(c, size, x + i, y + int(math.sin(i / max(1.0, scale) * 0.9) * scale * 0.6), shade(base, 1.45))
    for _ in range(int(6 * scale)):
        x, y = rng.randrange(size), rng.randrange(size)
        for i in range(int(3 * scale)):
            _wrapped(c, size, x + i, y, shade(base, 0.68))
