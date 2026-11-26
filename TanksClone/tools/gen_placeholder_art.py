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


def wrap_noise(seed, period, cell):
    """Smooth value noise in [0, 1] that repeats every `period` pixels, so opposite tile edges match."""
    cells = max(1, period // cell)
    rng = random.Random(seed)
    lattice = [[rng.random() for _ in range(cells)] for _ in range(cells)]

    def sample(x, y):
        fx, fy = x / cell, y / cell
        ix, iy = int(math.floor(fx)), int(math.floor(fy))
        tx, ty = fx - ix, fy - iy
        tx, ty = tx * tx * (3 - 2 * tx), ty * ty * (3 - 2 * ty)
        a = lattice[iy % cells][ix % cells]
        b = lattice[iy % cells][(ix + 1) % cells]
        d = lattice[(iy + 1) % cells][ix % cells]
        e = lattice[(iy + 1) % cells][(ix + 1) % cells]
        return (a + (b - a) * tx) * (1 - ty) + (d + (e - d) * tx) * ty
    return sample


def scattered_points(rng, size, count):
    """`count` points spread over a 4x4 grid of cells with jitter, so details neither clump nor line up."""
    grid = 4
    step = size // grid
    cells = [(cx, cy) for cy in range(grid) for cx in range(grid)]
    points = []
    while len(points) < count:
        rng.shuffle(cells)
        for cx, cy in cells[:count - len(points)]:
            points.append((cx * step + rng.randrange(step), cy * step + rng.randrange(step)))
    return points


def stamp_wrapped(target, draw, margin=8):
    """Draws onto a padded layer and folds it back with wraparound, so details crossing an edge continue on the opposite one."""
    layer = Canvas(target.width + margin * 2, target.height + margin * 2)
    draw(layer, margin)
    for y in range(layer.height):
        for x in range(layer.width):
            color = layer.get(x, y)
            if color[3]:
                target.set((x - margin) % target.width, (y - margin) % target.height, color)


def save(canvas, atlas, frame):
    folder = os.path.join(ART, *atlas.split("/"))
    os.makedirs(folder, exist_ok=True)
    pngio.save(canvas, os.path.join(folder, frame + ".png"))


# ------------------------------------------------------------------ tanks

class TankStyle:
    def __init__(self, size, body, track=(58, 60, 64), track_width=6, turret=6.0, barrels=(2,),
                 barrel_length=None, muzzle=0, decor=None, wheels=0):
        self.size = size
        self.body = rgb(*body)
        self.track = rgb(*track)
        self.track_width = track_width
        self.turret = turret
        self.barrels = barrels
        self.barrel_length = barrel_length
        self.muzzle = muzzle
        self.decor = decor
        self.wheels = wheels


TANK_PARTS = ("hull", "turret")
# Each track is its own layer so the left and right ones can run at different speeds (turning in place).
TRACK_SIDES = ("left", "right")
TRACK_LAYER_FRAMES = 4


def draw_tank(style, phase, part=None):
    """The whole tank, or one layer of it: "hull" (body, decor), "turret" (turret and barrels),
    "track_left" or "track_right" (one track or one row of wheels)."""
    width, height = style.size
    c = Canvas(width, height)
    tw = style.track_width
    hull_top = 6
    cx, cy = width / 2, hull_top + (height - hull_top) * 0.55
    if part in ("track_left", "track_right"):
        draw_running_gear(c, style, phase, hull_top, (part[6:],))
        return c
    if part in (None, "body"):
        draw_running_gear(c, style, phase, hull_top, TRACK_SIDES)
    if part != "turret":
        draw_hull(c, style, phase, hull_top)
    if part not in ("hull", "body"):
        draw_turret_layer(c, style, cx, cy)
    return c


def wreckify(c, style, scale, seed):
    """Turns a tank body (hull and tracks, no turret) into its burnt corpse: soot, scorch marks, embers,
    an empty turret ring and a broken track."""
    rng = random.Random(seed)
    width, height = c.width, c.height
    hull_top = 6 * scale
    cx, cy = width / 2, hull_top + (height - hull_top) * 0.55
    soot = rgb(36, 31, 28)
    for y in range(height):
        for x in range(width):
            color = c.get(x, y)
            if color[3]:
                burnt = mix(color, soot, 0.55)
                c.set(x, y, (burnt[0], burnt[1], burnt[2], color[3]))
    for _ in range(7):
        bx, by = rng.uniform(0, width), rng.uniform(hull_top, height)
        radius = rng.uniform(1.5, 3.5) * scale
        for y in range(max(0, int(by - radius)), min(height, int(by + radius) + 1)):
            for x in range(max(0, int(bx - radius)), min(width, int(bx + radius) + 1)):
                color = c.get(x, y)
                if color[3] and (x - bx) ** 2 + (y - by) ** 2 <= radius * radius:
                    dark = mix(color, rgb(16, 14, 13), 0.6)
                    c.set(x, y, (dark[0], dark[1], dark[2], color[3]))
    hole = style.turret * scale * 0.8
    c.disc(cx, cy, hole + scale, rgb(72, 66, 60))
    c.disc(cx, cy, hole, rgb(14, 12, 11))
    for _ in range(4 * scale):
        x, y = rng.randrange(width), rng.randrange(hull_top, height)
        if c.get(x, y)[3]:
            c.set(x, y, rgb(220, 100, 30) if rng.random() < 0.6 else rgb(250, 190, 60))
    tw = style.track_width * scale
    left = 0 if rng.random() < 0.5 else width - tw
    gap_top = rng.randrange(hull_top + 2 * scale, height - 5 * scale)
    for y in range(gap_top, gap_top + 3 * scale):
        for x in range(left, left + tw):
            c.set(x, y, (0, 0, 0, 0))
    return c


def track_lefts(c, style, sides):
    return [0 if side == "left" else c.width - style.track_width for side in sides]


def draw_running_gear(c, style, phase, hull_top, sides):
    width, height = style.size
    tw = style.track_width
    if style.wheels:
        draw_wheels(c, style, phase, hull_top, sides)
        return
    # treads move by one pixel per frame
    for left in track_lefts(c, style, sides):
        c.rect(left, hull_top - 2, tw, height - hull_top + 2, shade(style.track, 0.55))
        c.rect(left + 1, hull_top - 1, tw - 2, height - hull_top, style.track)
        for y in range(hull_top + (phase % TRACK_LAYER_FRAMES), height - 1, TRACK_LAYER_FRAMES):
            c.hline(left + 1, left + tw - 2, y, shade(style.track, 0.7))
        c.vline(left + (1 if left == 0 else tw - 2), hull_top - 1, height - 2, shade(style.track, 1.35))


def draw_hull(c, style, phase, hull_top):
    width, height = style.size
    tw = style.track_width
    # hull
    c.bevel(tw - 1, hull_top, width - 2 * tw + 2, height - hull_top - 1, style.body)
    c.hline(tw, width - tw - 1, height - 3, shade(style.body, 0.8))

    if style.decor:
        style.decor(c, style)



def draw_turret_layer(c, style, cx, cy):
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


def draw_wheels(c, style, phase, hull_top, sides):
    width, height = c.width, c.height
    tw = style.track_width
    span = height - hull_top
    wheel = max(4, span // style.wheels - 1)
    for i in range(style.wheels):
        top = hull_top - 1 + i * (span // style.wheels)
        for left in track_lefts(c, style, sides):
            c.rect(left, top, tw, wheel, shade(style.track, 0.5))
            c.rect(left + 1, top + 1, tw - 2, wheel - 2, style.track)
            c.hline(left + 1, left + tw - 2, top + 1 + (phase % max(1, wheel - 2)), shade(style.track, 1.5))


def decor_btr(c, style):
    width, height = c.width, c.height
    tw = style.track_width
    for x in range(tw + 1, width - tw - 1, 3):
        c.set(x, 7, shade(style.body, 1.35))
    c.rect(tw + 1, height - 6, width - 2 * tw - 2, 2, shade(style.body, 0.7))
    c.set(tw + 1, 8, rgb(240, 240, 200))
    c.set(width - tw - 2, 8, rgb(240, 240, 200))


def decor_ram(c, style):
    width = c.width
    tw = style.track_width
    steel = rgb(170, 172, 180)
    c.rect(tw - 2, 0, width - 2 * tw + 4, 3, shade(steel, 0.6))
    c.rect(tw - 1, 0, width - 2 * tw + 2, 2, steel)
    for x in range(tw, width - tw, 3):
        c.set(x, 0, shade(steel, 1.3))
    for y in range(9, c.height - 4, 4):
        c.hline(tw + 1, width - tw - 2, y, shade(style.body, 0.75))


def decor_cloaker(c, style):
    tw = style.track_width
    glint = rgb(170, 120, 230)
    for y in range(4, c.height - 3, 5):
        c.hline(tw + 1, c.width - tw - 2, y, shade(style.body, 1.25))
    c.set(c.width // 2 - 4, 6, glint)
    c.set(c.width // 2 + 3, 6, glint)


def decor_minelayer(c, style):
    tw = style.track_width
    rack = rgb(60, 62, 58)
    c.rect(tw, c.height - 7, c.width - 2 * tw, 6, shade(rack, 0.7))
    for x in range(tw + 1, c.width - tw - 2, 4):
        c.rect(x, c.height - 6, 3, 3, rack)
        c.set(x + 1, c.height - 5, rgb(230, 70, 50))


def decor_engineer(c, style):
    mid = c.width // 2
    cross = rgb(90, 220, 110)
    c.rect(mid - 1, c.height - 9, 3, 7, cross)
    c.rect(mid - 3, c.height - 7, 7, 3, cross)
    c.rect(style.track_width, c.height - 4, 3, 3, rgb(230, 190, 60))


def decor_mortar(c, style):
    mid = c.width // 2
    tube = rgb(70, 74, 66)
    c.rect(mid - 4, 6, 8, 9, shade(tube, 0.7))
    c.rect(mid - 3, 7, 6, 7, tube)
    c.rect(mid - 2, 8, 4, 4, rgb(20, 20, 20))
    c.hline(mid - 3, mid + 2, 7, shade(tube, 1.4))


def decor_shield(c, style):
    width = c.width
    plate = rgb(120, 150, 200)
    c.rect(1, 0, width - 2, 4, shade(plate, 0.6))
    c.rect(2, 0, width - 4, 3, plate)
    c.hline(3, width - 4, 0, shade(plate, 1.35))
    c.rect(0, 2, 2, 4, shade(plate, 0.7))
    c.rect(width - 2, 2, 2, 4, shade(plate, 0.7))


def decor_splitter(c, style):
    mid = c.width // 2
    for y in range(7, c.height - 1):
        c.set(mid - 1 + (y // 2) % 2, y, rgb(250, 240, 120))


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
    "enemies/Btr": ("enemy_btr", TankStyle((22, 30), (110, 128, 70), track=(46, 46, 50), track_width=5,
                                            turret=3.5, barrels=(1,), barrel_length=12, decor=decor_btr, wheels=4)),
    "enemies/Ram": ("enemy_ram", TankStyle((26, 30), (150, 96, 52), track=(52, 48, 44), track_width=6,
                                            turret=4.0, barrels=(2,), barrel_length=7, decor=decor_ram)),
    "enemies/Cloaker": ("enemy_cloaker", TankStyle((22, 28), (70, 60, 96), track=(40, 38, 48), track_width=5,
                                            turret=4.0, barrels=(1,), barrel_length=9, decor=decor_cloaker)),
    "enemies/Minelayer": ("enemy_minelayer", TankStyle((24, 30), (96, 104, 84), track=(48, 48, 46), track_width=6,
                                            turret=4.0, barrels=(1,), barrel_length=6, decor=decor_minelayer)),
    "enemies/Engineer": ("enemy_engineer", TankStyle((24, 28), (210, 168, 60), track=(52, 50, 46), track_width=5,
                                            turret=4.0, barrels=(1,), barrel_length=7, decor=decor_engineer)),
    "enemies/Mortar": ("enemy_mortar", TankStyle((26, 28), (120, 112, 80), track=(50, 48, 44), track_width=6,
                                            turret=3.0, barrels=(4,), barrel_length=3, decor=decor_mortar)),
    "enemies/ShieldBearer": ("enemy_shield_bearer", TankStyle((26, 28), (88, 98, 120), track=(48, 50, 58), track_width=6,
                                            turret=4.5, barrels=(2,), barrel_length=8, decor=decor_shield)),
    "enemies/Splitter": ("enemy_splitter", TankStyle((24, 28), (96, 160, 150), track=(50, 56, 56), track_width=5,
                                                      turret=4.5, barrels=(2, 2), barrel_length=9, decor=decor_splitter)),
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
        # The whole tank stays as the icon frame; in game the hull animates and the turret is its own layer.
        save(draw_tank(style, 0), atlas, name)
        for part in TANK_PARTS:
            save(draw_tank(style, 0, part), atlas, "%s_%s" % (name, part))
        save(wreckify(draw_tank(style, 0, "body"), style, 1, sum(map(ord, name))), atlas, name + "_wreck")
        for frame in range(1, TANK_FRAMES + 1):
            save(draw_tank(style, frame, "hull"), atlas, "%s_anim_%d" % (name, frame))
        for side in TRACK_SIDES:
            for frame in range(1, TRACK_LAYER_FRAMES + 1):
                save(draw_tank(style, frame, "track_" + side), atlas, "%s_track_%s_%d" % (name, side, frame))


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


def draw_fuel_barrel(scale=1):
    """A red fuel drum seen from above: rim, lid with a cap and a hazard mark."""
    size = 16 * scale
    c = Canvas(size, size)
    centre = size / 2
    c.disc(centre, centre, size / 2 - 0.5, rgb(90, 20, 18))
    c.disc(centre, centre, size / 2 - 1.5 * scale, rgb(200, 44, 36))
    c.ring(centre, centre, size / 2 - 3 * scale, max(1, scale), rgb(150, 30, 26))
    c.disc(centre - 1.5 * scale, centre - 1.5 * scale, size / 5, rgb(230, 80, 64))
    c.disc(centre + 2.5 * scale, centre - 2.5 * scale, 1.5 * scale, rgb(60, 60, 66))
    for i in range(-2 * scale, 2 * scale + 1):
        c.set(int(centre + i), int(centre + 2 * scale - abs(i)), rgb(250, 210, 60))
    c.rect(int(centre) - scale // 2, int(centre - scale), max(1, scale), 2 * scale, rgb(250, 210, 60))
    return c


def draw_crate(scale=1):
    """A wooden supply crate with metal corners; shooting it open drops the pickup inside."""
    size = 16 * scale
    c = Canvas(size, size)
    wood, dark, metal = rgb(176, 124, 66), rgb(110, 72, 36), rgb(150, 154, 162)
    c.rect(0, 0, size, size, dark)
    c.rect(scale, scale, size - 2 * scale, size - 2 * scale, wood)
    for y in range(3 * scale, size - scale, 4 * scale):
        c.hline(scale, size - scale - 1, y, shade(wood, 0.75))
    for i in range(size - 4 * scale):
        for t in range(scale):
            c.set(2 * scale + i, 2 * scale + i + t, dark)
            c.set(size - 3 * scale - i, 2 * scale + i + t, dark)
    for x, y in ((0, 0), (size - 3 * scale, 0), (0, size - 3 * scale), (size - 3 * scale, size - 3 * scale)):
        c.rect(x, y, 3 * scale, 3 * scale, metal)
        c.rect(x + scale, y + scale, scale, scale, shade(metal, 1.3))
    c.rect(size // 2 - 2 * scale, size // 2 - 2 * scale, 4 * scale, 4 * scale, rgb(235, 200, 70))
    c.rect(size // 2 - scale, size // 2 - scale, 2 * scale, 2 * scale, rgb(255, 245, 200))
    return c


def draw_bonus(kind):
    c = Canvas(16, 16)
    colors = {
        "ammo": rgb(230, 186, 70),
        "repair": rgb(90, 210, 110),
        "rapid_fire": rgb(246, 132, 52),
        "shield": rgb(90, 160, 240),
        "coin": rgb(250, 200, 60),
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
    elif kind == "coin":
        c.disc(8, 8, 5, shade(accent, 0.6))
        c.disc(8, 8, 4, accent)
        c.disc(7, 7, 2, shade(accent, 1.3))
        c.rect(8, 5, 1, 6, shade(accent, 0.55))
        c.set(6, 6, rgb(255, 250, 220))
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


# Logical 32x32 tank: the front (barrel) is the top of the image, the turret sits at TURRET with radius TURRET_RADIUS.
# Overlays stay off it, as if they were under the turret.
TURRET = (16.0, 20.0)
TURRET_RADIUS = 7.0
HULL = (6, 4, 26, 31)  # left, top, right, bottom

# Each crack stage adds one area: front, then back, then the sides.
CRACK_STAGE_PATHS = (
    ((11, 7, 1, 1), (21, 8, -1, 1)),
    ((12, 29, 1, -1), (21, 28, -1, -1)),
    ((7, 16, 1, 1), (25, 18, -1, 1)),
)


def under_turret(x, y):
    return (x + 0.5 - TURRET[0]) ** 2 + (y + 0.5 - TURRET[1]) ** 2 < TURRET_RADIUS ** 2


def draw_crack_path(c, rng, x, y, dx, dy, steps, scale, branch=True):
    """One jagged crack. SD: a 1px dark line. HD (same footprint): uneven core width, lit edge, shadow, chipped paint, hairlines."""
    dark, edge, soft = rgb(18, 16, 14, 240), rgb(215, 205, 180, 170), rgb(30, 26, 22, 90)
    chip, hair = rgb(170, 165, 150, 200), rgb(40, 34, 30, 150)
    left, top, right, bottom = HULL
    for step in range(steps):
        if rng.random() < 0.45:
            x += dx
        else:
            y += dy
        if rng.random() < 0.2:
            dx = -dx
        if not (left <= x < right and top <= y < bottom):
            return
        if under_turret(x, y):
            continue
        px, py = x * scale, y * scale
        if scale == 1:
            c.rect(px, py, 1, 1, dark)
        else:
            width = scale - 1 if step < steps - 3 else max(1, scale - 2)
            c.rect(px - 1, py - 1, scale + 2, scale + 2, soft)
            c.rect(px, py, width, scale, dark)
            c.rect(px + width, py, 1, scale, edge)
            if rng.random() < 0.35:
                c.rect(px + rng.choice((-2, scale + 1)), py + rng.randint(0, scale - 1), 1, 1, chip)
            if rng.random() < 0.25:
                hx, hy = px, py
                for _ in range(rng.randint(2, 4)):
                    hx += rng.choice((-1, 1))
                    hy += rng.choice((-1, 0, 1))
                    c.rect(hx, hy, 1, 1, hair)
        if branch and rng.random() < 0.18:
            draw_crack_path(c, rng, x, y, -dx, dy, rng.randint(2, 4), scale, False)


def draw_cracks(stage, size=32, scale=1):
    """Crack stage N holds every line of stage N-1 plus a new area, so the frame swap only adds damage."""
    c = Canvas(size, size)
    for area in range(min(len(CRACK_STAGE_PATHS), stage + 1)):
        for index, (sx, sy, dx, dy) in enumerate(CRACK_STAGE_PATHS[area]):
            draw_crack_path(c, random.Random(101 + area * 10 + index), sx, sy, dx, dy, 8, scale)
    return c


def draw_bullet_hole(scale=1):
    """A punched hole: black core, torn bright rim, soot around. HD has the same footprint with finer rim petals."""
    size = 7 * scale
    c = Canvas(size, size)
    x = y = size // 2
    if scale == 1:
        c.rect(x - 1, y - 1, 3, 3, rgb(40, 36, 32, 120))
        c.rect(x, y - 1, 1, 3, rgb(10, 8, 8))
        c.rect(x - 1, y, 3, 1, rgb(10, 8, 8))
        for ox, oy in ((-1, -1), (1, 1)):
            c.rect(x + ox, y + oy, 1, 1, rgb(210, 200, 175, 210))
        return c
    c.disc(x + 0.5, y + 0.5, 2.6 * scale, rgb(35, 30, 26, 90))
    for ang in range(0, 360, 45):
        r = 1.6 * scale
        c.rect(int(x + math.cos(math.radians(ang)) * r), int(y + math.sin(math.radians(ang)) * r), 1, 1, rgb(220, 210, 185, 220))
    c.disc(x + 0.5, y + 0.5, 1.3 * scale, rgb(60, 52, 46))
    c.disc(x + 0.5, y + 0.5, 0.9 * scale, rgb(8, 6, 6))
    c.rect(x - 1, y - 1, 1, 1, rgb(110, 100, 90))
    return c


def draw_dirt(size=32, scale=1):
    """Mud thrown up by the tracks: thick on the tracks and the front, none under the turret."""
    c = Canvas(size, size)
    rng = random.Random(37)
    grid = size / scale
    placed = 0
    while placed < int(120 * scale * scale):
        x, y = rng.uniform(0, grid), rng.uniform(3, grid - 1)
        on_tracks = x < 7 or x > grid - 7
        on_front = y < 11
        if under_turret(x, y) or not (on_tracks or on_front or rng.random() < 0.12):
            continue
        mud = rgb(72, 52, 30, rng.randint(210, 255)) if rng.random() < 0.7 else rgb(128, 96, 60, rng.randint(200, 245))
        c.disc(x * scale, y * scale, rng.uniform(0.7, 1.7) * scale, mud)
        placed += 1
    return c


def draw_lamp(size=6):
    """A white lamp with a highlight; the game tints it per AI behaviour."""
    c = Canvas(size, size)
    r = size / 2
    c.disc(r, r, r - 0.2, rgb(150, 150, 150))
    c.disc(r, r, r - 1.0, WHITE)
    c.disc(r - r * 0.35, r - r * 0.35, max(0.6, r * 0.25), rgb(255, 255, 255))
    return c


def draw_glow(size=16):
    """Soft white halo behind the lamp; tinted together with it."""
    c = Canvas(size, size)
    r = size / 2
    for step in range(int(r), 0, -1):
        c.disc(r, r, step, rgb(255, 255, 255, int(230 * (1 - step / r) ** 1.3)))
    return c


def draw_smoke_puff(size=12):
    c = Canvas(size, size)
    r = size / 2
    for step in range(int(r), 0, -1):
        c.disc(r, r, step, rgb(235, 235, 238, int(80 + 150 * (1 - step / r))))
    return c


def draw_soot_puff(size=16):
    """Thick, nearly opaque smoke of a badly hurt engine: several overlapping lobes."""
    c = Canvas(size, size)
    rng = random.Random(41)
    r = size / 2
    lobes = [(r, r, r * 0.62)] + [(r + rng.uniform(-0.3, 0.3) * size, r + rng.uniform(-0.3, 0.3) * size, r * rng.uniform(0.32, 0.45)) for _ in range(5)]
    for cx, cy, radius in lobes:
        for step in range(int(radius), 0, -1):
            c.disc(cx, cy, step, rgb(250, 250, 250, int(150 + 105 * (1 - step / radius))))
    return c


def draw_smoke_wisp(width=8, height=20):
    """A thin, soft, curling plume that smoulders over a wreck."""
    c = Canvas(width, height)
    for y in range(height):
        rise = y / (height - 1)
        cx = (width - 1) / 2 + math.sin(rise * 5.0) * width * 0.22
        radius = 1.0 + (1 - rise) * width * 0.3
        c.disc(cx, y, radius, rgb(240, 240, 240, int(40 + 90 * rise)))
    return c


def draw_track_mark(width=32, height=8, scale=1):
    """Two tread ruts with cross grooves, tiled along the path a tracked tank drives."""
    c = Canvas(width, height)
    tread = 7 * scale
    for x0 in (0, width - tread):
        for y in range(height):
            groove = (y // scale) % 2 == 0
            for x in range(x0, x0 + tread):
                edge = x in (x0, x0 + tread - 1)
                c.set(x, y, rgb(46, 36, 26, 150 if groove else (110 if edge else 90)))
    return c


def draw_wheel_mark(width=32, height=8, scale=1):
    """Two smooth tire lines, darker in the middle of each line."""
    c = Canvas(width, height)
    line = 3 * scale
    for x0 in (scale, width - scale - line):
        for y in range(height):
            for x in range(x0, x0 + line):
                inner = x0 < x < x0 + line - 1
                c.set(x, y, rgb(40, 32, 24, 140 if inner else 80))
    return c


CRACK_STAGES = 3


def tank_look_frames(scale=1):
    size = 32 * scale
    frames = [("damage_cracks_%d" % (stage + 1), draw_cracks(stage, size, scale)) for stage in range(CRACK_STAGES)]
    frames += [("damage_hole", draw_bullet_hole(scale)), ("damage_dirt", draw_dirt(size, scale)),
               ("lamp", draw_lamp(6 * scale)), ("lamp_glow", draw_glow(20 * scale)), ("smoke_puff", draw_smoke_puff(12 * scale)),
              ("smoke_soot", draw_soot_puff(16 * scale)), ("smoke_wisp", draw_smoke_wisp(8 * scale, 20 * scale)),
               ("mark_tracks", draw_track_mark(32 * scale, 8 * scale, scale)), ("mark_wheels", draw_wheel_mark(32 * scale, 8 * scale, scale))]
    return frames


# Hit sparks per material: (particle colours, particle size in logical pixels, count, spread in degrees).
HIT_KINDS = {
    "brick": ((rgb(196, 92, 60), rgb(150, 64, 44), rgb(220, 190, 160)), 1.0, 9, 70),
    "stone": ((rgb(170, 170, 176), rgb(120, 120, 128), rgb(255, 250, 220)), 1.0, 8, 60),
    "wood": ((rgb(168, 118, 64), rgb(120, 80, 40), rgb(210, 170, 110)), 1.0, 8, 80),
    "armor": ((rgb(255, 250, 210), rgb(255, 214, 90), rgb(246, 150, 50)), 0.6, 10, 55),
    "edge": ((rgb(190, 180, 160), rgb(150, 140, 120), rgb(220, 214, 200)), 1.2, 6, 90),
}
HIT_FRAMES = 3


def draw_hit(kind, index, scale=1):
    """One frame of a short burst flying back up (towards the shooter); the game turns it to face the shot."""
    colors, particle, count, spread = HIT_KINDS[kind]
    size = 14 * scale
    c = Canvas(size, size)
    rng = random.Random(sum(map(ord, kind)) + 7)
    progress = (index + 1) / HIT_FRAMES
    origin_x, origin_y = size / 2, size - 2 * scale
    if scale > 1:
        return draw_hit_hd(BlendCanvas(size, size), rng, kind, progress, origin_x, origin_y, size, scale)
    if kind == "armor" and index == 0:
        c.disc(origin_x, origin_y, 2.5 * scale, rgb(255, 255, 230, 230))
    for _ in range(count * scale):
        angle = math.radians(rng.uniform(-spread, spread))
        speed = rng.uniform(0.4, 1.0)
        distance = (size - 3 * scale) * speed * progress
        x, y = origin_x + math.sin(angle) * distance, origin_y - math.cos(angle) * distance
        alpha = int(255 * (1.0 - 0.6 * progress))
        color = rng.choice(colors)
        dot = max(1, int(round(particle * scale * (1.3 - 0.5 * progress))))
        if kind == "armor":
            tail = max(1, int(2 * scale * (1 - progress)))
            for step in range(tail):
                tx, ty = x - math.sin(angle) * step, y + math.cos(angle) * step
                c.rect(int(tx), int(ty), 1, 1, rgb(color[0], color[1], color[2], alpha // (step + 1)))
        c.rect(int(x), int(y), dot, dot, rgb(color[0], color[1], color[2], alpha))
    return c


class BlendCanvas(Canvas):
    """Draws over what is already there (alpha blending), so soft dust and glows build up instead of replacing pixels."""
    def set(self, x, y, color):
        if not (0 <= x < self.width and 0 <= y < self.height):
            return
        alpha = (color[3] if len(color) > 3 else 255) / 255.0
        under = self.get(x, y)
        under_alpha = under[3] / 255.0
        out_alpha = alpha + under_alpha * (1.0 - alpha)
        if out_alpha <= 0:
            return
        channels = [(color[i] * alpha + under[i] * under_alpha * (1.0 - alpha)) / out_alpha for i in range(3)]
        super().set(x, y, rgb(channels[0], channels[1], channels[2], int(round(out_alpha * 255))))


def draw_hit_hd(c, rng, kind, progress, origin_x, origin_y, size, scale):
    """HD keeps the SD footprint and adds detail: a dust cloud, lit and shaded chunks, splinters, glowing spark streaks."""
    colors, particle, count, spread = HIT_KINDS[kind]
    fade = 1.0 - 0.65 * progress
    if kind in ("brick", "stone", "edge", "wood"):
        dust = rgb(*(colors[2][:3]), 0)
        for _ in range(3 + scale):
            angle = math.radians(rng.uniform(-spread * 0.7, spread * 0.7))
            distance = (size * 0.3) * rng.uniform(0.2, 1.0) * (0.3 + progress)
            x, y = origin_x + math.sin(angle) * distance, origin_y - math.cos(angle) * distance
            radius = scale * rng.uniform(0.7, 1.2) * (0.5 + progress)
            for ring in range(int(radius), 0, -1):
                c.disc(x, y, ring, rgb(dust[0], dust[1], dust[2], int(22 * fade)))
    if kind == "armor":
        flash = 1.0 - progress
        if flash > 0.3:
            c.disc(origin_x, origin_y, 3.2 * scale * flash, rgb(255, 200, 90, int(120 * flash)))
            c.disc(origin_x, origin_y, 2.0 * scale * flash, rgb(255, 255, 235, int(240 * flash)))
    for _ in range(count + 4 * scale):
        angle = math.radians(rng.uniform(-spread, spread))
        speed = rng.uniform(0.35, 1.0)
        distance = (size - 3 * scale) * speed * progress
        dx, dy = math.sin(angle), -math.cos(angle)
        x, y = origin_x + dx * distance, origin_y + dy * distance
        alpha = int(255 * fade)
        base = rng.choice(colors[:2])
        if kind == "armor":
            length = int(3 * scale * (1.2 - progress))
            for step in range(length):
                share = 1 - step / max(1, length)
                tint = colors[1] if step < length / 2 else colors[2]
                c.rect(int(x - dx * step), int(y - dy * step), 1, 1, rgb(tint[0], tint[1], tint[2], int(alpha * share)))
            c.rect(int(x), int(y), 2, 2, rgb(255, 236, 150, alpha))
            c.rect(int(x), int(y), 1, 1, rgb(255, 255, 230, alpha))
        elif kind == "wood":
            length = rng.randint(scale, 2 * scale)
            for step in range(length):
                c.rect(int(x + dx * step * 0.6), int(y + step * 0.5), 2, 1, rgb(base[0], base[1], base[2], alpha))
            c.rect(int(x), int(y), 1, 1, rgb(colors[2][0], colors[2][1], colors[2][2], alpha))
        else:
            chunk = max(2, int(round(particle * scale * rng.uniform(0.7, 1.3) * (1.2 - 0.5 * progress))))
            dark = shade(base, 0.6)
            c.rect(int(x) + 1, int(y) + 1, chunk, chunk, rgb(0, 0, 0, int(alpha * 0.35)))
            c.rect(int(x), int(y), chunk, chunk, rgb(base[0], base[1], base[2], alpha))
            c.rect(int(x), int(y) + chunk - 1, chunk, 1, rgb(dark[0], dark[1], dark[2], alpha))
            c.rect(int(x), int(y), chunk - 1, 1, rgb(colors[2][0], colors[2][1], colors[2][2], alpha))
            if kind == "stone" and progress < 0.5 and rng.random() < 0.3:
                c.rect(int(x) - 1, int(y) - 1, 1, 1, rgb(255, 250, 220, alpha))
    return c


def hit_frames(scale=1):
    return [("hit_%s_%d" % (kind, index + 1), draw_hit(kind, index, scale)) for kind in HIT_KINDS for index in range(HIT_FRAMES)]


SMALL_EFFECT_FRAMES = {"muzzle_flash": 3, "bullet_fizzle": 3, "dust_puff": 3, "pickup_sparkle": 4}


def soft_disc(c, cx, cy, radius, color, alpha):
    """A disc that fades out towards its edge."""
    steps = max(2, int(radius))
    for step in range(steps):
        share = 1.0 - step / steps
        c.disc(cx, cy, radius * share, rgb(color[0], color[1], color[2], int(alpha * (0.35 + 0.65 * step / steps) / steps * 2)))


def draw_muzzle_flash(index, scale=1):
    """Flame from the barrel tip pointing up; the game turns it along the shot."""
    size = 10 * scale
    c = BlendCanvas(size, size)
    life = 1.0 - index / 3
    base_x, base_y = size / 2, size - 1
    length = (size - 2) * (0.55 + 0.45 * life)
    for layer, (color, width) in enumerate(((rgb(240, 120, 30), 0.45), (rgb(255, 210, 80), 0.3), (rgb(255, 250, 220), 0.15))):
        for y in range(int(length * (1 - 0.2 * layer))):
            half = max(0.5, size * width * (1 - y / length) * life + 0.5)
            alpha = int(230 * life)
            c.hline(int(base_x - half), int(base_x + half - 0.5), int(base_y - y), rgb(color[0], color[1], color[2], alpha))
    if index == 0:
        soft_disc(c, base_x, base_y - 1, 3 * scale, (255, 240, 180), 200)
    return c


def draw_bullet_fizzle(index, scale=1):
    """A spent bullet: a pop of grey dust and a few dying sparks."""
    size = 8 * scale
    c = BlendCanvas(size, size)
    rng = random.Random(31)
    progress = (index + 1) / 3
    soft_disc(c, size / 2, size / 2, size / 2 * (0.4 + 0.6 * progress), (180, 176, 168), int(200 * (1 - 0.6 * progress)))
    for _ in range(5):
        angle = rng.uniform(0, math.tau)
        distance = size / 2 * progress * rng.uniform(0.5, 1.0)
        c.rect(int(size / 2 + math.cos(angle) * distance), int(size / 2 + math.sin(angle) * distance), scale, scale,
               rgb(255, 220, 120, int(255 * (1 - progress) + 60)))
    return c


def draw_dust_puff(index, scale=1):
    """Ground dust kicked up behind a moving tank."""
    size = 10 * scale
    c = BlendCanvas(size, size)
    rng = random.Random(53)
    progress = (index + 1) / 3
    for _ in range(3):
        ox, oy = rng.uniform(-0.2, 0.2) * size, rng.uniform(-0.2, 0.2) * size
        soft_disc(c, size / 2 + ox, size / 2 + oy, size / 3 * (0.6 + 0.6 * progress), (176, 150, 110), int(110 * (1 - 0.7 * progress)))
    return c


def draw_pickup_sparkle(index, scale=1):
    """A ring and a four-point star that grow and fade where a bonus is taken."""
    size = 16 * scale
    c = BlendCanvas(size, size)
    progress = (index + 1) / 4
    center = size / 2
    alpha = int(255 * (1 - 0.75 * progress))
    c.ring(center, center, (size / 2 - 1) * progress, max(1, scale), rgb(255, 230, 120, alpha))
    arm = int((size / 2 - 1) * (1 - 0.5 * progress))
    for offset in range(-arm, arm + 1):
        fade = int(alpha * (1 - abs(offset) / (arm + 1)))
        c.set(int(center + offset), int(center), rgb(255, 255, 220, fade))
        c.set(int(center), int(center + offset), rgb(255, 255, 220, fade))
    soft_disc(c, center, center, 2 * scale * (1 - 0.5 * progress), (255, 255, 240), alpha)
    return c


def small_effect_frames(scale=1):
    draw = {"muzzle_flash": draw_muzzle_flash, "bullet_fizzle": draw_bullet_fizzle, "dust_puff": draw_dust_puff,
            "pickup_sparkle": draw_pickup_sparkle}
    return [("%s_%d" % (name, index + 1), draw[name](index, scale))
            for name, count in SMALL_EFFECT_FRAMES.items() for index in range(count)]


def generate_tank_look():
    for name, canvas in tank_look_frames() + hit_frames() + small_effect_frames():
        save(canvas, "common", name)


def generate_common():
    generate_tank_look()
    save(draw_bullet(), "common", "bullet")
    save(draw_machine_gun_bullet(), "common", "bullet_mg")
    save(draw_crate(), "common", "crate")
    save(draw_fuel_barrel(), "common", "fuel_barrel")
    for i in range(5):
        save(draw_explosion(i), "common", "boom%d" % (i + 1))
    for kind in ("ammo", "repair", "rapid_fire", "shield", "coin"):
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
    "snow": TilesetStyle(ground=(196, 206, 218), ground_detail=[(220, 228, 238), (170, 182, 198), (150, 168, 190)],
                         brick=(128, 92, 84), mortar=(176, 184, 196), stone=(96, 108, 124),
                         wood=(98, 80, 66), seed=3),
    "dirt": TilesetStyle(ground=(84, 66, 46), ground_detail=[(70, 96, 52), (150, 128, 86), (60, 46, 32)],
                         brick=(150, 82, 56), mortar=(104, 90, 70), stone=(100, 98, 92),
                         wood=(120, 84, 52), seed=7),
    "city": TilesetStyle(ground=(58, 60, 66), ground_detail=[(74, 76, 84), (44, 46, 52), (140, 134, 96)],
                         brick=(150, 70, 62), mortar=(100, 100, 108), stone=(120, 126, 136),
                         wood=(86, 72, 62), seed=6),
}


def draw_ground(style, variant):
    size = 32
    c = Canvas(size, size)
    noise = wrap_noise(style.seed * 100, size, 8)
    rng = random.Random(style.seed * 100 + variant)
    for y in range(size):
        for x in range(size):
            c.set(x, y, shade(style.ground, 0.93 + noise(x, y) * 0.07 + rng.random() * 0.03))
    details = [6, 10, 14, 20][variant - 1]

    def draw_details(layer, offset):
        for x, y in scattered_points(rng, size, details):
            x, y = offset + x, offset + y
            kind = rng.randrange(3)
            color = style.ground_detail[kind]
            if kind == 0:
                layer.set(x, y, color)
                layer.set(x - 1, y - 1, color)
                layer.set(x + 1, y - 1, color)
                layer.set(x, y - 2, shade(color, 1.2))
            elif kind == 1:
                layer.rect(x, y, 2, 2, color)
                layer.set(x, y, shade(color, 1.3))
            else:
                for i in range(rng.randrange(3, 6)):
                    layer.set(x + i, y + (i // 2), color)
    stamp_wrapped(c, draw_details)
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


def draw_base(scale=1):
    """The headquarters block: a gold armoured plate with a round emblem, unlike any wall."""
    size = 32 * scale
    c = Canvas(size, size)
    gold = rgb(214, 170, 52)
    c.bevel(0, 0, size, size, gold, depth=2 * scale)
    c.bevel(4 * scale, 4 * scale, size - 8 * scale, size - 8 * scale, shade(gold, 0.82), depth=scale)
    center = (size - 1) / 2
    navy = rgb(36, 58, 104)
    for y in range(size):
        for x in range(size):
            distance = math.hypot(x - center, y - center)
            diamond = abs(x - center) + abs(y - center)
            if diamond <= 5 * scale:
                c.set(x, y, shade(gold, 1.25 if x + y < size else 0.9))
            elif distance <= 8.5 * scale:
                c.set(x, y, navy)
            elif distance <= 10.5 * scale:
                c.set(x, y, rgb(232, 232, 226))
    for x, y in ((2, 2), (size - 4, 2), (2, size - 4), (size - 4, size - 4)):
        c.rect(x, y, 2 * scale, 2 * scale, shade(gold, 1.4))
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


def with_gap(c, width):
    """Edges fade in two steps (70%, 90% opaque from the outside in); the game adds a 50% rim outside the block, so touching blocks blend into a soft seam."""
    keep = (0.7, 0.9)
    for y in range(c.height):
        for x in range(c.width):
            step = min(x, y, c.width - 1 - x, c.height - 1 - y) // width
            if step < len(keep):
                r, g, b, a = c.get(x, y)
                c.pixels[y * c.width + x] = (r, g, b, int(a * keep[step]))
    return c


def generate_tilesets():
    for name, style in TILESETS.items():
        atlas = "tilesets/" + name
        for variant in range(1, 5):
            save(draw_ground(style, variant), atlas, "ground_%d" % variant)
        # Damage is shown by removing sub-cells in the game, so a wall needs only its intact frame.
        save(with_gap(draw_brick(style, 0), 1), atlas, "wall_brick")
        save(with_gap(draw_wood(style, 0), 1), atlas, "wall_wood")
        save(with_gap(draw_stone(style), 1), atlas, "wall_stone")
        save(with_gap(draw_base(), 1), atlas, "wall_base")


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
