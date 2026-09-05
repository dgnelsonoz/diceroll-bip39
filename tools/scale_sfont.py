#!/usr/bin/env python3
from pathlib import Path
import re
from PIL import Image
import sys

source, output, name, width, height = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
values = [int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]{2})", source.read_text())]
table = []
for index in range(95):
    old = Image.new("1", (11, 16), 0)
    pixels = old.load(); start = index * 32
    for y in range(16):
        for x in range(11):
            pixels[x, y] = bool(values[start + y * 2 + x // 8] & (0x80 >> (x % 8)))
    image = old.resize((width, height), Image.Resampling.NEAREST)
    for y in range(height):
        row = [0] * ((width + 7) // 8)
        for x in range(width):
            if image.getpixel((x, y)):
                row[x // 8] |= 0x80 >> (x % 8)
        table.extend(row)
with output.open("w") as out:
    out.write('#include "fonts.h"\n\nconst uint8_t ' + name + '_Table[] = {\n')
    for i in range(0, len(table), 16): out.write('    ' + ', '.join(f'0x{v:02x}' for v in table[i:i+16]) + ',\n')
    out.write('};\n\nsFONT ' + name + ' = { ' + name + '_Table, ' + str(width) + ', ' + str(height) + ' };\n')
