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


V2_MATERIALS = {
    "brick": ("stone wall 11.png", 4, (1.0, 1.15, (255, 196, 150))),
    "stone": ("stone wall 10.png", 4, (0.85, 0.9, (240, 240, 250))),
    "wood": ("wood floor 2.png", 3, (0.95, 1.2, (255, 200, 150))),
}


def bake_v2(top_src, rnd_round, mask, variant, lip):
    mat = {"top_img": top_src, "side_img": top_src, "tint": [255, 255, 255], "round": rnd_round}
    top = gt.bake_tile(mat, mask, variant, T, 0, 7, False)
    out = Image.new("RGBA", (T, T + lip), (0, 0, 0, 0))
    if lip and not mask & gt.DOWN:
        band = top.crop((0, T - lip // 2 - 2, T, T - 2)).resize((T, lip), Image.BILINEAR)
        grad = Image.linear_gradient("L").resize((T, lip)).point(lambda v: int(255 - v * 0.45))
        dark = Image.new("RGBA", band.size, (0, 0, 0, 255))
        band = Image.composite(band, dark, grad.point(lambda v: int(v * 0.75))).convert("RGBA")
        band.putalpha(255)
        ImageDraw.Draw(band, "RGBA").rectangle((0, lip - 2, T, lip), fill=(0, 0, 0, 140))
        out.paste(band, (0, T), band)
    out.alpha_composite(top)
    return out


def render_v2(atlas, lip, light=(1, 1), length=10):
    rows, cols = len(LEVEL), len(LEVEL[0])
    srcs = {}
    for name, (file, rnd, (bright, sat, warm)) in V2_MATERIALS.items():
        srcs[name] = (graded(gt.load(os.path.join(BASE, "assets_src/west_textures", file), T * 2), bright * 0.8, sat, warm), rnd)
    img = ground_canvas(atlas, cols, rows, lip)
    tiles = Image.new("RGBA", img.size, (0, 0, 0, 0))
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                src, rnd = srcs[GROUP[ch]]
                tiles.alpha_composite(bake_v2(src, rnd, neighbours(x, y, ch), (x + y) % 2, lip), (x * T, y * T))
            elif ch == "C":
                tiles.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
    shape = tiles.getchannel("A")
    cast = Image.new("L", img.size, 0)
    for k in range(1, length + 1):
        step = Image.new("L", img.size, 0)
        step.paste(shape, (light[0] * k, light[1] * k))
        cast = ImageChops.lighter(cast, step.point(lambda v, k=k: int(v * (1 - 0.3 * k / length))))
    cast = cast.filter(ImageFilter.GaussianBlur(1.5)).point(lambda v: int(v * 0.75))
    occlusion = shape.filter(ImageFilter.GaussianBlur(5)).point(lambda v: int(v * 0.35))
    img = Image.composite(Image.new("RGBA", img.size, (0, 0, 0, 255)), img, ImageChops.lighter(cast, occlusion))
    img.alpha_composite(tiles)
    return img.convert("RGB")


def top_v3(src, x, y, mask, radius):
    ox, oy = (x * T) % (T * 2), (y * T) % (T * 2)
    top = src.crop((ox, oy, ox + T, oy + T))
    if not mask & gt.DOWN:
        ImageDraw.Draw(top, "RGBA").rectangle((0, T - 2, T, T), fill=(255, 235, 210, 40))
    top.putalpha(gt.cell_mask(T, mask, radius))
    return top


def front_v3(top, lip):
    face = top.transpose(Image.FLIP_TOP_BOTTOM).resize((T, lip), Image.LANCZOS)
    face = gt.shade(face, 0.55)
    grad = Image.linear_gradient("L").resize((T, lip)).point(lambda v: int(v * 0.45))
    face = Image.composite(Image.new("RGBA", face.size, (0, 0, 0, 255)), face, grad)
    face.putalpha(top.getchannel("A").transpose(Image.FLIP_TOP_BOTTOM).resize((T, lip)))
    return face


def render_v3(atlas, lip, shadow_len, shadow_dx):
    rows, cols = len(LEVEL), len(LEVEL[0])
    srcs = {}
    for name, (file, rnd, (bright, sat, warm)) in V2_MATERIALS.items():
        raw = Image.open(os.path.join(BASE, "assets_src/west_textures", file)).convert("RGBA")
        if name == "brick":
            raw = raw.crop((0, 0, raw.width, raw.height * 3 // 4))
        srcs[name] = (graded(raw.resize((T * 2, T * 2), Image.LANCZOS), bright * 0.8, sat, warm), rnd)
    img = ground_canvas(atlas, cols, rows, lip)
    tops = Image.new("RGBA", img.size, (0, 0, 0, 0))
    fronts = Image.new("RGBA", img.size, (0, 0, 0, 0))
    base = Image.new("L", img.size, 0)
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                src, rnd = srcs[GROUP[ch]]
                mask = neighbours(x, y, ch)
                top = top_v3(src, x, y, mask, rnd)
                tops.alpha_composite(top, (x * T, y * T))
                if not mask & gt.DOWN:
                    face = front_v3(top, lip) if lip else None
                    if face:
                        fronts.alpha_composite(face, (x * T, y * T + T))
                    base.paste(255, (x * T, y * T + T + lip - 2, x * T + T, y * T + T + lip))
                base.paste(top.getchannel("A"), (x * T, y * T + lip), top.getchannel("A"))
            elif ch == "C":
                tops.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
                base.paste(255, (x * T, y * T, x * T + T, y * T + T))
    cast = Image.new("L", img.size, 0)
    for k in range(shadow_len + 1):
        step = Image.new("L", img.size, 0)
        step.paste(base, (shadow_dx * k // max(shadow_len, 1), k))
        cast = ImageChops.lighter(cast, step)
    cast = cast.filter(ImageFilter.GaussianBlur(2.5)).point(lambda v: int(v * 0.55))
    ao = Image.new("L", img.size, 0)
    for k in range(4):
        step = Image.new("L", img.size, 0)
        step.paste(base, (0, k))
        ao = ImageChops.lighter(ao, step.point(lambda v, k=k: int(v * (1 - k / 4))))
    img = Image.composite(Image.new("RGBA", img.size, (0, 0, 0, 255)), img, ImageChops.lighter(cast, ao.point(lambda v: int(v * 0.8))))
    img.alpha_composite(fronts)
    img.alpha_composite(tops)
    return img.convert("RGB"), srcs


def export_tiles(srcs, lip):
    root = os.path.join(OUT, "rework_tiles")
    os.makedirs(root, exist_ok=True)
    for name, (src, rnd) in srcs.items():
        src.save(os.path.join(root, f"{name}_texture_128.png"))
        top = top_v3(src, 0, 0, 0, rnd)
        top.save(os.path.join(root, f"{name}_top_single.png"))
        front_v3(top, lip).save(os.path.join(root, f"{name}_front_single.png"))


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
    render_v2(atlas, 24, (2, 1), 14).save(os.path.join(OUT, "mockup_v2_lip.png"))
    render_v2(atlas, 0, (2, 1), 10).save(os.path.join(OUT, "mockup_v2_sd.png"))
    img, srcs = render_v3(atlas, T // 2, T // 2, 10)
    img.save(os.path.join(OUT, "mockup_v3.png"))
    export_tiles(srcs, T // 2)


if __name__ == "__main__":
    main()


def cell_v4(src, x, y, mask, lip):
    """Whole wall cell inside 64x64: roof on top, mirrored squashed front face at the bottom when nothing is below."""
    ox, oy = (x * T) % (T * 2), (y * T) % (T * 2)
    roof = src.crop((ox, oy, ox + T, oy + T))
    cell = roof.copy()
    if not mask & gt.DOWN:
        face = roof.crop((0, 0, T, T - lip)).transpose(Image.FLIP_TOP_BOTTOM).resize((T, lip), Image.LANCZOS)
        face = gt.shade(face, 0.55)
        dark = Image.new("RGBA", face.size, (0, 0, 0, 255))
        face = Image.composite(dark, face, Image.linear_gradient("L").resize((T, lip)).point(lambda v: int(v * 0.4)))
        cell.paste(face, (0, T - lip))
        line = Image.new("L", (T, 1), 0)
        for i in range(T):
            edge = min(i, T - 1 - i) / 8
            line.putpixel((i, 0), int(46 * min(1.0, edge)))
        cell.paste(Image.composite(Image.new("RGBA", (T, 1), (255, 240, 220, 255)), cell.crop((0, T - lip - 1, T, T - lip)), line), (0, T - lip - 1))
    vig = Image.new("L", (T, T), 0)
    d = ImageDraw.Draw(vig)
    for i in range(4):
        a = int(40 * (1 - i / 4))
        if not mask & gt.LEFT:
            d.line((i, 0, i, T), fill=a)
        if not mask & gt.RIGHT:
            d.line((T - 1 - i, 0, T - 1 - i, T), fill=a)
    cell = Image.composite(Image.new("RGBA", cell.size, (0, 0, 0, 255)), cell, vig)
    cell.putalpha(gt.cell_mask(T, mask, 3))
    return cell


def render_v4(atlas, lip, fade=28, dx=6):
    rows, cols = len(LEVEL), len(LEVEL[0])
    srcs = {}
    for name, (file, rnd, (bright, sat, warm)) in V2_MATERIALS.items():
        raw = Image.open(os.path.join(BASE, "assets_src/west_textures", file)).convert("RGBA")
        if name == "brick":
            raw = raw.crop((0, 0, raw.width, raw.height * 3 // 4))
        srcs[name] = graded(raw.resize((T * 2, T * 2), Image.LANCZOS), bright * 0.8, sat, warm)
    img = ground_canvas(atlas, cols, rows, 0)
    walls = Image.new("RGBA", img.size, (0, 0, 0, 0))
    solid = Image.new("L", img.size, 0)
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                cell = cell_v4(srcs[GROUP[ch]], x, y, neighbours(x, y, ch), lip)
                walls.alpha_composite(cell, (x * T, y * T))
                solid.paste(cell.getchannel("A"), (x * T, y * T), cell.getchannel("A"))
            elif ch == "C":
                walls.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
                solid.paste(255, (x * T, y * T, x * T + T, y * T + T))
    cast = Image.new("L", img.size, 0)
    for k in range(1, fade + 1):
        step = Image.new("L", img.size, 0)
        step.paste(solid, (dx * k // fade, k))
        cast = ImageChops.lighter(cast, step.point(lambda v, k=k: int(v * 0.55 * (1 - k / fade))))
    crease = Image.new("L", img.size, 0)
    crease.paste(solid, (0, 2))
    crease = ImageChops.subtract(crease, solid).point(lambda v: int(v * 0.85))
    ao = solid.filter(ImageFilter.GaussianBlur(4)).point(lambda v: int(v * 0.3))
    shade_l = ImageChops.lighter(ImageChops.lighter(cast.filter(ImageFilter.GaussianBlur(1.5)), crease), ao)
    img = Image.composite(Image.new("RGBA", img.size, (0, 0, 0, 255)), img, shade_l)
    img.alpha_composite(walls)
    return img.convert("RGB"), srcs


def export_v4(srcs, lip):
    root = os.path.join(OUT, "rework_tiles")
    os.makedirs(root, exist_ok=True)
    for name, src in srcs.items():
        src.save(os.path.join(root, f"{name}_texture_128.png"))
        sheet = Image.new("RGBA", (T * 16, T), (0, 0, 0, 0))
        for m in range(16):
            sheet.paste(cell_v4(src, m, 0, m, lip), (m * T, 0))
        sheet.save(os.path.join(root, f"{name}_autotile_16.png"))



def rim(cell, side, alpha, width):
    """Soft sun-side light: a strip fading inward from one edge."""
    m = Image.new("L", cell.size, 0)
    d = ImageDraw.Draw(m)
    w, h = cell.size
    for i in range(width):
        a = int(255 * alpha * (1 - i / width))
        if side == "top":
            d.line((0, i, w, i), fill=a)
        elif side == "left":
            d.line((i, 0, i, h), fill=a)
        else:
            d.line((0, side - i, w, side - i), fill=a)
    return Image.composite(Image.new("RGBA", cell.size, (255, 236, 210, 255)), cell, m)


def cell_v5(roof_src, front_src, x, y, mask, lip):
    ox, oy = (x * T) % (T * 2), (y * T) % (T * 2)
    cell = roof_src.crop((ox, oy, ox + T, oy + T))
    roof_h = T if mask & gt.DOWN else T - lip
    if not mask & gt.UP:
        cell = rim(cell, "top", 0.4, 3)
    if not mask & gt.LEFT:
        cell = rim(cell, "left", 0.35, 3)
    if roof_h < T:
        cell = rim(cell, roof_h - 1, 0.12, 2)
        face = front_src.crop((ox, 0, ox + T, lip))
        if not mask & gt.LEFT:
            face = rim(face, "left", 0.12, 2)
        cell.paste(face, (0, roof_h))
    cell.putalpha(gt.cell_mask(T, mask, 3))
    return cell


def player_tank():
    return Image.open(os.path.join(BASE, "Resources", "textures_hd", "player.png")).convert("RGBA").crop((2, 2, 50, 60))


def render_v5(atlas, lip=24, fade=28, dx=6):
    rows, cols = len(LEVEL), len(LEVEL[0])
    srcs = {}
    for name, (file, rnd, (bright, sat, warm)) in V2_MATERIALS.items():
        raw = Image.open(os.path.join(BASE, "assets_src/west_textures", file)).convert("RGBA")
        roof = graded(raw.resize((T * 2, T * 2), Image.LANCZOS), bright * 0.8, sat, warm)
        front = gt.shade(roof.transpose(Image.FLIP_TOP_BOTTOM).resize((T * 2, lip), Image.LANCZOS), 0.55)
        srcs[name] = (roof, front)
    user = os.path.join(BASE, "assets_src", "user_textures")
    top = Image.open(os.path.join(user, "brick_top.png")).convert("RGBA").resize((T, T), Image.LANCZOS)
    both = Image.open(os.path.join(user, "brick_top_side.png")).convert("RGBA")
    side = both.crop((0, both.height * 530 // 1024, both.width, both.height)).resize((T, lip), Image.LANCZOS)
    roof2, side2 = Image.new("RGBA", (T * 2, T * 2)), Image.new("RGBA", (T * 2, lip))
    for i in range(2):
        side2.paste(side, (i * T, 0))
        for j in range(2):
            roof2.paste(top, (i * T, j * T))
    srcs["brick"] = (roof2, side2)
    img = ground_canvas(atlas, cols, rows, 0)
    walls = Image.new("RGBA", img.size, (0, 0, 0, 0))
    solid = Image.new("L", img.size, 0)
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                roof, front = srcs[GROUP[ch]]
                cell = cell_v5(roof, front, x, y, neighbours(x, y, ch), lip)
                walls.alpha_composite(cell, (x * T, y * T))
                solid.paste(cell.getchannel("A"), (x * T, y * T), cell.getchannel("A"))
            elif ch == "C":
                walls.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
                solid.paste(255, (x * T, y * T, x * T + T, y * T + T))
    black = Image.new("RGBA", img.size, (0, 0, 0, 255))
    crease = Image.new("L", img.size, 0)
    crease.paste(solid, (0, 2))
    crease = ImageChops.subtract(crease, solid).point(lambda v: int(v * 0.85))
    ao = solid.filter(ImageFilter.GaussianBlur(4)).point(lambda v: int(v * 0.3))
    img = Image.composite(black, img, ImageChops.lighter(crease, ao))
    tank = player_tank()
    for tx, ty in ((7 * T + 8, 9 * T + 2), (9 * T + 8, 4 * T + 2), (14 * T + 8, 7 * T + 4)):
        img.alpha_composite(tank, (tx, ty))
    cast = Image.new("L", img.size, 0)
    for k in range(1, fade + 1):
        step = Image.new("L", img.size, 0)
        step.paste(solid, (dx * k // fade, k))
        cast = ImageChops.lighter(cast, step.point(lambda v, k=k: int(v * 0.55 * (1 - k / fade))))
    img = Image.composite(black, img, cast.filter(ImageFilter.GaussianBlur(1.5)))
    img.alpha_composite(walls)
    return img.convert("RGB"), srcs


USER_SPLIT = {"brick": 530, "stone": 515, "wood": 510}


def user_material(name, lip, scale):
    """Roof and side strips from the user's top / top+side textures, one texture covering scale x scale tiles."""
    user = os.path.join(BASE, "assets_src", "user_textures")
    size = int(T * scale)
    roof = Image.open(os.path.join(user, f"{name}_top.png")).convert("RGBA").resize((size, size), Image.LANCZOS)
    both = Image.open(os.path.join(user, f"{name}_top_side.png")).convert("RGBA")
    split = both.height * USER_SPLIT[name] // 1024
    side_full = both.crop((0, split, both.width, both.height)).resize((size, size * (both.height - split) // both.height), Image.LANCZOS)
    side = side_full.crop((0, 0, size, lip))
    roof2, side2 = Image.new("RGBA", (size * 2, size * 2)), Image.new("RGBA", (size * 2, lip))
    for a in range(2):
        side2.paste(side, (a * size, 0))
        for b in range(2):
            roof2.paste(roof, (a * size, b * size))
    return roof2, side2, size


def cell_user(roof_src, side_src, size, x, y, mask, lip):
    ox, oy = (x * T) % size, (y * T) % size
    cell = roof_src.crop((ox, oy, ox + T, oy + T))
    roof_h = T if mask & gt.DOWN else T - lip
    if not mask & gt.UP:
        cell = rim(cell, "top", 0.4, 3)
    if not mask & gt.LEFT:
        cell = rim(cell, "left", 0.35, 3)
    if roof_h < T:
        cell = rim(cell, roof_h - 1, 0.12, 2)
        face = side_src.crop((ox, 0, ox + T, lip))
        if not mask & gt.LEFT:
            face = rim(face, "left", 0.12, 2)
        cell.paste(face, (0, roof_h))
    cell.putalpha(gt.cell_mask(T, mask, 3))
    return cell


def render_user(atlas, scales, lip=32, fade=28, dx=6):
    rows, cols = len(LEVEL), len(LEVEL[0])
    mats = {n: user_material(n, lip, scales[n]) for n in USER_SPLIT}
    img = ground_canvas(atlas, cols, rows, 0)
    walls = Image.new("RGBA", img.size, (0, 0, 0, 0))
    solid = Image.new("L", img.size, 0)
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                roof, side, size = mats[GROUP[ch]]
                cell = cell_user(roof, side, size, x, y, neighbours(x, y, ch), lip)
                walls.alpha_composite(cell, (x * T, y * T))
                solid.paste(cell.getchannel("A"), (x * T, y * T), cell.getchannel("A"))
            elif ch == "C":
                walls.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
                solid.paste(255, (x * T, y * T, x * T + T, y * T + T))
    black = Image.new("RGBA", img.size, (0, 0, 0, 255))
    crease = Image.new("L", img.size, 0)
    crease.paste(solid, (0, 2))
    crease = ImageChops.subtract(crease, solid).point(lambda v: int(v * 0.85))
    ao = solid.filter(ImageFilter.GaussianBlur(4)).point(lambda v: int(v * 0.3))
    img = Image.composite(black, img, ImageChops.lighter(crease, ao))
    tank = player_tank()
    for tx, ty in ((7 * T + 8, 9 * T + 2), (9 * T + 8, 4 * T + 2), (14 * T + 8, 7 * T + 4)):
        img.alpha_composite(tank, (tx, ty))
    cast = Image.new("L", img.size, 0)
    for k in range(1, fade + 1):
        step = Image.new("L", img.size, 0)
        step.paste(solid, (dx * k // fade, k))
        cast = ImageChops.lighter(cast, step.point(lambda v, k=k: int(v * 0.55 * (1 - k / fade))))
    img = Image.composite(black, img, cast.filter(ImageFilter.GaussianBlur(1.5)))
    img.alpha_composite(walls)
    return img.convert("RGB")


def side_mask(highlight=1.15, crease=(0.55, 0.4, 0.48), start=0.58, end=0.32, roof=0.5):
    """Row multipliers found by comparing the user's top and top+side textures: same pixels, bottom half darkened."""
    half = int(T * roof)
    rows = [1.0] * T
    rows[half - 2] = rows[half - 1] = highlight
    for k, v in enumerate(crease):
        rows[half + k] = v
    rest = T - half - len(crease)
    for k in range(rest):
        rows[half + len(crease) + k] = start + (end - start) * k / max(rest - 1, 1)
    for k in range(5):
        rows[T - 5 + k] = 0.20 - 0.01 * k
    return rows


def apply_rows(cell, rows):
    out = cell.copy()
    px = out.load()
    for y, f in enumerate(rows):
        for x in range(out.width):
            r, g, b, a = px[x, y]
            px[x, y] = (min(255, int(r * f)), min(255, int(g * f)), min(255, int(b * f)), a)
    return out


def render_mask_rule(atlas, frames, roof=0.5, length=32, dx=16, custom=None):
    rows_n, cols = len(LEVEL), len(LEVEL[0])
    rows = side_mask(roof=roof)
    img = ground_canvas(atlas, cols, rows_n, 0)
    walls = Image.new("RGBA", img.size, (0, 0, 0, 0))
    solid = Image.new("L", img.size, 0)
    for y, row in enumerate(LEVEL):
        for x, ch in enumerate(row):
            if ch in GROUP:
                mask = neighbours(x, y, ch)
                cell = custom[ch].copy() if custom and ch in custom else atlas_tile(atlas, *frames[ch]).copy()
                if not mask & gt.DOWN:
                    cell = apply_rows(cell, rows)
                if not mask & gt.UP:
                    cell = rim(cell, "top", 0.3, 3)
                if not mask & gt.LEFT:
                    cell = rim(cell, "left", 0.25, 3)
                cell.putalpha(255)
                walls.alpha_composite(cell, (x * T, y * T))
                solid.paste(255, (x * T, y * T, x * T + T, y * T + T))
            elif ch == "C":
                walls.alpha_composite(atlas_tile(atlas, 70, 70), (x * T, y * T))
                solid.paste(255, (x * T, y * T, x * T + T, y * T + T))
    black = Image.new("RGBA", img.size, (0, 0, 0, 255))
    crease = Image.new("L", img.size, 0)
    crease.paste(solid, (0, 2))
    crease = ImageChops.subtract(crease, solid).point(lambda v: int(v * 0.85))
    ao = solid.filter(ImageFilter.GaussianBlur(4)).point(lambda v: int(v * 0.3))
    img = Image.composite(black, img, ImageChops.lighter(crease, ao))
    tank = player_tank()
    for tx, ty in ((7 * T + 8, 9 * T + 2), (9 * T + 8, 4 * T + 2), (14 * T + 8, 7 * T + 4)):
        img.alpha_composite(tank, (tx, ty))
    cast = Image.new("L", img.size, 0)
    for k in range(1, length + 1):
        step = Image.new("L", img.size, 0)
        step.paste(solid, (dx * k // length, k))
        cast = ImageChops.lighter(cast, step)
    cast = ImageChops.subtract(cast, solid).filter(ImageFilter.GaussianBlur(2.5)).point(lambda v: int(v * 0.5))
    img = Image.composite(black, img, cast)
    img.alpha_composite(walls)
    mask_img = Image.new("L", (T, T))
    for y, f in enumerate(rows):
        mask_img.paste(min(255, int(f * 200)), (0, y, T, y + 1))
    return img.convert("RGB"), mask_img


def divisible_tiles(atlas):
    """Tiles whose seams fall on the half line: metal as 2x2 plates, wood as two wide planks split in the middle."""
    plate = atlas_tile(atlas, 2, 138).resize((T // 2, T // 2), Image.LANCZOS)
    metal = Image.new("RGBA", (T, T))
    for a in range(2):
        for b in range(2):
            metal.paste(plate, (a * T // 2, b * T // 2))
    turned = atlas_tile(atlas, 70, 138).transpose(Image.ROTATE_90)
    strip = turned.crop((36, 0, T, T))
    wood = Image.new("RGBA", (T, T))
    for x0 in range(0, T, strip.width):
        wood.paste(strip, (x0, 0))
    d = ImageDraw.Draw(wood, "RGBA")
    for k in range(1, 4):
        d.line((0, k * T // 4 - 1, T, k * T // 4 - 1), fill=(30, 18, 10, 200))
    for cx in (0, T - 6):
        d.rectangle((cx, 0, cx + 5, T), fill=(70, 72, 78, 255))
        d.line((cx, 0, cx, T), fill=(130, 132, 140, 255))
        d.line((cx + 5, 0, cx + 5, T), fill=(30, 30, 34, 255))
        for k in range(4):
            d.ellipse((cx + 1, k * 16 + 6, cx + 4, k * 16 + 9), fill=(170, 170, 175, 255))
    return metal, wood


def extrude(sprite, height, dark=0.45):
    """Fake side for any sprite: darker copies stepped downward under the top, so every direction shows a side."""
    w, h = sprite.size
    out = Image.new("RGBA", (w, h + height), (0, 0, 0, 0))
    side = gt.shade(sprite, dark)
    for k in range(height, 0, -1):
        out.alpha_composite(gt.shade(side, 1 - 0.4 * k / height), (0, k))
    out.alpha_composite(rim(sprite, "top", 0.25, 2), (0, 0))
    return out


def tank_sheet():
    tank = player_tank()
    sheet = Image.new("RGBA", (4 * 80, 2 * 90), (52, 70, 40, 255))
    d = ImageDraw.Draw(sheet)
    for i, angle in enumerate((0, 270, 180, 90)):
        flat = tank.rotate(angle, expand=True)
        sheet.alpha_composite(flat, (i * 80 + 10, 10))
        tilted = extrude(flat.resize((flat.width, int(flat.height * 0.8)), Image.LANCZOS), 8)
        sheet.alpha_composite(tilted, (i * 80 + 10, 95))
    d.text((4, 0), "now: rotation only", fill=(255, 255, 255))
    d.text((4, 85), "tilted: squash 80% + side 8px", fill=(255, 255, 255))
    return sheet


def scene_with_tanks(base_img, solid_cells):
    """Level with tilted tanks in four directions, each with its own cast shadow under the wall shadow layer."""
    img = base_img.convert("RGBA")
    tank = player_tank()
    for (tx, ty), angle in (((7 * T + 8, 9 * T + 4), 0), ((9 * T, 4 * T + 8), 270), ((2 * T + 4, 8 * T + 8), 90), ((14 * T + 8, 2 * T), 180)):
        flat = tank.rotate(angle, expand=True)
        tilted = extrude(flat.resize((flat.width, int(flat.height * 0.8)), Image.LANCZOS), 8)
        sh = tilted.getchannel("A").point(lambda v: int(v * 0.45)).filter(ImageFilter.GaussianBlur(2))
        img.paste((0, 0, 0, 255), (tx + 8, ty + 10), sh)
        img.alpha_composite(tilted, (tx, ty))
    return img.convert("RGB")

if __name__ == "__main__":
    a = Image.open(os.path.join(BASE, "Resources", "textures_hd", "tilesets", "field.png")).convert("RGBA")
    out, s = render_v5(a, lip=32)
    out.save(os.path.join(OUT, "mockup_v5.png"))
    render_user(a, {"brick": 1.5, "stone": 1, "wood": 1}).save(os.path.join(OUT, "mockup_user.png"))
    rule, m = render_mask_rule(a, {"B": (138, 70), "S": (2, 138), "W": (70, 138)})
    rule.save(os.path.join(OUT, "mockup_mask_rule.png"))
    third, m3 = render_mask_rule(a, {"B": (138, 70), "S": (2, 138), "W": (70, 138)}, roof=2 / 3, length=24, dx=12)
    third.save(os.path.join(OUT, "mockup_mask_third.png"))
    m3.save(os.path.join(OUT, "side_mask_third_64.png"))
    metal, wood = divisible_tiles(a)
    frames = {"B": (138, 70), "S": (2, 138), "W": (70, 138)}
    render_mask_rule(a, frames, custom={"S": metal, "W": wood})[0].save(os.path.join(OUT, "mockup_divisible_half.png"))
    render_mask_rule(a, frames, roof=2 / 3, length=24, dx=12, custom={"S": metal, "W": wood})[0].save(os.path.join(OUT, "mockup_divisible_third.png"))
    metal.save(os.path.join(OUT, "rework_tiles", "metal_2x2_64.png"))
    tank_sheet().save(os.path.join(OUT, "tank_directions.png"))
    scene_with_tanks(Image.open(os.path.join(OUT, "mockup_divisible_half.png")), None).save(os.path.join(OUT, "mockup_tanks.png"))
    wood.save(os.path.join(OUT, "rework_tiles", "wood_split_64.png"))
    m.save(os.path.join(OUT, "side_mask_64.png"))
