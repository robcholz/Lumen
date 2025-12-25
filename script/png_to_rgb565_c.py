#!/usr/bin/env python3
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    raise SystemExit("Pillow is required: pip install pillow")


def rgb_to_565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def format_c_array(name, width, height, values):
    lines = [f"const uint16_t {name}[] = {{"]

    per_line = 12
    for i in range(0, len(values), per_line):
        chunk = values[i: i + per_line]
        line = ", ".join(f"0x{v:04X}" for v in chunk)
        lines.append(f"    {line},")
    lines.append("};")
    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Convert PNG to RGB565 C array.")
    parser.add_argument("input", type=Path, help="Input PNG path")
    parser.add_argument("--name", default="image_rgb565", help="C array base name")
    parser.add_argument("--out", type=Path, help="Output .h/.c file (default: stdout)")
    parser.add_argument("--resize", metavar="WxH", help="Optional resize, e.g. 128x128")
    args = parser.parse_args()

    image = Image.open(args.input).convert("RGB")
    if args.resize:
        if "x" not in args.resize:
            raise SystemExit("Invalid --resize format. Use WxH, e.g. 128x128")
        w_str, h_str = args.resize.split("x", 1)
        image = image.resize((int(w_str), int(h_str)), Image.LANCZOS)

    width, height = image.size
    pixels = list(image.getdata())
    values = [rgb_to_565(r, g, b) for (r, g, b) in pixels]

    output = format_c_array(args.name, width, height, values)

    if args.out:
        args.out.write_text(output, encoding="utf-8")
    else:
        print(output)


if __name__ == "__main__":
    main()
