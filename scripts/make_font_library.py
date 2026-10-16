"""Builds the game's glyph library from MingLiU (mingliu.ttc, face 0 "細明體").

The original draws text with GDI: TextOutA at (0, 0) with the Big5 font "細明體" of 12, 16 or 20 pixels, then keeps
the non-black pixels of the size x size cell as a 1-bit glyph. MingLiU carries embedded 1-bit bitmaps (EBLC/EBDT) for
these sizes, which is what GDI draws without smoothing. This script takes those bitmaps directly: every code of Big5
with the ETen extensions (A140-F9FE) and every printable ASCII byte is mapped through cp950 (the ETen/HKSCS table for
the codes cp950 leaves out) and the font's Unicode cmap to a glyph (face 0 first, then the other faces of the
collection), whose bitmap is placed in the cell as TextOutA places it (x = bearing X, y = ascent - bearing Y).
Characters the strike has no bitmap for are drawn from the hinted outline, as GDI does.

Usage: python3 scripts/make_font_library.py <mingliu.ttc> <output file>

Output (little endian): "SWD3FONT", u32 version 1, u32 strike count; per strike u32 size, u32 glyph count, then per
glyph u16 code (first byte in the low byte, as the game stores characters) and size rows of ceil(size / 8) bytes.
"""

import struct
import sys

SIZES = (12, 16, 20)


def u8(data, offset):
    return data[offset]


def i8(data, offset):
    return struct.unpack_from(">b", data, offset)[0]


def u16(data, offset):
    return struct.unpack_from(">H", data, offset)[0]


def i16(data, offset):
    return struct.unpack_from(">h", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from(">I", data, offset)[0]


class Font:
    def __init__(self, data, index):
        self.data = data
        start = u32(data, 12 + 4 * index) if data[:4] == b"ttcf" else 0
        self.tables = {}
        for i in range(u16(data, start + 4)):
            entry = start + 12 + 16 * i
            self.tables[data[entry:entry + 4].decode("latin1")] = (u32(data, entry + 8), u32(data, entry + 12))
        self.cmap = self.read_cmap()

    def table(self, tag):
        return self.tables[tag][0]

    def read_cmap(self):
        data = self.data
        base = self.table("cmap")
        mapping = {}
        for i in range(u16(data, base + 2)):
            platform, encoding, offset = u16(data, base + 4 + 8 * i), u16(data, base + 6 + 8 * i), u32(data, base + 8 + 8 * i)
            sub = base + offset
            if platform == 3 and encoding == 1 and u16(data, sub) == 4:
                segments = u16(data, sub + 6) // 2
                ends = sub + 14
                starts = ends + 2 * segments + 2
                deltas = starts + 2 * segments
                ranges = deltas + 2 * segments
                for s in range(segments):
                    end, first = u16(data, ends + 2 * s), u16(data, starts + 2 * s)
                    delta, range_offset = i16(data, deltas + 2 * s), u16(data, ranges + 2 * s)
                    for code in range(first, end + 1):
                        if code == 0xFFFF:
                            continue
                        if range_offset == 0:
                            glyph = (code + delta) & 0xFFFF
                        else:
                            at = ranges + 2 * s + range_offset + 2 * (code - first)
                            glyph = u16(data, at)
                            if glyph != 0:
                                glyph = (glyph + delta) & 0xFFFF
                        if glyph != 0:
                            mapping.setdefault(code, glyph)
            elif platform == 3 and encoding == 10 and u16(data, sub) == 12:
                for g in range(u32(data, sub + 12)):
                    group = sub + 16 + 12 * g
                    first, last, glyph = u32(data, group), u32(data, group + 4), u32(data, group + 8)
                    for code in range(first, last + 1):
                        mapping[code] = glyph + code - first
        return mapping

    def outline_ascent(self, size):
        head = self.table("head")
        units = u16(self.data, head + 18)
        os2 = self.table("OS/2")
        win_ascent = u16(self.data, os2 + 74)
        return (win_ascent * size + units // 2) // units

    def strike(self, size):
        data = self.data
        eblc = self.table("EBLC")
        for s in range(u32(data, eblc + 4)):
            record = eblc + 8 + 48 * s
            if u8(data, record + 44) == size and u8(data, record + 46) == 1:
                return Strike(self, record)
        return None


class Strike:
    def __init__(self, font, record):
        data = font.data
        self.font = font
        self.ascender = i8(data, record + 16)
        self.descender = i8(data, record + 17)
        self.size = u8(data, record + 44)
        eblc = font.table("EBLC")
        array = eblc + u32(data, record)
        self.subtables = []
        for i in range(u32(data, record + 8)):
            first, last = u16(data, array + 8 * i), u16(data, array + 8 * i + 2)
            self.subtables.append((first, last, array + u32(data, array + 8 * i + 4)))

    def glyph(self, glyph):
        """(bitmap rows as lists of 0/1, width, height, bearing x, bearing y) of a glyph, or None."""
        data = self.font.data
        ebdt = self.font.table("EBDT")
        for first, last, sub in self.subtables:
            if not first <= glyph <= last:
                continue
            index_format, image_format, image_offset = u16(data, sub), u16(data, sub + 2), u32(data, sub + 4)
            body = sub + 8
            metrics = None
            if index_format == 1:
                offset = u32(data, body + 4 * (glyph - first))
                if u32(data, body + 4 * (glyph - first + 1)) == offset:
                    return None
            elif index_format == 3:
                offset = u16(data, body + 2 * (glyph - first))
                if u16(data, body + 2 * (glyph - first + 1)) == offset:
                    return None
            elif index_format == 2:
                image_size = u32(data, body)
                metrics = body + 4
                offset = image_size * (glyph - first)
            elif index_format == 4:
                count = u32(data, body)
                offset = None
                for g in range(count):
                    if u16(data, body + 4 + 4 * g) == glyph:
                        offset = u16(data, body + 6 + 4 * g)
                        break
                if offset is None:
                    return None
            elif index_format == 5:
                image_size = u32(data, body)
                metrics = body + 4
                count = u32(data, body + 12)
                ids = [u16(data, body + 16 + 2 * g) for g in range(count)]
                if glyph not in ids:
                    return None
                offset = image_size * ids.index(glyph)
            else:
                return None
            return self.image(ebdt + image_offset + offset, image_format, metrics)
        return None

    def image(self, at, image_format, big_metrics):
        data = self.font.data
        if image_format in (1, 2):
            height, width, bearing_x, bearing_y = u8(data, at), u8(data, at + 1), i8(data, at + 2), i8(data, at + 3)
            bits = at + 5
        elif image_format in (6, 7):
            height, width, bearing_x, bearing_y = u8(data, at), u8(data, at + 1), i8(data, at + 2), i8(data, at + 3)
            bits = at + 8
        elif image_format == 5 and big_metrics is not None:
            height, width = u8(data, big_metrics), u8(data, big_metrics + 1)
            bearing_x, bearing_y = i8(data, big_metrics + 2), i8(data, big_metrics + 3)
            bits = at
        else:
            return None
        rows = []
        if image_format in (1, 6):
            stride = (width + 7) // 8
            for y in range(height):
                row = data[bits + stride * y:bits + stride * (y + 1)]
                rows.append([(row[x // 8] >> (7 - x % 8)) & 1 for x in range(width)])
        else:
            for y in range(height):
                row = []
                for x in range(width):
                    bit = y * width + x
                    row.append((data[bits + bit // 8] >> (7 - bit % 8)) & 1)
                rows.append(row)
        return rows, width, height, bearing_x, bearing_y


def decode_big5(code_bytes):
    """The character of a Big5 code: Microsoft cp950 first, the ETen/HKSCS table for the codes cp950 leaves out."""
    for codec in ("cp950", "big5hkscs"):
        try:
            text = code_bytes.decode(codec)
        except UnicodeDecodeError:
            continue
        if len(text) == 1:
            return text
    return None


def is_big5(code_bytes):
    """Big5 proper with the ETen extensions: A140-F9FE (the user-defined areas belong to HKSCS)."""
    if len(code_bytes) == 1:
        return True
    code = code_bytes[0] << 8 | code_bytes[1]
    return 0xA140 <= code <= 0xF9FE


def big5_codes():
    for byte in range(0x20, 0x7F):
        yield bytes([byte])
    for lead in range(0x81, 0xFF):
        for trail in list(range(0x40, 0x7F)) + list(range(0xA1, 0xFF)):
            yield bytes([lead, trail])


OUTLINE_FONTS = {}


def outline_cell(source, face, size, ascent, text):
    """The cell of a character the strike has no bitmap for: GDI then draws the hinted outline without smoothing,
    approximated with FreeType's monochrome hinted rendering, baseline at the ascent."""
    from PIL import Image, ImageDraw, ImageFont

    key = (face, size)
    if key not in OUTLINE_FONTS:
        OUTLINE_FONTS[key] = ImageFont.truetype(source, size, index=face)
    image = Image.new("1", (size, size), 0)
    draw = ImageDraw.Draw(image)
    draw.fontmode = "1"
    draw.text((0, ascent), text, font=OUTLINE_FONTS[key], fill=1, anchor="ls")
    return [[1 if image.getpixel((x, y)) else 0 for x in range(size)] for y in range(size)]


def main():
    source, output = sys.argv[1], sys.argv[2]
    with open(source, "rb") as file:
        data = file.read()
    faces = [Font(data, index) for index in range(u32(data, 8))] if data[:4] == b"ttcf" else [Font(data, 0)]
    font = faces[0]
    out = bytearray(b"SWD3FONT")
    out += struct.pack("<II", 1, len(SIZES))
    for size in SIZES:
        strikes = [face.strike(size) for face in faces]
        strike = strikes[0]
        if strike is None:
            raise SystemExit("no %d pixel strike" % size)
        print("size %d: strike ascent %d, outline ascent %d" % (size, strike.ascender, font.outline_ascent(size)))
        stride = (size + 7) // 8
        glyphs = []
        outlined = 0
        for code_bytes in big5_codes():
            text = decode_big5(code_bytes) if is_big5(code_bytes) else None
            if text is None:
                continue
            owner = next((i for i, face in enumerate(faces) if ord(text) in face.cmap and strikes[i] is not None), None)
            if owner is None:
                continue
            image = strikes[owner].glyph(faces[owner].cmap[ord(text)])
            ascent = strikes[owner].ascender
            cell = [[0] * size for _ in range(size)]
            if image is not None:
                rows, width, height, bearing_x, bearing_y = image
                top = ascent - bearing_y
                for y in range(height):
                    for x in range(width):
                        cx, cy = bearing_x + x, top + y
                        if rows[y][x] and 0 <= cx < size and 0 <= cy < size:
                            cell[cy][cx] = 1
            else:
                cell = outline_cell(source, owner, size, ascent, text)
                outlined += 1
            code = code_bytes[0] | (code_bytes[1] << 8 if len(code_bytes) == 2 else 0)
            packed = bytearray()
            for row in cell:
                for b in range(stride):
                    value = 0
                    for bit in range(8):
                        x = 8 * b + bit
                        if x < size and row[x]:
                            value |= 0x80 >> bit
                    packed.append(value)
            glyphs.append((code, bytes(packed)))
        glyphs.sort()
        out += struct.pack("<II", size, len(glyphs))
        for code, packed in glyphs:
            out += struct.pack("<H", code) + packed
        print("size %d: %d glyphs, %d drawn from the outline" % (size, len(glyphs), outlined))
    with open(output, "wb") as file:
        file.write(out)


if __name__ == "__main__":
    main()
