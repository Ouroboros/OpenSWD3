"""Prints NUL-terminated strings of swd32.exe as C++ string literals (non-ASCII bytes escaped, Big5 kept as bytes)
together with their UTF-8 reading. Usage: python3 scripts/dump_string.py <exe> <va hex>..."""

import sys

IMAGE_BASE = 0x400000


def literal(data):
    # Octal escapes: at most three digits, so a following character can never extend them.
    out = []
    for byte in data:
        if 0x20 <= byte < 0x7F and byte not in (0x22, 0x5C, 0x3F):
            out.append(chr(byte))
        else:
            out.append("\\%03o" % byte)
    return '"' + "".join(out) + '"'


def main():
    path = sys.argv[1]
    with open(path, "rb") as exe:
        image = exe.read()
    for va_text in sys.argv[2:]:
        va = int(va_text, 16)
        start = va - IMAGE_BASE
        end = image.index(b"\0", start)
        data = image[start:end]
        try:
            reading = data.decode("cp950")
        except UnicodeDecodeError:
            reading = "?"
        print("0x%08X %s  // %s" % (va, literal(data), reading))


if __name__ == "__main__":
    main()
