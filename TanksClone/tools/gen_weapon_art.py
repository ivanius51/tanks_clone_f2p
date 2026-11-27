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

from PIL import Image, ImageDraw, ImageFilter

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
    STEEL, DARK_STEEL, BRASS = PALETTES[name]
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
    sheet = Image.new("RGBA", (len(WEAPONS) * 140, 140 + 80), (14, 30, 56, 255))
    for index, (name, drawer) in enumerate(WEAPONS.items()):
        apply_palette(name)
        art = Art()
        drawer(art)
        full = art.result(0 if name == "mine" else TILT)
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
            picture.save(os.path.join(target, "weapon_%s.png" % name))
    if preview:
        sheet.convert("RGB").save(preview)


if __name__ == "__main__":
    main()
