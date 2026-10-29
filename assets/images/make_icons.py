"""Render the six launcher icons as one transparent PNG sprite sheet."""
from pathlib import Path
from PIL import Image, ImageDraw

OUTPUT = Path(__file__).with_name("app-icons.png")
NAMES = ["phone", "messages", "browser", "settings", "files", "music"]
COLORS = [(47, 204, 145), (78, 129, 255), (255, 163, 76),
          (158, 105, 255), (60, 207, 218), (245, 83, 151)]
sheet = Image.new("RGBA", (768, 512), (0, 0, 0, 0))

for i, name in enumerate(NAMES):
    x0, y0 = (i % 3) * 256, (i // 3) * 256
    base = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    mask = Image.new("L", (256, 256), 0)
    ImageDraw.Draw(mask).rounded_rectangle((8, 8, 248, 248), radius=70, fill=255)
    grad = Image.new("RGBA", (256, 256))
    gp = grad.load()
    c = COLORS[i]
    for y in range(256):
        for x in range(256):
            t = (x * .25 + y * .75) / 255
            gp[x, y] = tuple(int(v * (1-t) + hi * t) for v, hi in zip(c, (min(255,c[0]+45),min(255,c[1]+35),min(255,c[2]+42)))) + (255,)
    base.paste(grad, (0, 0), mask)
    d = ImageDraw.Draw(base)
    white = (250, 252, 255, 255)
    if name == "phone":
        d.line([(92,55),(70,77),(77,112),(111,151),(150,181),(181,187),(203,164)], fill=white, width=25, joint="curve")
        d.rounded_rectangle((65,48,103,91), radius=13, fill=white)
        d.rounded_rectangle((168,154,208,195), radius=13, fill=white)
    elif name == "messages":
        d.rounded_rectangle((42,54,214,174), radius=34, fill=white)
        d.polygon([(78,164),(70,206),(119,174)], fill=white)
        for x in (85,120,155): d.ellipse((x-7,104,x+7,118), fill=COLORS[i])
    elif name == "browser":
        d.ellipse((48,48,208,208), outline=white, width=17)
        d.ellipse((90,48,166,208), outline=white, width=12)
        d.arc((54,79,202,177), 0, 180, fill=white, width=12)
        d.arc((54,79,202,177), 180, 360, fill=white, width=12)
        d.line((52,128,204,128), fill=white, width=12)
    elif name == "settings":
        import math
        pts=[]
        for k in range(32):
            angle=2*math.pi*k/32-math.pi/2
            radius=88 if k%4 in (0,1) else 68
            pts.append((128+radius*math.cos(angle),128+radius*math.sin(angle)))
        d.polygon(pts, fill=white)
        d.ellipse((98,98,158,158), fill=COLORS[i])
    elif name == "files":
        d.rounded_rectangle((43,75,137,119), radius=15, fill=white)
        d.rounded_rectangle((35,98,221,192), radius=20, fill=white)
        d.rounded_rectangle((48,116,208,179), radius=12, fill=(227,246,255,255))
    else:
        d.ellipse((68,67,188,187), outline=white, width=15)
        d.ellipse((102,101,154,153), fill=white)
        d.rounded_rectangle((169,154,205,201), radius=13, fill=white)
    sheet.alpha_composite(base, (x0, y0))

sheet.save(OUTPUT, "PNG", optimize=True)
