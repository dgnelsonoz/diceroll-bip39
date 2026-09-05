#!/usr/bin/env python3
"""Generate an sFONT-compatible fixed-width bitmap font from a TrueType font."""

from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageFont


def generate(ttf_path: Path, output_path: Path, name: str, width: int, height: int) -> None:
    font = ImageFont.truetype(str(ttf_path), height)
    table = bytearray()

    for codepoint in range(0x20, 0x7F):
        character = chr(codepoint)
        native_width = max(16, width)
        image = Image.new("L", (native_width, height), 0)
        draw = ImageDraw.Draw(image)
        left, top, right, bottom = draw.textbbox((0, 0), character, font=font,
                                                 anchor="ls")
        glyph_width = right - left
        x = (native_width - glyph_width) // 2 - left
        # Use a common baseline. Individual glyph bounding boxes must not
        # determine their vertical position.
        baseline = height - font.getmetrics()[1]
        draw.text((x, baseline), character, font=font, fill=255, anchor="ls")
        image = image.resize((width, height), Image.Resampling.LANCZOS)

        bytes_per_row = (width + 7) // 8
        for row in range(height):
            for byte in range(bytes_per_row):
                value = 0
                for bit in range(8):
                    column = byte * 8 + bit
                    if column < width and image.getpixel((column, row)) >= 64:
                        value |= 0x80 >> bit
                table.append(value)

    with output_path.open("w", encoding="ascii") as output:
        output.write("#include \"fonts.h\"\n\n")
        output.write(f"const uint8_t {name}_Table[] = {{\n")
        for offset in range(0, len(table), 16):
            values = ", ".join(f"0x{value:02x}" for value in table[offset:offset + 16])
            output.write(f"    {values},\n")
        output.write("};\n\n")
        output.write(f"sFONT {name} = {{ {name}_Table, {width}, {height} }};\n")


if __name__ == "__main__":
    if len(sys.argv) != 6:
        raise SystemExit("usage: generate_bitmap_font.py TTF OUTPUT NAME WIDTH HEIGHT")
    generate(Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3],
             int(sys.argv[4]), int(sys.argv[5]))
