#!/usr/bin/env python3
"""Probe indexed TALK streams with assembly-derived opcode length rules.

This is a physical linear probe, not a runtime VM simulation.  It validates
that the recovered encodings can walk bytes beginning at every sample index.
When an opcode performs an unconditional cross-window transfer, the probe
records the instruction footprint and stops that entry instead of pretending
the following file bytes execute sequentially.
"""

from __future__ import annotations

import csv
import hashlib
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
RULE_INPUT = INVENTORY_ROOT / "story-vm-opcode-length-rules.tsv"

ENTRY_OUTPUT = INVENTORY_ROOT / "story-vm-talk-linear-entry-probes.tsv"
RECORD_OUTPUT = INVENTORY_ROOT / "story-vm-talk-linear-records.tsv"
COVERAGE_OUTPUT = INVENTORY_ROOT / "story-vm-talk-opcode-coverage.tsv"

TALK_FILES = {
    "TALK1.DAT": (1202, "e85520a8158ec9d01364f3b00dde7965f3dd07c5d34829f380ce8d446cf38b6f"),
    "TALK2.DAT": (701, "cfe8059f8899eb5bd255885510e4b26fb3ec35793fb386b270e1b9e97dd5880e"),
    "TALK3.DAT": (2000, "369d5c03b63957c4c240a465d5cb9dba8dc7290a36ad8a480f28d41af9398959"),
    "TALK4.DAT": (1101, "5f6cf8d31d7b25fba78797b28c64f33f3c1ffbdc16b70930c083140bd90929a7"),
}

PAYLOAD_BASE = 0x200
MAX_STEPS = 65536
UNCONDITIONAL_TRANSFER = {15, 41, 87, 161}


@dataclass(frozen=True)
class Rule:
    opcode: int
    encoding: str
    fixed_length: int | None


@dataclass(frozen=True)
class Record:
    file_name: str
    file_offset: int
    payload_offset: int
    raw_word: int
    opcode: int
    length: int
    encoding: str
    detail: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def load_rules() -> dict[int, Rule]:
    with RULE_INPUT.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    rules = {
        int(row["effective_opcode"]): Rule(
            int(row["effective_opcode"]),
            row["encoding_class"],
            int(row["normal_fixed_length"]) if row["normal_fixed_length"] else None,
        )
        for row in rows
    }
    if len(rows) != 198 or len(rules) != 198:
        raise SystemExit("opcode length rule inventory is incomplete")
    return rules


def require(data: bytes, offset: int, size: int) -> None:
    if offset < 0 or size < 0 or offset + size > len(data):
        raise ValueError(f"file bounds: offset=0x{offset:X}, size=0x{size:X}")


def scan_word(data: bytes, offset: int, sentinel: int) -> int:
    count = 0
    while True:
        require(data, offset + count * 2, 2)
        if u16(data, offset + count * 2) == sentinel:
            return count
        count += 1


def scan_dword(data: bytes, offset: int, sentinel: int) -> int:
    count = 0
    while True:
        require(data, offset + count * 4, 4)
        if u32(data, offset + count * 4) == sentinel:
            return count
        count += 1


def scan_percent_q(data: bytes, offset: int) -> int:
    position = data.find(b"%Q", offset)
    if position < 0:
        raise ValueError(f"missing byte terminator 25 51 after 0x{offset:X}")
    return position - offset


def decode_record(file_name: str, data: bytes, offset: int, rules: dict[int, Rule]) -> Record:
    require(data, offset, 2)
    raw_word = u16(data, offset)
    opcode = raw_word & 0x3FFF
    rule = rules.get(opcode)
    if rule is None:
        raise ValueError(f"unmapped effective opcode {opcode}")

    encoding = rule.encoding
    detail = ""
    if rule.fixed_length is not None:
        length = rule.fixed_length
    elif encoding in {
        "byte_string_percent_q",
        "byte_string_percent_q_external",
        "byte_string_percent_q_with_prefix_flags",
        "mode_text_percent_q",
    }:
        count = scan_percent_q(data, offset + 2)
        length = 4 + count
        detail = f"byte_count={count}"
    elif encoding == "u16_prefix_then_byte_string_percent_q":
        count = scan_percent_q(data, offset + 4)
        length = 6 + count
        detail = f"byte_count={count}"
    elif encoding in {"counted_records_6byte", "counted_records_12byte"}:
        require(data, offset + 2, 2)
        count = u16(data, offset + 2) & 0x3FFF
        stride = 6 if encoding == "counted_records_6byte" else 12
        length = 4 + count * stride
        detail = f"u14_count={count}"
    elif encoding == "u16_list_ff00_then_u32_target":
        count = scan_word(data, offset + 2, 0xFF00)
        length = 8 + count * 2
        detail = f"item_count={count}"
    elif encoding == "dword_target_list_ff00ff00":
        count = scan_dword(data, offset + 2, 0xFF00FF00)
        length = 6 + count * 4
        detail = f"target_count={count}"
    elif encoding == "u16_prefix_list_ff00":
        count = scan_word(data, offset + 4, 0xFF00)
        length = 6 + count * 2
        detail = f"item_count={count}"
    elif encoding == "counted_u16_list":
        require(data, offset + 2, 2)
        count = u16(data, offset + 2)
        length = 4 + count * 2
        detail = f"u16_count={count}"
    elif encoding == "counted_6byte_records":
        require(data, offset + 2, 2)
        count = u16(data, offset + 2)
        length = 4 + count * 6
        detail = f"u16_count={count}"
    elif encoding == "u16_zero_terminated":
        count = scan_word(data, offset + 2, 0)
        length = 4 + count * 2
        detail = f"item_count={count}"
    else:
        raise ValueError(f"unsupported variable encoding {encoding}")
    # Opcode zero reaches the dispatcher's default arm.  Assembly proves that
    # arm yields without advancing the story IP, so zero is its exact physical
    # length for this probe rather than a malformed length.
    if length == 0 and opcode == 0:
        pass
    elif length <= 0:
        raise ValueError(f"nonpositive decoded length {length}")
    require(data, offset, length)
    return Record(
        file_name,
        offset,
        offset - PAYLOAD_BASE,
        raw_word,
        opcode,
        length,
        encoding,
        detail,
    )


def valid_slots(file_name: str, data: bytes, slot_count: int) -> list[tuple[int, int]]:
    rows = []
    for slot in range(slot_count):
        relative = u32(data, PAYLOAD_BASE + slot * 4)
        if relative == 0xFFFFFFFF:
            continue
        absolute = PAYLOAD_BASE + relative
        if absolute < PAYLOAD_BASE + slot_count * 4 or absolute + 2 > len(data):
            # TALK1 slot zero is the known non-normal sample value 0x64 inside
            # its own table; invalid/sentinel slots are evidence, not streams.
            continue
        rows.append((slot, relative))
    return rows


def write_tsv(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    rules = load_rules()
    entry_rows: list[tuple[object, ...]] = []
    unique_records: dict[tuple[str, int], Record] = {}
    instance_counts: Counter[tuple[str, int]] = Counter()
    opcode_instances: Counter[int] = Counter()
    opcode_files: dict[int, set[str]] = defaultdict(set)

    for file_name, (slot_count, expected_hash) in TALK_FILES.items():
        path = WORKSPACE_ROOT / file_name
        data = path.read_bytes()
        actual_hash = sha256(path)
        if actual_hash != expected_hash:
            raise SystemExit(f"locked TALK input changed: {file_name}: {actual_hash}")
        for slot, relative in valid_slots(file_name, data, slot_count):
            offset = PAYLOAD_BASE + relative
            start = offset
            first_opcode = ""
            last_opcode = ""
            reason = ""
            error = ""
            steps = 0
            local_seen: set[int] = set()
            while steps < MAX_STEPS:
                if offset in local_seen:
                    reason = "linear_offset_cycle"
                    break
                local_seen.add(offset)
                try:
                    record = decode_record(file_name, data, offset, rules)
                except ValueError as exc:
                    # Some numeric values in the nominal TALK3 2000-dword
                    # table point inside the file but are not story streams.
                    # Preserve them in the inventory, but do not conflate a
                    # rejected first word with a mid-stream decode failure.
                    reason = (
                        "invalid_index_candidate"
                        if file_name == "TALK3.DAT" and steps == 0
                        else "decode_error"
                    )
                    error = str(exc)
                    break
                if not first_opcode:
                    first_opcode = str(record.opcode)
                last_opcode = str(record.opcode)
                key = (file_name, offset)
                prior = unique_records.get(key)
                if prior is not None and prior != record:
                    raise SystemExit(f"non-deterministic decode at {file_name}:0x{offset:X}")
                unique_records[key] = record
                instance_counts[key] += 1
                opcode_instances[record.opcode] += 1
                opcode_files[record.opcode].add(file_name)
                steps += 1
                offset += record.length
                if record.opcode == 16383:
                    reason = "talk_end"
                    break
                if record.opcode in UNCONDITIONAL_TRANSFER:
                    reason = f"control_transfer_{record.opcode}"
                    break
                if record.opcode == 0:
                    reason = "default_no_advance"
                    break
            else:
                reason = "step_limit"
            entry_rows.append(
                (
                    file_name,
                    slot,
                    relative,
                    f"0x{start:08X}",
                    steps,
                    offset - start,
                    first_opcode,
                    last_opcode,
                    reason,
                    error,
                )
            )

    allowed_stop_reasons = {
        "control_transfer_15",
        "control_transfer_41",
        "control_transfer_87",
        "control_transfer_161",
        "invalid_index_candidate",
        "talk_end",
    }
    unexpected_stops = [
        row for row in entry_rows if str(row[8]) not in allowed_stop_reasons
    ]
    if unexpected_stops:
        raise SystemExit(
            f"unexpected TALK linear stop reasons: {unexpected_stops[:4]}"
        )
    invalid_index_candidates = sum(
        str(row[8]) == "invalid_index_candidate" for row in entry_rows
    )
    if invalid_index_candidates != 8:
        raise SystemExit(
            "unexpected TALK3 invalid index candidate count: "
            f"{invalid_index_candidates}"
        )

    record_rows = []
    opcode_unique: Counter[int] = Counter()
    for key in sorted(unique_records):
        record = unique_records[key]
        opcode_unique[record.opcode] += 1
        record_rows.append(
            (
                record.file_name,
                f"0x{record.file_offset:08X}",
                f"0x{record.payload_offset:08X}",
                f"0x{record.raw_word:04X}",
                record.opcode,
                record.length,
                record.encoding,
                record.detail,
                instance_counts[key],
            )
        )

    coverage_rows = []
    for opcode in sorted(rules):
        coverage_rows.append(
            (
                opcode,
                rules[opcode].encoding,
                opcode_unique[opcode],
                opcode_instances[opcode],
                ",".join(sorted(opcode_files[opcode])),
                "seen" if opcode_instances[opcode] else "not_seen_in_linear_prefix_probe",
            )
        )

    write_tsv(
        ENTRY_OUTPUT,
        (
            "file", "slot", "stream_relative", "file_start", "decoded_records",
            "linear_bytes", "first_opcode", "last_opcode", "stop_reason", "error",
        ),
        entry_rows,
    )
    write_tsv(
        RECORD_OUTPUT,
        (
            "file", "file_offset", "payload_offset", "raw_word", "effective_opcode",
            "decoded_length", "encoding_class", "length_detail", "entry_probe_hits",
        ),
        record_rows,
    )
    write_tsv(
        COVERAGE_OUTPUT,
        (
            "effective_opcode", "encoding_class", "unique_physical_records",
            "entry_probe_instances", "files", "coverage",
        ),
        coverage_rows,
    )
    reason_counts = Counter(str(row[8]) for row in entry_rows)
    print(f"wrote {ENTRY_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(entry_rows)} rows)")
    print(f"wrote {RECORD_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(record_rows)} rows)")
    print(f"wrote {COVERAGE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(coverage_rows)} rows)")
    for reason, count in sorted(reason_counts.items()):
        print(f"{reason}: {count}")
    print(f"seen opcodes: {sum(bool(opcode_instances[value]) for value in rules)}/{len(rules)}")


if __name__ == "__main__":
    main()
