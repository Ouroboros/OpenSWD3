#!/usr/bin/env python3
"""Build assembly-locked reachability evidence for dynamic blitter flags.

The narrow target is bit 1 of the battle actor runtime field at +0x2694.
`sub_479850` retains that bit while constructing mode 0x20, which would select
the empty dispatcher slots 0x22/0x23.  This tool proves the normal initialized
current-program invariant without treating IDA pseudocode as evidence.
"""

from __future__ import annotations

import csv
import hashlib
import re
import struct
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
ACTION_EFFECTS_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "act-command-field-effects.tsv"
)
ACTION_EXTERNAL_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "action-external-accesses.tsv"
)
OUTPUT_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "blitter-dynamic-flag-reachability.tsv"
)

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    ACTION_EFFECTS_PATH: "217113354cd98f6a6b9e54840d375f986e5b25bbaf0a0ee0be19825b79ffce9d",
    ACTION_EXTERNAL_PATH: "3528154ab8221b74f4027ebf50ae1adaf4214c01cf4ac16cf65f6d6decf1e30e",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")
MEMORY_DESTINATION_RE = re.compile(
    r"^(?:mov|or|and|xor|add|sub|inc|dec) "
    r"(?:byte ptr |word ptr |dword ptr )?\[[^]]+\+2694h\](?:, .+)?$"
)

EXPECTED_RUNTIME_WRITES = {
    0x00479024,
    0x00479064,
    0x004790B8,
    0x004790EC,
    0x00479996,
    0x00479AA1,
    0x0047BF15,
    0x0047C7D0,
    0x0047C7ED,
    0x0047C80B,
    0x0047C82E,
    0x0047CA94,
    0x0047CAB9,
    0x0047CAD7,
    0x0047CAFA,
}

EXPECTED_ACTION_MODE_WRITES = {
    0x004322AE,
    0x0043235C,
    0x00432404,
    0x00432518,
    0x0043252C,
    0x00432540,
    0x004326DC,
    0x004326F5,
    0x00432725,
    0x004328EA,
}

EXPECTED_SNIPPETS = {
    # ActionRecord +0x18 roots and transitions.
    0x0040F286: "lea edi, [esi+40h]",
    0x0040F289: "mov dword ptr [esi+58h], 0",
    0x004321E7: "mov ebp, 1",
    0x0043224D: "and eax, 80000000h",
    0x004322AE: "mov [esi+18h], eax",
    0x004322BF: "mov ecx, 80000003h",
    0x004322FE: "and eax, ecx",
    0x0043235C: "mov [esi+18h], eax",
    0x004323A6: "and eax, ecx",
    0x00432404: "mov [esi+18h], eax",
    0x0043250F: "and ecx, 8000000Bh",
    0x00432515: "or ecx, 8",
    0x00432523: "and edx, 80000007h",
    0x00432529: "or edx, 4",
    0x00432537: "and ecx, 8000002Fh",
    0x0043253D: "or ecx, 2Ch",
    0x004326DC: "and dword ptr [esi+18h], 0FFFFFFFEh",
    0x004326EC: "and edx, 80000013h",
    0x004326F2: "or edx, 10h",
    0x0043271C: "and ecx, 80000017h",
    0x00432722: "or ecx, 14h",
    0x004328EA: "or [esi+18h], ebp",
    # Battle ActionRecord[0] and [3] roots/copies.
    0x00478625: "lea eax, [ebx+0CB8h]",
    0x0047862B: "lea esi, [ebx+2A0h]",
    0x00478631: "mov ecx, 26h",
    0x00478638: "rep movsd",
    0x004786FA: "lea edi, [edx+2A0h]",
    0x00478700: "rep stosd",
    0x004789D9: "lea edi, [ebx+468h]",
    0x004789DF: "rep stosd",
    0x00478FB5: "lea edi, [ebp+468h]",
    0x00478FC0: "lea esi, [ebp+2A0h]",
    0x00478FC6: "rep movsd",
    0x0047CC1B: "lea edi, [esi+468h]",
    0x0047CC21: "rep stosd",
    0x0047D35C: "lea edi, [esi+2A0h]",
    0x0047D362: "rep stosd",
    0x0047D383: "lea edi, [esi+468h]",
    0x0047D389: "rep stosd",
    0x0047D916: "lea edi, [edx+2A0h]",
    0x0047D91C: "rep stosd",
    0x0048204F: "lea edi, [edx+2A0h]",
    0x00482055: "rep stosd",
    # Runtime +0x2694 sources, transforms, and dangerous mode-0x20 build.
    0x00479019: "mov ecx, [ebp+2B8h]",
    0x00479024: "mov [ebp+2694h], ecx",
    0x00479054: "test al, 1",
    0x00479058: "and al, 0FEh",
    0x00479062: "or eax, esi",
    0x004790B2: "test al, 2Ch",
    0x004790B6: "or al, 4",
    0x004790DC: "test al, 1",
    0x004790E0: "and al, 0FEh",
    0x004790EA: "or eax, esi",
    0x00479991: "and eax, 80000003h",
    0x00479996: "mov [esi+2694h], eax",
    0x00479A98: "and eax, 80000023h",
    0x00479A9F: "or al, 20h",
    0x00479AA1: "mov [esi+2694h], eax",
    0x00479AEC: "call sub_4170E0",
    0x0047BF0C: "and eax, 80000003h",
    0x0047BF15: "mov [ebp+2694h], eax",
    0x0047C7C8: "mov eax, [esi+480h]",
    0x0047C7D0: "mov [esi+2694h], eax",
    0x0047C80B: "or dword ptr [esi+2694h], 4",
    0x0047CA81: "mov eax, [esi+480h]",
    0x0047CA94: "mov [esi+2694h], eax",
    0x0047CAD7: "or dword ptr [esi+2694h], 4",
}


@dataclass(frozen=True)
class EvidenceRow:
    stage: str
    state: str
    source_or_transform: str
    bit1_effect: str
    assembly_addresses: str
    normal_reachability_conclusion: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(instruction: str) -> str:
    return " ".join(instruction.split())


def load_assembly() -> dict[int, str]:
    instructions: dict[int, str] = {}
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        instruction = match.group(2).split(";", 1)[0].rstrip()
        if not instruction or instruction.endswith(":"):
            continue
        instructions[int(match.group(1), 16)] = normalize(instruction)
    return instructions


def verify_locked_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"locked input changed: {path} expected {expected}, got {actual}"
            )


def verify_pe_zero_fill() -> tuple[int, int]:
    data = EXE_PATH.read_bytes()
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise SystemExit("invalid PE signature")
    coff_offset = pe_offset + 4
    section_count = struct.unpack_from("<H", data, coff_offset + 2)[0]
    optional_size = struct.unpack_from("<H", data, coff_offset + 16)[0]
    optional_offset = coff_offset + 20
    image_base = struct.unpack_from("<I", data, optional_offset + 28)[0]
    section_offset = optional_offset + optional_size

    sections: dict[str, tuple[int, int, int, int]] = {}
    for index in range(section_count):
        offset = section_offset + index * 40
        name = data[offset : offset + 8].split(b"\0", 1)[0].decode("ascii")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, offset + 8
        )
        sections[name] = (virtual_size, virtual_address, raw_size, raw_offset)

    expected_data = (0x000A07DC, 0x0009E000, 0x0000C000, 0x0009E000)
    if sections.get(".data") != expected_data or image_base != 0x00400000:
        raise SystemExit(
            f"unexpected PE .data mapping: image_base={image_base:#x}, "
            f"section={sections.get('.data')}"
        )

    virtual_size, virtual_address, raw_size, _ = expected_data
    zero_fill_start = image_base + virtual_address + raw_size
    zero_fill_end = image_base + virtual_address + virtual_size
    actor_ranges = (
        (0x005029D0, 10 * 0x2F34),
        (0x00525508, 8 * 0x2B28),
    )
    for base, size in actor_ranges:
        if not (zero_fill_start <= base and base + size <= zero_fill_end):
            raise SystemExit(
                f"battle actor array {base:#x}..{base + size:#x} is not in .data zero-fill"
            )
    return zero_fill_start, zero_fill_end


def verify_assembly(instructions: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = instructions.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: "
                f"expected {expected!r}, got {actual!r}"
            )

    runtime_writes = {
        address
        for address, instruction in instructions.items()
        if MEMORY_DESTINATION_RE.fullmatch(instruction)
    }
    if runtime_writes != EXPECTED_RUNTIME_WRITES:
        raise SystemExit(
            "runtime +0x2694 write set changed: "
            f"expected={sorted(EXPECTED_RUNTIME_WRITES)}, actual={sorted(runtime_writes)}"
        )

    action_mode_writes = {
        address
        for address, instruction in instructions.items()
        if 0x004321E0 <= address < 0x00432A16
        and re.match(
            r"^(?:mov|or|and|xor|add|sub|inc|dec) "
            r"(?:dword ptr )?\[esi\+18h\](?:,|$)",
            instruction,
        )
    }
    if action_mode_writes != EXPECTED_ACTION_MODE_WRITES:
        raise SystemExit(
            "ActionRecord +0x18 write set changed: "
            f"expected={sorted(EXPECTED_ACTION_MODE_WRITES)}, "
            f"actual={sorted(action_mode_writes)}"
        )

    direct_slot0_refs = {
        address
        for address, instruction in instructions.items()
        if re.search(r"\[[a-z]+\+2B8h\]", instruction)
    }
    if direct_slot0_refs != {0x00479019, 0x0047904E, 0x004790D6}:
        raise SystemExit(f"unexpected direct ActionRecord[0]+0x18 refs: {direct_slot0_refs}")

    direct_slot3_refs = {
        address
        for address, instruction in instructions.items()
        if re.search(r"\[[a-z]+\+480h\]", instruction)
    }
    expected_slot3_refs = {
        0x00471646,
        0x0047646A,
        0x004796AD,
        0x0047C749,
        0x0047C7C8,
        0x0047C9E7,
        0x0047CA81,
    }
    if direct_slot3_refs != expected_slot3_refs:
        raise SystemExit(f"unexpected direct ActionRecord[3]+0x18 refs: {direct_slot3_refs}")


def verify_action_effect_inventory() -> None:
    with ACTION_EFFECTS_PATH.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    mode_rows = [row for row in rows if row["action_offset_hex"] == "18"]
    expected_transforms = {
        "(old & 0x8000000B) | 0x08",
        "(old & 0x80000007) | 0x04",
        "(old & 0x8000002F) | 0x2C",
        "(old & 0x80000013) | 0x10",
        "(old & 0x80000017) | 0x14",
        "old & ~0x01",
        "old | 0x01",
    }
    transforms = {row["write_transform"] for row in mode_rows}
    if len(mode_rows) != 7 or transforms != expected_transforms:
        raise SystemExit(f"unexpected ACT ActionRecord+0x18 transforms: {mode_rows}")
    # Every command mask retains 0x02, while the only direct OR is 0x01.
    # Therefore a zero bit 1 remains zero under every ACT command transition.


def verify_external_access_inventory() -> None:
    with ACTION_EXTERNAL_PATH.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    mode_rows = [row for row in rows if row["action_offset_hex"] == "18"]
    if len(mode_rows) != 11 or {row["access_kind"] for row in mode_rows} != {"read"}:
        raise SystemExit(
            "the proven external ActionRecord+0x18 access set is no longer 11 reads"
        )


def build_rows(zero_fill_start: int, zero_fill_end: int) -> list[EvidenceRow]:
    return [
        EvidenceRow(
            "initialization",
            "battle actor storage",
            f"PE .data zero-fill 0x{zero_fill_start:08X}..0x{zero_fill_end:08X}",
            "initial bit1 = 0",
            "PE section table",
            "both static battle actor arrays and runtime +0x2694 start at zero",
        ),
        EvidenceRow(
            "initialization",
            "role ActionRecord+0x18",
            "explicit zero immediately before updater",
            "bit1 cleared",
            "0x0040F286,0x0040F289,0x0040F291",
            "role path cannot seed bit1",
        ),
        EvidenceRow(
            "initialization",
            "battle ActionRecord[0]+0x18",
            "whole-record exact clears",
            "bit1 cleared",
            "0x004786FA-0x00478700,0x0047D35C-0x0047D362,0x0047D916-0x0047D91C,0x0048204F-0x00482055",
            "primary battle draw source starts/restarts with bit1 zero",
        ),
        EvidenceRow(
            "initialization",
            "battle ActionRecord[3]+0x18",
            "whole-record exact clears",
            "bit1 cleared",
            "0x004789D9-0x004789DF,0x0047D383-0x0047D389,0x0047CC1B-0x0047CC21",
            "secondary battle draw source starts/restarts with bit1 zero",
        ),
        EvidenceRow(
            "copy",
            "ActionRecord[0] -> ActionRecord[3]",
            "copy exactly 0x26 dwords",
            "preserves source bit1",
            "0x00478FB5-0x00478FC6",
            "cannot create bit1 because source invariant is zero",
        ),
        EvidenceRow(
            "copy",
            "ActionRecord[0] -> ActionRecord[17]",
            "copy exactly 0x26 dwords; updater then receives slot 17",
            "preserves source bit1",
            "0x00478625-0x0047863B",
            "slot 17 is a sink, not an unknown source feeding slot 0",
        ),
        EvidenceRow(
            "action update",
            "lookup-key resets",
            "old&0x80000000 or old&0x80000003",
            "clears or preserves bit1; never sets it",
            "0x0043224D-0x00432404",
            "zero bit1 remains zero across all three reset paths",
        ),
        EvidenceRow(
            "action update",
            "HA/MA/NA/DL/SG/ON/IV commands",
            "seven exact in-place transforms",
            "all preserve bit1; none sets it",
            "0x0043250C-0x004328EA",
            "ACT command execution cannot create bit1 from zero",
        ),
        EvidenceRow(
            "external audit",
            "proven ActionRecord+0x18 external accesses",
            "11 reads, zero writes",
            "no external producer found in proven base intervals",
            "action-external-accesses.tsv",
            "no legitimate external bit1 setter is currently known",
        ),
        EvidenceRow(
            "runtime source",
            "battle actor +0x2694 from ActionRecord[0]+0x18",
            "direct dword copy from +0x2B8",
            "preserves bit1",
            "0x00479019-0x00479024",
            "normal source bit1 is zero",
        ),
        EvidenceRow(
            "runtime source",
            "battle actor +0x2694 from ActionRecord[3]+0x18",
            "two direct dword copies from +0x480",
            "preserves bit1",
            "0x0047C7C8-0x0047C7D0,0x0047CA81-0x0047CA94",
            "normal source bit1 is zero",
        ),
        EvidenceRow(
            "runtime transform",
            "all 15 direct +0x2694 writes",
            "source copies, bit0 toggles, mode-4 ORs, and masks",
            "no writer introduces bit1",
            ",".join(f"0x{address:08X}" for address in sorted(EXPECTED_RUNTIME_WRITES)),
            "runtime bit1 remains zero from PE initialization and action sources",
        ),
        EvidenceRow(
            "mode 0x20 dispatch",
            "sub_479850 dynamic flag build",
            "(runtime_flags & 0x80000023) | 0x20",
            "retains bit1 but does not set it",
            "0x00479A92-0x00479AEC",
            "normal initialized execution reaches only slots 0x20/0x21; 0x22/0x23 are unreachable",
        ),
        EvidenceRow(
            "malformed state",
            "forced runtime bit1 = 1",
            "memory corruption, injected state, or deliberately non-equivalent initialization",
            "would select 0x22/0x23",
            "dispatcher table contract",
            "original empty-slot/null-call behavior must not be replaced by a fallback",
        ),
    ]


def write_rows(rows: list[EvidenceRow]) -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "stage",
                "state",
                "source_or_transform",
                "bit1_effect",
                "assembly_addresses",
                "normal_reachability_conclusion",
            )
        )
        for row in rows:
            writer.writerow(
                (
                    row.stage,
                    row.state,
                    row.source_or_transform,
                    row.bit1_effect,
                    row.assembly_addresses,
                    row.normal_reachability_conclusion,
                )
            )


def main() -> None:
    verify_locked_inputs()
    instructions = load_assembly()
    zero_fill_start, zero_fill_end = verify_pe_zero_fill()
    verify_assembly(instructions)
    verify_action_effect_inventory()
    verify_external_access_inventory()
    rows = build_rows(zero_fill_start, zero_fill_end)
    write_rows(rows)
    relative_output = OUTPUT_PATH.relative_to(WORKSPACE_ROOT)
    print(
        f"wrote {len(rows)} dynamic-flag evidence rows to {relative_output}; "
        "normal mode-0x20 slots are 0x20/0x21 and 0x22/0x23 are unreachable"
    )


if __name__ == "__main__":
    main()
