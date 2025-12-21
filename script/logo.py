import argparse

from PIL import Image

parser = argparse.ArgumentParser()
parser.add_argument("input", help="input PNG file")
parser.add_argument("-o", "--output", default="output.png", help="output file")
parser.add_argument("-i", "--inverted", action="store_true", help="invert the image")
args = parser.parse_args()

img = Image.open(args.input).convert("RGB")
pixels = img.load()

width, height = img.size

for y in range(height):
    for x in range(width):
        r, g, b = pixels[x, y]

        if (r, g, b) != (255, 255, 255):
            pixels[x, y] = (0, 0, 0)
        else:
            pixels[x, y] = (255, 255, 255)

img.save(args.output)

print("Done! Saved as", args.output)
