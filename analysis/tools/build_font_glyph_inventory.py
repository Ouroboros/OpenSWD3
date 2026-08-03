#!/usr/bin/env python3
"""Build assembly-locked inventories for the GDI-backed glyph renderer.

IDA pseudocode is deliberately not consumed.  The complete assembly is the
behavioral authority; the PE is read only to lock the default Big5 face name
and the zero-filled storage relied on by the two static renderer objects.
"""

from __future__ import annotations

import csv
import hashlib
import re
import struct
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FIELDS_OUTPUT = INVENTORY_ROOT / "font-renderer-object-fields.tsv"
PIPELINE_OUTPUT = INVENTORY_ROOT / "font-glyph-pipeline.tsv"
STYLES_OUTPUT = INVENTORY_ROOT / "font-glyph-style-footprints.tsv"
CALLSITES_OUTPUT = INVENTORY_ROOT / "font-render-callsites.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")

EXPECTED_SNIPPETS = {
    # Renderer object, fixed scratch surface, and measured pitch.
    0x0043525A: "push eax",
    0x0043525B: "push ecx",
    0x0043525C: "mov ecx, [esi]",
    0x0043525E: "call sub_437B60",
    0x00435265: "push 0",
    0x0043526B: "call sub_437C00",
    0x00435284: "mov [esp+84h+var_7C], 7Ch",
    0x0043528C: "mov [esp+84h+var_78], 0Ah",
    0x00435298: "call dword ptr [edx+58h]",
    0x004352A3: "sar eax, 1",
    0x004352A5: "mov [esi+0FDCh], edx",
    0x004352AB: "mov [esi+0FD8h], eax",
    0x004353CA: "push 40h",
    0x004353CC: "push 40h",
    0x004353D0: "call sub_435240",
    # LOGFONTA contract.
    0x0043550A: "cmp edi, 40h",
    0x00435511: "mov edi, 40h",
    0x0043553A: "neg eax",
    0x0043553D: "mov [esp+50h+lf.lfHeight], eax",
    0x00435541: "mov [esp+50h+lf.lfWidth], ebx",
    0x0043554D: "mov [esp+50h+lf.lfWeight], 190h",
    0x00435555: "mov [esp+50h+lf.lfItalic], bl",
    0x00435559: "mov [esp+50h+lf.lfUnderline], bl",
    0x0043555D: "mov [esp+50h+lf.lfStrikeOut], bl",
    0x00435561: "mov [esp+50h+lf.lfCharSet], 88h",
    0x0043556E: "mov [esp+50h+lf.lfQuality], bl",
    0x00435576: "call ds:lstrcpyA",
    0x00435581: "call ds:CreateFontIndirectA",
    # Framebuffer dimensions, row-offset table, and glyph cache allocation.
    0x004355EA: "imul edx, edi",
    0x004355ED: "shl edx, 1",
    0x004355F1: "mov [esi+0Ch], edi",
    0x004355F4: "mov [esi+10h], edx",
    0x00435600: "mov [ebx+eax*4-4], edx",
    0x00435604: "add edx, ecx",
    0x00435440: "cdq",
    0x00435446: "sar eax, 3",
    0x0043544F: "mov [esi+1Ch], eax",
    0x00435459: "jz short loc_43545F",
    0x0043545B: "inc eax",
    0x00435465: "imul eax, [esi+1Ch]",
    0x00435469: "lea eax, [eax+eax*4]",
    0x0043546C: "lea eax, [eax+eax*4]",
    0x0043546F: "lea eax, [eax+eax*4]",
    0x00435472: "shl eax, 4",
    # GDI rasterization and 16-bit surface-to-binary-mask conversion.
    0x004352ED: "cmp eax, 887601C2h",
    0x00435313: "mov ecx, [eax]",
    0x00435315: "call dword ptr [ecx+44h]",
    0x0043533E: "call ds:SetBkColor",
    0x0043534B: "call ds:SetBkMode",
    0x0043535B: "call ds:SetTextColor",
    0x00435371: "call ds:TextOutA",
    0x00435390: "call ds:TextOutA",
    0x004353AA: "mov eax, [esi]",
    0x004353AC: "call dword ptr [eax+68h]",
    0x00436851: "call sub_416F10",
    0x00436879: "add ecx, ecx",
    0x00436889: "rep movsd",
    0x00436891: "rep movsb",
    0x004368A9: "add ebp, edi",
    0x004368C0: "call sub_416F60",
    0x004368D8: "push 0FFFFFFh",
    0x004368EB: "call sub_4352E0",
    0x0043690E: "mov edx, 80h",
    0x00436934: "cmp word ptr [ebp+eax*2+0], 0",
    0x0043694A: "or [eax], dl",
    0x00436950: "sar edx, 1",
    # Byte parsing, sorted cache, and the 1999->1998 cap behavior.
    0x00436B0A: "mov al, [ecx+edx]",
    0x00436B13: "cmp al, 80h",
    0x00436B19: "mov al, [ecx+edx+1]",
    0x00436B22: "mov [esi+0FC9h], al",
    0x00436B32: "mov eax, [esi+0FE0h]",
    0x00436B38: "mov byte ptr [esi+0FC9h], 0",
    0x00436B3F: "sar eax, 1",
    0x0043699D: "cmp bx, dx",
    0x004369A0: "jnb short loc_4369A7",
    0x00436A4D: "mov cx, [eax-2]",
    0x00436A51: "mov [eax], cx",
    0x00436A6A: "rep movsd",
    0x00436A71: "rep movsb",
    0x00436A9C: "mov [esi+ebp*2+20h], bx",
    0x00436E29: "inc edx",
    0x00436E32: "cmp eax, 7CFh",
    0x00436E3C: "lea edi, [eax-1]",
    0x00436E3F: "mov [esi+0FC4h], edi",
    0x00436E69: "rep stosd",
    0x00436E70: "rep stosb",
    # Style priority and raw packed-color row delta.
    0x00436BDF: "test al, 1",
    0x00436C0E: "test al, 2",
    0x00436C3D: "test al, 4",
    0x00436C6C: "test al, 8",
    0x00436C9B: "test al, 10h",
    0x004356E5: "test dh, 1",
    0x004356EA: "mov [esp+20h+var_D], 0FFh",
    0x004356F1: "test dl, 80h",
    0x004356F6: "mov [esp+20h+var_D], 1",
    # Representative footprint writes for all five selector families.
    0x0043576D: "mov [ebx], si",
    0x00435C06: "mov [eax], cx",
    0x00435C0D: "mov [esi], ecx",
    0x00435EB5: "mov [esi], edx",
    0x00435EB7: "mov [edi+2], ecx",
    0x0043615B: "mov [ebx], bp",
    0x0043615E: "mov [eax-2], esi",
    0x00436161: "mov [edx+eax-2], esi",
    0x0043616C: "mov [edx+eax+2], bx",
    0x004363F9: "call sub_435680",
    0x0043653B: "mov [ebx], bp",
    0x00436545: "mov [eax-2], bx",
    0x00436550: "mov [eax+4], bx",
    0x00436554: "mov [edx+eax-2], esi",
    0x00436558: "mov [edx+eax+2], esi",
    0x00436831: "call sub_4358C0",
    # Optional background rectangle and fixed ASCII/double-byte advance.
    0x00436B65: "cmp ax, 0FFFEh",
    0x00436B9F: "call sub_436EA0",
    0x00436C05: "sub eax, edx",
    0x00436C07: "add ebp, eax",
}

EXPECTED_STYLE_OPERANDS = Counter(
    {
        "4": 160,
        "10h": 100,
        "esi": 5,
        "ecx": 4,
        "15h": 4,
        "1": 3,
        "eax": 3,
        "edx": 2,
        "8": 2,
        "19h": 2,
        "84h": 1,
        "18h": 1,
        "0Fh": 1,
        "edi": 1,
        "0D0h": 1,
        "1Fh": 1,
    }
)
EXPECTED_CALL_COUNT = 291


@dataclass(frozen=True)
class Instruction:
    address: int
    text: str
    owner: str


@dataclass(frozen=True)
class FieldRow:
    offset: str
    width_or_extent: str
    physical_role: str
    producer_or_consumer: str
    fidelity_note: str


@dataclass(frozen=True)
class PipelineRow:
    stage: str
    assembly_addresses: str
    exact_contract: str
    compatibility_consequence: str


@dataclass(frozen=True)
class StyleRow:
    selector: str
    function_chain: str
    foreground_writes: str
    secondary_prepass_or_shadow_writes: str
    vertical_clip: str
    horizontal_clip: str
    row_color_contract: str
    write_order_note: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(instruction: str) -> str:
    return " ".join(instruction.split())


def load_assembly() -> tuple[list[Instruction], dict[int, str]]:
    ordered: list[Instruction] = []
    by_address: dict[int, str] = {}
    owner = ""
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        owner_match = re.search(r"\b(sub_[0-9A-F]{6})\s+proc near\b", raw)
        if owner_match:
            owner = owner_match.group(1)
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        instruction = match.group(2).split(";", 1)[0].rstrip()
        if not instruction or instruction.endswith(":"):
            continue
        address = int(match.group(1), 16)
        text = normalize(instruction)
        ordered.append(Instruction(address, text, owner))
        by_address[address] = text
    return ordered, by_address


def verify_locked_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"locked input changed: {path} expected {expected}, got {actual}"
            )


def pe_sections(data: bytes) -> tuple[int, dict[str, tuple[int, int, int, int]]]:
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise SystemExit("invalid PE signature")
    coff_offset = pe_offset + 4
    count = struct.unpack_from("<H", data, coff_offset + 2)[0]
    optional_size = struct.unpack_from("<H", data, coff_offset + 16)[0]
    optional_offset = coff_offset + 20
    image_base = struct.unpack_from("<I", data, optional_offset + 28)[0]
    section_offset = optional_offset + optional_size
    sections: dict[str, tuple[int, int, int, int]] = {}
    for index in range(count):
        offset = section_offset + index * 40
        name = data[offset : offset + 8].split(b"\0", 1)[0].decode("ascii")
        sections[name] = struct.unpack_from("<IIII", data, offset + 8)
    return image_base, sections


def va_to_file_offset(
    virtual_address: int,
    image_base: int,
    sections: dict[str, tuple[int, int, int, int]],
) -> int:
    rva = virtual_address - image_base
    for virtual_size, section_rva, raw_size, raw_offset in sections.values():
        if section_rva <= rva < section_rva + min(virtual_size, raw_size):
            return raw_offset + (rva - section_rva)
    raise SystemExit(f"VA 0x{virtual_address:08X} is not file-backed")


def verify_pe_contract() -> str:
    data = EXE_PATH.read_bytes()
    image_base, sections = pe_sections(data)
    expected_data = (0x000A07DC, 0x0009E000, 0x0000C000, 0x0009E000)
    if image_base != 0x00400000 or sections.get(".data") != expected_data:
        raise SystemExit(
            f"unexpected PE mapping: image_base={image_base:#x}, .data={sections.get('.data')}"
        )
    face_offset = va_to_file_offset(0x0049FB74, image_base, sections)
    face_bytes = data[face_offset : face_offset + 32].split(b"\0", 1)[0]
    if face_bytes != bytes.fromhex("B2 D3 A9 FA C5 E9"):
        raise SystemExit(f"unexpected default face bytes: {face_bytes.hex()}")
    face = face_bytes.decode("cp950")
    if face != "細明體":
        raise SystemExit(f"unexpected decoded default face: {face}")

    virtual_size, section_rva, raw_size, _ = expected_data
    zero_start = image_base + section_rva + raw_size
    zero_end = image_base + section_rva + virtual_size
    for base in (0x004AB998, 0x004C9A28):
        if not (zero_start <= base and base + 0xFF8 <= zero_end):
            raise SystemExit(f"static font object 0x{base:08X} is not in PE zero-fill")
    return face


def verify_assembly(by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: "
                f"expected {expected!r}, got {actual!r}"
            )

    font_set_calls = [
        address for address, text in by_address.items() if text == "call sub_435500"
    ]
    if font_set_calls != [0x0040F37C, 0x004351A9]:
        raise SystemExit(f"unexpected sub_435500 call set: {font_set_calls}")

    scratch_terminator_refs = [
        (address, text)
        for address, text in by_address.items()
        if re.search(r"\[[^]]+\+0?FCAh\]", text)
    ]
    if scratch_terminator_refs:
        raise SystemExit(
            f"character scratch +0xFCA unexpectedly has explicit refs: {scratch_terminator_refs}"
        )


def parse_immediate(operand: str) -> int | None:
    if re.fullmatch(r"[0-9]+", operand):
        return int(operand, 10)
    match = re.fullmatch(r"([0-9A-F]+)h", operand)
    if match:
        return int(match.group(1), 16)
    return None


def selected_style(value: int) -> str:
    for bit in (1, 2, 4, 8, 0x10):
        if value & bit:
            return f"0x{bit:02X}"
    return "none"


def row_delta(value: int) -> str:
    if value & 0x100:
        return "-1"
    if value & 0x80:
        return "+1"
    return "0"


def build_callsites(ordered: list[Instruction]) -> list[tuple[object, ...]]:
    rows: list[tuple[object, ...]] = []
    style_operands: Counter[str] = Counter()
    calls_between_counts: Counter[int] = Counter()
    for index, instruction in enumerate(ordered):
        if instruction.text != "call sub_436AD0":
            continue
        push_indices: list[int] = []
        cursor = index - 1
        while cursor >= 0 and len(push_indices) < 6:
            if ordered[cursor].text.startswith("push "):
                push_indices.append(cursor)
            cursor -= 1
        if len(push_indices) != 6:
            raise SystemExit(
                f"call 0x{instruction.address:08X} has only {len(push_indices)} prior pushes"
            )
        push_indices.reverse()
        pushes = [ordered[item] for item in push_indices]
        if pushes[0].owner != instruction.owner:
            raise SystemExit(f"call 0x{instruction.address:08X} push set crosses function")
        intervening_calls = sum(
            item.text.startswith("call ")
            for item in ordered[push_indices[0] + 1 : index]
        )
        calls_between_counts[intervening_calls] += 1
        operands = [item.text[5:] for item in pushes]
        style_operand = operands[0]
        style_operands[style_operand] += 1
        style_value = parse_immediate(style_operand)
        rows.append(
            (
                f"0x{instruction.address:08X}",
                instruction.owner,
                ",".join(f"0x{item.address:08X}" for item in pushes),
                operands[5],
                operands[4],
                operands[3],
                operands[2],
                operands[1],
                style_operand,
                "" if style_value is None else f"0x{style_value:X}",
                "runtime" if style_value is None else selected_style(style_value),
                "runtime" if style_value is None else row_delta(style_value),
                intervening_calls,
            )
        )

    if len(rows) != EXPECTED_CALL_COUNT:
        raise SystemExit(f"expected {EXPECTED_CALL_COUNT} text calls, got {len(rows)}")
    if style_operands != EXPECTED_STYLE_OPERANDS:
        raise SystemExit(
            f"static style operand distribution changed: {style_operands}"
        )
    if calls_between_counts != Counter({0: 277, 1: 14}):
        raise SystemExit(
            f"intervening-call distribution changed: {calls_between_counts}"
        )
    return rows


def field_rows() -> list[FieldRow]:
    return [
        FieldRow("0x000", "4", "DirectDraw owner/interface", "sub_435160; sub_437B60", "physical pointer; final C++ ownership model not yet named"),
        FieldRow("0x004", "4", "64x64 temporary DirectDraw surface", "sub_435240/sub_4352C0", "released through surface vtable +0x08"),
        FieldRow("0x008", "4", "destination width in 16-bit pixels", "sub_4355B0; sub_436EA0", "not a byte stride"),
        FieldRow("0x00C", "4", "destination height in rows", "sub_4355B0; sub_436EA0", "full-buffer clamp bound"),
        FieldRow("0x010", "4", "destination allocation size width*height*2", "sub_4355B0", "stored even though draw loops use row offsets"),
        FieldRow("0x014", "height*4", "row offset table; entry[y]=y*width", "sub_4355B0; all glyph writers", "offset unit is 16-bit pixels"),
        FieldRow("0x018", "font_width*font_height*2", "temporary copied 16-bit GDI raster", "sub_4353C0/sub_436840/sub_4368D0", "copies only configured font rectangle from 64x64 surface"),
        FieldRow("0x01C", "4", "glyph mask bytes per row ceil(font_width/8)", "sub_435430", "signed arithmetic sequence must remain exact for invalid widths"),
        FieldRow("0x020", "0xFA0 = 2000*u16", "sorted raw one/two-byte character keys", "sub_436980/sub_4369C0", "unsigned u16 ordering; no Unicode normalization"),
        FieldRow("0xFC0", "4", "packed glyph-mask cache pointer", "sub_435430/sub_4369C0", "allocation is 2000*font_height*mask_row_bytes"),
        FieldRow("0xFC4", "4", "current sorted cache count", "sub_436AD0", "when increment reaches 1999 it is forced to 1998 and slot 1998 is cleared"),
        FieldRow("0xFC8", "2+implicit NUL", "current raw character byte string", "sub_436AD0/sub_4368D0", "+0xFCA has no writer and relies on static PE zero-fill as terminator"),
        FieldRow("0xFCC", "4", "HFONT", "sub_435500/sub_4351F0", "created with CreateFontIndirectA and deleted with DeleteObject"),
        FieldRow("0xFD0", "4", "configured glyph rectangle width", "sub_4353C0/sub_435500", "normally equals clamped font size"),
        FieldRow("0xFD4", "4", "configured glyph rectangle height", "sub_4353C0/sub_435500", "normally equals clamped font size"),
        FieldRow("0xFD8", "4", "temporary surface pitch/2 in pixels", "sub_435240/sub_436840", "derived by signed arithmetic shift from reported byte pitch"),
        FieldRow("0xFDC", "4", "temporary surface reported height", "sub_435240", "stored physical field; no later direct consumer found"),
        FieldRow("0xFE0", "4", "double-byte horizontal advance", "sub_435650/sub_436AD0", "ASCII advance is arithmetic half; raw high-bit character uses full value"),
        FieldRow("0xFE4", "2", "secondary shadow/outline color", "sub_435670; style 0x02..0x10", "packed 16-bit value, sometimes duplicated into dword writes"),
        FieldRow("0xFE6", "2", "background rectangle color or 0xFFFE disable sentinel", "sub_435660/sub_436AD0", "background fill clamps to full destination, not glyph clip rectangle"),
        FieldRow("0xFE8..0xFF4", "4*4", "clip left, top, width, height", "sub_435620; glyph writers", "right=left+width and bottom=top+height with style-specific strict comparisons"),
    ]


def pipeline_rows(face: str) -> list[PipelineRow]:
    return [
        PipelineRow("font_definition", "0x00435500-0x00435591", f"default face={face}; CP950 bytes B2 D3 A9 FA C5 E9; height=-min(requested,64), width=0, weight=400, charset=0x88, quality=0", "font pixels are supplied by the installed Windows GDI font implementation, not by an EXE bitmap"),
        PipelineRow("scratch_surface", "0x00435240-0x004353D0,0x00437B60-0x00437BF4", "always create a 64x64 DirectDraw offscreen surface; clear with native color zero; read lPitch and divide by two for 16-bit pixel stride", "surface allocation and pitch are platform backend responsibilities, while the 64x64 and signed pitch rule are compatibility constants"),
        PipelineRow("gdi_raster", "0x004352E0-0x004353B1,0x004368D0-0x004368F0", "restore lost surface, clear black, acquire HDC, select HFONT, transparent background, white text; issue TextOutA for two spaces then the raw 1/2-byte character", "call order and default GDI quality can change masks across OS/font versions"),
        PipelineRow("surface_copy", "0x00436840-0x004368CD", "lock surface; copy font_width*2 bytes for font_height rows from surface pitch into tightly packed 16-bit scratch; unlock", "never infer tight surface stride"),
        PipelineRow("mask_pack", "0x00436905-0x00436974", "for each scratch word !=0 set an MSB-first bit (0x80..0x01); row stride=ceil(font_width/8); padding bits stay zero", "GDI coverage intensity is discarded; any nonzero pixel becomes fully set"),
        PipelineRow("character_parse", "0x00436B0A-0x00436B45", "byte<0x80 -> one byte plus zero; byte>=0x80 -> blindly consume the next byte; key is the little-endian raw u16", "this is a CP950-era byte protocol, not UTF-8 and not validated Big5"),
        PipelineRow("cache_lookup_insert", "0x00436980-0x00436AC2,0x00436CCA-0x00436E70", "unsigned binary search in sorted keys; insert by shifting keys and full mask slots; after count reaches 1999 force count=1998 and clear slot 1998", "preserve the off-by-one eviction behavior; do not replace it with an unbounded map in the compatibility core"),
        PipelineRow("background_fill", "0x00436B65-0x00436BD7,0x00436EA0-0x00436F66", "unless color==0xFFFE, fill a clipped-to-framebuffer rectangle before glyph pixels; last byte character uses a special width adjustment", "fill uses framebuffer bounds rather than the glyph clip rectangle"),
        PipelineRow("style_dispatch", "0x00436BDB-0x00436E15", "test selector bits in priority order 0x01,0x02,0x04,0x08,0x10; the first set bit wins; no low selector means no glyph-pixel writer", "combined style values are not an enum and must retain priority behavior"),
        PipelineRow("advance", "0x00436B32-0x00436B41,0x00436BFB-0x00436E29", "ASCII advance=FE0-(FE0>>1); raw high-bit two-byte advance=FE0; advance is fixed and ignores actual GDI glyph metrics", "layout determinism depends on integer byte classification, not shaped text metrics"),
    ]


def style_rows() -> list[StyleRow]:
    delta = "delta=-1 if flags&0x100 else +1 if flags&0x80 else 0; low16 foreground += delta per mask row"
    return [
        StyleRow("0x01", "sub_435680", "(0,0)=foreground", "none", "p.y>=top && p.y<bottom", "p.x>=left && p.x+1<right", delta, "single foreground word; right comparison intentionally reserves one extra column"),
        StyleRow("0x02", "sub_435AF0", "(0,0)=foreground", "(0,+1),(+1,+1)=secondary", "p.y>=top && p.y+1<bottom", "p.x>=left && p.x+1<right", delta, "foreground word first, then duplicated secondary dword on next row"),
        StyleRow("0x04", "sub_435D80", "(0,0),(+1,0)=foreground", "(+1,+1),(+2,+1)=secondary", "p.y>=top && p.y+1<bottom", "p.x>=left && p.x+2<right", delta, "duplicated foreground dword followed by diagonally shifted duplicated secondary dword"),
        StyleRow("0x08", "sub_436030 -> sub_435680", "overlay (0,0)=foreground", "prepass (0,-1),(-1,0),(0,0),(-1,+1),(0,+1),(+1,+1)=secondary", "prepass p.y>top && p.y+1<bottom; overlay uses 0x01 clip", "prepass p.x>=left-1 && p.x+1<right; overlay uses 0x01 clip", delta + "; overlay starts after the prepass has already advanced foreground by glyph_height*delta", "secondary prepass first; overlay normally replaces center, but its narrower clip can leave a secondary-only edge pixel"),
        StyleRow("0x10", "sub_436410 -> sub_4358C0", "overlay (0,0),(+1,0)=foreground", "prepass (0,-1),(-1,0),(+2,0),(-1,+1),(0,+1),(+1,+1),(+2,+1)=secondary", "prepass p.y>top && p.y+1<bottom; overlay uses current-row clip", "prepass p.x>=left-1 && p.x+2<right; overlay p.x>=left && p.x+2<right", delta + "; overlay starts after the prepass has already advanced foreground by glyph_height*delta", "secondary prepass first, then horizontally doubled foreground overlay"),
    ]


def write_table(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def write_outputs(
    fields: list[FieldRow],
    pipeline: list[PipelineRow],
    styles: list[StyleRow],
    callsites: list[tuple[object, ...]],
) -> None:
    write_table(
        FIELDS_OUTPUT,
        ("offset", "width_or_extent", "physical_role", "producer_or_consumer", "fidelity_note"),
        [(row.offset, row.width_or_extent, row.physical_role, row.producer_or_consumer, row.fidelity_note) for row in fields],
    )
    write_table(
        PIPELINE_OUTPUT,
        ("stage", "assembly_addresses", "exact_contract", "compatibility_consequence"),
        [(row.stage, row.assembly_addresses, row.exact_contract, row.compatibility_consequence) for row in pipeline],
    )
    write_table(
        STYLES_OUTPUT,
        ("selector_bit", "function_chain", "foreground_writes_relative_to_mask_pixel", "secondary_prepass_or_shadow_writes_relative_to_mask_pixel", "vertical_clip", "horizontal_clip", "row_color_contract", "write_order_note"),
        [(row.selector, row.function_chain, row.foreground_writes, row.secondary_prepass_or_shadow_writes, row.vertical_clip, row.horizontal_clip, row.row_color_contract, row.write_order_note) for row in styles],
    )
    write_table(
        CALLSITES_OUTPUT,
        ("call_address", "caller", "six_push_addresses_oldest_to_newest", "destination_operand", "x_operand", "y_operand", "string_operand", "color_operand", "style_operand", "static_style_value", "selected_style", "foreground_row_delta", "intervening_call_count"),
        callsites,
    )


def main() -> None:
    verify_locked_inputs()
    ordered, by_address = load_assembly()
    face = verify_pe_contract()
    verify_assembly(by_address)
    callsites = build_callsites(ordered)
    fields = field_rows()
    pipeline = pipeline_rows(face)
    styles = style_rows()
    write_outputs(fields, pipeline, styles, callsites)
    print(
        f"wrote {len(fields)} font fields, {len(pipeline)} pipeline stages, "
        f"{len(styles)} glyph style footprints, and {len(callsites)} direct text callsites; "
        f"default face is {face}"
    )


if __name__ == "__main__":
    main()
