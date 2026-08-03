#!/usr/bin/env python3
"""Build assembly-locked inventories for SWD3 packed-16 color helpers.

The complete assembly is the only behavioral authority.  Pseudocode is not
read.  The generated vectors are evaluations of the exact integer rules
recovered from the locked instruction stream, not screenshots or a new
implementation of the game.
"""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"

FUNCTION_OUTPUT = INVENTORY_ROOT / "frame-color-functions.tsv"
CALLSITE_OUTPUT = INVENTORY_ROOT / "frame-color-callsites.tsv"
VECTOR_OUTPUT = INVENTORY_ROOT / "frame-color-format-vectors.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")

TARGET_CALLS = {
    "sub_420490": (
        0x004147D6, 0x0041564A, 0x00416723, 0x004169CA,
        0x004169E1, 0x004169F7, 0x00416A1B, 0x00416A36,
        0x00416A4E, 0x00416A60, 0x00416A72, 0x00416A81,
        0x0043A81E, 0x0043B257, 0x0043B3FC, 0x0043B40A,
        0x00441A4F, 0x00441A9F, 0x004427A0, 0x0044450C,
        0x004445E3, 0x00447330, 0x004475DE, 0x0044923A,
        0x004496A3, 0x00449B35, 0x0044C2A0, 0x0044C85C,
        0x0044D995, 0x004538AA, 0x0045D3D6,
    ),
    "sub_420560": (0x00453747, 0x0045B026, 0x0045BD30),
    "sub_420600": (0x0045375F, 0x0045B043, 0x0045BD48),
    "sub_4206F0": (0x00453776, 0x0045B061, 0x0045BD5F),
    "sub_4207E0": (
        0x004346F7, 0x00434942, 0x00434A29, 0x00434B79,
        0x00434BA8, 0x00434C1E, 0x00434C53,
    ),
    "sub_421FB0": (0x0043B2E4, 0x00449AD9, 0x0044D9A5),
}

FULL_307200 = {
    0x004147D6, 0x0041564A, 0x0043A81E, 0x0044923A,
    0x004496A3, 0x00449B35, 0x0044D995,
    0x0045BD30, 0x0045BD48, 0x0045BD5F,
    0x00449AD9, 0x0044D9A5,
}
PREFIX_245760 = {
    0x004538AA, 0x0045D3D6,
    0x00453747, 0x0045375F, 0x00453776,
    0x0045B026, 0x0045B043, 0x0045B061,
}
DYNAMIC_COUNT = {0x0043B257, 0x0043B3FC, 0x0043B40A, 0x0043B2E4}

EXPECTED_SNIPPETS = {
    # All-channel signed offset helper.
    0x00420496: "mov ecx, dword_4CDBF8",
    0x0042049F: "imul ecx, eax",
    0x00420509: "mov eax, [esi]",
    0x00420510: "test dword_4CD9D0, eax",
    0x00420549: "mov [esi], dx",
    0x0042054F: "sub edi, 1",
    0x00420552: "jnz short loc_420509",
    # One-channel and packed two-pixel helpers.
    0x0042058C: "mov eax, [esi]",
    0x0042059F: "mov [esi], dx",
    0x004205A5: "sub edi, 1",
    0x00420654: "mov eax, [esi]",
    0x0042065A: "add eax, [ebp+arg_8]",
    0x0042065D: "test cx, ax",
    0x00420676: "mov [esi], edx",
    0x0042067B: "sub edi, 2",
    0x00420744: "mov eax, [esi]",
    0x0042074C: "add eax, [ebp+arg_8]",
    0x0042074F: "test cx, ax",
    0x00420766: "mov [esi], edx",
    0x0042076B: "sub edi, 2",
    # Two-input table combine; note the global mask truncation side effect.
    0x004207FD: "and esi, 0FFFFh",
    0x0042080F: "mov dword_4CDE4C, esi",
    0x00420815: "test eax, eax",
    0x00420823: "jbe loc_4208C2",
    0x00420854: "mov ax, [edx+ebx]",
    0x0042085A: "mov dx, [ebx]",
    0x00420886: "mov di, word ptr dword_4CDC84[ecx*4]",
    0x00420894: "add di, word ptr dword_4CDA54[ebp*4]",
    0x004208AA: "add di, word ptr dword_4CD850[ebp*4]",
    0x004208B7: "mov [ebx-2], di",
    # Quarter-sum grayscale.
    0x00421FC5: "and eax, dword_4CDE4C",
    0x00421FEF: "add eax, ebx",
    0x00421FF3: "shr eax, 2",
    0x00422016: "mov [edi], ax",
    0x0042201C: "sub esi, 1",
    # Table construction: 32 increasing entries followed by 32 zero entries.
    0x00423835: "mov dword_4CD850[eax], ecx",
    0x0042383B: "mov dword_4CDA54[eax], edx",
    0x00423841: "mov dword_4CDC84[eax], esi",
    0x00423850: "cmp eax, 80h",
    0x00423860: "mov edi, offset unk_4CDD04",
    0x00423870: "mov edi, offset unk_4CDAD4",
    0x00423880: "mov edi, offset unk_4CD8D0",
}


@dataclass(frozen=True)
class Instruction:
    address: int
    text: str
    owner: str


@dataclass(frozen=True)
class PixelFormat:
    name: str
    red_mask: int
    green_mask: int
    blue_mask: int
    red_shift: int
    green_shift: int
    blue_shift: int

    @property
    def red_unit(self) -> int:
        return 1 << self.red_shift

    @property
    def green_unit(self) -> int:
        return 1 << self.green_shift

    @property
    def blue_unit(self) -> int:
        return 1 << self.blue_shift


FORMATS = (
    PixelFormat("R7C00_G03E0_B001F", 0x7C00, 0x03E0, 0x001F, 10, 5, 0),
    PixelFormat("RF800_G07E0_B003F_narrow_B003E", 0xF800, 0x07E0, 0x003E, 11, 6, 1),
    PixelFormat("RF800_G07E0_B001F_narrow_G07C0", 0xF800, 0x07C0, 0x001F, 11, 6, 0),
    PixelFormat("RFC00_G03E0_B001F_narrow_R_F800", 0xF800, 0x03E0, 0x001F, 11, 5, 0),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(text: str) -> str:
    return " ".join(text.split())


def load_assembly() -> tuple[list[Instruction], dict[int, str]]:
    instructions: list[Instruction] = []
    by_address: dict[int, str] = {}
    owner = ""
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        owner_match = re.search(r"\b(sub_[0-9A-F]{6}|WinMain)\s+proc near\b", raw)
        if owner_match:
            owner = owner_match.group(1)
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        text = match.group(2).split(";", 1)[0].rstrip()
        if not text or text.endswith(":"):
            continue
        address = int(match.group(1), 16)
        text = normalize(text)
        instructions.append(Instruction(address, text, owner))
        by_address[address] = text
    return instructions, by_address


def verify_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"locked input changed: {path}: expected {expected}, got {actual}"
            )


def verify_assembly(instructions: list[Instruction], by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: expected {expected!r}, got {actual!r}"
            )
    for target, expected in TARGET_CALLS.items():
        actual = tuple(item.address for item in instructions if item.text == f"call {target}")
        if actual != expected:
            raise SystemExit(f"direct calls changed for {target}: {actual}")
    all_calls = {address for values in TARGET_CALLS.values() for address in values}
    if len(all_calls) != 50:
        raise SystemExit(f"expected 50 unique direct callsites, got {len(all_calls)}")


def build_function_rows() -> list[tuple[object, ...]]:
    return [
        (
            "0x00420490", "0x0042055A", "sub_420490", "in-place RGB signed offsets",
            "u16 pixels; dword load then u16 store", "1 pixel / iteration",
            "count==0 executes 2^32 iterations; any nonzero count executes u32(count) iterations",
            "per channel: add signed delta*unit with outside-mask test; choose mask for nonnegative overflow, zero for negative underflow",
        ),
        (
            "0x00420560", "0x004205F5", "sub_420560", "in-place red-only signed offset",
            "u16 pixels; dword load then u16 store", "1 pixel / iteration",
            "count==0 executes 2^32 iterations; any nonzero count executes u32(count) iterations",
            "same outside-mask saturation rule as sub_420490, red field only",
        ),
        (
            "0x00420600", "0x004206E4", "sub_420600", "in-place green-only signed offset",
            "two u16 pixels packed in one dword", "2 pixels / iteration",
            "positive even count is normal; zero executes 2^31 iterations; odd count never reaches zero",
            "duplicate green mask/delta into both 16-bit lanes; packed dword add/sub and per-lane outside-mask replacement",
        ),
        (
            "0x004206F0", "0x004207D4", "sub_4206F0", "in-place blue-only signed offset",
            "two u16 pixels packed in one dword", "2 pixels / iteration",
            "positive even count is normal; zero executes 2^31 iterations; odd count never reaches zero",
            "duplicate blue mask/delta into both 16-bit lanes; packed dword add/sub and per-lane outside-mask replacement",
        ),
        (
            "0x004207E0", "0x004208C5", "sub_4207E0", "two-input in-place table combine",
            "u16 source plus u16 destination", "1 pixel / iteration",
            "count==0 returns without access; nonzero executes u32(count) iterations",
            "channel scalar sum 0..31 is retained; sum 32..62 maps to zero; destination overwritten; masks are truncated and written back",
        ),
        (
            "0x00421FB0", "0x00422025", "sub_421FB0", "in-place quarter-sum grayscale",
            "u16 pixels", "1 pixel / iteration",
            "count==0 executes 2^32 iterations; any nonzero count executes u32(count) iterations",
            "q=(red_scalar+green_scalar+blue_scalar)>>2; write q into all three channel fields",
        ),
    ]


def count_contract(address: int) -> tuple[str, str]:
    if address in FULL_307200:
        return "0x4B000", "full 640x480 logical frame (307200 u16 pixels)"
    if address in PREFIX_245760:
        return "0x3C000", "245760-pixel framebuffer prefix"
    if address in DYNAMIC_COUNT:
        return "EBP", "caller-controlled positive row/buffer span"
    return "1", "single u16 pixel"


def operation_contract(target: str) -> str:
    return {
        "sub_420490": "buffer,count,red_delta,green_delta,blue_delta",
        "sub_420560": "buffer,count,red_delta",
        "sub_420600": "buffer,even_count,green_delta",
        "sub_4206F0": "buffer,even_count,blue_delta",
        "sub_4207E0": "source,destination,count; destination is overwritten",
        "sub_421FB0": "buffer,count",
    }[target]


def build_callsite_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    by_address = {item.address: item for item in instructions}
    rows = []
    for target, addresses in TARGET_CALLS.items():
        for address in addresses:
            item = by_address[address]
            count_operand, physical_scope = count_contract(address)
            rows.append(
                (
                    f"0x{address:08X}", item.owner, target, count_operand,
                    physical_scope, operation_contract(target),
                    "direct cdecl call; caller does not branch on a return value",
                )
            )
    rows.sort(key=lambda row: int(str(row[0]), 16))
    return rows


def pack(fmt: PixelFormat, red: int, green: int, blue: int) -> int:
    return (
        ((red << fmt.red_shift) & fmt.red_mask)
        | ((green << fmt.green_shift) & fmt.green_mask)
        | ((blue << fmt.blue_shift) & fmt.blue_mask)
    )


def unpack(fmt: PixelFormat, pixel: int) -> tuple[int, int, int]:
    return (
        (pixel & fmt.red_mask) >> fmt.red_shift,
        (pixel & fmt.green_mask) >> fmt.green_shift,
        (pixel & fmt.blue_mask) >> fmt.blue_shift,
    )


def offset_channel(field: int, mask: int, unit: int, delta: int) -> int:
    value = (field + ((delta * unit) & 0xFFFFFFFF)) & 0xFFFFFFFF
    outside_mask = (~mask) & 0xFFFFFFFF
    if value & outside_mask:
        return mask if delta >= 0 else 0
    return value


def offset_pixel(fmt: PixelFormat, pixel: int, deltas: tuple[int, int, int]) -> int:
    red = offset_channel(pixel & fmt.red_mask, fmt.red_mask, fmt.red_unit, deltas[0])
    green = offset_channel(pixel & fmt.green_mask, fmt.green_mask, fmt.green_unit, deltas[1])
    blue = offset_channel(pixel & fmt.blue_mask, fmt.blue_mask, fmt.blue_unit, deltas[2])
    return (red | green | blue) & 0xFFFF


def combine_pixel(fmt: PixelFormat, source: int, destination: int) -> int:
    out = []
    for mask, shift in (
        (fmt.red_mask, fmt.red_shift),
        (fmt.green_mask, fmt.green_shift),
        (fmt.blue_mask, fmt.blue_shift),
    ):
        summed = ((source & mask) >> shift) + ((destination & mask) >> shift)
        out.append(summed if summed < 32 else 0)
    return pack(fmt, out[0], out[1], out[2])


def grayscale_pixel(fmt: PixelFormat, pixel: int) -> int:
    red, green, blue = unpack(fmt, pixel)
    value = (red + green + blue) >> 2
    return pack(fmt, value, value, value)


def fmt_pixel(pixel: int) -> str:
    return f"0x{pixel & 0xFFFF:04X}"


def build_vector_rows() -> list[tuple[object, ...]]:
    rows = []
    for fmt in FORMATS:
        base = pack(fmt, 4, 10, 20)
        cases = [
            (
                "offset_plus_3", base, "delta=(3,3,3)",
                offset_pixel(fmt, base, (3, 3, 3)),
                "sub_420490 normal-domain signed addition",
            ),
            (
                "offset_minus_7", base, "delta=(-7,-7,-7)",
                offset_pixel(fmt, base, (-7, -7, -7)),
                "red underflows to zero; green/blue retain exact field subtraction",
            ),
            (
                "offset_mixed_saturation", pack(fmt, 30, 1, 31),
                "delta=(5,-5,1)",
                offset_pixel(fmt, pack(fmt, 30, 1, 31), (5, -5, 1)),
                "positive overflow chooses channel mask; negative underflow chooses zero",
            ),
            (
                "combine_below_32", pack(fmt, 5, 7, 11),
                f"destination={fmt_pixel(pack(fmt, 3, 13, 17))}",
                combine_pixel(fmt, pack(fmt, 5, 7, 11), pack(fmt, 3, 13, 17)),
                "sub_4207E0 retains each channel scalar sum below 32",
            ),
            (
                "combine_sum_32", pack(fmt, 20, 7, 31),
                f"destination={fmt_pixel(pack(fmt, 12, 25, 1))}",
                combine_pixel(fmt, pack(fmt, 20, 7, 31), pack(fmt, 12, 25, 1)),
                "all three sums are 32 and therefore index the zero-filled table half",
            ),
            (
                "quarter_sum_grayscale", pack(fmt, 31, 15, 0), "no second operand",
                grayscale_pixel(fmt, pack(fmt, 31, 15, 0)),
                "sub_421FB0 uses (31+15+0)>>2 = 11, not division by three",
            ),
        ]
        for case, input_pixel, second, output_pixel, note in cases:
            rows.append(
                (
                    fmt.name,
                    f"0x{fmt.red_mask:04X}", f"0x{fmt.green_mask:04X}",
                    f"0x{fmt.blue_mask:04X}", case, fmt_pixel(input_pixel),
                    second, fmt_pixel(output_pixel), note,
                )
            )
    return rows


def write_tsv(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    verify_inputs()
    instructions, by_address = load_assembly()
    verify_assembly(instructions, by_address)

    function_rows = build_function_rows()
    callsite_rows = build_callsite_rows(instructions)
    vector_rows = build_vector_rows()
    if len(function_rows) != 6 or len(callsite_rows) != 50 or len(vector_rows) != 24:
        raise SystemExit(
            f"unexpected row counts: functions={len(function_rows)}, "
            f"callsites={len(callsite_rows)}, vectors={len(vector_rows)}"
        )

    write_tsv(
        FUNCTION_OUTPUT,
        (
            "address", "end_address", "function", "semantic_role", "access_width",
            "loop_step", "invalid_count_behavior", "exact_integer_contract",
        ),
        function_rows,
    )
    write_tsv(
        CALLSITE_OUTPUT,
        (
            "call_address", "caller", "target", "count_operand", "physical_scope",
            "argument_contract", "return_contract",
        ),
        callsite_rows,
    )
    write_tsv(
        VECTOR_OUTPUT,
        (
            "format", "red_mask", "green_mask", "blue_mask", "case",
            "input_or_source", "second_operand", "output", "rule",
        ),
        vector_rows,
    )
    print(f"wrote {FUNCTION_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(function_rows)} rows)")
    print(f"wrote {CALLSITE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(callsite_rows)} rows)")
    print(f"wrote {VECTOR_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(vector_rows)} rows)")


if __name__ == "__main__":
    main()
