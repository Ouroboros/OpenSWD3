#!/usr/bin/env python3
"""Build an assembly-bounded all-branch candidate graph for TALK streams.

The graph is deliberately an over-approximation.  Conditional story branches
contribute both their sequential and transfer edges; opcode 41 contributes all
table entries plus its selector==count sentinel bug.  This validates framing
after reloaded 0x8000-byte windows without pretending to evaluate game state.
"""

from __future__ import annotations

import csv
import hashlib
import struct
import sys
from collections import Counter, defaultdict, deque
from dataclasses import dataclass
from pathlib import Path


TOOL_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOL_ROOT))
import build_story_vm_talk_linear_probe as linear  # noqa: E402


RESEARCH_ROOT = TOOL_ROOT.parent
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
LST_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.lst"

RULE_OUTPUT = INVENTORY_ROOT / "story-vm-control-transfer-rules.tsv"
NODE_OUTPUT = INVENTORY_ROOT / "story-vm-talk-cfg-nodes.tsv"
EDGE_OUTPUT = INVENTORY_ROOT / "story-vm-talk-cfg-edges.tsv"
ISSUE_OUTPUT = INVENTORY_ROOT / "story-vm-talk-cfg-issues.tsv"

EXPECTED_LST_SHA256 = "701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b"
WINDOW_SIZE = 0x8000


@dataclass(frozen=True, order=True)
class State:
    file_name: str
    window_relative: int
    ip: int


@dataclass(frozen=True)
class TransferRule:
    opcode: int
    branch_kind: str
    target_shape: str
    target_offset: str
    sequential_possible: bool
    selector_behavior: str
    call_site: str
    assembly_evidence: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def transfer_rules() -> dict[int, TransferRule]:
    rows: dict[int, TransferRule] = {}

    def add(
        opcodes: tuple[int, ...], branch_kind: str, target_shape: str,
        target_offset: str, sequential_possible: bool, selector_behavior: str,
        call_site: str, evidence: str,
    ) -> None:
        for opcode in opcodes:
            if opcode in rows:
                raise SystemExit(f"duplicate transfer rule for opcode {opcode}")
            rows[opcode] = TransferRule(
                opcode, branch_kind, target_shape, target_offset,
                sequential_possible, selector_behavior, call_site, evidence,
            )

    add((15,), "unconditional_same_file_reload", "u32", "+2", False,
        "single target", "0x0042CCDB", "0x00428310-0x00428313;0x0042CCD5-0x0042CCDB")
    add((16, 17), "conditional_same_file_reload", "u32", "+4", True,
        "role presence/state predicate", "0x00428388/0x0042841D", "0x00428318-0x00428447")
    add((21, 22), "conditional_same_file_reload", "u32", "+4", True,
        "flag predicate, inverted between opcodes", "0x0042CCDB", "0x00428533-0x00428576")
    add((23, 24), "conditional_same_file_reload", "u32_after_ff00", "after list sentinel", True,
        "all/any flag-list predicate", "0x0042CCDB", "0x0042857F-0x00428656")
    add((32, 33), "conditional_same_file_reload", "u32", "+6", True,
        "global numeric >= / <= predicate", "0x0042B1AB", "0x0042B13D-0x0042B1E4")
    add((35, 36), "conditional_same_file_reload", "u32", "+4", True,
        "global index range predicate", "0x00428956/0x00428994", "0x00428934-0x004289AB")
    add((41,), "indexed_same_file_reload", "u32_list_ff00ff00", "+2", False,
        "global selector; selector==count reads sentinel as target; selector>count falls back to zero",
        "0x00428CF2", "0x00428C9F-0x00428D13")
    add((87,), "random_same_file_reload", "u32_list_ff00ff00", "+2", False,
        "sub_439070(count) returns 0..count-1; count zero divides by zero",
        "0x0042A70B", "0x0042A6CB-0x0042A722;0x00439070-0x004390E0")
    add((110, 111), "conditional_same_file_reload", "u32", "+2", True,
        "party-state predicate, inverted for opcode 111", "0x0042CCDB", "0x0042B6A5-0x0042B703")
    add((126, 127), "conditional_same_file_reload", "u32", "+6", True,
        "role field equality predicate, inverted for opcode 127", "0x0042BE4D", "0x0042BDBC-0x0042BE85")
    add((129, 130, 167, 168), "conditional_same_file_reload", "u32", "+4", True,
        "role lookup/existence predicate refined by opcode", "0x0042CCDB", "0x0042BEFE-0x0042BF6D")
    add((138,), "conditional_same_file_reload", "u32", "+10", True,
        "distance threshold predicate", "0x0042CCDB", "0x0042C49E-0x0042C554")
    add((161,), "unconditional_story_reload", "s16_story_id", "+2", False,
        "sign-extended operand passed to unsigned file/slot arithmetic", "0x0042CBE3", "0x0042CBCC-0x0042CBFA;0x0042E480-0x0042E594")
    add((163, 164), "conditional_same_file_reload", "u32", "+4", True,
        "argument/global equality predicate, inverted between opcodes", "0x0042CCDB", "0x0042CBFF-0x0042CC30")
    add((165, 166), "conditional_same_file_reload", "u32", "+6", True,
        "combined role statistic threshold predicate", "0x0042CCDB", "0x0042CC35-0x0042CCF2")
    add((184, 185), "conditional_same_file_reload", "u32", "+8", True,
        "32-bit global numeric >= / <= predicate", "0x0042B1AB", "0x0042B13D-0x0042B1E4")
    add((186, 187), "conditional_same_file_reload", "u32", "+6", True,
        "sub_4112B0 result threshold predicate", "0x0042D0C1", "0x0042D05C-0x0042D0D3")
    return rows


def validate_assembly(rules: dict[int, TransferRule]) -> None:
    if sha256(LST_PATH) != EXPECTED_LST_SHA256:
        raise SystemExit("locked authoritative LST input changed")
    text = LST_PATH.read_text(encoding="utf-8", errors="replace")
    markers = (
        ".text:00428388 E8 A3 60 00                 call    sub_42E430",
        ".text:0042841D E8 0E 60 00                 call    sub_42E430",
        ".text:00428956 E8 D5 5A 00                 call    sub_42E430",
        ".text:00428994 E8 97 5A 00                 call    sub_42E430",
        ".text:00428CF2 E8 39 57 00                 call    sub_42E430",
        ".text:0042A70B E8 20 3D 00                 call    sub_42E430",
        ".text:0042B1AB E8 80 32 00                 call    sub_42E430",
        ".text:0042BE4D E8 DE 25 00                 call    sub_42E430",
        ".text:0042CBE3 E8 98 18 00                 call    sub_42E480",
        ".text:0042CCDB E8 50 17 00                 call    sub_42E430",
        ".text:0042D0C1 E8 6A 13 00                 call    sub_42E430",
        ".text:0043907D F7 F7                       div     edi",
    )
    missing = [marker for marker in markers if marker not in text]
    if missing:
        raise SystemExit(f"LST transfer markers missing: {missing}")
    if len(rules) != 31:
        raise SystemExit(f"expected 31 transfer opcodes, got {len(rules)}")


def write_tsv(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def file_offset(state: State) -> int:
    return linear.PAYLOAD_BASE + state.window_relative + state.ip


def u32_at(data: bytes, offset: int) -> int:
    linear.require(data, offset, 4)
    return struct.unpack_from("<I", data, offset)[0]


def s16_at(data: bytes, offset: int) -> int:
    linear.require(data, offset, 2)
    return struct.unpack_from("<h", data, offset)[0]


def same_file_target(source: State, relative: int) -> State:
    return State(source.file_name, relative, 0)


def target_resolution(state: State, data_by_file: dict[str, bytes]) -> tuple[str, str]:
    data = data_by_file.get(state.file_name)
    if data is None:
        return "missing_file", ""
    absolute = file_offset(state)
    if state.window_relative < 0 or absolute < linear.PAYLOAD_BASE or absolute + 2 > len(data):
        return "target_outside_file", f"0x{absolute:08X}"
    return "enqueued", f"0x{absolute:08X}"


def decode_targets(
    state: State, record: linear.Record, data: bytes, rule: TransferRule,
    data_by_file: dict[str, bytes],
) -> list[tuple[str, int, State | None, str, str]]:
    """Return edge kind, branch index, target, resolution, detail."""
    absolute = record.file_offset
    edges: list[tuple[str, int, State | None, str, str]] = []

    if rule.target_shape == "u32":
        operand_offset = int(rule.target_offset[1:])
        relative = u32_at(data, absolute + operand_offset)
        target = same_file_target(state, relative)
        resolution, detail = target_resolution(target, data_by_file)
        edges.append(("conditional_transfer" if rule.sequential_possible else "transfer", 0, target, resolution, detail))
    elif rule.target_shape == "u32_after_ff00":
        relative = u32_at(data, absolute + record.length - 4)
        target = same_file_target(state, relative)
        resolution, detail = target_resolution(target, data_by_file)
        edges.append(("conditional_transfer", 0, target, resolution, detail))
    elif rule.target_shape == "u32_list_ff00ff00":
        cursor = absolute + 2
        values: list[int] = []
        while True:
            value = u32_at(data, cursor)
            if value == 0xFF00FF00:
                break
            values.append(value)
            cursor += 4
        for index, relative in enumerate(values):
            target = same_file_target(state, relative)
            resolution, detail = target_resolution(target, data_by_file)
            edges.append(("indexed_transfer" if record.opcode == 41 else "random_transfer", index, target, resolution, detail))
        if record.opcode == 41:
            bug_target = same_file_target(state, 0xFF00FF00)
            resolution, detail = target_resolution(bug_target, data_by_file)
            edges.append(("selector_equal_count_sentinel_bug", len(values), bug_target, resolution, detail))
        elif not values:
            edges.append(("random_count_zero_divide_error", 0, None, "terminal_error", "sub_439070(0) divides by zero"))
    elif rule.target_shape == "s16_story_id":
        signed_id = s16_at(data, absolute + 2)
        unsigned_id = signed_id & 0xFFFFFFFF
        file_number = unsigned_id // 2000 + 1
        slot = unsigned_id % 2000
        target_file = f"TALK{file_number}.DAT"
        target_data = data_by_file.get(target_file)
        if target_data is None:
            edges.append(("story_reload", 0, None, "story_file_outside_sample", f"signed_id={signed_id};unsigned_id={unsigned_id};file={file_number};slot={slot}"))
        else:
            index_offset = linear.PAYLOAD_BASE + slot * 4
            if index_offset + 4 > len(target_data):
                edges.append(("story_reload", 0, None, "story_slot_outside_file", f"signed_id={signed_id};file={file_number};slot={slot}"))
            else:
                relative = u32_at(target_data, index_offset)
                target = State(target_file, relative, 0)
                resolution, target_detail = target_resolution(target, data_by_file)
                edges.append(("story_reload", 0, target, resolution, f"signed_id={signed_id};file={file_number};slot={slot};{target_detail}"))
    else:
        raise SystemExit(f"unsupported transfer target shape {rule.target_shape}")
    return edges


def main() -> None:
    rules = transfer_rules()
    validate_assembly(rules)
    length_rules = linear.load_rules()

    data_by_file: dict[str, bytes] = {}
    roots: dict[State, list[int]] = defaultdict(list)
    invalid_root_slots: list[tuple[str, int, int]] = []
    for file_name, (slot_count, expected_hash) in linear.TALK_FILES.items():
        path = WORKSPACE_ROOT / file_name
        data = path.read_bytes()
        if sha256(path) != expected_hash:
            raise SystemExit(f"locked TALK input changed: {file_name}")
        data_by_file[file_name] = data
        for slot, relative in linear.valid_slots(file_name, data, slot_count):
            try:
                linear.decode_record(
                    file_name,
                    data,
                    linear.PAYLOAD_BASE + relative,
                    length_rules,
                )
            except ValueError:
                if file_name != "TALK3.DAT":
                    raise
                invalid_root_slots.append((file_name, slot, relative))
                continue
            roots[State(file_name, relative, 0)].append(slot)
    if len(invalid_root_slots) != 8:
        raise SystemExit(
            "unexpected TALK3 invalid CFG root count: "
            f"{len(invalid_root_slots)}"
        )

    queue = deque(sorted(roots))
    visited: set[State] = set()
    records: dict[State, linear.Record] = {}
    outgoing: Counter[State] = Counter()
    edge_rows: list[tuple[object, ...]] = []
    issue_rows: list[tuple[object, ...]] = []

    def add_edge(
        source: State, record: linear.Record, kind: str, branch_index: int,
        target: State | None, resolution: str, detail: str, evidence: str,
    ) -> None:
        outgoing[source] += 1
        target_file = target.file_name if target else ""
        target_window = target.window_relative if target else ""
        target_ip = target.ip if target else ""
        target_absolute = f"0x{file_offset(target):08X}" if target and resolution == "enqueued" else ""
        edge_rows.append((
            source.file_name, source.window_relative, source.ip,
            f"0x{record.file_offset:08X}", record.opcode, kind, branch_index,
            target_file, target_window, target_ip, target_absolute,
            resolution, detail, evidence,
        ))
        if target is not None and resolution == "enqueued" and target not in visited:
            queue.append(target)

    while queue:
        state = queue.popleft()
        if state in visited:
            continue
        visited.add(state)
        data = data_by_file[state.file_name]
        absolute = file_offset(state)
        try:
            record = linear.decode_record(state.file_name, data, absolute, length_rules)
        except ValueError as exc:
            issue_rows.append((
                state.file_name, state.window_relative, state.ip,
                f"0x{absolute:08X}", "decode_error", str(exc),
                ",".join(map(str, roots.get(state, []))),
            ))
            continue
        if state.ip + record.length > WINDOW_SIZE:
            issue_rows.append((
                state.file_name, state.window_relative, state.ip,
                f"0x{absolute:08X}", "crosses_0x8000_window",
                f"opcode={record.opcode};length={record.length}",
                ",".join(map(str, roots.get(state, []))),
            ))
            continue
        records[state] = record

        if record.opcode in {0, 16383}:
            continue
        rule = rules.get(record.opcode)
        if rule is not None:
            try:
                transfers = decode_targets(state, record, data, rule, data_by_file)
            except ValueError as exc:
                issue_rows.append((
                    state.file_name, state.window_relative, state.ip,
                    f"0x{absolute:08X}", "target_decode_error", str(exc),
                    ",".join(map(str, roots.get(state, []))),
                ))
                continue
            for kind, branch_index, target, resolution, detail in transfers:
                add_edge(state, record, kind, branch_index, target, resolution, detail, rule.assembly_evidence)
            if not rule.sequential_possible:
                continue

        sequential = State(state.file_name, state.window_relative, state.ip + record.length)
        if sequential.ip >= WINDOW_SIZE:
            resolution = "sequential_at_or_past_window_end"
            detail = f"ip=0x{sequential.ip:X}"
        else:
            resolution, detail = target_resolution(sequential, data_by_file)
        add_edge(state, record, "sequential", 0, sequential, resolution, detail, "opcode length rule")

    if issue_rows:
        raise SystemExit(f"unexpected TALK CFG issues: {issue_rows[:4]}")

    rule_rows = [
        (
            value.opcode, value.branch_kind, value.target_shape, value.target_offset,
            "yes" if value.sequential_possible else "no", value.selector_behavior,
            value.call_site, value.assembly_evidence,
        )
        for value in sorted(rules.values(), key=lambda item: item.opcode)
    ]
    node_rows = []
    for state in sorted(records):
        record = records[state]
        node_rows.append((
            state.file_name, state.window_relative, state.ip,
            f"0x{record.file_offset:08X}", f"0x{record.raw_word:04X}",
            record.opcode, record.length, record.encoding, outgoing[state],
            ",".join(map(str, roots.get(state, []))),
        ))

    edge_rows.sort(key=lambda row: (str(row[0]), int(row[1]), int(row[2]), str(row[5]), int(row[6])))
    issue_rows.sort(key=lambda row: (str(row[0]), int(row[1]), int(row[2]), str(row[4])))

    write_tsv(RULE_OUTPUT, (
        "effective_opcode", "branch_kind", "target_shape", "target_offset",
        "sequential_possible", "selector_behavior", "call_site", "assembly_evidence",
    ), rule_rows)
    write_tsv(NODE_OUTPUT, (
        "file", "window_relative", "ip", "file_offset", "raw_word",
        "effective_opcode", "decoded_length", "encoding_class",
        "outgoing_edges", "root_slots",
    ), node_rows)
    write_tsv(EDGE_OUTPUT, (
        "source_file", "source_window_relative", "source_ip", "source_file_offset",
        "source_opcode", "edge_kind", "branch_index", "target_file",
        "target_window_relative", "target_ip", "target_file_offset",
        "resolution", "detail", "assembly_evidence",
    ), edge_rows)
    write_tsv(ISSUE_OUTPUT, (
        "file", "window_relative", "ip", "file_offset", "issue", "detail", "root_slots",
    ), issue_rows)

    print(f"wrote {RULE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(rule_rows)} rows)")
    print(f"wrote {NODE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(node_rows)} rows)")
    print(f"wrote {EDGE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(edge_rows)} rows)")
    print(f"wrote {ISSUE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(issue_rows)} rows)")
    print(f"unique root windows: {len(roots)}; root slots: {sum(map(len, roots.values()))}")
    print(f"excluded invalid TALK3 root slots: {len(invalid_root_slots)}")
    print(f"decoded context nodes: {len(records)}")
    print(f"physical instruction offsets: {len({(r.file_name, r.file_offset) for r in records.values()})}")
    print("edge kinds:", dict(sorted(Counter(str(row[5]) for row in edge_rows).items())))
    print("resolutions:", dict(sorted(Counter(str(row[11]) for row in edge_rows).items())))
    print("issues:", dict(sorted(Counter(str(row[4]) for row in issue_rows).items())))


if __name__ == "__main__":
    main()
