#!/usr/bin/env python3
"""Create a conservative, hand-cleaned derivative of the Waveshare Font16."""

from pathlib import Path
import re


SOURCE = Path("waveshare/vendor/waveshare/fonts/font16.c")
OUTPUT = Path("waveshare/vendor/waveshare/fonts/font16_clean.c")
WIDTH = 11
HEIGHT = 16
BYTES_PER_ROW = 2
GLYPH_SIZE = HEIGHT * BYTES_PER_ROW


def set_pixel(glyph: list[list[int]], x: int, y: int, value: bool) -> None:
    if 0 <= x < WIDTH and 0 <= y < HEIGHT:
        if value:
            glyph[y][x // 8] |= 0x80 >> (x % 8)
        else:
            glyph[y][x // 8] &= ~(0x80 >> (x % 8))


source_text = SOURCE.read_text(encoding="ascii")
values = [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", source_text)]
assert len(values) == 95 * GLYPH_SIZE

for character in "ilrg":
    offset = (ord(character) - 0x20) * GLYPH_SIZE
    glyph = [values[offset + row * BYTES_PER_ROW:offset + (row + 1) * BYTES_PER_ROW]
             for row in range(HEIGHT)]

    if character in "il":
        # Retain a shorter centered terminal instead of the original heavy bar.
        for x in range(WIDTH):
            set_pixel(glyph, x, 10, x in (2, 3, 4, 5, 6, 7, 8))
    elif character == "r":
        # Simplify the curled shoulder and remove the baseline flourish.
        for x in range(WIDTH):
            set_pixel(glyph, x, 4, x in (1, 2, 3, 4))
            set_pixel(glyph, x, 5, x in (3, 4, 5))
            set_pixel(glyph, x, 10, False)
    elif character == "g":
        # Keep the bowl, but make the descender a clean narrow stem.
        for x in range(WIDTH):
            set_pixel(glyph, x, 13, x in (6, 7))

    for row in range(HEIGHT):
        values[offset + row * BYTES_PER_ROW:offset + (row + 1) * BYTES_PER_ROW] = glyph[row]

with OUTPUT.open("w", encoding="ascii") as output:
    output.write('#include "fonts.h"\n\n')
    output.write("const uint8_t Font16_Table[] = {\n")
    for offset in range(0, len(values), 16):
        output.write("    " + ", ".join(f"0x{value:02x}" for value in values[offset:offset + 16]) + ",\n")
    output.write("};\n\n")
    output.write("sFONT Font16 = { Font16_Table, 11, 16 };\n")
