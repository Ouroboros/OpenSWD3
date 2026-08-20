#!/usr/bin/env python3
"""Build LST-locked inventories for the SWD3 story VM dispatcher.

The complete LST machine listing is the sole behavioral authority.  Jump-table
symbols are resolved through labels from that same listing; every visible first
dword is also checked against its little-endian machine bytes.  Internal byte
selector tables are reconstructed directly from the LST byte column.  The
generator does not interpret story opcodes beyond their first dispatch target.
"""

from __future__ import annotations

import csv
import hashlib
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import cast

RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
LST_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.lst"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"

DISPATCH_OUTPUT = INVENTORY_ROOT / "story-vm-opcode-dispatch.tsv"
RANGE_OUTPUT = INVENTORY_ROOT / "story-vm-dispatch-ranges.tsv"
GROUP_OUTPUT = INVENTORY_ROOT / "story-vm-entry-target-groups.tsv"
INTERNAL_OUTPUT = INVENTORY_ROOT / "story-vm-internal-opcode-switches.tsv"

EXPECTED_SHA256 = {
    LST_PATH: "701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b",
}

MAIN_TABLE = (0x0042D4F4, 98, 1, 0x00427B88)
SECONDARY_TABLE = (0x0042D67C, 94, 100, 0x0042ADB0)

LST_LINE_RE = re.compile(r"^\.[^:]+:([0-9A-F]{8})\s*(.*?)\s*$")
LST_BYTE_PREFIX_RE = re.compile(
    r"^(?P<bytes>(?:[0-9A-F]{2}(?:\s+|…))+)(?P<text>.*)$"
)
LST_LABEL_RE = re.compile(r"^([A-Za-z_?$@][A-Za-z0-9_?$@]*)\s*:")
OFFSET_TARGET_RE = re.compile(r"\boffset\s+([A-Za-z_?$@][A-Za-z0-9_?$@]*)\b")

EXPECTED_SNIPPETS = {
    0x00427B40: "mov cx, [ebx]",
    0x00427B4F: "and esi, 3FFFh",
    0x00427B69: "cmp edx, 63h",
    0x00427B76: "jz loc_42AD75",
    0x00427B7C: "lea ecx, [edx-1]",
    0x00427B7F: "cmp ecx, 61h",
    0x00427B82: "ja def_427B88",
    0x00427B88: "jmp ds:jpt_427B88[ecx*4]",
    0x0042AD92: "cmp edx, 400h",
    0x0042AD98: "jg loc_42D219",
    0x0042AD9E: "jz loc_42D200",
    0x0042ADA4: "add edx, 0FFFFFF9Ch",
    0x0042ADA7: "cmp edx, 5Dh",
    0x0042ADAA: "ja def_427B88",
    0x0042ADB0: "jmp ds:jpt_42ADB0[edx*4]",
    0x0042D219: "sub edx, 401h",
    0x0042D21F: "jz loc_42D49F",
    0x0042D225: "dec edx",
    0x0042D226: "jz short loc_42D1EA",
    0x0042D228: "sub edx, 3BFDh",
    0x0042D22E: "jz short loc_42D24E",
    0x0042D230: "push 0",
    0x0042D232: "call ds:MessageBeep",
}

SPECIALS = {
    99: (
        0x0042AD75,
        "special_eq_99",
        "compare global 0x004B7AC8 with u16 operand; wait without advance or advance 4",
    ),
    1024: (
        0x0042D200,
        "special_eq_1024",
        "advance 2; set local continue flag; redispatch in same call",
    ),
    1025: (
        0x0042D49F,
        "special_eq_1025",
        "advance 2; clear continue flags; yield with return 1",
    ),
    1026: (
        0x0042D1EA,
        "special_eq_1026",
        "advance 2; set ESI continue flag; redispatch in same call",
    ),
    16383: (
        0x0042D24E,
        "special_eq_16383",
        "enter TalkEnd cleanup path; full state effects require opcode semantics pass",
    ),
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(text: str) -> str:
    return " ".join(text.split())


def split_lst_line(raw: str) -> tuple[int, list[int], str] | None:
    match = LST_LINE_RE.match(raw)
    if not match:
        return None
    address = int(match.group(1), 16)
    remainder = match.group(2)
    byte_match = LST_BYTE_PREFIX_RE.match(remainder)
    if byte_match is None:
        return address, [], remainder.strip()
    byte_values = [
        int(value, 16)
        for value in re.findall(r"[0-9A-F]{2}", byte_match.group("bytes"))
    ]
    return address, byte_values, byte_match.group("text").strip()


def load_lst() -> tuple[list[str], dict[int, str], dict[str, int]]:
    lines = LST_PATH.read_text(encoding="utf-8", errors="replace").splitlines()
    by_address: dict[int, str] = {}
    labels: dict[str, int] = {}
    for raw in lines:
        parsed = split_lst_line(raw)
        if parsed is None:
            continue
        address, byte_values, text = parsed
        label_match = LST_LABEL_RE.match(text)
        if label_match is not None:
            labels.setdefault(label_match.group(1), address)
        if not byte_values:
            continue
        text = text.split(";", 1)[0].rstrip()
        if not text or text.endswith(":"):
            continue
        by_address.setdefault(address, normalize(text))
    return lines, by_address, labels


def verify_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"locked input changed: {path.name}: expected {expected}, got {actual}"
            )


def verify_assembly(by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: "
                f"expected {expected!r}, got {actual!r}"
            )


def parse_lst_target_table(
    lines: list[str], labels: dict[str, int], address: int, count: int
) -> list[int]:
    end = address + count * 4
    targets: list[int] = []
    for raw in lines:
        parsed = split_lst_line(raw)
        if parsed is None:
            continue
        line_address, byte_values, _text = parsed
        if not (address <= line_address < end):
            continue
        names = OFFSET_TARGET_RE.findall(raw)
        if not names:
            continue
        try:
            resolved = [labels[name] for name in names]
        except KeyError as error:
            raise SystemExit(f"unresolved LST table label: {error.args[0]}") from error
        if len(byte_values) >= 4:
            displayed = int.from_bytes(bytes(byte_values[:4]), "little")
            if displayed != resolved[0]:
                raise SystemExit(
                    f"LST dword mismatch at 0x{line_address:08X}: "
                    f"bytes=0x{displayed:08X}, label=0x{resolved[0]:08X}"
                )
        targets.extend(resolved)
    if len(targets) != count:
        raise SystemExit(
            f"LST table 0x{address:08X}: expected {count} entries, got {len(targets)}"
        )
    return targets


def parse_lst_bytes(lines: list[str], address: int, count: int) -> bytes:
    end = address + count
    values: dict[int, int] = {}
    for raw in lines:
        parsed = split_lst_line(raw)
        if parsed is None:
            continue
        line_address, byte_values, _text = parsed
        for index, value in enumerate(byte_values):
            byte_address = line_address + index
            if not (address <= byte_address < end):
                continue
            previous = values.setdefault(byte_address, value)
            if previous != value:
                raise SystemExit(
                    f"conflicting LST byte at 0x{byte_address:08X}: "
                    f"0x{previous:02X} vs 0x{value:02X}"
                )
    missing = [byte_address for byte_address in range(address, end) if byte_address not in values]
    if missing:
        raise SystemExit(
            f"LST byte table 0x{address:08X}: missing byte 0x{missing[0]:08X}"
        )
    return bytes(values[byte_address] for byte_address in range(address, end))


def parse_lst_dwords(lines: list[str], address: int, count: int) -> list[int]:
    data = parse_lst_bytes(lines, address, count * 4)
    return [
        int.from_bytes(data[index:index + 4], "little")
        for index in range(0, len(data), 4)
    ]


def verify_table(
    lines: list[str], labels: dict[str, int], descriptor: tuple[int, int, int, int]
) -> list[int]:
    address, count, _opcode_base, _dispatch = descriptor
    return parse_lst_target_table(lines, labels, address, count)


def raw_aliases(opcode: int) -> str:
    return "|".join(f"0x{opcode | high:04X}" for high in (0, 0x4000, 0x8000, 0xC000))


def compact_ranges(values: list[int]) -> str:
    if not values:
        return ""
    spans: list[tuple[int, int]] = []
    start = previous = values[0]
    for value in values[1:]:
        if value == previous + 1:
            previous = value
            continue
        spans.append((start, previous))
        start = previous = value
    spans.append((start, previous))
    return ",".join(str(a) if a == b else f"{a}-{b}" for a, b in spans)


def build_dispatch_rows(
    main_targets: list[int], secondary_targets: list[int], by_address: dict[int, str]
) -> list[tuple[object, ...]]:
    entries: list[dict[str, object]] = [
        {
            "opcode": 0,
            "kind": "default_exact_0",
            "table_index": "n/a",
            "slot": "n/a",
            "target": 0x0042D230,
            "behavior": "MessageBeep; diagnostic call; no advance; yield with return 1",
        }
    ]
    for descriptor, targets, kind in (
        (MAIN_TABLE, main_targets, "main_table"),
        (SECONDARY_TABLE, secondary_targets, "secondary_table"),
    ):
        address, count, opcode_base, _dispatch = descriptor
        for index in range(count):
            entries.append(
                {
                    "opcode": opcode_base + index,
                    "kind": kind,
                    "table_index": index,
                    "slot": f"0x{address + index * 4:08X}",
                    "target": targets[index],
                    "behavior": "entry target only; parameters/length/state pending semantic pass",
                }
            )
    for opcode, (target, kind, behavior) in SPECIALS.items():
        entries.append(
            {
                "opcode": opcode,
                "kind": kind,
                "table_index": "n/a",
                "slot": "n/a",
                "target": target,
                "behavior": behavior,
            }
        )
    entries.sort(key=lambda entry: cast(int, entry["opcode"]))
    if len(entries) != 198 or len({entry["opcode"] for entry in entries}) != 198:
        raise SystemExit("unexpected explicit opcode count")

    counts = Counter(cast(int, entry["target"]) for entry in entries)
    rows = []
    for entry in entries:
        opcode = cast(int, entry["opcode"])
        target = cast(int, entry["target"])
        rows.append(
            (
                opcode,
                f"0x{opcode:04X}",
                raw_aliases(opcode),
                entry["kind"],
                entry["table_index"],
                entry["slot"],
                f"0x{target:08X}",
                counts[target],
                by_address.get(target, "<no decoded instruction>"),
                entry["behavior"],
            )
        )
    return rows


def build_range_rows() -> list[tuple[object, ...]]:
    return [
        ("decode", "raw_word 0..65535", "raw_word & 0x3FFF", "0x00427B4F", "upper two bits do not select the primary route; four raw words alias each effective opcode"),
        ("default", "opcode == 0", "0x0042D230", "0x00427B7C-0x00427B82", "unsigned index underflow takes JA; beep/diagnostic, no advance, return 1"),
        ("main_table", "1 <= opcode <= 98", "jpt_427B88[opcode-1]", "0x00427B7C-0x00427B88", "98 LST dwords at 0x0042D4F4"),
        ("special", "opcode == 99", "0x0042AD75", "0x00427B69-0x00427B76", "operand-gated wait; instruction is four bytes when accepted"),
        ("secondary_table", "100 <= opcode <= 193", "jpt_42ADB0[opcode-100]", "0x0042ADA4-0x0042ADB0", "94 LST dwords at 0x0042D67C"),
        ("default", "194 <= opcode <= 1023", "0x0042D230", "0x0042AD92-0x0042ADAA", "secondary unsigned index is above 93"),
        ("special", "opcode == 1024", "0x0042D200", "0x0042AD92-0x0042AD9E", "advance two and continue in same interpreter call"),
        ("special", "opcode == 1025", "0x0042D49F", "0x0042D219-0x0042D21F", "advance two and yield"),
        ("special", "opcode == 1026", "0x0042D1EA", "0x0042D225-0x0042D226", "advance two and continue in same interpreter call"),
        ("default", "1027 <= opcode <= 16382", "0x0042D230", "0x0042D228-0x0042D230", "post-special arithmetic misses 0x3FFF case"),
        ("special", "opcode == 16383", "0x0042D24E", "0x0042D228-0x0042D22E", "TalkEnd cleanup path"),
    ]


def build_group_rows(
    dispatch_rows: list[tuple[object, ...]], by_address: dict[int, str]
) -> list[tuple[object, ...]]:
    target_to_opcodes: dict[int, list[int]] = defaultdict(list)
    target_to_kinds: dict[int, set[str]] = defaultdict(set)
    for row in dispatch_rows:
        opcode = cast(int, row[0])
        target = int(str(row[6]), 16)
        target_to_opcodes[target].append(opcode)
        target_to_kinds[target].add(str(row[3]))

    # The default target also owns two large ranges which are intentionally not
    # expanded into 16,187 TSV rows.
    default_values = [0] + list(range(194, 1024)) + list(range(1027, 16383))
    target_to_opcodes[0x0042D230] = default_values

    rows = []
    for target in sorted(target_to_opcodes):
        opcodes = sorted(target_to_opcodes[target])
        if target == 0x0042D230:
            kinds = "default"
            note = "all unmapped effective opcodes; no instruction-pointer advance"
        else:
            kinds = ",".join(sorted(target_to_kinds[target]))
            note = (
                "single primary entry"
                if len(opcodes) == 1
                else "shared first entry only; handler may redispatch on opcode or use different operands"
            )
        rows.append(
            (
                f"0x{target:08X}",
                len(opcodes),
                compact_ranges(opcodes),
                kinds,
                by_address.get(target, "<no decoded instruction>"),
                note,
            )
        )
    return rows


def build_internal_rows() -> list[tuple[object, ...]]:
    return [
        (
            "shared_numeric_operation_refinement",
            "0x0042B0E4",
            "effective opcode retained in EDX",
            "29..185",
            "0x0042D7F4/6 dwords",
            "0x0042D80C/157 bytes",
            "29,30,31,32,33,181,182,183,184,185",
            "34..180",
            "refines shared primary targets 0x0042B074/0x0042B070; not a third primary table",
        ),
        (
            "shared_flag_selection_refinement",
            "0x0042C57B",
            "effective opcode reloaded from stack into ECX",
            "102..174",
            "0x0042D8AC/9 dwords",
            "0x0042D8D0/73 bytes",
            "102,103,117,136,140,145,146,174",
            "104..116,118..135,137..139,141..144,147..173",
            "refines shared primary target 0x0042C567; not a third primary table",
        ),
    ]


def verify_internal_tables(lines: list[str]) -> None:
    expected_jump_tables = {
        (0x0042D7F4, 6): [
            0x0042B0EB, 0x0042B0F4, 0x0042B0FD,
            0x0042B13D, 0x0042B172, 0x0042B1BF,
        ],
        (0x0042D8AC, 9): [
            0x0042C582, 0x0042C58C, 0x0042C596,
            0x0042C5A0, 0x0042C5AA, 0x0042C5B4,
            0x0042C5BE, 0x0042C5C8, 0x0042C5D0,
        ],
    }
    for (address, count), expected in expected_jump_tables.items():
        actual = parse_lst_dwords(lines, address, count)
        if actual != expected:
            raise SystemExit(f"internal jump table changed at 0x{address:08X}")

    table_a = parse_lst_bytes(lines, 0x0042D80C, 157)
    table_b = parse_lst_bytes(lines, 0x0042D8D0, 73)
    expected_a = bytearray([5] * 157)
    for opcode, selector in {
        29: 0, 30: 1, 31: 2, 32: 3, 33: 4,
        181: 0, 182: 1, 183: 2, 184: 3, 185: 4,
    }.items():
        expected_a[opcode - 29] = selector
    expected_b = bytearray([8] * 73)
    for opcode, selector in {
        102: 0, 103: 1, 117: 2, 136: 3,
        140: 4, 145: 5, 146: 6, 174: 7,
    }.items():
        expected_b[opcode - 102] = selector
    if table_a != expected_a or table_b != expected_b:
        raise SystemExit("internal opcode selector byte table changed")


def write_tsv(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    verify_inputs()
    lines, by_address, labels = load_lst()
    verify_assembly(by_address)
    main_targets = verify_table(lines, labels, MAIN_TABLE)
    secondary_targets = verify_table(lines, labels, SECONDARY_TABLE)
    verify_internal_tables(lines)

    dispatch_rows = build_dispatch_rows(main_targets, secondary_targets, by_address)
    range_rows = build_range_rows()
    group_rows = build_group_rows(dispatch_rows, by_address)
    internal_rows = build_internal_rows()

    if len(range_rows) != 11 or len(internal_rows) != 2:
        raise SystemExit("unexpected dispatcher summary row count")
    if sum(1 for row in dispatch_rows if row[3] == "main_table") != 98:
        raise SystemExit("main opcode rows changed")
    if sum(1 for row in dispatch_rows if row[3] == "secondary_table") != 94:
        raise SystemExit("secondary opcode rows changed")
    if sum(cast(int, row[1]) for row in group_rows) != 16384:
        raise SystemExit("entry target groups do not cover the full 14-bit opcode domain")

    write_tsv(
        DISPATCH_OUTPUT,
        (
            "effective_opcode_dec", "effective_opcode_hex", "raw_word_aliases",
            "dispatch_kind", "table_index", "slot_address", "entry_target",
            "explicit_entry_group_size", "first_instruction", "initial_behavior",
        ),
        dispatch_rows,
    )
    write_tsv(
        RANGE_OUTPUT,
        ("class", "predicate", "route", "assembly_branch", "behavior"),
        range_rows,
    )
    write_tsv(
        GROUP_OUTPUT,
        (
            "entry_target", "effective_opcode_count", "effective_opcodes",
            "route_sources", "first_instruction", "interpretation_limit",
        ),
        group_rows,
    )
    write_tsv(
        INTERNAL_OUTPUT,
        (
            "switch", "dispatch_address", "selector", "input_domain",
            "jump_table", "byte_selector_table", "nondefault_opcodes",
            "default_opcodes", "role",
        ),
        internal_rows,
    )
    for path, rows in (
        (DISPATCH_OUTPUT, dispatch_rows),
        (RANGE_OUTPUT, range_rows),
        (GROUP_OUTPUT, group_rows),
        (INTERNAL_OUTPUT, internal_rows),
    ):
        print(f"wrote {path.relative_to(RESEARCH_ROOT)} ({len(rows)} rows)")


if __name__ == "__main__":
    main()
