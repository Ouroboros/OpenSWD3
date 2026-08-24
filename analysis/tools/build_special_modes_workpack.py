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
    "0x0044AF30": (
        "platform_adapted",
        "evidence/special-mode-catalog-initialization-0044af30.md",
    ),
    "0x0044AE70": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-overlay-value-0044ae70.md",
    ),
    "0x0044AB00": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-rebuild-0044ab00.md",
    ),
    "0x0044A280": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-render-0044a280.md",
    ),
    "0x0044A250": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-commit-0044a250.md",
    ),
    "0x0044A1D0": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-explicit-wrap-retreat-0044a1d0.md",
    ),
    "0x0044A160": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-mode-retreat-0044a160.md",
    ),
    "0x0044A0D0": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-mode-advance-0044a0d0.md",
    ),
    "0x0044A050": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-input-0044a050.md",
    ),
    "0x0044A030": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-release-0044a030.md",
    ),
    "0x00449FF0": (
        "platform_adapted",
        "evidence/special-mode-transition-pair-initialization-00449ff0.md",
    ),
    "0x00449D80": (
        "assembly_exact",
        "evidence/special-mode-transition-settings-profile-00449d80.md",
    ),
    "0x00449C30": (
        "platform_adapted",
        "evidence/special-mode-transition-panel-alpha-00449c30.md",
    ),
    "0x004490C0": (
        "platform_adapted",
        "evidence/special-mode-transition-frame-004490c0.md",
    ),
    "0x00449050": (
        "platform_adapted",
        "evidence/special-mode-transition-settings-commit-00449050.md",
    ),
    "0x00448EE0": (
        "platform_adapted",
        "evidence/special-mode-transition-confirmation-00448ee0.md",
    ),
    "0x00448EB0": (
        "assembly_exact",
        "evidence/special-mode-transition-mode-one-selection-advance-00448eb0.md",
    ),
    "0x00448DA0": (
        "platform_adapted",
        "evidence/special-mode-transition-setting-advance-00448da0.md",
    ),
    "0x00448CA0": (
        "platform_adapted",
        "evidence/special-mode-transition-setting-retreat-00448ca0.md",
    ),
    "0x00448C70": (
        "assembly_exact",
        "evidence/special-mode-transition-selection-first-00448c70.md",
    ),
    "0x00448C40": (
        "assembly_exact",
        "evidence/special-mode-transition-selection-last-00448c40.md",
    ),
    "0x00448C00": (
        "assembly_exact",
        "evidence/special-mode-transition-selection-retreat-00448c00.md",
    ),
    "0x00448BB0": (
        "assembly_exact",
        "evidence/special-mode-transition-selection-advance-00448bb0.md",
    ),
    "0x00448840": (
        "platform_adapted",
        "evidence/special-mode-transition-interaction-00448840.md",
    ),
    "0x00448700": (
        "platform_adapted",
        "evidence/special-mode-transition-visual-initialization-00448700.md",
    ),
    "0x00448650": (
        "platform_adapted",
        "evidence/special-mode-special-world-return-00448650.md",
    ),
    "0x004485F0": (
        "platform_adapted",
        "evidence/special-mode-special-world-transition-004485f0.md",
    ),
    "0x00448360": (
        "platform_adapted",
        "evidence/special-mode-resource-commit-00448360.md",
    ),
    "0x004482E0": (
        "platform_adapted",
        "evidence/special-mode-selection-record-cleanup-004482e0.md",
    ),
    "0x00448230": (
        "platform_adapted",
        "evidence/special-mode-selection-record-initialization-00448230.md",
    ),
    "0x00448020": (
        "platform_adapted",
        "evidence/special-mode-selection-record-clone-00448020.md",
    ),
    "0x00447100": (
        "platform_adapted",
        "evidence/special-mode-group-one-render-00447100.md",
    ),
    "0x00446FE0": (
        "platform_adapted",
        "evidence/special-mode-interaction-exit-00446fe0.md",
    ),
    "0x00446700": (
        "platform_adapted",
        "evidence/special-mode-multiphase-commit-00446700.md",
    ),
    "0x004466A0": (
        "platform_adapted",
        "evidence/special-mode-selection-cycle-runtime-advance-004466a0.md",
    ),
    "0x00446680": (
        "platform_adapted",
        "evidence/special-mode-selection-publish-runtime-advance-00446680.md",
    ),
    "0x00446550": (
        "platform_adapted",
        "evidence/special-mode-group-eight-mode-advance-00446550.md",
    ),
    "0x00446420": (
        "platform_adapted",
        "evidence/special-mode-group-eight-mode-retreat-00446420.md",
    ),
    "0x00446260": (
        "platform_adapted",
        "evidence/special-mode-group-eight-page-retreat-00446260.md",
    ),
    "0x00446090": (
        "platform_adapted",
        "evidence/special-mode-group-eight-page-advance-00446090.md",
    ),
    "0x00445E90": (
        "platform_adapted",
        "evidence/special-mode-group-eight-retreat-control-00445e90.md",
    ),
    "0x00445C90": (
        "platform_adapted",
        "evidence/special-mode-group-eight-advance-control-00445c90.md",
    ),
    "0x004455E0": (
        "platform_adapted",
        "evidence/special-mode-group-eight-main-input-004455e0.md",
    ),
    "0x004455A0": (
        "platform_adapted",
        "evidence/special-mode-group-eight-shared-cleanup-004455a0.md",
    ),
    "0x00445430": (
        "platform_adapted",
        "evidence/special-mode-group-eight-first-selection-initialization-00445430.md",
    ),
    "0x00445420": (
        "platform_adapted",
        "evidence/special-mode-group-eight-draw-dispatch-00445420.md",
    ),
    "0x004453F0": (
        "platform_adapted",
        "evidence/special-mode-group-eight-exit-004453f0.md",
    ),
    "0x00445360": (
        "platform_adapted",
        "evidence/special-mode-group-eight-selection-commit-00445360.md",
    ),
    "0x004452B0": (
        "platform_adapted",
        "evidence/special-mode-group-eight-selection-advance-004452b0.md",
    ),
    "0x00445210": (
        "platform_adapted",
        "evidence/special-mode-group-eight-selection-retreat-00445210.md",
    ),
    "0x004450E0": (
        "platform_adapted",
        "evidence/special-mode-group-eight-input-004450e0.md",
    ),
    "0x00444FC0": (
        "assembly_exact",
        "evidence/special-mode-callback-table-installation-00444fc0.md",
    ),
    "0x00444FB0": (
        "assembly_exact",
        "evidence/special-mode-equipment-final-action-count-00444fb0.md",
    ),
    "0x00444F60": (
        "platform_adapted",
        "evidence/special-mode-equipment-action-count-00444f60.md",
    ),
    "0x00444F00": (
        "platform_adapted",
        "evidence/special-mode-equipment-record-list-cleanup-00444f00.md",
    ),
    "0x00444E80": (
        "platform_adapted",
        "evidence/special-mode-equipment-record-list-rebuild-00444e80.md",
    ),
    "0x00444E50": (
        "assembly_exact",
        "evidence/special-mode-equipment-visible-count-00444e50.md",
    ),
    "0x00444DB0": (
        "platform_adapted",
        "evidence/special-mode-equipment-record-sort-00444db0.md",
    ),
    "0x004442B0": (
        "platform_adapted",
        "evidence/special-mode-equipment-render-004442b0.md",
    ),
    "0x004441A0": (
        "platform_adapted",
        "evidence/special-mode-equipment-exit-004441a0.md",
    ),
    "0x00443BD0": (
        "platform_adapted",
        "evidence/special-mode-equipment-commit-00443bd0.md",
    ),
    "0x00443B70": (
        "platform_adapted",
        "evidence/special-mode-equipment-list-kind-cycle-00443b70.md",
    ),
    "0x00443A60": (
        "platform_adapted",
        "evidence/special-mode-equipment-party-cycle-00443a60.md",
    ),
    "0x004439A0": (
        "platform_adapted",
        "evidence/special-mode-equipment-column-advance-004439a0.md",
    ),
    "0x004438E0": (
        "platform_adapted",
        "evidence/special-mode-equipment-column-toggle-004438e0.md",
    ),
    "0x004437C0": (
        "platform_adapted",
        "evidence/special-mode-equipment-page-retreat-004437c0.md",
    ),
    "0x00443670": (
        "platform_adapted",
        "evidence/special-mode-equipment-page-advance-00443670.md",
    ),
    "0x00443570": (
        "platform_adapted",
        "evidence/special-mode-equipment-retreat-00443570.md",
    ),
    "0x00443450": (
        "platform_adapted",
        "evidence/special-mode-equipment-advance-00443450.md",
    ),
    "0x00442F40": (
        "platform_adapted",
        "evidence/special-mode-equipment-input-dispatch-00442f40.md",
    ),
    "0x00442F10": (
        "platform_adapted",
        "evidence/special-mode-equipment-cleanup-00442f10.md",
    ),
    "0x00442E40": (
        "platform_adapted",
        "evidence/special-mode-equipment-initialization-00442e40.md",
    ),
    "0x00442D70": (
        "platform_adapted",
        "evidence/special-mode-guardian-record-exchange-attributes-00442d70.md",
    ),
    "0x00442CA0": (
        "platform_adapted",
        "evidence/special-mode-guardian-attribute-summary-00442ca0.md",
    ),
    "0x00442BC0": (
        "platform_adapted",
        "evidence/special-mode-guardian-party-attribute-finalize-00442bc0.md",
    ),
    "0x00442B10": (
        "platform_adapted",
        "evidence/special-mode-guardian-selected-attributes-00442b10.md",
    ),
    "0x00442AA0": (
        "platform_adapted",
        "evidence/special-mode-guardian-party-attributes-00442aa0.md",
    ),
    "0x00442A40": (
        "platform_adapted",
        "evidence/special-mode-guardian-attribute-seed-00442a40.md",
    ),
    "0x004429B0": (
        "platform_adapted",
        "evidence/special-mode-guardian-attribute-cache-refresh-004429b0.md",
    ),
    "0x00442960": (
        "platform_adapted",
        "evidence/special-mode-guardian-category-icon-00442960.md",
    ),
    "0x004425C0": (
        "platform_adapted",
        "evidence/special-mode-guardian-slot-panel-004425c0.md",
    ),
    "0x00442130": (
        "platform_adapted",
        "evidence/special-mode-guardian-attribute-panel-00442130.md",
    ),
    "0x004420F0": (
        "platform_adapted",
        "evidence/special-mode-guardian-record-list-drain-004420f0.md",
    ),
    "0x00442050": (
        "platform_adapted",
        "evidence/special-mode-guardian-record-list-refresh-00442050.md",
    ),
    "0x00441F70": (
        "assembly_exact",
        "evidence/special-mode-guardian-record-filter-00441f70.md",
    ),
    "0x00441680": (
        "platform_adapted",
        "evidence/special-mode-guardian-render-00441680.md",
    ),
    "0x00441590": (
        "platform_adapted",
        "evidence/special-mode-guardian-interaction-commit-00441590.md",
    ),
    "0x00441160": (
        "platform_adapted",
        "evidence/special-mode-guardian-interaction-switch-00441160.md",
    ),
    "0x00441060": (
        "platform_adapted",
        "evidence/special-mode-guardian-party-cycle-00441060.md",
    ),
    "0x00440FB0": (
        "platform_adapted",
        "evidence/special-mode-guardian-repeat-advance-00440fb0.md",
    ),
    "0x00440F00": (
        "platform_adapted",
        "evidence/special-mode-guardian-repeat-retreat-00440f00.md",
    ),
    "0x00440E10": (
        "platform_adapted",
        "evidence/special-mode-guardian-page-retreat-00440e10.md",
    ),
    "0x00440D20": (
        "platform_adapted",
        "evidence/special-mode-guardian-page-advance-00440d20.md",
    ),
    "0x00440C20": (
        "platform_adapted",
        "evidence/special-mode-guardian-selection-retreat-00440c20.md",
    ),
    "0x00440B20": (
        "platform_adapted",
        "evidence/special-mode-guardian-selection-00440b20.md",
    ),
    "0x004407F0": (
        "platform_adapted",
        "evidence/special-mode-guardian-input-dispatch-004407f0.md",
    ),
    "0x00440630": (
        "platform_adapted",
        "evidence/special-mode-guardian-initialization-00440630.md",
    ),
    "0x004405C0": (
        "platform_adapted",
        "evidence/special-mode-altar-surface-release-004405c0.md",
    ),
    "0x004404D0": (
        "assembly_exact",
        "evidence/special-mode-altar-attributes-004404d0.md",
    ),
    "0x004400A0": (
        "platform_adapted",
        "evidence/special-mode-altar-animation-004400a0.md",
    ),
    "0x0043FDE0": (
        "platform_adapted",
        "evidence/special-mode-original-surface-preparation-0043fde0.md",
    ),
    "0x0043FA70": (
        "platform_adapted",
        "evidence/special-mode-altar-record-panel-0043fa70.md",
    ),
    "0x0043F940": (
        "platform_adapted",
        "evidence/special-mode-synthesis-inline-record-refresh-0043f940.md",
    ),
    "0x0043F880": (
        "platform_adapted",
        "evidence/special-mode-database-window-refresh-0043f880.md",
    ),
    "0x0043F7C0": (
        "platform_adapted",
        "evidence/special-mode-database-record-filter-0043f7c0.md",
    ),
    "0x0043F1E0": (
        "platform_adapted",
        "evidence/special-mode-database-runtime-record-refresh-0043f1e0.md",
    ),
    "0x0043F160": (
        "platform_adapted",
        "evidence/special-mode-database-forward-sort-0043f160.md",
    ),
    "0x0043F0D0": (
        "platform_adapted",
        "evidence/special-mode-database-forward-build-0043f0d0.md",
    ),
    "0x0043F080": (
        "platform_adapted",
        "evidence/special-mode-database-forward-release-0043f080.md",
    ),
    "0x0043F000": (
        "platform_adapted",
        "evidence/special-mode-database-forward-refresh-0043f000.md",
    ),
    "0x0043E800": (
        "platform_adapted",
        "evidence/special-mode-database-render-0043e800.md",
    ),
    "0x0043E770": (
        "platform_adapted",
        "evidence/special-mode-database-interaction-exit-0043e770.md",
    ),
    "0x0043E3D0": (
        "platform_adapted",
        "evidence/special-mode-database-interaction-commit-0043e3d0.md",
    ),
    "0x0043E310": (
        "platform_adapted",
        "evidence/special-mode-database-primary-direction-0043e310.md",
    ),
    "0x0043E250": (
        "platform_adapted",
        "evidence/special-mode-database-direction-cycle-0043e250.md",
    ),
    "0x0043E170": (
        "platform_adapted",
        "evidence/special-mode-database-page-source-advance-0043e170.md",
    ),
    "0x0043E080": (
        "platform_adapted",
        "evidence/special-mode-database-page-cycle-0043e080.md",
    ),
    "0x0043DFA0": (
        "platform_adapted",
        "evidence/special-mode-database-page-retreat-0043dfa0.md",
    ),
    "0x0043DED0": (
        "platform_adapted",
        "evidence/special-mode-database-page-advance-0043ded0.md",
    ),
    "0x0043DDF0": (
        "platform_adapted",
        "evidence/special-mode-database-forward-retreat-0043ddf0.md",
    ),
    "0x0043DD20": (
        "platform_adapted",
        "evidence/special-mode-database-forward-advance-0043dd20.md",
    ),
    "0x0043DA30": (
        "platform_adapted",
        "evidence/special-mode-database-input-dispatch-0043da30.md",
    ),
    "0x0043D880": (
        "platform_adapted",
        "evidence/special-mode-database-cleanup-0043d880.md",
    ),
    "0x0043D530": (
        "platform_adapted",
        "evidence/special-mode-database-initialization-0043d530.md",
    ),
    "0x0043D470": (
        "platform_adapted",
        "evidence/special-mode-mode-strip-0043d470.md",
    ),
    "0x0043D370": (
        "platform_adapted",
        "evidence/special-mode-derived-text-0043d370.md",
    ),
    "0x0043D050": (
        "platform_adapted",
        "evidence/special-mode-selected-record-display-0043d050.md",
    ),
    "0x0043CEF0": (
        "platform_adapted",
        "evidence/special-mode-entry-consumption-0043cef0.md",
    ),
    "0x0043CC20": (
        "platform_adapted",
        "evidence/special-mode-entry-render-0043cc20.md",
    ),
    "0x0043CC00": (
        "assembly_exact",
        "evidence/special-mode-entry-alias-0043cc00.md",
    ),
    "0x0043CBD0": (
        "assembly_exact",
        "evidence/special-mode-page-refresh-0043cbd0.md",
    ),
    "0x0043C9C0": (
        "platform_adapted",
        "evidence/special-mode-entry-initialization-0043c9c0.md",
    ),
    "0x0043C820": (
        "platform_adapted",
        "evidence/special-mode-runtime-render-0043c820.md",
    ),
    "0x0043C760": (
        "platform_adapted",
        "evidence/special-mode-runtime-mode-advance-0043c760.md",
    ),
    "0x0043C670": (
        "platform_adapted",
        "evidence/special-mode-runtime-page-retreat-0043c670.md",
    ),
    "0x0043C590": (
        "platform_adapted",
        "evidence/special-mode-runtime-cursor-retreat-0043c590.md",
    ),
    "0x0043C520": (
        "platform_adapted",
        "evidence/special-mode-runtime-cursor-advance-0043c520.md",
    ),
    "0x0043C3C0": (
        "platform_adapted",
        "evidence/special-mode-runtime-input-dispatch-0043c3c0.md",
    ),
    "0x0043C0D0": (
        "platform_adapted",
        "evidence/special-mode-runtime-initialization-0043c0d0.md",
    ),
    "0x0043C090": (
        "platform_adapted",
        "evidence/special-mode-availability-0043c090.md",
    ),
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
