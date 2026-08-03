#!/usr/bin/env python3
"""Inventory all.snd according to sub_4862B0 and sub_486490 assembly."""

from __future__ import annotations

import csv
import hashlib
import struct
from collections import Counter
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
ARCHIVE_PATH = WORKSPACE_ROOT / "all.snd"
INDEX_OUTPUT = INVENTORY_ROOT / "snd-archive-index.tsv"
VIEW_OUTPUT = INVENTORY_ROOT / "snd-loader-views.tsv"
FORMAT_OUTPUT = INVENTORY_ROOT / "snd-format-summary.tsv"
SUMMARY_OUTPUT = INVENTORY_ROOT / "snd-archive-summary.tsv"

FIXED_SLOT_COUNT = 3000
INDEX_OFFSET = 0x1C
INDEX_RECORD_SIZE = 0x2C
INDEX_SIZE = FIXED_SLOT_COUNT * INDEX_RECORD_SIZE
PAYLOAD_START = INDEX_OFFSET + INDEX_SIZE
RUNTIME_RECORD_SIZE = 0x10
RUNTIME_SIZE_MASK = 0x03FFFFFF
TYPE_MASK = 0x00000003
RIFF_PREFIX_SIZE = 0x18
TEMPLATE = b"RIFF....WAVEfmt ....0123456789012345data..\0"

EXPECTED_FILE_SIZE = 54596397
EXPECTED_DECLARED_COUNT = 664
EXPECTED_NONEMPTY_COUNT = 664
EXPECTED_UNIQUE_PAYLOAD_OFFSETS = 662
EXPECTED_SPANNING_ALIAS_IDS = (270, 277)
EXPECTED_RUNTIME_VIEW_BYTES = 117128703
EXPECTED_LOADER_ALLOCATION_BYTES = 117144639
EXPECTED_MALFORMED_CHUNK_TAG_IDS = (506, 507)
EXPECTED_CONCATENATED_LOADER_HASH = (
    "38ecbbcbb7473ab8c8ec116837b8c472fa7ed274ad42e4a0e0f04aaef6241e27"
)


def u32(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def clean_name(raw: bytes) -> str:
    return raw.decode("cp950").replace("\t", "\\t").replace("\n", "\\n")


def loader_view(payload: bytes, runtime_type: int) -> bytes:
    """Reproduce the valid-file path of sub_486490.

    The executable's local _memcpy implementation detects overlap.  The only
    sample type is zero, where destination precedes source and the 18-byte copy
    proceeds forward.  Bounds checks here reject research-data damage only.
    """

    prefix_size = RIFF_PREFIX_SIZE if runtime_type == 0 else 0
    allocation_size = len(payload) + prefix_size
    if allocation_size < len(TEMPLATE):
        raise ValueError("loader allocation is smaller than the copied template")
    result = bytearray(allocation_size)
    result[: len(TEMPLATE)] = TEMPLATE
    result[prefix_size : prefix_size + len(payload)] = payload

    if runtime_type != 1:
        if len(result) < 44:
            raise ValueError("loader rewrite touches beyond the allocated view")
        # Exact overlap result for _memcpy(result+0x14, result+0x18, 0x12).
        result[0x14 : 0x14 + 0x12] = bytes(result[0x18 : 0x18 + 0x12])
        struct.pack_into("<I", result, 0x10, 0x10)
        struct.pack_into("<I", result, 0x04, len(payload))
        struct.pack_into("<H", result, 0x26, 0x6174)
        struct.pack_into("<I", result, 0x28, (len(payload) - 0x18) & 0xFFFFFFFF)

    return bytes(result)


def main() -> None:
    archive = ARCHIVE_PATH.read_bytes()
    if len(archive) != EXPECTED_FILE_SIZE:
        raise SystemExit(f"unexpected all.snd size: {len(archive)}")
    if archive[:0x18] != bytes(0x18):
        raise SystemExit("all.snd +0x00..+0x17 are not all zero")
    declared_count = u32(archive, 0x18)
    if declared_count != EXPECTED_DECLARED_COUNT:
        raise SystemExit(f"unexpected all.snd declared count: {declared_count}")
    if len(archive) < PAYLOAD_START:
        raise SystemExit("all.snd is shorter than its fixed 3000-slot index")

    records: list[dict[str, object]] = []
    for index in range(FIXED_SLOT_COUNT):
        ordinal = index + 1
        offset = INDEX_OFFSET + index * INDEX_RECORD_SIZE
        record = archive[offset : offset + INDEX_RECORD_SIZE]
        name_field = record[:0x14]
        name_raw = name_field.split(b"\0", 1)[0]
        raw_size, payload_offset, stored_id, raw_type, reserved_24, reserved_28 = (
            struct.unpack_from("<6I", record, 0x14)
        )
        runtime_size = raw_size & RUNTIME_SIZE_MASK
        runtime_type = raw_type & TYPE_MASK
        nonempty = payload_offset != 0
        records.append(
            {
                "ordinal": ordinal,
                "raw_record": record,
                "name_raw": name_raw,
                "name_cp950": clean_name(name_raw),
                "raw_size": raw_size,
                "runtime_size": runtime_size,
                "payload_offset": payload_offset,
                "stored_id": stored_id,
                "raw_type": raw_type,
                "runtime_type": runtime_type,
                "reserved_24": reserved_24,
                "reserved_28": reserved_28,
                "nonempty": nonempty,
            }
        )

    nonempty_records = [record for record in records if record["nonempty"]]
    if len(nonempty_records) != EXPECTED_NONEMPTY_COUNT:
        raise SystemExit(f"unexpected nonempty all.snd entries: {len(nonempty_records)}")
    if any(record["raw_record"] != bytes(INDEX_RECORD_SIZE) for record in records[declared_count:]):
        raise SystemExit("nonzero all.snd physical index slot after the declared entries")
    if any(record["stored_id"] != record["ordinal"] for record in nonempty_records):
        raise SystemExit("all.snd nonempty stored ID differs from physical ordinal")
    if any(record["raw_type"] != 0 for record in nonempty_records):
        raise SystemExit("current all.snd contains a nonzero raw type field")
    if any(record["reserved_24"] or record["reserved_28"] for record in nonempty_records):
        raise SystemExit("current all.snd contains nonzero reserved index fields")

    offset_counts = Counter(int(record["payload_offset"]) for record in nonempty_records)
    if len(offset_counts) != EXPECTED_UNIQUE_PAYLOAD_OFFSETS:
        raise SystemExit(f"unexpected unique all.snd payload offsets: {len(offset_counts)}")

    alias_ids = tuple(
        int(record["ordinal"])
        for record in nonempty_records
        if int(record["raw_size"]) & ~RUNTIME_SIZE_MASK
    )
    if alias_ids != EXPECTED_SPANNING_ALIAS_IDS:
        raise SystemExit(f"unexpected spanning alias IDs: {alias_ids}")

    physical_records = [
        record
        for record in nonempty_records
        if not (int(record["raw_size"]) & ~RUNTIME_SIZE_MASK)
    ]
    physical_records.sort(key=lambda record: int(record["payload_offset"]))
    if len(physical_records) != EXPECTED_UNIQUE_PAYLOAD_OFFSETS:
        raise SystemExit("base physical block count differs from unique payload offsets")
    expected_offset = PAYLOAD_START
    for record in physical_records:
        payload_offset = int(record["payload_offset"])
        runtime_size = int(record["runtime_size"])
        if payload_offset != expected_offset:
            raise SystemExit(
                f"physical sound block discontinuity before ID {record['ordinal']}: "
                f"0x{expected_offset:08X} != 0x{payload_offset:08X}"
            )
        expected_offset += runtime_size
    if expected_offset != len(archive):
        raise SystemExit(
            f"physical sound blocks end at 0x{expected_offset:08X}, "
            f"file ends at 0x{len(archive):08X}"
        )

    with INDEX_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "record_ordinal",
                "sound_id",
                "name_bytes_hex",
                "name_cp950",
                "raw_size_field_hex",
                "runtime_payload_size_bytes",
                "raw_size_high_bits_hex",
                "payload_offset_hex",
                "stored_metadata_id",
                "raw_type_field_hex",
                "runtime_type_low2",
                "reserved_24_hex",
                "reserved_28_hex",
                "nonempty",
                "same_offset_view_count",
                "view_kind",
            )
        )
        for record in records:
            payload_offset = int(record["payload_offset"])
            high_bits = int(record["raw_size"]) & ~RUNTIME_SIZE_MASK
            if not record["nonempty"]:
                view_kind = "empty"
            elif high_bits:
                view_kind = "spanning_alias_view"
            else:
                view_kind = "base_physical_block"
            writer.writerow(
                (
                    ARCHIVE_PATH.name,
                    record["ordinal"],
                    record["ordinal"],
                    bytes(record["name_raw"]).hex().upper(),
                    record["name_cp950"],
                    f"{int(record['raw_size']):08X}",
                    record["runtime_size"],
                    f"{high_bits:08X}",
                    f"{payload_offset:08X}",
                    record["stored_id"],
                    f"{int(record['raw_type']):08X}",
                    record["runtime_type"],
                    f"{int(record['reserved_24']):08X}",
                    f"{int(record['reserved_28']):08X}",
                    int(bool(record["nonempty"])),
                    offset_counts.get(payload_offset, 0) if payload_offset else 0,
                    view_kind,
                )
            )

    view_rows: list[tuple[object, ...]] = []
    format_counts: Counter[tuple[object, ...]] = Counter()
    concatenated_loader_hash = hashlib.sha256()
    total_runtime_payload_bytes = 0
    total_loader_allocation_bytes = 0
    malformed_chunk_tag_ids: list[int] = []
    for record in nonempty_records:
        sound_id = int(record["ordinal"])
        payload_offset = int(record["payload_offset"])
        runtime_size = int(record["runtime_size"])
        runtime_type = int(record["runtime_type"])
        payload_end = payload_offset + runtime_size
        if runtime_size == 0 or payload_end > len(archive):
            raise SystemExit(f"sound ID {sound_id} runtime view is out of file bounds")
        payload = archive[payload_offset:payload_end]
        returned = loader_view(payload, runtime_type)
        prefix_size = RIFF_PREFIX_SIZE if runtime_type == 0 else 0

        if runtime_type == 0:
            if returned[44:] != payload[20:]:
                raise SystemExit(f"sound ID {sound_id} type-zero tail mapping differs")
            if len(returned) != runtime_size + RIFF_PREFIX_SIZE:
                raise SystemExit(f"sound ID {sound_id} type-zero allocation differs")

        if len(payload) >= 16:
            format_tag, channels, sample_rate, byte_rate, block_align, bits = (
                struct.unpack_from("<HHIIHH", payload, 0)
            )
        else:
            format_tag = channels = sample_rate = byte_rate = block_align = bits = 0
        source_marker = payload[16:18]
        returned_chunk_tag = returned[36:40] if len(returned) >= 40 else b""
        if returned_chunk_tag != b"data":
            malformed_chunk_tag_ids.append(sound_id)
        returned_riff_size = u32(returned, 4) if len(returned) >= 8 else 0
        returned_data_size = u32(returned, 40) if len(returned) >= 44 else 0
        bytes_after_declared_data = (
            len(returned) - (44 + returned_data_size)
            if len(returned) >= 44 and returned_chunk_tag == b"data"
            else ""
        )
        format_key = (
            format_tag,
            channels,
            sample_rate,
            byte_rate,
            block_align,
            bits,
            source_marker.hex().upper(),
            returned_chunk_tag.hex().upper(),
        )
        format_counts[format_key] += 1
        raw_digest = hashlib.sha256(payload).hexdigest()
        returned_digest = hashlib.sha256(returned).hexdigest()
        concatenated_loader_hash.update(returned)
        total_runtime_payload_bytes += runtime_size
        total_loader_allocation_bytes += len(returned)
        view_rows.append(
            (
                ARCHIVE_PATH.name,
                sound_id,
                record["name_cp950"],
                f"{payload_offset:08X}",
                runtime_size,
                runtime_type,
                prefix_size,
                len(returned),
                format_tag,
                channels,
                sample_rate,
                byte_rate,
                block_align,
                bits,
                source_marker.hex().upper(),
                returned[:4].hex().upper(),
                returned_chunk_tag.hex().upper(),
                returned_riff_size,
                returned_data_size,
                bytes_after_declared_data,
                raw_digest,
                returned_digest,
            )
        )

    if total_runtime_payload_bytes != EXPECTED_RUNTIME_VIEW_BYTES:
        raise SystemExit(
            f"unexpected summed runtime SND view bytes: {total_runtime_payload_bytes}"
        )
    if total_loader_allocation_bytes != EXPECTED_LOADER_ALLOCATION_BYTES:
        raise SystemExit(
            f"unexpected summed SND loader allocation bytes: "
            f"{total_loader_allocation_bytes}"
        )
    if tuple(malformed_chunk_tag_ids) != EXPECTED_MALFORMED_CHUNK_TAG_IDS:
        raise SystemExit(
            f"unexpected malformed returned chunk-tag IDs: {malformed_chunk_tag_ids}"
        )
    concatenated_loader_digest = concatenated_loader_hash.hexdigest()
    if concatenated_loader_digest != EXPECTED_CONCATENATED_LOADER_HASH:
        raise SystemExit(
            f"unexpected concatenated SND loader-return hash: "
            f"{concatenated_loader_digest}"
        )

    with VIEW_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "sound_id",
                "name_cp950",
                "payload_offset_hex",
                "runtime_payload_size_bytes",
                "runtime_type",
                "loader_prefix_bytes",
                "loader_allocation_bytes",
                "source_format_tag",
                "source_channels",
                "source_sample_rate",
                "source_byte_rate",
                "source_block_align",
                "source_bits_per_sample",
                "source_marker_16_hex",
                "returned_first4_hex",
                "returned_chunk_36_hex",
                "returned_riff_size_field",
                "returned_data_size_field",
                "bytes_after_declared_data",
                "raw_payload_sha256",
                "loader_return_sha256",
            )
        )
        writer.writerows(view_rows)

    with FORMAT_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "source_format_tag",
                "channels",
                "sample_rate",
                "byte_rate",
                "block_align",
                "bits_per_sample",
                "source_marker_16_hex",
                "returned_chunk_36_hex",
                "entry_count",
            )
        )
        for key, count in sorted(format_counts.items()):
            writer.writerow((*key, count))

    with SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "file_size_bytes",
                "header_declared_count",
                "fixed_index_slots",
                "nonempty_index_entries",
                "unique_payload_offsets",
                "base_physical_blocks",
                "spanning_alias_views",
                "physical_payload_start_hex",
                "physical_payload_end_hex",
                "physical_payload_bytes",
                "runtime_view_payload_bytes",
                "loader_allocation_bytes",
                "runtime_type_0_entries",
                "runtime_type_1_entries",
                "runtime_type_2_entries",
                "runtime_type_3_entries",
                "concatenated_loader_return_sha256",
            )
        )
        type_counts = Counter(int(record["runtime_type"]) for record in nonempty_records)
        writer.writerow(
            (
                ARCHIVE_PATH.name,
                len(archive),
                declared_count,
                FIXED_SLOT_COUNT,
                len(nonempty_records),
                len(offset_counts),
                len(physical_records),
                len(alias_ids),
                f"{PAYLOAD_START:08X}",
                f"{len(archive):08X}",
                len(archive) - PAYLOAD_START,
                total_runtime_payload_bytes,
                total_loader_allocation_bytes,
                type_counts[0],
                type_counts[1],
                type_counts[2],
                type_counts[3],
                concatenated_loader_digest,
            )
        )

    print(
        f"wrote {len(records)} all.snd index slots, {len(nonempty_records)} loader "
        f"views over {len(physical_records)} contiguous physical blocks; "
        f"types={dict(sorted(type_counts.items()))}"
    )


if __name__ == "__main__":
    main()
