#!/usr/bin/env python3
"""Draw the icon options as 32x32 and 16x16 pixel art and write .ico files
plus an enlarged preview sheet."""
import os
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "icons")
os.makedirs(OUT, exist_ok=True)

BLK = (0, 0, 0, 255); WHT = (255, 255, 255, 255); GRY = (192, 192, 192, 255); DGR = (128, 128, 128, 255)
BEI = (222, 214, 186, 255); DBE = (170, 160, 130, 255); NAV = (0, 0, 128, 255); TEA = (0, 128, 128, 255)
GRN = (0, 200, 0, 255); YEL = (255, 220, 0, 255); RED = (230, 0, 0, 255); BLU = (0, 90, 220, 255)
ORG = (255, 140, 0, 255); CLR = (0, 0, 0, 0)


def gauge(d, cx, cy, r, needle_to):
    # coloured arc in three bands, then the needle
    for start, end, col in ((180, 240, GRN), (240, 300, YEL), (300, 360, RED)):
        d.pieslice([cx - r, cy - r, cx + r, cy + r], start, end, fill=col)
    d.pieslice([cx - r + 3, cy - r + 3, cx + r - 3, cy + r - 3], 180, 360, fill=BLK)
    d.line([cx, cy, needle_to[0], needle_to[1]], fill=WHT, width=1)
    d.rectangle([cx - 1, cy - 1, cx + 1, cy], fill=WHT)


def crt_gauge():
    im = Image.new("RGBA", (32, 32), CLR); d = ImageDraw.Draw(im)
    d.rectangle([2, 2, 29, 23], fill=BEI, outline=BLK)
    d.line([3, 3, 28, 3], fill=WHT); d.line([3, 3, 3, 22], fill=WHT)
    d.rectangle([5, 5, 26, 19], fill=BLK)
    gauge(d, 15, 17, 9, (21, 10))
    d.rectangle([24, 20, 26, 21], fill=GRN)
    d.polygon([(10, 24), (21, 24), (24, 28), (7, 28)], fill=DBE, outline=BLK)
    d.rectangle([5, 28, 26, 29], fill=BEI, outline=BLK)
    return im


def speedo():
    im = Image.new("RGBA", (32, 32), CLR); d = ImageDraw.Draw(im)
    d.ellipse([1, 3, 30, 30], fill=GRY, outline=BLK)
    d.ellipse([3, 5, 28, 28], fill=NAV)
    for start, end, col in ((150, 210, GRN), (210, 270, YEL), (270, 330, ORG), (330, 390, RED)):
        d.arc([5, 7, 26, 26], start, end, fill=col, width=3)
    d.line([16, 17, 23, 10], fill=WHT, width=2)
    d.ellipse([14, 15, 18, 19], fill=RED, outline=BLK)
    d.rectangle([11, 22, 20, 25], fill=BLK); d.rectangle([12, 23, 19, 24], fill=GRN)
    return im


def chip_bolt():
    im = Image.new("RGBA", (32, 32), CLR); d = ImageDraw.Draw(im)
    for i in range(6):
        x = 7 + i * 4
        d.rectangle([x, 1, x + 1, 5], fill=GRY); d.rectangle([x, 26, x + 1, 30], fill=GRY)
        d.rectangle([1, x, 5, x + 1], fill=GRY); d.rectangle([26, x, 30, x + 1], fill=GRY)
    d.rectangle([5, 5, 26, 26], fill=(40, 40, 48, 255), outline=BLK)
    d.rectangle([8, 8, 23, 23], fill=DGR, outline=(70, 70, 80, 255))
    d.polygon([(18, 6), (10, 17), (15, 17), (12, 26), (22, 13), (16, 13), (19, 6)], fill=YEL, outline=ORG)
    return im


def bars_window():
    im = Image.new("RGBA", (32, 32), CLR); d = ImageDraw.Draw(im)
    d.rectangle([1, 2, 30, 29], fill=GRY, outline=BLK)
    d.line([2, 3, 29, 3], fill=WHT); d.line([2, 3, 2, 28], fill=WHT)
    d.rectangle([3, 4, 28, 8], fill=NAV)
    d.rectangle([24, 5, 27, 7], fill=GRY)
    d.rectangle([4, 10, 27, 27], fill=WHT, outline=DGR)
    for i, (h, col) in enumerate(((5, RED), (9, YEL), (12, GRN), (15, BLU))):
        x = 6 + i * 5
        d.rectangle([x, 26 - h, x + 3, 26], fill=col, outline=BLK)
    return im


def scale16(im):
    return im.resize((16, 16), Image.LANCZOS)


ICONS = [("a_crt_gauge", "A  CRT with gauge", crt_gauge()),
         ("b_speedometer", "B  Speedometer", speedo()),
         ("c_chip_bolt", "C  CPU and bolt", chip_bolt()),
         ("d_bar_window", "D  Window with bars", bars_window())]

sheet = Image.new("RGB", (4 * 200, 260), (0, 128, 128))  # teal desktop
dr = ImageDraw.Draw(sheet)
for i, (key, label, im) in enumerate(ICONS):
    im.save(os.path.join(OUT, key + ".ico"), sizes=[(32, 32), (16, 16)],
            append_images=[scale16(im)])
    big = im.resize((160, 160), Image.NEAREST)
    x = i * 200 + 20
    sheet.paste((192, 192, 192), [x - 4, 16, x + 164, 180])
    sheet.paste(big, (x, 20), big)
    # actual size and 16x16 as Windows would show them
    sheet.paste(im, (x + 20, 200), im)
    s16 = scale16(im); sheet.paste(s16, (x + 70, 208), s16)
    dr.text((x, 236), label, fill=(255, 255, 255))
sheet.save(os.path.join(OUT, "icon-options.png"))
print("ok")
