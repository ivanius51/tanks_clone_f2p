"""Checks that ground tiles are seamless: the tone jump across each wrapped edge must be no larger than inside the tile.

For every art/tilesets/*/ground_*.png and art_hd/tilesets/*/ground_*.png it compares the tone jump
across the wrapped edge (4 px bands each side, averaged in 4 px blocks) with the jumps across
every interior cut line: the edge must not stand out, i.e. stay under --max-ratio times the 90th percentile. A ratio above --max-ratio means a visible seam.

    python tools/check_tileable.py [--max-ratio 1.25] [files...]
Exit code 0 when every tile passes.
"""

import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngio  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_MAX_RATIO = 1.25
BAND = 4


def difference(a, b):
    return (abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])) / 3.0


def band_mean(image, x0, y0, dx, dy, length):
    total = [0.0, 0.0, 0.0]
    for i in range(length):
        color = image.get((x0 + dx * i) % image.width, (y0 + dy * i) % image.height)
        for channel in range(3):
            total[channel] += color[channel]
    return [value / length for value in total]


def line_jump(image, position, bVertical):
    """Mean tone difference between the BAND pixels before and after a cut line, sampled in short blocks along it."""
    w, h = image.width, image.height
    span = h if bVertical else w
    jumps = []
    for start in range(0, span, BAND):
        if bVertical:
            before = [band_mean(image, position - BAND, y, 1, 0, BAND) for y in range(start, start + BAND)]
            after = [band_mean(image, position, y, 1, 0, BAND) for y in range(start, start + BAND)]
        else:
            before = [band_mean(image, x, position - BAND, 0, 1, BAND) for x in range(start, start + BAND)]
            after = [band_mean(image, x, position, 0, 1, BAND) for x in range(start, start + BAND)]
        a = [sum(c[i] for c in before) / BAND for i in range(3)]
        b = [sum(c[i] for c in after) / BAND for i in range(3)]
        jumps.append(difference(a, b))
    return sum(jumps) / len(jumps)


def seam_ratios(image):
    """Jump across the wrapped edge divided by the 90th percentile jump across every interior cut line."""
    ratios = []
    for bVertical, size in ((True, image.width), (False, image.height)):
        inner = sorted(line_jump(image, p, bVertical) for p in range(BAND, size - BAND + 1))
        typical = inner[int(len(inner) * 0.9)]
        ratios.append(line_jump(image, 0, bVertical) / max(typical, 0.5))
    return ratios


def display_path(path):
    try:
        return os.path.relpath(path, ROOT)
    except ValueError:
        return path


def main():
    args = sys.argv[1:]
    max_ratio = DEFAULT_MAX_RATIO
    if "--max-ratio" in args:
        index = args.index("--max-ratio")
        max_ratio = float(args[index + 1])
        del args[index:index + 2]
    files = args or sorted(glob.glob(os.path.join(ROOT, "art", "tilesets", "*", "ground_*.png"))
                           + glob.glob(os.path.join(ROOT, "art_hd", "tilesets", "*", "ground_*.png")))
    failed = 0
    for path in files:
        ratio_x, ratio_y = seam_ratios(pngio.load(path))
        ok = ratio_x <= max_ratio and ratio_y <= max_ratio
        failed += not ok
        print("%s  %-60s x=%.2f y=%.2f" % ("ok  " if ok else "FAIL", display_path(path), ratio_x, ratio_y))
    print("%d of %d tiles failed (max ratio %.2f)" % (failed, len(files), max_ratio))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
