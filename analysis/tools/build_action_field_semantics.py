#!/usr/bin/env python3
"""Merge updater accesses, initializer writes, and ACT producers into a field matrix."""

from __future__ import annotations

import csv
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
ACCESS_INPUT = INVENTORY_ROOT / "action-subrecord-accesses.tsv"
EFFECT_INPUT = INVENTORY_ROOT / "act-command-field-effects.tsv"
EXTERNAL_INPUT = INVENTORY_ROOT / "action-external-accesses.tsv"
OUTPUT = INVENTORY_ROOT / "action-field-semantics.tsv"


FIELD_METADATA = {
    0x00: ("requested_action_id", "external-producer-pending", "external request; not reset here"),
    0x04: ("cached_action_id", "confirmed-cache-key", "updated from +0x00 when action id changes"),
    0x08: ("requested_base_variant_index", "external-producer-pending", "external request; not reset here"),
    0x0C: ("cached_base_variant_index", "confirmed-cache-key", "updated from +0x08 when base variant changes"),
    0x10: ("yx_parameter_1_draw_x_offset", "producer-consumer-confirmed", "cleared on action/base/delta key change"),
    0x14: ("yx_parameter_2_draw_y_offset", "producer-consumer-confirmed", "cleared on action/base/delta key change"),
    0x18: ("action_mode_flags", "bit-semantics-pending", "masked, not uniformly cleared, on key change"),
    0x1C: ("initialized_minus_one_field_1c", "consumer-pending", "initializer writes 0xFFFFFFFF; updater does not access"),
    0x20: ("pending_base_variant_override_or_minus_one", "role-consumer-confirmed", "role path copies non-minus-one value to +0x08, then restores 0xFFFFFFFF; updater does not access"),
    0x24: ("ea_parameter", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x28: ("eq_parameter", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x2C: ("hw_parameter_1", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x30: ("hw_parameter_2", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x34: ("requested_variant_delta", "confirmed-lookup-key", "external request; not reset here"),
    0x38: ("cached_variant_delta", "confirmed-cache-key", "updated from +0x34 when delta changes"),
    0x3C: ("pending_variant_delta_override_or_minus_one", "role-consumer-confirmed", "role path copies non-minus-one value to +0x34, then restores 0xFFFFFFFF; updater does not access"),
    0x40: ("packed_ap_count_and_current_index", "cross-frame-rule-confirmed", "low byte is AP count; high byte is current 1-based index; not cleared on key change"),
    0x42: ("command_word_cursor", "confirmed", "cleared on action/base/delta key change"),
    0x44: ("wait_countdown", "cross-frame-rule-confirmed", "cleared on key change; decremented when nonzero"),
    0x46: ("ds_wait_value", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x48: ("wait_override_valid_low15", "producer-and-mask-confirmed", "not cleared here; bit 15 enables low-15 override"),
    0x4A: ("render_lookup_key_a_fr_parameter", "producer-consumer-confirmed", "not cleared on lookup-key change"),
    0x4C: ("render_lookup_key_b_ap_parameter", "producer-consumer-confirmed", "not cleared on lookup-key change"),
    0x4E: ("or_parameter", "command-producer-confirmed", "not cleared on lookup-key change"),
    0x50: ("ao_parameter", "command-producer-confirmed", "cleared before each command parse pass"),
    0x54: ("selected_command_stream_pointer", "borrowed-pointer-confirmed", "refreshed after ACT lookup on every parsing call"),
    0x58: ("vw_parameter", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x5A: ("ta_parameter", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x5E: ("xo_parameter", "command-producer-confirmed", "not cleared on lookup-key change"),
    0x60: ("yo_parameter", "command-producer-confirmed", "not cleared on lookup-key change"),
    0x62: ("dl_low_byte_value", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x64: ("rc_parameter_1", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x66: ("gc_parameter_1", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x68: ("bc_parameter_1", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x70: ("rc_parameter_2_or_lc_zero", "command-producer-confirmed", "not cleared on lookup-key change"),
    0x72: ("gc_parameter_2_or_lc_zero", "command-producer-confirmed", "not cleared on lookup-key change"),
    0x74: ("bc_parameter_2_or_lc_zero", "command-producer-confirmed", "not cleared on lookup-key change"),
    0x76: ("xa_parameter", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x78: ("ya_parameter", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x7A: ("lf_parameter_1", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x7C: ("lf_parameter_2", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x7E: ("lf_parameter_3", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x80: ("lf_parameter_4", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x82: ("lf_parameter_5", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x84: ("lf_parameter_6", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x86: ("lf_parameter_7", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x88: ("wt_low_byte_value", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x89: ("reset_only_unknown_byte", "external-producer-pending", "cleared on action/base/delta key change"),
    0x8A: ("sg_low_byte_value", "command-producer-confirmed", "cleared on action/base/delta key change"),
    0x8C: ("command_2o_latched_state", "condition-confirmed-business-name-pending", "cleared on action/base/delta key change"),
    0x90: ("external_stream_control_mode", "battle-producer-confirmed", "battle render paths write 0/1 immediately before update; updater key changes clear it"),
    0x94: ("ms_latched_state", "condition-confirmed-business-name-pending", "cleared on action/base/delta key change"),
}

INITIALIZER_WRITES = {
    0x1C: "0040DC07",
    0x20: "0040DC0A",
    0x3C: "0040DC0D",
    0x42: "0040DC1E",
    0x44: "0040DC1A",
    0x46: "0040DC16",
    0x48: "0040DC12",
    0x90: "0040DC22",
}

EXPECTED_UPDATER_FIELD_COUNT = 49
EXPECTED_KNOWN_FIELD_COUNT = 52


def main() -> None:
    with ACCESS_INPUT.open(encoding="utf-8", newline="") as input_file:
        access_rows = list(csv.DictReader(input_file, delimiter="\t"))
    with EFFECT_INPUT.open(encoding="utf-8", newline="") as input_file:
        effect_rows = list(csv.DictReader(input_file, delimiter="\t"))
    with EXTERNAL_INPUT.open(encoding="utf-8", newline="") as input_file:
        external_rows = list(csv.DictReader(input_file, delimiter="\t"))

    if (
        len(access_rows) != EXPECTED_UPDATER_FIELD_COUNT
        or len(FIELD_METADATA) != EXPECTED_KNOWN_FIELD_COUNT
    ):
        raise SystemExit("unexpected ActionRecord field count")

    effects_by_field: dict[int, list[dict[str, str]]] = {}
    for effect in effect_rows:
        offset = int(effect["action_offset_hex"], 16)
        effects_by_field.setdefault(offset, []).append(effect)

    accesses_by_field = {
        int(access["action_offset_hex"], 16): access for access in access_rows
    }
    widths_by_field = {
        offset: 4 if access is None else int(access["access_width_bytes"])
        for offset in FIELD_METADATA
        for access in (accesses_by_field.get(offset),)
    }
    external_by_field: dict[int, dict[str, set[str]]] = {
        offset: {"read": set(), "write": set(), "read_write": set(), "address": set()}
        for offset in FIELD_METADATA
    }
    for external in external_rows:
        raw_offset = int(external["action_offset_hex"], 16)
        raw_width = int(external["access_width_bytes"] or "1")
        containing_fields = [
            field_offset
            for field_offset, field_width in widths_by_field.items()
            if field_offset <= raw_offset
            and raw_offset + raw_width <= field_offset + field_width
        ]
        if not containing_fields:
            raise SystemExit(
                f"external access at +0x{raw_offset:X}/width {raw_width} does not map to a known field"
            )
        field_offset = max(containing_fields)
        access_kind = external["access_kind"]
        external_by_field[field_offset][access_kind].add(external["instruction_address"])

    output_rows: list[tuple[object, ...]] = []
    for offset in sorted(FIELD_METADATA):
        access = accesses_by_field.get(offset)
        name, status, persistence = FIELD_METADATA[offset]
        effects = effects_by_field.get(offset, [])
        commands = sorted({effect["command_word_hex"] for effect in effects})
        output_rows.append(
            (
                f"{offset:02X}",
                f"{0x40 + offset:04X}",
                "4" if access is None else access["access_width_bytes"],
                name,
                status,
                ",".join(commands),
                len(effects),
                persistence,
                "" if access is None else access["read_instruction_addresses"],
                "" if access is None else access["write_instruction_addresses"],
                "" if access is None else access["read_write_instruction_addresses"],
                INITIALIZER_WRITES.get(offset, ""),
                ",".join(sorted(external_by_field[offset]["read"])),
                ",".join(sorted(external_by_field[offset]["write"])),
                ",".join(sorted(external_by_field[offset]["read_write"])),
                sum(len(addresses) for addresses in external_by_field[offset].values()),
            )
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "action_offset_hex",
                "role_offset_hex",
                "access_width_bytes",
                "provisional_semantic_name",
                "status",
                "act_command_producers",
                "command_effect_count",
                "reset_or_persistence_rule",
                "updater_read_instruction_addresses",
                "updater_write_instruction_addresses",
                "updater_read_write_instruction_addresses",
                "initializer_write_instruction_addresses",
                "proven_external_read_instruction_addresses",
                "proven_external_write_instruction_addresses",
                "proven_external_read_write_instruction_addresses",
                "proven_external_access_count",
            )
        )
        writer.writerows(output_rows)

    produced_fields = len(effects_by_field)
    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(
        f"wrote {len(output_rows)} known ActionRecord fields; updater directly accesses "
        f"{len(access_rows)} and ACT commands directly affect "
        f"{produced_fields} fields in {relative_output}"
    )


if __name__ == "__main__":
    main()
