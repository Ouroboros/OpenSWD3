#!/usr/bin/env python3
"""Build the mechanical P2 function catalog from the IDA export manifests."""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXPORT_ROOT = WORKSPACE_ROOT / "swd3.exe_export_for_ai"
OUTPUT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "function-catalog.tsv"


@dataclass(frozen=True)
class FunctionRecord:
    address: int
    ida_name: str
    export_status: str
    exported_file: str
    reason: str
    ida_called_by_count: int
    ida_call_count: int


def field(block: str, label: str) -> str:
    match = re.search(rf"^{re.escape(label)}:\s*(.+)$", block, re.MULTILINE)
    return match.group(1).strip() if match else ""


def count_field(block: str, label: str) -> int:
    if re.search(rf"^{re.escape(label)}:\s*none\s*$", block, re.MULTILINE):
        return 0
    match = re.search(rf"^{re.escape(label)}\s*\((\d+)\):", block, re.MULTILINE)
    return int(match.group(1)) if match else 0


def parse_exported() -> list[FunctionRecord]:
    source = (EXPORT_ROOT / "function_index.txt").read_text(encoding="utf-8").replace("\r", "")
    records: list[FunctionRecord] = []
    for block in source.split("=" * 80):
        name = field(block, "Function")
        address_text = field(block, "Address")
        if not name or not address_text:
            continue
        records.append(
            FunctionRecord(
                address=int(address_text, 16),
                ida_name=name,
                export_status=field(block, "Type"),
                exported_file=field(block, "File"),
                reason=field(block, "Fallback reason"),
                ida_called_by_count=count_field(block, "Called by"),
                ida_call_count=count_field(block, "Calls"),
            )
        )
    return records


def parse_skipped() -> list[FunctionRecord]:
    source = (EXPORT_ROOT / "decompile_skipped.txt").read_text(encoding="utf-8").replace("\r", "")
    records: list[FunctionRecord] = []
    pattern = re.compile(r"^(0x[0-9a-fA-F]+)\s*\|\s*([^|]+?)\s*\|\s*(.+?)\s*$", re.MULTILINE)
    for address_text, name, reason in pattern.findall(source):
        records.append(
            FunctionRecord(
                address=int(address_text, 16),
                ida_name=name,
                export_status="skipped",
                exported_file="",
                reason=reason,
                ida_called_by_count=0,
                ida_call_count=0,
            )
        )
    return records


def main() -> None:
    records = sorted(parse_exported() + parse_skipped(), key=lambda record: record.address)
    addresses = {record.address for record in records}
    if len(records) != 1486 or len(addresses) != 1486:
        raise SystemExit(
            f"unexpected function set: records={len(records)}, unique_addresses={len(addresses)}"
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "address",
                "ida_name",
                "export_status",
                "exported_file",
                "fallback_or_skip_reason",
                "ida_called_by_count",
                "ida_call_count",
            )
        )
        for record in records:
            writer.writerow(
                (
                    f"0x{record.address:08X}",
                    record.ida_name,
                    record.export_status,
                    record.exported_file,
                    record.reason,
                    record.ida_called_by_count,
                    record.ida_call_count,
                )
            )

    print(f"wrote {len(records)} functions to {OUTPUT.relative_to(WORKSPACE_ROOT)}")


if __name__ == "__main__":
    main()
