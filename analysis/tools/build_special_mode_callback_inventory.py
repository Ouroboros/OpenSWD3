#!/usr/bin/env python3
"""Build the assembly-first special-mode callback-slot inventories.

The callback assignments are extracted only from sub_43B480 in the complete
IDA assembly export.  Group predicates and indirect-call guards are manually
reviewed assembly contracts; the existing global ABI inventory is used only to
cross-check target stack hints and RETN cleanup.
"""

from __future__ import annotations

import csv
import hashlib
import re
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
ABI_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "function-abi-candidates.tsv"
)
TARGET_OUTPUT_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "special-mode-callback-targets.tsv"
)
SLOT_OUTPUT_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "special-mode-callback-slots.tsv"
)
SECONDARY_OUTPUT_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "special-mode-secondary-dispatch.tsv"
)

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    ABI_PATH: "cd85c44a4b03f395199d7de82f1e520263fe3aec0d9a8528657624484b681bda",
}

ASSIGNMENT_RE = re.compile(
    r"^([0-9A-F]{8})\s+mov\s+"
    r"(dword_4F(?:B808|C64C|C828|C4E8|BAC0|C788|BF78|BA10|C83C|BF7C|C3C0|C838|BEC8)),"
    r"\s+(.+?)\s*$"
)
TARGET_SYMBOL_RE = re.compile(r"^(?:offset\s+)?(sub_|loc_)([0-9A-F]{6})$")
SECONDARY_ASSIGNMENT_RE = re.compile(
    r"^([0-9A-F]{8})\s+mov\s+(dword_4FC8(?:DC|E0|E4|E8|EC|F0|F4)),\s+(offset\s+\S+)\s*$"
)


@dataclass(frozen=True)
class Group:
    group_id: str
    first_assignment: int
    last_assignment: int
    selector_contract: str


@dataclass(frozen=True)
class Slot:
    address: int
    name: str
    callsites: str
    guard_contract: str
    call_phase: str


GROUPS = (
    Group("G01", 0x0043B49F, 0x0043B50D, "arg0 == 2 and word_4FB8A8 in [0x001E,0x0020]"),
    Group("G02", 0x0043B524, 0x0043B592, "arg0 == 2 and word_4FB8A8 in [0x0024,0x0029]"),
    Group("G03", 0x0043B5A9, 0x0043B617, "arg0 == 2 and word_4FB8A8 in [0x002A,0x002E]"),
    Group("G04", 0x0043B62E, 0x0043B692, "arg0 == 2 and word_4FB8A8 in [0x0030,0x0034]"),
    Group(
        "G05",
        0x0043B6C3,
        0x0043B727,
        "arg0 == 2 and ((word_4FB8A8 in [0x0036,0x003A] and sub_40DC50(0x49) == 0) or "
        "(word_4FB8A8 in [0x003C,0x003E] and sub_40DC50(0x49) != 0))",
    ),
    Group(
        "G06",
        0x0043B758,
        0x0043B7C6,
        "arg0 == 2 and ((word_4FB8A8 in [0x0036,0x003A] and sub_40DC50(0x49) != 0) or "
        "(word_4FB8A8 in [0x003C,0x003E] and sub_40DC50(0x49) == 0))",
    ),
    Group("G07", 0x0043B7E5, 0x0043B849, "arg0 == 2 and word_4FB8A8 in [0x0042,0x0047]"),
    Group("G08", 0x0043B862, 0x0043B8DA, "arg0 == 1"),
    Group("G09", 0x0043B8F5, 0x0043B96D, "arg0 == 0x0000EA60"),
)

SLOTS = (
    Slot(0x004FB808, "dword_4FB808", "0x0043A479", "slot != 0", "optional pre-update callback"),
    Slot(0x004FC64C, "dword_4FC64C", "0x0043A47C", "unconditional", "base update callback"),
    Slot(
        0x004FC838,
        "dword_4FC838",
        "0x0043A498;0x0043A4C2",
        "(dword_4B7CB0 != 0 and dword_4B7CBC == 1);"
        "(dword_4FC908 == 0 and dword_4B7D40 != 0 and dword_4B7D4C == 1)",
        "two overlay callbacks",
    ),
    Slot(0x004FC83C, "dword_4FC83C", "0x0043A4F0", "dword_4B7CD0 != 0 and dword_4B7CDC == 1", "conditional layer"),
    Slot(0x004FBF7C, "dword_4FBF7C", "0x0043A507", "dword_4B7D50 != 0 and dword_4B7D5C == 1", "conditional layer"),
    Slot(
        0x004FC828,
        "dword_4FC828",
        "0x0043A528",
        "dword_4B7D10 != 0 and (dword_4B7D1C == 1 or ((dword_4B7D1C & 1) == 0 and dword_4B7D1C > 7))",
        "conditional layer",
    ),
    Slot(
        0x004FC4E8,
        "dword_4FC4E8",
        "0x0043A549",
        "dword_4B7CF0 != 0 and (dword_4B7CFC == 1 or ((dword_4B7CFC & 1) == 0 and dword_4B7CFC > 7))",
        "conditional layer",
    ),
    Slot(
        0x004FBF78,
        "dword_4FBF78",
        "0x0043A56A",
        "dword_4B7D30 != 0 and (dword_4B7D3C == 1 or ((dword_4B7D3C & 1) == 0 and dword_4B7D3C > 7))",
        "conditional layer",
    ),
    Slot(
        0x004FBA10,
        "dword_4FBA10",
        "0x0043A58B",
        "dword_4B7D20 != 0 and (dword_4B7D2C == 1 or ((dword_4B7D2C & 1) == 0 and dword_4B7D2C > 7))",
        "conditional layer",
    ),
    Slot(
        0x004FBAC0,
        "dword_4FBAC0",
        "0x0043A5B3",
        "dword_4B7CE0 != 0 and (dword_4B7CEC == 1 or (stale AL bit0 == 0 and dword_4B7CEC > 7))",
        "conditional layer; bit test uses AL, not ECX",
    ),
    Slot(
        0x004FC788,
        "dword_4FC788",
        "0x0043A5DB",
        "dword_4B7D00 != 0 and (dword_4B7D0C == 1 or (stale AL bit0 == 0 and dword_4B7D0C > 7))",
        "conditional layer; bit test uses AL, not ECX",
    ),
    Slot(
        0x004FC3C0,
        "dword_4FC3C0",
        "0x0043A603",
        "(dword_4B7CC0 != 0 and dword_4B7CCC == 1) or (dword_4B7D70 != 0 and dword_4B7D7C == 1)",
        "combined conditional layer",
    ),
    Slot(0x004FBEC8, "dword_4FBEC8", "0x0043A6DB", "word_4FC900 != 1", "post-update callback in sub_43A610"),
)

EXPECTED_GROUP_COUNTS = {
    "G01": 12,
    "G02": 12,
    "G03": 12,
    "G04": 11,
    "G05": 11,
    "G06": 12,
    "G07": 11,
    "G08": 13,
    "G09": 13,
}

ASSEMBLY_PROC_ONLY_TARGETS = {
    "sub_43DA30",
    "sub_4439A0",
    "sub_4452B0",
    "sub_445430",
    "sub_4466A0",
    "sub_448C40",
    "sub_448DA0",
    "sub_44BBD0",
}

PRIMARY_ASSEMBLY_PROC_ONLY_TARGETS = ASSEMBLY_PROC_ONLY_TARGETS - {"sub_445430"}
TAIL_DISPATCH_TARGETS = {"sub_445420", "sub_449FF0"}


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
    with EXE_PATH.open("rb") as source:
        source.seek(0x0004A240)
        if source.read(1) != b"\xC3":
            raise SystemExit("locked nullsub_1 byte at file offset 0x4A240 is no longer RETN (C3)")


def load_abi() -> tuple[dict[int, dict[str, str]], dict[str, dict[str, str]]]:
    with ABI_PATH.open("r", encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    by_address = {int(row["address"], 16): row for row in rows}
    by_name: dict[str, dict[str, str]] = {}
    for row in rows:
        for field in ("ida_name", "proc_name"):
            if row[field]:
                by_name[row[field]] = row
    if len(rows) != 1510 or len(by_address) != 1510:
        raise SystemExit("ABI inventory is not the locked 1510-address set")
    return by_address, by_name


def group_for(address: int) -> Group:
    matches = [group for group in GROUPS if group.first_assignment <= address <= group.last_assignment]
    if len(matches) != 1:
        raise SystemExit(f"assignment 0x{address:08X} is not in exactly one reviewed group")
    return matches[0]


def target_name_and_address(target_expression: str, by_name: dict[str, dict[str, str]]) -> tuple[str, int | None]:
    if target_expression == "0":
        return "0", None
    target_name = target_expression.removeprefix("offset ")
    match = TARGET_SYMBOL_RE.match(target_expression)
    if match:
        return target_name, int(match.group(2), 16)
    row = by_name.get(target_name)
    if row:
        return target_name, int(row["address"], 16)
    raise SystemExit(f"cannot resolve target: {target_expression}")


def target_class(target_name: str) -> str:
    if target_name == "0":
        return "null_pointer"
    if target_name == "nullsub_1":
        return "one_byte_null_stub"
    if target_name.startswith("loc_"):
        return "callable_function_chunk_entry"
    if target_name in TAIL_DISPATCH_TARGETS:
        return "tail_dispatch_callback"
    if target_name in ASSEMBLY_PROC_ONLY_TARGETS:
        return "assembly_proc_only_callback"
    return "named_proc_callback"


def abi_fields(target_name: str, target_address: int | None, by_address: dict[int, dict[str, str]]) -> tuple[str, str, str, str]:
    if target_name == "0":
        return "not_callable", "", "", "explicit null slot value"
    if target_name == "nullsub_1":
        return "collapsed_one_byte_stub", "0", "0x0", "one-byte null stub"
    if target_name.startswith("loc_"):
        return "manual_chunk_path_review", "0", "0x0", "zero-stack entry; path returns with plain retn"
    if target_address is None or target_address not in by_address:
        raise SystemExit(f"missing ABI row for {target_name}")
    row = by_address[target_address]
    return (
        "function_abi_candidates.tsv",
        row["positive_stack_argument_hint_count"],
        row["retn_pop_values_hex"],
        row["assembly_cleanup_class"],
    )


def extract_assignments(lines: list[str], by_address: dict[int, dict[str, str]], by_name: dict[str, dict[str, str]]) -> list[dict[str, str]]:
    slot_by_name = {slot.name: slot for slot in SLOTS}
    rows: list[dict[str, str]] = []
    order_by_group: Counter[str] = Counter()
    for line in lines:
        match = ASSIGNMENT_RE.match(line)
        if not match:
            continue
        address = int(match.group(1), 16)
        if not 0x0043B480 <= address <= 0x0043B977:
            continue
        slot_name = match.group(2)
        target_expression = match.group(3)
        group = group_for(address)
        slot = slot_by_name[slot_name]
        order_by_group[group.group_id] += 1
        name, target_address = target_name_and_address(target_expression, by_name)
        abi_source, stack_hints, retn_values, cleanup = abi_fields(name, target_address, by_address)
        rows.append(
            {
                "assignment_address": f"0x{address:08X}",
                "group_id": group.group_id,
                "group_assignment_order": str(order_by_group[group.group_id]),
                "selector_contract": group.selector_contract,
                "slot_address": f"0x{slot.address:08X}",
                "slot_name": slot.name,
                "target_expression": target_expression,
                "target_address": "" if target_address is None else f"0x{target_address:08X}",
                "target_name": name,
                "target_class": target_class(name),
                "abi_evidence_source": abi_source,
                "positive_stack_argument_hint_count": stack_hints,
                "retn_pop_values_hex": retn_values,
                "cleanup_contract": cleanup,
                "indirect_call_stack_argument_bytes": "0",
                "indirect_return_use": "ignored or overwritten before observation",
                "review_status": "assembly_reviewed_zero_stack_callback_contract",
            }
        )

    if len(rows) != 107:
        raise SystemExit(f"expected 107 callback-slot assignments, found {len(rows)}")
    if len({row["slot_name"] for row in rows}) != 13:
        raise SystemExit("expected exactly 13 callback slots")
    if len({row["target_expression"] for row in rows}) != 92:
        raise SystemExit("expected exactly 92 unique target expressions including zero")
    group_counts = Counter(row["group_id"] for row in rows)
    if dict(group_counts) != EXPECTED_GROUP_COUNTS:
        raise SystemExit(f"unexpected group counts: {dict(group_counts)}")
    if {row["target_name"] for row in rows if row["target_class"] == "assembly_proc_only_callback"} != PRIMARY_ASSEMBLY_PROC_ONLY_TARGETS:
        raise SystemExit("assembly-only callback target set changed")
    for row in rows:
        if row["target_name"] == "0":
            continue
        if row["positive_stack_argument_hint_count"] != "0":
            raise SystemExit(f"callback gained a positive stack argument hint: {row}")
        if row["retn_pop_values_hex"] not in {"0x0", ""}:
            raise SystemExit(f"callback gained nonzero RETN cleanup: {row}")
        if row["retn_pop_values_hex"] == "" and row["target_name"] != "sub_445420":
            raise SystemExit(f"unexpected primary callback without a primary-body RETN: {row}")
    return rows


def build_slot_rows(assignments: list[dict[str, str]]) -> list[dict[str, str]]:
    by_slot: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in assignments:
        by_slot[row["slot_name"]].append(row)

    all_group_ids = [group.group_id for group in GROUPS]
    slot_rows: list[dict[str, str]] = []
    for slot in SLOTS:
        rows = by_slot[slot.name]
        written_groups = [group_id for group_id in all_group_ids if any(row["group_id"] == group_id for row in rows)]
        missing_groups = [group_id for group_id in all_group_ids if group_id not in written_groups]
        preservation = (
            "not written by listed groups; previous or dynamically installed value is retained"
            if missing_groups
            else "written by every configuration group"
        )
        slot_rows.append(
            {
                "slot_address": f"0x{slot.address:08X}",
                "slot_name": slot.name,
                "indirect_callsites": slot.callsites,
                "callsite_count": str(len(slot.callsites.split(";"))),
                "guard_contract": slot.guard_contract,
                "call_phase": slot.call_phase,
                "stack_argument_bytes_at_indirect_calls": "0",
                "return_use": "ignored or overwritten before observation",
                "assignment_count_in_sub_43B480": str(len(rows)),
                "written_groups": ";".join(written_groups),
                "missing_groups": ";".join(missing_groups),
                "missing_group_behavior": preservation,
                "review_status": "assembly_reviewed",
            }
        )

    expected_missing = {
        "dword_4FB808": "G01;G02;G03;G04;G05;G06;G07",
        "dword_4FBF7C": "G04;G05;G07",
    }
    actual_missing = {row["slot_name"]: row["missing_groups"] for row in slot_rows if row["missing_groups"]}
    if actual_missing != expected_missing:
        raise SystemExit(f"callback-slot retention set changed: {actual_missing}")
    return slot_rows


def extract_secondary_dispatch(
    lines: list[str],
    by_address: dict[int, dict[str, str]],
    by_name: dict[str, dict[str, str]],
) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for line in lines:
        match = SECONDARY_ASSIGNMENT_RE.match(line)
        if not match:
            continue
        assignment_address = int(match.group(1), 16)
        if not 0x00444FC0 <= assignment_address <= 0x004450DD:
            continue
        slot_name = match.group(2)
        slot_address = int(slot_name.removeprefix("dword_"), 16)
        target_expression = match.group(3)
        name, target_address = target_name_and_address(target_expression, by_name)
        abi_source, stack_hints, retn_values, cleanup = abi_fields(name, target_address, by_address)
        condition = (
            "unconditional base-table installation in sub_444FC0"
            if assignment_address < 0x004450A1
            else "override when sub_40DC50(0x49) == 1; swaps dispatch indices 15 and 16"
        )
        rows.append(
            {
                "assignment_address": f"0x{assignment_address:08X}",
                "assignment_condition": condition,
                "slot_address": f"0x{slot_address:08X}",
                "dispatch_index_decimal": str((slot_address - 0x004FC8B0) // 4),
                "target_address": "" if target_address is None else f"0x{target_address:08X}",
                "target_name": name,
                "target_class": target_class(name),
                "indexed_callsites": "0x0043A0CE;0x004453CC",
                "index_expression": "low 16 bits of dword_4FC3C4",
                "indirect_call_stack_argument_bytes": "0",
                "indirect_return_use": "ignored or overwritten before observation",
                "abi_evidence_source": abi_source,
                "positive_stack_argument_hint_count": stack_hints,
                "retn_pop_values_hex": retn_values,
                "cleanup_contract": cleanup,
                "review_status": "assembly_reviewed_zero_stack_indexed_callback_contract",
            }
        )

    if len(rows) != 9:
        raise SystemExit(f"expected 9 secondary dispatch assignments, found {len(rows)}")
    if len({row["slot_address"] for row in rows}) != 7:
        raise SystemExit("expected seven secondary dispatch slots")
    if {row["dispatch_index_decimal"] for row in rows} != {str(index) for index in range(11, 18)}:
        raise SystemExit("secondary dispatch index set changed")
    if len({row["target_name"] for row in rows}) != 7:
        raise SystemExit("expected seven unique secondary dispatch targets")
    if {row["target_name"] for row in rows if row["target_class"] == "assembly_proc_only_callback"} != {"sub_445430"}:
        raise SystemExit("secondary assembly-only callback target changed")
    for row in rows:
        if row["positive_stack_argument_hint_count"] != "0":
            raise SystemExit(f"secondary callback gained a positive stack argument hint: {row}")
        if row["retn_pop_values_hex"] not in {"0x0", ""}:
            raise SystemExit(f"secondary callback gained nonzero RETN cleanup: {row}")
        if row["retn_pop_values_hex"] == "" and row["target_name"] != "sub_449FF0":
            raise SystemExit(f"unexpected secondary callback without a primary-body RETN: {row}")
    return rows


def write_tsv(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as target:
        writer = csv.DictWriter(target, fieldnames=list(rows[0]), delimiter="\t", lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    verify_inputs()
    lines = ASM_PATH.read_text(encoding="utf-8").splitlines()
    by_address, by_name = load_abi()
    assignments = extract_assignments(lines, by_address, by_name)
    slots = build_slot_rows(assignments)
    secondary = extract_secondary_dispatch(lines, by_address, by_name)
    write_tsv(TARGET_OUTPUT_PATH, assignments)
    write_tsv(SLOT_OUTPUT_PATH, slots)
    write_tsv(SECONDARY_OUTPUT_PATH, secondary)
    print(f"wrote {len(assignments)} assignments to {TARGET_OUTPUT_PATH}")
    print(f"wrote {len(slots)} slot contracts to {SLOT_OUTPUT_PATH}")
    print(f"wrote {len(secondary)} secondary dispatch assignments to {SECONDARY_OUTPUT_PATH}")


if __name__ == "__main__":
    main()
