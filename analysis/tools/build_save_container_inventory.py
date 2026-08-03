#!/usr/bin/env python3
"""Recover and validate the compressed block framing of current SWD3 saves."""

from __future__ import annotations

import csv
import hashlib
from collections import defaultdict
from pathlib import Path
from struct import unpack_from

from compare_lzo1x_compatibility import library_decompress, load_lzo
from verify_tsw_decompression import decompress_sub_4399e0


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FILE_OUTPUT = INVENTORY_ROOT / "save-files.tsv"
BLOCK_OUTPUT = INVENTORY_ROOT / "save-compressed-blocks.tsv"
SUMMARY_OUTPUT = INVENTORY_ROOT / "save-container-summary.tsv"

EXPECTED_FILE_COUNT = 40
PREFIX_SIZE = 0x962C
RAW_AFTER_STATE_A = 0x1C
RAW_AFTER_STATE_B_PARTS = (0x84, 0x180)
RAW_AFTER_FAME = 0x84


def read_u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise ValueError(f"u32 at 0x{offset:X} is outside file")
    return unpack_from("<I", data, offset)[0]


def save_sort_key(path: Path) -> tuple[str, int]:
    return path.parent.name.casefold(), int(path.stem)


def main() -> None:
    save_files = sorted(
        (*WORKSPACE_ROOT.glob("Save/*.sav"), *WORKSPACE_ROOT.glob("Save1/*.sav")),
        key=save_sort_key,
    )
    if len(save_files) != EXPECTED_FILE_COUNT:
        raise SystemExit(f"unexpected save count: {len(save_files)}")

    _, safe_decompress, lzo_version = load_lzo()
    file_rows: list[tuple[object, ...]] = []
    block_rows: list[tuple[object, ...]] = []
    branch_counts: dict[str, int] = defaultdict(int)
    aggregate_output_hash = hashlib.sha256()
    kind_output_hashes: dict[str, object] = defaultdict(hashlib.sha256)
    kind_counts: dict[str, int] = defaultdict(int)
    kind_compressed: dict[str, int] = defaultdict(int)
    kind_output: dict[str, int] = defaultdict(int)
    total_file_bytes = 0

    for save_path in save_files:
        data = save_path.read_bytes()
        relative_path = save_path.relative_to(WORKSPACE_ROOT).as_posix()
        total_file_bytes += len(data)
        if len(data) < PREFIX_SIZE + 8:
            raise SystemExit(f"{relative_path}: shorter than fixed prefix")

        preview_header = PREFIX_SIZE
        preview_compressed = read_u32(data, preview_header)
        preview_declared = read_u32(data, preview_header + 4)
        state_a_header = preview_header + 8 + preview_compressed
        state_a_compressed = read_u32(data, state_a_header)
        state_a_declared = read_u32(data, state_a_header + 4)
        state_a_end = state_a_header + 8 + state_a_compressed

        state_b_header = state_a_end + RAW_AFTER_STATE_A
        state_b_compressed = read_u32(data, state_b_header)
        state_b_declared = read_u32(data, state_b_header + 4)
        state_b_end = state_b_header + 8 + state_b_compressed

        extension_a_offset = state_b_end
        extension_b_offset = extension_a_offset + RAW_AFTER_STATE_B_PARTS[0]
        fame_outer_header = extension_b_offset + RAW_AFTER_STATE_B_PARTS[1]
        fame_record_size = read_u32(data, fame_outer_header)
        fame_record = fame_outer_header + 4
        if fame_record + 10 > len(data):
            raise SystemExit(f"{relative_path}: truncated embedded Fame header")
        fame_marker = data[fame_record : fame_record + 2]
        fame_declared = read_u32(data, fame_record + 2)
        fame_compressed = read_u32(data, fame_record + 6)
        if fame_record_size != fame_compressed + 10:
            raise SystemExit(
                f"{relative_path}: Fame outer size {fame_record_size} != 10+{fame_compressed}"
            )

        post_fame_offset = fame_record + fame_record_size
        tail_header = post_fame_offset + RAW_AFTER_FAME
        tail_compressed = read_u32(data, tail_header)
        tail_declared = read_u32(data, tail_header + 4)
        logical_end = tail_header + 8 + tail_compressed
        if logical_end != len(data):
            raise SystemExit(
                f"{relative_path}: parsed end 0x{logical_end:X} != file size 0x{len(data):X}"
            )

        blocks = (
            (
                1,
                "preview_state",
                preview_header,
                preview_header + 8,
                preview_compressed,
                preview_declared,
                "full:fixed_global_unknown;preview:2*declared",
                preview_declared,
                "00408371_full_load;0040976A_preview_load",
                "full_load_ignores_return_and_actual_size;preview_ignores_both",
            ),
            (
                2,
                "primary_state",
                state_a_header,
                state_a_header + 8,
                state_a_compressed,
                state_a_declared,
                "declared",
                state_a_declared,
                "00408439_full_load",
                "ignores_return_and_actual_size",
            ),
            (
                3,
                "u16_state",
                state_b_header,
                state_b_header + 8,
                state_b_compressed,
                state_b_declared,
                "2*declared",
                state_b_declared,
                "00408546_full_load;0040980B_preview_load",
                "both_ignore_return_and_actual_size",
            ),
            (
                4,
                "embedded_fame",
                fame_record,
                fame_record + 10,
                fame_compressed,
                fame_declared,
                "declared",
                fame_declared,
                "00477F79_via_00408A7F",
                "ignores_return_but_requires_actual_size_equal_declared",
            ),
            (
                5,
                "tail_pair",
                tail_header,
                tail_header + 8,
                tail_compressed,
                tail_declared,
                "0xA8",
                0xA8,
                "00408B0E_full_load",
                "ignores_return_and_actual_size",
            ),
        )

        fame_group_counts: tuple[int, int, int] | None = None

        for (
            ordinal,
            kind,
            header_offset,
            payload_offset,
            compressed_size,
            declared_size,
            original_destination_capacity,
            validation_capacity,
            callsite,
            caller_policy,
        ) in blocks:
            compressed = data[payload_offset : payload_offset + compressed_size]
            if len(compressed) != compressed_size:
                raise SystemExit(f"{relative_path} {kind}: short compressed payload")
            local_branch_counts: dict[str, int] = defaultdict(int)
            assembly_output = decompress_sub_4399e0(
                compressed, declared_size, local_branch_counts
            )
            lzo_result, lzo_output, lzo_size = library_decompress(
                safe_decompress, compressed, validation_capacity
            )
            if (
                lzo_result != 0
                or lzo_size != declared_size
                or lzo_output != assembly_output
            ):
                raise SystemExit(
                    f"{relative_path} {kind}: LZO comparison failed "
                    f"rc={lzo_result}, output={lzo_size}/{declared_size}"
                )
            for branch, count in local_branch_counts.items():
                branch_counts[branch] += count
            if kind == "embedded_fame":
                fame_cursor = 0
                parsed_group_counts: list[int] = []
                for group_index in range(3):
                    if fame_cursor + 10 > len(assembly_output):
                        raise SystemExit(
                            f"{relative_path}: Fame group {group_index + 1} header truncated"
                        )
                    section_size = read_u32(assembly_output, fame_cursor)
                    record_count = unpack_from("<H", assembly_output, fame_cursor + 4)[0]
                    expected_section_size = 10 + record_count * 14
                    expected_stored_size = expected_section_size if record_count else 0
                    if section_size != expected_stored_size:
                        raise SystemExit(
                            f"{relative_path}: Fame group {group_index + 1} size "
                            f"{section_size} != expected stored {expected_stored_size}"
                        )
                    parsed_group_counts.append(record_count)
                    fame_cursor += expected_section_size
                if fame_cursor != len(assembly_output):
                    raise SystemExit(
                        f"{relative_path}: Fame groups end at {fame_cursor}, "
                        f"output is {len(assembly_output)}"
                    )
                fame_group_counts = tuple(parsed_group_counts)  # type: ignore[assignment]
            aggregate_output_hash.update(assembly_output)
            kind_output_hashes[kind].update(assembly_output)
            kind_counts[kind] += 1
            kind_compressed[kind] += compressed_size
            kind_output[kind] += len(assembly_output)
            block_rows.append(
                (
                    relative_path,
                    ordinal,
                    kind,
                    f"0x{header_offset:X}",
                    f"0x{payload_offset:X}",
                    "u32_compressed,u32_declared"
                    if kind != "embedded_fame"
                    else "u8_marker,u8_zero,u32_declared,u32_compressed",
                    compressed_size,
                    declared_size,
                    original_destination_capacity,
                    callsite,
                    caller_policy,
                    0,
                    lzo_result,
                    lzo_size,
                    int(lzo_output == assembly_output),
                    hashlib.sha256(assembly_output).hexdigest(),
                    ";".join(
                        f"{branch}:{count}"
                        for branch, count in sorted(local_branch_counts.items())
                    ),
                )
            )

        if fame_marker != b"\x01\x00":
            raise SystemExit(
                f"{relative_path}: unexpected Fame marker {fame_marker.hex()}"
            )
        if fame_group_counts is None:
            raise AssertionError("embedded Fame stream was not visited")
        timestamp = data[:12].decode("ascii", errors="replace")
        file_rows.append(
            (
                relative_path,
                len(data),
                hashlib.sha256(data).hexdigest(),
                timestamp,
                f"0x{preview_header:X}",
                f"0x{state_a_header:X}",
                f"0x{state_a_end:X}",
                f"0x{state_b_header:X}",
                f"0x{state_b_end:X}",
                f"0x{extension_a_offset:X}",
                f"0x{extension_b_offset:X}",
                f"0x{fame_outer_header:X}",
                fame_record_size,
                fame_marker.hex(),
                fame_declared,
                fame_compressed,
                fame_group_counts[0],
                fame_group_counts[1],
                fame_group_counts[2],
                sum(fame_group_counts),
                f"0x{post_fame_offset:X}",
                f"0x{tail_header:X}",
                tail_compressed,
                tail_declared,
                f"0x{logical_end:X}",
                int(logical_end == len(data)),
            )
        )

    with FILE_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "file_size",
                "file_sha256",
                "timestamp_ascii_12",
                "preview_header_offset_hex",
                "primary_state_header_offset_hex",
                "primary_state_end_offset_hex",
                "u16_state_header_offset_hex",
                "u16_state_end_offset_hex",
                "raw_0x84_offset_hex",
                "raw_0x180_offset_hex",
                "fame_outer_size_offset_hex",
                "fame_outer_record_size",
                "fame_marker_hex",
                "fame_declared_output_size",
                "fame_compressed_size",
                "fame_group_1_record_count",
                "fame_group_2_record_count",
                "fame_group_3_record_count",
                "fame_total_record_count",
                "post_fame_raw_0x84_offset_hex",
                "tail_block_header_offset_hex",
                "tail_compressed_size",
                "tail_declared_output_size",
                "logical_end_offset_hex",
                "ends_exactly_at_eof",
            )
        )
        writer.writerows(file_rows)

    with BLOCK_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "block_ordinal",
                "logical_context",
                "container_header_offset_hex",
                "payload_offset_hex",
                "header_layout",
                "compressed_size",
                "declared_output_size",
                "original_destination_capacity",
                "assembly_callsite",
                "original_caller_policy",
                "assembly_equivalent_return",
                "lzo1x_safe_return",
                "actual_output_size",
                "byte_exact_with_assembly",
                "output_sha256",
                "assembly_branch_hits",
            )
        )
        writer.writerows(block_rows)

    with SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("metric", "value"))
        writer.writerow(("save_files", len(file_rows)))
        writer.writerow(("total_file_bytes", total_file_bytes))
        writer.writerow(("minimum_file_size", min(path.stat().st_size for path in save_files)))
        writer.writerow(("maximum_file_size", max(path.stat().st_size for path in save_files)))
        writer.writerow(("compressed_streams", len(block_rows)))
        writer.writerow(("comparison_library_version", lzo_version))
        writer.writerow(("assembly_exact_input_streams", len(block_rows)))
        writer.writerow(("assembly_exact_declared_output_streams", len(block_rows)))
        writer.writerow(("lzo1x_safe_success_streams", len(block_rows)))
        writer.writerow(("byte_exact_streams", len(block_rows)))
        writer.writerow(
            (
                "compressed_bytes",
                sum(kind_compressed.values()),
            )
        )
        writer.writerow(("decompressed_bytes", sum(kind_output.values())))
        writer.writerow(
            ("concatenated_output_sha256", aggregate_output_hash.hexdigest())
        )
        for kind in sorted(kind_counts):
            writer.writerow((f"{kind}_streams", kind_counts[kind]))
            writer.writerow((f"{kind}_compressed_bytes", kind_compressed[kind]))
            writer.writerow((f"{kind}_decompressed_bytes", kind_output[kind]))
            writer.writerow(
                (f"{kind}_concatenated_output_sha256", kind_output_hashes[kind].hexdigest())
            )
        for branch in sorted(branch_counts):
            writer.writerow((f"branch_{branch}_hits", branch_counts[branch]))

    print(
        f"{len(file_rows)} saves: {len(block_rows)} exact LZO streams, "
        f"{sum(kind_compressed.values())} compressed -> "
        f"{sum(kind_output.values())} output bytes"
    )


if __name__ == "__main__":
    main()
