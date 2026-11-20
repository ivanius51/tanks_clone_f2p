"""Packs the source frames in art/ into Cocos2d-x sprite atlases (.plist + .png).

Every folder under art/ that holds PNG files becomes one atlas:
    art/common/*.png            -> Resources/textures/common.plist + common.png
    art/enemies/Heavy/*.png     -> Resources/textures/enemies/Heavy.plist + Heavy.png
The frame name is the file name without ".png". art/app/ (the application icon) is skipped.
Tilesets share frame names (every tileset has "wall_brick"), so their frames get the tileset as a prefix:
    art/tilesets/desert/wall_brick.png  -> frame "desert/wall_brick"
Frame names must be unique across all atlases: the sprite frame cache is shared.

Usage: python tools/pack_atlases.py [--hd] [--check]
    --hd      pack the HD set: art_hd/ -> Resources/textures_hd/ (the SD atlases are left untouched)
    --check   only verify that the atlases are up to date (exit code 1 if not)
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngio  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HD = "--hd" in sys.argv
ART = os.path.join(ROOT, "art_hd" if HD else "art")
OUTPUT = os.path.join(ROOT, "Resources", "textures_hd" if HD else "textures")
SKIPPED = {"app"}
PREFIXED = {"tilesets"}
PADDING = 2  # pixels between frames; the frame border is repeated into it against filtering seams


def find_atlas_folders():
    result = []
    for folder, _, files in os.walk(ART):
        relative = os.path.relpath(folder, ART).replace("\\", "/")
        if relative.split("/")[0] in SKIPPED:
            continue
        if any(name.lower().endswith(".png") for name in files):
            result.append((relative, folder))
    return sorted(result)


def shelf_pack(frames, width):
    """Places frames row by row, tallest first. Returns {name: (x, y)} and the used height."""
    placed = {}
    x = y = row_height = 0
    for name, image in sorted(frames.items(), key=lambda item: (-item[1].height, item[0])):
        w, h = image.width + PADDING * 2, image.height + PADDING * 2
        if x + w > width:
            x, y, row_height = 0, y + row_height, 0
        placed[name] = (x + PADDING, y + PADDING)
        x += w
        row_height = max(row_height, h)
    return placed, y + row_height


def next_power_of_two(value):
    result = 1
    while result < value:
        result *= 2
    return result


def pack(frames):
    area = sum((image.width + PADDING * 2) * (image.height + PADDING * 2) for image in frames.values())
    widest = max(image.width for image in frames.values()) + PADDING * 2
    width = max(next_power_of_two(int(area ** 0.5)), next_power_of_two(widest))
    placed, height = shelf_pack(frames, width)
    height = next_power_of_two(height)

    sheet = pngio.Image(width, height)
    for name, (left, top) in placed.items():
        image = frames[name]
        extrude(sheet, image, left, top)
        sheet.copy_into(image, left, top)
    return sheet, placed


def extrude(sheet, image, left, top):
    for y in range(-1, image.height + 1):
        for x in range(-1, image.width + 1):
            source = image.get(min(max(x, 0), image.width - 1), min(max(y, 0), image.height - 1))
            sheet.set(left + x, top + y, source)


def plist_text(frames, placed, texture_name, size):
    lines = [
        '<?xml version="1.0" encoding="UTF-8"?>',
        '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">',
        '<plist version="1.0">',
        "<dict>",
        "\t<key>frames</key>",
        "\t<dict>",
    ]
    for name in sorted(frames):
        image = frames[name]
        x, y = placed[name]
        w, h = image.width, image.height
        lines += [
            "\t\t<key>%s</key>" % name,
            "\t\t<dict>",
            "\t\t\t<key>frame</key><string>{{%d,%d},{%d,%d}}</string>" % (x, y, w, h),
            "\t\t\t<key>offset</key><string>{0,0}</string>",
            "\t\t\t<key>rotated</key><false/>",
            "\t\t\t<key>sourceColorRect</key><string>{{0,0},{%d,%d}}</string>" % (w, h),
            "\t\t\t<key>sourceSize</key><string>{%d,%d}</string>" % (w, h),
            "\t\t</dict>",
        ]
    lines += [
        "\t</dict>",
        "\t<key>metadata</key>",
        "\t<dict>",
        "\t\t<key>format</key><integer>2</integer>",
        "\t\t<key>realTextureFileName</key><string>%s</string>" % texture_name,
        "\t\t<key>size</key><string>{%d,%d}</string>" % size,
        "\t\t<key>textureFileName</key><string>%s</string>" % texture_name,
        "\t</dict>",
        "</dict>",
        "</plist>",
        "",
    ]
    return "\n".join(lines)


def frame_prefix(relative):
    parts = relative.split("/")
    return parts[-1] + "/" if parts[0] in PREFIXED else ""


def build(relative, folder):
    frames = {}
    prefix = frame_prefix(relative)
    for name in sorted(os.listdir(folder)):
        if name.lower().endswith(".png"):
            frames[prefix + name[:-4]] = pngio.load(os.path.join(folder, name))
    sheet, placed = pack(frames)

    target = os.path.join(OUTPUT, *relative.split("/"))
    texture_name = os.path.basename(target) + ".png"
    return target, pngio.encode(sheet), plist_text(frames, placed, texture_name, (sheet.width, sheet.height))


def read_bytes(path):
    if not os.path.exists(path):
        return None
    with open(path, "rb") as file:
        return file.read()


def main():
    check_only = "--check" in sys.argv
    stale = []
    owners = {}
    for relative, folder in find_atlas_folders():
        for name in os.listdir(folder):
            if name.lower().endswith(".png"):
                frame = frame_prefix(relative) + name[:-4]
                if frame in owners:
                    print("frame '%s' is in both %s and %s: frame names must be unique" % (frame, owners[frame], relative))
                    return 1
                owners[frame] = relative

    for relative, folder in find_atlas_folders():
        target, png, plist = build(relative, folder)
        plist_bytes = plist.encode("utf-8")
        if read_bytes(target + ".png") == png and read_bytes(target + ".plist") == plist_bytes:
            continue
        if check_only:
            stale.append(relative)
            continue
        os.makedirs(os.path.dirname(target), exist_ok=True)
        with open(target + ".png", "wb") as file:
            file.write(png)
        with open(target + ".plist", "wb") as file:
            file.write(plist_bytes)
        print("packed", os.path.relpath(target, ROOT) + ".plist")

    if check_only and stale:
        print("out of date atlases (run tools/pack_atlases.py):", ", ".join(stale))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
