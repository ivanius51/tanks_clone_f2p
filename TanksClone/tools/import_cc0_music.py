"""Converts the sorted CC0 music (cc0_raw/<mood>/) to game OGG files and writes the id lists.

Usage: python tools/import_cc0_music.py
Writes Resources/music/<slug>.ogg, Resources/configs/credits.json and prints the lines for sounds.json / music.json.
Menu tracks get a 5 second fade in and out baked in.
"""
import json
import os
import re
import subprocess
import urllib.parse

BASE = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
RAW = os.path.join(BASE, "..", "cc0_raw")
OUT = os.path.join(BASE, "Resources", "music")
MENU_FADE = 5.0

# file name (url-decoded, no extension) -> (author, title, source page, license)
SOURCES = {
    "Anti Matter Magic": ("tricksntraps", "Anti Matter Magic", "https://opengameart.org/content/t-t-free-cyberpunk-pack", "CC0"),
    "Caves": ("tricksntraps", "Caves", "https://opengameart.org/content/t-t-free-cyberpunk-pack", "CC0"),
    "Currents": ("tricksntraps", "Currents", "https://opengameart.org/content/t-t-free-cyberpunk-pack", "CC0"),
    "Cyberpunk Moonlight Sonata_0": ("Joth", "Cyberpunk Moonlight Sonata", "https://opengameart.org/content/cyberpunk-moonlight-sonata", "CC0"),
    "DST-TowerDefenseTheme_1": ("DST", "Tower Defense Theme", "https://opengameart.org/content/tower-defense-theme", "CC0"),
    "S31-Night Prowler": ("section31", "Night Prowler", "https://opengameart.org/content/night-prowler", "CC0"),
    "synth_type_1": ("G_P", "Synth Type", "https://opengameart.org/content/synthwavetype", "CC0"),
    "Ruskerdax - Pondering the Cosmos_0": ("Ruskerdax", "Pondering the Cosmos", "https://opengameart.org/content/pondering-the-cosmos", "CC0"),
    "tension_loop": ("Tsorthan Grove", "Tension", "https://opengameart.org/content/tension-0", "CC0"),
}
TECHNO = ("Pro Sensory", "Electronic", "https://opengameart.org/content/electronic", "CC0")
GENERATED = ("Stable Audio (generated)", "Techno cinematic", "https://stableaudio.com/", "see docs/CREDITS.md")


def slugify(text):
    return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")


def duration(path):
    out = subprocess.check_output(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", path])
    return float(out)


def convert(src, slug, bMenu):
    dest = os.path.join(OUT, slug + ".ogg")
    args = ["ffmpeg", "-y", "-v", "error", "-i", src, "-ac", "2", "-ar", "44100"]
    if bMenu:
        total = duration(src)
        args += ["-af", f"afade=t=in:d={MENU_FADE},afade=t=out:st={total - MENU_FADE}:d={MENU_FADE}"]
    subprocess.check_call(args + ["-c:a", "libvorbis", "-q:a", "4", dest])


def main():
    result = {"menu": [], "calm": [], "intense": []}
    credits = []
    for folder, mood in (("listened possible Menu", "menu"), ("calm", "calm"), ("Intense", "intense"), ("Too Long Intense", "intense")):
        directory = os.path.join(RAW, folder)
        for name in sorted(os.listdir(directory)):
            stem = urllib.parse.unquote(os.path.splitext(name)[0])
            if stem.startswith("Techno_"):
                info = (TECHNO[0], f"{TECHNO[1]} ({stem})", TECHNO[2], TECHNO[3])
            elif stem.startswith("techno-cinematic"):
                info = (GENERATED[0], f"{GENERATED[1]} ({stem})", GENERATED[2], GENERATED[3])
            else:
                info = SOURCES[stem]
            slug = mood + "_" + slugify(info[1])
            convert(os.path.join(directory, name), slug, mood == "menu")
            result[mood].append(slug)
            credits.append({"id": "music." + slug, "title": info[1], "author": info[0], "source": info[2], "license": info[3]})
    with open(os.path.join(BASE, "Resources", "configs", "credits.json"), "w", encoding="utf-8") as f:
        json.dump({"credits": credits}, f, indent="\t", ensure_ascii=False)
        f.write("\n")
    for mood, slugs in result.items():
        print(mood, json.dumps(["music." + s for s in slugs]))
    for slugs in result.values():
        for s in slugs:
            print(f'\t\t"music.{s}": "music/{s}.ogg",')


if __name__ == "__main__":
    main()
