"""Rebuilds the SD tank sprites in the cel-shaded pixel look from the HD ones (art_hd -> art).

Each HD frame is enlarged 4x (standing in for FullHD), flattened to a few tone bands, shrunk with nearest neighbour to the size of
the existing SD frame and cut to one palette per tank, so the hull, turret and track layers keep the same colours.
Skipped: the move frames "anim_N" (their phases differ between the sets; layered tanks do not use them). The tanks in KEEP
keep their old hull and tracks and only get the new turret.
Run tools/pack_atlases.py afterwards. Usage: python tools/gen_sd_tanks.py
"""

import glob
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_placeholder_art as placeholder  # noqa: E402
import gen_turrets  # noqa: E402
import pixel_art  # noqa: E402
import pngio  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
KEEP = ("Btr", "Boss", "Cloaker")
COLORS = 16
CEL = dict(sigma_color=50, shades=4, sat_boost=1.1, contrast_boost=1.0)
TURRET_CEL = dict(sigma_color=90, shades=3, sat_boost=1.1, contrast_boost=1.0)
TURRET_COLORS = 12
ALPHA_CUT = 110
UPSCALE = 4


def shrink(hd_path, size, cel=None):
    return shrink_image(Image.open(hd_path), size, cel)


def shrink_image(hd, size, cel=None):
    hd = hd.convert("RGBA")
    big = hd.resize((hd.width * UPSCALE, hd.height * UPSCALE), Image.LANCZOS)
    return pixel_art.cel_flatten(big, **(cel or CEL)).resize(size, Image.NEAREST)


def with_palette(image, palette):
    alpha = np.array(image.getchannel("A")) > ALPHA_CUT
    out = np.array(image.convert("RGB").quantize(palette=palette, dither=Image.NONE).convert("RGBA"))
    out[:, :, 3] = alpha * 255
    return Image.fromarray(out, "RGBA")


def tank_folders():
    return [os.path.join(ROOT, "art", "player")] + sorted(glob.glob(os.path.join(ROOT, "art", "enemies", "*")))


def old_body(atlas):
    """The hull and tracks of the old SD generator, for the tanks that keep their old look under a new turret."""
    style = placeholder.TANKS[atlas][1]
    path = os.path.join(ROOT, "art", "_body_tmp.png")
    pngio.save(placeholder.draw_tank(style, 0, "body"), path)
    image = Image.open(path).convert("RGBA")
    os.remove(path)
    return image


def hd_path_of(folder):
    return folder.replace(os.sep + "art" + os.sep, os.sep + "art_hd" + os.sep)


def convert_turret(folder, name):
    """The turret layer, with the flatter look that keeps a small cast shape readable."""
    path = os.path.join(folder, name + "_turret.png")
    reference = shrink(os.path.join(hd_path_of(folder), name + "_turret.png"), Image.open(path).size, TURRET_CEL)
    palette = reference.convert("RGB").quantize(colors=TURRET_COLORS, method=Image.MEDIANCUT, dither=Image.NONE)
    turret = with_palette(reference, palette)
    turret.save(path)
    return turret


def convert_folder(folder):
    atlas = "player" if os.path.basename(folder) == "player" else "enemies/" + os.path.basename(folder)
    name = placeholder.TANKS[atlas][0]
    turret = convert_turret(folder, name)
    files = [f for f in sorted(os.listdir(folder)) if f.endswith(".png") and "_anim_" not in f and not f.endswith("_turret.png") and f != name + ".png"]
    if os.path.basename(folder) in KEEP:
        whole = old_body(atlas)
    else:
        body = gen_turrets.body_image(placeholder.TANKS[atlas][1], atlas)
        size = Image.open(os.path.join(folder, name + ".png")).size
        reference = shrink_image(body, size)
        palette = reference.convert("RGB").quantize(colors=COLORS, method=Image.MEDIANCUT, dither=Image.NONE)
        whole = with_palette(reference, palette)
        for file in files:
            if file == name + "_wreck.png" or "_hull" in file or "_track_" in file:
                path = os.path.join(folder, file)
                with_palette(shrink(os.path.join(hd_path_of(folder), file), Image.open(path).size), palette).save(path)
    whole.alpha_composite(turret)
    whole.save(os.path.join(folder, name + ".png"))


def main():
    for folder in tank_folders():
        convert_folder(folder)
    print("ok")


if __name__ == "__main__":
    main()
