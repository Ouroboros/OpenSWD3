#!/usr/bin/env python3
"""Inventory every direct call to the software-blitter dispatcher.

The call ABI is recovered from the assembly stack order.  The dispatcher reads
six arguments, but some callers push extra ignored dwords; therefore the six
pushes nearest each CALL are the actual argument slots.
"""

from __future__ import annotations

import csv
import hashlib
import re
from collections import Counter
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
OUTPUT_PATH = (
    RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "blitter-callsites.tsv"
)

EXPECTED_ASM_SHA256 = "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052"
EXPECTED_CALLSITES = 248
EXPECTED_CALLERS = 101

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")
PROC_RE = re.compile(r"^(sub_[0-9A-F]{6})\s+proc\b")
ENDP_RE = re.compile(r"^sub_[0-9A-F]{6}\s+endp\b")
PUSH_RE = re.compile(r"^push\s+(.+?)$")
IMMEDIATE_HEX_RE = re.compile(r"^(?:0([0-9A-F]+)|([0-9][0-9A-F]*))h$")
IMMEDIATE_DEC_RE = re.compile(r"^[0-9]+$")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_immediate(operand: str) -> int | None:
    clean = operand.split(";", 1)[0].strip()
    match = IMMEDIATE_HEX_RE.fullmatch(clean)
    if match:
        return int(match.group(1) or match.group(2), 16)
    if IMMEDIATE_DEC_RE.fullmatch(clean):
        return int(clean, 10)
    return None


def load_lines() -> list[tuple[int, str]]:
    rows: list[tuple[int, str]] = []
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASM_LINE_RE.match(raw)
        if match:
            rows.append((int(match.group(1), 16), match.group(2).rstrip()))
    return rows


def main() -> None:
    actual_hash = sha256(ASM_PATH)
    if actual_hash != EXPECTED_ASM_SHA256:
        raise SystemExit(
            f"locked assembly changed: expected {EXPECTED_ASM_SHA256}, got {actual_hash}"
        )

    current_proc = ""
    instruction_history: list[tuple[int, str]] = []
    records: list[dict[str, str | int]] = []

    for address, raw_instruction in load_lines():
        instruction = raw_instruction.split(";", 1)[0].rstrip()
        proc_match = PROC_RE.match(instruction)
        if proc_match:
            current_proc = proc_match.group(1)
            instruction_history = []
            continue
        if ENDP_RE.match(instruction):
            current_proc = ""
            instruction_history = []
            continue

        if instruction == "call    sub_4170E0":
            pushes: list[tuple[int, str]] = []
            for previous_address, previous_instruction in reversed(instruction_history):
                push_match = PUSH_RE.match(previous_instruction)
                if push_match:
                    pushes.append((previous_address, push_match.group(1).strip()))
                    if len(pushes) == 6:
                        break
            if len(pushes) != 6:
                raise SystemExit(
                    f"call 0x{address:08X} in {current_proc} has only {len(pushes)} prior pushes"
                )

            # Backward order is arg_0, arg_4, arg_8, arg_C, arg_10, arg_14.
            operands = [operand for _, operand in pushes]
            flags_operand = operands[4]
            flags_value = parse_immediate(flags_operand)
            if flags_value is None:
                flags_hex = ""
                low_flags = ""
                mode = ""
                flip_bits = ""
                forced_family = "runtime/unresolved"
            else:
                flags_value &= 0xFFFFFFFF
                flags_hex = f"0x{flags_value:08X}"
                low = flags_value & 0xFFFF
                low_flags = f"0x{low:04X}"
                mode = f"0x{low & 0x7C:02X}"
                flip_bits = low & 3
                forced_family = (
                    "RLE/span forced by bit31"
                    if flags_value & 0x80000000
                    else "source header selects RLE/span or raw/+0x80"
                )

            records.append({
                "caller": current_proc,
                "call_address": f"0x{address:08X}",
                "first_argument_push": f"0x{pushes[0][0]:08X}",
                "push_span_bytes": address - pushes[-1][0],
                "arg_0_x": operands[0],
                "arg_4_y": operands[1],
                "arg_8_width": operands[2],
                "arg_C_height": operands[3],
                "arg_10_flags": flags_operand,
                "arg_14_auxiliary": operands[5],
                "flags_immediate": flags_hex,
                "low_flags": low_flags,
                "mode": mode,
                "flip_bits": flip_bits,
                "family_selection": forced_family,
            })

        instruction_history.append((address, instruction))

    if len(records) != EXPECTED_CALLSITES:
        raise SystemExit(f"expected {EXPECTED_CALLSITES} callsites, recovered {len(records)}")
    callers = {str(record["caller"]) for record in records}
    if len(callers) != EXPECTED_CALLERS:
        raise SystemExit(f"expected {EXPECTED_CALLERS} callers, recovered {len(callers)}")

    fieldnames = [
        "caller", "call_address", "first_argument_push", "push_span_bytes",
        "arg_0_x", "arg_4_y", "arg_8_width", "arg_C_height", "arg_10_flags",
        "arg_14_auxiliary", "flags_immediate", "low_flags", "mode", "flip_bits",
        "family_selection",
    ]
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, delimiter="\t", fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(records)

    immediate_counts = Counter(
        str(record["flags_immediate"])
        for record in records
        if record["flags_immediate"]
    )
    print(
        f"wrote {len(records)} calls in {len(callers)} callers; "
        f"{sum(immediate_counts.values())} have immediate flags across "
        f"{len(immediate_counts)} values"
    )


if __name__ == "__main__":
    main()
