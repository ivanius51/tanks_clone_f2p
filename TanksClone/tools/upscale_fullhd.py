"""Builds Resources/textures_fullhd/ from Resources/textures_hd/ (2x bigger) until real FullHD art is packed."""
import os
import re
import sys
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Resources')
SOURCE = os.path.join(ROOT, 'textures_hd')
TARGET = os.path.join(ROOT, 'textures_fullhd')
FACTOR = 2


def scale_plist(text):
    def scale_string(match):
        body = re.sub(r'-?\d+(\.\d+)?', lambda number: str(int(round(float(number.group(0)) * FACTOR))), match.group(2))
        return match.group(1) + body + match.group(3)
    text = re.sub(r'(<string>)(\{[-\d{},. ]*\})(</string>)', scale_string, text)
    return re.sub(r'(<key>(?:width|height)</key>\s*<integer>)(\d+)(</integer>)', lambda m: m.group(1) + str(int(m.group(2)) * FACTOR) + m.group(3), text)


def main():
    for folder, _, files in os.walk(SOURCE):
        out = os.path.join(TARGET, os.path.relpath(folder, SOURCE))
        os.makedirs(out, exist_ok=True)
        for name in files:
            path = os.path.join(folder, name)
            if name.endswith('.png'):
                image = Image.open(path).convert('RGBA')
                image.resize((image.width * FACTOR, image.height * FACTOR), Image.BICUBIC).save(os.path.join(out, name), optimize=True)
            elif name.endswith('.plist'):
                data = open(path, 'rb').read().decode('utf-8')
                new = scale_plist(data).replace(os.path.basename(path)[:-6] + '.png', name[:-6] + '.png')
                open(os.path.join(out, name), 'wb').write(new.encode('utf-8'))
    print('ok')


if __name__ == '__main__':
    sys.exit(main())
