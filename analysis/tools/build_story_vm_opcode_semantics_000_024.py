#!/usr/bin/env python3
"""Emit the assembly-audited story VM semantic batch for opcodes 0..24."""

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
OUTPUT = INVENTORY_ROOT / "story-vm-opcode-semantics-000-024.tsv"

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
    rows: list[tuple[object, ...]] = []
    rows.append(row(
        0, "default_invalid_opcode", "none", "effective opcode; dword_4CF6D8 diagnostic context",
        "none", "no IP advance; ESI=0; AIL serve then yield/return 1",
        "repeats the same invalid instruction on later frames; not a NOP",
        "MessageBeep; nullsub_1 diagnostic", "0x0042D230-0x0042D249",
    ))

    shared_operands = (
        "mixed byte payload at +2; first u16 is context/role selector "
        "(FFF0=current state+24, FFFD=special); byte scan ends at 25 51 and consumes both bytes"
    )
    shared_reads = (
        "state+24/+26; role lookup and role fields; dword_4A135C/4A1360/4A1364; "
        "dword_4CF730/4CF734/4CF738/4CF73C; word_4CF6B8"
    )
    shared_helpers = "sub_40C0D0; malloc(0x4C); sub_40AFF0; sub_40BB20; AIL_serve"
    shared_evidence = "0x00427B8F-0x00427E6D"
    variants = {
        1: ("text_action_mode0_odd_variant", "record mode=0; record flags include 0x10; selected role action status=1; dword_4A9920 increments"),
        2: ("text_action_mode0_even_variant", "record mode=0"),
        3: ("text_action_mode1_odd_variant", "record mode=1; record flags include 0x10; selected role action status=1; dword_4A9920 increments"),
        4: ("text_action_mode1_even_variant", "record mode=1"),
        5: ("text_action_mode1_flag40_odd_variant", "record mode=1; initial record flags=0x40 then include 0x10; selected role action status=1; dword_4A9920 increments"),
        6: ("text_action_mode1_flag40_even_variant", "record mode=1; initial record flags=0x40"),
    }
    for opcode in range(1, 7):
        operation, variant_write = variants[opcode]
        rows.append(row(
            opcode, operation, shared_operands, shared_reads,
            f"allocated 0x4C action/dialog record; {variant_write}; shared text globals reset to defaults",
            "IP becomes byte immediately after 25 51; ESI=0; yield after queuing record",
            "no bounds check for 25 51; odd byte count is accepted; FFF0 overwrites first payload u16 with state+24",
            shared_helpers, shared_evidence,
        ))

    rows.extend((
        row(7, "clear_text_control_bit31", "none", "dword_4A1360", "dword_4A1360 &= 0x7FFFFFFF",
            "advance 2; ESI=1; continue in same call", "none", "none", "0x00427E72-0x00427E95"),
        row(8, "set_next_text_aux_value", "+2 u16 value", "none", "dword_4CF738=1; dword_4A1364=value",
            "advance 4; ESI=1; continue in same call", "value is zero-extended", "none", "0x00427E9A-0x00427EBD"),
        row(9, "clear_text_control_bit30", "none", "dword_4A1360", "dword_4A1360 &= 0xBFFFFFFF",
            "advance 2; ESI=1; continue in same call", "none", "none", "0x00427EC2-0x00427ECE;0x00427E7E-0x00427E95"),
        row(10, "change_role_field_08", "+2 u16 role selector; +4 u16 value",
            "state+24 for FFF0; role lookup; role action fields",
            "found role: role+0x44=0 and role+0x08=value; missing role: fallback sub_40D460 path",
            "advance 6; ESI=1; continue in same call",
            "helper/diagnostic paths differ for existing and missing role; raw operation label ChangQQ",
            "sub_40C0D0; sub_42E740; sub_4321E0; sub_40D460", "0x00427ED0-0x00427FB0;0x004280F7-0x0042811A"),
        row(11, "change_role_field_34_and_flag1000", "+2 u16 role selector; +4 u16 value",
            "state+24 for FFF0; role lookup; role action fields",
            "found role: role+0x34=value, role+0x44=0, role+0x10 |= 0x00001000; missing role: fallback sub_40D460 path",
            "advance 6; ESI=1; continue in same call",
            "raw operation label ChangDir; OR on CH sets dword bit 0x1000, not 0x00100000",
            "sub_40C0D0; sub_42E740; sub_4321E0; sub_40D460", "0x00427FEB-0x0042811A"),
        row(12, "set_role_position", "+2 u16 role selector; +4 u16 component A; +6 u16 component B",
            "state+24 for FFF0; controlled role index; role current/action fields",
            "if target is current context: role+0x4C=-1, role+0x78=-1, role+0x10 clears 0x00080000; sub_42DAF0 receives both components shifted left 4; dword_4A9920 |= 0x8000 for controlled role",
            "advance 8; ESI=1; continue in same call",
            "missing role emits setpos diagnostic but still consumes instruction",
            "sub_40C0D0; sub_42DAF0", "0x0042811F-0x00428225"),
        row(13, "request_role_step_update", "+2 u16 role selector",
            "state+24 for FFF0; ArgList diagnostic value; role+0x10 bit 0x02000000",
            "if role exists and bit 0x02000000 is clear, call sub_42E280(role index)",
            "advance 4; ESI=0; yield after operation",
            "missing role and already-flagged role still consume instruction",
            "sub_40C0D0; sub_42E280", "0x0042822A-0x00428297;0x0042C7E6-0x0042C7F6"),
        row(14, "wait_role_action_status_zero", "+2 u16 selector; FFF0=current state+24; FFFD=state-local status",
            "state+24/+26 or resolved role+0x26 action status", "none",
            "status nonzero: no advance and yield; status zero: advance 4 then yield",
            "wait is frame retry, not sleep and not same-call busy loop",
            "sub_40C0D0 for ordinary selector", "0x0042829C-0x0042830B"),
        row(15, "reload_same_talk_window", "+2 u32 target relative to TALK payload base",
            "current TALK file object", "state+0x14=target; state+0x20=0; 0x8000-byte window reloaded",
            "no sequential advance; ESI=1; continue at target in same call",
            "target has no range validation", "sub_42E430", "0x00428310-0x00428313;0x0042CCD5-0x0042CCF2;0x0042E430-0x0042E47B"),
        row(16, "branch_if_matching_object_role_flag_clear", "+2 u16 role selector; +4 u32 target",
            "resolved role index; 72 object slots of stride 0x21C; role+0x10 bit 0x40000000",
            "transfer path reloads same TALK window", "matching type-2 object with role flag clear: transfer; otherwise advance 8; both continue same call",
            "object scan is fixed to 72 slots", "sub_40C0D0; sub_42E430", "0x00428318-0x004283A7"),
        row(17, "branch_if_matching_object_role_flag_set", "+2 u16 role selector; +4 u32 target",
            "resolved role index; 72 object slots of stride 0x21C; role+0x10 bit 0x40000000",
            "transfer path reloads same TALK window", "matching type-2 object with role flag set: transfer; otherwise advance 8; both continue same call",
            "object scan is fixed to 72 slots", "sub_40C0D0; sub_42E430", "0x004283AC-0x00428455"),
        row(18, "poll_role_release_helper", "+2 u16 role selector",
            "resolved role action state", "always clears role+0x10 bit31 and role+0x84 u16; helper performs additional effects",
            "sub_42D920 nonzero: advance 4; zero: keep same IP; ESI=1 in both cases, so zero retries in the same interpreter call",
            "a helper that remains zero can form a same-call busy loop", "sub_40C0D0; sub_42D920", "0x0042845A-0x004284BD"),
        row(19, "release_all_roles_with_bit31", "none",
            "role count dword_49E0C4; role records 1..count-1; role+0x10 bit31",
            "for every set bit: call sub_42D920, clear role+0x10 bit31 and role+0x84 u16",
            "advance 2; ESI=1; continue in same call", "role index zero is skipped",
            "sub_42D920", "0x004284C2-0x0042852E;0x00427E84-0x00427E95"),
        row(20, "schedule_role_records_then_wait_ready",
            "+2 u16 count/flags (low14=count, bit14=phase marker, bit15 forwarded flag); +4 count records of {u16 role,u16 componentA,u16 componentB}",
            "role lookup/current components; role+0x10 bit 0x02000000; sub_42E280 readiness",
            "phase 1 schedules each role through sub_42DAF0 and sets operand bit14 in the script buffer; ready phase clears operand high bits",
            "bit14 clear: schedule, do not advance, ESI=0 yield; bit14 set but not all ready: no advance, yield; all ready: advance 4+6*count, ESI=1 continue",
            "self-modifying operand word; FFFF components reuse current role values; opcode 169 shares handler but uses 12-byte records",
            "sub_40C0D0; sub_42DAF0; sub_42E280", "0x0042ADB7-0x0042B06E"),
        row(21, "branch_if_global_bit_set", "+2 u16 bit id; +4 u32 target",
            "global state bit through sub_40DC50", "transfer path reloads same TALK window",
            "bit set: transfer; bit clear: advance 8; both continue same call", "none",
            "sub_40DC50; sub_42E430", "0x00428533-0x0042857A"),
        row(22, "branch_if_global_bit_clear", "+2 u16 bit id; +4 u32 target",
            "global state bit through sub_40DC50", "transfer path reloads same TALK window",
            "bit clear: transfer; bit set: advance 8; both continue same call", "predicate is exact XOR inversion of opcode 21",
            "sub_40DC50; sub_42E430", "0x00428533-0x0042857A"),
        row(23, "branch_if_all_global_bits_set", "+2 u16 bit ids until FF00; then u32 target",
            "byte_4AB384 global bitset", "transfer path reloads same TALK window",
            "all listed bits set: transfer; otherwise advance 8+2*count; both continue same call",
            "empty list transfers; unbounded FF00 scan", "sub_42E430", "0x0042857F-0x004285E8"),
        row(24, "branch_if_any_global_bit_set", "+2 u16 bit ids until FF00; then u32 target",
            "byte_4AB384 global bitset", "transfer path reloads same TALK window",
            "any listed bit set: transfer; otherwise advance 8+2*count; both continue same call",
            "empty list does not transfer; unbounded FF00 scan", "sub_42E430", "0x004285ED-0x00428656"),
    ))
    return rows


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
    if [int(item[0]) for item in curated] != list(range(25)):
        raise SystemExit("semantic batch must contain exactly opcodes 0..24 in order")

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
