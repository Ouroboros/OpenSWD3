#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import hashlib
import struct
from collections import Counter
from pathlib import Path


MAGIC = b"OSW3GLYF"
VERSION = 1
HEADER_SIZE = 80
KEY_COUNT = 32_896
SECTION_COUNT = 3
GEOMETRIES = ((12, 12), (16, 16), (20, 20))
CANONICAL_COUNTS = Counter({(12, 12): 16, (16, 16): 90, (20, 20): 51})
BASE_HEADER = struct.Struct("<8s6I")
SECTION_HEADER = struct.Struct("<4H2I")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def read_table(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def key_index(first_byte: int, second_byte: int) -> int:
    if first_byte < 0x80:
        return first_byte
    return 128 + (first_byte - 0x80) * 256 + second_byte


def parse_atlas(atlas: bytes) -> dict[tuple[int, int], tuple[int, int]]:
    require(len(atlas) >= HEADER_SIZE, "atlas 小于固定文件头")
    (
        magic,
        version,
        header_size,
        key_count,
        section_count,
        reserved0,
        reserved1,
    ) = BASE_HEADER.unpack_from(atlas)
    require(magic == MAGIC, "atlas magic 不匹配")
    require(version == VERSION, "atlas version 不匹配")
    require(header_size == HEADER_SIZE, "atlas header_size 不匹配")
    require(key_count == KEY_COUNT, "atlas key_count 不匹配")
    require(section_count == SECTION_COUNT, "atlas section_count 不匹配")
    require(reserved0 == 0 and reserved1 == 0, "atlas 保留字段非零")

    sections: dict[tuple[int, int], tuple[int, int]] = {}
    expected_offset = HEADER_SIZE
    for index, expected_geometry in enumerate(GEOMETRIES):
        descriptor_offset = BASE_HEADER.size + index * SECTION_HEADER.size
        width, height, row_bytes, mask_bytes, data_offset, data_size = (
            SECTION_HEADER.unpack_from(atlas, descriptor_offset)
        )
        expected_row_bytes = (expected_geometry[0] + 7) // 8
        expected_mask_bytes = expected_row_bytes * expected_geometry[1]
        expected_data_size = KEY_COUNT * expected_mask_bytes
        require((width, height) == expected_geometry, "atlas geometry 顺序不匹配")
        require(row_bytes == expected_row_bytes, "atlas row_bytes 不匹配")
        require(mask_bytes == expected_mask_bytes, "atlas mask_bytes 不匹配")
        require(data_offset == expected_offset, "atlas section offset 不连续")
        require(data_size == expected_data_size, "atlas section size 不匹配")
        require(data_offset + data_size <= len(atlas), "atlas section 越出文件")
        sections[(width, height)] = (data_offset, mask_bytes)
        expected_offset += data_size

    require(expected_offset == len(atlas), "atlas 文件长度不匹配")
    return sections


def compare_oracle(
    atlas: bytes,
    sections: dict[tuple[int, int], tuple[int, int]],
    oracle_directory: Path,
) -> Counter[tuple[int, int]]:
    rows = read_table(oracle_directory / "glyph-masks.tsv")
    require(bool(rows), "oracle 没有 glyph 记录")
    counts: Counter[tuple[int, int]] = Counter()

    for row in rows:
        geometry = (int(row["width"]), int(row["height"]))
        require(geometry in sections, f"atlas 缺少 geometry：{geometry}")
        consumed_bytes = int(row["consumed_bytes"])
        raw_bytes = bytes.fromhex(row["raw_bytes"])
        require(consumed_bytes in (1, 2), "oracle consumed_bytes 无效")
        require(len(raw_bytes) >= consumed_bytes + 1, "oracle raw_bytes 不完整")
        require(raw_bytes[consumed_bytes] == 0, "oracle raw_bytes 缺少 NUL")

        first_byte = raw_bytes[0]
        second_byte = raw_bytes[1] if consumed_bytes == 2 else 0
        expected_key = first_byte | (second_byte << 8)
        require(int(row["cache_key"], 16) == expected_key, "oracle cache key 不匹配")

        section_offset, mask_bytes = sections[geometry]
        require(int(row["mask_bytes"]) == mask_bytes, "oracle mask_bytes 不匹配")
        mask_offset = section_offset + key_index(first_byte, second_byte) * mask_bytes
        actual = atlas[mask_offset : mask_offset + mask_bytes]
        expected = (oracle_directory / row["mask_file"]).read_bytes()
        if actual != expected:
            differing_bits = sum(
                bin(left ^ right).count("1")
                for left, right in zip(actual, expected)
            )
            raise RuntimeError(
                f"atlas/oracle 不一致：{geometry[0]}x{geometry[1]} "
                f"key=0x{expected_key:04X} differing_bits={differing_bits}"
            )
        counts[geometry] += 1

    return counts


def main() -> int:
    parser = argparse.ArgumentParser(
        description="校验 legacy glyph atlas 格式并与原版动态 oracle 逐字节比较"
    )
    parser.add_argument("atlas", type=Path)
    parser.add_argument("oracle", type=Path)
    parser.add_argument(
        "--expect-canonical",
        action="store_true",
        help="要求 oracle 为当前 16/90/51、共 157 个 mask 的唯一基准",
    )
    arguments = parser.parse_args()

    atlas = arguments.atlas.read_bytes()
    sections = parse_atlas(atlas)
    counts = compare_oracle(atlas, sections, arguments.oracle.resolve())
    if arguments.expect_canonical:
        require(counts == CANONICAL_COUNTS, "oracle 不是当前唯一基准")

    count_text = ", ".join(
        f"{width}x{height}={count}"
        for (width, height), count in sorted(counts.items())
    )
    print(
        f"atlas_bytes={len(atlas)}; sha256={hashlib.sha256(atlas).hexdigest()}"
    )
    print(f"oracle_exact={sum(counts.values())}/{sum(counts.values())}; {count_text}")
    print("legacy glyph atlas verification: ok")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, KeyError, ValueError, RuntimeError, struct.error) as error:
        print(f"legacy glyph atlas verification: failed: {error}")
        raise SystemExit(1) from None
