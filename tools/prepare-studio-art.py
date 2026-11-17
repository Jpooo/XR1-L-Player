#!/usr/bin/env python3
"""Export the original XR1 Lite art and a neutral lighting mask for SDL."""

import struct
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter


def save_rgba_bmp(image, path):
    width, height = image.size
    pixels = image.tobytes("raw", "BGRA")
    bitmap = struct.pack("<IiiHHIIiiII", 108, width, -height, 1, 32, 3,
                         len(pixels), 2835, 2835, 0, 0)
    bitmap += struct.pack("<IIIII", 0xff0000, 0xff00, 0xff, 0xff000000, 0x73524742)
    bitmap += bytes(48)
    header = b"BM" + struct.pack("<IHHI", 122 + len(pixels), 0, 0, 122)
    path.write_bytes(header + bitmap + pixels)


def main():
    assets = Path(__file__).resolve().parents[1] / "assets"
    studio = assets / "studio"
    studio.mkdir(exist_ok=True)
    with Image.open(assets / "branding" / "xr1-lite-master.png") as source:
        device = source.convert("RGBA")
    save_rgba_bmp(device, studio / "device.bmp")

    width, height = 640, 480
    silhouette = Image.new("L", (width, height))
    alpha = device.getchannel("A").resize((480, 320), Image.Resampling.LANCZOS)
    silhouette.paste(alpha, (80, 80))
    plate = Image.new("L", silhouette.size)
    ImageDraw.Draw(plate).rounded_rectangle((118, 131, 523, 386), radius=46, fill=255)
    gradient = Image.new("L", silhouette.size)
    gradient.putdata([
        round(255 * max(.13, .64 - .48 * (.62 * x / width + .55 * y / height)))
        for y in range(height) for x in range(width)
    ])
    lighting = ImageChops.multiply(plate, gradient)
    for radius, strength in ((2, .22), (13, .16), (28, .13)):
        shadow = silhouette.filter(ImageFilter.GaussianBlur(radius))
        lighting = ImageChops.screen(lighting, shadow.point(lambda value: round(value * strength)))
    mask = Image.new("RGBA", silhouette.size, "white")
    mask.putalpha(lighting)
    save_rgba_bmp(mask, studio / "device-light.bmp")


if __name__ == "__main__":
    main()
