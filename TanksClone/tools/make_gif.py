"""Turns numbered PNG frames into an animated GIF, with the standard library only.

Usage: python tools/make_gif.py <frames folder> <output.gif> [--fps 8] [--scale 0.5] [--every 1]
The fallback of tools/make_clip.ps1 when ffmpeg is not installed.

The frames are scaled down (nearest neighbour, the art is pixel art), reduced to one shared palette of
the 255 most used colours (every other colour goes to the nearest of them) and LZW-compressed.
"""

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngio  # noqa: E402

PALETTE_SIZE = 255  # index 255 stays unused


def load_frames(folder, scale, every):
    names = sorted(name for name in os.listdir(folder) if name.lower().endswith(".png"))[::every]
    frames = []
    for name in names:
        image = pngio.load(os.path.join(folder, name))
        width = max(1, int(image.width * scale))
        height = max(1, int(image.height * scale))
        frames.append(image.scaled(width, height))
    return frames


def quantize(pixel):
    r, g, b = pixel[0], pixel[1], pixel[2]
    return (r & 0xF8, g & 0xF8, b & 0xF8)


def build_palette(frames):
    counts = {}
    for frame in frames[:: max(1, len(frames) // 8)]:
        for pixel in frame.pixels:
            key = quantize(pixel)
            counts[key] = counts.get(key, 0) + 1
    return [color for color, _ in sorted(counts.items(), key=lambda item: -item[1])[:PALETTE_SIZE]]


def index_frames(frames, palette):
    exact = {color: index for index, color in enumerate(palette)}
    cache = {}

    def nearest(color):
        found = exact.get(color)
        if found is not None:
            return found
        found = cache.get(color)
        if found is None:
            found = min(range(len(palette)), key=lambda i: sum((a - b) ** 2 for a, b in zip(palette[i], color)))
            cache[color] = found
        return found

    return [bytes(nearest(quantize(pixel)) for pixel in frame.pixels) for frame in frames]


def lzw(indices, min_code_size=8):
    clear = 1 << min_code_size
    end = clear + 1
    table = {bytes([i]): i for i in range(clear)}
    next_code = end + 1
    code_size = min_code_size + 1
    out = bytearray()
    bit_buffer = 0
    bit_count = 0

    def emit(code):
        nonlocal bit_buffer, bit_count
        bit_buffer |= code << bit_count
        bit_count += code_size
        while bit_count >= 8:
            out.append(bit_buffer & 0xFF)
            bit_buffer >>= 8
            bit_count -= 8

    emit(clear)
    current = b""
    for value in indices:
        candidate = current + bytes([value])
        if candidate in table:
            current = candidate
            continue
        emit(table[current])
        if next_code < 4096:
            table[candidate] = next_code
            next_code += 1
            if next_code > (1 << code_size) and code_size < 12:
                code_size += 1
        else:
            emit(clear)
            table = {bytes([i]): i for i in range(clear)}
            next_code = end + 1
            code_size = min_code_size + 1
        current = bytes([value])
    if current:
        emit(table[current])
    emit(end)
    if bit_count:
        out.append(bit_buffer & 0xFF)
    return bytes(out)


def sub_blocks(data):
    result = bytearray()
    for start in range(0, len(data), 255):
        chunk = data[start:start + 255]
        result.append(len(chunk))
        result += chunk
    result.append(0)
    return bytes(result)


def write_gif(path, frames, indexed, palette, fps):
    width, height = frames[0].width, frames[0].height
    color_table = bytearray()
    for color in palette + [(0, 0, 0)] * (256 - len(palette)):
        color_table += bytes(color)

    out = bytearray(b"GIF89a")
    out += struct.pack("<HHBBB", width, height, 0xF7, 0, 0)
    out += color_table
    out += b"\x21\xFF\x0BNETSCAPE2.0\x03\x01\x00\x00\x00"
    delay = max(2, int(round(100 / fps)))
    for data in indexed:
        out += struct.pack("<BBBBHBB", 0x21, 0xF9, 4, 0, delay, 0, 0)
        out += struct.pack("<BHHHHB", 0x2C, 0, 0, width, height, 0)
        out.append(8)
        out += sub_blocks(lzw(data))
    out += b"\x3B"
    with open(path, "wb") as file:
        file.write(out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("frames")
    parser.add_argument("output")
    parser.add_argument("--fps", type=float, default=8.0)
    parser.add_argument("--scale", type=float, default=0.5)
    parser.add_argument("--every", type=int, default=1, help="take every N-th frame")
    args = parser.parse_args()

    frames = load_frames(args.frames, args.scale, max(1, args.every))
    if not frames:
        print("no PNG frames in", args.frames)
        return 1
    palette = build_palette(frames)
    indexed = index_frames(frames, palette)
    write_gif(args.output, frames, indexed, palette, args.fps)
    print("written", args.output, "(%d frames, %d bytes)" % (len(frames), os.path.getsize(args.output)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
