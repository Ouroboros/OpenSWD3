"""Extracts the resources of swd32.exe into the res directory the program reads beside itself, one standard file each,
named by its resource id: the launcher's pictures (<id>.bmp), the sounds (<id>.wav), the window icon (<id>.ico) and
cursor (<id>.cur), and the dialog templates (<id>.dlg, as stored in the executable).

Usage: python3 scripts/make_resources.py <swd32.exe> <output directory>
"""

import os
import struct
import sys

from dump_resources import sections, walk

RT_CURSOR = 1
RT_BITMAP = 2
RT_ICON = 3
RT_DIALOG = 5
RT_GROUP_CURSOR = 12
RT_GROUP_ICON = 14

BITMAP_FILE_HEADER = 14
GROUP_HEADER = 6
GROUP_ENTRY = 14
FILE_ENTRY = 16
ICON_FILE = 1
CURSOR_FILE = 2


def bitmap_file(dib):
    header_size, _, _, _, bits, _, _, _, _, colors = struct.unpack_from("<IiiHHIIiiI", dib, 0)
    if bits <= 8 and colors == 0:
        colors = 1 << bits
    pixels = BITMAP_FILE_HEADER + header_size + 4 * colors
    return struct.pack("<2sIHHI", b"BM", BITMAP_FILE_HEADER + len(dib), 0, 0, pixels) + dib


def group_file(group, members, cursor):
    count = struct.unpack_from("<H", group, 4)[0]
    images = []
    entries = []
    offset = GROUP_HEADER + FILE_ENTRY * count
    for index in range(count):
        at = GROUP_HEADER + GROUP_ENTRY * index
        member = struct.unpack_from("<H", group, at + 12)[0]
        data = members[member]
        if cursor:
            width, height = struct.unpack_from("<HH", group, at)
            hot_x, hot_y = struct.unpack_from("<HH", data, 0)
            data = data[4:]
            entries.append(struct.pack("<BBBBHHII", width & 0xFF, (height // 2) & 0xFF, 0, 0, hot_x, hot_y,
                                       len(data), offset))
        else:
            width, height, colors, reserved, planes, bits = struct.unpack_from("<BBBBHH", group, at)
            entries.append(struct.pack("<BBBBHHII", width, height, colors, reserved, planes, bits, len(data), offset))
        images.append(data)
        offset += len(data)
    kind = CURSOR_FILE if cursor else ICON_FILE
    return struct.pack("<HHH", 0, kind, count) + b"".join(entries) + b"".join(images)


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)

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
    resources = {}
    for path, offset, size in walk(image, rva_to_offset(resource_rva), rva_to_offset):
        resources.setdefault(path[0], {})[path[1]] = image[offset:offset + size]

    files = {}
    for name, data in resources.get(RT_BITMAP, {}).items():
        files["%d.bmp" % name] = bitmap_file(data)
    for name, data in resources.get("WAVE", {}).items():
        files["%d.wav" % name] = data
    for name, data in resources.get(RT_GROUP_ICON, {}).items():
        files["%d.ico" % name] = group_file(data, resources[RT_ICON], False)
    for name, data in resources.get(RT_GROUP_CURSOR, {}).items():
        files["%d.cur" % name] = group_file(data, resources[RT_CURSOR], True)
    for name, data in resources.get(RT_DIALOG, {}).items():
        files["%d.dlg" % name] = data

    os.makedirs(sys.argv[2], exist_ok=True)
    for name, data in sorted(files.items()):
        with open(os.path.join(sys.argv[2], name), "wb") as target:
            target.write(data)
    print("%d files" % len(files))


if __name__ == "__main__":
    main()
