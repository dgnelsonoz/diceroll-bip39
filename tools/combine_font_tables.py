#!/usr/bin/env python3
from pathlib import Path
import re

inputs = [
    ("Font16Original", "waveshare/vendor/waveshare/fonts/font16.c"),
    ("Font16TerminusVga", "waveshare/vendor/waveshare/fonts/font16_terminus_vga_bold.c"),
    ("Font16Unifont", "waveshare/vendor/waveshare/fonts/font16_unifont.c"),
    ("Font16Terminus", "waveshare/vendor/waveshare/fonts/font16_terminus.c"),
]
with Path("waveshare/vendor/waveshare/fonts/font16_compare.c").open("w") as out:
    out.write('#include "fonts.h"\n\n')
    for name, filename in inputs:
        text = Path(filename).read_text(encoding="ascii")
        values = [int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]{2})", text)]
        out.write(f"static const uint8_t {name}_Table[] = {{\n")
        for offset in range(0, len(values), 16):
            out.write("    " + ", ".join(f"0x{v:02x}" for v in values[offset:offset + 16]) + ",\n")
        out.write("};\n\n")
        out.write(f"const sFONT {name} = {{ {name}_Table, 11, 16 }};\n\n")
    out.write("const sFONT Font16Default = { Font16Original_Table, 11, 16 };\n")
