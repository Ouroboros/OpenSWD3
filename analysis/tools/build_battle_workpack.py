#!/usr/bin/env python3
from __future__ import annotations

import csv
from collections import Counter
from pathlib import Path

RESEARCH_ROOT = Path(__file__).resolve().parents[1]
REVERSE_ENGINEERING_ROOT = RESEARCH_ROOT / "04-reverse-engineering"
INVENTORY_ROOT = REVERSE_ENGINEERING_ROOT / "inventory"
OWNERSHIP_INPUT = INVENTORY_ROOT / "module-function-ownership.tsv"
OUTPUT = INVENTORY_ROOT / "battle-function-workpack.tsv"
EXPECTED_CANDIDATE_COUNT = 422
EXPECTED_FIRST_ADDRESS = "0x00433AA0"
EXPECTED_LAST_ADDRESS = "0x00484500"
EXPECTED_FAMILY_COUNTS = {
    "transferred_action_and_asset_helpers": 15,
    "battle_record_leaves": 2,
    "setup_frame_input_and_resolution": 93,
    "script_dispatch_ai_and_targeting": 77,
    "actor_actions_effects_and_rendering": 194,
    "shared_battle_object_services": 41,
}
EXPECTED_CONFIDENCE_COUNTS = {
    "confirmed_boundary": 61,
    "medium": 361,
}
ALLOWED_CLOSURE_STATUSES = {
    "pending_audit",
    "assembly_exact",
    "platform_adapted",
    "unreachable_current_assets",
    "blocked_runtime_oracle",
}

# Add an entry only after the complete authoritative LST body and every external
# FUNCTION CHUNK have been audited, implemented, tested, evidenced and committed.
CLOSURES: dict[str, tuple[str, str]] = {
    "0x00433AA0": (
        "platform_adapted",
        "evidence/battle-image-point-query-00433aa0.md",
    ),
    "0x00433C40": (
        "platform_adapted",
        "evidence/battle-render-geometry-initialization-00433c40.md",
    ),
    "0x00433D70": (
        "platform_adapted",
        "evidence/battle-render-resource-cleanup-00433d70.md",
    ),
    "0x00433DC0": (
        "platform_adapted",
        "evidence/battle-render-surface-rebuild-00433dc0-00437e90.md",
    ),
    "0x00433E20": (
        "platform_adapted",
        "evidence/battle-primary-row-offsets-00433e20.md",
    ),
    "0x00433E90": (
        "platform_adapted",
        "evidence/battle-surface-row-offsets-00433e90.md",
    ),
    "0x00433F00": (
        "platform_adapted",
        "evidence/battle-render-auxiliary-buffer-release-00433f00.md",
    ),
    "0x00433F30": (
        "assembly_exact",
        "evidence/battle-host-surface-setup-00433f30.md",
    ),
    "0x00433F70": (
        "platform_adapted",
        "evidence/battle-literal-image-cyclic-rotation-00433f70.md",
    ),
    "0x004342E0": (
        "assembly_exact",
        "evidence/battle-render-rectangle-004342e0.md",
    ),
    "0x00434350": (
        "assembly_exact",
        "evidence/battle-line-raster-step-00434350.md",
    ),
    "0x00434420": (
        "platform_adapted",
        "evidence/battle-direction-vector-raster-step-00434420.md",
    ),
    "0x004344E0": (
        "platform_adapted",
        "evidence/battle-directional-surface-scan-004344e0.md",
    ),
    "0x00434790": (
        "platform_adapted",
        "evidence/battle-image-particle-frame-00434790.md",
    ),
    "0x00434DD0": (
        "platform_adapted",
        "evidence/battle-image-particle-spawn-00434dd0.md",
    ),
    "0x0044FFC0": (
        "assembly_exact",
        "evidence/battle-action-threshold-0044ffc0.md",
    ),
    "0x0044FFE0": (
        "platform_adapted",
        "evidence/battle-border-panel-0044ffe0.md",
    ),
    "0x00450270": (
        "platform_adapted",
        "evidence/battle-frame-zero-draw-00450270.md",
    ),
    "0x004502B0": (
        "platform_adapted",
        "evidence/battle-action-frame-draw-004502b0.md",
    ),
    "0x00450400": (
        "platform_adapted",
        "evidence/battle-indexed-action-frame-draw-00450400.md",
    ),
    "0x00450490": (
        "platform_adapted",
        "evidence/battle-explicit-width-frame-draw-00450490.md",
    ),
    "0x004504E0": (
        "platform_adapted",
        "evidence/battle-selected-resource-frame-draw-004504e0.md",
    ),
    "0x00450530": (
        "platform_adapted",
        "evidence/battle-layered-resource-frame-draw-00450530.md",
    ),
    "0x004505B0": (
        "platform_adapted",
        "evidence/battle-layered-frame-two-draw-004505b0.md",
    ),
    "0x00450630": (
        "platform_adapted",
        "evidence/battle-layered-low-word-width-00450630.md",
    ),
    "0x004506B0": (
        "platform_adapted",
        "evidence/battle-decimal-frame-draw-004506b0.md",
    ),
    "0x004507A0": (
        "platform_adapted",
        "evidence/battle-ten-place-decimal-coordinator-004507a0.md",
    ),
    "0x00450900": (
        "platform_adapted",
        "evidence/battle-decimal-place-draw-00450900.md",
    ),
    "0x004509D0": (
        "platform_adapted",
        "evidence/battle-prepared-action-frame-draw-004509d0.md",
    ),
    "0x00450A50": (
        "platform_adapted",
        "evidence/battle-color-vertical-fade-00450a50.md",
    ),
    "0x00450A80": (
        "platform_adapted",
        "evidence/battle-offset-action-frame-draw-00450a80.md",
    ),
    "0x00450B40": (
        "assembly_exact",
        "evidence/battle-action-record-clear-00450b40.md",
    ),
    "0x00450B60": (
        "platform_adapted",
        "evidence/battle-standalone-action-frame-draw-00450b60.md",
    ),
    "0x00450BD0": (
        "platform_adapted",
        "evidence/battle-selected-cached-frame-draw-00450bd0.md",
    ),
    "0x00450C50": (
        "platform_adapted",
        "evidence/battle-vertical-status-panel-00450c50.md",
    ),
    "0x00450F90": (
        "platform_adapted",
        "evidence/battle-status-indicator-animation-00450f90.md",
    ),
}


def address_value(value: str) -> int:
    return int(value, 16)


def family(address: int) -> str:
    if 0x00433AA0 <= address <= 0x00434DD0:
        return "transferred_action_and_asset_helpers"
    if 0x0044FFC0 <= address <= 0x0044FFE0:
        return "battle_record_leaves"
    if 0x00450270 <= address <= 0x0045FC60:
        return "setup_frame_input_and_resolution"
    if 0x00460C40 <= address <= 0x0046FFF0:
        return "script_dispatch_ai_and_targeting"
    if 0x00470180 <= address <= 0x0047FC40:
        return "actor_actions_effects_and_rendering"
    if 0x004800F0 <= address <= 0x00484500:
        return "shared_battle_object_services"
    return "ownership_review_required"


def main() -> None:
    with OWNERSHIP_INPUT.open(encoding="utf-8", newline="") as source:
        rows = [
            row
            for row in csv.DictReader(source, delimiter="\t")
            if row["module_candidate"] == "battle"
            and row["code_origin"] == "game"
        ]
    if len(rows) != EXPECTED_CANDIDATE_COUNT:
        raise SystemExit(
            f"expected {EXPECTED_CANDIDATE_COUNT} battle candidates, "
            f"got {len(rows)}"
        )

    rows.sort(key=lambda row: address_value(row["address"]))
    addresses = {row["address"] for row in rows}
    if len(addresses) != len(rows):
        raise SystemExit("duplicate battle candidate address")
    if rows[0]["address"] != EXPECTED_FIRST_ADDRESS:
        raise SystemExit(f"unexpected first battle address: {rows[0]['address']}")
    if rows[-1]["address"] != EXPECTED_LAST_ADDRESS:
        raise SystemExit(f"unexpected last battle address: {rows[-1]['address']}")

    unknown_closures = sorted(set(CLOSURES) - addresses)
    if unknown_closures:
        raise SystemExit(f"closure address is outside battle scope: {unknown_closures}")

    output_rows = []
    for audit_order, row in enumerate(rows, start=1):
        address = address_value(row["address"])
        closure_status, closure_evidence = CLOSURES.get(
            row["address"], ("pending_audit", "")
        )
        if closure_status not in ALLOWED_CLOSURE_STATUSES:
            raise SystemExit(
                f"unsupported closure status for {row['address']}: "
                f"{closure_status}"
            )
        if closure_status == "pending_audit" and closure_evidence:
            raise SystemExit(
                f"pending closure unexpectedly has evidence: {row['address']}"
            )
        if closure_status != "pending_audit" and not closure_evidence:
            raise SystemExit(f"closed entry lacks evidence: {row['address']}")
        if closure_evidence:
            evidence_path = (REVERSE_ENGINEERING_ROOT / closure_evidence).resolve()
            if REVERSE_ENGINEERING_ROOT.resolve() not in evidence_path.parents:
                raise SystemExit(
                    f"closure evidence escapes reverse-engineering root: "
                    f"{row['address']}"
                )
            if not evidence_path.is_file():
                raise SystemExit(
                    f"closure evidence does not exist for {row['address']}: "
                    f"{closure_evidence}"
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
                "ownership and call-graph fields are navigation only; close from full LST body and external chunks",
            )
        )

    invalid_families = [
        row[1] for row in output_rows if row[3] == "ownership_review_required"
    ]
    if invalid_families:
        raise SystemExit(f"battle family is unresolved: {invalid_families}")

    family_counts = Counter(row[3] for row in output_rows)
    if family_counts != EXPECTED_FAMILY_COUNTS:
        raise SystemExit(f"unexpected battle family counts: {dict(family_counts)}")
    confidence_counts = Counter(row[4] for row in output_rows)
    if confidence_counts != EXPECTED_CONFIDENCE_COUNTS:
        raise SystemExit(
            f"unexpected battle confidence counts: {dict(confidence_counts)}"
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

    closed_count = sum(row[8] != "pending_audit" for row in output_rows)
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(output_rows)} rows)")
    if closed_count == EXPECTED_CANDIDATE_COUNT:
        print(
            f"closure {closed_count}/{EXPECTED_CANDIDATE_COUNT}; "
            "all candidates independently closed from full LST bodies"
        )
    else:
        print(
            f"closure {closed_count}/{EXPECTED_CANDIDATE_COUNT}; "
            "remaining candidates require independent LST audit"
        )


if __name__ == "__main__":
    main()
