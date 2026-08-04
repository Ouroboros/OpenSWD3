#!/usr/bin/env python3
"""Build LST-locked inventories for blitter modes 0x0C and 0x20.

The output records the complete direct flag-construction set for these two
transform modes and the exact 10.10 row-mapping contracts used by their RLE
kernels.  The complete IDA LST is the only disassembly input; pseudocode and
the generated ASM file are not read or used as evidence.
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
LST_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.lst"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
CALLSITE_PATH = INVENTORY_ROOT / "blitter-transform-callsites.tsv"
ROW_MAPPING_PATH = INVENTORY_ROOT / "blitter-row-resampling.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    LST_PATH: "701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b",
}

LST_LINE_RE = re.compile(r"^\.[^:]+:([0-9A-F]{8})\s")


@dataclass(frozen=True)
class TransformCall:
    mode: int
    caller: str
    mask_address: int | None
    mask_instruction: str
    build_address: int
    build_instruction: str
    call_address: int
    flag_contract: str
    possible_slots: str
    slot_status: str
    scale_contract: str


MODE_0C_CALLS = [
    ("sub_4145F0", 0x0041461F, "and esi, 8000000Fh", 0x0041462A, "or esi, 0Ch", 0x004146D6),
    ("sub_4721F0", 0x004723A0, "and eax, 8000000Fh", 0x004723A5, "or al, 0Ch", 0x004723E3),
    ("sub_4724D0", 0x00472634, "and eax, 8000000Fh", 0x0047263A, "or al, 0Ch", 0x00472676),
    ("sub_4728E0", 0x00472AB5, "and eax, 8000000Fh", 0x00472ABA, "or al, 0Ch", 0x00472AFC),
    ("sub_472CE0", 0x00472E51, "and eax, 8000000Fh", 0x00472E58, "or al, 0Ch", 0x00472E91),
    ("sub_4731A0", 0x0047333B, "and eax, 8000000Fh", 0x00473340, "or al, 0Ch", 0x00473383),
    ("sub_4735B0", 0x00473754, "and eax, 8000000Fh", 0x0047375C, "or al, 0Ch", 0x0047379C),
    ("sub_4751C0", 0x00475354, "and eax, 8000000Fh", 0x0047535B, "or al, 0Ch", 0x0047539B),
    ("sub_478B60", 0x0047926C, "and eax, 8000000Fh", 0x00479272, "or al, 0Ch", 0x004792A9),
    ("sub_47BA80", 0x0047BED4, "and eax, 8000000Fh", 0x0047BEDA, "or al, 0Ch", 0x0047BF00),
    ("sub_47F710", 0x0047F81F, "and eax, 8000000Fh", 0x0047F826, "or al, 0Ch", 0x0047F860),
    ("sub_482310", 0x0048275C, "and eax, 8000000Fh", 0x00482762, "or al, 0Ch", 0x004827A4),
    ("sub_482840", 0x00482C74, "and eax, 8000000Fh", 0x00482C7A, "or al, 0Ch", 0x00482CBC),
    ("sub_4831C0", 0x004834BB, "and eax, 8000000Fh", 0x004834C1, "or al, 0Ch", 0x00483508),
    ("sub_484230", 0x0048441D, "and ecx, 8000000Fh", 0x00484425, "or ecx, 0Ch", 0x0048446C),
]

EXPECTED_SNIPPETS = {
    # Dispatcher target-height and vertical 10.10 setup.
    0x00417243: "cmp edi, eax",
    0x00417247: "shl eax, 0Ah",
    0x0041724B: "idiv edi",
    0x0041724D: "mov dword_4CD2FC, 1",
    0x00417257: "mov dword_4CD744, eax",
    0x0041725E: "shl eax, 0Ah",
    0x00417262: "idiv edi",
    0x00417264: "sub eax, 400h",
    0x00417269: "mov dword_4CD744, eax",
    # Target-height clipping keeps the original minus-one boundaries.
    0x0041728F: "lea edi, [edi+esi-1]",
    0x004172A3: "sub ecx, esi",
    0x004172A7: "mov dword_4CD754, ecx",
    0x004172B9: "cmp ebx, edx",
    0x004172BB: "jl short loc_4172DF",
    0x004172C5: "lea edi, [ecx+eax-1]",
    # Dispatcher per-row X displacement setup used by mode 0x0C.
    0x004172E8: "mov eax, 1",
    0x004172ED: "mov dword_4CD718, eax",
    0x004172F4: "mov dword_4CC2F4, 1",
    0x00417300: "mov dword_4CC2F4, 0FFFFFFFFh",
    0x00417332: "shl eax, 0Ah",
    0x00417336: "idiv ecx",
    0x0041733C: "mov dword_4CD738, eax",
    # Mode 0x0C forward: unconditional prepass and per-output X phase.
    0x0041F9F2: "mov eax, [esi]",
    0x0041F963: "mov [ebp+var_8], 0",
    0x0041FA02: "add esi, eax",
    0x0041FA0D: "add eax, dword_4CD744",
    0x0041FA68: "mov edx, dword_4CD718",
    0x0041FA6E: "add ecx, dword_4CD738",
    0x0041FA81: "shr ebx, 0Ah",
    0x0041FA84: "imul ebx",
    0x0041FA89: "add dword_4CD718, eax",
    0x0041FA95: "cmp ebx, ecx",
    0x0041FB0F: "add ecx, dword_4CD738",
    0x0041FB2A: "add dword_4CD718, eax",
    0x0041FB46: "add edx, dword_4CD718",
    0x0041FDF9: "add eax, dword_4CD744",
    # Mode 0x20 forward: top clip is checked before source-row advance.
    0x004209BE: "cmp ebx, ecx",
    0x004209C2: "mov eax, [esi]",
    0x004209C4: "inc ebx",
    0x004209CA: "add esi, eax",
    0x004209D2: "mov eax, [ebp+var_8]",
    0x004209D5: "add eax, dword_4CD744",
    0x004209E5: "mov [ebp+var_8], eax",
    0x00420A37: "inc ebx",
    0x00420CCB: "add eax, dword_4CD744",
    0x00420E83: "inc ebx",
    0x00420E91: "mov eax, [ebp+var_8]",
    0x00420EA4: "mov [ebp+var_8], eax",
    0x00420EF6: "inc ebx",
    # Mode 0x20 call paths.
    0x00452B67: "push 20h",
    0x00452B8F: "mov dword_4CD75C, ecx",
    0x00452B95: "call sub_4170E0",
    0x00479991: "and eax, 80000003h",
    0x00479A98: "and eax, 80000023h",
    0x00479A9F: "or al, 20h",
    0x00479AEC: "call sub_4170E0",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(instruction: str) -> str:
    return " ".join(instruction.split())


def load_listing() -> tuple[dict[int, str], dict[int, str]]:
    instructions: dict[int, str] = {}
    owners: dict[int, str] = {}
    current_owner = ""
    for raw in LST_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        owner_match = re.search(r"\b(sub_[0-9A-F]{6})\s+proc near\b", raw)
        if owner_match:
            current_owner = owner_match.group(1)
        match = LST_LINE_RE.match(raw)
        if not match or len(raw) <= 43:
            continue

        byte_field = raw[15:43].strip()
        if not byte_field or not re.match(r"^[0-9A-F?]{2}(?:\s|$)", byte_field):
            continue

        address = int(match.group(1), 16)
        instruction = raw[43:].split(";", 1)[0].rstrip()
        if not instruction:
            continue
        instructions[address] = normalize(instruction)
        owners[address] = current_owner
    return instructions, owners


def verify_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(f"locked input changed: {path} expected {expected}, got {actual}")


def expect(instructions: dict[int, str], address: int, expected: str) -> None:
    actual = instructions.get(address)
    if actual != expected:
        raise SystemExit(
            f"instruction mismatch at 0x{address:08X}: expected {expected!r}, got {actual!r}"
        )


def verify_vertical_fraction_initialization(
    instructions: dict[int, str]
) -> None:
    def accesses(start: int, end: int) -> list[tuple[int, str]]:
        return [
            (address, instruction)
            for address, instruction in instructions.items()
            if start <= address <= end and "[ebp+var_8]" in instruction
        ]

    shifted_forward = accesses(0x0041F8D0, 0x0041FE9A)
    if not shifted_forward or shifted_forward[0] != (
        0x0041F963,
        "mov [ebp+var_8], 0",
    ):
        raise SystemExit(
            f"mode 0x0C vertical fraction is not explicitly zeroed: {shifted_forward[:2]}"
        )

    expected_first_accesses = {
        (0x004208D0, 0x00420D61): [
            (0x004209D2, "mov eax, [ebp+var_8]"),
            (0x004209E5, "mov [ebp+var_8], eax"),
        ],
        (0x00420D70, 0x0042122B): [
            (0x00420E91, "mov eax, [ebp+var_8]"),
            (0x00420EA4, "mov [ebp+var_8], eax"),
        ],
    }
    for (start, end), expected in expected_first_accesses.items():
        actual = accesses(start, end)
        if actual[:2] != expected:
            raise SystemExit(
                f"mode 0x20 vertical-fraction first-access mismatch at "
                f"0x{start:08X}: expected={expected}, actual={actual[:2]}"
            )


def build_transform_calls(
    instructions: dict[int, str], owners: dict[int, str]
) -> list[TransformCall]:
    recovered_0c_builds = {
        address
        for address, instruction in instructions.items()
        if re.fullmatch(r"or (?:al|cl|dl|eax|ecx|edx|esi), 0Ch", instruction)
    }
    expected_0c_builds = {row[3] for row in MODE_0C_CALLS}
    if recovered_0c_builds != expected_0c_builds:
        raise SystemExit(
            "mode 0x0C flag-build set mismatch: "
            f"expected={sorted(expected_0c_builds)}, actual={sorted(recovered_0c_builds)}"
        )

    rows: list[TransformCall] = []
    for caller, mask_address, mask_instruction, build_address, build_instruction, call_address in MODE_0C_CALLS:
        expect(instructions, mask_address, mask_instruction)
        expect(instructions, build_address, build_instruction)
        expect(instructions, call_address, "call sub_4170E0")
        if owners[mask_address] != caller or owners[build_address] != caller or owners[call_address] != caller:
            raise SystemExit(f"mode 0x0C owner mismatch for {caller}")
        rows.append(
            TransformCall(
                mode=0x0C,
                caller=caller,
                mask_address=mask_address,
                mask_instruction=mask_instruction,
                build_address=build_address,
                build_instruction=build_instruction,
                call_address=call_address,
                flag_contract="(runtime_flags & 0x8000000F) | 0x0C",
                possible_slots="0x0C,0x0D,0x0E,0x0F",
                slot_status="all four assigned",
                scale_contract="target height and initial per-row X displacement are written before call",
            )
        )

    rows.append(
        TransformCall(
            mode=0x20,
            caller="sub_4527E0",
            mask_address=None,
            mask_instruction="",
            build_address=0x00452B67,
            build_instruction="push 20h",
            call_address=0x00452B95,
            flag_contract="immediate 0x20",
            possible_slots="0x20",
            slot_status="assigned",
            scale_contract="target height = 0x280 + odd transition counter; source rectangle is 640x480",
        )
    )
    rows.append(
        TransformCall(
            mode=0x20,
            caller="sub_479850",
            mask_address=0x00479A98,
            mask_instruction="and eax, 80000023h",
            build_address=0x00479A9F,
            build_instruction="or al, 20h",
            call_address=0x00479AEC,
            flag_contract="((runtime_flags & 0x80000003) & 0x80000023) | 0x20",
            possible_slots="0x20,0x21,0x22,0x23",
            slot_status="0x20/0x21 assigned; normal initialized sources prove retained bit1 zero, so 0x22/0x23 are malformed-state only",
            scale_contract="target height = source height + transition counter",
        )
    )

    if len(rows) != 17:
        raise SystemExit(f"expected 17 transform calls, got {len(rows)}")
    return rows


def write_transform_calls(rows: list[TransformCall]) -> None:
    CALLSITE_PATH.parent.mkdir(parents=True, exist_ok=True)
    with CALLSITE_PATH.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(
            [
                "mode",
                "caller",
                "mask_address",
                "mask_instruction",
                "build_address",
                "build_instruction",
                "call_address",
                "flag_contract",
                "possible_low_slots",
                "slot_status",
                "scale_contract",
            ]
        )
        for row in rows:
            writer.writerow(
                [
                    f"0x{row.mode:02X}",
                    row.caller,
                    f"0x{row.mask_address:08X}" if row.mask_address is not None else "",
                    row.mask_instruction,
                    f"0x{row.build_address:08X}",
                    row.build_instruction,
                    f"0x{row.call_address:08X}",
                    row.flag_contract,
                    row.possible_slots,
                    row.slot_status,
                    row.scale_contract,
                ]
            )


def write_row_mapping() -> None:
    rows = [
        (
            "both",
            "sub_4170E0",
            "target-height gate",
            "0x00417130-0x00417243",
            "Ht=dword_4CD75C; Hs=arg_C; zero Ht uses the ordinary non-resampled clipping path",
            "Ht is reset to zero after every dispatcher call",
        ),
        (
            "both",
            "sub_4170E0",
            "target-height vertical clipping",
            "0x0041726E-0x004172D9",
            "top overlap uses visible=Ht+destination_y-clip_top-1 with top_skip=clip_top-destination_y; bottom overlap uses visible=clip_height-destination_y+clip_top-1 and equality enters clipping",
            "an exact target-height fit against the clip bottom draws Ht-1 rows; this minus-one behavior is not shared by the zero-target path",
        ),
        (
            "both",
            "sub_4170E0",
            "vertical step when Ht>Hs",
            "0x00417243-0x00417257",
            "enlarge=1; vstep=(Hs<<10)/Ht using signed idiv",
            "processed output rows advance floor((vfrac+vstep)/1024) source rows, so rows can repeat",
        ),
        (
            "both",
            "sub_4170E0",
            "vertical step when Ht<=Hs",
            "0x0041725E-0x00417269",
            "enlarge=0; vstep=((Hs<<10)/Ht)-0x400 using signed idiv",
            "processed output rows advance 1+floor((vfrac+vstep)/1024) source rows, so rows can be skipped",
        ),
        (
            "0x0C",
            "sub_4170E0",
            "X displacement setup",
            "0x004172DF-0x0041733C",
            "zero displacement is forced to +1; xdir=-1 when displacement>0 else +1; xstep=(abs(displacement)<<10)/(Ht!=0?Ht:Hs)",
            "the original +1 substitution is observable and must not be normalized back to zero",
        ),
        (
            "0x0C",
            "sub_41F8D0/sub_41FEA0",
            "initial prepass",
            "0x0041F9F2-0x0041FA9F / 0x0041FFDF-0x0042008C",
            "runs top_clip+1 iterations unconditionally; each advances one source row plus the vertical quotient and advances the X fraction once",
            "even top_clip=0 discards/moves past at least one RLE row before the first output",
        ),
        (
            "0x0C",
            "sub_41F8D0/sub_41FEA0",
            "per-output X displacement",
            "0x0041FB06-0x0041FB46 / 0x004200F0-0x00420130",
            "xfrac=(xfrac+xstep)&0x3FF; displacement += xdir*floor((old_xfrac+xstep)/1024); row X=arg_0+displacement",
            "X is recomputed and clipped independently for every output row; source pixels are not horizontally resampled",
        ),
        (
            "0x0C",
            "sub_41F8D0/sub_41FEA0",
            "literal pixel operation",
            "0x0041FC95-0x0041FCE6 / 0x0042029D-0x004202EE",
            "consume literal payload positions but ignore their u16 values; dst=per_channel_clamp(dst+signed_color_offset)",
            "this is a vertically mapped and row-shifted coverage mask, not scaled source-color add",
        ),
        (
            "0x20",
            "sub_4208D0/sub_420D70",
            "initial vertical fraction",
            "0x004208D0-0x004209E5 / 0x00420D70-0x00420EA4",
            "the first [ebp-8] access is a read; neither function initializes the full-width fraction before adding vstep and retaining its low 10 bits",
            "unlike mode 0x0C, zero is not assembly-proven; a deterministic port must expose or explicitly isolate the original stack residue",
        ),
        (
            "0x20",
            "sub_4208D0/sub_420D70",
            "top clipping",
            "0x004209BE-0x00420A3A / 0x00420E7D-0x00420EF9",
            "checks skipped_count==top_clip before advancing; each prepass first advances one row, then shrink adds q rows while enlarge adds one iff q!=0; the counter increments once before mapping and again when the post-map value does not equal top_clip",
            "top_clip=0 starts at the first RLE row; positive top clipping runs ceil(top_clip/2) prepasses, an original double-increment bug",
        ),
        (
            "0x20",
            "sub_4208D0/sub_420D70",
            "processed-row mapping",
            "0x00420CB1-0x00420D30 / 0x0042117B-0x004211FA",
            "after drawing, shrink advances 1+q rows while enlarge repeats for q=0 and advances exactly one row for any nonzero q; destination X and literal run lengths are not resampled",
            "only source RLE row selection changes",
        ),
        (
            "0x20",
            "sub_4208D0/sub_420D70",
            "literal pixel operation",
            "0x00420B1A-0x00420BB5 / 0x00420FD1-0x004210D0",
            "src'=per_channel_clamp(src+signed_color_offset); dst=per_channel_saturating_add(dst,src')",
            "pixel formula matches mode 0x04; only vertical source-row selection differs",
        ),
    ]
    with ROW_MAPPING_PATH.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(
            ["mode", "functions", "concern", "assembly_range", "exact_contract", "fidelity_note"]
        )
        writer.writerows(rows)


def main() -> None:
    verify_inputs()
    instructions, owners = load_listing()
    for address, expected in EXPECTED_SNIPPETS.items():
        expect(instructions, address, expected)
    verify_vertical_fraction_initialization(instructions)
    rows = build_transform_calls(instructions, owners)
    write_transform_calls(rows)
    write_row_mapping()
    print(
        "wrote 17 transform call paths and 12 row-mapping contracts; "
        "mode 0x0C has 15 safe low-slot constructions; dynamic mode 0x20 "
        "retains bit1, which the normal initialized caller chain proves clear"
    )


if __name__ == "__main__":
    main()
