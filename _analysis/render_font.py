# -*- coding: utf-8 -*-
"""
Parse Hardware/font.c and render text exactly the way LCD_ShowChar() /
LCD_ShowStringUTF8() does (8x16 cell, MSB = left-most pixel, fixed 8 px advance),
so the on-screen result can be inspected off-target.

Run:  python render_font.py
Out:  lcd_ascii_preview.png, line_*.png next to this script
"""
import os
import re
from collections import Counter

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Hardware", "font.c")
OUT = os.path.dirname(os.path.abspath(__file__))

text = open(SRC, "r", encoding="utf-8", errors="replace").read()

start = text.index("{", text.index("ASCII8x16["))
depth = 0
for i in range(start, len(text)):
    if text[i] == "{":
        depth += 1
    elif text[i] == "}":
        depth -= 1
        if depth == 0:
            end = i
            break
body = text[start + 1:end]

rows = re.findall(r"\{([^{}]*)\}", body)
assert len(rows) == 95, len(rows)

GLYPHS = []
for r in rows:
    vals = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]{2})", r)]
    assert len(vals) == 16, (len(vals), r)
    GLYPHS.append(vals)


def cell(ch, lsb_first=False):
    idx = ord(ch) - 0x20
    if not (0 <= idx < 95):
        idx = ord("?") - 0x20
    out = []
    for row in GLYPHS[idx]:
        out.append([(row >> (c if lsb_first else (7 - c))) & 1 for c in range(8)])
    return out


def render_line(s, scale=1, lsb_first=False):
    img = Image.new("RGB", (8 * len(s) * scale, 16 * scale), (255, 255, 255))
    px = img.load()
    for i, ch in enumerate(s):
        c = cell(ch, lsb_first)
        for y in range(16):
            for x in range(8):
                if c[y][x]:
                    for dy in range(scale):
                        for dx in range(scale):
                            px[i * 8 * scale + x * scale + dx, y * scale + dy] = (0, 0, 0)
    return img


def art(ch, lsb_first=False):
    c = cell(ch, lsb_first)
    print("  +--------+")
    for y in range(16):
        print("  |%s|" % "".join("#" if v else "." for v in c[y]))
    print("  +--------+")


print("=" * 84)
print("PER-GLYPH INK BOX  (cell is 8 wide x 16 tall; driver advances exactly 8 px)")
print("=" * 84)
print("code  ch   cols used        ink_w  rows          ink_h  left  right")
stats = []
for idx in range(95):
    ch = chr(0x20 + idx)
    c = cell(ch)
    cols = [x for x in range(8) if any(c[y][x] for y in range(16))]
    rws = [y for y in range(16) if any(c[y][x] for x in range(8))]
    if not cols:
        stats.append((ch, None))
        continue
    stats.append((ch, (min(cols), max(cols), min(rws), max(rws))))
    print("0x%02X  %-4s %-16s %2d     %2d..%2d        %2d     %d     %d"
          % (0x20 + idx, repr(ch)[1:-1].ljust(3), str(cols).ljust(16),
             max(cols) - min(cols) + 1, min(rws), max(rws),
             max(rws) - min(rws) + 1, min(cols), 7 - max(cols)))

print()
print("=" * 84)
print("SUMMARY")
print("=" * 84)
ink_h = [b[3] - b[2] + 1 for _, b in stats if b]
tops = Counter(b[2] for _, b in stats if b)
bots = Counter(b[3] for _, b in stats if b)
lefts = Counter(b[0] for _, b in stats if b)
rights = Counter(b[1] for _, b in stats if b)
print("ink height   : min=%d max=%d   -> %d px of variation inside one 16 px cell"
      % (min(ink_h), max(ink_h), max(ink_h) - min(ink_h)))
print("ink height hist :", sorted(Counter(ink_h).items()))
print("top row hist    :", sorted(tops.items()))
print("bottom row hist :", sorted(bots.items()))
print("left  col hist  :", sorted(lefts.items()))
print("right col hist  :", sorted(rights.items()))

full = [ch for ch, b in stats if b and b[3] - b[2] + 1 >= 14]
print("\nglyphs with 14+ px of ink (descenders) :", "".join(full))
tall = [ch for ch, b in stats if b and b[2] <= 2]
print("glyphs starting at row <= 2            :", "".join(tall))
low = [ch for ch, b in stats if b and b[2] >= 4]
print("glyphs starting at row >= 4            :", "".join(low))

print()
print("=" * 84)
print("DETAIL: the characters that make the jaggedness obvious")
print("=" * 84)
for ch in "17J2ivwmWlMitfQgjpqy_":
    b = dict((c, v) for c, v in stats)[ch]
    print("\n'%s'  cols %d..%d  rows %d..%d" % (ch, b[0], b[1], b[2], b[3]))
    art(ch)

print()
print("=" * 84)
print("BIT-ORDER CROSS-CHECK (must stay MSB-first, otherwise glyphs mirror)")
print("=" * 84)
for ch in "JULS2":
    print("\n'%s' interpreted LSB-first (WRONG):" % ch)
    art(ch, lsb_first=True)

samples = [
    "Hello, ",
    "POWERED BY",
    "STM32F103C8T6",
    "Distance:",
    "0123456789",
    "abcdefghijklm",
    "nopqrstuvwxyz",
    "ABCDEFGHIJKLM",
    "NOPQRSTUVWXYZ",
    '!"#$%&\'()*+,-./',
    ":;<=>?@[\\]^_`{|}~",
]

SCALE = 4
GAP = 8
sheet = Image.new("RGB", (8 * 14 * SCALE, (16 * SCALE + GAP) * len(samples)),
                  (210, 210, 235))
y = 0
for s in samples:
    sheet.paste(render_line(s, SCALE), (0, y))
    y += 16 * SCALE + GAP
sheet.save(os.path.join(OUT, "lcd_ascii_preview.png"))

for i, s in enumerate(["Hello, ", "POWERED BY", "STM32F103C8T6", "Distance:"]):
    render_line(s, 6).save(os.path.join(OUT, "line_%d.png" % i))

# same table, each glyph sliced to its own ink box -> what a proportional
# renderer with a real advance table would show
def render_proportional(s, scale=6):
    cw = []
    for ch in s:
        c = cell(ch)
        cols = [x for x in range(8) if any(c[y][x] for y in range(16))]
        cw.append((c, cols))
    total = sum((max(c) - min(c) + 2) if c else 3 for c, c in cw) * scale
    img = Image.new("RGB", (max(total, 8), 16 * scale), (255, 255, 255))
    px = img.load()
    ox = 0
    for c, cols in cw:
        if not cols:
            ox += 3 * scale
            continue
        for y in range(16):
            for x in range(min(cols), max(cols) + 1):
                if c[y][x]:
                    for dy in range(scale):
                        for dx in range(scale):
                            px[ox + (x - min(cols)) * scale + dx, y * scale + dy] = (0, 0, 0)
        ox += (max(cols) - min(cols) + 2) * scale
    return img


render_proportional("POWERED BY").save(os.path.join(OUT, "proportional_POWERED_BY.png"))
print("\npreviews written to", OUT)
