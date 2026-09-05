#!/usr/bin/env python3
"""Convert an 8-bit BDF bitmap font into an sFONT ASCII table."""

from pathlib import Path
import sys
from PIL import Image


def read_glyphs(path: Path):
    glyphs = {}
    block = None
    for line in path.read_text(encoding="latin-1").splitlines():
        if line.startswith( "STARTCHAR " ):
            block = {}
        elif line == "ENDCHAR" and block is not None:
            encoding = block.get("encoding", -1)
            if 0x20 <= encoding <= 0x7E:
                glyphs[encoding] = block
            block = None
        elif block is not None:
            fields = line.split()
            if fields and fields[0] == "ENCODING":
                block["encoding"] = int(fields[1])
            elif fields and fields[0] == "BBX":
                block["bbx"] = tuple(int(value) for value in fields[1:])
            elif fields and fields[0] == "BITMAP":
                block["bitmap"] = []
            elif "bitmap" in block and len(fields) == 1:
                block["bitmap"].append(int(fields[0], 16))
    return glyphs


def convert(source: Path, output: Path) -> None:
    glyphs = read_glyphs(source)
    table = []
    for encoding in range(0x20, 0x7F):
        glyph = glyphs[encoding]
        bbx_width, bbx_height, x_offset, y_offset = glyph["bbx"]
        bitmap = glyph["bitmap"]
        native_width = max(11, bbx_width)
        image = Image.new("1", (native_width, 16), 0)
        pixels = image.load()
        x_start = (11 - bbx_width) // 2 + x_offset
        # Spleen's 8x16 strike already contains its complete vertical
        # spacing. Do not apply the BDF baseline offset a second time.
        y_start = 0
        for row, value in enumerate(bitmap):
            target_y = y_start + row
            if 0 <= target_y < 16:
                for bit in range(bbx_width):
                    if value & (1 << (bbx_width - bit - 1)):
                        column = (native_width - bbx_width) // 2 + bit + x_offset
                        if 0 <= column < native_width:
                            pixels[column, target_y] = 1
        if native_width != 11:
            image = image.resize((11, 16), Image.Resampling.NEAREST)
        rows = [[0, 0] for _ in range(16)]
        for target_y in range(16):
            for column in range(11):
                if image.getpixel((column, target_y)):
                    rows[target_y][column // 8] |= 0x80 >> (column % 8)
        for row in rows:
            table.extend(row)

    with output.open("w", encoding="ascii") as out:
        out.write('#include "fonts.h"\n\nconst uint8_t Font16_Table[] = {\n')
        for offset in range(0, len(table), 16):
            out.write("    " + ", ".join(f"0x{v:02x}" for v in table[offset:offset + 16]) + ",\n")
        out.write("};\n\nsFONT Font16 = { Font16_Table, 11, 16 };\n")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: bdf_to_sfont.py INPUT.bdf OUTPUT.c")
    convert(Path(sys.argv[1]), Path(sys.argv[2]))
