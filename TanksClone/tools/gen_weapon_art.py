"""Draws shaded, realistic-looking weapon icons (side view, muzzle to the right) for the upgrade screen.

Each icon is drawn once at FULL (128 px, 4x supersampled), then written to
  art_hd/abilities/weapon_<id>.png   (HD, 64 px, smooth)
  art/abilities/weapon_<id>.png      (SD, 32 px, nearest-neighbour on a reduced palette: pixel-art look)
  art_fullhd/abilities/weapon_<id>.png (FullHD, 128 px)
Run tools/pack_atlases.py afterwards. Usage: python tools/gen_weapon_art.py [--preview out.png]
"""

import math
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FULL = 128
SUPER = 4
SD_SIZE = 32
HD_SIZE = 64
SD_COLORS = 24
FIT_SHARE = 0.95
TILT = 32
STEEL = (150, 160, 172)
DARK_STEEL = (70, 78, 90)
BRASS = (214, 170, 70)
WOOD = (140, 92, 52)
RED = (220, 60, 50)


def shade(color, factor):
    return tuple(max(0, min(255, int(channel * factor))) for channel in color)


class Art:
    def __init__(self):
        self.size = FULL * SUPER
        self.image = Image.new("RGBA", (self.size, self.size), (0, 0, 0, 0))
        self.draw = ImageDraw.Draw(self.image)

    def px(self, value):
        return int(round(value * SUPER))

    def cylinder(self, x0, x1, yc, radius, color):
        """A horizontal cylinder lit from the upper left: a vertical gradient with a highlight strip."""
        top, bottom = self.px(yc - radius), self.px(yc + radius)
        for row in range(top, bottom + 1):
            t = (row - top) / max(1, bottom - top)
            light = 1.25 - 0.95 * abs(t - 0.32) * 1.6
            self.draw.line([(self.px(x0), row), (self.px(x1), row)], fill=shade(color, light) + (255,))
        self.draw.line([(self.px(x0), top), (self.px(x1), top)], fill=shade(color, 0.45) + (255,), width=SUPER)
        self.draw.line([(self.px(x0), bottom), (self.px(x1), bottom)], fill=shade(color, 0.4) + (255,), width=SUPER)
        gloss = top + int((bottom - top) * 0.22)
        self.draw.line([(self.px(x0) + SUPER, gloss), (self.px(x1) - SUPER, gloss)], fill=(255, 255, 255, 120), width=SUPER)

    def box(self, x0, y0, x1, y1, color, rounded=2.0):
        """A rounded block with a vertical gradient."""
        mask = Image.new("L", self.image.size, 0)
        ImageDraw.Draw(mask).rounded_rectangle([self.px(x0), self.px(y0), self.px(x1), self.px(y1)], radius=self.px(rounded), fill=255)
        layer = Image.new("RGBA", self.image.size, (0, 0, 0, 0))
        layer_draw = ImageDraw.Draw(layer)
        top, bottom = self.px(y0), self.px(y1)
        for row in range(top, bottom + 1):
            t = (row - top) / max(1, bottom - top)
            layer_draw.line([(self.px(x0), row), (self.px(x1), row)], fill=shade(color, 1.2 - 0.8 * t) + (255,))
        self.image.paste(layer, (0, 0), mask)
        self.draw.rounded_rectangle([self.px(x0), self.px(y0), self.px(x1), self.px(y1)], radius=self.px(rounded), outline=shade(color, 0.35) + (255,), width=SUPER)

    def ring(self, x, yc, radius, half_width, color):
        self.cylinder(x - half_width, x + half_width, yc, radius, color)

    def disc(self, cx, cy, radius, color):
        mask = Image.new("L", self.image.size, 0)
        ImageDraw.Draw(mask).ellipse([self.px(cx - radius), self.px(cy - radius), self.px(cx + radius), self.px(cy + radius)], fill=255)
        layer = Image.new("RGBA", self.image.size, (0, 0, 0, 0))
        layer_draw = ImageDraw.Draw(layer)
        steps = self.px(radius)
        for step in range(steps, 0, -1):
            t = step / steps
            ox, oy = -self.px(radius) * 0.25 * (1 - t), -self.px(radius) * 0.25 * (1 - t)
            layer_draw.ellipse([self.px(cx) - step + ox, self.px(cy) - step + oy, self.px(cx) + step + ox, self.px(cy) + step + oy], fill=shade(color, 0.55 + 0.75 * (1 - t)) + (255,))
        self.image.paste(layer, (0, 0), mask)

    def shape(self, paint, color, outline=True):
        """Fills whatever `paint(draw, px)` draws on a mask with a lit vertical gradient and a dark edge."""
        mask = Image.new("L", self.image.size, 0)
        paint(ImageDraw.Draw(mask), self.px)
        layer = Image.new("RGBA", self.image.size, (0, 0, 0, 0))
        layer_draw = ImageDraw.Draw(layer)
        box = mask.getbbox()
        if not box:
            return
        top, bottom = box[1], box[3]
        for row in range(top, bottom + 1):
            t = (row - top) / max(1, bottom - top)
            layer_draw.line([(0, row), (self.size, row)], fill=shade(color, 1.3 - 0.85 * t) + (255,))
        if outline:
            edge = ImageChops.subtract(mask.filter(ImageFilter.MaxFilter(SUPER * 2 + 1)), mask)
            self.image.paste(Image.new("RGBA", self.image.size, shade(color, 0.3) + (255,)), (0, 0), edge)
        self.image.paste(layer, (0, 0), mask)

    def poly(self, points, color):
        self.draw.polygon([(self.px(x), self.px(y)) for x, y in points], fill=color + (255,))

    def result(self, tilt=0):
        # A soft drop shadow keeps the icon readable on the blueprint disc.
        alpha = self.image.getchannel("A").filter(ImageFilter.GaussianBlur(self.px(1.5)))
        shadow = Image.new("RGBA", self.image.size, (0, 0, 0, 0))
        shadow.putalpha(alpha.point(lambda v: int(v * 0.45)))
        out = Image.new("RGBA", self.image.size, (0, 0, 0, 0))
        out.alpha_composite(shadow, (self.px(1.5), self.px(2.5)))
        out.alpha_composite(self.image)
        if tilt:
            out = out.rotate(tilt, resample=Image.BICUBIC)
        box = out.getbbox()
        content = out.crop(box)
        fit = min(self.size * FIT_SHARE / content.width, self.size * FIT_SHARE / content.height)
        content = content.resize((max(1, int(content.width * fit)), max(1, int(content.height * fit))), Image.LANCZOS)
        framed = Image.new("RGBA", out.size, (0, 0, 0, 0))
        framed.alpha_composite(content, ((self.size - content.width) // 2, (self.size - content.height) // 2))
        return framed.resize((FULL, FULL), Image.LANCZOS)


def cannon(a):
    a.box(10, 56, 34, 78, DARK_STEEL, 4)
    a.cylinder(30, 112, 66, 7, STEEL)
    a.ring(36, 66, 11, 3, DARK_STEEL)
    a.ring(104, 66, 10, 5, STEEL)
    a.disc(108, 66, 2.5, (20, 22, 26))
    a.box(14, 78, 40, 90, DARK_STEEL, 3)


def heavy_cannon(a):
    a.box(8, 50, 40, 84, DARK_STEEL, 5)
    a.cylinder(34, 98, 67, 11, STEEL)
    a.ring(44, 67, 15, 4, DARK_STEEL)
    a.cylinder(94, 118, 67, 13, shade(STEEL, 0.9))
    for x in (99, 105, 111):
        a.draw.rectangle([a.px(x), a.px(56), a.px(x + 2), a.px(78)], fill=(25, 28, 34, 255))
    a.box(12, 84, 44, 98, DARK_STEEL, 3)


def machine_gun(a):
    a.box(8, 52, 46, 72, DARK_STEEL, 3)
    a.cylinder(44, 116, 60, 5, STEEL)
    a.cylinder(48, 88, 60, 9, shade(STEEL, 0.85))
    for x in range(52, 86, 6):
        a.draw.ellipse([a.px(x), a.px(56), a.px(x + 3), a.px(63)], fill=(25, 28, 34, 255))
    a.box(52, 72, 70, 96, BRASS, 2)
    a.box(16, 72, 28, 90, DARK_STEEL, 2)
    a.ring(114, 60, 7, 2, DARK_STEEL)


def shotgun(a):
    a.box(6, 56, 40, 80, WOOD, 6)
    a.cylinder(36, 116, 60, 5, STEEL)
    a.cylinder(36, 116, 70, 5, shade(STEEL, 0.92))
    a.box(54, 74, 84, 84, WOOD, 3)
    a.box(34, 54, 46, 78, DARK_STEEL, 2)
    a.ring(114, 60, 6, 2, DARK_STEEL)
    a.ring(114, 70, 6, 2, DARK_STEEL)


def flak(a):
    a.box(10, 62, 44, 92, DARK_STEEL, 4)
    for yc in (54, 66):
        a.cylinder(40, 108, yc, 5, STEEL)
        a.poly([(104, yc - 5), (118, yc - 9), (118, yc + 9), (104, yc + 5)], shade(STEEL, 0.9))
    a.box(20, 40, 50, 62, shade(DARK_STEEL, 1.2), 3)
    for x in (66, 82):
        a.draw.rectangle([a.px(x), a.px(44), a.px(x + 3), a.px(76)], fill=(35, 40, 48, 255))


def ricochet(a):
    a.box(8, 56, 36, 80, DARK_STEEL, 4)
    a.cylinder(32, 84, 68, 7, STEEL)
    a.ring(86, 68, 10, 3, BRASS)
    a.disc(98, 68, 6, (240, 210, 90))
    points = [(106, 62), (116, 40), (108, 22), (120, 12)]
    a.draw.line([(a.px(x), a.px(y)) for x, y in points], fill=(240, 210, 90, 255), width=a.px(3))
    a.poly([(120, 6), (124, 18), (113, 15)], (240, 210, 90))


def mine(a):
    a.disc(64, 76, 40, DARK_STEEL)
    a.disc(64, 70, 34, STEEL)
    for k in range(8):
        angle = k * math.pi / 4
        a.draw.polygon([(a.px(64 + 30 * math.cos(angle - 0.18)), a.px(70 + 30 * math.sin(angle - 0.18))),
                        (a.px(64 + 46 * math.cos(angle)), a.px(70 + 46 * math.sin(angle))),
                        (a.px(64 + 30 * math.cos(angle + 0.18)), a.px(70 + 30 * math.sin(angle + 0.18)))], fill=shade(DARK_STEEL, 0.9) + (255,))
    a.disc(64, 70, 16, shade(STEEL, 0.8))
    a.disc(64, 70, 9, RED)
    a.disc(61, 67, 3, (255, 190, 170))


def star_points(cx, cy, outer, inner, count, turn=0.0):
    points = []
    for k in range(count * 2):
        radius = outer if k % 2 == 0 else inner
        angle = turn + k * math.pi / count
        points.append((cx + radius * math.cos(angle), cy + radius * math.sin(angle)))
    return points


def stat_hp(a):
    def heart(draw, px, grow=0):
        draw.ellipse([px(14 - grow), px(18 - grow), px(66 + grow), px(70 + grow)], fill=255)
        draw.ellipse([px(62 - grow), px(18 - grow), px(114 + grow), px(70 + grow)], fill=255)
        draw.polygon([(px(17 - grow), px(52)), (px(111 + grow), px(52)), (px(64), px(116 + grow))], fill=255)
    a.shape(heart, STEEL)
    a.shape(lambda d, px: d.ellipse([px(24), px(26), px(46), px(46)], fill=255), (255, 235, 235), outline=False)
    a.shape(lambda d, px: (d.rectangle([px(56), px(42), px(72), px(86)], fill=255), d.rectangle([px(42), px(56), px(86), px(72)], fill=255)), BRASS)


def stat_armor(a):
    outer = [(64, 8), (112, 22), (108, 70), (64, 120), (20, 70), (16, 22)]
    inner = [(64, 22), (98, 32), (95, 66), (64, 104), (33, 66), (30, 32)]
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in outer], fill=255), DARK_STEEL)
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in inner], fill=255), STEEL)
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in star_points(64, 62, 26, 11, 5, -math.pi / 2)], fill=255), BRASS)


def stat_engine(a):
    teeth = []
    for k in range(16):
        angle = k * math.pi / 8
        for half, radius in ((-0.14, 44), (-0.1, 60), (0.1, 60), (0.14, 44)):
            teeth.append((64 + radius * math.cos(angle + half), 64 + radius * math.sin(angle + half)))
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in teeth], fill=255), STEEL)
    a.shape(lambda d, px: d.ellipse([px(24), px(24), px(104), px(104)], fill=255), DARK_STEEL)
    a.shape(lambda d, px: d.ellipse([px(40), px(40), px(88), px(88)], fill=255), BRASS)
    a.shape(lambda d, px: d.ellipse([px(54), px(54), px(74), px(74)], fill=255), DARK_STEEL)


def stat_tracks(a):
    a.shape(lambda d, px: d.rounded_rectangle([px(6), px(30), px(122), px(98)], radius=px(34), fill=255), DARK_STEEL)
    a.shape(lambda d, px: d.rounded_rectangle([px(16), px(40), px(112), px(88)], radius=px(24), fill=255), shade(DARK_STEEL, 0.55))
    for k in range(9):
        x = 14 + k * 12.5
        a.shape(lambda d, px, x=x: d.rectangle([px(x), px(24), px(x + 6), px(34)], fill=255), STEEL)
        a.shape(lambda d, px, x=x: d.rectangle([px(x), px(94), px(x + 6), px(104)], fill=255), STEEL)
    for x in (34, 64, 94):
        a.disc(x, 64, 14, STEEL)
        a.disc(x, 64, 5, DARK_STEEL)


def stat_reload(a):
    def ring(draw, px):
        draw.arc([px(16), px(16), px(112), px(112)], start=-60, end=235, fill=255, width=px(20))
    a.shape(ring, STEEL)
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in [(118, 44), (76, 52), (110, 94)]], fill=255), BRASS)


def stat_damage(a):
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in star_points(64, 64, 62, 34, 9, -math.pi / 2)], fill=255), DARK_STEEL)
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in star_points(64, 64, 48, 26, 9, -math.pi / 2)], fill=255), STEEL)
    a.shape(lambda d, px: d.polygon([(px(x), px(y)) for x, y in star_points(64, 64, 28, 15, 9, -math.pi / 2)], fill=255), BRASS)


def stat_ammo(a):
    for x, rise in ((8, 12), (50, 0), (92, 12)):
        a.shape(lambda d, px, x=x, rise=rise: d.rectangle([px(x), px(52 + rise), px(x + 28), px(118)], fill=255), BRASS)
        a.shape(lambda d, px, x=x, rise=rise: d.polygon([(px(x), px(52 + rise)), (px(x + 28), px(52 + rise)), (px(x + 24), px(32 + rise)), (px(x + 14), px(12 + rise)), (px(x + 4), px(32 + rise))], fill=255), STEEL)
        a.shape(lambda d, px, x=x, rise=rise: d.rectangle([px(x), px(100), px(x + 28), px(106)], fill=255), DARK_STEEL)


def stat_auto_fire(a):
    a.shape(lambda d, px: (d.ellipse([px(10), px(10), px(118), px(118)], fill=255), d.ellipse([px(26), px(26), px(102), px(102)], fill=0)), STEEL)
    for box in ([60, 2, 68, 36], [60, 92, 68, 126], [2, 60, 36, 68], [92, 60, 126, 68]):
        a.shape(lambda d, px, box=box: d.rectangle([px(box[0]), px(box[1]), px(box[2]), px(box[3])], fill=255), DARK_STEEL)
    a.shape(lambda d, px: d.ellipse([px(46), px(46), px(82), px(82)], fill=255), BRASS)
    a.shape(lambda d, px: d.ellipse([px(58), px(58), px(70), px(70)], fill=255), DARK_STEEL)


STATS = {"hp": stat_hp, "armor": stat_armor, "engine": stat_engine, "tracks": stat_tracks, "reload": stat_reload,
         "damage": stat_damage, "ammo": stat_ammo, "auto_fire": stat_auto_fire}

# (light, dark, accent) per upgrade: a colour family each, away from the weapon colours.
STAT_PALETTES = {
    "hp": ((235, 70, 90), (120, 20, 40), (255, 200, 205)),
    "armor": ((150, 185, 225), (60, 82, 125), (250, 215, 90)),
    "engine": ((205, 205, 212), (96, 100, 112), (245, 140, 40)),
    "tracks": ((150, 155, 140), (74, 82, 66), (212, 184, 90)),
    "reload": ((84, 200, 120), (30, 100, 70), (240, 235, 130)),
    "damage": ((250, 150, 40), (150, 50, 20), (255, 232, 80)),
    "ammo": ((200, 205, 215), (86, 90, 104), (235, 190, 70)),
    "auto_fire": ((90, 215, 235), (30, 90, 120), (250, 90, 70)),
}


WEAPONS = {"cannon": cannon, "heavy_cannon": heavy_cannon, "machine_gun": machine_gun, "shotgun": shotgun,
           "flak": flak, "ricochet": ricochet, "mine": mine}


# (barrel, body, accent) per weapon: each icon gets its own colour family so they read apart at a glance.
PALETTES = {
    "cannon": ((150, 168, 196), (62, 78, 112), (240, 200, 80)),
    "heavy_cannon": ((120, 124, 132), (60, 40, 36), (240, 120, 40)),
    "machine_gun": ((176, 168, 120), (70, 84, 52), (230, 190, 60)),
    "shotgun": ((170, 172, 178), (140, 80, 42), (220, 56, 48)),
    "flak": ((120, 190, 170), (36, 92, 84), (90, 230, 200)),
    "ricochet": ((190, 150, 230), (84, 56, 128), (250, 220, 90)),
    "mine": ((170, 70, 66), (80, 36, 40), (255, 230, 90)),
}


def apply_palette(name):
    global STEEL, DARK_STEEL, BRASS, WOOD
    STEEL, DARK_STEEL, BRASS = PALETTES.get(name) or STAT_PALETTES[name]
    WOOD = DARK_STEEL


def pixelate(image):
    """SD look: shrink with nearest neighbour after a light blur, then reduce the palette."""
    small = image.filter(ImageFilter.GaussianBlur(0.8)).resize((SD_SIZE, SD_SIZE), Image.NEAREST)
    alpha = small.getchannel("A").point(lambda v: 255 if v > 110 else 0)
    quant = small.convert("RGB").quantize(colors=SD_COLORS, dither=Image.NONE).convert("RGB")
    quant.putalpha(alpha)
    return quant


def main():
    preview = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None
    jobs = [("weapon_" + name, name, drawer, TILT if name != "mine" else 0) for name, drawer in WEAPONS.items()]
    jobs += [("stat_" + name, name, drawer, 0) for name, drawer in STATS.items()]
    sheet = Image.new("RGBA", (len(jobs) * 140, 140 + 80), (14, 30, 56, 255))
    for index, (file_name, name, drawer, tilt) in enumerate(jobs):
        apply_palette(name)
        art = Art()
        drawer(art)
        full = art.result(tilt)
        hd = full.resize((HD_SIZE, HD_SIZE), Image.LANCZOS)
        sd = pixelate(full)
        if preview:
            sheet.alpha_composite(full, (index * 140 + 6, 6))
            sheet.alpha_composite(hd, (index * 140 + 6, 140))
            sheet.alpha_composite(sd.resize((SD_SIZE * 2, SD_SIZE * 2), Image.NEAREST), (index * 140 + 76, 150))
            continue
        for folder, picture in (("art_fullhd", full), ("art_hd", hd), ("art", sd)):
            target = os.path.join(ROOT, folder, "abilities")
            os.makedirs(target, exist_ok=True)
            picture.save(os.path.join(target, file_name + ".png"))
    if preview:
        sheet.convert("RGB").save(preview)


if __name__ == "__main__":
    main()
