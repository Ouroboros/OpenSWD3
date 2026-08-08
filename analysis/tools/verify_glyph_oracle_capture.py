#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import hashlib
from collections import Counter, defaultdict
from pathlib import Path


EXPECTED_EXE_SHA256 = (
    "78ddd0acf752dde32bbc4ea5a12256954878342899309c33516efd6dace0508a"
)
LEGACY_AGENT_SHA256S = {
    "fa045bc4b6621fcd6ae792d1cb89c72a134d59d5c9bdff17572a1db28db1a1f6",
}
EXPECTED_RENDERERS = {
    0x004A9ED0: (12, 12),
    0x004C9A28: (16, 16),
    0x004AB998: (20, 20),
}
CANONICAL_COUNTS = {
    (12, 12): 16,
    (16, 16): 90,
    (20, 20): 51,
}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def expected_agent_sha256s() -> set[str]:
    hashes = set(LEGACY_AGENT_SHA256S)
    current_agent = Path(__file__).with_name("glyph-oracle") / "agent.js"
    if current_agent.is_file():
        hashes.add(sha256_file(current_agent))
    return hashes


def read_table(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        return list(csv.DictReader(stream, delimiter="\t"))


def read_run_manifest(run_directory: Path) -> dict[str, str]:
    rows = read_table(run_directory / "run.tsv")
    manifest: dict[str, str] = {}
    for row in rows:
        key = row["field"]
        if key in manifest:
            raise RuntimeError(f"run.tsv 字段重复：{key}")
        manifest[key] = row["value"]
    return manifest


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def verify_run(
    run_directory: Path,
) -> tuple[
    dict[tuple[int, int, int], set[str]],
    Counter[tuple[int, int]],
    set[tuple[int, int]],
]:
    manifest = read_run_manifest(run_directory)
    require(manifest.get("glyph_entry") == "0x004368D0", "glyph 入口不匹配")
    require(manifest.get("glyph_return") == "0x00436974", "glyph 返回点不匹配")
    require(
        manifest.get("runtime_file.swd3_nodvd.exe.sha256")
        == EXPECTED_EXE_SHA256,
        "原版 EXE SHA-256 不匹配",
    )
    require(
        manifest.get("agent_sha256") in expected_agent_sha256s(),
        "Frida agent SHA-256 不匹配",
    )

    rows = read_table(run_directory / "glyph-masks.tsv")
    require(bool(rows), "glyph-masks.tsv 没有记录")
    indices = [int(row["index"]) for row in rows]
    require(indices == list(range(1, len(rows) + 1)), "捕获索引不连续")

    counts: Counter[tuple[int, int]] = Counter()
    hashes: dict[tuple[int, int, int], set[str]] = defaultdict(set)
    listed_files: set[Path] = set()
    cp950_invalid = 0
    spaces: set[tuple[int, int]] = set()

    for row in rows:
        renderer = int(row["renderer"], 16)
        width = int(row["width"])
        height = int(row["height"])
        row_bytes = int(row["row_bytes"])
        mask_bytes = int(row["mask_bytes"])
        consumed_bytes = int(row["consumed_bytes"])
        cache_key = int(row["cache_key"], 16)

        require(renderer in EXPECTED_RENDERERS, f"未知 renderer：0x{renderer:08X}")
        require(
            (width, height) == EXPECTED_RENDERERS[renderer],
            f"renderer 0x{renderer:08X} 的 geometry 不匹配",
        )
        require(consumed_bytes in (1, 2), "consumed_bytes 不是 1 或 2")
        require(row_bytes == (width + 7) // 8, "row_bytes 不匹配")
        require(mask_bytes == row_bytes * height, "mask_bytes 不匹配")

        raw_bytes = bytes.fromhex(row["raw_bytes"])
        require(len(raw_bytes) >= consumed_bytes + 1, "raw_bytes 缺少 NUL")
        require(raw_bytes[consumed_bytes] == 0, "原始字符未按捕获合同 NUL 终止")
        expected_key = raw_bytes[0]
        if consumed_bytes == 2:
            expected_key |= raw_bytes[1] << 8
        require(cache_key == expected_key, "cache key 与原始字节不一致")

        relative_file = Path(row["mask_file"])
        require(not relative_file.is_absolute(), "mask_file 不是相对路径")
        require(".." not in relative_file.parts, "mask_file 越出运行目录")
        mask_file = run_directory / relative_file
        require(mask_file.is_file(), f"mask 文件不存在：{relative_file}")
        listed_files.add(relative_file)

        mask = mask_file.read_bytes()
        require(len(mask) == mask_bytes, f"mask 文件长度错误：{relative_file}")
        actual_sha256 = hashlib.sha256(mask).hexdigest()
        require(actual_sha256 == row["sha256"].lower(), f"SHA-256 错误：{relative_file}")

        used_bits = width % 8
        if used_bits != 0:
            padding_mask = (1 << (8 - used_bits)) - 1
            for y in range(height):
                require(
                    mask[(y + 1) * row_bytes - 1] & padding_mask == 0,
                    f"行尾 padding bit 非零：{relative_file}",
                )

        character_bytes = raw_bytes[:consumed_bytes]
        try:
            character_bytes.decode("cp950")
        except UnicodeDecodeError:
            cp950_invalid += 1

        if character_bytes == b" ":
            require(not any(mask), f"空格 mask 非空：{relative_file}")
            spaces.add((width, height))

        counts[(width, height)] += 1
        hashes[(width, height, cache_key)].add(actual_sha256)

    actual_files = {
        path.relative_to(run_directory)
        for path in (run_directory / "masks").iterdir()
        if path.is_file()
    }
    require(actual_files == listed_files, "masks 目录与 glyph-masks.tsv 不一致")

    conflicts = [key for key, values in hashes.items() if len(values) != 1]
    if conflicts:
        raise RuntimeError(f"同一字形产生不同 mask：{conflicts[0]}")

    count_text = ", ".join(
        f"{width}x{height}={count}"
        for (width, height), count in sorted(counts.items())
    )
    print(
        f"{run_directory.name}: glyphs={len(rows)}; {count_text}; "
        f"cp950_invalid={cp950_invalid}; spaces={len(spaces)}"
    )
    font_geometries: set[tuple[int, int]] = set()
    font_manifest_path = run_directory / "font-selections.tsv"
    if font_manifest_path.is_file():
        font_rows = read_table(font_manifest_path)
        require(bool(font_rows), "font-selections.tsv 没有记录")
        font_indices = [int(row["index"]) for row in font_rows]
        require(
            font_indices == list(range(1, len(font_rows) + 1)),
            "字体选择索引不连续",
        )
        for row in font_rows:
            renderer = int(row["renderer"], 16)
            width = int(row["width"])
            height = int(row["height"])
            require(
                renderer in EXPECTED_RENDERERS,
                f"字体选择包含未知 renderer：0x{renderer:08X}",
            )
            require(
                (width, height) == EXPECTED_RENDERERS[renderer],
                "字体选择的 renderer geometry 不匹配",
            )
            require(bool(row["face_name"]), "GDI 实际 face 为空")
            for field in (
                "tm_height",
                "tm_ascent",
                "tm_descent",
                "tm_internal_leading",
                "tm_external_leading",
                "tm_average_char_width",
                "tm_maximum_char_width",
                "tm_weight",
                "tm_overhang",
                "tm_pitch_and_family",
                "tm_charset",
            ):
                int(row[field])
            font_geometries.add((width, height))
        faces = sorted({row["face_name"] for row in font_rows})
        print(f"selected_faces={faces}; font_geometries={sorted(font_geometries)}")

    return hashes, counts, font_geometries


def main() -> int:
    parser = argparse.ArgumentParser(description="校验原版 glyph-mask 捕获目录")
    parser.add_argument("run", type=Path, nargs="+")
    parser.add_argument(
        "--expect-archived-counts",
        action="store_true",
        help="要求最后一次运行包含当前基准的 16/90/51 个三字号样本",
    )
    parser.add_argument(
        "--require-font-selection",
        action="store_true",
        help="要求最后一次运行记录三个 renderer 的实际 GDI face",
    )
    arguments = parser.parse_args()

    verified = [verify_run(path.resolve()) for path in arguments.run]
    if arguments.expect_archived_counts:
        require(
            verified[-1][1] == CANONICAL_COUNTS,
            "当前基准的三字号数量不匹配",
        )
    if arguments.require_font_selection:
        require(
            verified[-1][2] == set(EXPECTED_RENDERERS.values()),
            "没有记录三个 renderer 的实际 GDI face",
        )

    if len(verified) > 1:
        verified_hashes = [result[0] for result in verified]
        common = set(verified_hashes[0])
        for current in verified_hashes[1:]:
            common &= set(current)
        for key in common:
            baseline = verified_hashes[0][key]
            require(
                all(
                    not baseline.isdisjoint(current[key])
                    for current in verified_hashes[1:]
                ),
                f"跨运行重复字形不一致：{key}",
            )
        print(f"cross_run_overlap={len(common)}; mismatches=0")

    print("glyph oracle verification: ok")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, KeyError, ValueError, RuntimeError) as error:
        print(f"glyph oracle verification: failed: {error}")
        raise SystemExit(1) from None
