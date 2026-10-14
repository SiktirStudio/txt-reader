#!/usr/bin/env python3
"""Regenerate NovaPad's opaque-white PNG, multi-resolution ICO and ICNS (Pillow)."""
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SCALE = 4
SIZE = 1024


def main():
    def points(values):
        return [(int(x * SCALE), int(y * SCALE)) for x, y in values]

    symbol = Image.new("RGBA", (SIZE * SCALE, SIZE * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(symbol)
    navy = "#153C68"
    blue = "#147ED5"
    draw.rounded_rectangle((386 * SCALE, 184 * SCALE, 638 * SCALE, 298 * SCALE),
                           radius=28 * SCALE, fill=navy)
    draw.polygon(points([(386, 328), (638, 328), (724, 502),
                         (512, 818), (300, 502)]), fill=blue)
    draw.polygon(points([(512, 328), (638, 328), (724, 502), (512, 818)]), fill=navy)
    draw.ellipse((470 * SCALE, 454 * SCALE, 554 * SCALE, 538 * SCALE), fill="white")
    draw.line(points([(512, 496), (512, 818)]), fill="white", width=15 * SCALE)
    symbol = symbol.rotate(35, resample=Image.Resampling.BICUBIC)
    canvas = Image.new("RGBA", symbol.size, "white")
    canvas.alpha_composite(symbol)
    draw = ImageDraw.Draw(canvas)
    draw.polygon(points([(774, 180), (794, 228), (842, 248),
                         (794, 268), (774, 316), (754, 268),
                         (706, 248), (754, 228)]), fill="#13BCCA")
    canvas = canvas.resize((SIZE, SIZE), Image.Resampling.LANCZOS).convert("RGB")
    resources = ROOT / "resources"
    canvas.save(resources / "appicon.png", optimize=True)
    canvas.save(resources / "appicon.ico", format="ICO",
                sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
    canvas.save(resources / "appicon.icns", format="ICNS")
    for name in ("appicon.png", "appicon.ico"):
        with Image.open(resources / name) as image:
            assert image.convert("RGB").getpixel((0, 0)) == (255, 255, 255)
    print("Generated appicon.png, appicon.ico and appicon.icns with an opaque white background.")


if __name__ == "__main__":
    main()
