#!/usr/bin/env python3
"""Emit the assembly-audited story VM semantic batch for opcodes 50..74."""

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
OUTPUT = INVENTORY_ROOT / "story-vm-opcode-semantics-050-074.tsv"

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
    camera_shared_reads = (
        "existing camera remaining X/Y dword_4A99F0/4A98C0; viewport rectangle xmmword_4AB980; "
        "map dimensions dword_4B7930/4B7934"
    )
    camera_shared_writes = (
        "camera remaining X/Y dword_4A99F0/4A98C0 and signed per-frame steps dword_4A9480/4A93D8; "
        "remaining displacement is clamped to viewport/map bounds"
    )
    camera_shared_quirk = (
        "nonzero displacement is divided by the supplied unsigned step to test exact divisibility; step zero causes integer divide error; "
        "non-divisible step is diagnosed and forced to 4; existing nonzero camera motion is diagnosed but overwritten"
    )

    return [
        row(50, "start_relative_camera_move", "+2 s16 X tiles; +4 s16 Y tiles; +6 u16 X step; +8 u16 Y step",
            camera_shared_reads, camera_shared_writes,
            "initialize/clamp movement; advance 10; ESI=1; continue in same call",
            camera_shared_quirk, "nullsub_1 diagnostics", "0x00429066-0x0042935D"),
        row(51, "wait_camera_move_complete", "none",
            "dword_4A99F0/4A98C0 remaining displacement and dword_4A9480/4A93D8 steps", "none",
            "any of four values nonzero: no advance, ESI=0, yield; all zero: advance 2, ESI=1, continue",
            "wait includes both remaining displacement and step state", "none", "0x00429362-0x004293A7"),
        row(52, "start_three_component_float_interpolation",
            "+2/+4/+6 s16 start components; +8/+10/+12 s16 end components; +14 u16 duration",
            "none", "three current float globals, three end float globals, duration dword_4A9934, and three per-update float deltas=(end-start)/duration",
            "advance 16; ESI=1; continue in same call",
            "duration is zero-extended and used as x87 divisor without a zero guard; host floating-point behavior must not be silently normalized",
            "__ftol", "0x004293AC-0x00429498"),
        row(53, "wait_three_component_interpolation", "none", "dword_4A9934 duration/countdown", "none",
            "countdown >0: no advance, ESI=0, yield; countdown <=0: advance 2, ESI=1, continue",
            "completion predicate is signed <=0", "none", "0x0042949D-0x004294BB"),
        row(54, "repeat_action_refresh", "+2 u16 role selector; FFF0=current state+24; +4 s16 repeat count",
            "unchecked role lookup result; role embedded action record", "clear action+0x44 and +0x42; perform initial refresh; for each positive repeat clear action+0x44, refresh, and clear role+0x98 low word",
            "advance 6; ESI=1; continue in same call",
            "lookup result is unchecked and can form index -1 action pointer; negative/zero repeat still performs the initial refresh once",
            "sub_40C0D0; sub_4321E0; nullsub_1", "0x004294C0-0x004295EE;0x00428E38-0x00428E4D"),
        row(55, "set_role_low2_state_1", "+2 u16 role selector; FFF0=current state+24",
            "unchecked role lookup; old role+0x10 low2; role Y and GUID", "role+0x10 low2=1; call object placement helper using old low2",
            "advance 4; ESI=0; yield after operation",
            "lookup result is unchecked and can index before role array", "sub_40C0D0; sub_411530", "0x004295F3-0x00429676;0x0042C7E6-0x0042C7F6"),
        row(56, "set_role_low2_state_0", "+2 u16 role selector; FFF0=current state+24",
            "unchecked role lookup; old role+0x10 low2; role Y and GUID", "role+0x10 low2=0; call object placement helper using old low2",
            "advance 4; ESI=0; yield after operation",
            "lookup result is unchecked and can index before role array", "sub_40C0D0; sub_411530", "0x004295F3-0x00429676;0x0042C7E6-0x0042C7F6"),
        row(57, "set_role_low2_state_2", "+2 u16 role selector; FFF0=current state+24",
            "unchecked role lookup; old role+0x10 low2; role Y and GUID", "role+0x10 low2=2; call object placement helper using old low2",
            "advance 4; ESI=0; yield after operation",
            "lookup result is unchecked and can index before role array", "sub_40C0D0; sub_411530", "0x004295F3-0x00429676;0x0042C7E6-0x0042C7F6"),
        row(58, "enqueue_action_node", "+2 u16 node field0; +4 u16 node field2; +6 u16 action id; +8 u16 action base variant",
            "dword_4B7C70 list head", "allocate/zero 0xA4-byte node; initialize embedded 0x98-byte action at +8; write operands; prepend to dword_4B7C70",
            "advance 10; ESI=0; yield after enqueue",
            "allocation failure is unchecked", "malloc; sub_40DC00", "0x0042B1F1-0x0042B282"),
        row(59, "play_sound_effect", "+2 u16 one-based sound id", "global audio scaling value dword_4AB784",
            "requests Miles sound playback through global audio manager", "advance 4; ESI=0; yield after request",
            "sub_485610/sub_485CE0 return value is not observed; playback API returns zero on all paths",
            "sub_485610; sub_485CE0", "0x0042967B-0x0042968E;0x00485610-0x00485645"),
        row(60, "clear_scene_render_bit0", "none", "dword_4C9A18", "dword_4C9A18 &= 0xFFFFFFFE",
            "advance 2; ESI=0; yield after operation", "none", "none", "0x00429693-0x004296A8;0x0042D1C4-0x0042D1D0"),
        row(61, "clear_framebuffer_and_set_scene_render_bit0", "none", "dword_4C9A18; dword_4CD76C framebuffer pointer",
            "clear 0x25800 dwords (0x96000 bytes) at framebuffer; set dword_4C9A18 bit0",
            "advance 2; ESI=0; yield after operation",
            "framebuffer pointer is used without a null check", "rep stosd", "0x00429693-0x004296D9"),
        row(62, "upsert_map_role_record",
            "+2 u16 role selector (FFF0=current state+24); +4 u16 map id (FFFF=current ArgList); +6 u16 field; +8 u16 X tile (FFFF=controlled role X>>4); +10 u16 Y tile (FFFF=controlled role Y>>4); +12/+14/+16 u16 fields",
            "resolved role if present; 72 object slots; controlled role coordinates; active map id and active role array",
            "existing role path resets matching objects, clears status bits14/15, refreshes and sets status bit28; sub_40D460 upserts request; if target map is active, replace same-GUID role or append a new 0xD8 record and update auxiliary object table",
            "advance 18; ESI=1; continue in same call",
            "role count/capacity and auxiliary-table free slot are not guarded here; helper failure only diagnoses; multiple operands use distinct FFFF inheritance rules",
            "sub_40C0D0; sub_40DD40; sub_40AE20; sub_40D460; malloc; sub_40D560; sub_40F280; sub_411530; sub_411490; free",
            "0x004296DE-0x00429A16"),
        row(63, "set_u16_selection_list", "+2 u16 prefix; +4 u16 items terminated by FF00",
            "viewport origin xmmword_4AB980", "fill 64-u16 word_4ACE70 with CFCF; copy item list without terminator; set dword_4AD0D0=dword_4A94A4=prefix; snapshot viewport X/Y to dword_4A992C/4A93E4",
            "count<=56: advance 6+2*count, ESI=1, continue; count>56: no advance, ESI=0, yield/retry",
            "terminator scan is unbounded; overflow diagnostic path repeats same instruction; capacity check is 56 although destination has 64 u16 slots",
            "nullsub_1 on count>56", "0x00429A1B-0x00429ACD"),
        row(64, "clear_u16_selection_list", "none", "none", "fill 64-u16 word_4ACE70 with CFCF",
            "advance 2; ESI=1; continue in same call", "prefix and viewport snapshot globals are not cleared", "rep stosd", "0x00429AD2-0x00429AE3;0x0042D1EA-0x0042D1FB"),
        row(65, "queue_role_transfer_or_removal", "+2 u16 role selector",
            "role lookup and role/object state inside sub_40D610", "if role exists, sub_40D610 reconciles object/path state, writes role status/Talk fields and appends role index to transfer bookkeeping",
            "advance 4; ESI=0; yield after operation",
            "does not translate FFF0; missing role is silently consumed", "sub_40C0D0; sub_40D610", "0x00429AE8-0x00429B0F;0x0040D610-0x0040D785"),
        row(66, "apply_role_map_update", "+2/+4/+6/+8/+10/+12/+14 seven u16 fields",
            "role/map transfer bookkeeping inside sub_40D790", "sub_40D790 either updates an active role and transfer arrays or updates a pending map-role record",
            "advance 16; ESI=0; yield after operation",
            "all operands are zero-extended before call; helper return is ignored", "sub_40D790", "0x00429B14-0x00429B5D;0x0040D790-0x0040D9D2"),
        row(67, "self_modifying_frame_clock_wait", "+2 u16 duration/phase: low15 duration, bit15 phase marker",
            "accepted-frame clock dword_4AAECC; dword_4CF6B0 duration; dword_4CF6B4 start time",
            "phase 1 stores duration/start and sets script operand bit15; completion clears operand bit15",
            "bit15 clear: no advance, yield; bit15 set and unsigned elapsed<=duration: no advance, yield; elapsed>duration: advance 4, ESI=1, continue",
            "strict completion is elapsed>duration; elapsed uses u32 wrap subtraction; script operand is modified in place",
            "none", "0x00429B62-0x00429BB0;0x0042D182-0x0042D193"),
        row(68, "clear_role_status_bit_0400", "+2 u16 role selector; FFF0=current state+24",
            "role lookup; role+0x10", "found role: role+0x10 &= 0xFFFFFBFF; missing role: fallback sub_40D460 request",
            "advance 4; ESI=0; yield after operation", "fallback mask arguments differ from found direct write", "sub_40C0D0; sub_40D460", "0x00429BB5-0x00429C32;0x0042C7E6-0x0042C7F6"),
        row(69, "set_role_status_bit_0400", "+2 u16 role selector; FFF0=current state+24",
            "role lookup; role+0x10", "found role: role+0x10 |= 0x00000400; missing role: fallback sub_40D460 request",
            "advance 4; ESI=0; yield after operation", "none", "sub_40C0D0; sub_40D460", "0x00429C37-0x00429CB7;0x0042C7E6-0x0042C7F6"),
        row(70, "start_absolute_camera_move", "+2 s16 target X tile; +4 s16 target Y tile; +6 u16 X step; +8 u16 Y step",
            camera_shared_reads, camera_shared_writes,
            "convert target to displacement from viewport top-left, initialize/clamp; advance 10; ESI=1; continue",
            camera_shared_quirk, "nullsub_1 diagnostics", "0x00429066-0x0042935D"),
        row(71, "set_role_external_action_pointer", "+2 u16 role selector; +4 u16 static action slot",
            "role lookup", "found role: role+0x3C = 0x004B9F68 + slot*0x98",
            "advance 6; ESI=0; yield after operation",
            "does not translate FFF0; slot has no range check; missing role is silently consumed", "sub_40C0D0", "0x00429CBC-0x00429D0A;0x0042BEED-0x0042BEF9"),
        row(72, "clear_role_external_action_pointer", "+2 u16 role selector", "role lookup", "found role: role+0x3C=0",
            "advance 4; ESI=0; yield after operation", "does not translate FFF0; missing role is silently consumed", "sub_40C0D0", "0x00429D0F-0x00429D3E;0x0042C7E6-0x0042C7F6"),
        row(73, "start_camera_move_to_role", "+2 u16 role selector; +4 u16 X step; +6 u16 Y step",
            camera_shared_reads + "; unchecked role lookup and role X/Y", camera_shared_writes,
            "derive target viewport through sub_40D160, initialize/clamp movement; advance 8; ESI=1; continue",
            camera_shared_quirk + "; role lookup return is unchecked and can index before role array; no FFF0 substitution",
            "sub_40C0D0; sub_40D160; nullsub_1 diagnostics", "0x00429066-0x0042935D"),
        row(74, "cancel_three_component_interpolation", "none", "none",
            "dword_4A9A00=dword_4A94B8=dword_4C97F0=0; dword_4A9934=0",
            "advance 2; ESI=1; continue in same call", "current and target component values are not overwritten", "none", "0x00429D43-0x00429D6B;0x00427E84-0x00427E95"),
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
    if [int(item[0]) for item in curated] != list(range(50, 75)):
        raise SystemExit("semantic batch must contain exactly opcodes 50..74 in order")

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
