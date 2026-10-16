"""Lists or extracts the resources of swd32.exe.
Usage: python3 scripts/dump_resources.py <exe>                     (list type/name/language, offset and size)
       python3 scripts/dump_resources.py <exe> <type> <name> <out> (write one resource; type and name as listed)"""

import struct
import sys

TYPE_NAMES = {
    1: "CURSOR",
    2: "BITMAP",
    3: "ICON",
    4: "MENU",
    5: "DIALOG",
    6: "STRING",
    10: "RCDATA",
    12: "GROUP_CURSOR",
    14: "GROUP_ICON",
    16: "VERSION",
}


def sections(image):
    pe = struct.unpack_from("<I", image, 0x3C)[0]
    count = struct.unpack_from("<H", image, pe + 6)[0]
    optional_size = struct.unpack_from("<H", image, pe + 20)[0]
    table = pe + 24 + optional_size
    result = []
    for index in range(count):
        entry = table + 40 * index
        name = image[entry:entry + 8].rstrip(b"\0").decode()
        virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from("<IIII", image, entry + 8)
        result.append((name, virtual_address, max(virtual_size, raw_size), raw_pointer))
    return result


def label(image, base, value):
    if value & 0x80000000:
        offset = base + (value & 0x7FFFFFFF)
        length = struct.unpack_from("<H", image, offset)[0]
        return image[offset + 2:offset + 2 + 2 * length].decode("utf-16-le")
    return value


def walk(image, base, rva_to_offset):
    found = []

    def directory(offset, path):
        named, numbered = struct.unpack_from("<HH", image, offset + 12)
        for index in range(named + numbered):
            name, target = struct.unpack_from("<II", image, offset + 16 + 8 * index)
            key = label(image, base, name)
            if target & 0x80000000:
                directory(base + (target & 0x7FFFFFFF), path + [key])
            else:
                data_rva, size = struct.unpack_from("<II", image, base + target)
                found.append((path + [key], rva_to_offset(data_rva), size))

    directory(base, [])
    return found


def main():
    with open(sys.argv[1], "rb") as exe:
        image = exe.read()
    table = sections(image)

    def rva_to_offset(rva):
        for _, address, size, raw in table:
            if address <= rva < address + size:
                return raw + rva - address
        raise ValueError(hex(rva))

    pe = struct.unpack_from("<I", image, 0x3C)[0]
    resource_rva = struct.unpack_from("<I", image, pe + 24 + 96 + 8 * 2)[0]
    entries = walk(image, rva_to_offset(resource_rva), rva_to_offset)
    if len(sys.argv) == 2:
        for path, offset, size in entries:
            kind = TYPE_NAMES.get(path[0], path[0])
            print("%s\t%s\t%s\t0x%X\t%d" % (kind, path[1], path[2], offset, size))
        return
    kind, name, out = sys.argv[2], sys.argv[3], sys.argv[4]
    for path, offset, size in entries:
        if str(TYPE_NAMES.get(path[0], path[0])) == kind and str(path[1]) == name:
            with open(out, "wb") as target:
                target.write(image[offset:offset + size])
            return
    raise SystemExit("not found")


if __name__ == "__main__":
    main()
