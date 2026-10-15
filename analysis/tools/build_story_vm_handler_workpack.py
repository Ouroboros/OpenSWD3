#!/usr/bin/env python3
"""Build the P1 story-VM handler and non-handler runtime work packages.

The primary dispatch is regenerated from the locked full LST before this tool
reads it.  Existing length, static-triage, direct-effect, semantic and TALK
coverage inventories are navigation aids only: every handler row starts as
pending regardless of prior prose, C++ case presence, or asset coverage.
"""

from __future__ import annotations

import csv
import re
from collections import defaultdict
from pathlib import Path

import build_story_vm_dispatch_inventory as dispatch

RESEARCH_ROOT = Path(__file__).resolve().parents[1]
PROJECT_ROOT = RESEARCH_ROOT.parent
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"

DISPATCH_INPUT = INVENTORY_ROOT / "story-vm-opcode-dispatch.tsv"
LENGTH_INPUT = INVENTORY_ROOT / "story-vm-opcode-length-rules.tsv"
TRIAGE_INPUT = INVENTORY_ROOT / "story-vm-opcode-static-triage.tsv"
COVERAGE_INPUT = INVENTORY_ROOT / "story-vm-talk-opcode-coverage.tsv"
CONTROL_TRANSFER_INPUT = INVENTORY_ROOT / "story-vm-control-transfer-rules.tsv"
OWNERSHIP_INPUT = INVENTORY_ROOT / "module-function-ownership.tsv"
STORY_VM_SOURCE = PROJECT_ROOT / "src" / "world_map" / "legacy_world_story_vm.cpp"
STORY_VM_HEADER = (
    PROJECT_ROOT / "include" / "openswd3" / "world_map" / "legacy_world_story_vm.hpp"
)
SEMANTIC_INPUTS = tuple(
    INVENTORY_ROOT / f"story-vm-opcode-semantics-{start:03d}-{start + 24:03d}.tsv"
    for start in range(0, 125, 25)
) + (INVENTORY_ROOT / "story-vm-opcode-semantics-125-193.tsv",)

HANDLER_OUTPUT = INVENTORY_ROOT / "story-vm-handler-workpack.tsv"
RUNTIME_OUTPUT = INVENTORY_ROOT / "story-vm-runtime-paths.tsv"

EXPECTED_EXPLICIT_OPCODES = tuple(range(194)) + (1024, 1025, 1026, 16383)
EXPECTED_HANDLER_COUNT = 146
EXPECTED_SHARED_HANDLER_COUNT = 25
EXPECTED_MODERN_CASE_COUNT = 197
EXPECTED_CLOSED_HANDLER_COUNT = 0

CLOSURE_OVERRIDES = {
    "0x0042D230": (
        "pending",
        "story-vm-default-invalid-0042d230.md",
        "pending",
    ),
    "0x00427B8F": (
        "pending",
        "story-vm-dialog-handler-00427b8f.md",
        "pending",
    ),
    "0x00427E72": (
        "pending",
        "story-vm-dialog-flag-clear-00427e72.md",
        "pending",
    ),
    "0x00427E9A": (
        "pending",
        "story-vm-dialog-lifetime-00427e9a.md",
        "pending",
    ),
    "0x00427EC2": (
        "pending",
        "story-vm-dialog-flag-clear-00427ec2.md",
        "pending",
    ),
    "0x00427ED0": (
        "pending",
        "story-vm-role-base-variant-00427ed0.md",
        "pending",
    ),
    "0x00427FEB": (
        "pending",
        "story-vm-role-variant-delta-00427feb.md",
        "pending",
    ),
    "0x0042811F": (
        "pending",
        "story-vm-role-position-0042811f.md",
        "pending",
    ),
    "0x0042822A": (
        "pending",
        "story-vm-role-step-0042822a.md",
        "pending",
    ),
    "0x0042829C": (
        "pending",
        "story-vm-role-action-wait-0042829c.md",
        "pending",
    ),
    "0x00428310": (
        "pending",
        "story-vm-same-file-jump-00428310.md",
        "pending",
    ),
    "0x00428318": (
        "pending",
        "story-vm-role-path-conditional-jump-00428318.md",
        "pending",
    ),
    "0x004283AC": (
        "pending",
        "story-vm-role-path-prepared-jump-004283ac.md",
        "pending",
    ),
    "0x0042845A": (
        "pending",
        "story-vm-role-path-release-0042845a.md",
        "pending",
    ),
    "0x004284C2": (
        "pending",
        "story-vm-role-path-release-all-004284c2.md",
        "pending",
    ),
    "0x0042ADB7": (
        "pending",
        "story-vm-role-path-schedule-0042adb7.md",
        "pending",
    ),
    "0x00428533": (
        "pending",
        "story-vm-global-bit-conditional-jump-00428533.md",
        "pending",
    ),
    "0x0042857F": (
        "pending",
        "story-vm-all-global-bits-conditional-jump-0042857f.md",
        "pending",
    ),
    "0x004285ED": (
        "pending",
        "story-vm-any-global-bit-conditional-jump-004285ed.md",
        "pending",
    ),
    "0x0042865B": (
        "pending",
        "story-vm-global-bit-set-0042865b.md",
        "pending",
    ),
    "0x00428679": (
        "pending",
        "story-vm-global-bit-clear-00428679.md",
        "pending",
    ),
    "0x004286C5": (
        "pending",
        "story-vm-world-session-reload-004286c5.md",
        "pending",
    ),
    "0x00428713": (
        "pending",
        "story-vm-role-path-id-change-00428713.md",
        "pending",
    ),
    "0x0042B074": (
        "pending",
        "story-vm-global-integers-0042b074.md",
        "pending",
    ),
    "0x0042890F": (
        "pending",
        "story-vm-script-clock-set-0042890f.md",
        "pending",
    ),
    "0x00428934": (
        "pending",
        "story-vm-script-clock-byte-jump-00428934.md",
        "pending",
    ),
    "0x0042896C": (
        "pending",
        "story-vm-script-clock-origin-delta-jump-0042896c.md",
        "pending",
    ),
    "0x004289BE": (
        "pending",
        "story-vm-script-clock-snapshot-004289be.md",
        "pending",
    ),
    "0x004289DE": (
        "pending",
        "story-vm-role-scene-clear-004289de.md",
        "pending",
    ),
    "0x00428ADC": (
        "pending",
        "story-vm-role-flag-8000-00428adc.md",
        "pending",
    ),
    "0x00428BA0": (
        "pending",
        "story-vm-role-relocation-completion-00428ba0.md",
        "pending",
    ),
    "0x00428C9F": (
        "pending",
        "story-vm-indexed-target-reload-00428c9f.md",
        "pending",
    ),
    "0x00428D18": (
        "pending",
        "story-vm-interaction-lock-00428d18.md",
        "pending",
    ),
    "0x00428DB8": (
        "pending",
        "story-vm-role-action-wait-override-00428db8.md",
        "pending",
    ),
    "0x00428E52": (
        "pending",
        "story-vm-role-action-id-00428e52.md",
        "pending",
    ),
    "0x00428F7B": (
        "pending",
        "story-vm-role-action-override-restore-00428f7b.md",
        "pending",
    ),
    "0x00429066": (
        "pending",
        "story-vm-camera-move-00429066.md",
        "pending",
    ),
    "0x00429362": (
        "pending",
        "story-vm-camera-wait-00429362.md",
        "pending",
    ),
    "0x004293AC": (
        "pending",
        "story-vm-frame-color-transition-004293ac.md",
        "pending",
    ),
    "0x0042949D": (
        "pending",
        "story-vm-frame-color-wait-0042949d.md",
        "pending",
    ),
    "0x004294C0": (
        "pending",
        "story-vm-role-action-repeat-refresh-004294c0.md",
        "pending",
    ),
    "0x004295F3": (
        "pending",
        "story-vm-role-spatial-groups-004295f3.md",
        "pending",
    ),
    "0x0042B1F1": (
        "pending",
        "story-vm-picture-action-enqueue-0042b1f1.md",
        "pending",
    ),
    "0x0042967B": (
        "pending",
        "story-vm-sound-effect-0042967b.md",
        "pending",
    ),
    "0x00429693": (
        "pending",
        "story-vm-scene-render-control-00429693.md",
        "pending",
    ),
    "0x004296DE": (
        "pending",
        "story-vm-map-role-write-004296de.md",
        "pending",
    ),
    "0x00429A1B": (
        "pending",
        "story-vm-selection-scroll-write-00429a1b.md",
        "pending",
    ),
    "0x00429AD2": (
        "pending",
        "story-vm-selection-scroll-clear-00429ad2.md",
        "pending",
    ),
    "0x00429AE8": (
        "pending",
        "story-vm-role-transfer-00429ae8.md",
        "pending",
    ),
    "0x00429B14": (
        "pending",
        "story-vm-role-map-update-00429b14.md",
        "pending",
    ),
    "0x00429B62": (
        "pending",
        "story-vm-frame-clock-wait-00429b62.md",
        "pending",
    ),
    "0x00429BB5": (
        "pending",
        "story-vm-role-flag-0400-clear-00429bb5.md",
        "pending",
    ),
    "0x00429C37": (
        "pending",
        "story-vm-role-flag-0400-set-00429c37.md",
        "pending",
    ),
    "0x00429CBC": (
        "pending",
        "story-vm-role-head-sign-00429cbc.md",
        "pending",
    ),
    "0x00429D0F": (
        "pending",
        "story-vm-role-head-sign-clear-00429d0f.md",
        "pending",
    ),
    "0x00429D43": (
        "pending",
        "story-vm-frame-color-cancel-00429d43.md",
        "pending",
    ),
    "0x00429D70": (
        "pending",
        "story-vm-role-suspend-00429d70.md",
        "pending",
    ),
    "0x00429DA6": (
        "pending",
        "story-vm-role-turn-suspend-00429da6.md",
        "pending",
    ),
    "0x00429F7B": (
        "pending",
        "story-vm-role-wait-override-00429f7b.md",
        "pending",
    ),
    "0x0042A0A6": (
        "pending",
        "story-vm-moving-action-enqueue-0042a0a6.md",
        "pending",
    ),
    "0x0042A1EF": (
        "pending",
        "story-vm-text-control-bit29-clear-0042a1ef.md",
        "pending",
    ),
    "0x0042A200": (
        "pending",
        "story-vm-role-head-action-enqueue-0042a200.md",
        "pending",
    ),
    "0x0042A2C6": (
        "pending",
        "story-vm-role-head-action-dismiss-0042a2c6.md",
        "pending",
    ),
    "0x0042A341": (
        "pending",
        "story-vm-packed-row-effect-upsert-0042a341.md",
        "pending",
    ),
    "0x0042A54C": (
        "pending",
        "story-vm-packed-row-effect-control-0042a54c.md",
        "pending",
    ),
    "0x0042A611": (
        "pending",
        "story-vm-video-start-0042a611.md",
        "pending",
    ),
    "0x0042A673": (
        "pending",
        "story-vm-role-head-action-key-rewrite-0042a673.md",
        "pending",
    ),
    "0x0042A6CB": (
        "pending",
        "story-vm-random-target-reload-0042a6cb.md",
        "pending",
    ),
    "0x0042A727": (
        "pending",
        "story-vm-battle-request-0042a727.md",
        "pending",
    ),
    "0x0042B287": (
        "pending",
        "story-vm-name-record-load-0042b287.md",
        "pending",
    ),
    "0x0042A756": (
        "pending",
        "story-vm-reserved-global-bit-set-0042a756.md",
        "pending",
    ),
    "0x0042A792": (
        "pending",
        "story-vm-reserved-global-bit-clear-0042a792.md",
        "pending",
    ),
    "0x0042A7CE": (
        "pending",
        "story-vm-scene-render-bit1-set-0042a7ce.md",
        "pending",
    ),
    "0x0042A7EE": (
        "pending",
        "story-vm-scene-render-bit1-clear-0042a7ee.md",
        "pending",
    ),
    "0x0042A80E": (
        "pending",
        "story-vm-custom-ani-start-0042a80e.md",
        "pending",
    ),
    "0x0042AD3C": (
        "pending",
        "story-vm-custom-ani-wait-0042ad3c.md",
        "pending",
    ),
    "0x0042C7EA": (
        "pending",
        "story-vm-four-byte-noop-0042c7ea.md",
        "pending",
    ),
    "0x0042AD75": (
        "pending",
        "story-vm-custom-ani-phase-wait-0042ad75.md",
        "pending",
    ),
    "0x0042B3B0": (
        "pending",
        "story-vm-role-talk-script-write-0042b3b0.md",
        "pending",
    ),
    "0x0042B43B": (
        "pending",
        "story-vm-role-status-bit26-set-0042b43b.md",
        "pending",
    ),
    "0x0042C567": (
        "pending",
        "story-vm-role-status-boolean-flags-0042c567.md",
        "pending",
    ),
    "0x0042B47E": (
        "pending",
        "story-vm-text-layout-pair-0042b47e.md",
        "pending",
    ),
    "0x0042B4B9": (
        "pending",
        "story-vm-text-control-bit27-clear-0042b4b9.md",
        "pending",
    ),
    "0x0042B4CA": (
        "pending",
        "story-vm-picture-action-byte-wait-0042b4ca.md",
        "pending",
    ),
    "0x0042B50F": (
        "pending",
        "story-vm-role-action-index-wait-0042b50f.md",
        "pending",
    ),
    "0x0042B5F2": (
        "pending",
        "story-vm-next-dialog-anchor-0042b5f2.md",
        "pending",
    ),
    "0x0042B63C": (
        "pending",
        "story-vm-role-step-list-0042b63c.md",
        "pending",
    ),
    "0x0042B6A5": (
        "pending",
        "story-vm-secondary-role-bit30-reload-0042b6a5.md",
        "pending",
    ),
    "0x0042B70C": (
        "pending",
        "story-vm-overlay-action-lists-wait-0042b70c.md",
        "pending",
    ),
    "0x0042B723": (
        "pending",
        "story-vm-sound-effect-unread-padding-0042b723.md",
        "pending",
    ),
    "0x0042B739": (
        "pending",
        "story-vm-scene-music-stream-request-0042b739.md",
        "pending",
    ),
    "0x0042B7FC": (
        "pending",
        "story-vm-music-stream-volume-0042b7fc.md",
        "pending",
    ),
    "0x0042B83A": (
        "pending",
        "story-vm-batch-role-position-0042b83a.md",
        "pending",
    ),
    "0x0042B8E6": (
        "pending",
        "story-vm-dialog-role-remove-0042b8e6.md",
        "pending",
    ),
    "0x0042B9C2": (
        "pending",
        "story-vm-dialog-flag-wait-0042b9c2.md",
        "pending",
    ),
    "0x0042BAB8": (
        "pending",
        "story-vm-role-action-fields-0042bab8.md",
        "pending",
    ),
    "0x0042BC2C": (
        "pending",
        "story-vm-text-control-bit26-clear-0042bc2c.md",
        "pending",
    ),
    "0x0042BC3D": (
        "pending",
        "story-vm-speed-mode-clear-0042bc3d.md",
        "pending",
    ),
    "0x0042BC4C": (
        "pending",
        "story-vm-scene-music-table-entry-0042bc4c.md",
        "pending",
    ),
    "0x0042BCF5": (
        "pending",
        "story-vm-text-control-bit25-clear-0042bcf5.md",
        "pending",
    ),
    "0x0042BD06": (
        "pending",
        "story-vm-text-allocation-append-0042bd06.md",
        "pending",
    ),
    "0x0042BDBC": (
        "pending",
        "story-vm-role-base-variant-reload-0042bdbc.md",
        "pending",
    ),
    "0x0042BE8A": (
        "pending",
        "story-vm-player-item-quantity-0042be8a.md",
        "pending",
    ),
    "0x0042BEFE": (
        "pending",
        "story-vm-item-presence-reload-0042befe.md",
        "pending",
    ),
    "0x0042BF72": (
        "pending",
        "story-vm-party-item-conditional-add-0042bf72.md",
        "pending",
    ),
    "0x0042C033": (
        "pending",
        "story-vm-player-item-role-slot-swap-0042c033.md",
        "pending",
    ),
    "0x0042C234": (
        "pending",
        "story-vm-shop-request-0042c234.md",
        "pending",
    ),
    "0x0042C2C6": (
        "pending",
        "story-vm-party-member-resources-0042c2c6.md",
        "pending",
    ),
    "0x0042C3B0": (
        "pending",
        "story-vm-input-menu-reset-0042c3b0.md",
        "pending",
    ),
    "0x0042C3F7": (
        "pending",
        "story-vm-scene-music-stream-stop-0042c3f7.md",
        "pending",
    ),
    "0x0042C49E": (
        "pending",
        "story-vm-role-distance-reload-0042c49e.md",
        "pending",
    ),
    "0x0042C6DD": (
        "pending",
        "story-vm-music-stream-transition-config-0042c6dd.md",
        "pending",
    ),
    "0x0042C732": (
        "pending",
        "story-vm-primary-countdown-initialize-0042c732.md",
        "pending",
    ),
    "0x0042C766": (
        "pending",
        "story-vm-primary-countdown-disable-0042c766.md",
        "pending",
    ),
    "0x0042C79D": (
        "pending",
        "story-vm-special-mode-four-five-request-0042c79d.md",
        "pending",
    ),
    "0x0042C7FB": (
        "pending",
        "story-vm-fixed-story-flag-70-set-0042c7fb.md",
        "pending",
    ),
    "0x0042C81A": (
        "pending",
        "story-vm-fixed-story-flag-19-set-0042c81a.md",
        "pending",
    ),
    "0x0042C839": (
        "pending",
        "story-vm-fixed-story-flag-19-clear-0042c839.md",
        "pending",
    ),
    "0x0042C858": (
        "pending",
        "story-vm-ani-follower-position-config-0042c858.md",
        "pending",
    ),
    "0x0042C8F8": (
        "pending",
        "story-vm-ani-follower-target-config-0042c8f8.md",
        "pending",
    ),
    "0x0042C936": (
        "pending",
        "story-vm-ani-follower-target-wait-0042c936.md",
        "pending",
    ),
    "0x0042C95B": (
        "pending",
        "story-vm-current-world-session-reload-0042c95b.md",
        "pending",
    ),
    "0x0042C9CE": (
        "pending",
        "story-vm-deferred-world-session-reload-0042c9ce.md",
        "pending",
    ),
    "0x0042CA3A": (
        "pending",
        "story-vm-deferred-world-session-config-0042ca3a.md",
        "pending",
    ),
    "0x0042CA7C": (
        "pending",
        "story-vm-file-copy-delete-0042ca7c.md",
        "pending",
    ),
    "0x0042CBB0": (
        "pending",
        "story-vm-next-dialog-flag18-suppression-0042cbb0.md",
        "pending",
    ),
    "0x0042CBCC": (
        "pending",
        "story-vm-story-transfer-0042cbcc.md",
        "pending",
    ),
    "0x0042CBFF": (
        "pending",
        "story-vm-current-map-conditional-reload-0042cbff.md",
        "pending",
    ),
    "0x0042CC35": (
        "pending",
        "story-vm-item-total-conditional-reload-0042cc35.md",
        "pending",
    ),
    "0x0042CCF7": (
        "pending",
        "story-vm-mode-text-0042ccf7.md",
        "pending",
    ),
    "0x0042CDED": (
        "pending",
        "story-vm-ani-suspend-0042cded.md",
        "pending",
    ),
    "0x0042CE12": (
        "pending",
        "story-vm-ani-resume-0042ce12.md",
        "pending",
    ),
    "0x0042CE32": (
        "pending",
        "story-vm-party-gather-0042ce32.md",
        "pending",
    ),
    "0x0042CF7C": (
        "pending",
        "story-vm-role-collision-bypass-0042cf7c.md",
        "pending",
    ),
    "0x0042CFBD": (
        "pending",
        "story-vm-frame-deformation-0042cfbd.md",
        "pending",
    ),
    "0x0042D041": (
        "pending",
        "story-vm-frame-execution-gate-clear-0042d041.md",
        "pending",
    ),
    "0x0042B070": (
        "pending",
        "story-vm-wide-global-integers-0042b070.md",
        "pending",
    ),
    "0x0042D05C": (
        "pending",
        "story-vm-party-member-field-reload-0042d05c.md",
        "pending",
    ),
    "0x0042D0D8": (
        "pending",
        "story-vm-party-member-field-write-0042d0d8.md",
        "pending",
    ),
    "0x0042D170": (
        "pending",
        "story-vm-camera-top-wait-0042d170.md",
        "pending",
    ),
    "0x0042D1AA": (
        "pending",
        "story-vm-music-stream-transition-wait-0042d1aa.md",
        "pending",
    ),
    "0x0042D1D5": (
        "pending",
        "story-vm-video-wait-0042d1d5.md",
        "pending",
    ),
    "0x0042D200": (
        "pending",
        "story-vm-common-join-latch-0042d200.md",
        "pending",
    ),
    "0x0042D49F": (
        "pending",
        "story-vm-common-join-latch-clear-0042d49f.md",
        "pending",
    ),
    "0x0042D1EA": (
        "pending",
        "story-vm-common-join-continue-0042d1ea.md",
        "pending",
    ),
    "0x0042D24E": (
        "pending",
        "story-vm-talk-end-cleanup-0042d24e.md",
        "pending",
    ),
}


class InventoryError(RuntimeError):
    """Raised when a locked P1 work-package invariant changes."""


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def write_tsv(
    path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def compact_ranges(values: list[int]) -> str:
    if not values:
        return ""
    ranges: list[str] = []
    start = previous = values[0]
    for value in values[1:]:
        if value == previous + 1:
            previous = value
            continue
        ranges.append(str(start) if start == previous else f"{start}-{previous}")
        start = previous = value
    ranges.append(str(start) if start == previous else f"{start}-{previous}")
    return ",".join(ranges)


def split_values(value: str) -> list[str]:
    return [item for item in value.split("|") if item]


def parse_decimal(value: str, field: str) -> int:
    try:
        return int(value)
    except (TypeError, ValueError) as error:
        raise InventoryError(f"invalid decimal {field}: {value!r}") from error


def modern_cases() -> set[int]:
    source_text = STORY_VM_SOURCE.read_text(encoding="utf-8")
    header_text = STORY_VM_HEADER.read_text(encoding="utf-8")
    opcode_constants = {
        match.group(1): parse_decimal(match.group(2), "opcode constant")
        for match in re.finditer(
            r"^\s*(?:inline constexpr compat::u16 )?"
            r"(OP_\d+(?:_[A-Z0-9_]+)?)\s*=\s*(\d+)U[,;]$",
            header_text,
            re.MULTILINE,
        )
    }
    values: set[int] = set()
    for match in re.finditer(
        r"^\s*case\s+((?:\d+U)|(?:OP_\d+(?:_[A-Z0-9_]+)?))\s*:",
        source_text,
        re.MULTILINE,
    ):
        label = match.group(1)
        if label.endswith("U") and label[:-1].isdigit():
            values.add(parse_decimal(label[:-1], "modern case label"))
            continue
        if label not in opcode_constants:
            raise InventoryError(f"unknown opcode case constant: {label}")
        values.add(opcode_constants[label])
    if len(values) != EXPECTED_MODERN_CASE_COUNT:
        raise InventoryError(
            f"expected {EXPECTED_MODERN_CASE_COUNT} modern cases, got {len(values)}"
        )
    return values


def semantic_navigation() -> dict[int, dict[str, str]]:
    rows: dict[int, dict[str, str]] = {}
    for path in SEMANTIC_INPUTS:
        for row in read_tsv(path):
            opcode = parse_decimal(row["effective_opcode"], "semantic opcode")
            if opcode in rows:
                raise InventoryError(f"duplicate semantic row for opcode {opcode}")
            rows[opcode] = row
    opcode_domain = set(rows)
    if not set(range(125)).issubset(opcode_domain):
        raise InventoryError("manual semantic navigation no longer fully covers 0..124")
    if not opcode_domain.issubset(set(range(194))):
        raise InventoryError("manual semantic navigation escaped opcode 0..193")
    return rows


def symbol_modules() -> dict[str, str]:
    modules: dict[str, str] = {}
    for row in read_tsv(OWNERSHIP_INPUT):
        symbol = row["ida_name_navigation_only"]
        owner = row["follow_up_module"] or row["module_candidate"]
        if symbol and owner:
            modules.setdefault(symbol, owner)
    return modules


def classify_external_symbol(symbol: str) -> str:
    lowered = symbol.lower()
    if "ail_" in lowered or "stream" in lowered:
        return "audio_video"
    if any(token in lowered for token in ("directdraw", "blt", "surface")):
        return "rendering"
    if any(token in lowered for token in ("fopen", "fread", "fseek", "file")):
        return "resource_io"
    if any(token in lowered for token in ("rand", "timegettime")):
        return "input_time_rng"
    return "runtime_platform"


def call_targets(triage_rows: list[dict[str, str]]) -> set[str]:
    targets: set[str] = set()
    for row in triage_rows:
        for call in split_values(row["direct_calls"]):
            if ":" not in call:
                continue
            targets.add(call.split(":", 1)[1])
    return targets


def is_symbolic_call_target(target: str) -> bool:
    registers = {"eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp"}
    return (
        target not in registers and "ptr [" not in target and not target.startswith("[")
    )


def runtime_rows(window_transfer_opcodes: list[int]) -> list[tuple[object, ...]]:
    return [
        (
            "entry_activation",
            "0x00427920..0x0042794E",
            "entry_gate",
            "all",
            "one Talk-state pointer; inactive 0xFFFF context routes to return-one",
            "pending",
            "story-vm-full-validation-p3.md; typed status preserves inactive return-one semantics",
        ),
        (
            "initial_window_load",
            "0x0042794F..0x00427B3F",
            "window_load",
            "all",
            "load TalkN.dat entry into the 0x8000-byte window before fetch",
            "pending",
            "story-vm-full-validation-p3.md; checked resource owner preserves load and failure ordering",
        ),
        (
            "fetch_decode",
            "0x00427B40..0x00427B88",
            "fetch_decode",
            "14-bit domain",
            "raw u16 & 0x3FFF; preserve modifier bits in the raw word",
            "pending",
            "story-vm-full-validation-p3.md; raw aliases and all dispatch boundaries verified",
        ),
        (
            "main_dispatch",
            "0x00427B7C..0x00427B88;0x0042D4F4..0x0042D67B",
            "jump_table",
            "1-98",
            "98 LST dwords",
            "pending",
            "story-vm-full-validation-p3.md; 98-entry primary table regenerated from LST",
        ),
        (
            "secondary_dispatch",
            "0x0042ADA4..0x0042ADB0;0x0042D67C..0x0042D7F3",
            "jump_table",
            "100-193",
            "94 LST dwords",
            "pending",
            "story-vm-full-validation-p3.md; 94-entry secondary table regenerated from LST",
        ),
        (
            "default_invalid",
            "0x0042D230..0x0042D24D",
            "default",
            "0,194-1023,1027-16382",
            "MessageBeep/diagnostic; no automatic IP advance",
            "pending",
            "story-vm-default-invalid-0042d230.md; story-vm-common-join-audio-0042b0ae.md; story-vm-common-join-latch-0042d200.md",
        ),
        (
            "numeric_refinement",
            "0x0042B0CD..0x0042B1BF;0x0042D7F4..0x0042D8A8",
            "internal_switch",
            "29-185",
            "6-dword jump table plus 157-byte selector table",
            "pending",
            "story-vm-full-validation-p3.md; table extraction and all ten owning opcodes pending",
        ),
        (
            "flag_refinement",
            "0x0042C567..0x0042C5D0;0x0042D8AC..0x0042D918",
            "internal_switch",
            "102-174",
            "9-dword jump table plus 73-byte selector table",
            "pending",
            "story-vm-full-validation-p3.md; table extraction and all eight owning opcodes pending",
        ),
        (
            "window_transfers",
            "handler-specific",
            "window_transfer",
            compact_ranges(window_transfer_opcodes),
            "31-row control-transfer inventory marks same-file or story-window reload/transfer",
            "pending",
            "story-vm-full-validation-p3.md; 31 transfer opcodes and zero-issue TALK CFG verified",
        ),
        (
            "common_join",
            "0x0042B0AA..0x0042B0CC",
            "continue_or_yield",
            "all handlers",
            "local continuation OR ESI; nonzero loops to 0x00427B40",
            "pending",
            "story-vm-common-join-audio-0042b0ae.md; story-vm-common-join-latch-0042d200.md",
        ),
        (
            "special_1024",
            "0x0042D200..0x0042D218",
            "special",
            "1024",
            "advance 2; local continue flag; same-call fetch",
            "pending",
            "story-vm-common-join-latch-0042d200.md",
        ),
        (
            "special_1025",
            "0x0042D49F..0x0042D4B5",
            "special",
            "1025",
            "advance 2; clear continuation; yield",
            "pending",
            "story-vm-common-join-latch-clear-0042d49f.md",
        ),
        (
            "special_1026",
            "0x0042D1EA..0x0042D1FF",
            "special",
            "1026",
            "advance 2; ESI continue; same-call fetch",
            "pending",
            "story-vm-common-join-continue-0042d1ea.md",
        ),
        (
            "talk_end_16383",
            "0x0042D24E..0x0042D49E",
            "special",
            "16383",
            "Talk cleanup and common join",
            "pending",
            "story-vm-talk-end-cleanup-0042d24e.md",
        ),
        (
            "fatal_return_zero",
            "0x0042D4B6..0x0042D4D6",
            "return",
            "handler-selected",
            "set close bit 0x04 and return zero",
            "pending",
            "story-vm-full-validation-p3.md; typed fatal status preserves close-bit side effect",
        ),
        (
            "yield_return_one",
            "0x0042D4D7..0x0042D4F3",
            "return",
            "ordinary yield/inactive",
            "AIL serve then return one; inactive skips AIL",
            "pending",
            "story-vm-full-validation-p3.md; typed yield/inactive results preserve AIL-service split",
        ),
        (
            "load_failure_return_zero",
            "0x00427A18..0x00427A4B",
            "return",
            "initial load failure",
            "send WM_DESTROY and return zero",
            "pending",
            "story-vm-full-validation-p3.md; checked load failure replaces WM_DESTROY with typed stop",
        ),
    ]


def main() -> None:
    # Regenerate and validate the four primary dispatcher inventories from the
    # full LST before composing any navigation-only P1 fields.
    dispatch.main()

    dispatch_rows = read_tsv(DISPATCH_INPUT)
    length_rows = read_tsv(LENGTH_INPUT)
    triage_rows = read_tsv(TRIAGE_INPUT)
    coverage_rows = read_tsv(COVERAGE_INPUT)
    control_transfer_rows = read_tsv(CONTROL_TRANSFER_INPUT)
    semantics = semantic_navigation()
    cases = modern_cases()
    modules_by_symbol = symbol_modules()

    if len(dispatch_rows) != 198 or len(length_rows) != 198:
        raise InventoryError("dispatch/length inventory must contain 198 rows")
    if len(triage_rows) != 198 or len(coverage_rows) != 198:
        raise InventoryError("triage/coverage inventory must contain 198 rows")

    dispatch_by_opcode = {
        parse_decimal(row["effective_opcode_dec"], "dispatch opcode"): row
        for row in dispatch_rows
    }
    length_by_opcode = {
        parse_decimal(row["effective_opcode"], "length opcode"): row
        for row in length_rows
    }
    triage_by_opcode = {
        parse_decimal(row["effective_opcode"], "triage opcode"): row
        for row in triage_rows
    }
    coverage_by_opcode = {
        parse_decimal(row["effective_opcode"], "coverage opcode"): row
        for row in coverage_rows
    }
    expected = set(EXPECTED_EXPLICIT_OPCODES)
    for name, rows in (
        ("dispatch", dispatch_by_opcode),
        ("length", length_by_opcode),
        ("triage", triage_by_opcode),
        ("coverage", coverage_by_opcode),
    ):
        if set(rows) != expected:
            raise InventoryError(f"{name} explicit opcode domain changed")

    grouped: dict[str, list[int]] = defaultdict(list)
    for opcode, row in dispatch_by_opcode.items():
        grouped[row["entry_target"]].append(opcode)
    if len(grouped) != EXPECTED_HANDLER_COUNT:
        raise InventoryError(f"expected 146 handler entries, got {len(grouped)}")
    shared_count = sum(1 for opcodes in grouped.values() if len(opcodes) > 1)
    if shared_count != EXPECTED_SHARED_HANDLER_COUNT:
        raise InventoryError(f"expected 25 shared entries, got {shared_count}")

    handler_rows: list[tuple[object, ...]] = []
    ordered_groups = sorted(grouped.items(), key=lambda item: min(item[1]))
    for order, (target, unsorted_opcodes) in enumerate(ordered_groups, start=1):
        opcodes = sorted(unsorted_opcodes)
        dispatch_kinds = sorted(
            {dispatch_by_opcode[opcode]["dispatch_kind"] for opcode in opcodes}
        )
        encoding_classes = sorted(
            {length_by_opcode[opcode]["encoding_class"] for opcode in opcodes}
        )
        control_effects = sorted(
            {length_by_opcode[opcode]["control_effect"] for opcode in opcodes}
        )
        semantic_opcodes = [opcode for opcode in opcodes if opcode in semantics]
        present_cases = [opcode for opcode in opcodes if opcode in cases]
        observed_opcodes = [
            opcode
            for opcode in opcodes
            if parse_decimal(
                coverage_by_opcode[opcode]["unique_physical_records"],
                "coverage physical-record count",
            )
            > 0
        ]
        handler_triage = [triage_by_opcode[opcode] for opcode in opcodes]
        targets = call_targets(handler_triage)
        candidate_modules = {"story_scene"}
        for target_name in filter(is_symbolic_call_target, targets):
            candidate_modules.add(
                modules_by_symbol.get(
                    target_name, classify_external_symbol(target_name)
                )
            )
        unresolved_edges = sorted(
            {
                edge
                for row in handler_triage
                for edge in split_values(row["unresolved_edges"])
            }
        )
        internal_refinement = ""
        if target in {"0x0042B070", "0x0042B074"}:
            internal_refinement = "shared_numeric_operation_refinement"
        elif target == "0x0042C567":
            internal_refinement = "shared_flag_selection_refinement"
        case_coverage = "none"
        if present_cases:
            case_coverage = "all" if len(present_cases) == len(opcodes) else "partial"
        closure = CLOSURE_OVERRIDES.get(target)
        closure_status = closure[0] if closure is not None else "pending"
        closure_evidence = closure[1] if closure is not None else ""
        closure_proof = closure[2] if closure is not None else ""
        handler_rows.append(
            (
                order,
                target,
                len(opcodes),
                compact_ranges(opcodes),
                "yes" if len(opcodes) > 1 else "no",
                ",".join(dispatch_kinds),
                internal_refinement,
                ",".join(encoding_classes),
                "|".join(control_effects),
                f"{len(semantic_opcodes)}/{len(opcodes)}",
                compact_ranges(semantic_opcodes),
                case_coverage,
                compact_ranges(present_cases),
                f"{len(observed_opcodes)}/{len(opcodes)}",
                compact_ranges(observed_opcodes),
                ",".join(sorted(candidate_modules)),
                "|".join(sorted(targets)),
                "|".join(unresolved_edges),
                closure_status,
                closure_evidence,
                closure_proof,
                "existing semantics/C++/asset/triage fields are navigation only; independently audit full LST entry and every opcode variant",
            )
        )

    window_transfer_opcodes = sorted(
        {
            parse_decimal(row["effective_opcode"], "control-transfer opcode")
            for row in control_transfer_rows
        }
    )
    if len(control_transfer_rows) != 31 or len(window_transfer_opcodes) != 31:
        raise InventoryError("control-transfer navigation must contain 31 opcodes")
    runtime = runtime_rows(window_transfer_opcodes)
    if len(handler_rows) != EXPECTED_HANDLER_COUNT or len(runtime) != 17:
        raise InventoryError("unexpected P1 work-package row count")
    closed_count = sum(row[18] not in {"pending", "pending_audit"} for row in handler_rows)
    if closed_count != EXPECTED_CLOSED_HANDLER_COUNT:
        raise InventoryError(
            f"expected {EXPECTED_CLOSED_HANDLER_COUNT} closed handler, got {closed_count}"
        )

    write_tsv(
        HANDLER_OUTPUT,
        (
            "audit_order",
            "entry_target",
            "explicit_opcode_count",
            "effective_opcodes",
            "shared_entry",
            "dispatch_kinds",
            "internal_refinement",
            "encoding_classes_navigation",
            "control_effects_navigation",
            "manual_semantics_rows",
            "manual_semantics_opcodes",
            "modern_case_presence",
            "modern_case_opcodes",
            "asset_observed_rows",
            "asset_observed_opcodes",
            "candidate_port_modules_navigation",
            "direct_call_targets_navigation",
            "unresolved_edges_navigation",
            "closure_status",
            "closure_evidence",
            "closure_proof",
            "closure_rule",
        ),
        handler_rows,
    )
    write_tsv(
        RUNTIME_OUTPUT,
        (
            "path_id",
            "address_range",
            "path_kind",
            "covered_values",
            "lst_contract",
            "status",
            "notes",
        ),
        runtime,
    )
    print(
        f"wrote {HANDLER_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(handler_rows)} rows)"
    )
    print(f"wrote {RUNTIME_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(runtime)} rows)")
    print(
        "locked P1 scope: 198 explicit opcodes, "
        f"{len(handler_rows)} handlers, {shared_count} shared entries, "
        f"{len(cases)} modern case labels; closure {closed_count}/146"
    )


if __name__ == "__main__":
    main()
