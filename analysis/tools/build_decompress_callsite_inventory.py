#!/usr/bin/env python3
"""Build the proven sub_4399E0 direct and wrapper call-site inventory."""

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
    / "decompress-call-sites.tsv"
)

INSTRUCTION_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")


@dataclass(frozen=True)
class CallSite:
    address: int
    target: str
    caller: str
    context: str
    source: str
    compressed_size: str
    destination: str
    actual_output_size: str
    destination_capacity_basis: str
    return_code_policy: str
    output_size_policy: str
    note: str


CALL_SITES = (
    CallSite(
        0x00426836,
        "sub_4399E0",
        "00426820:sub_426820",
        "shared_argument_order_wrapper",
        "wrapper_arg_0",
        "wrapper_arg_8",
        "wrapper_arg_4",
        "wrapper_arg_C",
        "not_present_in_wrapper",
        "propagated_in_EAX",
        "written_through_caller_pointer",
        "pushes an unused fifth literal zero before the four consumed arguments",
    ),
    CallSite(
        0x0043366B,
        "sub_4399E0",
        "00433540:sub_433540",
        "tsw_primary_payload_cold_path",
        "lpBuffer",
        "EBX",
        "ESI",
        "stack_var_28",
        "descriptor_declared_decompressed_size_EDI",
        "ignored_then_overwritten_by_AIL_serve",
        "strict_equal_to_EDI",
        "all current TSW frames also consume input exactly, but this caller does not inspect EAX",
    ),
    CallSite(
        0x004339F5,
        "sub_4399E0",
        "004338F0:sub_4338F0",
        "tsw_primary_payload_cached_path",
        "dword_4FB558",
        "EDI",
        "ESI",
        "stack_var_28",
        "descriptor_declared_decompressed_size_EBX",
        "ignored_then_overwritten_by_actual_size_load",
        "strict_equal_to_EBX",
        "same TSW stream contract as the cold path",
    ),
    CallSite(
        0x00477F79,
        "sub_4399E0",
        "00477F10:sub_477F10",
        "fame_embedded_compressed_record",
        "heap_copy_at_input_plus_10",
        "input_plus_6_u32",
        "allocated_input_plus_2_u32",
        "stack_var_4",
        "input_plus_2_u32",
        "ignored_then_overwritten_by_actual_size_load",
        "strict_equal_to_input_plus_2_u32",
        "length mismatch shows Load fame decompress false; EAX is not checked",
    ),
    CallSite(
        0x00408371,
        "sub_426820",
        "004070A0:sub_4070A0",
        "save_full_load_initial_compressed_state",
        "mapped_save_plus_0x9634",
        "mapped_save_plus_0x962C_u32",
        "byte_4AB384",
        "stack_var_8C",
        "fixed_global_destination_unknown_capacity",
        "ignored",
        "ignored",
        "wrapper EAX and actual output size are both discarded",
    ),
    CallSite(
        0x00408439,
        "sub_426820",
        "004070A0:sub_4070A0",
        "save_full_load_variable_record_array",
        "current_block_plus_8",
        "current_block_plus_0_u32",
        "heap_allocation",
        "stack_var_8C",
        "current_block_plus_4_u32",
        "ignored",
        "ignored",
        "destination allocation uses the adjacent declared size",
    ),
    CallSite(
        0x00408546,
        "sub_426820",
        "004070A0:sub_4070A0",
        "save_full_load_u16_state_block",
        "current_block_plus_8",
        "current_block_plus_0_u32",
        "heap_allocation",
        "stack_var_8C",
        "two_times_current_block_plus_4_u32",
        "ignored",
        "ignored",
        "decompressed bytes are consumed immediately as fixed game state",
    ),
    CallSite(
        0x00408B0E,
        "sub_426820",
        "004070A0:sub_4070A0",
        "save_full_load_two_optional_0x34_records",
        "current_block_plus_8",
        "current_block_plus_0_u32",
        "heap_allocation",
        "stack_var_8C",
        "fixed_0xA8_bytes",
        "ignored",
        "ignored",
        "output is split into two optional 0x34-byte records",
    ),
    CallSite(
        0x0040976A,
        "sub_426820",
        "00409600:sub_409600",
        "save_slot_preview_first_compressed_block",
        "current_block_plus_8",
        "current_block_plus_0_u32",
        "heap_allocation",
        "stack_var_64",
        "two_times_current_block_plus_4_u32",
        "ignored",
        "ignored",
        "save path is save\\%d.sav; output drives preview state",
    ),
    CallSite(
        0x0040980B,
        "sub_426820",
        "00409600:sub_409600",
        "save_slot_preview_second_compressed_block",
        "current_block_plus_8",
        "current_block_plus_0_u32",
        "heap_allocation",
        "stack_var_64",
        "two_times_current_block_plus_4_u32",
        "ignored",
        "ignored",
        "save path is save\\%d.sav; output drives preview state",
    ),
    CallSite(
        0x0040ADC6,
        "sub_426820",
        "0040AD10:sub_40AD10",
        "lmf_indexed_render_or_map_block",
        "block_header_plus_0x10",
        "block_header_plus_0x0C_u32",
        "heap_allocation",
        "stack_var_4",
        "block_header_plus_0x08_u32",
        "ignored",
        "ignored",
        "source block is read from huge.lmf through the shared map file object",
    ),
    CallSite(
        0x00415B57,
        "sub_426820",
        "004158C0:sub_4158C0",
        "ani_frame_payload",
        "memory_or_file_frame_payload",
        "record_size_minus_0x0C_or_actual_file_read",
        "dword_4B8738",
        "stack_var_10",
        "global_frame_buffer_capacity_not_passed",
        "ignored",
        "ignored",
        "two control-flow arms converge on the same wrapper call",
    ),
    CallSite(
        0x00426182,
        "sub_426820",
        "00425BE0:sub_425BE0",
        "lmf_map_cell_or_object_table",
        "temporary_file_payload",
        "actual_read_minus_trailing_4",
        "heap_allocation_stored_at_map_record_plus_0x18",
        "stack_var_4",
        "map_width_times_map_height_times_4",
        "ignored",
        "ignored",
        "the trailing four input bytes are removed before decompression",
    ),
    CallSite(
        0x0042660E,
        "sub_426820",
        "00425BE0:sub_425BE0",
        "lmf_indexed_map_subblock",
        "temporary_compressed_buffer",
        "actual_file_read_var_1C",
        "heap_allocation_at_node_plus_0",
        "stack_Size",
        "declared_Size_from_map_metadata",
        "ignored",
        "passed_by_pointer_to_sub_401B70",
        "no equality comparison; downstream postprocessor receives actual size pointer",
    ),
    CallSite(
        0x00426FDB,
        "sub_426820",
        "00426DF0:sub_426DF0",
        "data_cm_generation_chunk",
        "temporary_compressed_buffer",
        "per_chunk_size_from_0x1A8_header_table",
        "scratch_buffer",
        "stack_var_60",
        "chunk_size_plus_chunk_size_div_1024",
        "ignored",
        "ignored",
        "output writes min(remaining, chunk_size) bytes to Data\\%d.cm",
    ),
)

EXPECTED_DIRECT_4399E0_CALLS = (0x00426836, 0x0043366B, 0x004339F5, 0x00477F79)
EXPECTED_WRAPPER_CALLS = (
    0x00408371,
    0x00408439,
    0x00408546,
    0x00408B0E,
    0x0040976A,
    0x0040980B,
    0x0040ADC6,
    0x00415B57,
    0x00426182,
    0x0042660E,
    0x00426FDB,
)


def main() -> None:
    instructions: dict[int, str] = {}
    direct_calls: list[int] = []
    wrapper_calls: list[int] = []
    with ASSEMBLY.open(encoding="utf-8", errors="replace") as assembly_file:
        for line in assembly_file:
            match = INSTRUCTION_RE.match(line.rstrip("\n"))
            if not match:
                continue
            address = int(match.group(1), 16)
            instruction = match.group(2)
            instructions[address] = instruction
            if re.search(r"\bcall\s+sub_4399E0\b", instruction):
                direct_calls.append(address)
            if re.search(r"\bcall\s+sub_426820\b", instruction):
                wrapper_calls.append(address)

    if tuple(direct_calls) != EXPECTED_DIRECT_4399E0_CALLS:
        raise SystemExit(f"unexpected direct sub_4399E0 calls: {direct_calls}")
    if tuple(wrapper_calls) != EXPECTED_WRAPPER_CALLS:
        raise SystemExit(f"unexpected sub_426820 wrapper calls: {wrapper_calls}")

    for call_site in CALL_SITES:
        instruction = instructions.get(call_site.address, "")
        if not re.search(rf"\bcall\s+{call_site.target}\b", instruction):
            raise SystemExit(
                f"0x{call_site.address:08X} is not a call to {call_site.target}: "
                f"{instruction!r}"
            )

    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "call_instruction_address",
                "immediate_target",
                "containing_function",
                "logical_context",
                "source_expression",
                "compressed_size_expression",
                "destination_expression",
                "actual_output_size_expression",
                "destination_capacity_basis",
                "return_code_policy",
                "output_size_policy",
                "note",
            )
        )
        for call_site in CALL_SITES:
            writer.writerow(
                (
                    f"{call_site.address:08X}",
                    call_site.target,
                    call_site.caller,
                    call_site.context,
                    call_site.source,
                    call_site.compressed_size,
                    call_site.destination,
                    call_site.actual_output_size,
                    call_site.destination_capacity_basis,
                    call_site.return_code_policy,
                    call_site.output_size_policy,
                    call_site.note,
                )
            )

    print(
        f"wrote {len(CALL_SITES)} decompression call contexts: "
        f"{len(direct_calls)} direct sub_4399E0 calls and "
        f"{len(wrapper_calls)} sub_426820 callers"
    )


if __name__ == "__main__":
    main()
