#!/usr/bin/env python3
"""Emit the assembly-audited story VM semantic batch for opcodes 100..124."""

from __future__ import annotations

import csv
import hashlib
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
DISPATCH_INPUT = INVENTORY_ROOT / "story-vm-opcode-dispatch.tsv"
LENGTH_INPUT = INVENTORY_ROOT / "story-vm-opcode-length-rules.tsv"
OUTPUT = INVENTORY_ROOT / "story-vm-opcode-semantics-100-124.tsv"

EXPECTED_ASM_SHA256 = "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def row(
    opcode: int, operation: str, operands: str, state_reads: str,
    state_writes: str, flow: str, quirk: str, helpers: str, evidence: str,
) -> tuple[object, ...]:
    return (
        opcode, operation, operands, state_reads, state_writes, flow, quirk,
        helpers, evidence, "assembly_audited_business_name_neutral",
    )


def curated_rows() -> list[tuple[object, ...]]:
    role_mask_reads = "role lookup; role+0x10 status dword"
    role_mask_flow = "advance 6; ESI=1; continue in same call"
    role_mask_helpers = "sub_40C0D0; sub_40AE20; sub_40AEC0; sub_40D460"

    return [
        row(100, "set_role_talk_script_number",
            "+2 u16 role selector (FFF0=current state+24); +4 u16 Talk script number",
            "role lookup; role+0x1E Talk script field",
            "found role: role+0x1E=value; missing role: route the same field override through sub_40D460",
            "advance 6; ESI=1; continue in same call",
            "no script-number validation; helper-native FFFE also resolves to the controlled role",
            "sub_40C0D0; sub_40D460", "0x0042B3B0-0x0042B436;0x00403814-0x0040386D"),
        row(101, "set_role_status_bit26", "+2 u16 role selector (FFF0=current state+24)",
            role_mask_reads, "found role: role+0x10 |= 0x04000000",
            "advance 4; ESI=1; continue in same call", "missing role is silently consumed",
            "sub_40C0D0", "0x0042B43B-0x0042B479;0x0042D182-0x0042D193"),
        row(102, "set_role_status_bit6_from_boolean",
            "+2 u16 role selector (FFF0 is replaced by dword_4AB378 before lookup); +4 u16 boolean",
            role_mask_reads, "clear role+0x10 bit6, then set it when +4 is nonzero; refresh two role-derived states",
            role_mask_flow,
            "FFF0 uses the controlled role index itself as the subsequent lookup key, not current state+24; lookup failure diagnoses and calls sub_40D460 with the same clear/set mask intent",
            role_mask_helpers, "0x0042C567-0x0042C6D8;0x00429F61-0x00429F76"),
        row(103, "set_role_status_bit5_from_boolean",
            "+2 u16 role selector (FFF0 is replaced by dword_4AB378 before lookup); +4 u16 boolean",
            role_mask_reads, "clear role+0x10 bit5, then set it when +4 is nonzero; refresh two role-derived states",
            role_mask_flow,
            "FFF0 uses the controlled role index itself as the subsequent lookup key, not current state+24; lookup failure diagnoses and calls sub_40D460 with the same clear/set mask intent",
            role_mask_helpers, "0x0042C567-0x0042C6D8;0x00429F61-0x00429F76"),
        row(104, "set_text_layout_pair_and_clear_control_bit28", "+2 s16 first value; +4 s16 second value",
            "dword_4A1360 text control", "clear dword_4A1360 bit28; sign-extend values to dword_4CF730/4CF734",
            "advance 6; ESI=1; continue in same call",
            "the pair is passed unchanged to subsequent text-action creation; no range check",
            "none", "0x0042B47E-0x0042B4B4;0x00427D60-0x00427D86"),
        row(105, "clear_text_control_bit27", "none", "dword_4A1360", "dword_4A1360 &= 0xF7FFFFFF",
            "advance 2; ESI=1; continue in same call", "none", "none",
            "0x0042B4B9-0x0042B4C5;0x00427E7E-0x00427E95"),
        row(106, "wait_primary_stream_byte_strictly_above", "+2 u16 threshold",
            "pointer at unk_4B7BD0+0xA0; when nonnull, byte at pointed object+0x49", "none",
            "nonnull pointer and byte<=threshold: no advance, ESI=0, yield; null pointer or byte>threshold: advance 4, ESI=1, continue",
            "comparison is unsigned and strict; while the pointer remains nonnull, thresholds >=255 cannot complete",
            "none", "0x0042B4CA-0x0042B50A"),
        row(107, "wait_role_action_index_at_least_threshold",
            "+2 u16 role selector (FFF0=current state+24); +4 u16 threshold",
            "role lookup; action+0x40 packed word: low byte item limit, high byte one-based current index", "none",
            "valid threshold and current index<threshold: no advance, ESI=0, yield; current index>=threshold: advance 6, ESI=1; lookup failure or threshold>limit diagnoses and consumes",
            "threshold equal to the low-byte limit is accepted; only threshold greater than the limit takes the diagnostic path",
            "sub_40C0D0; nullsub_1", "0x0042B50F-0x0042B5ED"),
        row(108, "set_next_text_position_with_fallback_16", "+2 u16 X; +4 u16 Y",
            "none", "write X/Y to the low/high words of dword_4A135C; replace X>639 or Y>479 independently with 16",
            "advance 6; ESI=1; continue in same call",
            "this is replacement, not max-bound clamping; u16 encodings of negative values also become 16",
            "none", "0x0042B5F2-0x0042B637;0x00427C2D-0x00427C5F"),
        row(109, "apply_role_map_reconciliation_to_list", "+2 u16 count; then count u16 role selectors",
            "each role lookup; map-object and role state read by sub_42E280",
            "for each found role call sub_42E280, which reconciles its map object/role bit30 state; missing entries are skipped",
            "advance physical pointer by 4+2*count; add only the low 16 bits to state IP; ESI=0; yield",
            "no FFF0 substitution in the handler, but helper-native FFFE remains active; helper returns are ignored; zero count is accepted; large counts desynchronize wrapped 16-bit IP from the full pointer",
            "sub_40C0D0; sub_42E280", "0x0042B63C-0x0042B6A0;0x0042E280-0x0042E422"),
        row(110, "transfer_if_no_secondary_role_has_bit30", "+2 u32 TALK target",
            "role count dword_49E0C4; role+0x10 bit30 for indices 1..count-1",
            "on transfer, load the target through sub_42E430 and replace the current TALK window pointer",
            "no matching role: control transfer to +2 target, ESI=1; any match: advance 6 and continue in same call",
            "role index 0 is never tested; role count <=1 therefore takes the transfer",
            "sub_42E430", "0x0042B6A5-0x0042B707;0x00428310-0x00428313;0x0042CCD5-0x0042CCF2"),
        row(111, "transfer_if_any_secondary_role_has_bit30", "+2 u32 TALK target",
            "role count dword_49E0C4; role+0x10 bit30 for indices 1..count-1",
            "on transfer, load the target through sub_42E430 and replace the current TALK window pointer",
            "any matching role: control transfer to +2 target, ESI=1; no match: advance 6 and continue in same call",
            "role index 0 is never tested; opcode 111 is the exact boolean inversion of opcode 110 after the scan",
            "sub_42E430", "0x0042B6A5-0x0042B707;0x00428310-0x00428313;0x0042CCD5-0x0042CCF2"),
        row(112, "wait_two_overlay_action_lists_empty", "none",
            "dword_4BAB9C framebuffer-area list and dword_4BA6E0 role-head action list", "none",
            "either list nonnull: no advance, ESI=0, yield; both null: advance 2, ESI=0, yield after completion",
            "moving-action list dword_4AD3E8 is not tested; even successful completion does not continue in the same call",
            "none", "0x0042B70C-0x0042B71E;0x0042D1BC-0x0042D1D0"),
        row(113, "play_sound_effect_with_unread_padding", "+2 u16 sound id; +4 u16 unread padding",
            "global sound mix level dword_4AB784", "start the sound through sub_485610",
            "advance 6; ESI=0; yield", "the final payload word is physically consumed but never read; playback return value is ignored",
            "sub_485610", "0x0042B723-0x0042B734;0x0042BEE6-0x0042BEF9;0x00485610-0x00485645"),
        row(114, "stage_scene_music_stream_request", "+2 u16 first stream value; +4 u16 second stream value; +6 u16 flags",
            "current stream state dword_4B7380/4B74F0 and dword_4ACDBC flags",
            "dword_4B7C80=0x80000001; store +2/+4 in dword_4B7C84/4B7C88; synchronize old stream; set bit23, derive bits16/17 from flag bits13..15, and clear the low byte of dword_4ACDBC",
            "advance 8; ESI=1; continue in same call",
            "flag bit15 suppresses both derived bits; otherwise bit14 sets 0x30000 and bit13 sets 0x20000; unrelated upper flag bits survive while all low eight bits are cleared",
            "sub_485880; nullsub_1", "0x0042B739-0x0042B7F7;0x00485880-0x004858C4"),
        row(115, "set_stream_100_volume_level_0_to_11", "+2 u16 level",
            "Miles stream manager entry 100", "clamp values >11 to 11, scale the level in sub_485850, and call AIL_set_stream_volume for stream 100 when present",
            "advance 4; ESI=0; yield",
            "the apparent negative clamp is dead because the operand was zero-extended; missing stream/manager errors are ignored",
            "sub_485850; sub_4866C0; AIL_set_stream_volume", "0x0042B7FC-0x0042B835;0x00485850-0x00485874;0x004866C0-0x0048671F"),
        row(116, "batch_set_role_positions", "+2 u16 count; then count records of {u16 selector,u16 X,u16 Y}; FFF0=current state+24",
            "role lookup; controlled role index dword_4AB378", "for every record call sub_42DAF0(role_index,(X<<4)&0xFFFF,(Y<<4)&0xFFFF,0,-1,-1,-1); set dword_4A9920 bit15 when the resolved role is controlled",
            "advance physical pointer by 4+6*count; add only the low 16 bits to state IP; ESI=1; continue",
            "lookup return is ignored, so a missing role passes index -1; X/Y shifts wrap at 16 bits; no record bounds/capacity check; large counts desynchronize wrapped IP from the full pointer",
            "sub_40C0D0; sub_42DAF0", "0x0042B83A-0x0042B8E1;0x0042DAF0-0x0042E27D"),
        row(117, "set_role_status_bit4_from_boolean",
            "+2 u16 role selector (FFF0 is replaced by dword_4AB378 before lookup); +4 u16 boolean",
            role_mask_reads, "clear role+0x10 bit4, then set it when +4 is nonzero; refresh two role-derived states",
            role_mask_flow,
            "FFF0 uses the controlled role index itself as the subsequent lookup key, not current state+24; lookup failure diagnoses and calls sub_40D460 with the same clear/set mask intent",
            role_mask_helpers, "0x0042C567-0x0042C6D8;0x00429F61-0x00429F76"),
        row(118, "remove_dialog_records_for_role_guid", "+2 u16 role GUID selector (FFF0=current state+24)",
            "dword_4ACF48 dialog list and parallel predecessor chain; each record+0x16 role index mapped to GUID by sub_40C060; dword_4A9920",
            "unlink and free every matching record and its +0x38/+0x44 allocations; clear matching role+0x26; decrement dword_4A9920 low15 with clamp at zero while preserving bit15",
            "advance 4; ESI=1; continue in same call",
            "the record's raw +0x16 value is used as a role-array index when clearing +0x26; FFFD records skip that clear",
            "sub_40C060; sub_4885A0", "0x0042B8E6-0x0042B9BD;0x0040C060-0x0040C0C2"),
        row(119, "wait_dialog_record_bit0_set_or_absent", "+2 u16 role selector (FFF0=current state+24; FFFD selects special records)",
            "role lookup unless FFFD; dword_4ACF48 dialog list; first matching record+0x08 bit0", "none",
            "matching record with bit0 clear: no advance, ESI=0, yield; bit0 set, no matching record, or lookup failure: advance 4, ESI=1, continue",
            "only the first matching record controls the decision; FFFD bypasses role lookup and matches record+0x16==FFFD",
            "sub_40C0D0; nullsub_1", "0x0042B9C2-0x0042BAB3"),
        row(120, "update_role_action_triplet_with_ffff_keep",
            "+2 u16 role selector (FFF0=current state+24); +4 s16 action+0x00; +6 s16 action+0x08; +8 u16 action+0x34; each FFFF means keep",
            "role lookup; embedded action at role+0x40", "found role: apply non-FFFF fields, clear action+0x44, refresh, and set role+0x10 bit12; missing role: route fields/status through sub_40D460",
            "advance 10; ESI=1; continue in same call",
            "+4/+6 are sign-extended but +8 is zero-extended; refresh failure only diagnoses and does not roll back writes",
            "sub_40C0D0; sub_4321E0; sub_40D460; nullsub_1", "0x0042BAB8-0x0042BC27"),
        row(121, "clear_text_control_bit26", "none", "dword_4A1360", "dword_4A1360 &= 0xFBFFFFFF",
            "advance 2; ESI=1; continue in same call", "none", "none",
            "0x0042BC2C-0x0042BC38;0x00427E7E-0x00427E95"),
        row(122, "clear_text_fast_forward_toggle", "none", "none", "dword_4CAEB8=0",
            "advance 2; ESI=1; continue in same call",
            "the same global is toggled by key C and directly changes text progression; this opcode only clears it",
            "none", "0x0042BC3D-0x0042BC47;0x004030A9-0x004030DE;0x0042F129-0x0042F14D"),
        row(123, "update_scene_music_table_entry", "+2 u16 key; +4 u16 value packed with key; +6 u16 third value; +8 u16 diagnostic-only value",
            "relative scene-music table rooted at dword_4C9A10, terminated by key zero; ArgList when +2 is FFF0",
            "on matching resolved key, copy raw dword at +2 to entry+0 and raw word at +6 to entry+4",
            "advance 10; ESI=1; continue in same call",
            "+8 is ignored on success but included in missing-key diagnostics; FFF0 is resolved only for matching, while the copied raw dword still begins with literal FFF0; no insertion occurs",
            "nullsub_1", "0x0042BC4C-0x0042BCF0;0x0040CF40-0x0040D05C"),
        row(124, "clear_text_control_bit25", "none", "dword_4A1360", "dword_4A1360 &= 0xFDFFFFFF",
            "advance 2; ESI=1; continue in same call", "none", "none",
            "0x0042BCF5-0x0042BD01;0x00427E7E-0x00427E95"),
    ]


def load_map(path: Path, key: str) -> dict[int, dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    return {int(item[key]): item for item in rows}


def main() -> None:
    if sha256(ASM_PATH) != EXPECTED_ASM_SHA256:
        raise SystemExit("locked full assembly input changed")
    dispatch = load_map(DISPATCH_INPUT, "effective_opcode_dec")
    lengths = load_map(LENGTH_INPUT, "effective_opcode")
    curated = curated_rows()
    if [int(item[0]) for item in curated] != list(range(100, 125)):
        raise SystemExit("semantic batch must contain exactly opcodes 100..124 in order")

    output_rows = []
    for item in curated:
        opcode = int(item[0])
        dispatch_row = dispatch[opcode]
        length_row = lengths[opcode]
        output_rows.append((
            opcode, dispatch_row["entry_target"], item[1], item[2],
            length_row["sequential_length_rule"], item[3], item[4], item[5],
            item[6], item[7], item[8], item[9],
        ))

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow((
            "effective_opcode", "entry_target", "working_operation", "operand_layout",
            "physical_length_rule", "state_reads", "state_writes", "flow_and_timing",
            "error_or_quirk", "helper_calls", "assembly_evidence", "audit_status",
        ))
        writer.writerows(output_rows)
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(output_rows)} rows)")


if __name__ == "__main__":
    main()
