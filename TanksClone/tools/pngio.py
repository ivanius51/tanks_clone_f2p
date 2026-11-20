"""Minimal PNG reading and writing (8-bit RGB/RGBA, no interlace) with the standard library only."""

import struct
import zlib


class Image:
    """An RGBA image stored as a flat list of (r, g, b, a) tuples, row by row from the top."""

    def __init__(self, width, height, fill=(0, 0, 0, 0)):
        self.width = width
        self.height = height
        self.pixels = [fill] * (width * height)

    def get(self, x, y):
        return self.pixels[y * self.width + x]

    def set(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            if len(color) == 3:
                color = (color[0], color[1], color[2], 255)
            self.pixels[y * self.width + x] = color

    def blit(self, other, left, top):
        for y in range(other.height):
            for x in range(other.width):
                color = other.get(x, y)
                if color[3]:
                    self.set(left + x, top + y, color)

    def copy_into(self, other, left, top):
        """Copies every pixel of other, transparent ones included."""
        for y in range(other.height):
            row = other.pixels[y * other.width:(y + 1) * other.width]
            start = (top + y) * self.width + left
            self.pixels[start:start + other.width] = row

    def scaled(self, width, height):
        """Nearest-neighbour resize: keeps pixel art sharp."""
        result = Image(width, height)
        for y in range(height):
            sy = min(self.height - 1, y * self.height // height)
            for x in range(width):
                sx = min(self.width - 1, x * self.width // width)
                result.pixels[y * width + x] = self.get(sx, sy)
        return result


def _chunk(kind, data):
    body = kind + data
    return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)


def encode(image):
    raw = bytearray()
    for y in range(image.height):
        raw.append(0)
        for r, g, b, a in image.pixels[y * image.width:(y + 1) * image.width]:
            raw += bytes((r, g, b, a))
    header = struct.pack(">IIBBBBB", image.width, image.height, 8, 6, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + _chunk(b"IHDR", header)
            + _chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + _chunk(b"IEND", b""))


def save(image, path):
    with open(path, "wb") as file:
        file.write(encode(image))


def read_size(path):
    with open(path, "rb") as file:
        head = file.read(24)
    return struct.unpack(">II", head[16:24])


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


def load(path):
    with open(path, "rb") as file:
        data = file.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(path + ": not a PNG file")

    position = 8
    idat = b""
    width = height = channels = 0
    while position < len(data):
        length, kind = struct.unpack(">I4s", data[position:position + 8])
        body = data[position + 8:position + 8 + length]
        position += 12 + length
        if kind == b"IHDR":
            width, height, depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", body)
            if depth != 8 or interlace or color_type not in (2, 6):
                raise ValueError(path + ": only 8-bit RGB/RGBA PNG without interlace is supported")
            channels = 4 if color_type == 6 else 3
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            break

    raw = zlib.decompress(idat)
    stride = width * channels
    image = Image(width, height)
    previous = bytearray(stride)
    offset = 0
    for y in range(height):
        kind = raw[offset]
        line = bytearray(raw[offset + 1:offset + 1 + stride])
        offset += 1 + stride
        for i in range(stride):
            left = line[i - channels] if i >= channels else 0
            up = previous[i]
            corner = previous[i - channels] if i >= channels else 0
            if kind == 1:
                line[i] = (line[i] + left) & 0xFF
            elif kind == 2:
                line[i] = (line[i] + up) & 0xFF
            elif kind == 3:
                line[i] = (line[i] + (left + up) // 2) & 0xFF
            elif kind == 4:
                line[i] = (line[i] + _paeth(left, up, corner)) & 0xFF
        for x in range(width):
            pixel = line[x * channels:(x + 1) * channels]
            image.pixels[y * width + x] = tuple(pixel) if channels == 4 else (pixel[0], pixel[1], pixel[2], 255)
        previous = line
    return image
