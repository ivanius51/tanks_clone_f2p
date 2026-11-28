"""Crops the settings preview pictures out of `--showcase <dir> --part=previews --window=1920x1080` shots.

Usage: python tools/make_previews.py <showcase dir>
Writes Resources/images/previews/preview_*.jpg (the middle of the play area, 16:9, 512x288).
"""
import pathlib
import sys

from PIL import Image

SHOT_SIZE = (1920, 1080)
PLAY_TOP = 60
PLAY_BOTTOM = 940
BORDER = 4
OUT_SIZE = (512, 288)
# The shots are at 200% map scale; halving a 1024x576 middle keeps about 8 by 4.5 tiles.
CROP_FACTOR = 2
JPEG_QUALITY = 88
OUT_DIR = pathlib.Path(__file__).resolve().parent.parent / "Resources" / "images" / "previews"


def crop(path: pathlib.Path) -> Image.Image:
    shot = Image.open(path).convert("RGB")
    if shot.size != SHOT_SIZE:
        raise SystemExit(f"{path.name}: expected {SHOT_SIZE}, got {shot.size}")
    width, height = OUT_SIZE[0] * CROP_FACTOR, OUT_SIZE[1] * CROP_FACTOR
    # The camera follows the tank, so the middle of the play area holds it (or the bottom, when the camera stops at the map edge).
    left = (SHOT_SIZE[0] - width) // 2
    top = PLAY_BOTTOM - BORDER - height
    return shot.crop((left, top, left + width, top + height)).resize(OUT_SIZE, Image.BOX)


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    shots = sorted(pathlib.Path(sys.argv[1]).glob("preview_*.png"))
    if not shots:
        raise SystemExit("no preview_*.png shots")
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for shot in shots:
        target = OUT_DIR / (shot.stem + ".jpg")
        crop(shot).save(target, quality=JPEG_QUALITY, optimize=True)
        print(target)


if __name__ == "__main__":
    main()
