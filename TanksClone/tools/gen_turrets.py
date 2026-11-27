"""Draws realistic tank turrets and barrels (top view, muzzle up) into the HD tank layers.

For every tank of gen_placeholder_art.TANKS it writes into art_hd/<atlas>/
  <name>_turret.png   the turret layer: shadow, barrels (segments, fume extractor, muzzle brake), mantlet, cast turret with hatch,
                      cupola and rear bustle (kind per tank: dome, angular or box)
  <name>.png          the whole tank: hull and tracks (hd_art) with this turret on top
Everything is drawn 8x supersampled and shrunk, so the same code serves any texture size later.
Run it after tools/hd_art.py (which draws the old turrets), then tools/gen_sd_tanks.py and tools/pack_atlases.py --hd.
Usage: python tools/gen_turrets.py [--preview out.png]
"""

import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_placeholder_art as base  # noqa: E402
import hd_art  # noqa: E402
import pngio  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SUPER = 8
SCALE = hd_art.SCALE
STEEL = (118, 122, 130)
KIND = {"enemies/Fast": "angular", "enemies/Heavy": "angular", "enemies/Boss": "angular", "enemies/Cloaker": "angular",
        "enemies/Splitter": "angular", "enemies/Engineer": "box", "enemies/Minelayer": "box", "enemies/Mortar": "box"}


def tint(color, factor):
    return tuple(max(0, min(255, int(channel * factor))) for channel in color[:3])


def mix(a, b, share):
    return tuple(int(a[i] * (1 - share) + b[i] * share) for i in range(3))


class Layer:
    def __init__(self, width, height):
        self.width, self.height = width, height
        self.size = (width * SUPER, height * SUPER)
        self.image = Image.new("RGBA", self.size, (0, 0, 0, 0))

    def mask(self, paint):
        mask = Image.new("L", self.size, 0)
        paint(ImageDraw.Draw(mask), lambda value: value * SUPER)
        return mask

    def fill(self, mask, color, mode="vert", light=(0.0, 0.0), radius=1.0, rim=0.38):
        bounds = mask.getbbox()
        if not bounds:
            return
        ys, xs = np.mgrid[0:self.size[1], 0:self.size[0]].astype(np.float32)
        if mode == "radial":
            distance = np.hypot(xs - light[0] * SUPER, ys - light[1] * SUPER) / (radius * SUPER)
            factor = 1.38 - 0.95 * np.clip(distance, 0, 1.5)
        elif mode == "horiz":
            t = (xs - bounds[0]) / max(1, bounds[2] - bounds[0])
            factor = 1.4 - 1.0 * np.clip(np.abs(t - 0.3) * 1.5, 0, 1)
        elif mode == "vert":
            t = (ys - bounds[1]) / max(1, bounds[3] - bounds[1])
            factor = 1.25 - 0.6 * np.clip(t, 0, 1)
        else:
            factor = np.full(xs.shape, float(mode))
        shaded = np.clip(np.array(color[:3], dtype=np.float32)[None, None, :] * factor[:, :, None], 0, 255).astype(np.uint8)
        layer = Image.fromarray(np.dstack([shaded, np.full(xs.shape, 255, np.uint8)]), "RGBA")
        if rim:
            dilated = np.array(mask.filter(ImageFilter.MaxFilter(SUPER + 1))).astype(int)
            edge = Image.fromarray(np.clip(dilated - np.array(mask).astype(int), 0, 255).astype(np.uint8))
            outline = Image.new("RGBA", self.size, tint(color, rim) + (255,))
            self.image.paste(outline, (0, 0), edge)
        self.image.paste(layer, (0, 0), mask)

    def shadow(self, mask, dx, dy, opacity):
        blurred = mask.filter(ImageFilter.GaussianBlur(SUPER * 0.5)).point(lambda v: int(v * opacity / 255))
        black = Image.new("RGBA", self.size, (0, 0, 0, 255))
        black.putalpha(blurred)
        self.image.alpha_composite(black, (int(dx * SUPER), int(dy * SUPER)))

    def result(self):
        return self.image.resize((self.width, self.height), Image.LANCZOS)


def ellipse(cx, cy, rx, ry):
    return lambda d, p: d.ellipse([p(cx - rx), p(cy - ry), p(cx + rx), p(cy + ry)], fill=255)


def rect(x0, y0, x1, y1, corner=0.0):
    return lambda d, p: d.rounded_rectangle([p(x0), p(y0), p(x1), p(y1)], radius=p(corner), fill=255)


def polygon(points):
    return lambda d, p: d.polygon([(p(x), p(y)) for x, y in points], fill=255)


def shrink(points, cx, cy, share):
    return [(cx + (x - cx) * share, cy + (y - cy) * share) for x, y in points]


def draw_barrels(layer, style, cx, cy, steel):
    widths = [w * SCALE for w in style.barrels]
    length = (style.barrel_length * SCALE) if style.barrel_length else int(cy)
    spacing = widths[0] + 4
    top = max(0.5, cy - length)
    count = len(widths)
    for i, width in enumerate(widths):
        x0 = cx - width / 2 + (i - (count - 1) / 2) * spacing
        x1 = x0 + width
        layer.fill(layer.mask(rect(x0, top, x1, cy)), steel, "horiz", rim=0.3)
        for share in (0.28, 0.72):
            y = top + (cy - top) * share
            layer.fill(layer.mask(rect(x0, y, x1, y + 0.7)), tint(steel, 0.5), 0.9, rim=0)
        if width >= 3:
            y = top + (cy - top) * 0.42
            layer.fill(layer.mask(rect(x0 - 0.7, y, x1 + 0.7, y + width * 0.9, 0.3)), tint(steel, 1.05), "horiz", rim=0.3)
        brake = max(0.7, style.muzzle * SCALE)
        layer.fill(layer.mask(rect(x0 - brake, top, x1 + brake, top + max(2.0, width * 1.1), 0.3)), tint(steel, 0.85), "horiz", rim=0.3)
        if width >= 3:
            for k in (0.35, 0.65):
                y = top + max(2.0, width * 1.1) * k
                layer.fill(layer.mask(rect(x0 - brake, y, x1 + brake, y + 0.45)), (10, 10, 12), 1.0, rim=0)
        layer.fill(layer.mask(rect(x0 + width * 0.28, top, x1 - width * 0.28, top + 0.9)), (8, 8, 10), 1.0, rim=0)
    span = (count - 1) * spacing + max(widths)
    return top, span


def draw_dome(layer, body, cx, cy, r, tiny):
    outline = ellipse(cx, cy, r, r * 1.05)
    layer.shadow(layer.mask(outline), 1.4, 1.6, 130)
    layer.fill(layer.mask(outline), body, "radial", light=(cx - 0.3 * r, cy - 0.35 * r), radius=r * 1.9)
    layer.fill(layer.mask(ellipse(cx, cy - 0.04 * r, r * 0.74, r * 0.78)), tint(body, 1.1), "radial", light=(cx - 0.3 * r, cy - 0.4 * r), radius=r * 1.5, rim=0.7)


def draw_angular(layer, body, cx, cy, r, tiny):
    points = [(cx - 0.55 * r, cy - 1.12 * r), (cx + 0.55 * r, cy - 1.12 * r), (cx + r, cy - 0.4 * r), (cx + 0.92 * r, cy + 0.6 * r),
              (cx + 0.55 * r, cy + 1.05 * r), (cx - 0.55 * r, cy + 1.05 * r), (cx - 0.92 * r, cy + 0.6 * r), (cx - r, cy - 0.4 * r)]
    layer.shadow(layer.mask(polygon(points)), 1.4, 1.6, 130)
    layer.fill(layer.mask(polygon(points)), tint(body, 0.9), "vert")
    top = shrink(points, cx, cy - 0.05 * r, 0.7)
    layer.fill(layer.mask(polygon(top)), tint(body, 1.12), "radial", light=(cx - 0.3 * r, cy - 0.4 * r), radius=r * 1.6, rim=0.6)
    layer.fill(layer.mask(polygon([points[0], points[1], top[1], top[0]])), tint(body, 1.3), 1.0, rim=0)
    layer.fill(layer.mask(polygon([points[5], points[4], top[4], top[5]])), tint(body, 0.5), 1.0, rim=0)


def draw_box(layer, body, cx, cy, r, tiny):
    outline = rect(cx - r, cy - 0.95 * r, cx + r, cy + 0.95 * r, 0.25 * r)
    layer.shadow(layer.mask(outline), 1.4, 1.6, 130)
    layer.fill(layer.mask(outline), body, "vert")
    layer.fill(layer.mask(rect(cx - 0.62 * r, cy - 0.45 * r, cx + 0.62 * r, cy + 0.6 * r, 0.12 * r)), (16, 16, 18), 1.0, rim=0.0)
    layer.fill(layer.mask(rect(cx - 0.55 * r, cy - 0.38 * r, cx + 0.55 * r, cy + 0.2 * r, 0.1 * r)), tint(body, 0.35), "vert", rim=0)
    for sx in (-0.78, 0.78):
        layer.fill(layer.mask(ellipse(cx + sx * r, cy - 0.7 * r, 0.1 * r + 0.4, 0.1 * r + 0.4)), tint(body, 1.35), 1.0, rim=0.4)


def draw_details(layer, body, cx, cy, r, kind):
    if kind == "box":
        return
    hatch = (cx + 0.3 * r, cy + 0.08 * r)
    hr = max(1.0, 0.27 * r)
    layer.fill(layer.mask(ellipse(hatch[0], hatch[1], hr, hr)), tint(body, 0.8), "radial", light=(hatch[0] - hr * 0.3, hatch[1] - hr * 0.3), radius=hr * 2.2, rim=0.4)
    layer.fill(layer.mask(ellipse(hatch[0], hatch[1], hr * 0.6, hr * 0.6)), tint(body, 1.15), "radial", light=(hatch[0] - hr * 0.2, hatch[1] - hr * 0.2), radius=hr * 1.4, rim=0.5)
    layer.fill(layer.mask(rect(hatch[0] - hr * 0.5, hatch[1] - 0.3, hatch[0] + hr * 0.5, hatch[1] + 0.3)), tint(body, 0.4), 1.0, rim=0)
    if r >= 8:
        cupola = (cx - 0.45 * r, cy + 0.5 * r)
        cr = 0.2 * r
        layer.fill(layer.mask(ellipse(cupola[0], cupola[1], cr, cr)), tint(body, 0.6), "radial", light=(cupola[0] - cr * 0.3, cupola[1] - cr * 0.3), radius=cr * 2.4, rim=0.35)
        layer.fill(layer.mask(rect(cupola[0] - cr * 0.7, cupola[1] - 0.3, cupola[0] + cr * 0.7, cupola[1] + 0.3)), (12, 12, 14), 1.0, rim=0)


def draw_turret_layer(style, atlas, width, height):
    layer = Layer(width, height)
    cx, cy = width / 2, 6 * SCALE + (height - 6 * SCALE) * 0.55
    r = style.turret * SCALE
    body = style.turret_body
    steel = mix(body, STEEL, 0.55)
    kind = KIND.get(atlas, "dome")
    top, span = draw_barrels(layer, style, cx, cy, steel)
    layer.fill(layer.mask(rect(cx - span / 2 - 1.2, cy - r * 1.22, cx + span / 2 + 1.2, cy - r * 0.5, 0.6)), tint(steel, 0.85), "vert", rim=0.35)
    if kind != "box":
        layer.fill(layer.mask(rect(cx - 0.62 * r, cy + 0.6 * r, cx + 0.62 * r, cy + 1.3 * r, 0.25 * r)), tint(body, 0.62), "vert", rim=0.4)
    {"dome": draw_dome, "angular": draw_angular, "box": draw_box}[kind](layer, body, cx, cy, r, r < 8)
    draw_details(layer, body, cx, cy, r, kind)
    return layer.result()


def body_image(style, atlas):
    canvas = hd_art.draw_hd_tank(style, hd_art.HD_DECOR[atlas], 0, "body")
    path = os.path.join(ROOT, "art_hd", "_body_tmp.png")
    pngio.save(canvas, path)
    image = Image.open(path).convert("RGBA")
    os.remove(path)
    return image


def main():
    preview = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None
    sheet = Image.new("RGBA", (len(base.TANKS) * 150, 140), (40, 48, 40, 255))
    for index, (atlas, (name, style)) in enumerate(base.TANKS.items()):
        width, height = style.size[0] * SCALE, style.size[1] * SCALE
        turret = draw_turret_layer(style, atlas, width, height)
        whole = body_image(style, atlas)
        whole.alpha_composite(turret)
        if preview:
            sheet.alpha_composite(whole.resize((width * 2, height * 2), Image.NEAREST), (index * 150 + 6, 6))
            continue
        folder = os.path.join(ROOT, "art_hd", *atlas.split("/"))
        turret.save(os.path.join(folder, name + "_turret.png"))
        whole.save(os.path.join(folder, name + ".png"))
    if preview:
        sheet.convert("RGB").save(preview)
    print("ok")


if __name__ == "__main__":
    main()
