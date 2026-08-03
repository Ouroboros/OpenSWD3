#!/usr/bin/env python3
"""Extract field accesses made by sub_4321E0 to its action-record argument."""

from __future__ import annotations

import csv
import re
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASSEMBLY = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
OUTPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "action-subrecord-accesses.tsv"
)

FUNCTION_START = "004321E0 sub_4321E0      proc near"
FUNCTION_END = "00432A16 sub_4321E0      endp"
ROLE_OFFSET = 0x40
EXPECTED_SIZE = 0x98

INSTRUCTION_RE = re.compile(r"^([0-9A-F]{8})\s+([a-z][a-z0-9]*)\s*(.*)$")
FIELD_RE = re.compile(r"\[esi(?:\+([0-9A-F]+)h?)?\]", re.IGNORECASE)
PTR_WIDTH_RE = re.compile(r"\b(byte|word|dword|qword) ptr\b", re.IGNORECASE)
PTR_WIDTHS = {"byte": 1, "word": 2, "dword": 4, "qword": 8}
REGISTER_WIDTHS = {
    **{name: 1 for name in ("al", "ah", "bl", "bh", "cl", "ch", "dl", "dh")},
    **{name: 2 for name in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp")},
    **{name: 4 for name in ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")},
}


@dataclass
class Accesses:
    widths: set[int] = field(default_factory=set)
    reads: set[str] = field(default_factory=set)
    writes: set[str] = field(default_factory=set)
    read_writes: set[str] = field(default_factory=set)
    instruction_count: int = 0


def split_operands(text: str) -> list[str]:
    return [operand.strip() for operand in text.split(",") if operand.strip()]


def infer_width(operand: str, operands: list[str]) -> int:
    ptr_width = PTR_WIDTH_RE.search(operand)
    if ptr_width:
        return PTR_WIDTHS[ptr_width.group(1).lower()]

    for other_operand in operands:
        if other_operand == operand:
            continue
        tokens = re.findall(r"\b[a-z][a-z0-9]*\b", other_operand.lower())
        for token in tokens:
            if token in REGISTER_WIDTHS:
                return REGISTER_WIDTHS[token]

    raise SystemExit(f"cannot infer memory width: {operand!r} in {operands!r}")


def classify(mnemonic: str, operand_index: int) -> str:
    if operand_index > 0 or mnemonic in {"cmp", "test", "push"}:
        return "read"
    if mnemonic in {"mov", "movzx", "movsx", "seta", "setae", "setb", "setbe", "sete", "setne"}:
        return "write"
    return "read_write"


def main() -> None:
    lines = ASSEMBLY.read_text(encoding="utf-8", errors="replace").replace("\r", "").splitlines()
    try:
        start = next(index for index, line in enumerate(lines) if line.startswith(FUNCTION_START))
        end = next(
            index
            for index, line in enumerate(lines[start:], start)
            if line.startswith(FUNCTION_END)
        )
    except StopIteration as error:
        raise SystemExit("sub_4321E0 boundary not found") from error

    fields: dict[int, Accesses] = defaultdict(Accesses)
    for line in lines[start : end + 1]:
        code = line.split(";", 1)[0].rstrip()
        instruction = INSTRUCTION_RE.match(code)
        if not instruction:
            continue
        address, mnemonic, operand_text = instruction.groups()
        operands = split_operands(operand_text)
        for operand_index, operand in enumerate(operands):
            field_match = FIELD_RE.search(operand)
            if not field_match:
                continue
            offset = int(field_match.group(1) or "0", 16)
            width = infer_width(operand, operands)
            access = classify(mnemonic, operand_index)
            record = fields[offset]
            record.widths.add(width)
            getattr(record, {"read": "reads", "write": "writes", "read_write": "read_writes"}[access]).add(address)
            record.instruction_count += 1

    if len(fields) != 49:
        raise SystemExit(f"unexpected action field count: {len(fields)}")
    extent = max(offset + max(record.widths) for offset, record in fields.items())
    if extent != EXPECTED_SIZE:
        raise SystemExit(f"unexpected action record extent: 0x{extent:X}")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "action_offset_hex",
                "role_offset_hex",
                "access_width_bytes",
                "read_instruction_addresses",
                "write_instruction_addresses",
                "read_write_instruction_addresses",
                "instruction_reference_count",
            )
        )
        for offset, record in sorted(fields.items()):
            writer.writerow(
                (
                    f"{offset:04X}",
                    f"{ROLE_OFFSET + offset:04X}",
                    ",".join(str(width) for width in sorted(record.widths)),
                    ",".join(sorted(record.reads)),
                    ",".join(sorted(record.writes)),
                    ",".join(sorted(record.read_writes)),
                    record.instruction_count,
                )
            )

    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(
        f"wrote {len(fields)} action fields spanning 0x{extent:X} bytes to {relative_output}"
    )


if __name__ == "__main__":
    main()
