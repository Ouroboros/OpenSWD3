#!/usr/bin/env python3
"""Validate the six ACT archives against the assembly-derived lookup format."""

from __future__ import annotations

import csv
import struct
from collections import Counter
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
ARCHIVE_OUTPUT = INVENTORY_ROOT / "act-archives.tsv"
ENTRY_OUTPUT = INVENTORY_ROOT / "act-archive-index.tsv"
COMMAND_OUTPUT = INVENTORY_ROOT / "act-command-words.tsv"
ANOMALY_OUTPUT = INVENTORY_ROOT / "act-stream-default-words.tsv"
REPROCESSED_OUTPUT = INVENTORY_ROOT / "act-reprocessed-parameter-words.tsv"

HEADER_SIZE = 0x1C
COUNT_OFFSET = 0x18
INDEX_RECORD_SIZE = 0x2C
NAME_SIZE = 0x14
GROUP_SPAN = 3000

ARCHIVES = (
    ("all_char.act", 0),
    ("all_item.act", 1),
    ("all_magic.act", 2),
    ("all_sys.act", 3),
    ("all_map1.act", 4),
    ("all_map2.act", 5),
)

EXPECTED_ENTRY_COUNTS = {
    "all_char.act": 414,
    "all_item.act": 479,
    "all_magic.act": 498,
    "all_sys.act": 73,
    "all_map1.act": 6,
    "all_map2.act": 30,
}
EXPECTED_TOTAL_ENTRIES = 1500
EXPECTED_NONEMPTY_VARIANTS = 4326
EXPECTED_DEFAULT_WORD_LOCATIONS = 52
EXPECTED_REPROCESSED_SECOND_PARAMETERS = 9209


@dataclass(frozen=True)
class Command:
    parameter_words: int
    field_effect: str


# Numeric words and parameter counts come from sub_4321E0.  ASCII is rendered
# from the two bytes in file order, not used to decide behavior.
COMMANDS = {
    0x0000: Command(0, "default/no-op word"),
    0x4145: Command(1, "write action+0x24"),
    0x4148: Command(0, "set action+0x18 mode bits to 0x08"),
    0x414D: Command(0, "set action+0x18 mode bits to 0x04"),
    0x414E: Command(0, "set action+0x18 mode bits to 0x2C"),
    0x4154: Command(1, "write action+0x5A"),
    0x4158: Command(1, "write action+0x76"),
    0x4159: Command(1, "write action+0x78"),
    0x4342: Command(2, "write action+0x68 and action+0x74"),
    0x4347: Command(2, "write action+0x66 and action+0x72"),
    0x434C: Command(0, "clear action+0x70/+0x72/+0x74"),
    0x4352: Command(2, "write action+0x64 and action+0x70"),
    0x4544: Command(0, "conditional stream wait/end marker"),
    0x464C: Command(7, "write action+0x7A..+0x86"),
    0x4753: Command(1, "set mode 0x14 and write low byte to action+0x8A"),
    0x4C44: Command(1, "set mode 0x10 and write low byte to action+0x62"),
    0x4E4F: Command(0, "clear action+0x18 bit 0"),
    0x4F32: Command(0, "rewind marker; set action+0x8C when action+0x44 is zero"),
    0x4F41: Command(1, "write action+0x50"),
    0x4F56: Command(0, "conditional rewind or reset stream cursor"),
    0x4F58: Command(1, "write action+0x5E"),
    0x4F59: Command(1, "write action+0x60"),
    0x5041: Command(1, "write action+0x4C and advance packed action+0x40 counter"),
    0x5145: Command(1, "write action+0x28"),
    0x5246: Command(1, "write action+0x4A"),
    0x524F: Command(1, "write action+0x4E"),
    0x5344: Command(1, "write action+0x46 and refresh action+0x44"),
    0x534D: Command(0, "set action+0x94 to one"),
    0x544E: Command(1, "write packed action+0x40"),
    0x5457: Command(1, "write low byte to action+0x88"),
    0x5649: Command(0, "set action+0x18 bit 0"),
    0x5748: Command(2, "write action+0x2C and action+0x30"),
    0x5756: Command(1, "write action+0x58"),
    0x5859: Command(2, "write action+0x10 and action+0x14"),
}

REPROCESSED_SECOND_PARAMETER_COMMANDS = {0x4342, 0x4347, 0x4352, 0x5748, 0x5859}


def read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def file_ascii(word: int) -> str:
    raw = bytes((word & 0xFF, word >> 8))
    return "".join(chr(byte) if 0x20 <= byte <= 0x7E else "." for byte in raw)


def decode_name(raw: bytes) -> str:
    raw = raw.split(b"\0", 1)[0]
    for encoding in ("cp950", "gbk"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue
    return ""


def parse_stream(
    stream: bytes,
) -> tuple[Counter[int], list[tuple[int, int]], list[tuple[int, int, int]]]:
    if len(stream) % 2:
        raise SystemExit(f"odd ACT stream length: {len(stream)}")
    words = struct.unpack(f"<{len(stream) // 2}H", stream)
    commands: Counter[int] = Counter()
    default_words: list[tuple[int, int]] = []
    reprocessed_words: list[tuple[int, int, int]] = []
    cursor = 0
    while cursor < len(words):
        word = words[cursor]
        cursor += 1
        command = COMMANDS.get(word)
        if command is None:
            default_words.append((cursor - 1, word))
            continue
        commands[word] += 1
        if cursor + command.parameter_words > len(words):
            raise SystemExit(f"truncated command 0x{word:04X}")
        if word in REPROCESSED_SECOND_PARAMETER_COMMANDS:
            reprocessed_words.append((word, cursor + 1, words[cursor + 1]))
        cursor += command.parameter_words
    return commands, default_words, reprocessed_words


def main() -> None:
    archive_rows: list[tuple[object, ...]] = []
    entry_rows: list[tuple[object, ...]] = []
    command_by_archive: dict[int, Counter[int]] = {}
    unknown_by_archive: dict[int, Counter[int]] = {}
    anomaly_rows: list[tuple[object, ...]] = []
    reprocessed_counts: Counter[tuple[int, int]] = Counter()
    reprocessed_by_archive: dict[int, Counter[tuple[int, int]]] = {}

    for archive_name, group_index in ARCHIVES:
        path = WORKSPACE_ROOT / archive_name
        data = path.read_bytes()
        if len(data) < HEADER_SIZE:
            raise SystemExit(f"short ACT archive: {archive_name}")
        entry_count = read_u32(data, COUNT_OFFSET)
        if entry_count != EXPECTED_ENTRY_COUNTS[archive_name]:
            raise SystemExit(f"unexpected ACT entry count in {archive_name}: {entry_count}")
        index_end = HEADER_SIZE + entry_count * INDEX_RECORD_SIZE
        if index_end > len(data):
            raise SystemExit(f"ACT index exceeds file: {archive_name}")

        archive_commands: Counter[int] = Counter()
        archive_unknown: Counter[int] = Counter()
        archive_reprocessed: Counter[tuple[int, int]] = Counter()
        total_variant_slots = 0
        nonempty_variants = 0
        total_stream_bytes = 0
        nonempty_entries = 0
        reserved_nonzero_entries = 0
        stored_id_mismatch_entries = 0

        for ordinal in range(1, entry_count + 1):
            record_offset = HEADER_SIZE + (ordinal - 1) * INDEX_RECORD_SIZE
            name_raw = data[record_offset : record_offset + NAME_SIZE]
            block_size, block_offset, action_id, reserved_0, reserved_1, reserved_2 = struct.unpack_from(
                "<IIIIII", data, record_offset + NAME_SIZE
            )
            expected_action_id = group_index * GROUP_SPAN + ordinal
            blank_record = not any(
                name_raw
                + struct.pack(
                    "<IIIIII",
                    block_size,
                    block_offset,
                    action_id,
                    reserved_0,
                    reserved_1,
                    reserved_2,
                )
            )
            if action_id != expected_action_id and not blank_record:
                stored_id_mismatch_entries += 1
            if any((reserved_0, reserved_1, reserved_2)):
                reserved_nonzero_entries += 1

            variant_count = 0
            entry_nonempty_variants = 0
            if block_size or block_offset:
                if not block_size or block_offset + block_size > len(data):
                    raise SystemExit(
                        f"invalid block range in {archive_name} record {ordinal}"
                    )
                nonempty_entries += 1
                block = data[block_offset : block_offset + block_size]
                variant_count = read_u16(block, 0)
                table_end = 2 + variant_count * 4
                if table_end > len(block):
                    raise SystemExit(
                        f"variant table exceeds block in {archive_name} record {ordinal}"
                    )
                offsets = [read_u32(block, 2 + index * 4) for index in range(variant_count)]
                nonzero_offsets = [offset for offset in offsets if offset]
                if any(offset < table_end or offset > len(block) for offset in nonzero_offsets):
                    raise SystemExit(
                        f"variant offset outside block in {archive_name} record {ordinal}"
                    )
                if any(right <= left for left, right in zip(nonzero_offsets, nonzero_offsets[1:])):
                    raise SystemExit(
                        f"variant offsets not strictly increasing in {archive_name} record {ordinal}"
                    )
                for variant_index, start in enumerate(offsets):
                    if not start:
                        continue
                    if variant_index == len(offsets) - 1:
                        end = len(block)
                    else:
                        end = next(
                            (candidate for candidate in offsets[variant_index + 1 :] if candidate),
                            0,
                        )
                        if not end:
                            raise SystemExit(
                                "assembly would scan beyond the variant table after trailing zero offsets "
                                f"in {archive_name} record {ordinal} variant {variant_index}"
                            )
                    if end <= start:
                        raise SystemExit(
                            f"empty/reversed stream in {archive_name} record {ordinal}"
                        )
                    stream = block[start:end]
                    commands, default_words, reprocessed_words = parse_stream(stream)
                    archive_commands.update(commands)
                    archive_unknown.update(word for _, word in default_words)
                    for word_index, word in default_words:
                        anomaly_rows.append(
                            (
                                archive_name,
                                expected_action_id,
                                action_id,
                                variant_index,
                                word_index,
                                f"{word:04X}",
                                file_ascii(word),
                                f"{block_offset + start + word_index * 2:08X}",
                            )
                        )
                    for source_command, _, word in reprocessed_words:
                        archive_reprocessed[(source_command, word)] += 1
                        reprocessed_counts[(source_command, word)] += 1
                    entry_nonempty_variants += 1
                    total_stream_bytes += len(stream)

            total_variant_slots += variant_count
            nonempty_variants += entry_nonempty_variants
            entry_rows.append(
                (
                    archive_name,
                    group_index,
                    ordinal,
                    expected_action_id,
                    action_id,
                    name_raw.hex().upper(),
                    decode_name(name_raw),
                    f"{record_offset:08X}",
                    f"{block_offset:08X}",
                    block_size,
                    variant_count,
                    entry_nonempty_variants,
                    f"{reserved_0:08X}",
                    f"{reserved_1:08X}",
                    f"{reserved_2:08X}",
                )
            )

        command_by_archive[group_index] = archive_commands
        unknown_by_archive[group_index] = archive_unknown
        reprocessed_by_archive[group_index] = archive_reprocessed
        reprocessed_total = sum(archive_reprocessed.values())
        reprocessed_command_collisions = sum(
            count
            for (_, word), count in archive_reprocessed.items()
            if word in COMMANDS and word != 0
        )
        archive_rows.append(
            (
                archive_name,
                group_index,
                len(data),
                entry_count,
                f"{HEADER_SIZE:08X}",
                f"{index_end:08X}",
                nonempty_entries,
                total_variant_slots,
                nonempty_variants,
                total_stream_bytes,
                sum(count for word, count in archive_commands.items() if word != 0),
                archive_commands.get(0, 0),
                sum(archive_unknown.values()),
                reserved_nonzero_entries,
                stored_id_mismatch_entries,
                reprocessed_total,
                reprocessed_command_collisions,
            )
        )

    if len(entry_rows) != EXPECTED_TOTAL_ENTRIES:
        raise SystemExit(f"unexpected total ACT index entries: {len(entry_rows)}")
    if sum(row[8] for row in archive_rows) != EXPECTED_NONEMPTY_VARIANTS:
        raise SystemExit("unexpected total nonempty ACT variants")
    if len(anomaly_rows) != EXPECTED_DEFAULT_WORD_LOCATIONS:
        raise SystemExit(f"unexpected ACT default-word locations: {len(anomaly_rows)}")
    if sum(reprocessed_counts.values()) != EXPECTED_REPROCESSED_SECOND_PARAMETERS:
        raise SystemExit("unexpected reprocessed ACT second-parameter count")
    if any(row[16] for row in archive_rows):
        raise SystemExit("an ACT reprocessed parameter now collides with a recognized command")

    INVENTORY_ROOT.mkdir(parents=True, exist_ok=True)
    with ARCHIVE_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "group_index",
                "file_size_bytes",
                "index_entry_count",
                "index_start_hex",
                "index_end_hex",
                "nonempty_entries",
                "variant_slots",
                "nonempty_variants",
                "action_stream_bytes",
                "recognized_command_occurrences",
                "zero_default_word_occurrences",
                "other_default_word_occurrences",
                "reserved_nonzero_entries",
                "stored_id_mismatch_entries",
                "reprocessed_second_parameter_occurrences",
                "reprocessed_recognized_command_collisions",
            )
        )
        writer.writerows(archive_rows)

    with ENTRY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "group_index",
                "record_ordinal",
                "lookup_action_id",
                "stored_action_id",
                "name_bytes_hex",
                "name_text_best_effort",
                "index_record_offset_hex",
                "block_offset_hex",
                "block_size_bytes",
                "variant_slots",
                "nonempty_variants",
                "reserved_0_hex",
                "reserved_1_hex",
                "reserved_2_hex",
            )
        )
        writer.writerows(entry_rows)

    all_commands: Counter[int] = Counter()
    all_unknown: Counter[int] = Counter()
    for counter in command_by_archive.values():
        all_commands.update(counter)
    for counter in unknown_by_archive.values():
        all_unknown.update(counter)

    with COMMAND_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "command_word_hex",
                "file_bytes_ascii",
                "parameter_words",
                "action_field_effect",
                *(f"{name}_occurrences" for name, _ in ARCHIVES),
                "total_occurrences",
                "status",
            )
        )
        for word in sorted(set(COMMANDS) | set(all_unknown)):
            command = COMMANDS.get(word)
            writer.writerow(
                (
                    f"{word:04X}",
                    file_ascii(word),
                    "" if command is None else command.parameter_words,
                    "" if command is None else command.field_effect,
                    *(command_by_archive[group].get(word, 0) for _, group in ARCHIVES),
                    all_commands.get(word, 0) + all_unknown.get(word, 0),
                    (
                        "assembly-default-zero"
                        if word == 0
                        else "assembly-recognized"
                        if command is not None
                        else "assembly-default-word"
                    ),
                )
            )

    with ANOMALY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "lookup_action_id",
                "stored_action_id",
                "variant_index",
                "stream_word_index",
                "word_hex",
                "file_bytes_ascii",
                "absolute_file_offset_hex",
            )
        )
        writer.writerows(anomaly_rows)

    with REPROCESSED_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "source_command_word_hex",
                "source_file_bytes_ascii",
                "reprocessed_word_hex",
                "reprocessed_file_bytes_ascii",
                *(f"{name}_occurrences" for name, _ in ARCHIVES),
                "total_occurrences",
                "runtime_dispatch",
                "runtime_effect_if_recognized",
            )
        )
        for source_command, word in sorted(reprocessed_counts):
            target = COMMANDS.get(word)
            recognized = target is not None and word != 0
            writer.writerow(
                (
                    f"{source_command:04X}",
                    file_ascii(source_command),
                    f"{word:04X}",
                    file_ascii(word),
                    *(
                        reprocessed_by_archive[group].get((source_command, word), 0)
                        for _, group in ARCHIVES
                    ),
                    reprocessed_counts[(source_command, word)],
                    "recognized-command" if recognized else "default-path",
                    target.field_effect if recognized and target is not None else "",
                )
            )

    print(
        f"wrote {len(archive_rows)} archives, {len(entry_rows)} index entries, "
        f"{sum(row[8] for row in archive_rows)} nonempty variants, "
        f"{len(set(COMMANDS) | set(all_unknown))} command/default words, "
        f"{len(anomaly_rows)} default-word locations, and "
        f"{sum(reprocessed_counts.values())} reprocessed second parameters"
    )


if __name__ == "__main__":
    main()
