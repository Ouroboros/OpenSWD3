#!/usr/bin/env python3
"""Inventory ANI frame containers and validate the sub_4158C0 LZO boundary."""

from __future__ import annotations

import csv
import hashlib
import struct
from collections import defaultdict
from pathlib import Path

from compare_lzo1x_compatibility import library_decompress, load_lzo
from verify_tsw_decompression import decompress_sub_4399e0


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FILE_OUTPUT = INVENTORY_ROOT / "ani-files.tsv"
FRAME_OUTPUT = INVENTORY_ROOT / "ani-frames.tsv"
SUMMARY_OUTPUT = INVENTORY_ROOT / "ani-container-summary.tsv"

HEADER_SIZE = 0x24
FRAME_TRAILER_SIZE = 0x0C
INITIAL_PALETTE_SIZE = 0x300
FRAME_BUFFER_CAPACITY = 0x9C400
VISIBLE_FRAME_BYTES = 640 * 480 * 2

EXPECTED_FILES = (
    "Bd2Dh2.Ani",
    "BigArmy.Ani",
    "ChaosWar.Ani",
    "GetSword.Ani",
    "LiliaDie.Ani",
    "MonkDie.Ani",
    "Withdraw.Ani",
    "attack-1.Ani",
    "bd2dh.Ani",
    "combat01.Ani",
    "dh2ch3.Ani",
    "dm2bd.Ani",
    "expv.Ani",
    "fogg.Ani",
    "kungfu.Ani",
    "memory.Ani",
    "monk.Ani",
    "nicole.Ani",
    "ve2dm.Ani",
)
EXPECTED_FRAME_COUNT = 5312


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def parse_spans(output: bytes, expected_count: int, context: str) -> tuple[int, int, int, int]:
    position = 0
    maximum_target_end = 0
    total_indexed_pixels = 0
    for ordinal in range(1, expected_count + 1):
        if position + 8 > len(output):
            raise SystemExit(f"{context}: span {ordinal} header exceeds output")
        record_size, target_offset = struct.unpack_from("<II", output, position)
        if record_size < 8:
            raise SystemExit(f"{context}: span {ordinal} has size {record_size}")
        record_end = position + record_size
        if record_end > len(output):
            raise SystemExit(f"{context}: span {ordinal} exceeds output")
        indexed_pixels = record_size - 8
        target_end = target_offset + indexed_pixels * 2
        if target_end > VISIBLE_FRAME_BYTES:
            raise SystemExit(
                f"{context}: span {ordinal} writes to {target_end}, "
                f"past visible frame {VISIBLE_FRAME_BYTES}"
            )
        maximum_target_end = max(maximum_target_end, target_end)
        total_indexed_pixels += indexed_pixels
        position = record_end
    return (
        maximum_target_end,
        total_indexed_pixels,
        position,
        len(output) - position,
    )


def main() -> None:
    paths = sorted((WORKSPACE_ROOT / "Video").glob("*.Ani"), key=lambda path: path.name)
    if tuple(path.name for path in paths) != EXPECTED_FILES:
        raise SystemExit("unexpected ANI file set")

    _, safe_decompress, lzo_version = load_lzo()
    file_rows: list[tuple[object, ...]] = []
    frame_rows: list[tuple[object, ...]] = []
    branch_counts: dict[str, int] = defaultdict(int)
    total_file_bytes = 0
    total_frames = 0
    total_compressed_bytes = 0
    total_decompressed_bytes = 0
    total_spans = 0
    total_indexed_pixels = 0
    total_unconsumed_command_bytes = 0
    compressed_frame_count = 0
    zero_payload_frame_count = 0
    all_frame_outputs = hashlib.sha256()

    for path in paths:
        data = path.read_bytes()
        if len(data) < HEADER_SIZE + INITIAL_PALETTE_SIZE:
            raise SystemExit(f"{path.name}: file is too small")
        (
            magic,
            frame_count,
            storage_bpp,
            canvas_width,
            canvas_height,
            display_width,
            display_height,
            flags_12,
            palette_event_count,
            header_u16_16,
            initial_declared_total_size,
            initial_span_count,
            initial_record_size,
        ) = struct.unpack_from("<4sI8H3I", data, 0)
        if magic != b"ANI\0":
            raise SystemExit(f"{path.name}: invalid ANI magic")
        if (storage_bpp, canvas_width, canvas_height, display_width, display_height) != (
            8,
            640,
            480,
            640,
            480,
        ):
            raise SystemExit(f"{path.name}: unexpected fixed display fields")
        if flags_12 != 0x0101 or palette_event_count != 0:
            raise SystemExit(f"{path.name}: unexpected flags or palette-event count")

        record_offset = HEADER_SIZE
        declared_total_size = initial_declared_total_size
        span_count = initial_span_count
        record_size = initial_record_size
        file_compressed_bytes = 0
        file_decompressed_bytes = 0
        file_spans = 0
        file_indexed_pixels = 0
        file_maximum_target_end = 0
        file_unconsumed_command_bytes = 0
        file_outputs = hashlib.sha256()

        for frame_index in range(1, frame_count + 1):
            context = f"{path.name} frame {frame_index}"
            if record_size < FRAME_TRAILER_SIZE:
                raise SystemExit(f"{context}: record is shorter than its trailer")
            record_end = record_offset + record_size
            if record_end > len(data):
                raise SystemExit(f"{context}: record ends past EOF")
            record = data[record_offset:record_end]
            compressed = record[:-FRAME_TRAILER_SIZE]
            expected_output_size = declared_total_size - FRAME_TRAILER_SIZE
            if expected_output_size < 0:
                raise SystemExit(f"{context}: declared output total is below 12")

            if compressed:
                assembly_output = decompress_sub_4399e0(
                    compressed,
                    FRAME_BUFFER_CAPACITY,
                    branch_counts,
                    require_exact_output=False,
                )
                if len(assembly_output) != expected_output_size:
                    raise SystemExit(
                        f"{context}: output {len(assembly_output)} != "
                        f"declared {expected_output_size}"
                    )
                exact_result, exact_output, exact_size = library_decompress(
                    safe_decompress, compressed, FRAME_BUFFER_CAPACITY
                )
                if exact_result != 0 or exact_size != expected_output_size:
                    raise SystemExit(
                        f"{context}: exact LZO result {exact_result}, size {exact_size}"
                    )
                if exact_output != assembly_output:
                    raise SystemExit(f"{context}: LZO and assembly output differ")

                cached_result, cached_output, cached_size = library_decompress(
                    safe_decompress, record, FRAME_BUFFER_CAPACITY
                )
                if cached_result != -8 or cached_size != expected_output_size:
                    raise SystemExit(
                        f"{context}: cached-path LZO result {cached_result}, "
                        f"size {cached_size}"
                    )
                if cached_output != assembly_output:
                    raise SystemExit(f"{context}: cached-path output differs")
                first_load_policy: object = exact_result
                cached_reload_policy: object = cached_result
                compressed_frame_count += 1
            else:
                if expected_output_size != 0 or span_count != 0:
                    raise SystemExit(
                        f"{context}: empty payload has nonzero output/span declaration"
                    )
                assembly_output = b""
                first_load_policy = "skipped_when_size_minus_12_is_zero"
                cached_reload_policy = (
                    "unsafe_12_byte_metadata_plus_stale_scratch_tail"
                )
                zero_payload_frame_count += 1

            (
                maximum_target_end,
                indexed_pixels,
                consumed,
                unconsumed_command_bytes,
            ) = parse_spans(
                assembly_output, span_count, context
            )
            next_declared_total_size, next_span_count, next_record_size = (
                struct.unpack_from("<III", record, record_size - FRAME_TRAILER_SIZE)
            )
            output_hash = hashlib.sha256(assembly_output).hexdigest()
            frame_rows.append(
                (
                    path.name,
                    frame_index,
                    f"0x{record_offset:08X}",
                    record_size,
                    len(compressed),
                    declared_total_size,
                    expected_output_size,
                    span_count,
                    indexed_pixels,
                    consumed,
                    unconsumed_command_bytes,
                    maximum_target_end,
                    first_load_policy,
                    cached_reload_policy,
                    next_declared_total_size,
                    next_span_count,
                    next_record_size,
                    output_hash,
                )
            )

            file_compressed_bytes += len(compressed)
            file_decompressed_bytes += len(assembly_output)
            file_spans += span_count
            file_indexed_pixels += indexed_pixels
            file_unconsumed_command_bytes += unconsumed_command_bytes
            file_maximum_target_end = max(file_maximum_target_end, maximum_target_end)
            file_outputs.update(assembly_output)
            all_frame_outputs.update(assembly_output)

            if frame_index < frame_count:
                if next_record_size < FRAME_TRAILER_SIZE:
                    raise SystemExit(f"{context}: next record size is invalid")
                declared_total_size = next_declared_total_size
                span_count = next_span_count
                record_size = next_record_size
            record_offset = record_end

        palette_start = len(data) - INITIAL_PALETTE_SIZE
        if record_offset - FRAME_TRAILER_SIZE != palette_start:
            raise SystemExit(
                f"{path.name}: final frame/palette overlap is not 12 bytes"
            )
        palette = data[palette_start:]
        file_rows.append(
            (
                path.name,
                len(data),
                hashlib.sha256(data).hexdigest(),
                frame_count,
                storage_bpp,
                canvas_width,
                canvas_height,
                display_width,
                display_height,
                f"0x{flags_12:04X}",
                palette_event_count,
                header_u16_16,
                initial_declared_total_size,
                initial_span_count,
                initial_record_size,
                file_compressed_bytes,
                file_decompressed_bytes,
                file_spans,
                file_indexed_pixels,
                file_unconsumed_command_bytes,
                file_maximum_target_end,
                INITIAL_PALETTE_SIZE,
                FRAME_TRAILER_SIZE,
                hashlib.sha256(palette).hexdigest(),
                file_outputs.hexdigest(),
            )
        )

        total_file_bytes += len(data)
        total_frames += frame_count
        total_compressed_bytes += file_compressed_bytes
        total_decompressed_bytes += file_decompressed_bytes
        total_spans += file_spans
        total_indexed_pixels += file_indexed_pixels
        total_unconsumed_command_bytes += file_unconsumed_command_bytes

    if total_frames != EXPECTED_FRAME_COUNT:
        raise SystemExit(f"unexpected total ANI frame count: {total_frames}")

    with FILE_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "file_size",
                "file_sha256",
                "frame_count",
                "storage_bpp",
                "canvas_width",
                "canvas_height",
                "display_width",
                "display_height",
                "flags_12_hex",
                "palette_event_count",
                "header_u16_16",
                "initial_declared_total_size",
                "initial_span_count",
                "initial_record_size",
                "compressed_payload_bytes",
                "decompressed_command_bytes",
                "span_count",
                "indexed_pixel_count",
                "unconsumed_command_bytes",
                "maximum_target_end",
                "initial_palette_bytes",
                "final_record_palette_overlap_bytes",
                "initial_palette_sha256",
                "concatenated_frame_output_sha256",
            )
        )
        writer.writerows(file_rows)

    with FRAME_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "frame_index",
                "record_offset_hex",
                "record_size",
                "compressed_size_without_trailer",
                "declared_total_size",
                "decompressed_command_size",
                "span_count",
                "indexed_pixel_count",
                "span_bytes_consumed",
                "unconsumed_command_bytes",
                "maximum_target_end",
                "first_load_lzo_return",
                "cached_reload_lzo_return",
                "trailer_next_declared_total_size",
                "trailer_next_span_count",
                "trailer_next_record_size",
                "decompressed_sha256",
            )
        )
        writer.writerows(frame_rows)

    with SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("metric", "value"))
        writer.writerow(("ani_file_count", len(paths)))
        writer.writerow(("ani_file_bytes", total_file_bytes))
        writer.writerow(("frame_count", total_frames))
        writer.writerow(("compressed_payload_bytes", total_compressed_bytes))
        writer.writerow(("decompressed_command_bytes", total_decompressed_bytes))
        writer.writerow(("span_count", total_spans))
        writer.writerow(("indexed_pixel_count", total_indexed_pixels))
        writer.writerow(("unconsumed_command_bytes", total_unconsumed_command_bytes))
        writer.writerow(("compressed_frame_count", compressed_frame_count))
        writer.writerow(("zero_payload_frame_count", zero_payload_frame_count))
        writer.writerow(("first_load_return_zero_count", compressed_frame_count))
        writer.writerow(("first_load_skip_count", zero_payload_frame_count))
        writer.writerow(("cached_reload_return_minus_8_count", compressed_frame_count))
        writer.writerow(("cached_reload_stale_scratch_path_count", zero_payload_frame_count))
        writer.writerow(("lzo_comparison_version", lzo_version))
        writer.writerow(("concatenated_frame_output_sha256", all_frame_outputs.hexdigest()))
        writer.writerow(("assembly_branch_rows_hit", sum(count > 0 for count in branch_counts.values())))
        for branch in sorted(branch_counts):
            writer.writerow((f"branch_{branch}_hits", branch_counts[branch]))

    print(
        f"validated {len(paths)} ANI files and {total_frames} frames: "
        f"{total_compressed_bytes} compressed bytes -> "
        f"{total_decompressed_bytes} command bytes"
    )


if __name__ == "__main__":
    main()
