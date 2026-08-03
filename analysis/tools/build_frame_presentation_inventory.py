#!/usr/bin/env python3
"""Build assembly-locked inventories for frame surfaces and presentation.

The complete assembly is the only behavioral authority.  Pseudocode is not
read.  This tool deliberately inventories every DirectDraw Surface::Blt call,
every common Lock/Unlock wrapper pair, the primary-surface commits, and the
current exclusive-display lifecycle.
"""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
LOCK_OUTPUT = INVENTORY_ROOT / "frame-surface-lock-pairs.tsv"
BLT_OUTPUT = INVENTORY_ROOT / "directdraw-blt-callsites.tsv"
PRESENT_OUTPUT = INVENTORY_ROOT / "primary-presentation-paths.tsv"
LIFECYCLE_OUTPUT = INVENTORY_ROOT / "display-lifecycle-stages.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")

EXPECTED_LOCKS = (
    0x00407AB2, 0x00408CF6, 0x0040EDE7, 0x00411A0F, 0x00411B5F,
    0x004126A2, 0x00424C8D, 0x00424DF3, 0x0042EDAC, 0x00436851,
    0x0043A667, 0x0044D93B, 0x0044EAF4, 0x00452870, 0x004528E7,
    0x0045295D, 0x004529D6, 0x00452C0C, 0x00453265, 0x0045AFD7,
    0x0045B0BB,
)
EXPECTED_UNLOCKS = (
    0x00407AC4, 0x00408D08, 0x0040EDF9, 0x00411A21, 0x00411B71,
    0x004126B3, 0x00424C9F, 0x00424E05, 0x0042EDBD, 0x004368C0,
    0x0043A679, 0x0044D94D, 0x0044EB06, 0x00452882, 0x004528F9,
    0x0045296E, 0x004529E8, 0x00452C1E, 0x00453277, 0x0045AFE5,
    0x0045B0CD,
)
EXPECTED_SELECTORS = (
    0x00408263, 0x00408D44, 0x0040AB5A, 0x0040AC20, 0x0040ACDD,
    0x0040EF9B, 0x00411AEB, 0x00411BE8, 0x0041202F, 0x004126FF,
    0x0042502C, 0x0042A63E, 0x0042ACED, 0x0043A83A, 0x0043BFDA,
    0x00446A33, 0x0044F74E, 0x0045282F, 0x00452BD0, 0x00452D18,
    0x00452D5D, 0x00452DC9, 0x004531BA, 0x004534F7, 0x0045E879,
    0x0045E927, 0x004849F1,
)
EXPECTED_BLTS = (
    0x00408276, 0x00408D5E, 0x0040EFAE, 0x00411B05, 0x00411C02,
    0x00412046, 0x00412716, 0x0042A658, 0x0042AD00, 0x00437B39,
    0x00437C32, 0x00437C6C, 0x0043A854, 0x0043BFF1, 0x00446A46,
    0x0044F765, 0x0045283D, 0x0045294B, 0x00452BE7, 0x00452D2F,
    0x00452D6B, 0x00452DBC, 0x00452DE0, 0x0045311C, 0x004531A1,
    0x004531CF, 0x0045350A, 0x00453808, 0x0045393B, 0x0045E898,
    0x0045E946, 0x00484A11,
)

EXPECTED_SNIPPETS = {
    # Exact Win32 window geometry immediately before DirectDraw initialization.
    0x00409F9A: "push 1E0h",
    0x00409F9F: "push 280h",
    0x00409FA4: "push ebp",
    0x00409FA5: "push 0FFFFF8F8h",
    0x00409FAA: "push 86000000h",
    0x00409FB9: "push 40000h",
    0x00409FC4: "call ds:CreateWindowExA",
    # Only active display initialization call: 640x480x16, magic 0x4E22.
    0x00424F9B: "push 10h",
    0x00424F9D: "push 1E0h",
    0x00424FA2: "push 280h",
    0x00424FA7: "push 4E22h",
    0x00424FB2: "call sub_437570",
    0x00424FF2: "push 1E0h",
    0x00424FF7: "push 280h",
    0x00425001: "call sub_437B60",
    0x0042500B: "mov dword_4ACBA0, eax",
    # Cooperative level and display-mode split.
    0x00437598: "cmp [esp+10h+arg_4], 4E21h",
    0x004375A2: "push 8",
    0x004375AD: "push 13h",
    0x00437694: "call dword ptr [ecx+50h]",
    0x004376D5: "push 1",
    0x004376D7: "push 0",
    0x004376EB: "call dword ptr [ecx+54h]",
    # Primary, clipper, and unreachable-in-current-call windowed helper surface.
    0x004377E9: "cmp eax, 8",
    0x00437811: "mov [esp+98h+var_78], 1",
    0x00437819: "mov [esp+98h+var_14], 200h",
    0x0043789F: "call sub_437720",
    0x004379AE: "mov edi, ds:GetSystemMetrics",
    0x004379CE: "call sub_437B60",
    0x004379D3: "mov [esi+8], eax",
    # Game-owned software source surface: DDSD_CAPS|HEIGHT|WIDTH and 0x2840 caps.
    0x00437B9B: "mov [esp+94h+var_78], 7",
    0x00437BA3: "mov [esp+94h+var_14], 2840h",
    # Measured surface geometry becomes the software renderer contract.
    0x0042346A: "mov dword_4A0E78, edx",
    0x00423474: "mov dword_4A0E74, ecx",
    0x0042347A: "mov dword_4A0E7C, eax",
    0x00416D50: "mov dword_4CD310, ecx",
    0x00416D76: "mov [ecx], eax",
    0x00416D78: "add eax, esi",
    0x004A0E74: "dword_4A0E74 dd 500h",
    0x004A0E78: "dword_4A0E78 dd 280h",
    0x004A0E7C: "dword_4A0E7C dd 1E0h",
    # Common lock wrapper and its otherwise-unused pitch shadow.
    0x00416F36: "mov [esp+94h+var_7C], 7Ch",
    0x00416F3E: "call dword ptr [ecx+64h]",
    0x00416F50: "sar eax, 1",
    0x00416F52: "mov dword_4CDE2C, eax",
    0x00416F6C: "call dword ptr [ecx+80h]",
    # Surface selector only exposes +4/+8/+0x0C for 0x2711..0x2713.
    0x00437DF4: "sub eax, 2711h",
    0x00437E06: "mov eax, [ecx+0Ch]",
    0x00437E0C: "mov eax, [ecx+8]",
    0x00437E12: "mov eax, [ecx+4]",
    # Current full-screen recovery and teardown behavior.
    0x00437AE3: "test byte ptr [esi+34h], 1",
    0x00437AF2: "cmp eax, 887601C2h",
    0x00437B0B: "call dword ptr [ecx+58h]",
    0x00437B18: "call dword ptr [edx+2Ch]",
    0x00437B39: "call dword ptr [ecx+14h]",
    0x00437AAF: "test byte ptr [esi+34h], 10h",
    0x00437ABA: "call dword ptr [edx+4Ch]",
    # Battle two-Blt vertical displacement and fixed table.
    0x0045E82D: "movsx edx, byte_4A75EC[edx]",
    0x0045E885: "push 1000000h",
    0x0045E898: "call dword ptr [edx+14h]",
    0x0045E8C6: "mov edx, ecx",
    0x0045E8CB: "rep stosd",
    0x0045E933: "push 1000000h",
    0x0045E946: "call dword ptr [edx+14h]",
    # Bink deliberately uses the same 639x479 RECT for source and destination.
    0x004849E1: "mov [esp+18h+var_8], 27Fh",
    0x004849E9: "mov [esp+18h+var_4], 1DFh",
    0x004849FA: "lea ecx, [esp+18h+var_10]",
    0x004849FE: "push 1000000h",
    0x00484A0B: "lea ecx, [esp+24h+var_10]",
    0x00484A11: "call dword ptr [edx+14h]",
}


@dataclass(frozen=True)
class Instruction:
    address: int
    text: str
    owner: str


@dataclass(frozen=True)
class BltContract:
    role: str
    destination: str
    source: str
    rectangle_contract: str
    flags: str
    result_policy: str


BLT_CONTRACTS = {
    0x00408276: BltContract("transient_save_or_load_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00408D5E: BltContract("steady_high_priority_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x0040EFAE: BltContract("transient_game_ui_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00411B05: BltContract("media_check_status_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x00411C02: BltContract("media_check_result_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x00412046: BltContract("pause_overlay_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "HRESULT left in EAX; caller ignores"),
    0x00412716: BltContract("steady_world_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x0042A658: BltContract("story_video_preclear_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x0042AD00: BltContract("story_vm_conditional_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00437B39: BltContract("windowed_recovery_copy", "primary(+0x04)", "wrapper_surface(+0x08)", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "retry forever; Restore both only on DDERR_SURFACELOST"),
    0x00437C32: BltContract("surface_color_fill", "argument_surface", "NULL", "destination NULL/full", "DDBLT_WAIT|DDBLT_COLORFILL(0x01000400)", "returned to caller"),
    0x00437C6C: BltContract("surface_color_fill_member_color", "argument_surface", "NULL", "destination NULL/full", "DDBLT_WAIT|DDBLT_COLORFILL(0x01000400)", "returned to caller"),
    0x0043A854: BltContract("steady_special_modes_1_3_4_5_6_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x0043BFF1: BltContract("special_transition_clear_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00446A46: BltContract("world_or_story_transition_clear_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x0044F765: BltContract("steady_special_mode_2_shop_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x0045283D: BltContract("battle_primary_snapshot", "battle_surface_0x005244C8", "primary(+0x04)", "both NULL/full surfaces", "0", "ignored"),
    0x0045294B: BltContract("battle_game_surface_snapshot", "battle_surface_0x005244C4", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00452BE7: BltContract("battle_transition_commit_loop_A", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00452D2F: BltContract("battle_transition_commit_loop_B", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00452D6B: BltContract("battle_primary_to_snapshot", "battle_surface_0x005244C8", "primary(+0x04)", "both NULL/full surfaces", "0", "ignored"),
    0x00452DBC: BltContract("battle_game_surface_to_snapshot", "battle_surface_0x005244C4", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00452DE0: BltContract("battle_snapshot_commit", "primary(+0x04)", "battle_surface_0x005244C8", "both NULL/full surfaces", "0", "ignored"),
    0x0045311C: BltContract("battle_random_wipe_seed_copy", "temporary_screen_surface", "function_arg_surface", "both NULL/full surfaces", "0", "ignored"),
    0x004531A1: BltContract("battle_random_wipe_row_copy", "temporary_screen_surface", "function_arg_surface", "one-row dynamic matching rectangles", "0", "ignored"),
    0x004531CF: BltContract("battle_random_wipe_commit", "primary(+0x04)", "temporary_screen_surface", "both NULL/full surfaces", "0", "ignored"),
    0x0045350A: BltContract("steady_battle_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "both NULL/full surfaces", "0", "ignored"),
    0x00453808: BltContract("battle_layer_to_game_surface", "game_surface_0x004ACBA0", "battle_surface_table", "both NULL/full surfaces", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x0045393B: BltContract("battle_layer_to_game_surface_reverse", "game_surface_0x004ACBA0", "battle_surface_table", "both NULL/full surfaces", "0", "ignored"),
    0x0045E898: BltContract("battle_vertical_displacement_part_1", "primary(+0x04)", "game_surface_0x004ACBA0", "dynamic source/destination rectangles from state/table", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x0045E946: BltContract("battle_vertical_displacement_part_2", "primary(+0x04)", "game_surface_0x004ACBA0", "dynamic exposed-band rectangles after black clear", "DDBLT_WAIT(0x01000000)", "ignored"),
    0x00484A11: BltContract("bink_video_commit", "primary(+0x04)", "game_surface_0x004ACBA0", "same RECT {0,0,639,479} for source and destination", "DDBLT_WAIT(0x01000000)", "branches over an explicit DDERR table; success advances video"),
}

PRIMARY_PRESENTS = (
    0x00408276, 0x00408D5E, 0x0040EFAE, 0x00411B05, 0x00411C02,
    0x00412046, 0x00412716, 0x0042A658, 0x0042AD00, 0x0043A854,
    0x0043BFF1, 0x00446A46, 0x0044F765, 0x00452BE7, 0x00452D2F,
    0x00452DE0, 0x004531CF, 0x0045350A, 0x0045E898, 0x0045E946,
    0x00484A11,
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(text: str) -> str:
    return " ".join(text.split())


def load_assembly() -> tuple[list[Instruction], dict[int, str]]:
    instructions: list[Instruction] = []
    by_address: dict[int, str] = {}
    owner = ""
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        owner_match = re.search(r"\b(sub_[0-9A-F]{6})\s+proc near\b", raw)
        if owner_match:
            owner = owner_match.group(1)
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        text = match.group(2).split(";", 1)[0].rstrip()
        if not text or text.endswith(":"):
            continue
        address = int(match.group(1), 16)
        text = normalize(text)
        instructions.append(Instruction(address, text, owner))
        by_address[address] = text
    return instructions, by_address


def verify_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(f"locked input changed: {path}: expected {expected}, got {actual}")


def verify_assembly(instructions: list[Instruction], by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: expected {expected!r}, got {actual!r}"
            )

    def addresses(text: str) -> tuple[int, ...]:
        return tuple(item.address for item in instructions if item.text == text)

    if addresses("call sub_416F10") != EXPECTED_LOCKS:
        raise SystemExit("common surface Lock call set changed")
    if addresses("call sub_416F60") != EXPECTED_UNLOCKS:
        raise SystemExit("common surface Unlock call set changed")
    if addresses("call sub_437DF0") != EXPECTED_SELECTORS:
        raise SystemExit("display surface selector call set changed")
    blts = tuple(
        item.address
        for item in instructions
        if re.fullmatch(r"call dword ptr \[[a-z]+\+14h\]", item.text)
    )
    if blts != EXPECTED_BLTS:
        raise SystemExit(f"DirectDraw Blt call set changed: {blts}")
    init_calls = addresses("call sub_437570")
    if init_calls != (0x00424FB2,):
        raise SystemExit(f"unexpected display-wrapper initialization calls: {init_calls}")

    index_by_address = {item.address: index for index, item in enumerate(instructions)}
    for address in EXPECTED_SELECTORS:
        index = index_by_address[address]
        prior_pushes = [
            item.text for item in instructions[max(0, index - 24):index] if item.text.startswith("push ")
        ]
        if not prior_pushes or prior_pushes[-1] != "push 2711h":
            raise SystemExit(f"selector 0x{address:08X} is not called with 0x2711")


def build_lock_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    index_by_address = {item.address: index for index, item in enumerate(instructions)}
    rows: list[tuple[object, ...]] = []
    for lock_address, unlock_address in zip(EXPECTED_LOCKS, EXPECTED_UNLOCKS):
        lock_index = index_by_address[lock_address]
        unlock_index = index_by_address[unlock_address]
        lock = instructions[lock_index]
        unlock = instructions[unlock_index]
        if lock.owner != unlock.owner or unlock_index <= lock_index:
            raise SystemExit(f"invalid Lock/Unlock pairing at 0x{lock_address:08X}")
        preceding_push = next(
            (item.text[5:] for item in reversed(instructions[max(0, lock_index - 5):lock_index]) if item.text.startswith("push ")),
            "",
        )
        between = instructions[lock_index + 1:unlock_index]
        global_refresh = any(item.text == "mov dword_4CD76C, eax" for item in between)
        rows.append(
            (
                f"0x{lock_address:08X}",
                f"0x{unlock_address:08X}",
                lock.owner,
                preceding_push,
                unlock_index - lock_index - 1,
                "yes" if global_refresh else "no",
                "store returned pointer, then Unlock before later software writes" if global_refresh else "local surface read/copy path",
            )
        )
    if sum(row[5] == "yes" for row in rows) != 19:
        raise SystemExit("expected 19 global framebuffer-pointer refresh pairs")
    return rows


def build_blt_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    owners = {item.address: item.owner for item in instructions}
    if set(BLT_CONTRACTS) != set(EXPECTED_BLTS):
        raise SystemExit("Blt contract table is incomplete")
    return [
        (
            f"0x{address:08X}", owners[address], contract.role,
            contract.destination, contract.source, contract.rectangle_contract,
            contract.flags, contract.result_policy,
        )
        for address, contract in ((address, BLT_CONTRACTS[address]) for address in EXPECTED_BLTS)
    ]


def build_primary_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    owners = {item.address: item.owner for item in instructions}
    rows = []
    for sequence, address in enumerate(PRIMARY_PRESENTS, start=1):
        contract = BLT_CONTRACTS[address]
        rows.append(
            (
                sequence, f"0x{address:08X}", owners[address], contract.role,
                contract.source, contract.rectangle_contract, contract.flags,
                contract.result_policy,
            )
        )
    if len(rows) != 21:
        raise SystemExit("expected 21 gameplay-visible primary commits")
    wait_count = sum(row[6] == "DDBLT_WAIT(0x01000000)" for row in rows)
    zero_count = sum(row[6] == "0" for row in rows)
    if (wait_count, zero_count) != (9, 12):
        raise SystemExit(
            f"primary commit flag distribution changed: wait={wait_count}, zero={zero_count}"
        )
    return rows


def lifecycle_rows() -> list[tuple[str, str, str, str]]:
    return [
        ("win32_window", "0x00409F96-0x00409FCF", "popup 640x480 window: exStyle=0x00040000, style=0x86000000, initial position=(-1800,0)", "window object precedes DirectDraw exclusive initialization"),
        ("current_init_arguments", "0x00424F96-0x00424FB2", "sub_437570(hwnd,0x4E22,640,480,16)", "only call in the complete assembly"),
        ("directdraw_interface", "0x00437610-0x00437671", "DirectDrawCreate then QueryInterface to the wrapper's active interface at +0x00", "failures return zero after message/release"),
        ("cooperative_level", "0x00437598-0x004376C5", "0x4E21 would select 0x08; current 0x4E22 selects flags 0x13", "current executable enters exclusive/full-screen cooperative mode; alternate branch has no caller"),
        ("display_mode", "0x004376D0-0x00437719", "SetDisplayMode(640,480,16,0,1)", "failure releases DirectDraw and aborts initialization"),
        ("primary_and_clipper", "0x004377D0-0x004379AE", "create primary caps=0x200 at +0x04; create HWND clipper at +0x10 and attach it", "current full-screen branch records primary dimensions in both geometry pairs"),
        ("alternate_normal_surface", "0x004379AE-0x00437A3C", "only cooperative flags==0x08: create GetSystemMetrics-sized 0x2840 surface at +0x08", "physically present but unreachable from the sole 0x4E22 initializer"),
        ("game_software_surface", "0x00424FF2-0x00425010,0x00437B60-0x00437BF4", "create 640x480 surface with desc flags=7 and caps=0x2840; store at 0x004ACBA0", "creation result is not checked before later use"),
        ("pixel_geometry", "0x00423400-0x0042347A,0x00416D30-0x00416D81", "Lock once; store actual pitch bytes/width/height; build per-row byte offsets", "static defaults are 0x500/640/480, but queried values become active"),
        ("framebuffer_pointer", "0x00416F10-0x00416F72", "common Lock returns lpSurface, stores pitch/2 shadow, then common Unlock takes that pointer", "19 paths publish 0x004CD76C and Unlock before later software writes; failure is generally unchecked"),
        ("primary_commits", "21 callsites in primary-presentation-paths.tsv", "18 whole-surface NULL-RECT commits; two dynamic battle displacement commits; one Bink 639x479 commit", "nine use DDBLT_WAIT, twelve use flags zero; only Bink interprets normal commit HRESULT"),
        ("deactivate", "0x0040AB50-0x0040ABDD", "maintain audio/battle state, destroy three font renderers, set active=0 and suppression=1, ShowWindow(SW_MINIMIZE=6)", "does not destroy DirectDraw wrapper objects and does not call the DirectInput wrapper"),
        ("reactivate", "0x0040ABDE-0x0040ACBE", "ShowWindow(SW_RESTORE=9), SetWindowPos(0,0,640,480), Restore primary/game surfaces, rebuild row offsets and three font renderers, then battle state", "does not call SetCooperativeLevel, SetDisplayMode, or the DirectInput wrapper again"),
        ("recovery_loop", "0x0040ACBE-0x0040ACFA,0x00437AE0-0x00437B59", "loop until GetGDISurface equals selector 0x2711; current bit0 path waits for vertical blank and calls Flip", "alternate normal path retries Blt forever and restores both surfaces only on DDERR_SURFACELOST"),
        ("teardown", "0x004252DD-0x004252FA,0x00437A50-0x00437ADC", "release game surface; release wrapper surfaces/clipper/primary; if flags&0x10 RestoreDisplayMode; release DirectDraw", "release order is fixed; globals are not uniformly zeroed before process exit"),
    ]


def write_table(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    verify_inputs()
    instructions, by_address = load_assembly()
    verify_assembly(instructions, by_address)
    locks = build_lock_rows(instructions)
    blts = build_blt_rows(instructions)
    primary = build_primary_rows(instructions)
    lifecycle = lifecycle_rows()
    write_table(LOCK_OUTPUT, ("lock_address", "unlock_address", "owner", "surface_operand", "instructions_between", "refreshes_global_0x004CD76C", "contract"), locks)
    write_table(BLT_OUTPUT, ("blt_address", "owner", "role", "destination", "source", "rectangle_contract", "flags", "result_policy"), blts)
    write_table(PRESENT_OUTPUT, ("sequence", "blt_address", "owner", "role", "source", "rectangle_contract", "flags", "result_policy"), primary)
    write_table(LIFECYCLE_OUTPUT, ("stage", "assembly_addresses", "exact_contract", "compatibility_consequence"), lifecycle)
    print(
        f"wrote {len(locks)} Lock/Unlock pairs, {len(blts)} DirectDraw Blt calls, "
        f"{len(primary)} primary commits, and {len(lifecycle)} display lifecycle stages"
    )


if __name__ == "__main__":
    main()
