#!/usr/bin/env python3
"""Build a conservative CFG triage for SWD3 story VM opcode entries.

This is not the final opcode semantics table.  It follows the complete
assembly from each primary entry to the common interpreter join, recording
instruction-pointer mutations and direct calls that are statically reachable.
Conditional branches are deliberately over-approximated; shared entry targets
therefore remain separate opcodes but may initially have the same evidence set.
"""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
DISPATCH_INPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "story-vm-opcode-dispatch.tsv"
)
OUTPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "story-vm-opcode-static-triage.tsv"
)

EXPECTED_ASM_SHA256 = "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052"
FUNCTION_START = 0x00427920
FUNCTION_END = 0x0042D4F3
COMMON_JOIN = 0x0042B0AE

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")
DIRECT_TARGET_RE = re.compile(r"\b((?:loc|def)_[0-9A-F]{6})\b")
LABEL_RE = re.compile(r"\b((?:loc|def)_[0-9A-F]{6}):")
CALL_RE = re.compile(r"^call\s+(.+)$")


@dataclass(frozen=True)
class Instruction:
    address: int
    text: str

    @property
    def mnemonic(self) -> str:
        return self.text.split(" ", 1)[0]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(text: str) -> str:
    return " ".join(text.split())


def load_instructions() -> tuple[dict[int, Instruction], dict[int, int], dict[str, int]]:
    instructions: list[Instruction] = []
    labels: dict[str, int] = {}
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        address = int(match.group(1), 16)
        if not (FUNCTION_START <= address <= FUNCTION_END):
            continue
        label_match = LABEL_RE.search(raw)
        if label_match:
            labels[label_match.group(1)] = address
        text = match.group(2).split(";", 1)[0].rstrip()
        if not text or text.endswith(":"):
            continue
        if " = " in text or re.search(r"\bproc near\b|\bendp\b", text):
            continue
        instructions.append(Instruction(address, normalize(text)))
    by_address: dict[int, Instruction] = {}
    for instruction in instructions:
        by_address.setdefault(instruction.address, instruction)
    ordered = sorted(by_address)
    next_address = {ordered[index]: ordered[index + 1] for index in range(len(ordered) - 1)}
    if ordered[0] != FUNCTION_START or ordered[-1] != FUNCTION_END:
        raise SystemExit("story VM instruction span changed")
    return by_address, next_address, labels


def load_dispatch_rows() -> list[dict[str, str]]:
    if not DISPATCH_INPUT.exists():
        raise SystemExit("run build_story_vm_dispatch_inventory.py first")
    with DISPATCH_INPUT.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    if len(rows) != 198:
        raise SystemExit(f"unexpected primary dispatch row count: {len(rows)}")
    return rows


def internal_switch_target(address: int, opcode: int) -> int:
    if address == 0x0042B0E4:
        selectors = {
            29: 0x0042B0EB,
            30: 0x0042B0F4,
            31: 0x0042B0FD,
            32: 0x0042B13D,
            33: 0x0042B172,
            181: 0x0042B0EB,
            182: 0x0042B0F4,
            183: 0x0042B0FD,
            184: 0x0042B13D,
            185: 0x0042B172,
        }
        return selectors.get(opcode, 0x0042B1BF)
    if address == 0x0042C57B:
        selectors = {
            102: 0x0042C582,
            103: 0x0042C58C,
            117: 0x0042C596,
            136: 0x0042C5A0,
            140: 0x0042C5AA,
            145: 0x0042C5B4,
            146: 0x0042C5BE,
            174: 0x0042C5C8,
        }
        return selectors.get(opcode, 0x0042C5D0)
    raise ValueError(f"not an internal switch: 0x{address:08X}")


def successors(
    instruction: Instruction,
    opcode: int,
    next_address: dict[int, int],
    labels: dict[str, int],
) -> list[int]:
    address = instruction.address
    mnemonic = instruction.mnemonic
    if address in (0x0042B0E4, 0x0042C57B):
        return [internal_switch_target(address, opcode)]
    if mnemonic in {"retn", "ret"}:
        return []
    if mnemonic == "jmp":
        match = DIRECT_TARGET_RE.search(instruction.text)
        return [labels[match.group(1)]] if match else []
    if mnemonic.startswith("j") or mnemonic in {"loop", "loope", "loopne"}:
        result = []
        match = DIRECT_TARGET_RE.search(instruction.text)
        if match:
            result.append(labels[match.group(1)])
        if address in next_address:
            result.append(next_address[address])
        return result
    return [next_address[address]] if address in next_address else []


def ip_mutation_kind(text: str) -> tuple[str, int | None] | None:
    canonical = text.lower()
    is_primary = "[ebp+0]" in canonical
    is_ani_rebind = "[ecx+20h]" in canonical and canonical.startswith("add word ptr")
    if not is_primary and not is_ani_rebind:
        return None
    if canonical.startswith("inc word ptr"):
        return ("fixed_add", 1)
    match = re.match(r"add(?:\s+word ptr)?\s+\[[^]]+\],\s+([0-9a-f]+)h?$", canonical)
    if match:
        token = match.group(1)
        # IDA numbers containing A-F always use an h suffix; plain decimal 2/4/6/8
        # has the same value either way, while 10h/12h/0Eh must be hexadecimal.
        base = 16 if canonical.endswith("h") or any(ch in "abcdef" for ch in token) else 10
        return ("fixed_add", int(token, base))
    if canonical.startswith("add "):
        return ("dynamic_add", None)
    if canonical.startswith("mov "):
        return ("absolute_write", None)
    return ("other_write", None)


def analyze_opcode(
    opcode: int,
    entry: int,
    instructions: dict[int, Instruction],
    next_address: dict[int, int],
    labels: dict[str, int],
) -> tuple[object, ...]:
    pending = [entry]
    visited: set[int] = set()
    reached_join = False
    mutations: dict[int, tuple[str, int | None, str]] = {}
    calls: dict[int, str] = {}
    terminal_returns: set[str] = set()
    indirect_stops: dict[int, str] = {}

    while pending:
        address = pending.pop()
        if address == COMMON_JOIN:
            reached_join = True
            continue
        if address in visited:
            continue
        visited.add(address)
        instruction = instructions.get(address)
        if instruction is None:
            indirect_stops[address] = "missing_instruction"
            continue

        mutation = ip_mutation_kind(instruction.text)
        if mutation:
            mutations[address] = (mutation[0], mutation[1], instruction.text)
        call = CALL_RE.match(instruction.text)
        if call:
            calls[address] = call.group(1)
        if instruction.mnemonic in {"retn", "ret"}:
            terminal_returns.add(f"0x{address:08X}")
            continue

        next_nodes = successors(instruction, opcode, next_address, labels)
        if instruction.mnemonic == "jmp" and not next_nodes:
            indirect_stops[address] = instruction.text
        for successor in next_nodes:
            if FUNCTION_START <= successor <= FUNCTION_END:
                pending.append(successor)
            else:
                indirect_stops[successor] = "outside_function"

    fixed_values = sorted(
        {
            value
            for kind, value, _text in mutations.values()
            if kind == "fixed_add" and value is not None
        }
    )
    dynamic = sorted(
        address
        for address, (kind, _value, _text) in mutations.items()
        if kind != "fixed_add"
    )
    if not mutations:
        triage = "no_ip_mutation_reached"
    elif dynamic:
        triage = "dynamic_or_absolute_ip_mutation"
    elif len(mutations) == 1:
        triage = "single_fixed_advance_site"
    else:
        triage = "multiple_fixed_mutation_sites_or_variable_text_scan"

    mutation_text = "|".join(
        f"0x{address:08X}:{kind}:{text}"
        for address, (kind, _value, text) in sorted(mutations.items())
    )
    call_text = "|".join(
        f"0x{address:08X}:{target}" for address, target in sorted(calls.items())
    )
    return (
        opcode,
        f"0x{entry:08X}",
        len(visited),
        "yes" if reached_join else "no",
        ",".join(terminal_returns),
        mutation_text,
        ",".join(str(value) for value in fixed_values),
        ",".join(f"0x{address:08X}" for address in dynamic),
        call_text,
        "|".join(
            f"0x{address:08X}:{reason}" for address, reason in sorted(indirect_stops.items())
        ),
        triage,
        "CFG over-approximation; conditional feasibility and semantics require assembly audit",
    )


def write_tsv(rows: list[tuple[object, ...]]) -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "effective_opcode", "entry_target", "reachable_instruction_count",
                "reaches_common_join", "direct_return_sites", "ip_mutation_sites",
                "fixed_add_values", "dynamic_or_absolute_sites", "direct_calls",
                "unresolved_edges", "triage", "scope_limit",
            )
        )
        writer.writerows(rows)


def main() -> None:
    actual_hash = sha256(ASM_PATH)
    if actual_hash != EXPECTED_ASM_SHA256:
        raise SystemExit(f"assembly changed: {actual_hash}")
    instructions, next_address, labels = load_instructions()
    dispatch_rows = load_dispatch_rows()
    rows = []
    for dispatch in dispatch_rows:
        opcode = int(dispatch["effective_opcode_dec"])
        entry = int(dispatch["entry_target"], 16)
        rows.append(analyze_opcode(opcode, entry, instructions, next_address, labels))
    if len(rows) != 198:
        raise SystemExit("unexpected static triage row count")
    if any(row[9] for row in rows):
        unresolved = [(row[0], row[9]) for row in rows if row[9]]
        raise SystemExit(f"unresolved CFG edges: {unresolved[:8]}")
    write_tsv(rows)
    counts: dict[str, int] = {}
    for row in rows:
        counts[str(row[10])] = counts.get(str(row[10]), 0) + 1
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(rows)} rows)")
    for key in sorted(counts):
        print(f"{key}: {counts[key]}")


if __name__ == "__main__":
    main()
