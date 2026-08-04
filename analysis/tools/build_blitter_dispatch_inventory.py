#!/usr/bin/env python3
"""Build assembly-first inventories for the 16-bit software blitter.

The dispatch slots are recovered from sub_416D90.  The 16 opacity steps are
recovered from the scalar forward jump table in sub_41D340 and from the
instructions in each selected basic block.  Pseudocode is intentionally not
used as evidence.
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
DISPATCH_PATH = INVENTORY_ROOT / "blitter-dispatch.tsv"
ALPHA_PATH = INVENTORY_ROOT / "blitter-opacity-steps.tsv"
COVERAGE_PATH = INVENTORY_ROOT / "blitter-coverage-composite-steps.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")
MOV_REG_TARGET_RE = re.compile(r"mov\s+(eax|ecx),\s+offset\s+(sub_[0-9A-F]{6})$")
MOV_SLOT_REG_RE = re.compile(r"mov\s+(dword_[0-9A-F]{6}),\s+(eax|ecx)$")
MOV_SLOT_TARGET_RE = re.compile(
    r"mov\s+(dword_[0-9A-F]{6}),\s+offset\s+(sub_[0-9A-F]{6})$"
)
LEA_TARGET_RE = re.compile(r"lea\s+(eax|esi),\s+((?:loc|sub)_[0-9A-F]{6})$")
MOV_TABLE_REG_RE = re.compile(r"mov\s+\[edi(?:\+4)?\],\s+(eax|esi)$")
MOV_PIXEL_RE = re.compile(r"mov\s+(eax|ebx),\s+\[(edi|esi)\]$")
SHR_RE = re.compile(r"shr\s+(eax|ebx),\s+([1-4])$")
MASK_RE = re.compile(r"and\s+(eax|ebx),\s+(dword_4CD78[8C]|dword_4CD79[04])$")

DISPATCH_BASE = 0x004CD318


@dataclass(frozen=True)
class Mode:
    name: str
    status: str
    special_run: str


MODES = {
    0x00: Mode("literal copy", "closed", "skip"),
    0x04: Mode("per-channel saturated add", "closed", "skip"),
    0x08: Mode("coverage composite or raw constant vertical fade", "partial", "0x8000 is processed"),
    0x0C: Mode("row-resampled and row-shifted destination RGB offset under source coverage", "closed", "skip"),
    0x10: Mode("destination RGB offset under source coverage", "closed", "skip"),
    0x14: Mode("15-step packed-field opacity", "closed", "skip"),
    0x18: Mode("literal copy with run-edge green-mask replacement", "closed", "skip"),
    0x1C: Mode("RLE vertical opacity fade", "closed", "skip"),
    0x20: Mode("vertically row-resampled per-channel saturated add", "closed", "skip"),
    0x24: Mode("constant-color fill under source coverage", "closed", "skip"),
    0x28: Mode("destination grayscale under source coverage", "closed", "skip"),
    0x2C: Mode("per-channel saturated subtract", "closed", "skip"),
    0x30: Mode("16-step destination-neighbor smear", "closed", "skip"),
}

EXPECTED_DISPATCH = {
    0: "sub_418350", 1: "sub_4185C0", 2: "sub_418350", 3: "sub_4185C0",
    4: "sub_418840", 5: "sub_418EB0",
    8: "sub_419570", 9: "sub_41A3B0",
    12: "sub_41F8D0", 13: "sub_41FEA0", 14: "sub_41F8D0", 15: "sub_41FEA0",
    16: "sub_41B280", 17: "sub_41B620",
    20: "sub_41D340", 21: "sub_41E5C0", 22: "sub_41D340", 23: "sub_41E5C0",
    24: "sub_41CCF0", 25: "sub_41D010",
    28: "sub_41B9F0",
    32: "sub_4208D0", 33: "sub_420D70",
    36: "sub_421230", 37: "sub_421540", 38: "sub_421230", 39: "sub_421540",
    40: "sub_421850", 41: "sub_421BE0", 42: "sub_421850", 43: "sub_421BE0",
    44: "sub_422030", 45: "sub_4223A0",
    48: "sub_422730", 49: "sub_4229C0", 50: "sub_422730", 51: "sub_4229C0",
    128: "sub_4176D0", 129: "sub_4177D0",
    132: "sub_417840", 133: "sub_417E40",
    136: "sub_417EC0",
    148: "sub_417950",
}

SEMANTIC_OVERRIDES = {
    25: "literal copy with right edge plus one pixel left of the copied run (original reverse off-by-one bug)",
    132: "raw/uncompressed color-key copy",
    133: "raw/uncompressed color-key copy with 16/32-bit reverse comparison bug",
}

EXPECTED_ALPHA_TARGETS = [
    "loc_41DAAA", "loc_41D790", "loc_41D7C9", "loc_41D802",
    "loc_41D83B", "loc_41D874", "loc_41D8AD", "loc_41D8E6",
    "loc_41D91F", "loc_41D958", "loc_41D991", "loc_41D9CA",
    "loc_41DA03", "loc_41DA39", "loc_41DA6F", "loc_41DAA5",
]

EXPECTED_COVERAGE_TARGETS = [
    "loc_419BE0", "loc_4199F9", "loc_419A29", "loc_419A59",
    "loc_419A7C", "loc_419AAC", "loc_419ACF", "loc_419AF2",
    "loc_419B08", "loc_419B39", "loc_419B5D", "loc_419B7E",
    "loc_419B92", "loc_419BB3", "loc_419BC7", "loc_419BDB",
]

EXPECTED_RAW_VERTICAL_FADE_TARGETS = [
    "sub_41830E", "sub_417FF4", "sub_41802D", "sub_418066",
    "sub_41809F", "sub_4180D8", "sub_418111", "sub_41814A",
    "sub_418183", "sub_4181BC", "sub_4181F5", "sub_41822E",
    "sub_418267", "sub_41829D", "sub_4182D3", "sub_418309",
]

EXPECTED_RLE_VERTICAL_FADE_TARGETS = [
    "loc_41C1CC", "loc_41BEB2", "loc_41BEEB", "loc_41BF24",
    "loc_41BF5D", "loc_41BF96", "loc_41BFCF", "loc_41C008",
    "loc_41C041", "loc_41C07A", "loc_41C0B3", "loc_41C0EC",
    "loc_41C125", "loc_41C15B", "loc_41C191", "loc_41C1C7",
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(f"locked input changed: {path} expected {expected}, got {actual}")


def assembly_lines() -> list[tuple[int, str]]:
    rows: list[tuple[int, str]] = []
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASM_LINE_RE.match(raw)
        if match:
            instruction = match.group(2).split(";", 1)[0].rstrip()
            rows.append((int(match.group(1), 16), instruction))
    return rows


def recover_dispatch(lines: list[tuple[int, str]]) -> dict[int, tuple[str, int]]:
    registers: dict[str, str] = {}
    recovered: dict[int, tuple[str, int]] = {}
    for address, instruction in lines:
        if not 0x00416D90 <= address <= 0x00416F0E:
            continue
        match = MOV_REG_TARGET_RE.fullmatch(instruction)
        if match:
            registers[match.group(1)] = match.group(2)
            continue
        match = MOV_SLOT_REG_RE.fullmatch(instruction)
        if match:
            symbol_address = int(match.group(1).split("_")[1], 16)
            target = registers.get(match.group(2))
            if target is None:
                raise SystemExit(f"unresolved register assignment at 0x{address:08X}")
        else:
            match = MOV_SLOT_TARGET_RE.fullmatch(instruction)
            if not match:
                continue
            symbol_address = int(match.group(1).split("_")[1], 16)
            target = match.group(2)
        delta = symbol_address - DISPATCH_BASE
        if delta < 0 or delta % 4:
            raise SystemExit(f"invalid dispatch slot address 0x{symbol_address:08X}")
        slot = delta // 4
        if slot in recovered:
            raise SystemExit(f"duplicate dispatch slot {slot}")
        recovered[slot] = (target, address)
    actual = {slot: target for slot, (target, _) in recovered.items()}
    if actual != EXPECTED_DISPATCH:
        raise SystemExit(f"dispatch table mismatch\nexpected={EXPECTED_DISPATCH}\nactual={actual}")
    return recovered


def recover_alpha_targets(lines: list[tuple[int, str]]) -> list[str]:
    registers: dict[str, str] = {}
    targets: list[str] = []
    for address, instruction in lines:
        if not 0x0041D42F <= address <= 0x0041D4D7:
            continue
        match = LEA_TARGET_RE.fullmatch(instruction)
        if match:
            registers[match.group(1)] = match.group(2)
            continue
        match = MOV_TABLE_REG_RE.fullmatch(instruction)
        if match:
            target = registers.get(match.group(1))
            if target is None:
                raise SystemExit(f"unresolved opacity-table register at 0x{address:08X}")
            targets.append(target)
    if targets != EXPECTED_ALPHA_TARGETS:
        raise SystemExit(f"opacity target table mismatch: {targets}")
    return targets


def recover_coverage_targets(lines: list[tuple[int, str]]) -> list[str]:
    registers: dict[str, str] = {}
    targets: list[str] = []
    for address, instruction in lines:
        if not 0x0041966D <= address <= 0x00419715:
            continue
        match = LEA_TARGET_RE.fullmatch(instruction)
        if match:
            registers[match.group(1)] = match.group(2)
            continue
        match = MOV_TABLE_REG_RE.fullmatch(instruction)
        if match:
            target = registers.get(match.group(1))
            if target is None:
                raise SystemExit(f"unresolved coverage-table register at 0x{address:08X}")
            targets.append(target)
    if targets != EXPECTED_COVERAGE_TARGETS:
        raise SystemExit(f"coverage target table mismatch: {targets}")
    return targets


def recover_target_table(
    lines: list[tuple[int, str]], start: int, end: int, expected: list[str], name: str
) -> list[str]:
    registers: dict[str, str] = {}
    targets: list[str] = []
    for address, instruction in lines:
        if not start <= address <= end:
            continue
        match = LEA_TARGET_RE.fullmatch(instruction)
        if match:
            registers[match.group(1)] = match.group(2)
            continue
        match = MOV_TABLE_REG_RE.fullmatch(instruction)
        if match:
            target = registers.get(match.group(1))
            if target is None:
                raise SystemExit(f"unresolved {name} register at 0x{address:08X}")
            targets.append(target)
    if targets != expected:
        raise SystemExit(f"{name} target table mismatch: {targets}")
    return targets


def recover_alpha_terms(
    lines: list[tuple[int, str]], targets: list[str]
) -> dict[int, list[tuple[str, int, str]]]:
    addresses = [int(target.split("_")[1], 16) for target in targets]
    terms: dict[int, list[tuple[str, int, str]]] = {0: [], 15: [("src", 0, "exact")]}
    for step in range(1, 15):
        start = addresses[step]
        end = addresses[step + 1]
        registers: dict[str, str] = {}
        shifts: dict[str, int] = {}
        block_terms: list[tuple[str, int, str]] = []
        for address, instruction in lines:
            if not start <= address < end:
                continue
            match = MOV_PIXEL_RE.fullmatch(instruction)
            if match:
                registers[match.group(1)] = "dst" if match.group(2) == "edi" else "src"
                shifts.pop(match.group(1), None)
                continue
            match = SHR_RE.fullmatch(instruction)
            if match:
                shifts[match.group(1)] = int(match.group(2))
                continue
            match = MASK_RE.fullmatch(instruction)
            if match:
                register = match.group(1)
                source = registers.get(register)
                shift = shifts.get(register)
                if source is None or shift is None:
                    raise SystemExit(f"incomplete opacity term at 0x{address:08X}")
                expected_mask = {
                    1: "dword_4CD788",
                    2: "dword_4CD78C",
                    3: "dword_4CD790",
                    4: "dword_4CD794",
                }[shift]
                if match.group(2) != expected_mask:
                    raise SystemExit(f"opacity shift/mask mismatch at 0x{address:08X}")
                block_terms.append((source, shift, match.group(2)))
        if len(block_terms) != 4:
            raise SystemExit(f"opacity step {step} has {len(block_terms)} terms, expected 4")
        terms[step] = block_terms
    return terms


def recover_coverage_destination_terms(
    lines: list[tuple[int, str]], targets: list[str]
) -> dict[int, list[tuple[int, str]]]:
    addresses = [int(target.split("_")[1], 16) for target in targets]
    terms: dict[int, list[tuple[int, str]]] = {0: [], 15: []}
    for step in range(1, 15):
        start = addresses[step]
        end = addresses[step + 1]
        registers: dict[str, str] = {}
        shifts: dict[str, int] = {}
        destination_terms: list[tuple[int, str]] = []
        exact_source_reads = 0
        for address, instruction in lines:
            if not start <= address < end:
                continue
            match = MOV_PIXEL_RE.fullmatch(instruction)
            if match:
                source = "dst" if match.group(2) == "edi" else "src"
                registers[match.group(1)] = source
                shifts.pop(match.group(1), None)
                if source == "src":
                    exact_source_reads += 1
                continue
            match = SHR_RE.fullmatch(instruction)
            if match:
                shifts[match.group(1)] = int(match.group(2))
                continue
            match = MASK_RE.fullmatch(instruction)
            if match:
                register = match.group(1)
                if registers.get(register) != "dst":
                    raise SystemExit(f"coverage table masks a non-destination term at 0x{address:08X}")
                shift = shifts.get(register)
                if shift is None:
                    raise SystemExit(f"coverage table missing shift at 0x{address:08X}")
                expected_mask = {
                    1: "dword_4CD788",
                    2: "dword_4CD78C",
                    3: "dword_4CD790",
                    4: "dword_4CD794",
                }[shift]
                if match.group(2) != expected_mask:
                    raise SystemExit(f"coverage shift/mask mismatch at 0x{address:08X}")
                destination_terms.append((shift, match.group(2)))
        if exact_source_reads != 1:
            raise SystemExit(
                f"coverage step {step} has {exact_source_reads} exact source reads, expected 1"
            )
        destination_units = sum(16 >> shift for shift, _ in destination_terms)
        if destination_units != 15 - step:
            raise SystemExit(
                f"coverage step {step} destination units {destination_units}, expected {15-step}"
            )
        terms[step] = destination_terms
    return terms


def direction_for_slot(slot: int) -> str:
    if slot & 1:
        return "horizontal reverse"
    return "horizontal forward"


def write_dispatch(recovered: dict[int, tuple[str, int]]) -> None:
    fieldnames = [
        "slot", "slot_hex", "raw_family", "mode", "mode_hex", "flip_bits",
        "direction", "function", "assignment_address", "semantic_family",
        "semantic_status", "special_run_0x8000",
    ]
    with DISPATCH_PATH.open("w", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, delimiter="\t", fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        for slot, (function, assignment_address) in sorted(recovered.items()):
            raw_family = bool(slot & 0x80)
            visible_slot = slot & 0x7F
            mode_value = visible_slot & 0x7C
            mode = MODES.get(mode_value, Mode("raw/unclassified", "open", "unknown"))
            writer.writerow({
                "slot": slot,
                "slot_hex": f"0x{slot:02X}",
                "raw_family": int(raw_family),
                "mode": mode_value,
                "mode_hex": f"0x{mode_value:02X}",
                "flip_bits": visible_slot & 3,
                "direction": direction_for_slot(visible_slot),
                "function": function,
                "assignment_address": f"0x{assignment_address:08X}",
                "semantic_family": SEMANTIC_OVERRIDES.get(
                    slot,
                    mode.name if not raw_family else f"raw/uncompressed {mode.name}",
                ),
                "semantic_status": mode.status,
                "special_run_0x8000": mode.special_run if not raw_family else "not RLE",
            })


def write_alpha(targets: list[str], terms: dict[int, list[tuple[str, int, str]]]) -> None:
    fieldnames = [
        "step", "jump_target", "source_binary_units", "destination_binary_units",
        "combined_weight_sixteenths", "operation", "term_sequence", "rounding",
    ]
    with ALPHA_PATH.open("w", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, delimiter="\t", fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        for step in range(16):
            if step == 0:
                src_units, dst_units = 0, 16
                operation = "leave destination unchanged"
            elif step == 15:
                src_units, dst_units = 16, 0
                operation = "copy source exactly"
            else:
                src_units = sum(16 >> shift for source, shift, _ in terms[step] if source == "src")
                dst_units = sum(16 >> shift for source, shift, _ in terms[step] if source == "dst")
                if src_units != step or dst_units != 15 - step:
                    raise SystemExit(
                        f"opacity step {step} unexpected units src={src_units} dst={dst_units}"
                    )
                operation = "sum independently shifted and component-masked packed fields"
            sequence = ";".join(
                f"{source}>>{shift}&{mask}" if mask != "exact" else "src exact"
                for source, shift, mask in terms[step]
            )
            writer.writerow({
                "step": step,
                "jump_target": targets[step],
                "source_binary_units": src_units,
                "destination_binary_units": dst_units,
                "combined_weight_sixteenths": src_units + dst_units,
                "operation": operation,
                "term_sequence": sequence,
                "rounding": "none",
            })


def write_coverage(
    targets: list[str], terms: dict[int, list[tuple[int, str]]]
) -> None:
    fieldnames = [
        "coverage_byte", "jump_target", "source_operation",
        "destination_binary_units", "destination_weight_sixteenths",
        "operation", "destination_term_sequence", "rounding",
    ]
    with COVERAGE_PATH.open("w", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, delimiter="\t", fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        for step in range(16):
            if step == 0:
                source_operation = "not read"
                destination_units = 16
                operation = "leave destination unchanged"
            elif step == 15:
                source_operation = "copy exact"
                destination_units = 0
                operation = "copy source exactly"
            else:
                source_operation = "add exact packed source"
                destination_units = sum(16 >> shift for shift, _ in terms[step])
                operation = "add exact source to independently shifted and masked destination fields"
            sequence = ";".join(
                f"dst>>{shift}&{mask}" for shift, mask in terms[step]
            )
            writer.writerow({
                "coverage_byte": step,
                "jump_target": targets[step],
                "source_operation": source_operation,
                "destination_binary_units": destination_units,
                "destination_weight_sixteenths": f"{destination_units}/16",
                "operation": operation,
                "destination_term_sequence": sequence,
                "rounding": "none",
            })


def main() -> None:
    verify_inputs()
    lines = assembly_lines()
    recovered = recover_dispatch(lines)
    targets = recover_alpha_targets(lines)
    terms = recover_alpha_terms(lines, targets)
    coverage_targets = recover_coverage_targets(lines)
    coverage_terms = recover_coverage_destination_terms(lines, coverage_targets)
    raw_fade_targets = recover_target_table(
        lines, 0x00417EED, 0x00417F95,
        EXPECTED_RAW_VERTICAL_FADE_TARGETS, "raw vertical-fade",
    )
    raw_fade_terms = recover_alpha_terms(lines, raw_fade_targets)
    rle_fade_targets = recover_target_table(
        lines, 0x0041BAFA, 0x0041BBA2,
        EXPECTED_RLE_VERTICAL_FADE_TARGETS, "RLE vertical-fade",
    )
    rle_fade_terms = recover_alpha_terms(lines, rle_fade_targets)
    if raw_fade_terms != terms:
        raise SystemExit("raw vertical-fade formulas differ from the 0x14 opacity formulas")
    if rle_fade_terms != terms:
        raise SystemExit("RLE vertical-fade formulas differ from the 0x14 opacity formulas")
    INVENTORY_ROOT.mkdir(parents=True, exist_ok=True)
    write_dispatch(recovered)
    write_alpha(targets, terms)
    write_coverage(coverage_targets, coverage_terms)
    print(
        f"wrote {len(recovered)} dispatch slots, {len(targets)} opacity steps, and "
        f"{len(coverage_targets)} coverage-composite steps; opacity steps 1..14 preserve "
        "the original 15/16 combined packed-field weight; both vertical-fade tables match"
    )


if __name__ == "__main__":
    main()
