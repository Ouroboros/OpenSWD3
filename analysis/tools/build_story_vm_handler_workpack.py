#!/usr/bin/env python3
"""Build the P1 story-VM handler and non-handler runtime work packages.

The primary dispatch is regenerated from the locked full LST before this tool
reads it.  Existing length, static-triage, direct-effect, semantic and TALK
coverage inventories are navigation aids only: every handler row starts as
pending_audit regardless of prior prose, C++ case presence, or asset coverage.
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
EXPECTED_MODERN_CASE_COUNT = 135
EXPECTED_CLOSED_HANDLER_COUNT = 95

CLOSURE_OVERRIDES = {
    "0x0042D230": (
        "platform_adapted",
        "story-vm-default-invalid-0042d230.md",
        "assembly_exact;unit_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00427B8F": (
        "platform_adapted",
        "story-vm-dialog-handler-00427b8f.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested",
    ),
    "0x00427E72": (
        "assembly_exact",
        "story-vm-dialog-flag-clear-00427e72.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated",
    ),
    "0x00427E9A": (
        "assembly_exact",
        "story-vm-dialog-lifetime-00427e9a.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated",
    ),
    "0x00427EC2": (
        "assembly_exact",
        "story-vm-dialog-flag-clear-00427ec2.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated",
    ),
    "0x00427ED0": (
        "platform_adapted",
        "story-vm-role-base-variant-00427ed0.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00427FEB": (
        "platform_adapted",
        "story-vm-role-variant-delta-00427feb.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042811F": (
        "platform_adapted",
        "story-vm-role-position-0042811f.md",
        "assembly_exact;unit_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042822A": (
        "platform_adapted",
        "story-vm-role-step-0042822a.md",
        "assembly_exact;unit_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042829C": (
        "platform_adapted",
        "story-vm-role-action-wait-0042829c.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428310": (
        "platform_adapted",
        "story-vm-same-file-jump-00428310.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428318": (
        "platform_adapted",
        "story-vm-role-path-conditional-jump-00428318.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004283AC": (
        "platform_adapted",
        "story-vm-role-path-prepared-jump-004283ac.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042845A": (
        "platform_adapted",
        "story-vm-role-path-release-0042845a.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004284C2": (
        "platform_adapted",
        "story-vm-role-path-release-all-004284c2.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042ADB7": (
        "platform_adapted",
        "story-vm-role-path-schedule-0042adb7.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428533": (
        "platform_adapted",
        "story-vm-global-bit-conditional-jump-00428533.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042857F": (
        "platform_adapted",
        "story-vm-all-global-bits-conditional-jump-0042857f.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004285ED": (
        "platform_adapted",
        "story-vm-any-global-bit-conditional-jump-004285ed.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042865B": (
        "platform_adapted",
        "story-vm-global-bit-set-0042865b.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428679": (
        "platform_adapted",
        "story-vm-global-bit-clear-00428679.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004286C5": (
        "platform_adapted",
        "story-vm-world-session-reload-004286c5.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested",
    ),
    "0x00428713": (
        "platform_adapted",
        "story-vm-role-path-id-change-00428713.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042B074": (
        "platform_adapted",
        "story-vm-global-integers-0042b074.md",
        "assembly_exact;unit_tested;real_asset_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042890F": (
        "assembly_exact",
        "story-vm-script-clock-set-0042890f.md",
        "assembly_exact;unit_tested;asset_absence_verified;sdl_runtime_integrated",
    ),
    "0x00428934": (
        "platform_adapted",
        "story-vm-script-clock-byte-jump-00428934.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042896C": (
        "platform_adapted",
        "story-vm-script-clock-origin-delta-jump-0042896c.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004289BE": (
        "assembly_exact",
        "story-vm-script-clock-snapshot-004289be.md",
        "assembly_exact;unit_tested;asset_absence_verified;sdl_runtime_integrated",
    ),
    "0x004289DE": (
        "platform_adapted",
        "story-vm-role-scene-clear-004289de.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428ADC": (
        "platform_adapted",
        "story-vm-role-flag-8000-00428adc.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428BA0": (
        "platform_adapted",
        "story-vm-role-relocation-completion-00428ba0.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428C9F": (
        "platform_adapted",
        "story-vm-indexed-target-reload-00428c9f.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428D18": (
        "platform_adapted",
        "story-vm-interaction-lock-00428d18.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428DB8": (
        "platform_adapted",
        "story-vm-role-action-wait-override-00428db8.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428E52": (
        "platform_adapted",
        "story-vm-role-action-id-00428e52.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00428F7B": (
        "platform_adapted",
        "story-vm-role-action-override-restore-00428f7b.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429066": (
        "platform_adapted",
        "story-vm-camera-move-00429066.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429362": (
        "platform_adapted",
        "story-vm-camera-wait-00429362.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004293AC": (
        "platform_adapted",
        "story-vm-frame-color-transition-004293ac.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042949D": (
        "platform_adapted",
        "story-vm-frame-color-wait-0042949d.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004294C0": (
        "platform_adapted",
        "story-vm-role-action-repeat-refresh-004294c0.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004295F3": (
        "platform_adapted",
        "story-vm-role-spatial-groups-004295f3.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042B1F1": (
        "platform_adapted",
        "story-vm-picture-action-enqueue-0042b1f1.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042967B": (
        "platform_adapted",
        "story-vm-sound-effect-0042967b.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429693": (
        "platform_adapted",
        "story-vm-scene-render-control-00429693.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x004296DE": (
        "platform_adapted",
        "story-vm-map-role-write-004296de.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429A1B": (
        "platform_adapted",
        "story-vm-selection-scroll-write-00429a1b.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429AD2": (
        "platform_adapted",
        "story-vm-selection-scroll-clear-00429ad2.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429AE8": (
        "platform_adapted",
        "story-vm-role-transfer-00429ae8.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429B14": (
        "platform_adapted",
        "story-vm-role-map-update-00429b14.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429B62": (
        "assembly_exact",
        "story-vm-frame-clock-wait-00429b62.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated",
    ),
    "0x00429BB5": (
        "platform_adapted",
        "story-vm-role-flag-0400-clear-00429bb5.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429C37": (
        "platform_adapted",
        "story-vm-role-flag-0400-set-00429c37.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429CBC": (
        "platform_adapted",
        "story-vm-role-head-sign-00429cbc.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429D0F": (
        "assembly_exact",
        "story-vm-role-head-sign-clear-00429d0f.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated",
    ),
    "0x00429D43": (
        "platform_adapted",
        "story-vm-frame-color-cancel-00429d43.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429D70": (
        "platform_adapted",
        "story-vm-role-suspend-00429d70.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429DA6": (
        "platform_adapted",
        "story-vm-role-turn-suspend-00429da6.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x00429F7B": (
        "platform_adapted",
        "story-vm-role-wait-override-00429f7b.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042A0A6": (
        "platform_adapted",
        "story-vm-moving-action-enqueue-0042a0a6.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042A1EF": (
        "assembly_exact",
        "story-vm-text-control-bit29-clear-0042a1ef.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated",
    ),
    "0x0042A200": (
        "platform_adapted",
        "story-vm-role-head-action-enqueue-0042a200.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042A2C6": (
        "platform_adapted",
        "story-vm-role-head-action-dismiss-0042a2c6.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042A341": (
        "platform_adapted",
        "story-vm-packed-row-effect-upsert-0042a341.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated",
    ),
    "0x0042A54C": (
        "platform_adapted",
        "story-vm-packed-row-effect-control-0042a54c.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;stale_local_typed_stop",
    ),
    "0x0042A611": (
        "platform_adapted",
        "story-vm-video-start-0042a611.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested",
    ),
    "0x0042A673": (
        "platform_adapted",
        "story-vm-role-head-action-key-rewrite-0042a673.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;staged_unsafe_order_tested",
    ),
    "0x0042A6CB": (
        "platform_adapted",
        "story-vm-random-target-reload-0042a6cb.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;rng_sequence_tested;divide_by_zero_typed_stop",
    ),
    "0x0042A727": (
        "platform_adapted",
        "story-vm-battle-request-0042a727.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;staged_unsafe_order_tested;external_dependency_tested",
    ),
    "0x0042B287": (
        "platform_adapted",
        "story-vm-name-record-load-0042b287.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;shared_handler_all_variants_tested;staged_unsafe_order_tested;u32_wrap_tested",
    ),
    "0x0042A756": (
        "platform_adapted",
        "story-vm-reserved-global-bit-set-0042a756.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;u32_wrap_tested;unsafe_owner_boundary_tested",
    ),
    "0x0042A792": (
        "platform_adapted",
        "story-vm-reserved-global-bit-clear-0042a792.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;u32_wrap_tested;unsafe_owner_boundary_tested",
    ),
    "0x0042A7CE": (
        "platform_adapted",
        "story-vm-scene-render-bit1-set-0042a7ce.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;exact_tail_tested;typed_owner_boundary_tested",
    ),
    "0x0042A7EE": (
        "platform_adapted",
        "story-vm-scene-render-bit1-clear-0042a7ee.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;exact_tail_tested;typed_owner_boundary_tested",
    ),
    "0x0042A80E": (
        "platform_adapted",
        "story-vm-custom-ani-start-0042a80e.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested",
    ),
    "0x0042AD3C": (
        "platform_adapted",
        "story-vm-custom-ani-wait-0042ad3c.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested",
    ),
    "0x0042C7EA": (
        "assembly_exact",
        "story-vm-four-byte-noop-0042c7ea.md",
        "assembly_exact;unit_tested;real_asset_tested;exact_tail_tested;unread_payload_tested",
    ),
    "0x0042AD75": (
        "platform_adapted",
        "story-vm-custom-ani-phase-wait-0042ad75.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;signed_comparison_tested;staged_operand_tested",
    ),
    "0x0042B3B0": (
        "platform_adapted",
        "story-vm-role-talk-script-write-0042b3b0.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested",
    ),
    "0x0042B43B": (
        "assembly_exact",
        "story-vm-role-status-bit26-set-0042b43b.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested",
    ),
    "0x0042C567": (
        "platform_adapted",
        "story-vm-role-status-boolean-flags-0042c567.md",
        "assembly_exact;unit_tested;real_asset_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested",
    ),
    "0x0042B47E": (
        "platform_adapted",
        "story-vm-text-layout-pair-0042b47e.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;exact_tail_tested;staged_operand_tested;signed_operand_tested;partial_failure_tested",
    ),
    "0x0042B4B9": (
        "assembly_exact",
        "story-vm-text-control-bit27-clear-0042b4b9.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated;exact_tail_tested;selector_alias_tested",
    ),
    "0x0042B4CA": (
        "platform_adapted",
        "story-vm-picture-action-byte-wait-0042b4ca.md",
        "assembly_exact;unit_tested;real_asset_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested",
    ),
    "0x0042B50F": (
        "assembly_exact",
        "story-vm-role-action-index-wait-0042b50f.md",
        "assembly_exact;unit_tested;real_asset_tested;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested",
    ),
    "0x0042B5F2": (
        "assembly_exact",
        "story-vm-next-dialog-anchor-0042b5f2.md",
        "assembly_exact;unit_tested;asset_absence_verified;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested",
    ),
    "0x0042B63C": (
        "platform_adapted",
        "story-vm-role-step-list-0042b63c.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested;wrapping_tested",
    ),
    "0x0042B6A5": (
        "platform_adapted",
        "story-vm-secondary-role-bit30-reload-0042b6a5.md",
        "assembly_exact;unit_tested;real_asset_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested;shared_handler_all_variants_tested;unread_payload_tested;typed_owner_boundary_tested",
    ),
    "0x0042B70C": (
        "platform_adapted",
        "story-vm-overlay-action-lists-wait-0042b70c.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;selector_alias_tested;typed_owner_boundary_tested",
    ),
    "0x0042B723": (
        "platform_adapted",
        "story-vm-sound-effect-unread-padding-0042b723.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;unread_payload_tested;wrapping_tested",
    ),
    "0x0042B739": (
        "platform_adapted",
        "story-vm-scene-music-stream-request-0042b739.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested",
    ),
    "0x0042B7FC": (
        "platform_adapted",
        "story-vm-music-stream-volume-0042b7fc.md",
        "assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested",
    ),
    "0x0042B83A": (
        "platform_adapted",
        "story-vm-batch-role-position-0042b83a.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;external_dependency_tested;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested;wrapping_tested",
    ),
    "0x0042B8E6": (
        "platform_adapted",
        "story-vm-dialog-role-remove-0042b8e6.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;exact_tail_tested;staged_operand_tested;selector_alias_tested;partial_failure_tested;shared_owner_tested",
    ),
    "0x0042B9C2": (
        "platform_adapted",
        "story-vm-dialog-flag-wait-0042b9c2.md",
        "assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated;exact_tail_tested;staged_operand_tested;selector_alias_tested;shared_handler_all_variants_tested;shared_owner_tested;typed_owner_boundary_tested",
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
            "scope_locked_not_semantically_closed",
            "ABI and inactive gate",
        ),
        (
            "initial_window_load",
            "0x0042794F..0x00427B3F",
            "window_load",
            "all",
            "load TalkN.dat entry into the 0x8000-byte window before fetch",
            "scope_locked_not_semantically_closed",
            "file/open/failure and state side effects require P2 review",
        ),
        (
            "fetch_decode",
            "0x00427B40..0x00427B88",
            "fetch_decode",
            "14-bit domain",
            "raw u16 & 0x3FFF; preserve modifier bits in the raw word",
            "scope_locked_not_semantically_closed",
            "routes main/default/99/secondary",
        ),
        (
            "main_dispatch",
            "0x00427B7C..0x00427B88;0x0042D4F4..0x0042D67B",
            "jump_table",
            "1-98",
            "98 LST dwords",
            "lst_locked",
            "feeds handler work package",
        ),
        (
            "secondary_dispatch",
            "0x0042ADA4..0x0042ADB0;0x0042D67C..0x0042D7F3",
            "jump_table",
            "100-193",
            "94 LST dwords",
            "lst_locked",
            "feeds handler work package",
        ),
        (
            "default_invalid",
            "0x0042D230..0x0042D24D",
            "default",
            "0,194-1023,1027-16382",
            "MessageBeep/diagnostic; no automatic IP advance",
            "platform_adapted",
            "story-vm-default-invalid-0042d230.md; common join for other handlers remains pending",
        ),
        (
            "numeric_refinement",
            "0x0042B0CD..0x0042B1BF;0x0042D7F4..0x0042D8A8",
            "internal_switch",
            "29-185",
            "6-dword jump table plus 157-byte selector table",
            "pending_audit",
            "refines handlers 0x0042B074/0x0042B070",
        ),
        (
            "flag_refinement",
            "0x0042C567..0x0042C5D0;0x0042D8AC..0x0042D918",
            "internal_switch",
            "102-174",
            "9-dword jump table plus 73-byte selector table",
            "pending_audit",
            "refines handler 0x0042C567",
        ),
        (
            "window_transfers",
            "handler-specific",
            "window_transfer",
            compact_ranges(window_transfer_opcodes),
            "31-row control-transfer inventory marks same-file or story-window reload/transfer",
            "pending_audit",
            "navigation set only; each owning handler must re-prove load order and IP reset",
        ),
        (
            "common_join",
            "0x0042B0AA..0x0042B0CC",
            "continue_or_yield",
            "all handlers",
            "local continuation OR ESI; nonzero loops to 0x00427B40",
            "pending_audit",
            "zero routes to audio service/yield",
        ),
        (
            "special_1024",
            "0x0042D200..0x0042D218",
            "special",
            "1024",
            "advance 2; local continue flag; same-call fetch",
            "pending_audit",
            "outside the 198 ordinary 0-193 span but explicit",
        ),
        (
            "special_1025",
            "0x0042D49F..0x0042D4B5",
            "special",
            "1025",
            "advance 2; clear continuation; yield",
            "pending_audit",
            "explicit special value",
        ),
        (
            "special_1026",
            "0x0042D1EA..0x0042D1FF",
            "special",
            "1026",
            "advance 2; ESI continue; same-call fetch",
            "pending_audit",
            "explicit special value",
        ),
        (
            "talk_end_16383",
            "0x0042D24E..0x0042D49E",
            "special",
            "16383",
            "Talk cleanup and common join",
            "pending_audit",
            "explicit special value",
        ),
        (
            "fatal_return_zero",
            "0x0042D4B6..0x0042D4D6",
            "return",
            "handler-selected",
            "set close bit 0x04 and return zero",
            "pending_audit",
            "caller ignores EAX; side effect is authoritative",
        ),
        (
            "yield_return_one",
            "0x0042D4D7..0x0042D4F3",
            "return",
            "ordinary yield/inactive",
            "AIL serve then return one; inactive skips AIL",
            "pending_audit",
            "one return value has multiple meanings",
        ),
        (
            "load_failure_return_zero",
            "0x00427A18..0x00427A4B",
            "return",
            "initial load failure",
            "send WM_DESTROY and return zero",
            "pending_audit",
            "separate zero-return site",
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
        closure_status = closure[0] if closure is not None else "pending_audit"
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
    closed_count = sum(row[18] != "pending_audit" for row in handler_rows)
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
