#!/usr/bin/env python3
"""Inventory every assembly call to the generic action-record updater sub_4321E0."""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASSEMBLY = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
OUTPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "action-subrecord-callsites.tsv"
)

FUNCTION_RE = re.compile(r"^([0-9A-F]{8})\s+(\S+)\s+proc near\b")
CHUNK_RE = re.compile(r"FUNCTION CHUNK AT ([0-9A-F]{8}) SIZE ([0-9A-F]{8}) BYTES")
INSTRUCTION_RE = re.compile(r"^([0-9A-F]{8})\s+([a-z][a-z0-9]*)\s*(.*)$")
REGISTER_RE = re.compile(r"^(e(?:ax|bx|cx|dx|si|di|bp|sp))$", re.IGNORECASE)
CONFIRMED_ROLE_PARENT_FUNCTIONS = {
    0x00402F80,
    0x00405500,
    0x0040F280,
    0x004120B0,
    0x00427300,
}
BATTLE_ACTION_BASE = 0x2A0
BATTLE_ACTION_OFFSETS = {BATTLE_ACTION_BASE + 0x98 * index for index in range(18)}
BATTLE_ACTION_OWNER_START = 0x00471540
BATTLE_ACTION_OWNER_END = 0x00482840
CONFIRMED_ROLE_SHIFTED_CALLS = {0x0040548B}
CONFIRMED_DIALOG_ACTION_CALLS = {0x0040B30B, 0x0040B362}
CONFIRMED_BATTLE_ACTION_CALLS = {0x00479921}
CONFIRMED_STACK_ACTION_CALLS = {0x004502E3, 0x00478511}
CONFIRMED_ACTION_WRAPPER_CALLS = {
    0x00451466,
    0x00451511,
    0x00451562,
    0x00451629,
    0x004516EC,
}
CONFIRMED_ACTION_PARAMETER_CALLS = {
    0x0040EBF6,
    0x0040ECC6,
    0x0043B086,
    0x00449C39,
    0x0047F97D,
    0x004831D8,
    0x00483903,
    0x00483B3A,
    0x00483DE3,
    0x00484239,
}
CONFIRMED_PICTURE_ACTION_LIST_NODE_CALLS = {0x004147FB}
CONFIRMED_PICPAINT_ACTION_LIST_NODE_CALLS = {0x00414BA3, 0x00414CF7}
CONFIRMED_SHIFTED_STATIC_ACTION_POOL_CALLS = {0x00416325}
CONFIRMED_BATTLE_STATUS_ACTION_NODE_CALLS = {0x0047E6A3}
CONFIRMED_BATTLE_EFFECT_ACTION_PAIR_CALLS = {0x0047FC8C, 0x0047FF38}


@dataclass(frozen=True)
class Function:
    address: int
    name: str


@dataclass(frozen=True)
class Instruction:
    address: int
    mnemonic: str
    operands: str

    @property
    def text(self) -> str:
        return f"{self.mnemonic} {self.operands}".rstrip()


def find_register_origin(register: str, history: list[Instruction]) -> Instruction | None:
    destination = re.compile(rf"^{re.escape(register)}\s*,", re.IGNORECASE)
    survives_calls = register.lower() in {"ebx", "esi", "edi", "ebp"}
    for instruction in reversed(history):
        if instruction.mnemonic in {"ret", "retn"} or (
            instruction.mnemonic == "call" and not survives_calls
        ):
            break
        if instruction.mnemonic in {"lea", "mov"} and destination.match(instruction.operands):
            return instruction
    return None


def category(
    call_address: int, argument: str, origin: Instruction | None, owner: Function
) -> str:
    evidence = f"{argument} {origin.text if origin else ''}"
    if call_address in CONFIRMED_ROLE_SHIFTED_CALLS:
        return "confirmed_role_shifted_parent"
    if call_address in CONFIRMED_DIALOG_ACTION_CALLS:
        return "confirmed_dialog_action_pool"
    if call_address in CONFIRMED_BATTLE_ACTION_CALLS:
        return "confirmed_battle_actor_action_array"
    if call_address in CONFIRMED_STACK_ACTION_CALLS:
        return "confirmed_stack_action_record"
    if call_address in CONFIRMED_ACTION_WRAPPER_CALLS:
        return "confirmed_action_wrapper_record"
    if call_address in CONFIRMED_ACTION_PARAMETER_CALLS:
        return "confirmed_action_pointer_parameter"
    if call_address in CONFIRMED_PICTURE_ACTION_LIST_NODE_CALLS:
        return "confirmed_picture_action_list_node"
    if call_address in CONFIRMED_PICPAINT_ACTION_LIST_NODE_CALLS:
        return "confirmed_picpaint_action_list_node"
    if call_address in CONFIRMED_SHIFTED_STATIC_ACTION_POOL_CALLS:
        return "confirmed_shifted_static_action_pool"
    if call_address in CONFIRMED_BATTLE_STATUS_ACTION_NODE_CALLS:
        return "confirmed_battle_status_action_node"
    if call_address in CONFIRMED_BATTLE_EFFECT_ACTION_PAIR_CALLS:
        return "confirmed_battle_effect_action_pair"
    if "4BABE8" in evidence.upper():
        return "confirmed_role_action_base"
    if re.search(r"\+40h\]", evidence, re.IGNORECASE):
        if owner.address in CONFIRMED_ROLE_PARENT_FUNCTIONS:
            return "confirmed_role_parent_plus_40"
        return "parent_plus_40_unclassified"
    parent_offset = re.search(r"\+([0-9A-F]+)h\]", origin.text, re.IGNORECASE) if origin else None
    if (
        parent_offset
        and int(parent_offset.group(1), 16) in BATTLE_ACTION_OFFSETS
        and BATTLE_ACTION_OWNER_START <= owner.address <= BATTLE_ACTION_OWNER_END
    ):
        return "confirmed_battle_actor_action_array"
    if (
        parent_offset
        and int(parent_offset.group(1), 16) == 0x2BC8
        and owner.address == 0x00471FC0
    ):
        return "confirmed_battle_group_a_action_array"
    if origin and re.search(
        r"\b(?:dword|word|byte|unk)_[0-9A-F]+\[[^]]+\]", origin.operands, re.IGNORECASE
    ):
        return "indexed_static_action_pool"
    if origin and origin.mnemonic == "mov" and "offset " in origin.operands.lower():
        return "static_action_object"
    if "offset " in argument.lower() or re.search(r"\b(?:dword|word|byte|unk)_4", argument):
        return "static_action_object"
    if "esp" in evidence.lower() or "ebp" in argument.lower():
        return "stack_or_parent_relative"
    if REGISTER_RE.match(argument) and origin is None:
        return "register_origin_unresolved"
    return "other_pointer_expression"


def main() -> None:
    lines = ASSEMBLY.read_text(encoding="utf-8", errors="replace").replace("\r", "").splitlines()

    chunks: list[tuple[int, int, Function]] = []
    current_function: Function | None = None
    for line in lines:
        function_match = FUNCTION_RE.match(line)
        if function_match:
            current_function = Function(int(function_match.group(1), 16), function_match.group(2))
        chunk_match = CHUNK_RE.search(line)
        if chunk_match and current_function is not None:
            start = int(chunk_match.group(1), 16)
            size = int(chunk_match.group(2), 16)
            chunks.append((start, start + size, current_function))

    rows: list[tuple[object, ...]] = []
    history: list[Instruction] = []
    current_function = None
    for line in lines:
        function_match = FUNCTION_RE.match(line)
        if function_match:
            current_function = Function(int(function_match.group(1), 16), function_match.group(2))
            history = []

        code = line.split(";", 1)[0].rstrip()
        instruction_match = INSTRUCTION_RE.match(code)
        if not instruction_match:
            continue
        address_text, mnemonic, operands = instruction_match.groups()
        address = int(address_text, 16)
        owner = current_function
        for start, end, chunk_owner in chunks:
            if start <= address < end:
                owner = chunk_owner
                break
        if owner is None:
            continue

        instruction = Instruction(address, mnemonic, operands.strip())
        if mnemonic == "call" and operands.strip() == "sub_4321E0":
            push_instruction = next(
                (candidate for candidate in reversed(history[-24:]) if candidate.mnemonic == "push"),
                None,
            )
            if push_instruction is None:
                raise SystemExit(f"argument push not found before {address_text}")
            argument = push_instruction.operands
            register_match = REGISTER_RE.match(argument)
            origin = (
                find_register_origin(register_match.group(1), history[-96:])
                if register_match
                else None
            )
            rows.append(
                (
                    address_text,
                    f"{owner.address:08X}",
                    owner.name,
                    f"{push_instruction.address:08X}",
                    argument,
                    f"{origin.address:08X}" if origin else "",
                    origin.text if origin else "",
                    category(address, argument, origin, owner),
                )
            )

        history.append(instruction)
        if len(history) > 64:
            history.pop(0)

    if len(rows) != 161:
        raise SystemExit(f"unexpected sub_4321E0 call count: {len(rows)}")
    if len({row[1] for row in rows}) != 101:
        raise SystemExit("unexpected unique caller count")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "call_address",
                "owner_function_address",
                "owner_function_name",
                "argument_push_address",
                "argument_expression",
                "register_origin_address",
                "register_origin_instruction",
                "syntactic_origin_class",
            )
        )
        writer.writerows(rows)

    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(f"wrote {len(rows)} calls from 101 functions to {relative_output}")


if __name__ == "__main__":
    main()
