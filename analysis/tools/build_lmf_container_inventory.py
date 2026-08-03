#!/usr/bin/env python3
"""Recover HUGE.LMF framing and validate every active LZO stream."""

from __future__ import annotations

import csv
import hashlib
import mmap
from collections import defaultdict
from pathlib import Path
from struct import unpack_from

from compare_lzo1x_compatibility import BRANCH_ROWS, library_decompress, load_lzo
from verify_tsw_decompression import decompress_sub_4399e0


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
INDEX_OUTPUT = INVENTORY_ROOT / "lmf-tail-index.tsv"
MAP_OUTPUT = INVENTORY_ROOT / "lmf-maps.tsv"
BLOCK_OUTPUT = INVENTORY_ROOT / "lmf-compressed-blocks.tsv"
SUMMARY_OUTPUT = INVENTORY_ROOT / "lmf-container-summary.tsv"

LMF_PATH = WORKSPACE_ROOT / "huge.lmf"
MCACHE_PATH = WORKSPACE_ROOT / "Data" / "mcache.dat"
EXPECTED_LMF_SIZE = 472_447_346
EXPECTED_LMF_SHA256 = "1fde9e3757a914a4235faa491733f06f236aa720ef8402522404636875833fc7"
EXPECTED_INDEX_RECORDS = 310
EXPECTED_MAPS = 309
EXPECTED_STREAMS = 1094
EXPECTED_COMPRESSED_BYTES = 410_310_015
EXPECTED_DECOMPRESSED_BYTES = 838_618_552
EXPECTED_OUTPUT_SHA256 = "382a63d7b8179590f584ee34f6208c7a3d6ae062d24c34c66dbab5355e31f312"


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as input_file:
        for chunk in iter(lambda: input_file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    if LMF_PATH.stat().st_size != EXPECTED_LMF_SIZE:
        raise SystemExit(f"unexpected huge.lmf size: {LMF_PATH.stat().st_size}")
    lmf_hash = file_sha256(LMF_PATH)
    if lmf_hash != EXPECTED_LMF_SHA256:
        raise SystemExit(f"unexpected huge.lmf SHA-256: {lmf_hash}")

    mcache = MCACHE_PATH.read_bytes()
    if len(mcache) % 16:
        raise SystemExit("Data/mcache.dat is not an array of 16-byte records")
    cache_by_map: dict[int, tuple[int, int, int, int]] = {}
    for unit in range(len(mcache) // 16):
        map_id, output_size, state, stored_unit = unpack_from("<IIII", mcache, unit * 16)
        if map_id != 0xFFFFFFFF:
            cache_by_map[map_id] = (unit, output_size, state, stored_unit)

    _, safe_decompress, lzo_version = load_lzo()
    index_rows: list[tuple[object, ...]] = []
    map_rows: list[tuple[object, ...]] = []
    block_rows: list[tuple[object, ...]] = []
    branch_counts: dict[str, int] = defaultdict(int)
    kind_counts: dict[str, int] = defaultdict(int)
    kind_compressed: dict[str, int] = defaultdict(int)
    kind_decompressed: dict[str, int] = defaultdict(int)
    kind_written: dict[str, int] = defaultdict(int)
    kind_discarded: dict[str, int] = defaultdict(int)
    kind_hashes: dict[str, object] = defaultdict(hashlib.sha256)
    concatenated_output_hash = hashlib.sha256()
    concatenated_written_hash = hashlib.sha256()
    stream_ordinal = 0

    with LMF_PATH.open("rb") as input_file:
        data = mmap.mmap(input_file.fileno(), 0, access=mmap.ACCESS_READ)
        try:
            index_offset = unpack_from("<I", data, 0)[0]
            tail_size = len(data) - index_offset
            if tail_size % 16:
                raise SystemExit("LMF tail size is not divisible by 16")
            index_count = tail_size // 16
            if index_count != EXPECTED_INDEX_RECORDS:
                raise SystemExit(f"unexpected LMF index record count: {index_count}")

            maps: list[tuple[int, int, int, int]] = []
            for index_ordinal in range(index_count):
                record_offset = index_offset + index_ordinal * 16
                map_offset, map_span, map_id, reserved = unpack_from(
                    "<IIII", data, record_offset
                )
                is_sentinel = int(
                    map_offset == 0 and map_span == 0 and map_id == 0 and reserved == 0
                )
                index_rows.append(
                    (
                        index_ordinal,
                        f"0x{record_offset:X}",
                        map_offset,
                        f"0x{map_offset:X}",
                        map_span,
                        map_id,
                        reserved,
                        is_sentinel,
                    )
                )
                if not is_sentinel:
                    maps.append((index_ordinal, map_offset, map_span, map_id))

            if len(maps) != EXPECTED_MAPS:
                raise SystemExit(f"unexpected active map count: {len(maps)}")
            if len({map_id for _, _, _, map_id in maps}) != len(maps):
                raise SystemExit("LMF map identifiers are not unique")
            for map_index, (_, map_offset, map_span, _) in enumerate(maps):
                expected_next = (
                    maps[map_index + 1][1]
                    if map_index + 1 < len(maps)
                    else index_offset
                )
                if map_offset + map_span != expected_next:
                    raise SystemExit(
                        f"map index {map_index}: span does not reach next physical record"
                    )

            for index_ordinal, map_offset, map_span, map_id in maps:
                map_end = map_offset + map_span
                if data[map_offset : map_offset + 4] != b"MSF2":
                    raise SystemExit(f"map {map_id}: current package is not MSF2")
                header_offsets = unpack_from("<IIII", data, map_offset + 0x14)
                offset_14, offset_18, offset_1c, offset_20 = header_offsets
                if not (
                    0 <= offset_14 <= offset_18 <= offset_1c <= offset_20 < map_span
                ):
                    raise SystemExit(f"map {map_id}: unordered header offsets")
                dimensions = unpack_from("<5H", data, map_offset + 0x84)
                width, height, field_88, field_8a, layers = dimensions
                name_start = map_offset + 0x96
                name_zero = data.find(b"\0", name_start, min(map_end, map_offset + 0x20000))
                if name_zero < 0:
                    raise SystemExit(f"map {map_id}: unterminated header name")
                name_bytes = data[name_start:name_zero]
                name_text = name_bytes.decode("cp950", errors="replace")

                raw_table_offset = name_zero + 1
                raw_table_size = width * height * layers * 4 + 4
                surface_size_field = raw_table_offset + raw_table_size - 4
                surface_compressed_size = unpack_from("<I", data, surface_size_field)[0]
                surface_payload = raw_table_offset + raw_table_size
                surface_trailer = surface_payload + surface_compressed_size
                if surface_trailer + 4 > map_end:
                    raise SystemExit(f"map {map_id}: surface stream exceeds map span")
                post_surface_record_count = unpack_from("<I", data, surface_trailer)[0]
                surface_capacity = width * height * 4

                object_table = map_offset + offset_18
                object_count = unpack_from("<I", data, object_table)[0]
                object_streams: list[tuple[int, int, int, int]] = []
                for object_index in range(object_count):
                    relative_header = unpack_from(
                        "<I", data, object_table + 4 + object_index * 4
                    )[0]
                    object_header = map_offset + relative_header
                    object_capacity, object_compressed_size = unpack_from(
                        "<II", data, object_header + 0x12
                    )
                    object_payload = object_header + 0x1A
                    if object_payload + object_compressed_size > map_end:
                        raise SystemExit(
                            f"map {map_id} object {object_index}: stream exceeds map span"
                        )
                    object_streams.append(
                        (
                            object_index,
                            object_header,
                            object_payload,
                            object_compressed_size,
                            object_capacity,
                        )
                    )

                cm_header = map_offset + offset_20
                cm_total_output, cm_chunk_size = unpack_from("<II", data, cm_header + 0x10)
                if cm_chunk_size == 0:
                    raise SystemExit(f"map {map_id}: zero CM chunk size")
                cm_chunk_count = (cm_chunk_size + cm_total_output) // cm_chunk_size
                cm_payload = cm_header + 0x1A8
                cm_streams: list[tuple[int, int, int, int, int]] = []
                cm_cursor = cm_payload
                cm_remaining = cm_total_output
                for chunk_index in range(cm_chunk_count):
                    compressed_size = unpack_from(
                        "<I", data, cm_header + 0x1C + chunk_index * 8
                    )[0]
                    written_size = min(cm_remaining, cm_chunk_size)
                    if cm_cursor + compressed_size > map_end:
                        raise SystemExit(
                            f"map {map_id} CM chunk {chunk_index}: stream exceeds map span"
                        )
                    cm_streams.append(
                        (
                            chunk_index,
                            cm_header + 0x1C + chunk_index * 8,
                            cm_cursor,
                            compressed_size,
                            written_size,
                        )
                    )
                    cm_cursor += compressed_size
                    cm_remaining -= cm_chunk_size
                cm_trailing_bytes = map_end - cm_cursor
                if cm_trailing_bytes != 0x60:
                    raise SystemExit(
                        f"map {map_id}: CM streams leave {cm_trailing_bytes}, expected 0x60"
                    )

                cache = cache_by_map.get(map_id)
                cache_unit = "" if cache is None else cache[0]
                cache_declared_size = "" if cache is None else cache[1]
                cache_file_size: int | str = ""
                cache_size_matches: int | str = ""
                if cache is not None:
                    cache_path = WORKSPACE_ROOT / "Data" / f"{cache[0]}.cm"
                    cache_file_size = cache_path.stat().st_size
                    cache_size_matches = int(
                        cache[1] == cm_total_output == cache_file_size
                    )

                stream_specs: list[tuple[object, ...]] = [
                    (
                        "surface_grid",
                        0,
                        surface_size_field,
                        surface_payload,
                        surface_compressed_size,
                        surface_capacity,
                        surface_capacity,
                        "u32_compressed_before_payload,u32_post_payload_record_count",
                        "00426182",
                        "ignores_return_and_actual_output_size",
                        True,
                    )
                ]
                for object_index, object_header, object_payload, compressed_size, capacity in object_streams:
                    stream_specs.append(
                        (
                            "indexed_object",
                            object_index,
                            object_header,
                            object_payload,
                            compressed_size,
                            capacity,
                            capacity,
                            "u32_capacity_at_+0x12,u32_compressed_at_+0x16,payload_at_+0x1A",
                            "0042660E",
                            "ignores_return;passes_actual_output_size_to_00401B70",
                            False,
                        )
                    )
                cm_capacity = cm_chunk_size + cm_chunk_size // 1024
                for chunk_index, table_entry, payload, compressed_size, written_size in cm_streams:
                    stream_specs.append(
                        (
                            "cm_generation_chunk",
                            chunk_index,
                            table_entry,
                            payload,
                            compressed_size,
                            cm_capacity,
                            written_size,
                            "u32_compressed_in_8_byte_table_entry,sequential_payload",
                            "00426FDB",
                            "ignores_return_and_actual_output_size;writes_min_remaining_chunk_prefix",
                            False,
                        )
                    )

                for (
                    context,
                    context_index,
                    header_offset,
                    payload_offset,
                    compressed_size,
                    destination_capacity,
                    written_size,
                    header_layout,
                    callsite,
                    caller_policy,
                    require_exact_capacity,
                ) in stream_specs:
                    stream_ordinal += 1
                    source = data[payload_offset : payload_offset + compressed_size]
                    local_branch_counts: dict[str, int] = defaultdict(int)
                    assembly_output = decompress_sub_4399e0(
                        source,
                        destination_capacity,
                        local_branch_counts,
                        require_exact_output=require_exact_capacity,
                    )
                    lzo_result, lzo_output, lzo_output_size = library_decompress(
                        safe_decompress, source, destination_capacity
                    )
                    if lzo_result != 0 or lzo_output != assembly_output:
                        raise SystemExit(
                            f"map {map_id} {context} {context_index}: LZO mismatch "
                            f"rc={lzo_result}, output={lzo_output_size}/{len(assembly_output)}"
                        )
                    if context in ("surface_grid", "indexed_object"):
                        if len(assembly_output) != destination_capacity:
                            raise SystemExit(
                                f"map {map_id} {context}: current output does not fill allocation"
                            )
                    elif len(assembly_output) != cm_chunk_size:
                        raise SystemExit(
                            f"map {map_id} CM chunk {context_index}: output is not full chunk"
                        )
                    if written_size > len(assembly_output):
                        raise SystemExit(f"map {map_id} {context}: caller writes beyond output")

                    written_output = assembly_output[:written_size]
                    discarded_size = len(assembly_output) - written_size
                    for branch, count in local_branch_counts.items():
                        branch_counts[branch] += count
                    kind_counts[context] += 1
                    kind_compressed[context] += compressed_size
                    kind_decompressed[context] += len(assembly_output)
                    kind_written[context] += written_size
                    kind_discarded[context] += discarded_size
                    kind_hashes[context].update(assembly_output)
                    concatenated_output_hash.update(assembly_output)
                    concatenated_written_hash.update(written_output)
                    block_rows.append(
                        (
                            stream_ordinal,
                            index_ordinal,
                            map_id,
                            context,
                            context_index,
                            f"0x{header_offset:X}",
                            f"0x{payload_offset:X}",
                            header_layout,
                            compressed_size,
                            destination_capacity,
                            written_size,
                            len(assembly_output),
                            discarded_size,
                            callsite,
                            caller_policy,
                            0,
                            lzo_result,
                            int(lzo_output == assembly_output),
                            hashlib.sha256(assembly_output).hexdigest(),
                            hashlib.sha256(written_output).hexdigest(),
                            ";".join(
                                f"{branch}:{count}"
                                for branch, count in sorted(local_branch_counts.items())
                            ),
                        )
                    )

                map_rows.append(
                    (
                        index_ordinal,
                        map_id,
                        f"0x{map_offset:X}",
                        map_span,
                        "MSF2",
                        name_text,
                        name_bytes.hex(),
                        f"0x{offset_14:X}",
                        f"0x{offset_18:X}",
                        f"0x{offset_1c:X}",
                        f"0x{offset_20:X}",
                        width,
                        height,
                        field_88,
                        field_8a,
                        layers,
                        f"0x{raw_table_offset:X}",
                        raw_table_size,
                        surface_compressed_size,
                        surface_capacity,
                        post_surface_record_count,
                        object_count,
                        cm_total_output,
                        cm_chunk_size,
                        cm_chunk_count,
                        f"0x{cm_payload:X}",
                        f"0x{cm_cursor:X}",
                        cm_trailing_bytes,
                        cache_unit,
                        cache_declared_size,
                        cache_file_size,
                        cache_size_matches,
                    )
                )
        finally:
            data.close()

    if len(block_rows) != EXPECTED_STREAMS:
        raise SystemExit(f"unexpected LMF stream count: {len(block_rows)}")
    compressed_total = sum(kind_compressed.values())
    decompressed_total = sum(kind_decompressed.values())
    if compressed_total != EXPECTED_COMPRESSED_BYTES:
        raise SystemExit(f"unexpected compressed total: {compressed_total}")
    if decompressed_total != EXPECTED_DECOMPRESSED_BYTES:
        raise SystemExit(f"unexpected decompressed total: {decompressed_total}")
    if concatenated_output_hash.hexdigest() != EXPECTED_OUTPUT_SHA256:
        raise SystemExit(
            f"unexpected concatenated output hash: {concatenated_output_hash.hexdigest()}"
        )

    with INDEX_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "index_ordinal",
                "record_offset_hex",
                "map_offset",
                "map_offset_hex",
                "map_span",
                "map_id",
                "reserved_u32",
                "is_zero_sentinel",
            )
        )
        writer.writerows(index_rows)

    with MAP_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "index_ordinal",
                "map_id",
                "map_offset_hex",
                "map_span",
                "signature",
                "name_cp950",
                "name_raw_hex",
                "header_offset_14_hex",
                "header_offset_18_hex",
                "header_offset_1c_hex",
                "header_offset_20_hex",
                "dimension_84_width",
                "dimension_86_height",
                "field_88_u16",
                "field_8A_u16",
                "dimension_8C_layers",
                "raw_table_offset_hex",
                "raw_table_size",
                "surface_compressed_size",
                "surface_output_size",
                "post_surface_record_count",
                "indexed_object_stream_count",
                "cm_total_written_size",
                "cm_chunk_output_size",
                "cm_chunk_count",
                "cm_payload_offset_hex",
                "cm_payload_end_hex",
                "map_trailing_bytes_after_cm",
                "current_mcache_unit",
                "current_mcache_declared_size",
                "current_cm_file_size",
                "current_cache_sizes_match",
            )
        )
        writer.writerows(map_rows)

    with BLOCK_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "stream_ordinal",
                "map_index_ordinal",
                "map_id",
                "logical_context",
                "context_index",
                "container_header_offset_hex",
                "payload_offset_hex",
                "header_layout",
                "compressed_size",
                "original_destination_capacity",
                "caller_written_size",
                "actual_output_size",
                "discarded_output_size",
                "assembly_callsite",
                "original_caller_policy",
                "assembly_equivalent_return",
                "lzo1x_safe_return",
                "byte_exact_with_assembly",
                "full_output_sha256",
                "written_prefix_sha256",
                "assembly_branch_hits",
            )
        )
        writer.writerows(block_rows)

    covered_branches = sum(
        1 for branch, *_ in BRANCH_ROWS if branch_counts.get(branch, 0) > 0
    )
    with SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("metric", "value"))
        writer.writerow(("lmf_file_size", EXPECTED_LMF_SIZE))
        writer.writerow(("lmf_sha256", lmf_hash))
        writer.writerow(("tail_index_offset", index_rows[0][1] if index_rows else ""))
        writer.writerow(("tail_index_records", len(index_rows)))
        writer.writerow(("active_maps", len(map_rows)))
        writer.writerow(("zero_sentinels", sum(row[-1] for row in index_rows)))
        writer.writerow(("comparison_library_version", lzo_version))
        writer.writerow(("compressed_streams", len(block_rows)))
        writer.writerow(("assembly_exact_input_streams", len(block_rows)))
        writer.writerow(("lzo1x_safe_success_streams", len(block_rows)))
        writer.writerow(("byte_exact_streams", len(block_rows)))
        writer.writerow(("compressed_bytes", compressed_total))
        writer.writerow(("decompressed_bytes", decompressed_total))
        writer.writerow(("caller_written_bytes", sum(kind_written.values())))
        writer.writerow(("caller_discarded_output_bytes", sum(kind_discarded.values())))
        writer.writerow(("concatenated_full_output_sha256", concatenated_output_hash.hexdigest()))
        writer.writerow(("concatenated_written_prefix_sha256", concatenated_written_hash.hexdigest()))
        writer.writerow(("covered_branch_rows", covered_branches))
        writer.writerow(("total_branch_rows", len(BRANCH_ROWS)))
        for kind in sorted(kind_counts):
            writer.writerow((f"{kind}_streams", kind_counts[kind]))
            writer.writerow((f"{kind}_compressed_bytes", kind_compressed[kind]))
            writer.writerow((f"{kind}_decompressed_bytes", kind_decompressed[kind]))
            writer.writerow((f"{kind}_caller_written_bytes", kind_written[kind]))
            writer.writerow((f"{kind}_discarded_output_bytes", kind_discarded[kind]))
            writer.writerow((f"{kind}_concatenated_output_sha256", kind_hashes[kind].hexdigest()))
        for branch in sorted(branch_counts):
            writer.writerow((f"branch_{branch}_hits", branch_counts[branch]))

    print(
        f"{len(map_rows)} maps, {len(block_rows)} exact LZO streams: "
        f"{compressed_total} compressed -> {decompressed_total} output bytes; "
        f"discarded {sum(kind_discarded.values())} CM tail bytes"
    )


if __name__ == "__main__":
    main()
