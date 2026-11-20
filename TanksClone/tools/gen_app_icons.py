"""Writes the application icon (art/app/icon.png) into every platform project.

Replaces the default Cocos2d-x icons: proj.win32/res/game.ico, Android mipmaps, the iOS app icon set,
the macOS Icon.icns and the iOS launch screen background. Existing image sizes are kept.
Run tools/gen_placeholder_art.py first.
"""

import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngio  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ICON = os.path.join(ROOT, "art", "app", "icon.png")
BACKGROUND = (24, 30, 40, 255)
ICO_SIZES = (16, 24, 32, 48, 64, 256)
ICNS_TYPES = ((b"ic07", 128), (b"ic08", 256), (b"ic09", 512), (b"ic10", 1024))


def ico_bitmap(image):
    """An icon entry in the classic DIB form: BGRA bottom-up plus a 1-bit AND mask."""
    size = image.width
    header = struct.pack("<IiiHHIIiiII", 40, size, size * 2, 1, 32, 0, 0, 0, 0, 0, 0)
    pixels = bytearray()
    for y in range(size - 1, -1, -1):
        for x in range(size):
            r, g, b, a = image.get(x, y)
            pixels += bytes((b, g, r, a))
    mask_stride = ((size + 31) // 32) * 4
    mask = bytes(mask_stride * size)
    return header + bytes(pixels) + mask


def write_ico(source, path):
    entries = []
    for size in ICO_SIZES:
        image = source.scaled(size, size)
        data = pngio.encode(image) if size >= 256 else ico_bitmap(image)
        entries.append((size, data))

    header = struct.pack("<HHH", 0, 1, len(entries))
    offset = 6 + 16 * len(entries)
    directory = b""
    body = b""
    for size, data in entries:
        dimension = 0 if size >= 256 else size
        directory += struct.pack("<BBBBHHII", dimension, dimension, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
        body += data
    with open(path, "wb") as file:
        file.write(header + directory + body)


def write_icns(source, path):
    body = b""
    for kind, size in ICNS_TYPES:
        data = pngio.encode(source.scaled(size, size))
        body += kind + struct.pack(">I", len(data) + 8) + data
    with open(path, "wb") as file:
        file.write(b"icns" + struct.pack(">I", len(body) + 8) + body)


def replace_png(source, path):
    width, height = pngio.read_size(path)
    pngio.save(source.scaled(width, height), path)


def replace_with_background(path):
    width, height = pngio.read_size(path)
    pngio.save(pngio.Image(width, height, BACKGROUND), path)


def main():
    source = pngio.load(ICON)
    write_ico(source, os.path.join(ROOT, "proj.win32", "res", "game.ico"))

    for folder, _, files in os.walk(os.path.join(ROOT, "proj.android", "app", "res")):
        for name in files:
            if name == "ic_launcher.png":
                replace_png(source, os.path.join(folder, name))

    icon_set = os.path.join(ROOT, "proj.ios_mac", "ios", "Images.xcassets", "AppIcon.appiconset")
    if os.path.isdir(icon_set):
        for name in os.listdir(icon_set):
            if name.endswith(".png"):
                replace_png(source, os.path.join(icon_set, name))

    icns = os.path.join(ROOT, "proj.ios_mac", "mac", "Icon.icns")
    if os.path.exists(icns):
        write_icns(source, icns)

    launch = os.path.join(ROOT, "proj.ios_mac", "ios", "LaunchScreenBackground.png")
    if os.path.exists(launch):
        replace_with_background(launch)
    print("icons written")


if __name__ == "__main__":
    main()
