"""Turns a smooth icon into small pixel art: area downscale, a limited palette, an optional outline.

Usage:
  python tools/pixel_art.py --sheet out.png            comparison sheet of the settings below on several icons
  python tools/pixel_art.py in.png out.png [--size 32 --colors 16 --outline none|fixed|selout --downscale box|nearest|lanczos --boost 1.15]
"""

import argparse
import os
import sys

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ALPHA_CUT = 110
OUTLINE_FIXED = (15, 20, 35)
SELOUT_SHADE = 0.4
DOWNSCALE = {"box": Image.BOX, "nearest": Image.NEAREST, "lanczos": Image.LANCZOS}


def clean_background(image, tolerance=25):
    """For a flat-colour background (no alpha): the top-left colour becomes transparent."""
    arr = np.array(image.convert("RGBA"))
    if arr[0, 0, 3] < 255:
        return Image.fromarray(arr)
    diff = np.abs(arr[:, :, :3].astype(int) - arr[0, 0, :3].astype(int)).sum(axis=2)
    arr[diff <= tolerance, 3] = 0
    return Image.fromarray(arr)


def flatten_tones(image, opaque, tones):
    """Gradients become `tones` flat bands of value (shadow, base, light), each hue kept: the icon is lit by one light."""
    hsv = np.array(image.convert("HSV")).astype(float)
    values = hsv[:, :, 2]
    low, high = values[opaque].min(), values[opaque].max()
    if high - low < 1:
        return image
    band = np.clip(np.round((values - low) / (high - low) * (tones - 1)), 0, tones - 1)
    hsv[:, :, 2] = low + band / (tones - 1) * (high - low)
    return Image.fromarray(hsv.astype(np.uint8), "HSV").convert("RGB")


def add_light_and_shadow(rgb, solid, blink, edge_shadow):
    """One highlight spot in the upper left and a one-pixel shadow line along the lower and right edges."""
    result = rgb.copy().astype(float)
    padded = np.pad(solid, 1)
    height, width = solid.shape
    inside = solid & padded[0:height, 1:width + 1] & padded[2:height + 2, 1:width + 1] & padded[1:height + 1, 0:width] & padded[1:height + 1, 2:width + 2]
    if edge_shadow:
        lower = ~padded[2:height + 2, 1:width + 1] | ~padded[1:height + 1, 2:width + 2]
        edge = solid & lower
        result[edge] *= 0.62
    if blink and inside.any():
        ys, xs = np.nonzero(solid)
        quadrant = inside.copy()
        quadrant[int((ys.min() + ys.max()) / 2 + 1):, :] = False
        quadrant[:, int((xs.min() + xs.max()) / 2 + 1):] = False
        if quadrant.any():
            luma = result.sum(axis=2) * quadrant
            y, x = np.unravel_index(np.argmax(luma), luma.shape)
            for dy, dx in ((0, 0), (0, 1)):
                if solid[y + dy, min(width - 1, x + dx)]:
                    result[y + dy, min(width - 1, x + dx)] = result[y + dy, min(width - 1, x + dx)] * 0.25 + 255 * 0.75
    return Image.fromarray(np.clip(result, 0, 255).astype(np.uint8), "RGB")


def to_pixel_art(image, size=32, colors=16, outline="fixed", outline_color=OUTLINE_FIXED, downscale="box", boost=1.0, sharpen=True, tones=0, blink=False, edge_shadow=False):
    image = image.convert("RGBA")
    box = image.getbbox()
    if box:
        image = image.crop(box)
    inner = size - 2 if outline != "none" else size
    fit = min(inner / image.width, inner / image.height)
    width, height = max(1, round(image.width * fit)), max(1, round(image.height * fit))
    if boost != 1.0:
        alpha = image.getchannel("A")
        image = ImageEnhance.Contrast(ImageEnhance.Color(image.convert("RGB")).enhance(boost)).enhance(boost).convert("RGBA")
        image.putalpha(alpha)
    # Colours are averaged only under opaque pixels, so a transparent edge does not darken the rim.
    premultiplied = np.array(image).astype(float)
    premultiplied[:, :, :3] *= premultiplied[:, :, 3:4] / 255.0
    channels = [Image.fromarray(premultiplied[:, :, k].astype(np.float32), mode="F").resize((width, height), DOWNSCALE[downscale]) for k in range(4)]
    small = np.stack([np.array(c) for c in channels], axis=2)
    alpha = small[:, :, 3]
    rgb = np.where(alpha[:, :, None] > 0, small[:, :, :3] / np.maximum(alpha[:, :, None], 1) * 255.0, 0)
    rgb_image = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8), "RGB")
    if sharpen:
        rgb_image = rgb_image.filter(ImageFilter.UnsharpMask(radius=0.6, percent=70, threshold=2))
    if tones:
        rgb_image = flatten_tones(rgb_image, alpha >= ALPHA_CUT, tones)
    quant = rgb_image.quantize(colors=colors, method=Image.MEDIANCUT, dither=Image.NONE).convert("RGB")
    solid = (alpha >= ALPHA_CUT).astype(np.uint8)
    if edge_shadow or blink:
        quant = add_light_and_shadow(np.array(quant), solid > 0, blink, edge_shadow)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    layer = np.dstack([np.array(quant), solid * 255]).astype(np.uint8)
    offset = ((size - width) // 2, (size - height) // 2)
    canvas.paste(Image.fromarray(layer, "RGBA"), offset)
    if outline == "none":
        return canvas
    pixels = np.array(canvas)
    opaque = pixels[:, :, 3] > 0
    padded = np.pad(opaque, 1)
    neighbours = [padded[1 + dy:1 + dy + size, 1 + dx:1 + dx + size] for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1))]
    border = ~opaque & (neighbours[0] | neighbours[1] | neighbours[2] | neighbours[3])
    if outline == "fixed":
        pixels[border] = OUTLINE_FIXED + (255,) if outline_color is None else tuple(outline_color) + (255,)
        return Image.fromarray(pixels, "RGBA")
    # selout: the outline takes the darkened colour of the neighbouring pixel, so it reads as part of the shape.
    padded_rgb = np.pad(pixels[:, :, :3], ((1, 1), (1, 1), (0, 0)))
    for y, x in zip(*np.nonzero(border)):
        candidates = [padded_rgb[1 + y + dy, 1 + x + dx] for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)) if padded[1 + y + dy, 1 + x + dx]]
        pixels[y, x] = tuple(int(v * SELOUT_SHADE) for v in np.mean(candidates, axis=0)) + (255,)
    return Image.fromarray(pixels, "RGBA")


VARIANTS = [
    ("D box 12c navy +sat", dict(colors=12, outline="fixed", boost=1.2)),
    ("F box 8c selout +sat", dict(colors=8, outline="selout", boost=1.25)),
    ("G lanczos 3 tones light+shadow", dict(colors=12, outline="selout", downscale="lanczos", boost=1.3, tones=3, blink=True, edge_shadow=True)),
    ("H lanczos 4 tones light+shadow navy", dict(colors=14, outline="fixed", downscale="lanczos", boost=1.3, tones=4, blink=True, edge_shadow=True)),
    ("I lanczos 3 tones blink only", dict(colors=10, outline="selout", downscale="lanczos", boost=1.4, tones=3, blink=True)),
    ("J lanczos gradients kept +sat", dict(colors=32, outline="selout", downscale="lanczos", boost=1.3, blink=True, edge_shadow=True)),
]
SAMPLES = ["weapon_cannon", "stat_hp", "icon_shield", "icon_speed", "stat_damage", "weapon_mine", "stat_tracks"]


def make_sheet(path):
    cell, scale = 32 * 4 + 8, 4
    sheet = Image.new("RGBA", (cell * (len(VARIANTS) + 1), cell * len(SAMPLES)), (14, 30, 56, 255))
    for row, name in enumerate(SAMPLES):
        source = Image.open(os.path.join(ROOT, "art_fullhd", "abilities", name + ".png"))
        sheet.alpha_composite(source.resize((128, 128), Image.LANCZOS), (4, row * cell + 4))
        for column, (_, options) in enumerate(VARIANTS):
            small = to_pixel_art(source, **options)
            sheet.alpha_composite(small.resize((32 * scale, 32 * scale), Image.NEAREST), ((column + 1) * cell + 4, row * cell + 4))
    sheet.convert("RGB").save(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="*")
    parser.add_argument("--sheet")
    parser.add_argument("--size", type=int, default=32)
    parser.add_argument("--colors", type=int, default=16)
    parser.add_argument("--outline", default="selout", choices=["none", "fixed", "selout"])
    parser.add_argument("--downscale", default="box", choices=list(DOWNSCALE))
    parser.add_argument("--boost", type=float, default=1.0)
    args = parser.parse_args()
    if args.sheet:
        make_sheet(args.sheet)
        return 0
    source, target = args.paths
    to_pixel_art(clean_background(Image.open(source)), args.size, args.colors, args.outline, None, args.downscale, args.boost).save(target)
    return 0


if __name__ == "__main__":
    sys.exit(main())
