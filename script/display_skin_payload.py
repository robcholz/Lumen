#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required: pip install pillow") from exc


def rgb565_to_rgb888_be(data: bytes, width: int, height: int) -> Image.Image:
    expected = width * height * 2
    if len(data) < expected:
        raise ValueError(f"payload too small: {len(data)} < {expected}")
    if len(data) > expected:
        data = data[:expected]

    rgb = bytearray(width * height * 3)
    dst = 0
    for i in range(0, expected, 2):
        value = (data[i] << 8) | data[i + 1]  # big-endian RGB565
        r = (value >> 11) & 0x1F
        g = (value >> 5) & 0x3F
        b = value & 0x1F
        rgb[dst] = (r << 3) | (r >> 2)
        rgb[dst + 1] = (g << 2) | (g >> 4)
        rgb[dst + 2] = (b << 3) | (b >> 2)
        dst += 3

    return Image.frombytes("RGB", (width, height), bytes(rgb))


def main() -> int:
    parser = argparse.ArgumentParser(description="Display an RGB565 big-endian payload.")
    parser.add_argument("file", help="Binary payload file")
    parser.add_argument("--width", type=int, required=True, help="Image width")
    parser.add_argument("--height", type=int, required=True, help="Image height")
    parser.add_argument("--out", help="Output PNG path (optional)")
    args = parser.parse_args()

    path = Path(args.file)
    if not path.exists():
        print(f"file not found: {path}", file=sys.stderr)
        return 2

    blob = path.read_bytes()
    data = blob[4:]  # skip width/height header
    image = rgb565_to_rgb888_be(data, args.width, args.height)

    if args.out:
        image.save(args.out)
    else:
        image.show()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
