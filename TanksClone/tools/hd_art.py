"""HD placeholder art: every sprite at twice the size of the SD set, a tile is 64x64.

Written to art_hd/<atlas>/<frame>.png by "python tools/gen_placeholder_art.py --hd";
"python tools/pack_atlases.py --hd" packs it into Resources/textures_hd/ next to the SD atlases.
Frame names match the SD set, so the game can switch sets by search path; the HD set adds frames:
    <tank>_anim_1..8          track animation (one tread link is 8 px, it moves 1 px per frame)
    boom1..boom10             explosion
    muzzle_flash_1..3         shot flash, the barrel points up
    bullet_trail_1..4, bullet_mg_trail_1..2   trail drawn behind a bullet that flies up
"""

import math
import os
import random

import pngio
from gen_placeholder_art import BLACK, TANKS, TANK_PARTS, wreckify, TILESETS, TRACK_LAYER_FRAMES, TRACK_SIDES, Canvas, hit_frames, small_effect_frames, mix, rgb, scattered_points, shade, stamp_wrapped, tank_look_frames

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART_HD = os.path.join(ROOT, "art_hd")
SCALE = 2
TILE = 32 * SCALE
TRACK_FRAMES = 8
EXPLOSION_FRAMES = 10
SUPERSAMPLING = 4


def save(canvas, atlas, frame):
    folder = os.path.join(ART_HD, *atlas.split("/"))
    os.makedirs(folder, exist_ok=True)
    pngio.save(canvas, os.path.join(folder, frame + ".png"))


# ---------------------------------------------------------------- drawing

def blend(c, x, y, color, coverage=1.0):
    if coverage <= 0 or not (0 <= x < c.width and 0 <= y < c.height):
        return
    alpha = (color[3] if len(color) > 3 else 255) / 255.0 * min(1.0, coverage)
    under = c.get(x, y)
    under_alpha = under[3] / 255.0
    out_alpha = alpha + under_alpha * (1.0 - alpha)
    if out_alpha <= 0:
        return
    channels = [(color[i] * alpha + under[i] * under_alpha * (1.0 - alpha)) / out_alpha for i in range(3)]
    c.set(x, y, rgb(channels[0], channels[1], channels[2], int(round(out_alpha * 255))))


def fill_shape(c, inside, bounds, color):
    """Anti-aliased fill: coverage is counted on a SUPERSAMPLING x SUPERSAMPLING grid inside each pixel.

    color is either a colour or a function (x, y) -> colour."""
    x0, y0, x1, y1 = bounds
    step = 1.0 / SUPERSAMPLING
    samples = SUPERSAMPLING * SUPERSAMPLING
    for y in range(max(0, int(math.floor(y0))), min(c.height, int(math.ceil(y1)) + 1)):
        for x in range(max(0, int(math.floor(x0))), min(c.width, int(math.ceil(x1)) + 1)):
            hits = 0
            for sy in range(SUPERSAMPLING):
                for sx in range(SUPERSAMPLING):
                    if inside(x + (sx + 0.5) * step, y + (sy + 0.5) * step):
                        hits += 1
            if hits:
                blend(c, x, y, color(x, y) if callable(color) else color, hits / samples)


def polygon_test(points):
    def inside(px, py):
        result = False
        j = len(points) - 1
        for i in range(len(points)):
            xi, yi = points[i]
            xj, yj = points[j]
            if (yi > py) != (yj > py) and px < (xj - xi) * (py - yi) / (yj - yi) + xi:
                result = not result
            j = i
        return result
    return inside


def fill_polygon(c, points, color):
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    fill_shape(c, polygon_test(points), (min(xs), min(ys), max(xs), max(ys)), color)


def fill_circle(c, cx, cy, radius, color):
    fill_shape(c, lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 <= radius * radius,
               (cx - radius, cy - radius, cx + radius, cy + radius), color)


def fill_ring(c, cx, cy, outer, inner, color, dashes=0):
    def inside(x, y):
        distance = math.hypot(x - cx, y - cy)
        if not inner <= distance <= outer:
            return False
        if dashes:
            angle = math.atan2(y - cy, x - cx)
            return int((angle + math.pi) / (2 * math.pi) * dashes * 2) % 2 == 0
        return True
    fill_shape(c, inside, (cx - outer, cy - outer, cx + outer, cy + outer), color)


def lit_sphere(cx, cy, radius, base):
    """Colour function of a ball lit from the top left."""
    light_x, light_y = cx - radius * 0.45, cy - radius * 0.45

    def color(x, y):
        t = min(1.0, math.hypot(x + 0.5 - light_x, y + 0.5 - light_y) / (radius * 1.6))
        return mix(shade(base, 1.45), shade(base, 0.55), t)
    return color


def vertical_gradient(top_color, bottom_color, y0, y1):
    def color(x, y):
        return mix(top_color, bottom_color, max(0.0, min(1.0, (y - y0) / max(1, y1 - y0))))
    return color


def horizontal_gradient(left_color, right_color, x0, x1):
    def color(x, y):
        return mix(left_color, right_color, max(0.0, min(1.0, (x - x0) / max(1, x1 - x0))))
    return color


def gradient_rect(c, x, y, w, h, color):
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            c.set(xx, yy, color(xx, yy))


def bevel(c, x, y, w, h, color, depth=2):
    gradient_rect(c, x, y, w, h, vertical_gradient(shade(color, 1.1), shade(color, 0.85), y, y + h))
    for d in range(depth):
        light = shade(color, 1.35 - d * 0.12)
        dark = shade(color, 0.55 + d * 0.12)
        c.hline(x + d, x + w - 1 - d, y + d, light)
        c.vline(x + d, y + d, y + h - 1 - d, light)
        c.hline(x + d, x + w - 1 - d, y + h - 1 - d, dark)
        c.vline(x + w - 1 - d, y + d, y + h - 1 - d, dark)


def rivet(c, x, y, color):
    c.set(x, y, shade(color, 1.5))
    c.set(x + 1, y, shade(color, 1.1))
    c.set(x, y + 1, shade(color, 1.1))
    c.set(x + 1, y + 1, shade(color, 0.5))


def value_noise(seed, period, cell):
    """Smooth noise in [0, 1] that tiles every `period` pixels."""
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


# ------------------------------------------------------------------ tanks

def side_lefts(c, style, sides):
    tw = style.track_width * SCALE
    return [0 if side == "left" else c.width - tw for side in sides]


def draw_tracks(c, style, phase, hull_top, sides=TRACK_SIDES):
    width, height = c.width, c.height
    tw = style.track_width * SCALE
    base = style.track
    for left in side_lefts(c, style, sides):
        outer = left if left == 0 else left + tw - 1
        c.rect(left, hull_top - 4, tw, height - hull_top + 4, shade(base, 0.45))
        for y in range(hull_top - 3, height - 1):
            link = (y - phase) % TRACK_FRAMES
            if link in (0, 1):
                color = shade(base, 0.62)
            elif link == 2:
                color = shade(base, 1.3)
            else:
                color = shade(base, 1.0 - 0.04 * (link - 3))
            c.hline(left + 1, left + tw - 2, y, color)
            if link in (4, 5):
                c.set(outer, y, shade(base, 0.3))
            c.set(left + tw // 2, y, shade(color, 0.8))
        c.vline(left + (1 if left == 0 else tw - 2), hull_top - 3, height - 2, shade(base, 1.45))


def draw_hd_wheels(c, style, phase, hull_top, sides=TRACK_SIDES):
    width, height = c.width, c.height
    tw = style.track_width * SCALE
    span = height - hull_top + 2
    step = span // style.wheels
    radius = min(tw, step) / 2 - 0.5
    for i in range(style.wheels):
        cy = hull_top - 2 + step * i + step / 2
        for left in side_lefts(c, style, sides):
            cx = left + tw / 2
            fill_circle(c, cx, cy, radius, shade(style.track, 0.45))
            gradient_rect(c, left + 1, int(cy - radius + 1), tw - 2, int(radius * 2 - 2),
                          horizontal_gradient(shade(style.track, 1.25), shade(style.track, 0.7), left, left + tw))
            for k in range(3):
                y = int(cy - radius + 2 + (phase * 2 + k * radius * 0.66) % (radius * 2 - 3))
                c.hline(left + 1, left + tw - 2, y, shade(style.track, 0.55))
            fill_circle(c, cx, cy, radius * 0.35, rgb(150, 150, 156))


def draw_turret(c, style, cx, cy):
    radius = style.turret * SCALE
    fill_circle(c, cx + 1.5, cy + 1.5, radius, rgb(0, 0, 0, 110))
    fill_circle(c, cx, cy, radius, lit_sphere(cx, cy, radius, style.body))
    fill_ring(c, cx, cy, radius, radius - 1.2, shade(style.body, 0.45))
    hatch_x, hatch_y, hatch = cx + radius * 0.3, cy + radius * 0.28, radius * 0.3
    fill_circle(c, hatch_x, hatch_y, hatch, shade(style.body, 0.7))
    fill_ring(c, hatch_x, hatch_y, hatch, hatch - 1, shade(style.body, 1.25))


def draw_barrels(c, style, cx, cy):
    widths = [w * SCALE for w in style.barrels]
    length = (style.barrel_length * SCALE) if style.barrel_length else int(cy)
    spacing = widths[0] + 4
    count = len(widths)
    top = max(0, int(cy) - length)
    for i, barrel_width in enumerate(widths):
        bx = int(round(cx - barrel_width / 2 + (i - (count - 1) / 2) * spacing))
        gradient_rect(c, bx, top, barrel_width, int(cy) - top,
                      horizontal_gradient(shade(style.body, 0.95), shade(style.body, 0.38), bx, bx + barrel_width))
        brake = style.muzzle * SCALE
        if brake:
            bevel(c, bx - brake, top, barrel_width + 2 * brake, 4, shade(style.body, 0.5), depth=1)
        c.hline(bx, bx + barrel_width - 1, top, shade(style.body, 0.25))


def draw_hd_tank(style, decor, phase, part=None):
    width, height = style.size[0] * SCALE, style.size[1] * SCALE
    c = Canvas(width, height)
    tw = style.track_width * SCALE
    hull_top = 6 * SCALE
    cx, cy = width / 2, hull_top + (height - hull_top) * 0.55
    if part == "turret":
        draw_barrels(c, style, cx, cy)
        draw_turret(c, style, cx, cy)
        return c
    sides = (part[6:],) if part in ("track_left", "track_right") else (() if part == "hull" else TRACK_SIDES)
    if style.wheels:
        draw_hd_wheels(c, style, phase, hull_top, sides)
    else:
        draw_tracks(c, style, phase, hull_top, sides)
    if part in ("track_left", "track_right"):
        return c

    hull_left, hull_width = tw - 2, width - 2 * tw + 4
    bevel(c, hull_left, hull_top, hull_width, height - hull_top - 2, style.body)
    for y in range(height - 11, height - 4, 2):
        c.hline(tw + 2, width - tw - 3, y, shade(style.body, 0.7))
    for x in range(hull_left + 4, hull_left + hull_width - 4, 8):
        rivet(c, x, hull_top + 3, style.body)
    if decor:
        decor(c, style)

    if part not in ("hull", "body"):
        draw_barrels(c, style, cx, cy)
        draw_turret(c, style, cx, cy)
    return c


def hd_decor_player(c, style):
    tw = style.track_width * SCALE
    for offset, color in ((10, rgb(245, 214, 90)), (9, rgb(235, 200, 70)), (8, rgb(190, 150, 40))):
        c.hline(tw + 2, c.width - tw - 3, c.height - offset, color)


def hd_decor_scout(c, style):
    tw = style.track_width * SCALE
    for x in range(tw + 3, c.width - tw - 3, 6):
        c.rect(x, 17, 2, 2, shade(style.body, 0.6))


def hd_decor_raider(c, style):
    mid = c.width // 2
    for row in (c.height - 17, c.height - 11):
        fill_polygon(c, [(mid - 9, row), (mid, row + 5), (mid + 9, row), (mid + 9, row + 3), (mid, row + 8),
                         (mid - 9, row + 3)], BLACK)


def hd_decor_btr(c, style):
    tw = style.track_width * SCALE
    for x in range(tw + 3, c.width - tw - 3, 6):
        c.vline(x, 12, 16, shade(style.body, 1.35))
    gradient_rect(c, tw + 2, c.height - 12, c.width - 2 * tw - 4, 4,
                  vertical_gradient(shade(style.body, 0.85), shade(style.body, 0.55), c.height - 12, c.height - 8))
    for x in (tw + 3, c.width - tw - 6):
        fill_circle(c, x + 1.5, 15, 2, rgb(245, 240, 200))


def hd_decor_ram(c, style):
    tw = style.track_width * SCALE
    steel = rgb(170, 172, 180)
    blade = [(tw - 4, 6), (c.width - tw + 4, 6), (c.width - tw, 0), (tw, 0)]
    fill_polygon(c, blade, vertical_gradient(shade(steel, 1.3), shade(steel, 0.6), 0, 6))
    for x in range(tw + 2, c.width - tw - 2, 6):
        fill_polygon(c, [(x, 0), (x + 3, -3), (x + 6, 0)], shade(steel, 1.15))
    for y in range(18, c.height - 8, 8):
        c.hline(tw + 2, c.width - tw - 3, y, shade(style.body, 0.7))


def hd_decor_cloaker(c, style):
    tw = style.track_width * SCALE
    glint = rgb(170, 120, 230)
    for y in range(8, c.height - 6, 10):
        c.hline(tw + 2, c.width - tw - 3, y, shade(style.body, 1.25))
    c.rect(c.width // 2 - 9, 12, 3, 3, glint)
    c.rect(c.width // 2 + 6, 12, 3, 3, glint)


def hd_decor_minelayer(c, style):
    tw = style.track_width * SCALE
    rack = rgb(60, 62, 58)
    c.rect(tw, c.height - 14, c.width - 2 * tw, 12, shade(rack, 0.7))
    for x in range(tw + 2, c.width - tw - 6, 8):
        c.rect(x, c.height - 12, 6, 6, rack)
        c.rect(x + 2, c.height - 10, 2, 2, rgb(230, 70, 50))


def hd_decor_engineer(c, style):
    mid = c.width // 2
    cross = rgb(90, 220, 110)
    c.rect(mid - 3, c.height - 20, 6, 14, cross)
    c.rect(mid - 7, c.height - 16, 14, 6, cross)
    c.rect(mid - 2, c.height - 19, 4, 12, shade(cross, 1.3))


def hd_decor_mortar(c, style):
    mid = c.width // 2
    tube = rgb(70, 74, 66)
    fill_polygon(c, [(mid - 9, 12), (mid + 9, 12), (mid + 9, 30), (mid - 9, 30)],
                 vertical_gradient(shade(tube, 1.3), shade(tube, 0.6), 12, 30))
    c.rect(mid - 6, 15, 12, 10, rgb(18, 18, 18))
    c.hline(mid - 8, mid + 7, 13, shade(tube, 1.5))


def hd_decor_shield(c, style):
    plate = rgb(120, 150, 200)
    fill_polygon(c, [(0, 12), (2, 0), (c.width - 2, 0), (c.width, 12), (c.width - 4, 8), (4, 8)],
                 vertical_gradient(shade(plate, 1.35), shade(plate, 0.6), 0, 12))
    for x in range(8, c.width - 8, 10):
        c.rect(x, 3, 2, 2, shade(plate, 0.5))


def hd_decor_splitter(c, style):
    mid = c.width // 2
    for y in range(14, c.height - 2):
        x = mid - 2 + (y // 4) % 2 * 2
        c.rect(x, y, 2, 1, rgb(250, 240, 120))


def hd_decor_sniper(c, style):
    x = c.width - style.track_width * SCALE - 9
    bevel(c, x, 15, 6, 6, rgb(60, 66, 80), depth=1)
    fill_circle(c, x + 3, 18, 1.8, rgb(120, 220, 255))
    c.set(x + 2, 17, rgb(240, 250, 255))


def hd_decor_heavy(c, style):
    tw = style.track_width * SCALE
    plate = shade(style.body, 0.85)
    for y in (16, c.height - 15):
        bevel(c, tw + 2, y, c.width - 2 * tw - 4, 8, plate)
        for x in (tw + 4, c.width - tw - 6):
            rivet(c, x, y + 3, rgb(200, 200, 210))


def hd_decor_boss(c, style):
    tw = style.track_width * SCALE
    red = rgb(200, 40, 40)
    for y in (14, 15, c.height - 6, c.height - 5):
        c.hline(tw, c.width - tw - 1, y, red if y % 2 == 0 else shade(red, 0.7))
    for x in (tw + 2, c.width - tw - 4):
        gradient_rect(c, x, 16, 2, c.height - 22, vertical_gradient(shade(red, 0.9), shade(red, 0.55), 16,
                                                                    c.height - 6))
    for y in range(18, c.height - 8, 8):
        rivet(c, tw, y, rgb(230, 230, 230))
        rivet(c, c.width - tw - 2, y, rgb(230, 230, 230))


HD_DECOR = {
    "player": hd_decor_player,
    "enemies/Basic": hd_decor_scout,
    "enemies/Fast": hd_decor_raider,
    "enemies/Btr": hd_decor_btr,
    "enemies/Ram": hd_decor_ram,
    "enemies/Cloaker": hd_decor_cloaker,
    "enemies/Minelayer": hd_decor_minelayer,
    "enemies/Engineer": hd_decor_engineer,
    "enemies/Mortar": hd_decor_mortar,
    "enemies/ShieldBearer": hd_decor_shield,
    "enemies/Splitter": hd_decor_splitter,
    "enemies/Sniper": hd_decor_sniper,
    "enemies/Heavy": hd_decor_heavy,
    "enemies/Boss": hd_decor_boss,
}


def generate_tanks():
    for atlas, (name, style) in TANKS.items():
        decor = HD_DECOR[atlas]
        save(draw_hd_tank(style, decor, 0), atlas, name)
        for part in TANK_PARTS:
            save(draw_hd_tank(style, decor, 0, part), atlas, "%s_%s" % (name, part))
        save(wreckify(draw_hd_tank(style, decor, 0, "body"), style, SCALE, sum(map(ord, name))), atlas, name + "_wreck")
        for frame in range(1, TRACK_FRAMES + 1):
            save(draw_hd_tank(style, decor, frame, "hull"), atlas, "%s_anim_%d" % (name, frame))
        for side in TRACK_SIDES:
            for frame in range(1, TRACK_LAYER_FRAMES + 1):
                # 8 links per period, 4 frames: two links a frame
                save(draw_hd_tank(style, decor, frame * 2, "track_" + side), atlas, "%s_track_%s_%d" % (name, side, frame))


# ----------------------------------------------------------------- shots

def draw_shell(width, height, brass, tip):
    c = Canvas(width, height)
    nose = height * 0.35
    body = horizontal_gradient(shade(brass, 1.3), shade(brass, 0.6), 0, width)
    gradient_rect(c, 1, int(nose), width - 2, height - int(nose), body)
    fill_polygon(c, [(1, nose), (width / 2, 0), (width - 1, nose)],
                 horizontal_gradient(shade(tip, 1.1), shade(tip, 0.7), 0, width))
    c.hline(1, width - 2, height - 3, shade(brass, 0.5))
    c.hline(1, width - 2, height - 1, shade(brass, 0.45))
    return c


def draw_explosion(index):
    c = Canvas(TILE, TILE)
    t = index / (EXPLOSION_FRAMES - 1)
    center = TILE / 2
    radius = 6 + 25 * (1 - (1 - t) ** 2)
    noise = value_noise(500 + index, TILE, 8)
    grain = random.Random(900 + index)
    palette = [rgb(255, 252, 230), rgb(255, 222, 110), rgb(250, 150, 46), rgb(212, 68, 32), rgb(96, 84, 82),
               rgb(60, 56, 58)]
    fade = 1.0 - max(0.0, t - 0.55) / 0.45 * 0.8
    for y in range(TILE):
        for x in range(TILE):
            distance = math.hypot(x + 0.5 - center, y + 0.5 - center) / radius
            jitter = noise(x, y) * 0.45 + grain.random() * 0.08
            if distance + jitter * 0.5 > 1.05:
                continue
            level = distance * 0.75 + t * 0.85 + jitter - 0.3
            position = max(0.0, min(len(palette) - 1.001, level * (len(palette) - 1)))
            low = int(position)
            color = mix(palette[low], palette[low + 1], position - low)
            edge = max(0.0, min(1.0, (1.05 - distance - jitter * 0.5) / 0.15))
            blend(c, x, y, rgb(color[0], color[1], color[2], int(255 * fade)), edge)
    if t < 0.5:
        sparks = random.Random(1200 + index)
        for _ in range(10):
            angle = sparks.random() * 2 * math.pi
            reach = radius * (1.05 + sparks.random() * 0.35)
            blend(c, int(center + math.cos(angle) * reach), int(center + math.sin(angle) * reach), rgb(255, 240, 170))
    return c


def draw_muzzle_flash(index):
    c = Canvas(24, 32)
    length = [30, 22, 12][index]
    spread = [11, 8, 5][index]
    base_y, mid = 31.5, 12
    fill_polygon(c, [(mid - spread * 0.45, base_y), (mid - spread, base_y - length * 0.45), (mid - spread * 0.3,
                     base_y - length * 0.6), (mid, base_y - length), (mid + spread * 0.3, base_y - length * 0.6),
                     (mid + spread, base_y - length * 0.45), (mid + spread * 0.45, base_y)],
                 rgb(250, 140, 40, 210))
    fill_polygon(c, [(mid - spread * 0.35, base_y), (mid - spread * 0.4, base_y - length * 0.5), (mid,
                     base_y - length * 0.82), (mid + spread * 0.4, base_y - length * 0.5),
                     (mid + spread * 0.35, base_y)], rgb(255, 220, 100, 235))
    fill_circle(c, mid, base_y - length * 0.22, max(1.5, spread * 0.3), rgb(255, 252, 230))
    return c


def draw_trail(width, height, index, frames, hot):
    c = Canvas(width, height)
    decay = 1.0 - index / frames
    length = height * (1.0 - 0.18 * index)
    for y in range(int(length)):
        t = y / max(1.0, length)
        half = (width / 2) * (1.0 - t * 0.7)
        color = mix(hot, rgb(150, 150, 150), min(1.0, t * 1.6))
        alpha = int(230 * decay * (1.0 - t) ** 1.4)
        for x in range(width):
            coverage = max(0.0, min(1.0, half - abs(x + 0.5 - width / 2) + 0.5))
            blend(c, x, y, rgb(color[0], color[1], color[2], alpha), coverage)
    return c


# ---------------------------------------------------------------- pickups

BONUS_COLORS = {
    "ammo": rgb(230, 186, 70),
    "repair": rgb(90, 210, 110),
    "rapid_fire": rgb(246, 132, 52),
    "shield": rgb(90, 160, 240),
    "coin": rgb(250, 200, 60),
}


def shield_points(cx, top, half, height):
    return [(cx - half, top), (cx + half, top), (cx + half, top + height * 0.45), (cx, top + height),
            (cx - half, top + height * 0.45)]


def chevron_points(cx, top, half, thickness):
    return [(cx - half, top + half), (cx, top), (cx + half, top + half), (cx + half, top + half + thickness),
            (cx, top + thickness), (cx - half, top + half + thickness)]


def draw_bonus(kind):
    size = 16 * SCALE
    c = Canvas(size, size)
    accent = BONUS_COLORS[kind]
    gradient_rect(c, 2, 2, size - 4, size - 4, vertical_gradient(rgb(44, 48, 60), rgb(20, 22, 30), 2, size - 2))
    for d in range(2):
        c.hline(2, size - 3, d, shade(accent, 1.2 - d * 0.3))
        c.hline(2, size - 3, size - 1 - d, shade(accent, 0.7 + d * 0.2))
        c.vline(d, 2, size - 3, shade(accent, 1.1 - d * 0.2))
        c.vline(size - 1 - d, 2, size - 3, shade(accent, 0.75 + d * 0.15))
    for x, y in ((1, 1), (size - 2, 1), (1, size - 2), (size - 2, size - 2)):
        c.set(x, y, accent)
    bright = horizontal_gradient(shade(accent, 1.3), shade(accent, 0.75), 6, size - 6)
    if kind == "ammo":
        for x in (8, 14, 20):
            gradient_rect(c, x, 12, 4, 13, horizontal_gradient(shade(accent, 1.3), shade(accent, 0.7), x, x + 4))
            fill_polygon(c, [(x, 12), (x + 2, 6), (x + 4, 12)], rgb(255, 240, 190))
            c.hline(x, x + 3, 22, shade(accent, 0.55))
    elif kind == "repair":
        fill_polygon(c, [(12, 6), (20, 6), (20, 12), (26, 12), (26, 20), (20, 20), (20, 26), (12, 26), (12, 20),
                         (6, 20), (6, 12), (12, 12)], bright)
        c.rect(15, 9, 2, 14, shade(accent, 1.45))
        c.rect(9, 15, 14, 2, shade(accent, 1.45))
    elif kind == "rapid_fire":
        for top in (6, 15):
            fill_polygon(c, chevron_points(16, top, 9, 5), bright)
    elif kind == "shield":
        fill_polygon(c, shield_points(16, 6, 9, 21), bright)
        c.vline(16, 7, 25, shade(accent, 1.5))
    elif kind == "coin":
        fill_circle(c, 16, 16, 11, shade(accent, 0.55))
        fill_circle(c, 16, 16, 9.5, bright)
        fill_ring(c, 16, 16, 7.5, 6.5, shade(accent, 0.7))
        fill_polygon(c, [(16, 9), (18, 14), (23, 14), (19, 17), (20.5, 22), (16, 19), (11.5, 22), (13, 17),
                         (9, 14), (14, 14)], shade(accent, 1.35))
        fill_circle(c, 12, 11, 1.5, rgb(255, 250, 225))
    return c


def draw_spawn_marker():
    c = Canvas(TILE, TILE)
    red = rgb(240, 80, 70, 220)
    fill_ring(c, 32, 32, 30.5, 27, red, dashes=12)
    fill_ring(c, 32, 32, 19, 17, rgb(255, 150, 120, 200))
    for d0, d1 in ((7, 14),):
        for points in ([(32 - d1, 31), (32 - d0, 31), (32 - d0, 33), (32 - d1, 33)],
                       [(32 + d0, 31), (32 + d1, 31), (32 + d1, 33), (32 + d0, 33)],
                       [(31, 32 - d1), (33, 32 - d1), (33, 32 - d0), (31, 32 - d0)],
                       [(31, 32 + d0), (33, 32 + d0), (33, 32 + d1), (31, 32 + d1)]):
            fill_polygon(c, points, red)
    fill_circle(c, 32, 32, 3, rgb(255, 220, 200))
    return c


def generate_tank_look():
    for name, canvas in tank_look_frames(SCALE) + hit_frames(SCALE) + small_effect_frames(SCALE):
        save(canvas, "common", name)


def generate_common():
    generate_tank_look()
    from gen_placeholder_art import draw_crate
    save(draw_crate(SCALE), "common", "crate")
    from gen_placeholder_art import draw_fuel_barrel
    save(draw_fuel_barrel(SCALE), "common", "fuel_barrel")
    save(draw_shell(6 * SCALE, 8 * SCALE, rgb(232, 190, 80), rgb(255, 240, 200)), "common", "bullet")
    save(draw_shell(3 * SCALE, 5 * SCALE, rgb(210, 200, 170), rgb(255, 250, 220)), "common", "bullet_mg")
    for index in range(EXPLOSION_FRAMES):
        save(draw_explosion(index), "common", "boom%d" % (index + 1))
    for index in range(3):
        save(draw_muzzle_flash(index), "common", "muzzle_flash_%d" % (index + 1))
    for index in range(4):
        save(draw_trail(10, 40, index, 4, rgb(255, 230, 140)), "common", "bullet_trail_%d" % (index + 1))
    for index in range(2):
        save(draw_trail(6, 24, index, 2, rgb(240, 236, 210)), "common", "bullet_mg_trail_%d" % (index + 1))
    for kind in BONUS_COLORS:
        save(draw_bonus(kind), "common", "bonus_" + kind)
    save(draw_spawn_marker(), "common", "spawn_marker")


# ------------------------------------------------------------- ability icons

ICON = 22 * SCALE


def finish_icon(c):
    c.outline(BLACK)
    return c


def draw_icon_shoot():
    c = Canvas(ICON, ICON)
    c.blit(draw_shell(14, 34, rgb(232, 190, 80), rgb(255, 240, 200)), 15, 5)
    return finish_icon(c)


def draw_icon_machine_gun():
    c = Canvas(ICON, ICON)
    steel = rgb(150, 156, 168)
    bevel(c, 5, 17, 25, 10, steel)
    gradient_rect(c, 30, 19, 11, 6, vertical_gradient(shade(steel, 1.1), shade(steel, 0.6), 19, 25))
    bevel(c, 9, 27, 7, 11, shade(steel, 0.7), depth=1)
    for x in range(8, 28, 5):
        c.rect(x, 21, 2, 2, shade(steel, 0.5))
    for i, x in enumerate((31, 35, 39)):
        c.blit(draw_shell(4, 8, rgb(232, 190, 80), rgb(255, 240, 200)), x - 2, 6 + i * 2)
    return finish_icon(c)


def draw_icon_shield():
    c = Canvas(ICON, ICON)
    blue = rgb(80, 150, 236)
    fill_polygon(c, shield_points(22, 5, 16, 34), horizontal_gradient(shade(blue, 1.3), shade(blue, 0.7), 6, 38))
    fill_polygon(c, shield_points(22, 9, 11, 25), horizontal_gradient(shade(blue, 1.05), shade(blue, 0.8), 11, 33))
    c.vline(22, 7, 36, shade(blue, 1.5))
    return finish_icon(c)


def draw_icon_rapid_fire():
    c = Canvas(ICON, ICON)
    orange = rgb(246, 132, 52)
    for top in (5, 20):
        fill_polygon(c, chevron_points(22, top, 15, 8), vertical_gradient(shade(orange, 1.35), shade(orange, 0.8),
                                                                           top, top + 23))
    return finish_icon(c)


def draw_icon_speed():
    c = Canvas(ICON, ICON)
    yellow = rgb(250, 220, 80)
    fill_polygon(c, [(27, 3), (11, 24), (21, 24), (15, 41), (33, 18), (23, 18), (29, 3)],
                 vertical_gradient(shade(yellow, 1.3), shade(yellow, 0.75), 3, 41))
    return finish_icon(c)


def draw_icon_armor():
    c = Canvas(ICON, ICON)
    steel = rgb(176, 184, 196)
    bevel(c, 6, 6, 32, 32, steel, depth=3)
    bevel(c, 14, 14, 16, 16, shade(steel, 0.85), depth=2)
    for x, y in ((9, 9), (33, 9), (9, 33), (33, 33)):
        rivet(c, x, y, steel)
    return finish_icon(c)


def generate_icons():
    save(draw_icon_shoot(), "abilities", "icon_shoot")
    save(draw_icon_machine_gun(), "abilities", "icon_machine_gun")
    save(draw_icon_shield(), "abilities", "icon_shield")
    save(draw_icon_rapid_fire(), "abilities", "icon_rapid_fire")
    save(draw_icon_speed(), "abilities", "icon_speed")
    save(draw_icon_armor(), "abilities", "icon_armor")


# --------------------------------------------------------------- tilesets

def draw_ground(style, variant):
    c = Canvas(TILE, TILE)
    noise = value_noise(style.seed * 100, TILE, 16)
    rng = random.Random(style.seed * 1000 + variant)
    for y in range(TILE):
        for x in range(TILE):
            c.set(x, y, shade(style.ground, 0.88 + noise(x, y) * 0.12 + rng.random() * 0.04))

    def draw_details(layer, offset):
        for x, y in scattered_points(rng, TILE, [16, 24, 32, 40][variant - 1]):
            x, y = offset + x, offset + y
            kind = rng.randrange(3)
            color = style.ground_detail[kind]
            if kind == 0:
                for blade in (-2, 0, 2):
                    height = 3 + rng.randrange(3)
                    for i in range(height):
                        layer.set(x + blade + (i * blade) // 4, y - i, shade(color, 0.9 + i * 0.1))
            elif kind == 1:
                size = 2 + rng.randrange(2)
                layer.rect(x, y, size, size, color)
                layer.set(x, y, shade(color, 1.35))
                layer.hline(x, x + size - 1, y + size, shade(style.ground, 0.7))
            else:
                for i in range(rng.randrange(5, 11)):
                    layer.set(x + i, y + (i // 3), color)
    stamp_wrapped(c, draw_details, margin=16)
    return c


def draw_brick(style):
    c = Canvas(TILE, TILE)
    noise = value_noise(style.seed * 7, TILE, 8)
    rng = random.Random(style.seed * 10 + 7)
    gradient_rect(c, 0, 0, TILE, TILE, lambda x, y: shade(style.mortar, 0.85 + noise(x, y) * 0.2))
    height, length = 8 * SCALE, 16 * SCALE
    for row in range(TILE // height):
        x = 0 if row % 2 == 0 else -length // 2
        while x < TILE:
            color = shade(style.brick, 0.86 + rng.random() * 0.24)
            left, right = max(0, x + 2), min(TILE, x + length - 1)
            top, bottom = row * height + 2, row * height + height - 1
            for yy in range(top, bottom):
                for xx in range(left, right):
                    c.set(xx, yy, shade(color, 0.92 + noise(xx * 3, yy * 3) * 0.16))
            c.hline(left, right - 1, top, shade(color, 1.3))
            c.hline(left, right - 1, top + 1, shade(color, 1.12))
            c.hline(left, right - 1, bottom - 1, shade(color, 0.7))
            for _ in range(4):
                c.set(rng.randrange(left, right), rng.randrange(top + 2, bottom - 1), shade(color, 0.75))
            x += length
    return c


def draw_stone(style):
    c = Canvas(TILE, TILE)
    bevel(c, 0, 0, TILE, TILE, style.stone, depth=4)
    inner = shade(style.stone, 0.9)
    bevel(c, 10, 10, TILE - 20, TILE - 20, inner, depth=2)
    rng = random.Random(style.seed * 5)
    for _ in range(90):
        c.set(rng.randrange(13, TILE - 13), rng.randrange(13, TILE - 13), shade(inner, 0.86 + rng.random() * 0.26))
    for x, y in ((5, 5), (TILE - 7, 5), (5, TILE - 7), (TILE - 7, TILE - 7)):
        fill_circle(c, x + 1, y + 1, 2.2, lit_sphere(x + 1, y + 1, 2.2, shade(style.stone, 1.2)))
    return c


def draw_wood(style):
    c = Canvas(TILE, TILE)
    rng = random.Random(style.seed * 11)
    plank = 8 * SCALE
    for index in range(TILE // plank):
        x = index * plank
        color = shade(style.wood, 0.88 + rng.random() * 0.22)
        wave = rng.random() * 6.28
        for yy in range(TILE):
            for xx in range(x, x + plank):
                grain = math.sin((xx - x) * 1.3 + math.sin(yy * 0.18 + wave) * 1.6)
                c.set(xx, yy, shade(color, 0.93 + grain * 0.07))
        c.vline(x, 0, TILE - 1, shade(color, 0.5))
        c.vline(x + 1, 0, TILE - 1, shade(color, 1.25))
        c.vline(x + plank - 1, 0, TILE - 1, shade(color, 0.72))
        for _ in range(2):
            kx, ky = x + rng.randrange(5, plank - 5), rng.randrange(6, TILE - 6)
            fill_circle(c, kx, ky, 1.8, shade(color, 0.6))
        for y in (6, TILE - 8):
            rivet(c, x + plank // 2 - 1, y, rgb(190, 190, 196))
    gradient_rect(c, 0, 28, TILE, 6, vertical_gradient(shade(style.wood, 1.05), shade(style.wood, 0.55), 28, 34))
    c.hline(0, TILE - 1, 28, shade(style.wood, 1.3))
    return c


def generate_tilesets():
    for name, style in TILESETS.items():
        atlas = "tilesets/" + name
        for variant in range(1, 5):
            save(draw_ground(style, variant), atlas, "ground_%d" % variant)
        save(draw_brick(style), atlas, "wall_brick")
        save(draw_wood(style), atlas, "wall_wood")
        save(draw_stone(style), atlas, "wall_stone")


def generate():
    generate_tanks()
    generate_common()
    generate_icons()
    generate_tilesets()
    print("HD art written to", ART_HD)
