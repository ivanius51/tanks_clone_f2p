"""Visual mock-up: current tiles vs. new autotiled CC0 walls with grading, shadows and ambient occlusion on a level-1 style map.

Usage: python tools/tileset/mockup.py   (writes tools/tileset/out/mockup_*.png)
"""
import os
import sys

from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter

sys.path.insert(0, os.path.dirname(__file__))
import gen_tileset as gt  # noqa: E402

BASE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(BASE, "tools", "tileset", "out")
T = 64

LEVEL = [
    ".......BB.......",
    "..B..........B..",
    "..B..........B..",
    "..B..WWWWWW..B..",
    "..B..........B..",
    "SS...BBBBBB...SS",
    "....BB....BB....",
    "SS..BB.CC.BB..SS",
    "....BBBBBBBB....",
    "..B..........B..",
    "....B.......B...",
]
GROUP = {"B": "brick", "S": "stone", "W": "wood"}


def atlas_tile(atlas, x, y):
    return atlas.crop((x, y, x + T, y + T))


def graded(img, brightness, saturation, warmth):
    img = ImageEnhance.Brightness(img).enhance(brightness)
    img = ImageEnhance.Color(img).enhance(saturation)
    tint = Image.new("RGBA", img.size, warmth + (255,))
    return ImageChops.multiply(img, tint)


def make_materials(cfg, look):
    mats = {}
    for name, m in cfg["materials"].items():
        m = dict(m)
        top = gt.load(os.path.join(BASE, m["top"]), T * 2)
        side = gt.load(os.path.join(BASE, m["side"]), T)
        top = graded(top, look["brightness"], look["saturation"], look["warmth"])
        side = graded(side, look["brightness"], look["saturation"], look["warmth"])
        m["top_img"], m["side_img"] = top, side
        m["tint"] = [255, 255, 255]
        mats[name] = m
    wood = dict(cfg["materials"]["stone"])
    wood["top_img"] = graded(gt.load(os.path.join(BASE, "assets_src/west_textures/planks.png"), T * 2), look["brightness"], look["saturation"], look["warmth"])
    wood["side_img"] = graded(gt.load(os.path.join(BASE, "assets_src/west_textures/wood wall 1.png"), T), look["brightness"], look["saturation"], look["warmth"])
    wood["tint"], wood["round"] = [255, 255, 255], 3
    mats["wood"] = wood
    return mats


def ground_canvas(atlas, cols, rows, extra):
    grass = [atlas_tile(atlas, 2, 2), atlas_tile(atlas, 70, 2), atlas_tile(atlas, 138, 2), atlas_tile(atlas, 2, 70)]
    img = Image.new("RGBA", (cols * T, rows * T + extra), (0, 0, 0, 255))
    for y in range(rows + 1):
        for x in range(cols):
            img.paste(grass[(x * 7 + y * 3) % 4], (x * T, y * T))
    return img


def neighbours(x, y, ch):
    def same(dx, dy):
        nx, ny = x + dx, y + dy
        return 0 <= ny < len(LEVEL) and 0 <= nx < len(LEVEL[0]) and LEVEL[ny][nx] == ch
    return (gt.UP if same(0, -1) else 0) | (gt.RIGHT if same(1, 0) else 0) | (gt.DOWN if same(0, 1) else 0) | (gt.LEFT if same(-1, 0) else 0)


def render_new(cfg, atlas, look, hd):
    rows, cols = len(LEVEL), len(LEVEL[0])
    side_h = look["front"] if hd else 0
    mats = make_materials(cfg, look)
    img = ground_canvas(atlas, cols, rows, side_h)
    tiles = Image.new("RGBA", img.size, (0, 0, 0, 0))
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                tile = gt.bake_tile(mats[GROUP[ch]], neighbours(x, y, ch), (x + y) % 2, T, side_h, cfg["seed"], hd)
                tiles.alpha_composite(tile, (x * T, y * T))
            elif ch == "C":
                tiles.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
    shape = tiles.getchannel("A")
    dx, dy = look["shadow_offset"]
    shadow = Image.new("L", img.size, 0)
    shadow.paste(shape, (dx, dy))
    shadow = shadow.filter(ImageFilter.GaussianBlur(look["shadow_blur"]))
    shadow = shadow.point(lambda v: int(v * look["shadow_strength"]))
    occlusion = shape.filter(ImageFilter.GaussianBlur(look["ao_blur"])).point(lambda v: int(v * look["ao_strength"]))
    dark = Image.new("RGBA", img.size, (0, 0, 0, 255))
    img = Image.composite(dark, img, ImageChops.lighter(shadow, occlusion))
    img.alpha_composite(tiles)
    if look.get("vignette"):
        vig = Image.radial_gradient("L").resize(img.size).point(lambda v: int(v * look["vignette"]))
        img = Image.composite(Image.new("RGBA", img.size, (0, 0, 0, 255)), img, vig)
    if look.get("pixel"):
        small = img.resize((img.width // look["pixel"], img.height // look["pixel"]), Image.BILINEAR)
        img = small.resize(img.size, Image.NEAREST)
    return img.convert("RGB")


def render_now(atlas):
    rows, cols = len(LEVEL), len(LEVEL[0])
    img = ground_canvas(atlas, cols, rows, 0)
    frames = {"B": atlas_tile(atlas, 138, 70), "S": atlas_tile(atlas, 2, 138), "W": atlas_tile(atlas, 70, 138), "C": atlas_tile(atlas, 70, 70)}
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in frames:
                img.alpha_composite(frames[ch], (x * T, y * T))
    return img.convert("RGB")


def main():
    import json
    cfg = json.load(open(os.path.join(BASE, "tools", "tileset", "tileset.json")))
    atlas = Image.open(os.path.join(BASE, "Resources", "textures_hd", "tilesets", "field.png")).convert("RGBA")
    os.makedirs(OUT, exist_ok=True)
    sd = {"brightness": 0.62, "saturation": 0.8, "warmth": (255, 244, 236), "front": 0, "shadow_offset": (5, 7), "shadow_blur": 6,
          "shadow_strength": 0.6, "ao_blur": 10, "ao_strength": 0.45, "vignette": 0.25}
    hd = dict(sd, front=24, shadow_offset=(8, 14), shadow_blur=8)
    retro = dict(sd, pixel=2, saturation=0.7)
    render_now(atlas).save(os.path.join(OUT, "mockup_now.png"))
    render_new(cfg, atlas, sd, False).save(os.path.join(OUT, "mockup_sd.png"))
    render_new(cfg, atlas, hd, True).save(os.path.join(OUT, "mockup_hd.png"))
    render_new(cfg, atlas, retro, False).save(os.path.join(OUT, "mockup_retro.png"))


if __name__ == "__main__":
    main()
