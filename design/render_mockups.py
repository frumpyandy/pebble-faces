"""Render mockups of TTMM-inspired watchface concepts for Pebble Round 2 (260x260).

Pure-Python preview of the designs implemented in C under ../faces/.
Usage: python3 render_mockups.py [out_dir]
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

W = 260          # Pebble Round 2 screen size (platform "gabbro")
S = 4            # supersampling factor for the preview
CX = CY = W // 2

BG = (0, 0, 0)
FG = (255, 255, 255)
DIM = (85, 85, 85)          # GColorDarkGray
ACCENT = (255, 170, 0)      # GColorChromeYellow
ACCENT2 = (255, 0, 0)       # GColorRed

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"


def polar(r, deg, cx=CX, cy=CY):
    """Point at radius r, angle deg clockwise from 12 o'clock (Pebble convention)."""
    a = math.radians(deg - 90)
    return (cx + r * math.cos(a), cy + r * math.sin(a))


class Canvas:
    def __init__(self):
        self.im = Image.new("RGB", (W * S, W * S), BG)
        self.d = ImageDraw.Draw(self.im)

    def dot(self, p, r, color):
        x, y = p
        self.d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S], fill=color)

    def ring(self, p, r, width, color):
        x, y = p
        self.d.ellipse([(x - r) * S, (y - r) * S, (x + r) * S, (y + r) * S],
                       outline=color, width=max(1, int(width * S)))

    def line(self, p1, p2, width, color):
        self.d.line([(p1[0] * S, p1[1] * S), (p2[0] * S, p2[1] * S)],
                    fill=color, width=max(1, int(width * S)))

    def tick(self, r_in, r_out, deg, width, color):
        self.line(polar(r_in, deg), polar(r_out, deg), width, color)

    def arc_band(self, r_out, thickness, deg_start, deg_end, color):
        """Equivalent of graphics_fill_radial: filled band between two angles."""
        box = [(CX - r_out) * S, (CY - r_out) * S, (CX + r_out) * S, (CY + r_out) * S]
        self.d.pieslice(box, deg_start - 90, deg_end - 90, fill=color)
        r_in = r_out - thickness
        box = [(CX - r_in) * S, (CY - r_in) * S, (CX + r_in) * S, (CY + r_in) * S]
        self.d.ellipse(box, fill=BG)

    def rect(self, x0, y0, x1, y1, color):
        self.d.rectangle([x0 * S, y0 * S, x1 * S, y1 * S], fill=color)

    def text(self, s, size, center, color, font_path=FONT_PATH):
        font = ImageFont.truetype(font_path, int(size * S))
        self.d.text((center[0] * S, center[1] * S), s, fill=color, font=font, anchor="mm")

    def finish(self):
        # round mask
        mask = Image.new("L", (W * S, W * S), 0)
        ImageDraw.Draw(mask).ellipse([0, 0, W * S - 1, W * S - 1], fill=255)
        out = Image.new("RGB", (W * S, W * S), (24, 24, 24))
        out.paste(self.im, (0, 0), mask)
        return out.resize((W * 2, W * 2), Image.LANCZOS)


# ---------------------------------------------------------------- digits
# 3x5 dot-matrix digits, the "count the dots" feel of TTMM.
DIGITS = {
    "0": ["###", "#.#", "#.#", "#.#", "###"],
    "1": [".#.", "##.", ".#.", ".#.", "###"],
    "2": ["###", "..#", "###", "#..", "###"],
    "3": ["###", "..#", "###", "..#", "###"],
    "4": ["#.#", "#.#", "###", "..#", "..#"],
    "5": ["###", "#..", "###", "..#", "###"],
    "6": ["###", "#..", "###", "#.#", "###"],
    "7": ["###", "..#", "..#", "..#", "..#"],
    "8": ["###", "#.#", "###", "#.#", "###"],
    "9": ["###", "#.#", "###", "..#", "###"],
}


def dot_digit(c, ch, x0, y0, pitch, r, color, shape="dot"):
    rows = DIGITS[ch]
    for j, row in enumerate(rows):
        for i, cell in enumerate(row):
            if cell == "#":
                x, y = x0 + i * pitch, y0 + j * pitch
                if shape == "dot":
                    c.dot((x, y), r, color)
                else:
                    c.rect(x - r, y - r, x + r, y + r, color)


def dot_number(c, n, cx, cy, pitch, r, color, gap=None, shape="dot", pad=False):
    s = f"{n:02d}" if pad else str(n)
    gap = gap or pitch * 1.3
    dw = 2 * pitch
    total = len(s) * dw + (len(s) - 1) * gap
    x0 = cx - total / 2
    y0 = cy - 2 * pitch
    for ch in s:
        dot_digit(c, ch, x0, y0, pitch, r, color, shape)
        x0 += dw + gap


# ---------------------------------------------------------------- faces
def face_beads(h, m):
    """Hands-less analog: hour = big bead on outer ring, minute = small accent bead on inner ring."""
    c = Canvas()
    R_H, R_M = 112, 84
    for i in range(60):
        if i % 5 == 0:
            c.tick(R_H + 8, R_H + 14, i * 6, 3, DIM)
        else:
            c.dot(polar(R_H + 11, i * 6), 1.2, DIM)
    c.ring((CX, CY), R_M, 1.5, DIM)
    hour_deg = (h % 12) * 30 + m * 0.5
    c.dot(polar(R_H, hour_deg), 14, FG)
    c.dot(polar(R_M, m * 6), 8, ACCENT)
    return c.finish()


def face_tally(h, m):
    """60 minute dots around the rim fill up as the hour passes; hour as dot-matrix numeral."""
    c = Canvas()
    R = 117
    for i in range(60):
        big = i % 5 == 0
        r = 3.6 if big else 2.4
        if i < m:
            col = ACCENT if big else FG
        elif i == m:
            col = ACCENT
            r = 4.5
        else:
            col = DIM
        c.dot(polar(R, i * 6), r, col)
    hh = h % 12 or 12
    dot_number(c, hh, CX, CY, pitch=24, r=9.5, color=FG)
    return c.finish()


def face_ninefold(h, m):
    """Homage to TIC-9TTMM: six 3x3 dot squares = 6 x 10 minutes, arranged as a wheel."""
    c = Canvas()
    R = 92
    pitch, r = 11, 3.6
    for block in range(6):
        deg = block * 60 + 30      # blocks sit between the even hours
        bx, by = polar(R, deg)
        # dot k of the block represents minute block*10 + k + 1
        complete = m >= (block + 1) * 10
        for k in range(9):
            i, j = k % 3, k // 3
            x = bx + (i - 1) * pitch
            y = by + (j - 1) * pitch
            minute_val = block * 10 + k + 1
            on = minute_val <= m
            col = ACCENT if complete else (FG if on else DIM)
            c.dot((x, y), r if on else 1.6, col)
    hh = h % 12 or 12
    c.text(str(hh), 72, (CX, CY - 2), FG)
    return c.finish()


def face_sweep(h, m):
    """Minute as a radial sweep in the outer band; 12 hour ticks; big hour numeral."""
    c = Canvas()
    R_OUT, T = 128, 16
    c.arc_band(R_OUT, T, 0, 360, (40, 40, 40))
    if m > 0:
        c.arc_band(R_OUT, T, 0, m * 6, ACCENT)
    for i in range(12):
        hh12 = h % 12
        col = FG if i == hh12 else DIM
        c.tick(R_OUT - T - 6, R_OUT - T - 18 if i == hh12 else R_OUT - T - 12, i * 30, 4 if i == hh12 else 2, col)
    hh = h % 12 or 12
    c.text(str(hh), 96, (CX, CY - 4), FG)
    return c.finish()


def face_horizon(h, m):
    """A horizon line descends through the hour as minutes pass; below it the face is inverted."""
    c = Canvas()
    y = 20 + (W - 40) * m / 59.0
    c.rect(0, 0, W, W, FG)
    c.rect(0, y, W, W, BG)
    hh = h % 12 or 12
    # numeral drawn twice, clipped by the horizon: dark above, light below
    top = Image.new("RGB", (W * S, W * S), FG)
    ImageDraw.Draw(top)
    font = ImageFont.truetype(FONT_PATH, int(104 * S))
    img_fg = Image.new("RGB", (W * S, W * S), FG)
    ImageDraw.Draw(img_fg).text((CX * S, (CY - 4) * S), str(hh), fill=BG, font=font, anchor="mm")
    img_bg = Image.new("RGB", (W * S, W * S), BG)
    ImageDraw.Draw(img_bg).text((CX * S, (CY - 4) * S), str(hh), fill=FG, font=font, anchor="mm")
    c.im.paste(img_fg.crop((0, 0, W * S, int(y * S))), (0, 0))
    c.im.paste(img_bg.crop((0, int(y * S), W * S, W * S)), (0, int(y * S)))
    c.d = ImageDraw.Draw(c.im)
    c.line((0, y), (W, y), 3, ACCENT)
    return c.finish()


FACES = {
    "beads": face_beads,
    "tally": face_tally,
    "ninefold": face_ninefold,
    "sweep": face_sweep,
    "horizon": face_horizon,
}

TIMES = [(10, 8), (4, 47), (7, 30)]


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out, exist_ok=True)
    tile = W * 2
    pad = 16
    sheet = Image.new("RGB", (len(TIMES) * (tile + pad) + pad, len(FACES) * (tile + pad) + pad + 30), (24, 24, 24))
    label_font = ImageFont.truetype(FONT_PATH, 22)
    sd = ImageDraw.Draw(sheet)
    for row, (name, fn) in enumerate(FACES.items()):
        for col, (h, m) in enumerate(TIMES):
            img = fn(h, m)
            img.save(os.path.join(out, f"{name}_{h:02d}{m:02d}.png"))
            x = pad + col * (tile + pad)
            y = pad + 30 + row * (tile + pad)
            sheet.paste(img, (x, y))
            sd.text((x + 8, y + 4), f"{name}  {h}:{m:02d}", fill=(200, 200, 200), font=label_font)
    sheet.save(os.path.join(out, "contact_sheet.png"))
    print("wrote", os.path.join(out, "contact_sheet.png"))


if __name__ == "__main__":
    main()
