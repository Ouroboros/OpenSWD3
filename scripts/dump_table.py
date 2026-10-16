"""Prints a table of swd32.exe as a C++ initializer list (for constexpr tables; a unit test compares them with the
executable). Usage: python3 scripts/dump_table.py <exe> <va hex> <count> <i8|u8|i16|u16|i32|u32> [per line]"""

import struct
import sys

FORMATS = {"i8": "b", "u8": "B", "i16": "h", "u16": "H", "i32": "i", "u32": "I"}
IMAGE_BASE = 0x400000


def main():
    path, va, count, kind = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3]), sys.argv[4]
    per_line = int(sys.argv[5]) if len(sys.argv) > 5 else 16
    code = FORMATS[kind]
    size = struct.calcsize(code)
    with open(path, "rb") as exe:
        exe.seek(va - IMAGE_BASE)
        data = exe.read(size * count)
    values = struct.unpack("<" + code * count, data)
    for start in range(0, count, per_line):
        print("    " + ", ".join(str(v) for v in values[start:start + per_line]) + ",")


if __name__ == "__main__":
    main()
