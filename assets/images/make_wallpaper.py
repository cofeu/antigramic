#!/usr/bin/env python3
"""Generate the bundled AntigramicOS portrait wallpaper."""

from pathlib import Path

from PIL import Image, ImageDraw


SIZE = (1080, 1920)
OUTPUT = Path(__file__).with_name("home.png")

image = Image.new("RGB", SIZE)
pixels = image.load()
top, bottom = (8, 15, 34), (31, 20, 70)
for y in range(SIZE[1]):
    t = y / (SIZE[1] - 1)
    color = tuple(round(a * (1 - t) + b * t) for a, b in zip(top, bottom))
    for x in range(SIZE[0]):
        pixels[x, y] = color

# Diffuse teal and violet light, kept mostly behind the clock area.
glow = Image.new("RGBA", SIZE, (0, 0, 0, 0))
for radius, alpha, color in ((520, 7, (64, 236, 221)), (390, 9, (92, 124, 255))):
    layer = Image.new("RGBA", SIZE, (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    cx, cy = 540, 980
    for r in range(radius, 0, -8):
        a = max(0, round(alpha * (1 - r / radius)))
        d.ellipse((cx-r, cy-r, cx+r, cy+r), fill=(*color, a))
    glow = Image.alpha_composite(glow, layer)

# Thin orbital arcs frame the clock without obscuring it.
orbit = Image.new("RGBA", SIZE, (0, 0, 0, 0))
od = ImageDraw.Draw(orbit)
od.ellipse((114, 760, 966, 1612), outline=(116, 198, 255, 30), width=3)
od.arc((158, 808, 922, 1572), 200, 330, fill=(103, 231, 217, 105), width=7)
od.arc((184, 834, 896, 1546), 20, 145, fill=(165, 119, 255, 92), width=5)
glow = Image.alpha_composite(glow, orbit)

# Layered flowing shapes add depth near the lower edge of the phone screen.
waves = Image.new("RGBA", SIZE, (0, 0, 0, 0))
wd = ImageDraw.Draw(waves)
wd.polygon([(0, 1470), (170, 1390), (340, 1460), (515, 1360), (710, 1450),
            (900, 1340), (1080, 1430), (1080, 1920), (0, 1920)], fill=(27, 76, 125, 120))
wd.polygon([(0, 1590), (210, 1500), (390, 1560), (585, 1460), (790, 1550),
            (960, 1475), (1080, 1525), (1080, 1920), (0, 1920)], fill=(41, 53, 126, 175))
wd.polygon([(0, 1740), (210, 1640), (405, 1715), (620, 1625), (835, 1720),
            (1010, 1630), (1080, 1665), (1080, 1920), (0, 1920)], fill=(18, 31, 75, 205))
glow = Image.alpha_composite(glow, waves)

# A sparse star field gives the upper half a little texture.
stars = Image.new("RGBA", SIZE, (0, 0, 0, 0))
sd = ImageDraw.Draw(stars)
for x, y, r, a in [(98,190,3,90),(892,245,2,105),(760,430,2,80),(250,610,2,70),
                    (970,780,3,65),(126,1010,2,80),(860,1110,2,75),(300,1290,2,60),
                    (1020,1200,2,70),(60,770,2,65),(680,210,2,75)]:
    sd.ellipse((x-r,y-r,x+r,y+r), fill=(224,245,255,a))
glow = Image.alpha_composite(glow, stars)
image = Image.alpha_composite(image.convert("RGBA"), glow).convert("RGB")
image.save(OUTPUT, format="PNG", optimize=True)
