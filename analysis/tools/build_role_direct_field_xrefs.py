#!/usr/bin/env python3
"""Extract direct-symbol xrefs for the first 0xD8-byte role record."""

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
    / "role-direct-field-xrefs.tsv"
)

ROLE_BASE = 0x004BABA8
ROLE_END = ROLE_BASE + 0xD8
TEXT_END = 0x00499000

DECLARATION_RE = re.compile(
    r"^([0-9A-F]{8})\s+([A-Za-z][A-Za-z0-9_]*_4B[A-F0-9]+)\s+(db|dw|dd|dq)\b"
)
FUNCTION_RE = re.compile(r"^([0-9A-F]{8})\s+(\S+)\s+proc near\b")
CHUNK_RE = re.compile(r"FUNCTION CHUNK AT ([0-9A-F]{8}) SIZE ([0-9A-F]{8}) BYTES")
INSTRUCTION_RE = re.compile(r"^([a-z][a-z0-9]*)\s*(.*)$")
WIDTHS = {"db": 1, "dw": 2, "dd": 4, "dq": 8}


@dataclass(frozen=True)
class Function:
    address: int
    name: str

    @property
    def label(self) -> str:
        return f"{self.address:08X}:{self.name}"


@dataclass
class FieldXrefs:
    readers: set[str] = field(default_factory=set)
    writers: set[str] = field(default_factory=set)
    read_writers: set[str] = field(default_factory=set)
    address_takers: set[str] = field(default_factory=set)
    instruction_count: int = 0


def parse_operands(text: str) -> list[str]:
    return [operand.strip() for operand in text.split(",")]


def classify(mnemonic: str, operands: list[str], operand_index: int, operand: str) -> str:
    if mnemonic == "lea" or re.search(r"\boffset\b", operand):
        return "address"
    if operand_index > 0:
        return "read"
    if mnemonic in {"cmp", "test", "push", "call"}:
        return "read"
    if mnemonic in {"mov", "movzx", "movsx", "seta", "setae", "setb", "setbe", "sete", "setne"}:
        return "write"
    return "read_write"


def main() -> None:
    lines = ASSEMBLY.read_text(encoding="utf-8", errors="replace").replace("\r", "").splitlines()

    declarations: dict[str, tuple[int, int]] = {}
    chunks: list[tuple[int, int, Function]] = []
    current_function: Function | None = None

    for line in lines:
        function_match = FUNCTION_RE.match(line)
        if function_match:
            current_function = Function(int(function_match.group(1), 16), function_match.group(2))
        chunk_match = CHUNK_RE.search(line)
        if chunk_match and current_function is not None:
            start = int(chunk_match.group(1), 16)
            size = int(chunk_match.group(2), 16)
            chunks.append((start, start + size, current_function))
        declaration_match = DECLARATION_RE.match(line)
        if declaration_match:
            address = int(declaration_match.group(1), 16)
            if ROLE_BASE <= address < ROLE_END:
                declarations[declaration_match.group(2)] = (
                    address,
                    WIDTHS[declaration_match.group(3)],
                )

    if len(declarations) != 27:
        raise SystemExit(f"unexpected direct role field count: {len(declarations)}")

    symbol_re = re.compile(r"\b(" + "|".join(map(re.escape, declarations)) + r")\b")
    xrefs = {symbol: FieldXrefs() for symbol in declarations}
    current_function = None

    for line in lines:
        function_match = FUNCTION_RE.match(line)
        if function_match:
            current_function = Function(int(function_match.group(1), 16), function_match.group(2))

        if len(line) < 8 or not line[:8].isalnum():
            continue
        try:
            instruction_address = int(line[:8], 16)
        except ValueError:
            continue
        if instruction_address >= TEXT_END:
            continue

        owner = current_function
        for start, end, chunk_owner in chunks:
            if start <= instruction_address < end:
                owner = chunk_owner
                break
        if owner is None:
            continue

        code = line[8:].split(";", 1)[0].strip()
        instruction_match = INSTRUCTION_RE.match(code)
        if not instruction_match:
            continue
        mnemonic = instruction_match.group(1)
        operands = parse_operands(instruction_match.group(2))

        for symbol_match in symbol_re.finditer(code):
            symbol = symbol_match.group(1)
            for operand_index, operand in enumerate(operands):
                if re.search(rf"\b{re.escape(symbol)}\b", operand):
                    access = classify(mnemonic, operands, operand_index, operand)
                    record = xrefs[symbol]
                    target_set = {
                        "read": record.readers,
                        "write": record.writers,
                        "read_write": record.read_writers,
                        "address": record.address_takers,
                    }[access]
                    target_set.add(owner.label)
                    record.instruction_count += 1
                    break

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "offset_hex",
                "absolute_address",
                "declared_width_bytes",
                "ida_symbol",
                "readers",
                "writers",
                "read_writers",
                "address_takers",
                "instruction_reference_count",
            )
        )
        for symbol, (address, width) in sorted(declarations.items(), key=lambda item: item[1][0]):
            record = xrefs[symbol]
            writer.writerow(
                (
                    f"{address - ROLE_BASE:04X}",
                    f"{address:08X}",
                    width,
                    symbol,
                    ",".join(sorted(record.readers)),
                    ",".join(sorted(record.writers)),
                    ",".join(sorted(record.read_writers)),
                    ",".join(sorted(record.address_takers)),
                    record.instruction_count,
                )
            )

    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(f"wrote {len(declarations)} direct role fields to {relative_output}")


if __name__ == "__main__":
    main()
