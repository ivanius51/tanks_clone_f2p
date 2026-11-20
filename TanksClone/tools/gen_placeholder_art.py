"""Generates the placeholder pixel art of the game into art/<atlas>/<frame>.png.

Every sprite is drawn here from code, so the game ships no borrowed graphics.
Run tools/pack_atlases.py afterwards to build Resources/textures/ from art/.

--hd writes the HD set (64x64 tiles, more animation frames) into art_hd/ instead, see tools/hd_art.py;
pack it with "tools/pack_atlases.py --hd" into Resources/textures_hd/.
"""

import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngio  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART = os.path.join(ROOT, "art")

CLEAR = (0, 0, 0, 0)
BLACK = (14, 14, 18, 255)
WHITE = (250, 250, 245, 255)


def rgb(r, g, b, a=255):
    return (max(0, min(255, int(r))), max(0, min(255, int(g))), max(0, min(255, int(b))), a)


def shade(color, factor):
    return rgb(color[0] * factor, color[1] * factor, color[2] * factor, color[3] if len(color) > 3 else 255)


def mix(a, b, t):
    return rgb(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t)


class Canvas(pngio.Image):
    def rect(self, x, y, w, h, color):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, color)

    def hline(self, x0, x1, y, color):
        for x in range(x0, x1 + 1):
            self.set(x, y, color)

    def vline(self, x, y0, y1, color):
        for y in range(y0, y1 + 1):
            self.set(x, y, color)

    def bevel(self, x, y, w, h, color, depth=1):
        """A filled box with a light top-left edge and a dark bottom-right edge."""
        self.rect(x, y, w, h, color)
        light, dark = shade(color, 1.3), shade(color, 0.62)
        for d in range(depth):
            self.hline(x + d, x + w - 1 - d, y + d, light)
            self.vline(x + d, y + d, y + h - 1 - d, light)
            self.hline(x + d, x + w - 1 - d, y + h - 1 - d, dark)
            self.vline(x + w - 1 - d, y + d, y + h - 1 - d, dark)

    def disc(self, cx, cy, radius, color):
        for y in range(int(cy - radius - 1), int(cy + radius + 2)):
            for x in range(int(cx - radius - 1), int(cx + radius + 2)):
                if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= radius * radius:
                    self.set(x, y, color)

    def ring(self, cx, cy, radius, width, color, dash=0):
        for y in range(self.height):
            for x in range(self.width):
                distance = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if radius - width <= distance <= radius:
                    if dash:
                        angle = math.atan2(y + 0.5 - cy, x + 0.5 - cx)
                        if int((angle + math.pi) / (2 * math.pi) * dash * 2) % 2:
                            continue
                    self.set(x, y, color)

    def outline(self, color):
        """Draws a 1px outline around the opaque pixels, inside the canvas."""
        solid = [[self.get(x, y)[3] > 0 for x in range(self.width)] for y in range(self.height)]
        for y in range(self.height):
            for x in range(self.width):
                if solid[y][x]:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < self.width and 0 <= ny < self.height and solid[ny][nx]:
                        self.set(x, y, color)
                        break


def save(canvas, atlas, frame):
    folder = os.path.join(ART, *atlas.split("/"))
    os.makedirs(folder, exist_ok=True)
    pngio.save(canvas, os.path.join(folder, frame + ".png"))


# ------------------------------------------------------------------ tanks

class TankStyle:
    def __init__(self, size, body, track=(58, 60, 64), track_width=6, turret=6.0, barrels=(2,),
                 barrel_length=None, muzzle=0, decor=None):
        self.size = size
        self.body = rgb(*body)
        self.track = rgb(*track)
        self.track_width = track_width
        self.turret = turret
        self.barrels = barrels
        self.barrel_length = barrel_length
        self.muzzle = muzzle
        self.decor = decor


def draw_tank(style, phase):
    width, height = style.size
    c = Canvas(width, height)
    tw = style.track_width
    hull_top = 6

    # tracks with treads that move by one pixel per frame
    for left in (0, width - tw):
        c.rect(left, hull_top - 2, tw, height - hull_top + 2, shade(style.track, 0.55))
        c.rect(left + 1, hull_top - 1, tw - 2, height - hull_top, style.track)
        for y in range(hull_top + (phase % 3), height - 1, 3):
            c.hline(left + 1, left + tw - 2, y, shade(style.track, 0.7))
        c.vline(left + (1 if left == 0 else tw - 2), hull_top - 1, height - 2, shade(style.track, 1.35))

    # hull
    c.bevel(tw - 1, hull_top, width - 2 * tw + 2, height - hull_top - 1, style.body)
    c.hline(tw, width - tw - 1, height - 3, shade(style.body, 0.8))

    if style.decor:
        style.decor(c, style)

    # turret and barrels
    cx, cy = width / 2, hull_top + (height - hull_top) * 0.55
    length = style.barrel_length or int(cy)
    count = len(style.barrels)
    spacing = style.barrels[0] + 2
    for i, barrel_width in enumerate(style.barrels):
        bx = int(round(cx - barrel_width / 2 + (i - (count - 1) / 2) * spacing))
        top = int(cy) - length
        c.rect(bx, max(0, top), barrel_width, length, shade(style.body, 0.5))
        c.vline(bx, max(0, top), int(cy), shade(style.body, 0.85))
        if style.muzzle:
            c.rect(bx - style.muzzle, max(0, top), barrel_width + 2 * style.muzzle, 2, shade(style.body, 0.4))
    c.disc(cx, cy, style.turret, shade(style.body, 0.55))
    c.disc(cx - 0.4, cy - 0.4, style.turret - 1, shade(style.body, 1.1))
    c.disc(cx - 1.2, cy - 1.2, max(1.0, style.turret - 3.5), shade(style.body, 1.35))
    c.set(int(cx), int(cy), shade(style.body, 0.6))
    return c


def decor_player(c, style):
    width, height = c.width, c.height
    c.hline(style.track_width + 1, width - style.track_width - 2, height - 5, rgb(235, 200, 70))
    c.hline(style.track_width + 1, width - style.track_width - 2, height - 4, rgb(190, 150, 40))


def decor_scout(c, style):
    width, height = c.width, c.height
    for x in range(style.track_width + 1, width - style.track_width - 1, 3):
        c.set(x, 8, shade(style.body, 0.6))


def decor_raider(c, style):
    width, height = c.width, c.height
    mid = width // 2
    for row in (height - 8, height - 5):
        for dx in range(0, 5):
            c.set(mid - dx, row + dx // 2, BLACK)
            c.set(mid + dx - 1, row + dx // 2, BLACK)


def decor_sniper(c, style):
    width = c.width
    c.rect(width - style.track_width - 4, 8, 2, 2, rgb(240, 250, 255))
    c.set(width - style.track_width - 4, 8, rgb(120, 220, 255))


def decor_heavy(c, style):
    width, height = c.width, c.height
    tw = style.track_width
    plate = shade(style.body, 0.85)
    c.bevel(tw + 1, 8, width - 2 * tw - 2, 4, plate)
    c.bevel(tw + 1, height - 7, width - 2 * tw - 2, 4, plate)
    for x in (tw + 2, width - tw - 3):
        for y in (9, height - 6):
            c.set(x, y, rgb(210, 210, 220))


def decor_boss(c, style):
    width, height = c.width, c.height
    tw = style.track_width
    red = rgb(200, 40, 40)
    c.hline(tw, width - tw - 1, 7, red)
    c.hline(tw, width - tw - 1, height - 3, red)
    for x in (tw + 1, width - tw - 2):
        c.vline(x, 8, height - 4, shade(red, 0.75))
    for y in range(9, height - 4, 4):
        c.set(tw, y, rgb(230, 230, 230))
        c.set(width - tw - 1, y, rgb(230, 230, 230))


TANKS = {
    "player": ("player_tank", TankStyle((24, 29), (78, 150, 66), decor=decor_player)),
    "enemies/Basic": ("enemy_basic", TankStyle((24, 29), (176, 72, 58), turret=5.5, decor=decor_scout)),
    "enemies/Fast": ("enemy_fast", TankStyle((22, 27), (226, 172, 52), track=(70, 62, 50), track_width=5,
                                              turret=4.5, barrel_length=11, decor=decor_raider)),
    "enemies/Sniper": ("enemy_sniper", TankStyle((24, 29), (62, 150, 172), turret=5.0, barrels=(2,), muzzle=1,
                                                  decor=decor_sniper)),
    "enemies/Heavy": ("enemy_heavy", TankStyle((28, 30), (124, 76, 168), track_width=7, turret=7.0,
                                                barrels=(2, 2), decor=decor_heavy)),
    "enemies/Boss": ("enemy_boss", TankStyle((30, 30), (74, 76, 90), track=(40, 40, 46), track_width=7,
                                              turret=8.0, barrels=(2, 3, 2), barrel_length=15, decor=decor_boss)),
}
TANK_FRAMES = 3


def generate_tanks():
    for atlas, (name, style) in TANKS.items():
        save(draw_tank(style, 0), atlas, name)
        for frame in range(1, TANK_FRAMES + 1):
            save(draw_tank(style, frame), atlas, "%s_anim_%d" % (name, frame))


# ----------------------------------------------------------------- common

def draw_bullet():
    c = Canvas(6, 8)
    brass, tip = rgb(232, 190, 80), rgb(255, 245, 200)
    c.rect(1, 2, 4, 6, brass)
    c.rect(2, 0, 2, 2, tip)
    c.set(1, 1, tip)
    c.set(4, 1, tip)
    c.vline(1, 2, 7, shade(brass, 1.2))
    c.vline(4, 2, 7, shade(brass, 0.7))
    c.hline(1, 4, 6, rgb(150, 90, 40))
    return c


def draw_machine_gun_bullet():
    c = Canvas(3, 5)
    c.rect(0, 1, 3, 4, rgb(210, 200, 170))
    c.set(1, 0, rgb(255, 250, 220))
    c.vline(2, 1, 4, rgb(150, 140, 120))
    return c


def draw_explosion(index):
    sizes = [8, 12, 18, 24, 30]
    size = sizes[index]
    c = Canvas(size, size)
    rng = random.Random(100 + index)
    center = size / 2
    radius = size / 2
    smoke = index / 4.0
    palette = [rgb(255, 250, 220), rgb(255, 214, 90), rgb(246, 136, 40), rgb(200, 60, 30), rgb(90, 80, 80)]
    for y in range(size):
        for x in range(size):
            distance = math.hypot(x + 0.5 - center, y + 0.5 - center) / radius
            noise = rng.random() * 0.35
            if distance + noise * 0.6 > 1.0:
                continue
            level = distance * 0.9 + smoke * 0.3 + noise - 0.2
            step = max(0, min(len(palette) - 1, int(level * len(palette))))
            c.set(x, y, palette[step])
    return c


def draw_bonus(kind):
    c = Canvas(16, 16)
    colors = {
        "ammo": rgb(230, 186, 70),
        "repair": rgb(90, 210, 110),
        "rapid_fire": rgb(246, 132, 52),
        "shield": rgb(90, 160, 240),
    }
    accent = colors[kind]
    c.rect(1, 1, 14, 14, rgb(28, 30, 38))
    for x, y, w, h in ((1, 0, 14, 1), (1, 15, 14, 1), (0, 1, 1, 14), (15, 1, 1, 14)):
        c.rect(x, y, w, h, accent)
    c.rect(1, 1, 14, 1, shade(accent, 0.6))
    if kind == "ammo":
        for x in (4, 7, 10):
            c.rect(x, 6, 2, 6, accent)
            c.rect(x, 4, 2, 2, rgb(255, 240, 190))
            c.hline(x, x + 1, 10, shade(accent, 0.6))
    elif kind == "repair":
        c.rect(6, 3, 4, 10, accent)
        c.rect(3, 6, 10, 4, accent)
        c.rect(7, 4, 2, 8, shade(accent, 1.3))
        c.rect(4, 7, 8, 2, shade(accent, 1.3))
    elif kind == "rapid_fire":
        for top in (3, 8):
            for dx in range(4):
                c.set(4 + dx, top + 3 - dx, accent)
                c.set(11 - dx, top + 3 - dx, accent)
                c.set(4 + dx, top + 4 - dx, shade(accent, 0.7))
                c.set(11 - dx, top + 4 - dx, shade(accent, 0.7))
            c.rect(7, top, 2, 1, shade(accent, 1.3))
    elif kind == "shield":
        draw_shield(c, 8, 8, 5, accent)
    return c


def draw_shield(c, cx, cy, half, color):
    for y in range(-half, half + 2):
        span = half if y <= 0 else half - y
        for x in range(-span, span):
            c.set(cx + x, cy + y, color if x < 0 else shade(color, 0.8))
    c.vline(cx, cy - half, cy + half, shade(color, 1.4))


def draw_spawn_marker():
    c = Canvas(32, 32)
    red = rgb(240, 80, 70, 220)
    c.ring(16, 16, 15, 2, red, dash=8)
    c.ring(16, 16, 9, 1, rgb(255, 150, 120, 200))
    for d in range(3, 7):
        for x, y in ((16 - d, 16), (15 + d, 16), (16, 16 - d), (16, 15 + d)):
            c.set(x, y, red)
    c.rect(15, 15, 2, 2, rgb(255, 220, 200))
    return c


def generate_common():
    save(draw_bullet(), "common", "bullet")
    save(draw_machine_gun_bullet(), "common", "bullet_mg")
    for i in range(5):
        save(draw_explosion(i), "common", "boom%d" % (i + 1))
    for kind in ("ammo", "repair", "rapid_fire", "shield"):
        save(draw_bonus(kind), "common", "bonus_" + kind)
    save(draw_spawn_marker(), "common", "spawn_marker")


# ------------------------------------------------------------ ability icons

ICON = 22


def icon_canvas(color):
    c = Canvas(ICON, ICON)
    return c, rgb(*color)


def draw_icon_shoot():
    c, brass = icon_canvas((232, 190, 80))
    c.rect(8, 8, 6, 11, brass)
    c.rect(9, 4, 4, 4, rgb(255, 240, 200))
    c.rect(10, 2, 2, 2, rgb(255, 250, 230))
    c.vline(8, 8, 18, shade(brass, 1.25))
    c.vline(13, 8, 18, shade(brass, 0.65))
    c.hline(8, 13, 16, rgb(150, 90, 40))
    c.outline(BLACK)
    return c


def draw_icon_shield():
    c, blue = icon_canvas((80, 150, 236))
    for y in range(3, 20):
        half = 8 if y < 11 else 8 - (y - 10)
        for x in range(11 - half, 11 + half):
            c.set(x, y, blue if x < 11 else shade(blue, 0.8))
    c.vline(11, 4, 18, shade(blue, 1.4))
    c.hline(4, 17, 3, rgb(210, 235, 255))
    c.outline(BLACK)
    return c


def draw_icon_rapid_fire():
    c, orange = icon_canvas((246, 132, 52))
    for top in (3, 10):
        for dx in range(7):
            for t in range(3):
                c.set(4 + dx, top + 6 - dx + t, orange if t else shade(orange, 1.35))
                c.set(17 - dx, top + 6 - dx + t, orange if t else shade(orange, 1.35))
    c.outline(BLACK)
    return c


def draw_icon_speed():
    c, yellow = icon_canvas((250, 220, 80))
    bolt = [(12, 2), (11, 3), (10, 4), (9, 5), (8, 6), (7, 7), (6, 8), (6, 9), (7, 10), (8, 10), (9, 10),
            (10, 10), (11, 10), (11, 11), (10, 12), (9, 13), (8, 14), (7, 15), (6, 16), (5, 17)]
    for x, y in bolt:
        c.rect(x, y, 4, 2, yellow)
    for x, y in bolt[::2]:
        c.set(x + 1, y, shade(yellow, 1.25))
    c.outline(BLACK)
    return c


def draw_icon_armor():
    c, steel = icon_canvas((176, 184, 196))
    c.bevel(3, 3, 16, 16, steel, depth=2)
    c.rect(7, 7, 8, 8, shade(steel, 0.85))
    for x, y in ((5, 5), (16, 5), (5, 16), (16, 16)):
        c.set(x, y, rgb(240, 240, 250))
    c.outline(BLACK)
    return c


def draw_icon_machine_gun():
    c, steel = icon_canvas((150, 156, 168))
    c.rect(3, 9, 12, 5, steel)
    c.rect(15, 10, 5, 3, shade(steel, 0.8))
    c.rect(5, 14, 3, 5, shade(steel, 0.7))
    c.hline(3, 14, 9, shade(steel, 1.3))
    for x in range(4, 14, 3):
        c.set(x, 11, shade(steel, 0.55))
    for i, x in enumerate((16, 18, 20)):
        c.rect(x - 1, 4 + i, 2, 3, rgb(232, 190, 80))
    c.outline(BLACK)
    return c


def generate_icons():
    save(draw_icon_shoot(), "abilities", "icon_shoot")
    save(draw_icon_machine_gun(), "abilities", "icon_machine_gun")
    save(draw_icon_shield(), "abilities", "icon_shield")
    save(draw_icon_rapid_fire(), "abilities", "icon_rapid_fire")
    save(draw_icon_speed(), "abilities", "icon_speed")
    save(draw_icon_armor(), "abilities", "icon_armor")


# --------------------------------------------------------------- tilesets

class TilesetStyle:
    def __init__(self, ground, ground_detail, brick, mortar, stone, wood, seed):
        self.ground = rgb(*ground)
        self.ground_detail = [rgb(*color) for color in ground_detail]
        self.brick = rgb(*brick)
        self.mortar = rgb(*mortar)
        self.stone = rgb(*stone)
        self.wood = rgb(*wood)
        self.seed = seed


TILESETS = {
    "field": TilesetStyle(ground=(38, 46, 34), ground_detail=[(52, 82, 44), (70, 70, 66), (26, 30, 24)],
                          brick=(164, 74, 50), mortar=(120, 112, 100), stone=(128, 134, 146),
                          wood=(140, 96, 56), seed=1),
    "desert": TilesetStyle(ground=(118, 98, 66), ground_detail=[(138, 116, 80), (96, 78, 52), (150, 130, 96)],
                           brick=(196, 150, 92), mortar=(140, 108, 70), stone=(70, 64, 70),
                           wood=(110, 76, 50), seed=2),
}


def draw_ground(style, variant):
    c = Canvas(32, 32)
    rng = random.Random(style.seed * 100 + variant)
    for y in range(32):
        for x in range(32):
            c.set(x, y, shade(style.ground, 0.93 + rng.random() * 0.12))
    details = [0, 6, 12, 20][variant - 1]
    for _ in range(details):
        x, y = rng.randrange(1, 30), rng.randrange(1, 30)
        kind = rng.randrange(3)
        color = style.ground_detail[kind]
        if kind == 0:
            c.set(x, y, color)
            c.set(x - 1, y - 1, color)
            c.set(x + 1, y - 1, color)
            c.set(x, y - 2, shade(color, 1.2))
        elif kind == 1:
            c.rect(x, y, 2, 2, color)
            c.set(x, y, shade(color, 1.3))
        else:
            length = rng.randrange(3, 6)
            for i in range(length):
                c.set(x + i, y + (i // 2), color)
    return c


def brick_cells():
    cells = []
    for row in range(4):
        offset = 0 if row % 2 == 0 else -8
        x = offset
        while x < 32:
            cells.append((x, row * 8, 16, 8))
            x += 16
    return cells


def draw_brick(style, damage):
    c = Canvas(32, 32)
    rng = random.Random(style.seed * 10 + 7)
    c.rect(0, 0, 32, 32, style.mortar)
    cells = brick_cells()
    missing = set()
    order = list(range(len(cells)))
    random.Random(style.seed + 42).shuffle(order)
    missing.update(order[:[0, 1, 3, 6][damage]])
    for index, (x, y, w, h) in enumerate(cells):
        color = shade(style.brick, 0.88 + rng.random() * 0.22)
        left, right = max(0, x + 1), min(32, x + w - 1)
        if index in missing:
            for yy in range(y, y + h):
                for xx in range(max(0, x), min(32, x + w)):
                    c.set(xx, yy, CLEAR)
            continue
        c.rect(left, y + 1, right - left, h - 2, color)
        c.hline(left, right - 1, y + 1, shade(color, 1.25))
        c.hline(left, right - 1, y + h - 2, shade(color, 0.75))
    crack = shade(style.mortar, 0.35)
    crack_rng = random.Random(style.seed * 3 + damage)
    for _ in range(damage * 2):
        x, y = crack_rng.randrange(2, 30), crack_rng.randrange(2, 30)
        for i in range(5):
            c.set(x, y, crack)
            x += crack_rng.choice((-1, 0, 1))
            y += 1
    return c


def draw_stone(style):
    c = Canvas(32, 32)
    c.bevel(0, 0, 32, 32, style.stone, depth=2)
    inner = shade(style.stone, 0.9)
    c.bevel(5, 5, 22, 22, inner)
    rng = random.Random(style.seed * 5)
    for _ in range(26):
        c.set(rng.randrange(6, 26), rng.randrange(6, 26), shade(inner, 0.9 + rng.random() * 0.2))
    for x, y in ((3, 3), (28, 3), (3, 28), (28, 28)):
        c.rect(x, y, 2, 2, shade(style.stone, 1.5))
        c.set(x + 1, y + 1, shade(style.stone, 0.5))
    return c


def draw_wood(style, damage):
    c = Canvas(32, 32)
    rng = random.Random(style.seed * 11)
    broken = [(), (2,), (2, 0), (2, 0, 3)][damage]
    for plank in range(4):
        x = plank * 8
        color = shade(style.wood, 0.9 + rng.random() * 0.2)
        c.rect(x, 0, 8, 32, color)
        c.vline(x, 0, 31, shade(color, 0.55))
        c.vline(x + 1, 0, 31, shade(color, 1.2))
        for _ in range(3):
            gx, gy = x + rng.randrange(3, 7), rng.randrange(0, 26)
            c.vline(gx, gy, gy + rng.randrange(3, 7), shade(color, 0.8))
        for y in (3, 28):
            c.set(x + 4, y, rgb(200, 200, 205))
        if plank in broken:
            top = 10 + plank * 3
            for y in range(top, top + 9):
                jag = (y * 7 + plank) % 3
                for xx in range(x + jag, x + 8 - (2 - jag)):
                    c.set(xx, y, CLEAR)
    c.hline(0, 31, 14, shade(style.wood, 0.5))
    c.hline(0, 31, 15, shade(style.wood, 0.75))
    for damaged_y in range(damage):
        c.set(3 + damaged_y * 9, 20, shade(style.wood, 0.4))
    return c


def generate_tilesets():
    for name, style in TILESETS.items():
        atlas = "tilesets/" + name
        for variant in range(1, 5):
            save(draw_ground(style, variant), atlas, "ground_%d" % variant)
        # Damage is shown by removing sub-cells in the game, so a wall needs only its intact frame.
        save(draw_brick(style, 0), atlas, "wall_brick")
        save(draw_wood(style, 0), atlas, "wall_wood")
        save(draw_stone(style), atlas, "wall_stone")


# ------------------------------------------------------------------- icon

def draw_app_icon():
    """32x32 application icon: the player's tank on a dark rounded tile."""
    c = Canvas(32, 32)
    c.rect(1, 0, 30, 32, rgb(24, 30, 40))
    c.rect(0, 1, 32, 30, rgb(24, 30, 40))
    c.rect(2, 2, 28, 28, rgb(34, 44, 58))
    tank = draw_tank(TANKS["player"][1], 0)
    c.blit(tank, (32 - tank.width) // 2, (32 - tank.height) // 2 + 1)
    return c


def main():
    if "--hd" in sys.argv:
        import hd_art
        hd_art.generate()
        return
    generate_tanks()
    generate_common()
    generate_icons()
    generate_tilesets()
    os.makedirs(os.path.join(ART, "app"), exist_ok=True)
    pngio.save(draw_app_icon(), os.path.join(ART, "app", "icon.png"))
    print("art written to", ART)


if __name__ == "__main__":
    main()
