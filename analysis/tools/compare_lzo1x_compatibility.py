#!/usr/bin/env python3
"""Compare sub_4399E0 valid streams with the host LZO1X 2.x safe decoder."""

from __future__ import annotations

import csv
import ctypes
import ctypes.util
import hashlib
from collections import defaultdict
from pathlib import Path

from verify_tsw_decompression import decompress_sub_4399e0


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FRAME_INPUT = INVENTORY_ROOT / "tsw-frame-descriptors.tsv"
BRANCH_OUTPUT = INVENTORY_ROOT / "lzo1x-branch-compatibility.tsv"
SUMMARY_OUTPUT = INVENTORY_ROOT / "lzo1x-compatibility-summary.tsv"
VECTOR_OUTPUT = INVENTORY_ROOT / "lzo1x-library-control-vectors.tsv"
VALID_VECTOR_OUTPUT = INVENTORY_ROOT / "lzo1x-valid-branch-vectors.tsv"

EXPECTED_FRAME_COUNT = 20091
EXPECTED_COMPRESSED_BYTES = 558351505
EXPECTED_DECOMPRESSED_BYTES = 1378998573

BRANCH_ROWS = (
    ("initial_literal_1_to_3", "0x004399FC-0x00439ACD", "first_byte-0x11", "initial literal then match-next", "same"),
    ("initial_literal_4_plus", "0x004399FC-0x00439A1E", "first_byte-0x11", "initial literal run then first-literal state", "same"),
    ("literal_run_short", "0x00439A20-0x00439A89", "token+3", "ordinary literal run", "same"),
    ("literal_run_extended", "0x00439A2E-0x00439A49", "255*zero_count+next+0x12", "zero-extended literal run", "same"),
    ("short_match_after_literal_len3", "0x00439A89-0x00439AC1", "3", "offset=0x801+4*next+(token>>2)", "same"),
    ("short_match_after_match_len2", "0x00439B13-0x00439AC1", "2", "offset=1+4*next+(token>>2)", "same"),
    ("match_40_to_ff", "0x00439ADB-0x00439B11", "(token>>5)+1", "one-byte short-offset match", "same"),
    ("match_20_to_3f_short", "0x00439B48-0x00439BCB", "(token&0x1f)+2", "two-byte medium-offset match", "same"),
    ("match_20_to_3f_extended", "0x00439B4D-0x00439BCB", "255*zero_count+next+0x21", "extended medium-offset match", "same"),
    ("match_10_to_1f_short", "0x00439B7F-0x00439BCB", "(token&7)+2", "two-range long-offset match or end marker", "same"),
    ("match_10_to_1f_extended", "0x00439B84-0x00439BCB", "255*zero_count+next+9", "extended long-offset match", "same"),
    ("end_marker", "0x00439BB0-0x00439B47", "0", "zero M4 offset terminates and compares input end", "same valid-stream marker"),
    ("trailing_literal_0", "0x00439AC1-0x00439A20", "0", "return to ordinary literal token", "same"),
    ("trailing_literal_1", "0x00439AC1-0x00439ADA", "1", "copy low-two-bit literal tail then next match", "same"),
    ("trailing_literal_2", "0x00439AC1-0x00439ADA", "2", "copy low-two-bit literal tail then next match", "same"),
    ("trailing_literal_3", "0x00439AC1-0x00439ADA", "3", "copy low-two-bit literal tail then next match", "same"),
)

CONTROL_VECTORS = (
    ("empty_exact_end_marker", bytes.fromhex("110000"), 16, 0, 0),
    ("end_marker_with_one_tail_byte", bytes.fromhex("11000058"), 16, -8, 0),
    ("truncated_end_marker", bytes.fromhex("1100"), 16, -4, 0),
    ("four_literals_output_capacity_three", bytes.fromhex("0141424344110000"), 3, -5, 0),
    ("lookbehind_before_output", bytes.fromhex("014142434440ff110000"), 64, -6, 4),
)


def valid_branch_vectors() -> tuple[tuple[str, bytes, int], ...]:
    long_literals = bytes((index * 37) & 0xFF for index in range(2050))
    return (
        ("initial_literal_1_to_3", bytes((0x12, 0x41, 0x11, 0, 0)), 1),
        ("initial_literal_4_plus", bytes((0x15, 0x41, 0x42, 0x43, 0x44, 0x11, 0, 0)), 4),
        ("short_match_after_match_len2", bytes((0x12, 0x41, 0, 0, 0x11, 0, 0)), 3),
        (
            "short_match_after_literal_len3",
            bytes((0,)) + bytes(7) + bytes((247,)) + long_literals + bytes((0, 0, 0x11, 0, 0)),
            2053,
        ),
    )


def load_lzo() -> tuple[ctypes.CDLL, object, str]:
    library_name = ctypes.util.find_library("lzo2")
    if library_name is None:
        raise SystemExit("liblzo2 was not found")
    library = ctypes.CDLL(library_name)
    library.lzo_version_string.restype = ctypes.c_char_p
    version = library.lzo_version_string().decode("ascii")
    function = library.lzo1x_decompress_safe
    function.argtypes = (
        ctypes.c_void_p,
        ctypes.c_size_t,
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_size_t),
        ctypes.c_void_p,
    )
    function.restype = ctypes.c_int
    return library, function, version


def library_decompress(function: object, source: bytes, capacity: int) -> tuple[int, bytes, int]:
    source_buffer = ctypes.create_string_buffer(source)
    destination_buffer = ctypes.create_string_buffer(max(1, capacity))
    output_size = ctypes.c_size_t(capacity)
    result = function(
        source_buffer,
        len(source),
        destination_buffer,
        ctypes.byref(output_size),
        None,
    )
    return result, destination_buffer.raw[: output_size.value], output_size.value


def main() -> None:
    _, safe_decompress, version = load_lzo()
    with FRAME_INPUT.open(encoding="utf-8", newline="") as input_file:
        frames = list(csv.DictReader(input_file, delimiter="\t"))
    if len(frames) != EXPECTED_FRAME_COUNT:
        raise SystemExit(f"unexpected TSW frame count: {len(frames)}")

    branch_counts: dict[str, int] = defaultdict(int)
    synthetic_branch_counts: dict[str, int] = defaultdict(int)
    archive_hashes: dict[str, object] = defaultdict(hashlib.sha256)
    exact_matches = 0
    compressed_bytes = 0
    decompressed_bytes = 0

    open_files = {
        archive: (WORKSPACE_ROOT / archive).open("rb")
        for archive in sorted({frame["archive"] for frame in frames})
    }
    try:
        for frame in frames:
            archive = frame["archive"]
            compressed_size = int(frame["compressed_size_bytes"])
            declared_size = int(frame["declared_decompressed_size_bytes"])
            absolute_offset = int(frame["payload_absolute_offset_hex"], 16)
            input_file = open_files[archive]
            input_file.seek(absolute_offset)
            compressed = input_file.read(compressed_size)
            if len(compressed) != compressed_size:
                raise SystemExit(f"short read: {archive} record {frame['record_ordinal']}")

            assembly_output = decompress_sub_4399e0(
                compressed, declared_size, branch_counts
            )
            result, library_output, output_size = library_decompress(
                safe_decompress, compressed, declared_size
            )
            if result != 0 or output_size != declared_size:
                raise SystemExit(
                    f"liblzo2 mismatch: {archive} record {frame['record_ordinal']} "
                    f"return {result}, output {output_size}/{declared_size}"
                )
            if library_output != assembly_output:
                raise SystemExit(
                    f"byte mismatch: {archive} record {frame['record_ordinal']}"
                )

            exact_matches += 1
            compressed_bytes += compressed_size
            decompressed_bytes += output_size
            archive_hashes[archive].update(library_output)
    finally:
        for input_file in open_files.values():
            input_file.close()

    if compressed_bytes != EXPECTED_COMPRESSED_BYTES:
        raise SystemExit(f"unexpected compressed byte total: {compressed_bytes}")
    if decompressed_bytes != EXPECTED_DECOMPRESSED_BYTES:
        raise SystemExit(f"unexpected decompressed byte total: {decompressed_bytes}")

    valid_vector_rows: list[tuple[object, ...]] = []
    for case, source, expected_size in valid_branch_vectors():
        vector_counts: dict[str, int] = defaultdict(int)
        assembly_output = decompress_sub_4399e0(source, expected_size, vector_counts)
        result, library_output, output_size = library_decompress(
            safe_decompress, source, expected_size
        )
        if result != 0 or output_size != expected_size or library_output != assembly_output:
            raise SystemExit(f"valid branch vector mismatch: {case}")
        for branch, count in vector_counts.items():
            synthetic_branch_counts[branch] += count
        valid_vector_rows.append(
            (
                case,
                len(source),
                expected_size,
                result,
                hashlib.sha256(assembly_output).hexdigest(),
                ";".join(f"{branch}:{count}" for branch, count in sorted(vector_counts.items())),
            )
        )

    with BRANCH_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("branch", "assembly_range", "logical_length", "state_or_offset", "lzo1x_2_10_relation", "tsw_hit_count", "synthetic_hit_count", "combined_covered"))
        for branch, address, length, state, relation in BRANCH_ROWS:
            writer.writerow((branch, address, length, state, relation, branch_counts[branch], synthetic_branch_counts[branch], int(branch_counts[branch] + synthetic_branch_counts[branch] > 0)))

    with VALID_VECTOR_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("case", "compressed_size", "expected_output_size", "lzo1x_safe_return", "output_sha256", "assembly_branch_hits"))
        writer.writerows(valid_vector_rows)

    with VECTOR_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("case", "input_hex", "destination_capacity", "lzo1x_safe_return", "actual_output_size"))
        for case, source, capacity, expected_result, expected_size in CONTROL_VECTORS:
            result, _, output_size = library_decompress(safe_decompress, source, capacity)
            if (result, output_size) != (expected_result, expected_size):
                raise SystemExit(
                    f"unexpected control-vector result for {case}: {result}, {output_size}"
                )
            writer.writerow((case, source.hex(), capacity, result, output_size))

    tsw_uncovered = [branch for branch, *_ in BRANCH_ROWS if branch_counts[branch] == 0]
    combined_uncovered = [
        branch
        for branch, *_ in BRANCH_ROWS
        if branch_counts[branch] + synthetic_branch_counts[branch] == 0
    ]
    with SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(("metric", "value"))
        writer.writerow(("comparison_api", "lzo1x_decompress_safe"))
        writer.writerow(("comparison_library_version", version))
        writer.writerow(("validated_tsw_frames", exact_matches))
        writer.writerow(("compressed_bytes", compressed_bytes))
        writer.writerow(("decompressed_bytes", decompressed_bytes))
        writer.writerow(("byte_exact_frame_matches", exact_matches))
        writer.writerow(("tsw_covered_branch_rows", len(BRANCH_ROWS) - len(tsw_uncovered)))
        writer.writerow(("total_branch_rows", len(BRANCH_ROWS)))
        writer.writerow(("tsw_uncovered_branch_rows", ";".join(tsw_uncovered)))
        writer.writerow(("combined_covered_branch_rows", len(BRANCH_ROWS) - len(combined_uncovered)))
        writer.writerow(("combined_uncovered_branch_rows", ";".join(combined_uncovered)))
        for archive in sorted(archive_hashes):
            writer.writerow((f"{archive}_concatenated_sha256", archive_hashes[archive].hexdigest()))

    print(
        f"liblzo2 {version}: {exact_matches} TSW frames byte-exact; "
        f"TSW covered {len(BRANCH_ROWS) - len(tsw_uncovered)}/{len(BRANCH_ROWS)}, "
        f"combined vectors covered {len(BRANCH_ROWS) - len(combined_uncovered)}/{len(BRANCH_ROWS)} branch rows"
    )


if __name__ == "__main__":
    main()
