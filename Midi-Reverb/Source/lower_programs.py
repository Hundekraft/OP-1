"""Specialize MIDIVerb_RE -O 7 output for a reentrant mono Blackfin tank."""
import hashlib
import re
import sys
import zlib
from pathlib import Path

generated, rom, out = map(Path, sys.argv[1:])
data = rom.read_bytes()
assert len(data) == 16384 and hashlib.md5(data).hexdigest() == "11a460c8e64d3325411bba0c11a2ae49"
result = ["/* Mono native routines derived from MIDIVerb_RE mv_gencode -O 7. */"]
result.append(f"#define MV1_ROM_CRC 0x{zlib.crc32(data):08x}u")
result.append("""static inline int half(int value)
{
    return (value >> 1) + (int)((unsigned)value >> 31);
}""")
programs = re.findall(r"void (prog\d+)\([^\n]+\) \{\n(.*?)^\}", generated.read_text(), re.M | re.S)
assert len(programs) == 63
for name, body in programs:
    result.append(f"static int16_t {name}(int16_t in, int16_t *mem, unsigned *pointer, int16_t *state)\n{{")
    result.append("    unsigned base = *pointer;\n    int16_t acc = *state, left = 0;")
    address = 0
    for raw in body.splitlines():
        for statement in raw.split("//")[0].split(";"):
            statement = statement.strip()
            if not statement or statement.startswith("*outr"):
                continue
            advance = re.fullmatch(r"addr=\(addr\+0x([\dA-F]+)\)&0x3fff", statement)
            if advance:
                address = (address + int(advance[1], 16)) & 0x3fff
                continue
            for expr in ("mem[addr]", "acc", "~acc"):
                wrapped = f"({expr})" if expr == "~acc" else expr
                statement = statement.replace(f"({wrapped}>>1)+(({wrapped}>>15)&1)", f"half({expr})")
            statement = statement.replace("*outl", "left")
            statement = statement.replace("mem[addr]", f"mem[(base + {address}) & 16383]")
            result.append(f"    {statement};")
    result.append(f"    *pointer = (base + {address}) & 16383;\n    *state = acc;\n    return left;\n}}")
result.append("static int16_t (*const programs[63])(int16_t, int16_t *, unsigned *, int16_t *) = {")
result += [f"    {name}," for name, _ in programs]
result.append("};")
out.write_text("\n".join(result) + "\n")
