"""Rebuilds the SD tank sprites in the cel-shaded pixel look from the HD ones (art_hd -> art).

Each HD frame is enlarged 4x (standing in for FullHD), flattened to a few tone bands, shrunk with nearest neighbour to the size of
the existing SD frame and cut to one palette per tank, so the hull, turret and track layers keep the same colours.
Skipped: the move frames "anim_N" (their phases differ between the sets; layered tanks do not use them) and the tanks in KEEP.
Run tools/pack_atlases.py afterwards. Usage: python tools/gen_sd_tanks.py
"""

import glob
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pixel_art  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KEEP = ("Btr", "Boss", "Cloaker")
COLORS = 16
CEL = dict(sigma_color=50, shades=4, sat_boost=1.1, contrast_boost=1.0)
ALPHA_CUT = 110
UPSCALE = 4


def shrink(hd_path, size):
    hd = Image.open(hd_path).convert("RGBA")
    big = hd.resize((hd.width * UPSCALE, hd.height * UPSCALE), Image.LANCZOS)
    return pixel_art.cel_flatten(big, **CEL).resize(size, Image.NEAREST)


def with_palette(image, palette):
    alpha = np.array(image.getchannel("A")) > ALPHA_CUT
    out = np.array(image.convert("RGB").quantize(palette=palette, dither=Image.NONE).convert("RGBA"))
    out[:, :, 3] = alpha * 255
    return Image.fromarray(out, "RGBA")


def tank_folders():
    return [os.path.join(ROOT, "art", "player")] + sorted(glob.glob(os.path.join(ROOT, "art", "enemies", "*")))


def convert_folder(folder):
    if os.path.basename(folder) in KEEP:
        return
    files = [f for f in sorted(os.listdir(folder)) if f.endswith(".png") and "_anim_" not in f]
    base = min(files, key=len)
    hd_dir = folder.replace(os.sep + "art" + os.sep, os.sep + "art_hd" + os.sep)
    reference = shrink(os.path.join(hd_dir, base), Image.open(os.path.join(folder, base)).size)
    palette = reference.convert("RGB").quantize(colors=COLORS, method=Image.MEDIANCUT, dither=Image.NONE)
    for name in files:
        sd_path = os.path.join(folder, name)
        result = with_palette(shrink(os.path.join(hd_dir, name), Image.open(sd_path).size), palette)
        result.save(sd_path)


def main():
    for folder in tank_folders():
        convert_folder(folder)
    print("ok")


if __name__ == "__main__":
    main()
