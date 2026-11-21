"""Generates the blueprint art of the upgrades screen into art_hd/abilities/ and art/abilities/.

Frames: blueprint_top and blueprint_side (line drawings of the player tank, facing right),
weapon_* (one per weapon) and stat_* (reload, damage, auto-fire, ammo, engine, tracks, armor, hp).
Everything is drawn at HD size; the SD frame is the HD one shrunk 2x, keeping thin lines visible.
Run tools/pack_atlases.py and "tools/pack_atlases.py --hd" afterwards.
"""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngio  # noqa: E402
from gen_placeholder_art import Canvas, rgb  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LINE = rgb(95, 180, 238)
FAINT = rgb(95, 180, 238, 150)
GOLD = rgb(240, 192, 64)
GOLD_DARK = rgb(170, 120, 30)
ICON = 64
BLUEPRINT_SCALE = 1.4


class Pen:
    """Draws lines and shapes in mockup units, mapped onto a canvas by an offset and a scale."""

    def __init__(self, canvas, origin, scale):
        self.c, self.ox, self.oy, self.k = canvas, origin[0], origin[1], scale

    def map(self, x, y):
        return (x - self.ox) * self.k, (y - self.oy) * self.k

    def dot(self, x, y, radius, color):
        for yy in range(int(y - radius - 1), int(y + radius + 2)):
            for xx in range(int(x - radius - 1), int(x + radius + 2)):
                if (xx + 0.5 - x) ** 2 + (yy + 0.5 - y) ** 2 <= radius * radius:
                    self.c.set(xx, yy, color)

    def line(self, x0, y0, x1, y1, width=1.2, color=LINE, dash=0):
        (ax, ay), (bx, by) = self.map(x0, y0), self.map(x1, y1)
        length = math.hypot(bx - ax, by - ay)
        steps = max(1, int(length * 2))
        for i in range(steps + 1):
            t = i / steps
            if dash and int(t * length / (dash * self.k)) % 2:
                continue
            self.dot(ax + (bx - ax) * t, ay + (by - ay) * t, width * self.k / 2, color)

    def poly(self, points, width=1.2, color=LINE, closed=True, dash=0):
        pairs = list(zip(points, points[1:] + (points[:1] if closed else [])))
        for (x0, y0), (x1, y1) in pairs:
            self.line(x0, y0, x1, y1, width, color, dash)

    def rect(self, x, y, w, h, width=1.2, color=LINE, dash=0):
        self.poly([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], width, color, True, dash)

    def circle(self, cx, cy, r, width=1.2, color=LINE):
        segments = max(12, int(r * 3))
        points = [(cx + r * math.cos(a * 2 * math.pi / segments), cy + r * math.sin(a * 2 * math.pi / segments))
                  for a in range(segments)]
        self.poly(points, width, color)


def draw_top():
    c = Canvas(int(352 * BLUEPRINT_SCALE), int(140 * BLUEPRINT_SCALE))
    p = Pen(c, (128, 116), BLUEPRINT_SCALE)
    for y in (122, 236):
        p.rect(135, y, 270, 14)
        for x in range(139, 405, 9):
            p.line(x, y, x, y + 14, 0.5, FAINT)
    p.poly([(142, 136), (392, 136), (404, 150), (404, 222), (392, 236), (142, 236)], 1.6)
    p.rect(150, 146, 72, 80, 1.2, LINE, dash=4)
    for x in range(158, 216, 12):
        p.line(x, 154, x, 218, 0.6, FAINT)
    p.rect(232, 176, 16, 20)
    p.poly([(262, 150), (332, 150), (350, 166), (350, 206), (332, 222), (262, 222), (250, 206), (250, 166)], 1.6)
    p.circle(282, 186, 8)
    p.rect(310, 172, 12, 10)
    p.rect(350, 183, 112, 6, 1.4)
    p.rect(460, 181, 10, 10)
    return c


def draw_side():
    c = Canvas(int(352 * BLUEPRINT_SCALE), int(124 * BLUEPRINT_SCALE))
    p = Pen(c, (128, 306), BLUEPRINT_SCALE)
    p.poly([(140, 380), (400, 380), (415, 350), (380, 350), (370, 340), (160, 340), (145, 352)], 1.6)
    p.poly([(250, 340), (262, 312), (340, 312), (352, 322), (352, 340)], 1.6, LINE, closed=False)
    p.rect(352, 321, 110, 6, 1.4)
    p.rect(460, 319, 10, 10)
    p.rect(150, 345, 70, 32, 1.2, LINE, dash=4)
    p.rect(160, 351, 44, 20)
    for i in range(4):
        p.rect(164 + i * 10, 355, 6, 6, 0.6)
    p.rect(235, 350, 140, 22, 0.8)
    p.poly([(157, 380), (400, 380), (420, 402), (398, 424), (157, 424), (135, 402)], 1.4)
    for i in range(7):
        p.circle(172 + i * 35, 404, 12)
        p.circle(172 + i * 35, 404, 3, 0.8)
    p.circle(148, 396, 7)
    p.circle(408, 396, 7)
    return c


def icon(draw):
    c = Canvas(ICON, ICON)
    draw(c)
    c.outline(rgb(12, 35, 64))
    return c


def bar(c, x, y, w, h, color=GOLD):
    c.rect(int(x), int(y), int(w), int(h), color)


def weapon_cannon(c):
    bar(c, 8, 22, 22, 22)
    bar(c, 30, 28, 20, 9)
    bar(c, 48, 25, 8, 15)


def weapon_heavy_cannon(c):
    bar(c, 6, 18, 26, 30)
    bar(c, 32, 26, 20, 13)
    bar(c, 50, 22, 10, 21)


def weapon_machine_gun(c):
    bar(c, 8, 24, 24, 14)
    bar(c, 32, 29, 26, 5)
    for x in (12, 20, 28):
        bar(c, x, 42, 5, 6, GOLD_DARK)


def weapon_shotgun(c):
    bar(c, 8, 22, 18, 20)
    bar(c, 26, 23, 30, 6)
    bar(c, 26, 35, 30, 6)


def weapon_flak(c):
    bar(c, 8, 18, 18, 28)
    for y in (21, 30, 39):
        bar(c, 26, y, 30, 4)


def weapon_ricochet(c):
    bar(c, 6, 36, 18, 16)
    p = Pen(c, (0, 0), 1)
    p.poly([(24, 44), (40, 16), (58, 40)], 5, GOLD, closed=False)


def weapon_mine(c):
    p = Pen(c, (0, 0), 1)
    for angle in range(0, 180, 45):
        a = math.radians(angle)
        p.line(32 - 22 * math.cos(a), 32 - 22 * math.sin(a), 32 + 22 * math.cos(a), 32 + 22 * math.sin(a), 5, GOLD)
    p.dot(32, 32, 14, GOLD)
    p.dot(32, 32, 5, GOLD_DARK)


def stat_reload(c):
    p = Pen(c, (0, 0), 1)
    for i in range(40):
        a = math.radians(-60 + i * 7.5)
        p.dot(32 + 18 * math.cos(a), 32 + 18 * math.sin(a), 3.5, GOLD)
    p.poly([(50, 10), (52, 26), (38, 22)], 4, GOLD)


def stat_damage(c):
    p = Pen(c, (0, 0), 1)
    points = []
    for i in range(16):
        r = 26 if i % 2 == 0 else 11
        a = math.radians(i * 22.5)
        points.append((32 + r * math.cos(a), 32 + r * math.sin(a)))
    p.poly(points, 4, GOLD)
    p.dot(32, 32, 10, GOLD)


def stat_auto_fire(c):
    for top in (10, 26, 42):
        bar(c, 12, top, 28, 9)
        bar(c, 40, top + 2, 10, 5)


def stat_ammo(c):
    for x in (10, 26, 42):
        bar(c, x, 22, 12, 30)
        bar(c, x + 2, 12, 8, 10)
        bar(c, x, 46, 12, 4, GOLD_DARK)


def stat_engine(c):
    bar(c, 8, 20, 42, 28)
    for x in (12, 22, 32, 42):
        bar(c, x, 12, 6, 8)
    bar(c, 50, 28, 8, 10)
    bar(c, 14, 28, 30, 4, GOLD_DARK)


def stat_tracks(c):
    p = Pen(c, (0, 0), 1)
    p.poly([(12, 22), (52, 22), (60, 34), (52, 46), (12, 46), (4, 34)], 4, GOLD)
    for x in (16, 32, 48):
        p.dot(x, 34, 6, GOLD)


def stat_armor(c):
    bar(c, 10, 10, 44, 44)
    bar(c, 16, 16, 32, 32, GOLD_DARK)
    for x, y in ((13, 13), (48, 13), (13, 48), (48, 48)):
        bar(c, x, y, 4, 4, rgb(255, 240, 200))


def stat_hp(c):
    bar(c, 24, 8, 16, 48)
    bar(c, 8, 24, 48, 16)


FRAMES = {
    "blueprint_top": draw_top,
    "blueprint_side": draw_side,
}
ICONS = {
    "weapon_cannon": weapon_cannon, "weapon_heavy_cannon": weapon_heavy_cannon,
    "weapon_machine_gun": weapon_machine_gun, "weapon_shotgun": weapon_shotgun, "weapon_flak": weapon_flak,
    "weapon_ricochet": weapon_ricochet, "weapon_mine": weapon_mine,
    "stat_reload": stat_reload, "stat_damage": stat_damage, "stat_auto_fire": stat_auto_fire,
    "stat_ammo": stat_ammo, "stat_engine": stat_engine, "stat_tracks": stat_tracks,
    "stat_armor": stat_armor, "stat_hp": stat_hp,
}


def shrink(image):
    """Halves the image; each pixel takes the most opaque of its 2x2 block, so 1px lines survive."""
    result = Canvas(max(1, image.width // 2), max(1, image.height // 2))
    for y in range(result.height):
        for x in range(result.width):
            block = [image.get(x * 2 + dx, y * 2 + dy) for dy in (0, 1) for dx in (0, 1)]
            result.set(x, y, max(block, key=lambda color: color[3]))
    return result


def main():
    frames = {name: draw() for name, draw in FRAMES.items()}
    frames.update({name: icon(draw) for name, draw in ICONS.items()})
    for folder, shrink_frame in (("art_hd", False), ("art", True)):
        target = os.path.join(ROOT, folder, "abilities")
        os.makedirs(target, exist_ok=True)
        for name, image in frames.items():
            pngio.save(shrink(image) if shrink_frame else image, os.path.join(target, name + ".png"))
    print("upgrade art written:", len(frames), "frames")


if __name__ == "__main__":
    main()
