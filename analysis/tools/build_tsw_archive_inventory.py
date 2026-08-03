#!/usr/bin/env python3
"""Inventory the six TSW archives using the layout proven by assembly."""

from __future__ import annotations

import csv
import struct
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
ARCHIVE_OUTPUT = INVENTORY_ROOT / "tsw-archives.tsv"
INDEX_OUTPUT = INVENTORY_ROOT / "tsw-archive-index.tsv"
FRAME_OUTPUT = INVENTORY_ROOT / "tsw-frame-descriptors.tsv"

ARCHIVES = (
    (0, "all_char.tsw"),
    (1, "all_item.tsw"),
    (2, "all_magic.tsw"),
    (3, "all_sys.tsw"),
    (4, "all_map1.tsw"),
    (5, "all_map2.tsw"),
)

ARCHIVE_HEADER_SIZE = 0x1C
INDEX_RECORD_SIZE = 0x2C
BLOCK_HEADER_SIZE = 0x0C
FRAME_DESCRIPTOR_SIZE = 0x24
TSW_MAGIC = 0xABCD
FIXED_INDEX_POSITION_COUNT = 3000
EXPECTED_ARCHIVE_COUNT = 6
EXPECTED_INDEX_POSITION_COUNT = 18000
EXPECTED_NONEMPTY_RECORD_COUNT = 2928
EXPECTED_FRAME_COUNT = 20091
EXPECTED_COMPRESSED_PAYLOAD_BYTES = 558351505
EXPECTED_DECLARED_DECOMPRESSED_BYTES = 1378998573


@dataclass(frozen=True)
class IndexRecord:
    ordinal: int
    name_raw: bytes
    block_size: int
    block_offset: int
    metadata_id: int
    unknown_20: int
    unknown_24: int
    unknown_28: int


def read_exact(input_file, size: int, context: str) -> bytes:
    data = input_file.read(size)
    if len(data) != size:
        raise SystemExit(f"short read for {context}: wanted {size}, got {len(data)}")
    return data


def decode_name(raw: bytes) -> tuple[str, str]:
    trimmed = raw.split(b"\0", 1)[0]
    decoded = trimmed.decode("big5", errors="replace")
    return decoded.replace("\t", " ").replace("\n", " "), trimmed.hex().upper()


def main() -> None:
    archive_rows: list[tuple[object, ...]] = []
    index_rows: list[tuple[object, ...]] = []
    frame_rows: list[tuple[object, ...]] = []

    total_index_records = 0
    total_nonempty_records = 0
    total_frames = 0
    total_compressed_bytes = 0
    total_decompressed_bytes = 0
    total_trailing_span_bytes = 0

    for group, filename in ARCHIVES:
        path = WORKSPACE_ROOT / filename
        file_size = path.stat().st_size
        with path.open("rb") as input_file:
            header = read_exact(input_file, ARCHIVE_HEADER_SIZE, f"{filename} header")
            if any(header[:0x18]):
                raise SystemExit(f"{filename}: nonzero reserved archive header bytes")
            declared_record_count = struct.unpack_from("<I", header, 0x18)[0]
            records: list[IndexRecord] = []
            for ordinal in range(1, FIXED_INDEX_POSITION_COUNT + 1):
                raw = read_exact(
                    input_file,
                    INDEX_RECORD_SIZE,
                    f"{filename} index record {ordinal}",
                )
                fields = struct.unpack_from("<6I", raw, 0x14)
                records.append(IndexRecord(ordinal, raw[:0x14], *fields))

            archive_frame_count = 0
            archive_nonempty_records = 0
            archive_compressed_bytes = 0
            archive_decompressed_bytes = 0
            archive_trailing_span_bytes = 0
            archive_8bpp_records = 0
            archive_16bpp_records = 0
            archive_other_bpp_records = 0
            archive_nonempty_beyond_declared = 0
            min_block_offset = file_size
            max_block_end = 0
            block_ranges: list[tuple[int, int, int]] = []

            for record in records:
                lookup_id = group * 3000 + record.ordinal
                name, name_hex = decode_name(record.name_raw)
                is_nonempty = int(record.block_size != 0 or record.block_offset != 0)
                if not is_nonempty:
                    index_rows.append(
                        (
                            filename,
                            group,
                            record.ordinal,
                            lookup_id,
                            0,
                            record.metadata_id,
                            name,
                            name_hex,
                            record.block_size,
                            "",
                            "",
                            record.unknown_20,
                            record.unknown_24,
                            record.unknown_28,
                            "",
                            "",
                            "",
                            "",
                            "",
                            "",
                        )
                    )
                    continue
                if record.block_size == 0 or record.block_offset == 0:
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: half-empty index record"
                    )
                block_end = record.block_offset + record.block_size
                if (
                    record.block_offset
                    < ARCHIVE_HEADER_SIZE
                    + FIXED_INDEX_POSITION_COUNT * INDEX_RECORD_SIZE
                ):
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: block overlaps archive index"
                    )
                if block_end > file_size:
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: block ends past EOF"
                    )

                input_file.seek(record.block_offset)
                block_header = read_exact(
                    input_file,
                    BLOCK_HEADER_SIZE,
                    f"{filename} block header {record.ordinal}",
                )
                (
                    block_value_00,
                    magic,
                    frame_count,
                    storage_bpp,
                    header_size,
                ) = struct.unpack("<IHHHH", block_header)
                if magic != TSW_MAGIC or header_size != BLOCK_HEADER_SIZE:
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: unexpected block header "
                        f"magic=0x{magic:04X}, size={header_size}"
                    )
                if storage_bpp == 0:
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: zero storage bpp"
                    )

                palette_bytes = 0x200 if storage_bpp == 8 else 0
                descriptor_table_offset = (
                    record.block_offset + BLOCK_HEADER_SIZE + palette_bytes
                )
                descriptor_table_bytes = frame_count * FRAME_DESCRIPTOR_SIZE
                payload_floor = (
                    BLOCK_HEADER_SIZE + palette_bytes + descriptor_table_bytes
                )
                if payload_floor > record.block_size:
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: descriptor table exceeds block"
                    )

                input_file.seek(descriptor_table_offset)
                descriptors = read_exact(
                    input_file,
                    descriptor_table_bytes,
                    f"{filename} frame descriptors {record.ordinal}",
                )
                previous_frame_extent_end = payload_floor
                for variant in range(frame_count):
                    values = struct.unpack_from(
                        "<9I", descriptors, variant * FRAME_DESCRIPTOR_SIZE
                    )
                    (
                        payload_relative_offset,
                        compressed_size,
                        decompressed_size,
                        metadata_0c,
                        metadata_10,
                        metadata_14,
                        metadata_18,
                        metadata_1c,
                        dimensions,
                    ) = values
                    width = dimensions & 0xFFFF
                    height = dimensions >> 16
                    payload_end = payload_relative_offset + compressed_size
                    if payload_relative_offset < payload_floor:
                        raise SystemExit(
                            f"{filename} record {record.ordinal} variant {variant}: "
                            "payload overlaps headers"
                        )
                    if payload_end > record.block_size:
                        raise SystemExit(
                            f"{filename} record {record.ordinal} variant {variant}: "
                            "payload exceeds block"
                        )
                    frame_extent_end = payload_end + metadata_10
                    if frame_extent_end > record.block_size:
                        raise SystemExit(
                            f"{filename} record {record.ordinal} variant {variant}: "
                            "primary payload plus descriptor +0x10 trailing span exceeds block"
                        )
                    if payload_relative_offset != previous_frame_extent_end:
                        raise SystemExit(
                            f"{filename} record {record.ordinal} variant {variant}: "
                            "frame extent is not contiguous with previous descriptor"
                        )

                    frame_rows.append(
                        (
                            filename,
                            group,
                            record.ordinal,
                            lookup_id,
                            record.metadata_id,
                            variant,
                            f"{record.block_offset:08X}",
                            f"{payload_relative_offset:08X}",
                            f"{record.block_offset + payload_relative_offset:08X}",
                            compressed_size,
                            decompressed_size,
                            metadata_0c,
                            metadata_10,
                            metadata_14,
                            metadata_18,
                            metadata_1c,
                            width,
                            height,
                            storage_bpp,
                            f"{frame_extent_end:08X}",
                            1,
                        )
                    )
                    previous_frame_extent_end = frame_extent_end
                    archive_compressed_bytes += compressed_size
                    archive_decompressed_bytes += decompressed_size
                    archive_trailing_span_bytes += metadata_10

                exact_block_extent = int(previous_frame_extent_end == record.block_size)
                if not exact_block_extent:
                    raise SystemExit(
                        f"{filename} record {record.ordinal}: frame extents do not reach block end"
                    )
                index_rows.append(
                    (
                        filename,
                        group,
                        record.ordinal,
                        lookup_id,
                        1,
                        record.metadata_id,
                        name,
                        name_hex,
                        record.block_size,
                        f"{record.block_offset:08X}",
                        f"{block_end:08X}",
                        record.unknown_20,
                        record.unknown_24,
                        record.unknown_28,
                        block_value_00,
                        frame_count,
                        storage_bpp,
                        palette_bytes,
                        f"{descriptor_table_offset:08X}",
                        exact_block_extent,
                    )
                )

                archive_frame_count += frame_count
                archive_nonempty_records += 1
                archive_8bpp_records += int(storage_bpp == 8)
                archive_16bpp_records += int(storage_bpp == 16)
                archive_other_bpp_records += int(storage_bpp not in (8, 16))
                archive_nonempty_beyond_declared += int(
                    record.ordinal > declared_record_count
                )
                min_block_offset = min(min_block_offset, record.block_offset)
                max_block_end = max(max_block_end, block_end)
                block_ranges.append((record.block_offset, block_end, record.ordinal))

            block_ranges.sort()
            inter_block_gap_bytes = 0
            for previous, current in zip(block_ranges, block_ranges[1:]):
                if current[0] < previous[1]:
                    raise SystemExit(
                        f"{filename}: block {current[2]} overlaps block {previous[2]}"
                    )
                inter_block_gap_bytes += current[0] - previous[1]
            fixed_index_end = (
                ARCHIVE_HEADER_SIZE
                + FIXED_INDEX_POSITION_COUNT * INDEX_RECORD_SIZE
            )
            bytes_between_index_and_first_block = min_block_offset - fixed_index_end

            archive_rows.append(
                (
                    filename,
                    group,
                    file_size,
                    declared_record_count,
                    FIXED_INDEX_POSITION_COUNT,
                    archive_nonempty_records,
                    archive_nonempty_beyond_declared,
                    archive_frame_count,
                    archive_8bpp_records,
                    archive_16bpp_records,
                    archive_other_bpp_records,
                    archive_compressed_bytes,
                    archive_decompressed_bytes,
                    archive_trailing_span_bytes,
                    f"{min_block_offset:08X}",
                    f"{max_block_end:08X}",
                    int(max_block_end == file_size),
                    bytes_between_index_and_first_block,
                    inter_block_gap_bytes,
                )
            )
            total_index_records += FIXED_INDEX_POSITION_COUNT
            total_nonempty_records += archive_nonempty_records
            total_frames += archive_frame_count
            total_compressed_bytes += archive_compressed_bytes
            total_decompressed_bytes += archive_decompressed_bytes
            total_trailing_span_bytes += archive_trailing_span_bytes

    observed = (
        len(ARCHIVES),
        total_index_records,
        total_nonempty_records,
        total_frames,
        total_compressed_bytes,
        total_decompressed_bytes,
    )
    expected = (
        EXPECTED_ARCHIVE_COUNT,
        EXPECTED_INDEX_POSITION_COUNT,
        EXPECTED_NONEMPTY_RECORD_COUNT,
        EXPECTED_FRAME_COUNT,
        EXPECTED_COMPRESSED_PAYLOAD_BYTES,
        EXPECTED_DECLARED_DECOMPRESSED_BYTES,
    )
    if observed != expected:
        raise SystemExit(f"unexpected TSW sample totals: observed={observed}, expected={expected}")

    INVENTORY_ROOT.mkdir(parents=True, exist_ok=True)
    with ARCHIVE_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "group",
                "file_size_bytes",
                "declared_record_count",
                "physical_index_position_count",
                "nonempty_record_count",
                "nonempty_beyond_declared_count",
                "frame_count",
                "record_count_8bpp",
                "record_count_16bpp",
                "record_count_other_bpp",
                "compressed_payload_bytes",
                "declared_decompressed_bytes",
                "descriptor_10_trailing_span_bytes",
                "minimum_block_offset_hex",
                "maximum_block_end_hex",
                "blocks_reach_exact_eof",
                "bytes_between_fixed_index_and_first_block",
                "inter_block_gap_bytes",
            )
        )
        writer.writerows(archive_rows)

    with INDEX_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "group",
                "record_ordinal",
                "lookup_resource_id",
                "is_nonempty",
                "stored_metadata_id",
                "name_big5_best_effort",
                "name_raw_hex",
                "block_size_bytes",
                "block_offset_hex",
                "block_end_hex",
                "index_unknown_20",
                "index_unknown_24",
                "index_unknown_28",
                "block_value_00",
                "frame_count",
                "storage_bpp",
                "palette_bytes",
                "descriptor_table_offset_hex",
                "payloads_reach_exact_block_end",
            )
        )
        writer.writerows(index_rows)

    with FRAME_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "group",
                "record_ordinal",
                "lookup_resource_id",
                "stored_metadata_id",
                "variant_index",
                "block_offset_hex",
                "payload_relative_offset_hex",
                "payload_absolute_offset_hex",
                "compressed_size_bytes",
                "declared_decompressed_size_bytes",
                "metadata_0c",
                "descriptor_10_trailing_span_bytes",
                "metadata_14",
                "metadata_18",
                "metadata_1c",
                "width",
                "height",
                "storage_bpp",
                "frame_extent_end_relative_hex",
                "frame_extent_contiguous_with_previous",
            )
        )
        writer.writerows(frame_rows)

    print(
        f"wrote {len(ARCHIVES)} TSW archives, {total_index_records} index records, "
        f"{total_frames} frames, {total_compressed_bytes} compressed bytes and "
        f"{total_decompressed_bytes} declared decompressed bytes; descriptor +0x10 "
        f"adds {total_trailing_span_bytes} trailing-span bytes"
    )


if __name__ == "__main__":
    main()
