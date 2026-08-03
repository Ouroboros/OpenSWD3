#!/usr/bin/env python3
"""Extract direct data-symbol effects from each story opcode CFG.

This is a mechanical triage layer, not semantic naming.  It records direct
reads, writes, read/writes, and address references before the common interpreter
join.  Indirect effects performed by callees and feasibility of ordinary
conditional branches remain for assembly audit.
"""

from __future__ import annotations

import csv
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path


TOOL_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOL_ROOT))
import build_story_vm_opcode_static_triage as triage  # noqa: E402


RESEARCH_ROOT = TOOL_ROOT.parent
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
SUMMARY_OUTPUT = INVENTORY_ROOT / "story-vm-opcode-direct-effects.tsv"
SITE_OUTPUT = INVENTORY_ROOT / "story-vm-opcode-direct-effect-sites.tsv"

DATA_SYMBOL_RE = re.compile(
    r"\b(?:"
    r"(?:dword|word|byte|qword|unk|off|asc|jpt)_[0-9A-F]+"
    r"|a[A-Z][A-Za-z0-9_]*"
    r"|FileName|ExistingFileName|Buffer|ArgList|hWnd"
    r")\b"
)
CALL_RE = re.compile(r"^call\s+(.+)$")

READ_ONLY = {
    "cmp", "test", "push", "fld", "fild", "fldz", "fld1", "div", "idiv",
    "mul", "call",
}
READ_WRITE_DEST = {
    "add", "adc", "sub", "sbb", "and", "or", "xor", "inc", "dec", "neg",
    "not", "shl", "shr", "sal", "sar", "rol", "ror", "rcl", "rcr", "bts",
    "btr", "btc", "xadd",
}
WRITE_DEST = {
    "mov", "movzx", "movsx", "pop", "sete", "setz", "setne", "setnz", "seta",
    "setae", "setb", "setbe", "setg", "setge", "setl", "setle", "setns", "sets",
    "seto", "setno", "setp", "setnp", "setc", "setnc", "fst", "fstp", "fist",
    "fistp",
}


def split_operands(text: str) -> tuple[str, list[str]]:
    parts = text.split(" ", 1)
    mnemonic = parts[0].lower()
    if len(parts) == 1:
        return mnemonic, []
    return mnemonic, [part.strip() for part in parts[1].split(",")]


def symbols(operand: str) -> set[str]:
    return set(DATA_SYMBOL_RE.findall(operand))


def operand_access(mnemonic: str, index: int, operand: str) -> str:
    lower = operand.lower()
    if "offset " in lower or mnemonic == "lea":
        return "address_ref"
    if mnemonic in READ_ONLY:
        return "read"
    if mnemonic == "xchg":
        return "read_write"
    if mnemonic in READ_WRITE_DEST:
        return "read_write" if index == 0 else "read"
    if mnemonic in WRITE_DEST:
        return "write" if index == 0 else "read"
    if mnemonic in {"imul"}:
        return "read_write" if index == 0 else "read"
    # Unknown instructions with explicit data symbols are conservatively reads;
    # the site inventory keeps the original instruction for manual correction.
    return "read"


def reachable_addresses(
    opcode: int, entry: int, instructions: dict[int, triage.Instruction],
    next_address: dict[int, int], labels: dict[str, int],
) -> set[int]:
    pending = [entry]
    visited: set[int] = set()
    while pending:
        address = pending.pop()
        if address == triage.COMMON_JOIN or address in visited:
            continue
        visited.add(address)
        instruction = instructions.get(address)
        if instruction is None or instruction.mnemonic in {"ret", "retn"}:
            continue
        for successor in triage.successors(instruction, opcode, next_address, labels):
            if triage.FUNCTION_START <= successor <= triage.FUNCTION_END:
                pending.append(successor)
    return visited


def merge_accesses(values: set[str]) -> str:
    if "read_write" in values or ({"read", "write"} <= values):
        return "read_write"
    if "write" in values:
        return "write"
    if "read" in values:
        return "read"
    return "address_ref"


def write_tsv(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    if triage.sha256(triage.ASM_PATH) != triage.EXPECTED_ASM_SHA256:
        raise SystemExit("locked full assembly input changed")
    instructions, next_address, labels = triage.load_instructions()
    dispatch_rows = triage.load_dispatch_rows()

    site_rows: list[tuple[object, ...]] = []
    summary_rows: list[tuple[object, ...]] = []
    for dispatch in dispatch_rows:
        opcode = int(dispatch["effective_opcode_dec"])
        entry = int(dispatch["entry_target"], 16)
        addresses = reachable_addresses(opcode, entry, instructions, next_address, labels)
        per_symbol: dict[str, set[str]] = defaultdict(set)
        calls: dict[int, str] = {}
        for address in sorted(addresses):
            instruction = instructions[address]
            call = CALL_RE.match(instruction.text)
            if call:
                calls[address] = call.group(1)
            mnemonic, operands = split_operands(instruction.text)
            site_accesses: dict[str, set[str]] = defaultdict(set)
            for index, operand in enumerate(operands):
                access = operand_access(mnemonic, index, operand)
                for symbol in symbols(operand):
                    site_accesses[symbol].add(access)
            for symbol, accesses in sorted(site_accesses.items()):
                access = merge_accesses(accesses)
                per_symbol[symbol].add(access)
                site_rows.append((
                    opcode, f"0x{entry:08X}", f"0x{address:08X}",
                    access, symbol, instruction.text,
                    "direct access in opcode-specific conservative CFG",
                ))

        aggregate: dict[str, str] = {
            symbol: merge_accesses(accesses) for symbol, accesses in per_symbol.items()
        }
        reads = sorted(symbol for symbol, access in aggregate.items() if access == "read")
        writes = sorted(symbol for symbol, access in aggregate.items() if access == "write")
        read_writes = sorted(symbol for symbol, access in aggregate.items() if access == "read_write")
        refs = sorted(symbol for symbol, access in aggregate.items() if access == "address_ref")
        summary_rows.append((
            opcode, f"0x{entry:08X}", len(addresses), len(aggregate),
            "|".join(reads), "|".join(writes), "|".join(read_writes), "|".join(refs),
            "|".join(f"0x{address:08X}:{target}" for address, target in sorted(calls.items())),
            "direct accesses only; callee effects excluded; ordinary branch feasibility over-approximated",
        ))

    if len(summary_rows) != 198:
        raise SystemExit("unexpected opcode effect summary row count")
    site_rows.sort(key=lambda row: (int(row[0]), int(str(row[2]), 16), str(row[4]), str(row[3])))
    write_tsv(SUMMARY_OUTPUT, (
        "effective_opcode", "entry_target", "reachable_instruction_count",
            "direct_symbol_count", "read_symbols", "write_symbols",
            "read_write_symbols", "address_refs", "call_sites", "scope_limit",
    ), summary_rows)
    write_tsv(SITE_OUTPUT, (
        "effective_opcode", "entry_target", "instruction_address", "access",
        "symbol", "instruction", "scope",
    ), site_rows)
    print(f"wrote {SUMMARY_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(summary_rows)} rows)")
    print(f"wrote {SITE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(site_rows)} rows)")
    print("site access classes:", dict(sorted(Counter(str(row[3]) for row in site_rows).items())))
    print("opcodes with direct writes:", sum(bool(row[5] or row[6]) for row in summary_rows))
    print("opcodes with call sites:", sum(bool(row[8]) for row in summary_rows))


if __name__ == "__main__":
    main()
