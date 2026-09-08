"""Generate original transparent coastal inventory icons (requires Pillow)."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "art/ui/icons"
SCALE = 4


def canvas():
    image = Image.new("RGBA", (256 * SCALE, 256 * SCALE))
    return image, ImageDraw.Draw(image)


def box(draw, bounds, fill, radius=0, outline=None, width=1):
    draw.rounded_rectangle(tuple(int(v * SCALE) for v in bounds),
                           radius=radius * SCALE, fill=fill, outline=outline,
                           width=width * SCALE)


def line(draw, points, fill, width):
    draw.line([(x * SCALE, y * SCALE) for x, y in points], fill=fill,
              width=width * SCALE, joint="curve")


def battery():
    image, draw = canvas()
    box(draw, (82, 24, 108, 48), "#d6e0da", 5)
    box(draw, (147, 28, 175, 48), "#d6e0da", 5)
    box(draw, (59, 43, 197, 229), "#e9e4cd", 16)
    box(draw, (65, 49, 191, 99), "#537a79", 10)
    box(draw, (65, 87, 191, 99), "#537a79")
    box(draw, (67, 108, 189, 220), "#244b52", 8)
    line(draw, [(83, 72), (101, 72)], "#f5f1df", 5)
    line(draw, [(92, 63), (92, 81)], "#f5f1df", 5)
    line(draw, [(153, 72), (171, 72)], "#f5f1df", 5)
    draw.polygon([(int(x*SCALE), int(y*SCALE)) for x, y in
                  [(135, 122), (100, 170), (124, 170), (115, 204),
                   (159, 153), (134, 153)]], fill="#e9b86b")
    return image


def fuse():
    image, draw = canvas()
    box(draw, (37, 96, 219, 161), "#92b7b0", 11, "#e3ecdf", 4)
    box(draw, (67, 104, 189, 153), "#345960", 3)
    line(draw, [(68, 129), (90, 129), (102, 118), (115, 141),
                (128, 117), (141, 140), (153, 128), (188, 128)], "#f4bf6e", 5)
    line(draw, [(80, 110), (172, 110)], "#c3dfd7", 3)
    for left in (28, 188):
        box(draw, (left, 90, left+40, 167), "#e9e4cd", 8)
        box(draw, (left+7, 94, left+14, 163), "#fff8e1", 3)
        line(draw, [(left+32, 101), (left+32, 156)], "#8fa5a2", 3)
    return image.rotate(34, Image.Resampling.BICUBIC)


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    for name, make in (("T_RadioBattery", battery), ("T_MarineFuse", fuse)):
        path = OUT / (name + ".png")
        make().resize((256, 256), Image.Resampling.LANCZOS).save(path)
        print(path)
