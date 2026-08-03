#!/usr/bin/env python3
"""Build a mechanical assembly-first ABI triage inventory.

This deliberately does not turn IDA argument declarations into final C/C++
signatures.  The immediate value of each RETN is an assembly fact; argument
labels and the short EAX-use scan are navigation candidates for manual P2.5
review.
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
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
CATALOG_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "function-catalog.tsv"
)
OUTPUT_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "function-abi-candidates.tsv"
)
SUMMARY_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "function-abi-summary.tsv"
)

EXPECTED_SHA256 = {
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    CATALOG_PATH: "d8b707b5550c64dee539f25c7032fc700f48ae70fa0661e50faabf7a45df5c73",
}

PROC_RE = re.compile(r"^([0-9A-F]{8})\s+(\S+)\s+proc\s+(?:near|far)\b")
ENDP_TEMPLATE = r"^[0-9A-F]{{8}}\s+{}\s+endp\b"
ARG_RE = re.compile(
    r"^[0-9A-F]{8}\s+(\S+)\s+=\s+(.+?\s+ptr)\s+([0-9A-F]+h|\d+)\s*$"
)
RETN_RE = re.compile(r"^([0-9A-F]{8}) {17}retn(?:\s+([0-9A-F]+h|\d+))?(?:\s*;.*)?$")
CALL_RE = re.compile(r"^([0-9A-F]{8}) {17}call\s+(.+?)(?:\s+;.*)?$")
INSTRUCTION_RE = re.compile(r"^([0-9A-F]{8}) {17}([A-Za-z][A-Za-z0-9]*)\s*(.*?)\s*$")
RETURN_REGISTER_RE = re.compile(r"(?<![A-Za-z0-9_])(eax|ax|al|ah)(?![A-Za-z0-9_])", re.I)


@dataclass(frozen=True)
class CatalogRow:
    address: int
    ida_name: str
    export_status: str


@dataclass
class ProcBody:
    address: int
    proc_name: str
    ida_prototype_hint: str
    argument_hints: list[tuple[str, str, int, int]]
    ret_pop_values: list[int]
    function_chunk_count: int


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_number(text: str) -> int:
    return int(text[:-1], 16) if text.endswith("h") else int(text)


def declared_width(type_hint: str) -> int:
    first = type_hint.split()[0].lower()
    return {
        "byte": 1,
        "word": 2,
        "dword": 4,
        "qword": 8,
        "tbyte": 10,
        "xmmword": 16,
        "oword": 16,
    }.get(first, 0)


def load_catalog() -> list[CatalogRow]:
    with CATALOG_PATH.open("r", encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    result = [
        CatalogRow(
            address=int(row["address"], 16),
            ida_name=row["ida_name"],
            export_status=row["export_status"],
        )
        for row in rows
    ]
    if len(result) != 1486 or len({row.address for row in result}) != 1486:
        raise SystemExit("function catalog is not the locked 1486-address set")
    return result


def prototype_hint(lines: list[str], proc_line_index: int) -> str:
    if proc_line_index == 0:
        return ""
    match = re.match(r"^[0-9A-F]{8}\s+;\s+(.+?)\s*$", lines[proc_line_index - 1])
    if not match:
        return ""
    hint = match.group(1)
    if hint.startswith(("===", "---", "CODE XREF", "DATA XREF")):
        return ""
    return hint


def parse_proc_bodies(lines: list[str]) -> dict[int, ProcBody]:
    starts: list[tuple[int, int, str]] = []
    for index, line in enumerate(lines):
        match = PROC_RE.match(line)
        if match:
            starts.append((index, int(match.group(1), 16), match.group(2)))

    bodies: dict[int, ProcBody] = {}
    for start_index, address, proc_name in starts:
        end_pattern = re.compile(ENDP_TEMPLATE.format(re.escape(proc_name)))
        end_index = start_index + 1
        while end_index < len(lines) and not end_pattern.match(lines[end_index]):
            end_index += 1
        if end_index == len(lines):
            raise SystemExit(f"missing ENDP for {proc_name} at 0x{address:08X}")

        block = lines[start_index : end_index + 1]
        argument_hints: list[tuple[str, str, int, int]] = []
        for line in block:
            match = ARG_RE.match(line)
            if not match:
                continue
            offset = parse_number(match.group(3))
            if offset <= 0:
                continue
            symbol = match.group(1)
            type_hint = match.group(2)
            reference_count = sum(
                bool(re.search(rf"(?<![A-Za-z0-9_]){re.escape(symbol)}(?![A-Za-z0-9_])", candidate))
                for candidate in block
            ) - 1
            argument_hints.append((symbol, type_hint, offset, max(reference_count, 0)))

        ret_pop_values: list[int] = []
        for line in block:
            match = RETN_RE.match(line)
            if match:
                ret_pop_values.append(parse_number(match.group(2)) if match.group(2) else 0)

        if address in bodies:
            raise SystemExit(f"duplicate PROC address 0x{address:08X}")
        bodies[address] = ProcBody(
            address=address,
            proc_name=proc_name,
            ida_prototype_hint=prototype_hint(lines, start_index),
            argument_hints=argument_hints,
            ret_pop_values=ret_pop_values,
            function_chunk_count=sum("FUNCTION CHUNK AT" in line for line in block),
        )

    if len(bodies) != 1215:
        raise SystemExit(f"unexpected expanded PROC count: {len(bodies)}")
    return bodies


def direct_target_name(operand: str) -> str:
    operand = operand.strip()
    for prefix in ("near ptr ", "far ptr ", "short "):
        if operand.startswith(prefix):
            operand = operand[len(prefix) :]
    return operand.split()[0]


def eax_reference_class(mnemonic: str, operands: str) -> str | None:
    if mnemonic.lower() == "cdq":
        return "consumed_before_clobber"
    if not RETURN_REGISTER_RE.search(operands):
        return None

    parts = [part.strip() for part in operands.split(",", 1)]
    destination = parts[0] if parts else ""
    source = parts[1] if len(parts) > 1 else ""
    destination_is_return_register = bool(RETURN_REGISTER_RE.fullmatch(destination))
    source_uses_return_register = bool(RETURN_REGISTER_RE.search(source))
    lower_mnemonic = mnemonic.lower()

    if lower_mnemonic in {"mov", "lea", "pop", "seta", "setae", "setb", "setbe", "sete", "setne", "setz", "setnz"}:
        if destination_is_return_register and not source_uses_return_register:
            return "overwritten_before_use"
    if lower_mnemonic in {"xor", "sub"} and len(parts) == 2 and parts[0].lower() == parts[1].lower() and destination_is_return_register:
        return "overwritten_before_use"
    return "consumed_before_clobber"


def classify_return_use(
    instruction_rows: list[tuple[int, str, str]],
    call_index: int,
    window: int = 12,
) -> str:
    for _, mnemonic, operands in instruction_rows[call_index + 1 : call_index + 1 + window]:
        lower_mnemonic = mnemonic.lower()
        register_class = eax_reference_class(lower_mnemonic, operands)
        if register_class:
            return register_class
        if lower_mnemonic == "call":
            return "clobbered_by_later_call"
        if lower_mnemonic in {"retn", "retf", "iret", "jmp"}:
            return "not_observed_before_control_transfer"
        if lower_mnemonic.startswith("j") or lower_mnemonic.startswith("loop"):
            return "branch_before_observation"
    return "not_observed_in_12_instruction_window"


def callsite_stats(
    lines: list[str], bodies: dict[int, ProcBody]
) -> tuple[dict[int, Counter[str]], int]:
    name_to_address = {body.proc_name: address for address, body in bodies.items()}
    instructions: list[tuple[int, str, str]] = []
    for line in lines:
        match = INSTRUCTION_RE.match(line)
        if match:
            instructions.append((int(match.group(1), 16), match.group(2), match.group(3)))

    stats: dict[int, Counter[str]] = defaultdict(Counter)
    direct_internal_calls = 0
    for index, (_, mnemonic, operands) in enumerate(instructions):
        if mnemonic.lower() != "call":
            continue
        target_name = direct_target_name(operands)
        target_address = name_to_address.get(target_name)
        if target_address is None:
            continue
        direct_internal_calls += 1
        stats[target_address][classify_return_use(instructions, index)] += 1
    return stats, direct_internal_calls


def ret_cleanup_class(values: list[int], chunk_count: int) -> str:
    unique = sorted(set(values))
    if not unique:
        return "no_retn_in_primary_body" if not chunk_count else "no_retn_in_primary_body_chunks_present"
    if len(unique) != 1:
        return "mixed_retn_cleanup_manual_review"
    if unique[0] == 0:
        return "plain_retn_caller_or_register_abi"
    return f"callee_pops_0x{unique[0]:X}_bytes"


def write_inventory(
    catalog: list[CatalogRow],
    bodies: dict[int, ProcBody],
    call_stats: dict[int, Counter[str]],
) -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "address",
                "ida_name",
                "export_status",
                "asm_body_status",
                "proc_name",
                "ida_prototype_hint_navigation_only",
                "positive_stack_argument_hint_count",
                "maximum_positive_stack_offset_hex",
                "argument_hints_navigation_only",
                "retn_exit_count_primary_body",
                "retn_pop_values_hex",
                "assembly_cleanup_class",
                "function_chunk_count",
                "direct_internal_callsite_count",
                "eax_consumed_before_clobber_heuristic",
                "eax_overwritten_or_clobbered_heuristic",
                "eax_unresolved_heuristic",
                "review_status",
            )
        )

        for row in catalog:
            body = bodies.get(row.address)
            stats = call_stats.get(row.address, Counter())
            consumed = stats["consumed_before_clobber"]
            clobbered = stats["overwritten_before_use"] + stats["clobbered_by_later_call"]
            unresolved = sum(stats.values()) - consumed - clobbered
            if body is None:
                writer.writerow(
                    (
                        f"0x{row.address:08X}",
                        row.ida_name,
                        row.export_status,
                        "collapsed_or_no_proc_body",
                        "",
                        "",
                        0,
                        "",
                        "",
                        0,
                        "",
                        "body_unavailable",
                        0,
                        0,
                        0,
                        0,
                        0,
                        "manual_or_library_boundary_review_required",
                    )
                )
                continue

            max_offset = max((argument[2] for argument in body.argument_hints), default=0)
            argument_text = ";".join(
                f"{name}@+0x{offset:X}:{type_hint}:refs={references}"
                for name, type_hint, offset, references in body.argument_hints
            )
            ret_values = ",".join(f"0x{value:X}" for value in sorted(set(body.ret_pop_values)))
            writer.writerow(
                (
                    f"0x{row.address:08X}",
                    row.ida_name,
                    row.export_status,
                    "expanded_proc_body",
                    body.proc_name,
                    body.ida_prototype_hint,
                    len(body.argument_hints),
                    f"0x{max_offset:X}" if max_offset else "",
                    argument_text,
                    len(body.ret_pop_values),
                    ret_values,
                    ret_cleanup_class(body.ret_pop_values, body.function_chunk_count),
                    body.function_chunk_count,
                    sum(stats.values()),
                    consumed,
                    clobbered,
                    unresolved,
                    "assembly_cleanup_extracted_arguments_and_eax_use_need_manual_review",
                )
            )


def write_summary(
    manifest_catalog: list[CatalogRow],
    combined_catalog: list[CatalogRow],
    bodies: dict[int, ProcBody],
    call_stats: dict[int, Counter[str]],
    direct_internal_calls: int,
) -> None:
    ret_pattern_counts = Counter(
        tuple(sorted(set(body.ret_pop_values))) for body in bodies.values()
    )
    all_call_classes: Counter[str] = Counter()
    for stats in call_stats.values():
        all_call_classes.update(stats)

    manifest_addresses = {row.address for row in manifest_catalog}
    proc_addresses = set(bodies)
    manifest_only = manifest_addresses - proc_addresses
    manifest_by_address = {row.address: row for row in manifest_catalog}
    summary_rows: list[tuple[str, str]] = [
        ("export_manifest_unique_addresses", str(len(manifest_catalog))),
        ("assembly_proc_markers", str(len(bodies))),
        ("manifest_and_proc_intersection", str(len(manifest_addresses & proc_addresses))),
        ("assembly_proc_only_addresses", str(len(proc_addresses - manifest_addresses))),
        ("manifest_only_addresses", str(len(manifest_only))),
        (
            "manifest_only_skipped_library_addresses",
            str(sum(manifest_by_address[address].export_status == "skipped" for address in manifest_only)),
        ),
        (
            "manifest_only_exported_alias_thin_or_null_addresses",
            str(sum(manifest_by_address[address].export_status != "skipped" for address in manifest_only)),
        ),
        ("union_abi_candidate_addresses", str(len(combined_catalog))),
        ("expanded_proc_bodies", str(len(bodies))),
        ("union_without_proc_body", str(len(combined_catalog) - len(bodies))),
        ("expanded_with_function_chunks", str(sum(body.function_chunk_count > 0 for body in bodies.values()))),
        ("expanded_without_retn_in_primary_body", str(sum(not body.ret_pop_values for body in bodies.values()))),
        ("expanded_plain_retn_only", str(ret_pattern_counts[(0,)])),
        (
            "expanded_callee_pop_single_value",
            str(sum(count for pattern, count in ret_pattern_counts.items() if len(pattern) == 1 and pattern[0] > 0)),
        ),
        ("expanded_mixed_retn_pop_values", str(sum(count for pattern, count in ret_pattern_counts.items() if len(pattern) > 1))),
        ("direct_internal_calls_to_expanded_bodies", str(direct_internal_calls)),
    ]
    for pattern, count in sorted(ret_pattern_counts.items(), key=lambda item: (item[0], item[1])):
        label = "none" if not pattern else ",".join(f"0x{value:X}" for value in pattern)
        summary_rows.append((f"retn_pop_pattern_{label}", str(count)))
    for classification, count in sorted(all_call_classes.items()):
        summary_rows.append((f"eax_use_heuristic_{classification}", str(count)))

    with SUMMARY_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("metric", "value"))
        writer.writerows(summary_rows)


def main() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"input hash mismatch for {path.relative_to(WORKSPACE_ROOT)}: {actual} != {expected}"
            )

    manifest_catalog = load_catalog()
    lines = ASM_PATH.read_text(encoding="utf-8", errors="replace").replace("\r", "").splitlines()
    bodies = parse_proc_bodies(lines)
    manifest_addresses = {row.address for row in manifest_catalog}
    proc_only_rows = [
        CatalogRow(
            address=address,
            ida_name=bodies[address].proc_name,
            export_status="assembly-proc-only-not-in-export-manifests",
        )
        for address in sorted(set(bodies) - manifest_addresses)
    ]
    if len(proc_only_rows) != 24:
        raise SystemExit(f"unexpected assembly-PROC-only count: {len(proc_only_rows)}")
    combined_catalog = sorted(manifest_catalog + proc_only_rows, key=lambda row: row.address)
    if len(combined_catalog) != 1510:
        raise SystemExit(f"unexpected ABI candidate union size: {len(combined_catalog)}")

    call_stats, direct_internal_calls = callsite_stats(lines, bodies)
    if direct_internal_calls != 7159:
        raise SystemExit(f"unexpected direct internal call-site count: {direct_internal_calls}")
    expected_ret_patterns = {
        (): 44,
        (0,): 893,
        (4,): 139,
        (8,): 80,
        (12,): 15,
        (16,): 20,
        (20,): 6,
        (24,): 15,
        (28,): 1,
        (32,): 1,
        (36,): 1,
    }
    actual_ret_patterns = Counter(
        tuple(sorted(set(body.ret_pop_values))) for body in bodies.values()
    )
    if actual_ret_patterns != expected_ret_patterns:
        raise SystemExit(f"unexpected RETN pattern distribution: {actual_ret_patterns}")
    write_inventory(combined_catalog, bodies, call_stats)
    write_summary(
        manifest_catalog,
        combined_catalog,
        bodies,
        call_stats,
        direct_internal_calls,
    )
    print(
        f"wrote {len(combined_catalog)} ABI candidates, {len(bodies)} expanded bodies and "
        f"{direct_internal_calls} direct internal call sites"
    )


if __name__ == "__main__":
    main()
