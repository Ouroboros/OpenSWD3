#!/usr/bin/env python3
"""Build the B9 special-modes audit workpack from the frozen ownership inventory."""

from __future__ import annotations

import csv
from pathlib import Path

TOOL_ROOT = Path(__file__).resolve().parent
RESEARCH_ROOT = TOOL_ROOT.parent
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
OWNERSHIP_INPUT = INVENTORY_ROOT / "module-function-ownership.tsv"
OUTPUT = INVENTORY_ROOT / "special-modes-function-workpack.tsv"
EXPECTED_CANDIDATE_COUNT = 227
CLOSURES = {
    "0x0043BFC0": (
        "platform_adapted",
        "evidence/special-mode-dialog-setup-0043bfc0.md",
    ),
    "0x0043BE90": (
        "platform_adapted",
        "evidence/special-mode-filtered-records-0043be90.md",
    ),
    "0x0043BE40": (
        "platform_adapted",
        "evidence/special-mode-value-group-0043be40.md",
    ),
    "0x0043BD70": (
        "platform_adapted",
        "evidence/special-mode-animated-panel-0043bd70.md",
    ),
    "0x0043BCC0": (
        "platform_adapted",
        "evidence/special-mode-window-selection-0043bcc0.md",
    ),
    "0x0043BC90": (
        "platform_adapted",
        "evidence/special-mode-bounded-forward-count-0043bc90.md",
    ),
    "0x0043BC60": (
        "platform_adapted",
        "evidence/special-mode-window-page-retreat-0043bc60.md",
    ),
    "0x0043BBE0": (
        "platform_adapted",
        "evidence/special-mode-window-page-advance-0043bbe0.md",
    ),
    "0x0043BBC0": (
        "platform_adapted",
        "evidence/special-mode-window-cursor-retreat-0043bbc0.md",
    ),
    "0x0043BB80": (
        "platform_adapted",
        "evidence/special-mode-window-cursor-advance-0043bb80.md",
    ),
    "0x0043BB40": (
        "platform_adapted",
        "evidence/special-mode-window-cursor-0043bb40.md",
    ),
    "0x0043BA40": (
        "platform_adapted",
        "evidence/special-mode-input-status-0043ba40.md",
    ),
    "0x0043B9E0": (
        "platform_adapted",
        "evidence/special-mode-shared-text-0043b9e0.md",
    ),
    "0x0043B9C0": (
        "platform_adapted",
        "evidence/special-mode-forward-index-0043b9c0.md",
    ),
    "0x0043B9A0": (
        "platform_adapted",
        "evidence/special-mode-forward-advance-0043b9a0.md",
    ),
    "0x0043B980": (
        "platform_adapted",
        "evidence/special-mode-forward-count-0043b980.md",
    ),
    "0x0043B480": (
        "platform_adapted",
        "evidence/special-mode-callback-binding-0043b480.md",
    ),
    "0x0043B080": (
        "platform_adapted",
        "evidence/special-mode-ghost-action-0043b080.md",
    ),
    "0x0043AE40": (
        "platform_adapted",
        "evidence/special-mode-split-bar-0043ae40.md",
    ),
    "0x0043AAA0": (
        "platform_adapted",
        "evidence/special-mode-transition-blocks-0043aaa0.md",
    ),
    "0x0043A880": (
        "platform_adapted",
        "evidence/special-mode-panel-preparation-0043a880.md",
    ),
    "0x0043A610": (
        "platform_adapted",
        "evidence/special-mode-frame-render-0043a610.md",
    ),
    "0x0043A470": (
        "platform_adapted",
        "evidence/special-mode-input-dispatch-0043a470.md",
    ),
    "0x0043A380": (
        "platform_adapted",
        "evidence/special-mode-item-availability-0043a380.md",
    ),
    "0x0043A2A0": (
        "platform_adapted",
        "evidence/special-mode-selector-initialization-0043a2a0.md",
    ),
    "0x00439DE0": (
        "platform_adapted",
        "evidence/special-mode-global-initialization-00439de0.md",
    ),
    "0x00439FD0": (
        "platform_adapted",
        "evidence/special-mode-standard-dispatch-00439fd0.md",
    ),
}


def address_value(value: str) -> int:
    return int(value, 16)


def family(address: int) -> str:
    if 0x00406D30 <= address <= 0x004070A0:
        return "high_priority_dispatch"
    if 0x0040F890 <= address <= 0x00411700:
        return "shared_dialog_and_party_helpers"
    if 0x00439DE0 <= address < 0x0044EA60:
        return "standard_modes_1_3_4_5_6"
    if 0x0044EA60 <= address <= 0x0044FF90:
        return "shop_mode_2"
    return "ownership_review_required"


def main() -> None:
    with OWNERSHIP_INPUT.open(encoding="utf-8", newline="") as source:
        rows = [
            row
            for row in csv.DictReader(source, delimiter="\t")
            if row["module_candidate"] == "special_modes"
        ]
    if len(rows) != EXPECTED_CANDIDATE_COUNT:
        raise SystemExit(
            f"expected {EXPECTED_CANDIDATE_COUNT} special-mode candidates, "
            f"got {len(rows)}"
        )

    rows.sort(key=lambda row: address_value(row["address"]))
    output_rows = []
    for audit_order, row in enumerate(rows, start=1):
        address = address_value(row["address"])
        closure_status, closure_evidence = CLOSURES.get(
            row["address"], ("pending_audit", "")
        )
        output_rows.append(
            (
                audit_order,
                row["address"],
                row["ida_name_navigation_only"],
                family(address),
                row["candidate_confidence"],
                row["assembly_direct_callers_address_count"],
                row["assembly_direct_callees_address_count"],
                row["review_status"],
                closure_status,
                closure_evidence,
                "ownership and call-graph fields are navigation only; close from full LST body",
            )
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as destination:
        writer = csv.writer(destination, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "audit_order",
                "address",
                "navigation_name",
                "entry_family",
                "candidate_confidence_navigation",
                "direct_callers_navigation",
                "direct_callees_navigation",
                "prior_review_navigation",
                "closure_status",
                "closure_evidence",
                "closure_rule",
            )
        )
        writer.writerows(output_rows)
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(output_rows)} rows)")
    closed_count = sum(row[8] != "pending_audit" for row in output_rows)
    print(
        f"closure {closed_count}/{EXPECTED_CANDIDATE_COUNT}; "
        "remaining candidates require independent LST audit"
    )


if __name__ == "__main__":
    main()
