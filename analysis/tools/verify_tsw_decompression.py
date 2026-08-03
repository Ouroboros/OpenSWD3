#!/usr/bin/env python3
"""Validate every TSW primary payload against the sub_4399E0 control flow."""

from __future__ import annotations

import csv
import hashlib
from collections import defaultdict
from collections.abc import MutableMapping
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FRAME_INPUT = INVENTORY_ROOT / "tsw-frame-descriptors.tsv"
FRAME_OUTPUT = INVENTORY_ROOT / "tsw-decompression-validation.tsv"
ARCHIVE_OUTPUT = INVENTORY_ROOT / "tsw-decompression-summary.tsv"

EXPECTED_FRAME_COUNT = 20091
EXPECTED_COMPRESSED_BYTES = 558351505
EXPECTED_DECOMPRESSED_BYTES = 1378998573
EXPECTED_ARCHIVE_HASHES = {
    "all_char.tsw": "c6c401cfdd33d047ad16afb0e3af047f5f24a203a76fe2b0679c70e07235fff3",
    "all_item.tsw": "27a624cd08659b0723839b8b4ae8673192346fea8455aca0cdd631c2b76c2402",
    "all_magic.tsw": "0c797e28826a8da9cd18dcf265126b6b9ff86726520f509a07237dc89b8b59b1",
    "all_map1.tsw": "888680220725bee08718b6203c8f63cad8f9ed84c09e6e4dc913e80bca101934",
    "all_map2.tsw": "79c8f249ce2fb2a17e677f05078f88b70736ada3b554eec05bc5c8a4e453f1f8",
    "all_sys.tsw": "45a68c036ed196e91414502c24b1ea12f826832c0a25e98929f7bb8628a27c17",
}


class DecompressionError(RuntimeError):
    pass


def decompress_sub_4399e0(
    source: bytes,
    expected_size: int,
    branch_counts: MutableMapping[str, int] | None = None,
    require_exact_output: bool = True,
) -> bytes:
    """Safe transliteration of the assembly's valid-stream behavior.

    The original routine has no destination-capacity argument.  This validator
    treats expected_size as a research-side capacity and, by default, an exact
    output assertion.  Callers for containers without a declared output length
    can disable only the exact-size assertion.  Token decoding, copy lengths,
    offsets, end marker, and exact-input return rule follow the assembly.
    """

    ip = 0
    output = bytearray()

    def hit(branch: str) -> None:
        if branch_counts is not None:
            branch_counts[branch] = branch_counts.get(branch, 0) + 1

    def read_byte() -> int:
        nonlocal ip
        if ip >= len(source):
            raise DecompressionError("compressed input exhausted")
        value = source[ip]
        ip += 1
        return value

    def read_u16() -> int:
        nonlocal ip
        if ip + 2 > len(source):
            raise DecompressionError("compressed 16-bit offset exhausted")
        value = source[ip] | (source[ip + 1] << 8)
        ip += 2
        return value

    def copy_literals(count: int) -> None:
        nonlocal ip
        if count < 0 or ip + count > len(source):
            raise DecompressionError("literal run exceeds compressed input")
        if len(output) + count > expected_size:
            raise DecompressionError("literal run exceeds declared output")
        output.extend(source[ip : ip + count])
        ip += count

    def copy_match(match_position: int, count: int) -> None:
        if count <= 0:
            raise DecompressionError("nonpositive match length")
        output_position = len(output)
        if match_position < 0 or match_position >= output_position:
            raise DecompressionError("match points before output or at current output")
        if output_position + count > expected_size:
            raise DecompressionError("match exceeds declared output")
        distance = output_position - match_position
        pattern = bytes(output[match_position:output_position])
        if count <= distance:
            output.extend(pattern[:count])
        else:
            repeats = (count + distance - 1) // distance
            output.extend((pattern * repeats)[:count])

    def extended_count(base: int) -> int:
        count = 0
        while True:
            value = read_byte()
            if value:
                return count + value + base
            count += 0xFF

    if not source:
        raise DecompressionError("empty compressed input")

    first = source[0]
    if first > 0x11:
        ip = 1
        initial_literals = first - 0x11
        if initial_literals < 4:
            hit("initial_literal_1_to_3")
            copy_literals(initial_literals)
            token = read_byte()
            state = "match"
        else:
            hit("initial_literal_4_plus")
            copy_literals(initial_literals)
            token = read_byte()
            if token < 0x10:
                hit("short_match_after_literal_len3")
                match_position = (
                    len(output)
                    - 4 * read_byte()
                    - (token >> 2)
                    - 0x801
                )
                copy_match(match_position, 3)
                state = "match_done"
            else:
                state = "match"
    else:
        state = "literal"
        token = 0

    while True:
        if state == "literal":
            token = read_byte()
            if token < 0x10:
                literal_token = token
                if literal_token == 0:
                    hit("literal_run_extended")
                    literal_token = extended_count(0x0F)
                else:
                    hit("literal_run_short")
                copy_literals(literal_token + 3)
                token = read_byte()
                if token < 0x10:
                    hit("short_match_after_literal_len3")
                    match_position = (
                        len(output)
                        - 4 * read_byte()
                        - (token >> 2)
                        - 0x801
                    )
                    copy_match(match_position, 3)
                    state = "match_done"
                    continue
            state = "match"

        if state == "match":
            if token >= 0x40:
                hit("match_40_to_ff")
                match_position = (
                    len(output)
                    - 8 * read_byte()
                    - 1
                    - ((token >> 2) & 7)
                )
                copy_match(match_position, (token >> 5) + 1)
            elif token >= 0x20:
                match_length = token & 0x1F
                if match_length == 0:
                    hit("match_20_to_3f_extended")
                    match_length = extended_count(0x1F)
                else:
                    hit("match_20_to_3f_short")
                match_position = len(output) - (read_u16() >> 2) - 1
                copy_match(match_position, match_length + 2)
            elif token < 0x10:
                hit("short_match_after_match_len2")
                match_position = (
                    len(output) - 4 * read_byte() - (token >> 2) - 1
                )
                copy_match(match_position, 2)
            else:
                match_base = len(output) - (0x4000 if token & 8 else 0)
                match_length = token & 7
                if match_length == 0:
                    hit("match_10_to_1f_extended")
                    match_length = extended_count(7)
                else:
                    hit("match_10_to_1f_short")
                match_position = match_base - (read_u16() >> 2)
                if match_position == len(output):
                    hit("end_marker")
                    if ip != len(source):
                        raise DecompressionError(
                            f"end marker leaves {len(source) - ip} compressed bytes"
                        )
                    if require_exact_output and len(output) != expected_size:
                        raise DecompressionError(
                            f"output size {len(output)} != declared {expected_size}"
                        )
                    return bytes(output)
                match_position -= 0x4000
                copy_match(match_position, match_length + 2)
            state = "match_done"

        if state == "match_done":
            if ip < 2:
                raise DecompressionError("match completion lacks offset byte")
            trailing_literals = source[ip - 2] & 3
            if trailing_literals == 0:
                hit("trailing_literal_0")
                state = "literal"
                continue
            hit(f"trailing_literal_{trailing_literals}")
            copy_literals(trailing_literals)
            token = read_byte()
            state = "match"


def main() -> None:
    with FRAME_INPUT.open(encoding="utf-8", newline="") as input_file:
        frames = list(csv.DictReader(input_file, delimiter="\t"))
    if len(frames) != EXPECTED_FRAME_COUNT:
        raise SystemExit(f"unexpected TSW frame count: {len(frames)}")

    validation_rows: list[tuple[object, ...]] = []
    archive_hashes: dict[str, object] = defaultdict(hashlib.sha256)
    archive_counts: dict[str, int] = defaultdict(int)
    archive_compressed: dict[str, int] = defaultdict(int)
    archive_decompressed: dict[str, int] = defaultdict(int)
    maximum_compressed: dict[str, int] = defaultdict(int)
    maximum_decompressed: dict[str, int] = defaultdict(int)

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
                raise SystemExit(
                    f"{archive} resource {frame['lookup_resource_id']} variant "
                    f"{frame['variant_index']}: short compressed read"
                )
            try:
                decompressed = decompress_sub_4399e0(compressed, declared_size)
            except DecompressionError as error:
                raise SystemExit(
                    f"{archive} resource {frame['lookup_resource_id']} variant "
                    f"{frame['variant_index']}: {error}"
                ) from error

            digest = hashlib.sha256(decompressed).hexdigest()
            archive_hashes[archive].update(decompressed)
            archive_counts[archive] += 1
            archive_compressed[archive] += compressed_size
            archive_decompressed[archive] += len(decompressed)
            maximum_compressed[archive] = max(
                maximum_compressed[archive], compressed_size
            )
            maximum_decompressed[archive] = max(
                maximum_decompressed[archive], len(decompressed)
            )
            validation_rows.append(
                (
                    archive,
                    frame["record_ordinal"],
                    frame["lookup_resource_id"],
                    frame["variant_index"],
                    frame["payload_absolute_offset_hex"],
                    compressed_size,
                    declared_size,
                    len(decompressed),
                    1,
                    digest,
                )
            )
    finally:
        for input_file in open_files.values():
            input_file.close()

    total_compressed = sum(archive_compressed.values())
    total_decompressed = sum(archive_decompressed.values())
    if total_compressed != EXPECTED_COMPRESSED_BYTES:
        raise SystemExit(f"unexpected validated compressed bytes: {total_compressed}")
    if total_decompressed != EXPECTED_DECOMPRESSED_BYTES:
        raise SystemExit(f"unexpected validated decompressed bytes: {total_decompressed}")
    observed_hashes = {
        archive: digest.hexdigest() for archive, digest in archive_hashes.items()
    }
    if observed_hashes != EXPECTED_ARCHIVE_HASHES:
        raise SystemExit(
            f"unexpected concatenated TSW decompressed hashes: {observed_hashes}"
        )

    with FRAME_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "record_ordinal",
                "lookup_resource_id",
                "variant_index",
                "payload_absolute_offset_hex",
                "compressed_size_bytes",
                "declared_decompressed_size_bytes",
                "actual_decompressed_size_bytes",
                "input_consumed_exactly",
                "decompressed_sha256",
            )
        )
        writer.writerows(validation_rows)

    with ARCHIVE_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "validated_frame_count",
                "compressed_bytes",
                "decompressed_bytes",
                "maximum_frame_compressed_bytes",
                "maximum_frame_decompressed_bytes",
                "concatenated_decompressed_sha256",
            )
        )
        for archive in sorted(archive_counts):
            writer.writerow(
                (
                    archive,
                    archive_counts[archive],
                    archive_compressed[archive],
                    archive_decompressed[archive],
                    maximum_compressed[archive],
                    maximum_decompressed[archive],
                    archive_hashes[archive].hexdigest(),
                )
            )

    print(
        f"validated {len(validation_rows)} TSW frames: {total_compressed} compressed "
        f"bytes -> {total_decompressed} bytes, all with exact input consumption"
    )


if __name__ == "__main__":
    main()
