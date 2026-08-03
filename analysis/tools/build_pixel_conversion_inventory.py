#!/usr/bin/env python3
"""Rebuild the active CM caches through every assembly-selected pixel converter."""

from __future__ import annotations

import csv
import hashlib
import mmap
import sys
from array import array
from pathlib import Path
from struct import unpack_from

from compare_lzo1x_compatibility import library_decompress, load_lzo
from verify_tsw_decompression import decompress_sub_4399e0


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
OUTPUT = INVENTORY_ROOT / "pixel-conversion-cache-comparison.tsv"
SUMMARY_OUTPUT = INVENTORY_ROOT / "pixel-conversion-summary.tsv"

LMF_PATH = WORKSPACE_ROOT / "huge.lmf"
MCACHE_PATH = WORKSPACE_ROOT / "Data" / "mcache.dat"
TARGET_MAPS = {24, 81}

TRANSFORMS = (
    ("identity_rgb555", "0044A240", 0x7C00, 0x03E0, 0x001F),
    ("shift_whole_word_left_1", "004238F0", 0xF800, 0x07C0, 0x003F),
    ("rgb555_to_rgb565", "00423920", 0xF800, 0x07E0, 0x001F),
    ("shift_red_field_left_1", "00423990", 0xFC00, 0x03E0, 0x001F),
)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def transform_pixels(source: bytes, transform_name: str) -> bytes:
    if len(source) % 2:
        raise ValueError("16-bit conversion received an odd byte count")
    if sys.byteorder != "little":
        raise RuntimeError("this verifier requires a little-endian host")
    pixels = array("H")
    pixels.frombytes(source)
    if transform_name == "identity_rgb555":
        return source
    if transform_name == "shift_whole_word_left_1":
        for index, pixel in enumerate(pixels):
            pixels[index] = (pixel << 1) & 0xFFFF
    elif transform_name == "rgb555_to_rgb565":
        for index, pixel in enumerate(pixels):
            pixels[index] = (((pixel & 0xFFE0) << 1) + (pixel & 0x001F)) & 0xFFFF
    elif transform_name == "shift_red_field_left_1":
        for index, pixel in enumerate(pixels):
            pixels[index] = (((pixel & 0x7C00) << 1) | (pixel & 0x03FF)) & 0xFFFF
    else:
        raise ValueError(f"unknown transform: {transform_name}")
    return pixels.tobytes()


def compare_bytes(actual: bytes, expected: bytes) -> tuple[int, int]:
    if len(actual) != len(expected):
        raise ValueError("comparison inputs have different sizes")
    mismatch_count = 0
    first_mismatch = -1
    for offset, (left, right) in enumerate(zip(actual, expected)):
        if left != right:
            mismatch_count += 1
            if first_mismatch < 0:
                first_mismatch = offset
    return mismatch_count, first_mismatch


def active_cache_records() -> dict[int, tuple[int, int]]:
    data = MCACHE_PATH.read_bytes()
    if len(data) % 16:
        raise SystemExit("Data/mcache.dat is not an array of 16-byte records")
    records: dict[int, tuple[int, int]] = {}
    for unit in range(len(data) // 16):
        map_id, output_size, _, _ = unpack_from("<IIII", data, unit * 16)
        if map_id != 0xFFFFFFFF:
            records[map_id] = (unit, output_size)
    if set(records) != TARGET_MAPS:
        raise SystemExit(f"unexpected active cache maps: {sorted(records)}")
    return records


def map_offsets(data: mmap.mmap) -> dict[int, int]:
    index_offset = unpack_from("<I", data, 0)[0]
    result: dict[int, int] = {}
    for record_offset in range(index_offset, len(data), 16):
        map_offset, map_span, map_id, reserved = unpack_from("<IIII", data, record_offset)
        if map_offset == map_span == map_id == reserved == 0:
            break
        if map_id in TARGET_MAPS:
            result[map_id] = map_offset
    if set(result) != TARGET_MAPS:
        raise SystemExit(f"target maps missing from LMF index: {sorted(TARGET_MAPS - set(result))}")
    return result


def decode_cm(
    data: mmap.mmap,
    map_offset: int,
    safe_decompress: object,
) -> tuple[bytes, int, int, int]:
    cm_header = map_offset + unpack_from("<I", data, map_offset + 0x20)[0]
    total_size, chunk_size = unpack_from("<II", data, cm_header + 0x10)
    chunk_count = (chunk_size + total_size) // chunk_size
    capacity = chunk_size + chunk_size // 1024
    payload_offset = cm_header + 0x1A8
    remaining = total_size
    compressed_total = 0
    output = bytearray()
    for chunk_index in range(chunk_count):
        compressed_size = unpack_from("<I", data, cm_header + 0x1C + chunk_index * 8)[0]
        source = data[payload_offset : payload_offset + compressed_size]
        assembly_output = decompress_sub_4399e0(
            source,
            capacity,
            {},
            require_exact_output=False,
        )
        lzo_result, lzo_output, _ = library_decompress(safe_decompress, source, capacity)
        if lzo_result != 0 or lzo_output != assembly_output:
            raise SystemExit(f"CM chunk {chunk_index}: library/assembly mismatch")
        if len(assembly_output) != chunk_size:
            raise SystemExit(f"CM chunk {chunk_index}: output is not the declared full chunk")
        written_size = min(remaining, chunk_size)
        output.extend(assembly_output[:written_size])
        remaining -= chunk_size
        payload_offset += compressed_size
        compressed_total += compressed_size
    if len(output) != total_size:
        raise SystemExit(f"rebuilt CM size {len(output)} != declared {total_size}")
    return bytes(output), chunk_size, chunk_count, compressed_total


def main() -> None:
    cache_records = active_cache_records()
    _, safe_decompress, lzo_version = load_lzo()
    rows: list[tuple[object, ...]] = []
    exact_matches: list[tuple[int, int, str]] = []
    total_source_bytes = 0
    total_compressed_bytes = 0
    aggregate_raw = hashlib.sha256()
    aggregate_cache = hashlib.sha256()

    with LMF_PATH.open("rb") as input_file:
        data = mmap.mmap(input_file.fileno(), 0, access=mmap.ACCESS_READ)
        try:
            offsets = map_offsets(data)
            for map_id in sorted(TARGET_MAPS):
                unit, declared_size = cache_records[map_id]
                cache_path = WORKSPACE_ROOT / "Data" / f"{unit}.cm"
                cache = cache_path.read_bytes()
                raw, chunk_size, chunk_count, compressed_bytes = decode_cm(
                    data, offsets[map_id], safe_decompress
                )
                if len(raw) != declared_size or len(cache) != declared_size:
                    raise SystemExit(f"map {map_id}: cache size mismatch")
                total_source_bytes += len(raw)
                total_compressed_bytes += compressed_bytes
                aggregate_raw.update(raw)
                aggregate_cache.update(cache)
                for transform_name, function_address, red_mask, green_mask, blue_mask in TRANSFORMS:
                    transformed = transform_pixels(raw, transform_name)
                    mismatch_count, first_mismatch = compare_bytes(transformed, cache)
                    exact = int(mismatch_count == 0)
                    if exact:
                        exact_matches.append((map_id, unit, transform_name))
                    rows.append(
                        (
                            map_id,
                            unit,
                            declared_size,
                            chunk_size,
                            chunk_count,
                            compressed_bytes,
                            transform_name,
                            function_address,
                            f"0x{red_mask:04X}",
                            f"0x{green_mask:04X}",
                            f"0x{blue_mask:04X}",
                            sha256(raw),
                            sha256(transformed),
                            sha256(cache),
                            exact,
                            mismatch_count,
                            "" if first_mismatch < 0 else first_mismatch,
                            "" if first_mismatch < 0 else f"0x{first_mismatch:X}",
                        )
                    )
        finally:
            data.close()

    if len(exact_matches) != len(TARGET_MAPS):
        raise SystemExit(f"expected one exact transform per cache, got {exact_matches}")
    if {match[2] for match in exact_matches} != {"rgb555_to_rgb565"}:
        raise SystemExit(f"unexpected exact transform: {exact_matches}")

    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "map_id",
                "cache_unit",
                "caller_written_size",
                "cm_chunk_output_size",
                "cm_chunk_count",
                "compressed_bytes",
                "candidate_transform",
                "assembly_function",
                "surface_red_mask",
                "surface_green_mask",
                "surface_blue_mask",
                "raw_written_prefix_sha256",
                "candidate_output_sha256",
                "current_cache_sha256",
                "byte_exact_with_current_cache",
                "mismatching_bytes",
                "first_mismatch_offset",
                "first_mismatch_offset_hex",
            )
        )
        writer.writerows(rows)

    with SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("metric", "value"))
        writer.writerow(("comparison_library_version", lzo_version))
        writer.writerow(("active_cache_files", len(TARGET_MAPS)))
        writer.writerow(("candidate_transforms_per_file", len(TRANSFORMS)))
        writer.writerow(("comparison_rows", len(rows)))
        writer.writerow(("source_compressed_bytes", total_compressed_bytes))
        writer.writerow(("caller_written_bytes", total_source_bytes))
        writer.writerow(("exact_match_rows", len(exact_matches)))
        writer.writerow(("exact_transform", exact_matches[0][2]))
        writer.writerow(("surface_masks", "R=0xF800,G=0x07E0,B=0x001F"))
        writer.writerow(("assembly_forward_converter", "00423920"))
        writer.writerow(("assembly_reverse_converter", "00423950"))
        writer.writerow(("concatenated_raw_written_prefix_sha256", aggregate_raw.hexdigest()))
        writer.writerow(("concatenated_current_cache_sha256", aggregate_cache.hexdigest()))

    print(
        f"{len(TARGET_MAPS)} CM caches: only 00423920 RGB555->RGB565 is byte-exact "
        f"across {total_source_bytes} bytes"
    )


if __name__ == "__main__":
    main()
