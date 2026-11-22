"""Bakes autotile sets (16 neighbour masks x variants) from seamless textures.

Mask bits: up=1, right=2, down=4, left=8 (a set bit means a neighbour of the same group is there).
Two looks per material: "sd" (top-down) and "hd" (slightly rotated: a front face strip under open bottom edges).
Usage: python gen_tileset.py [config] [out_dir]
"""
import json
import os
import random
import sys

from PIL import Image, ImageChops, ImageDraw

UP, RIGHT, DOWN, LEFT = 1, 2, 4, 8


def load(path, size):
    return Image.open(path).convert("RGBA").resize((size, size), Image.LANCZOS)


def tint(img, rgb):
    return ImageChops.multiply(img, Image.new("RGBA", img.size, tuple(rgb) + (255,)))


def shade(img, factor):
    r, g, b, a = img.split()
    return Image.merge("RGBA", tuple(c.point(lambda v: int(v * factor)) for c in (r, g, b)) + (a,))


def cell_mask(size, mask, radius):
    """Alpha of one cell: rounded only at corners whose two sides are both open."""
    m = Image.new("L", (size, size), 255)
    big = Image.new("L", (radius * 8, radius * 8), 0)
    ImageDraw.Draw(big).ellipse((0, 0, radius * 8 - 1, radius * 8 - 1), fill=255)
    disc = big.resize((radius * 2, radius * 2), Image.LANCZOS)
    corners = (
        (UP | LEFT, (0, 0), (0, 0), (0, 0)),
        (UP | RIGHT, (size - radius, 0), (radius, 0), (0, 0)),
        (DOWN | RIGHT, (size - radius, size - radius), (radius, radius), (0, 0)),
        (DOWN | LEFT, (0, size - radius), (0, radius), (0, 0)),
    )
    for bits, dest, src, _ in corners:
        if mask & bits:
            continue
        quarter = disc.crop((src[0], src[1], src[0] + radius, src[1] + radius))
        hole = ImageChops.invert(quarter)
        region = m.crop((dest[0], dest[1], dest[0] + radius, dest[1] + radius))
        m.paste(ImageChops.multiply(region, quarter), dest)
        del hole
    return m


def bevel(img, mask, size):
    """Dark outline and light/dark bevel on the sides that are open (no neighbour)."""
    d = ImageDraw.Draw(img, "RGBA")
    w = max(2, size // 16)
    if not mask & UP:
        d.rectangle((0, 0, size, w), fill=(255, 255, 255, 70))
        d.rectangle((0, 0, size, 1), fill=(0, 0, 0, 150))
    if not mask & LEFT:
        d.rectangle((0, 0, w, size), fill=(255, 255, 255, 50))
        d.rectangle((0, 0, 1, size), fill=(0, 0, 0, 150))
    if not mask & DOWN:
        d.rectangle((0, size - w, size, size), fill=(0, 0, 0, 80))
        d.rectangle((0, size - 2, size, size), fill=(0, 0, 0, 160))
    if not mask & RIGHT:
        d.rectangle((size - w, 0, size, size), fill=(0, 0, 0, 60))
        d.rectangle((size - 2, 0, size, size), fill=(0, 0, 0, 150))


def bake_tile(mat, mask, variant, size, side_h, seed, hd):
    top_src, side_src = mat["top_img"], mat["side_img"]
    rnd = random.Random(seed + mask * 31 + variant * 997)
    ox, oy = rnd.randrange(size), rnd.randrange(size)
    top = top_src.crop((ox, oy, ox + size, oy + size))
    top = tint(top, mat["tint"])
    bevel(top, mask, size)
    top.putalpha(ImageChops.multiply(top.getchannel("A"), cell_mask(size, mask, mat["round"])))

    out = Image.new("RGBA", (size, size + (side_h if hd else 0)), (0, 0, 0, 0))
    if hd and not mask & DOWN:
        strip = shade(tint(side_src.crop((0, 0, size, side_h)), mat["tint"]), 0.62)
        d = ImageDraw.Draw(strip, "RGBA")
        d.rectangle((0, 0, size, 1), fill=(0, 0, 0, 120))
        d.rectangle((0, side_h - 2, size, side_h), fill=(0, 0, 0, 170))
        sm = Image.new("L", (size, side_h), 255)
        if not mask & LEFT:
            ImageDraw.Draw(sm).rectangle((0, 0, 1, side_h), fill=0)
        if not mask & RIGHT:
            ImageDraw.Draw(sm).rectangle((size - 2, 0, size, side_h), fill=0)
        strip.putalpha(ImageChops.multiply(strip.getchannel("A"), sm))
        out.paste(strip, (0, size - 6), strip)
    out.alpha_composite(top, (0, 0))
    return out


def bake_set(name, mat, cfg, hd, root):
    size, side_h, variants = cfg["tile_size"], cfg["hd_side_height"], cfg["variants"]
    height = size + (side_h if hd else 0)
    atlas = Image.new("RGBA", (size * 16, height * variants), (0, 0, 0, 0))
    frames = {}
    for v in range(variants):
        for m in range(16):
            atlas.paste(bake_tile(mat, m, v, size, side_h, cfg["seed"], hd), (m * size, v * height))
            frames[f"{m}_{v}"] = [m * size, v * height, size, height]
    style = "hd" if hd else "sd"
    os.makedirs(root, exist_ok=True)
    atlas.save(os.path.join(root, f"{name}_{style}.png"))
    with open(os.path.join(root, f"{name}_{style}.json"), "w") as f:
        json.dump({"material": name, "style": style, "tile": size, "frame_height": height, "variants": variants,
                   "mask_bits": {"up": 1, "right": 2, "down": 4, "left": 8}, "frames": frames}, f, indent=1)


SAMPLE = [
    "..........",
    ".BBBBBB.S.",
    ".B....B.S.",
    ".B.SS.BBB.",
    ".B.SS.....",
    ".BB.BBBB..",
    "....B..B..",
    ".SSSB..B..",
    "..........",
]


def preview(cfg, mats, hd, root):
    size, side_h = cfg["tile_size"], cfg["hd_side_height"]
    rows, cols = len(SAMPLE), len(SAMPLE[0])
    img = Image.new("RGBA", (cols * size, rows * size + side_h), (0, 0, 0, 255))
    ground = load(cfg["preview"]["ground"], size * 2)
    for y in range(0, img.height, size * 2):
        for x in range(0, img.width, size * 2):
            img.paste(ground, (x, y))
    group = {"B": "brick", "S": "stone"}
    for y in range(rows):
        for x, ch in enumerate(SAMPLE[y]):
            if ch not in group:
                continue

            def same(dx, dy):
                nx, ny = x + dx, y + dy
                return 0 <= ny < rows and 0 <= nx < cols and SAMPLE[ny][nx] == ch

            m = (UP if same(0, -1) else 0) | (RIGHT if same(1, 0) else 0) | (DOWN if same(0, 1) else 0) | (LEFT if same(-1, 0) else 0)
            img.alpha_composite(bake_tile(mats[group[ch]], m, (x + y) % cfg["variants"], size, side_h, cfg["seed"], hd), (x * size, y * size))
    path = os.path.join(root, f"preview_{'hd' if hd else 'sd'}.png")
    img.convert("RGB").save(path)
    return path


def sheet(cfg, mats, root):
    """All 16 masks of every material, both looks."""
    size, side_h = cfg["tile_size"], cfg["hd_side_height"]
    h = size + side_h
    img = Image.new("RGBA", (16 * (size + 4), len(mats) * 2 * (h + 6)), (60, 90, 50, 255))
    row = 0
    for mat in mats.values():
        for hd in (False, True):
            for m in range(16):
                img.alpha_composite(bake_tile(mat, m, 0, size, side_h, cfg["seed"], hd), (m * (size + 4), row * (h + 6)))
            row += 1
    path = os.path.join(root, "all_masks.png")
    img.convert("RGB").save(path)
    return path


def main():
    base = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    cfg_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(base, "tools", "tileset", "tileset.json")
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(base, "tools", "tileset", "out")
    cfg = json.load(open(cfg_path))
    size = cfg["tile_size"]
    mats = {}
    for name, m in cfg["materials"].items():
        m = dict(m)
        m["top_img"] = load(os.path.join(base, m["top"]), size * 2)
        m["side_img"] = load(os.path.join(base, m["side"]), size)
        mats[name] = m
    os.makedirs(out, exist_ok=True)
    for name, m in mats.items():
        for hd in (False, True):
            bake_set(name, m, cfg, hd, out)
    print(preview(cfg, mats, False, out))
    print(preview(cfg, mats, True, out))
    print(sheet(cfg, mats, out))


if __name__ == "__main__":
    main()
