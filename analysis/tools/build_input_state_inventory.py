#!/usr/bin/env python3
"""Build assembly-locked inventories for SWD3 input normalization.

The complete assembly is the only behavioral authority. Pseudocode is not
read. Env.dat is used only to report the current configurable DIK bindings.
"""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
ENV_PATH = WORKSPACE_ROOT / "Env.dat"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"

RECORD_OUTPUT = INVENTORY_ROOT / "input-normalized-records.tsv"
TRANSITION_OUTPUT = INVENTORY_ROOT / "input-state-transitions.tsv"
ACCESS_OUTPUT = INVENTORY_ROOT / "input-state-direct-accesses.tsv"
RAW_QUERY_OUTPUT = INVENTORY_ROOT / "input-raw-key-queries.tsv"
SYNTHETIC_OUTPUT = INVENTORY_ROOT / "input-synthetic-key-writes.tsv"
REBASE_OUTPUT = INVENTORY_ROOT / "input-mouse-coordinate-rebases.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    ENV_PATH: "2c55dddc9a6808afda5d69688f2c27ac268caf2b9155ae82b18596ed593ed9a4",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")
STATE_SYMBOL_RE = re.compile(r"dword_4B7([CD][0-9A-F]{2})")

EXPECTED_SNIPPETS = {
    # Frame order: persistent keyboard snapshot, then normalization.
    0x0040A73A: "mov ecx, offset byte_4B8748",
    0x0040A744: "call sub_4372B0",
    0x0040A749: "call sub_4050E0",
    # Input clock snapshot, mouse poll, and first/last normalized records.
    0x004050E0: "mov eax, dword_4AAECC",
    0x004050F5: "mov dword_4CB224, eax",
    0x004050FA: "call sub_437310",
    0x00405118: "call sub_4053C0",
    0x004052E4: "call sub_4053C0",
    0x004052F4: "call sub_4053C0",
    0x00405304: "call sub_4053C0",
    # Left-button-only suppression countdown.
    0x00405309: "mov eax, dword_4CAE94",
    0x00405321: "dec eax",
    0x00405324: "mov dword_4B7DA0, edx",
    0x0040532A: "mov dword_4CAE94, eax",
    0x0040532F: "mov dword_4B7DA4, edx",
    0x00405335: "mov dword_4B7DA8, edx",
    0x0040533B: "mov dword_4B7DAC, edx",
    # Mouse inactivity bit 9: clear every frame; set after count exceeds 0x1C2.
    0x00405341: "push 9",
    0x00405343: "call sub_40DCB0",
    0x00405372: "inc edx",
    0x00405373: "cmp edx, 1C2h",
    0x00405381: "push 9",
    0x00405383: "call sub_40DC80",
    # Exact 16-byte normalized-record state machine.
    0x004053D1: "mov [eax+0Ch], edx",
    0x004053D8: "mov ecx, dword_4CB224",
    0x004053DE: "mov [eax+4], ecx",
    0x004053E5: "mov [eax], edx",
    0x004053E7: "mov esi, dword_4CB224",
    0x004053ED: "sub esi, ecx",
    0x004053EF: "cmp esi, 96h",
    0x004053F8: "mov [eax+8], edx",
    0x00405404: "mov ecx, [eax+8]",
    0x00405407: "mov ecx, ds:dword_49940C[ecx*4]",
    0x0040540E: "mov [eax+0Ch], edx",
    0x00405411: "mov [eax], ecx",
    0x00405413: "mov [eax+8], ecx",
    0x00405419: "mov [eax+4], edx",
    0x0040541C: "inc ecx",
    0x0040541D: "mov [eax+0Ch], ecx",
    # Multi-press transition table 0->1->2->3->3.
    0x0049940C: "dword_49940C dd 1",
    0x00499410: "db 2",
    0x00499414: "db 3",
    0x00499418: "db 3",
    # Keyboard raw query and synthetic raw-key write.
    0x004372D4: "mov al, [eax+ecx]",
    0x004372D7: "and eax, 80h",
    0x00437304: "or byte ptr [eax+ecx], 80h",
    # Mouse absolute custom data format: X, Y, button 0, button 1.
    0x00437058: "mov [esp+8Ch+var_58], 18h",
    0x00437060: "mov [esp+8Ch+var_50], 1",
    0x00437068: "mov [esp+8Ch+var_4C], 1Ch",
    0x00437070: "mov [esp+8Ch+var_48], edi",
    0x00437029: "mov [esp+8Ch+var_34], ebp",
    0x00437035: "mov [esp+8Ch+var_2C], edi",
    0x00437041: "mov [esp+8Ch+var_1C], 0Ch",
    0x0043704D: "mov [esp+8Ch+var_10], edx",
    # Mouse coordinate formula and inclusive clamps.
    0x00437331: "call dword ptr [ecx+24h]",
    0x00437346: "sub ecx, eax",
    0x0043734D: "imul ecx, edi",
    0x004373BE: "cmp eax, 27Fh",
    0x004373EB: "cmp ecx, 1DFh",
    0x00437412: "mov [edx], eax",
    0x0043741D: "mov [edx], ecx",
    0x004374C4: "fmul ds:dbl_4996B0",
    0x004996B0: "dbl_4996B0 dq 10.0",
    # DirectInput is acquired once at initialization, not display reactivation.
    0x004370F0: "push 0Ah",
    0x00437118: "call dword ptr [eax+1Ch]",
    0x00437194: "push 0Ah",
    0x0043725E: "call dword ptr [ecx+1Ch]",
}


@dataclass(frozen=True)
class Instruction:
    address: int
    text: str
    owner: str


@dataclass(frozen=True)
class RecordContract:
    index: int
    updater: int | None
    source: str
    binding_global: str
    env_offset: int | None
    current_binding: str
    logical_role: str


RECORDS = (
    RecordContract(0, 0x00405118, "keyboard_bit_0x80", "0x004B7384", 0x0B, "0x01 DIK_ESCAPE", "cancel/escape binding"),
    RecordContract(1, 0x00405137, "keyboard_bit_0x80", "0x004B7388", 0x08, "0x39 DIK_SPACE", "primary confirm/action binding"),
    RecordContract(2, 0x00405174, "keyboard_bit_0x80", "0x004B738C", 0x0D, "0x36 DIK_RSHIFT", "configurable binding slot 2"),
    RecordContract(3, 0x00405193, "keyboard_bit_0x80", "0x004B7390", 0x06, "0xCB DIK_LEFT", "left binding"),
    RecordContract(4, 0x004051B1, "keyboard_bit_0x80", "0x004B7394", 0x04, "0xC8 DIK_UP", "up binding"),
    RecordContract(5, 0x004051D0, "keyboard_bit_0x80", "0x004B7398", 0x07, "0xCD DIK_RIGHT", "right binding"),
    RecordContract(6, 0x004051EF, "keyboard_bit_0x80", "0x004B739C", 0x05, "0xD0 DIK_DOWN", "down binding"),
    RecordContract(7, 0x00405269, "keyboard_bit_0x80", "0x004B73A0", 0x12, "0xC9 DIK_PRIOR/PageUp", "page-up binding"),
    RecordContract(8, 0x0040524B, "keyboard_bit_0x80", "0x004B73A4", 0x13, "0xD1 DIK_NEXT/PageDown", "page-down binding"),
    RecordContract(9, 0x0040520D, "keyboard_bit_0x80", "0x004B73A8", 0x0A, "0x9D DIK_RCONTROL", "configurable binding slot 9"),
    RecordContract(10, 0x0040522C, "keyboard_bit_0x80", "0x004B73AC", 0x0C, "0xCF DIK_END", "configurable binding slot 10"),
    RecordContract(11, None, "none", "none", None, "none", "reserved/unupdated record"),
    RecordContract(12, 0x00405155, "keyboard_bit_0x80", "0x004B73B4", 0x09, "0x1C DIK_RETURN", "alternate confirm binding"),
    RecordContract(13, None, "none", "none", None, "none", "reserved/unupdated record"),
    RecordContract(14, 0x00405304, "mouse_mask_bit_1 (value 2)", "none", None, "mouse button 1", "right mouse button"),
    RecordContract(15, 0x004052F4, "mouse_mask_bit_0 (value 1)", "none", None, "mouse button 0", "left mouse button; suppression target"),
    RecordContract(16, 0x00405288, "keyboard_bit_0x80", "0x004B73C4", 0x0E, "0x13 DIK_R", "configurable binding slot 16"),
    RecordContract(17, 0x004052A7, "keyboard_bit_0x80", "0x004B73C8", 0x0F, "0x1E DIK_A", "configurable binding slot 17"),
    RecordContract(18, 0x004052C5, "keyboard_bit_0x80", "0x004B73CC", 0x10, "0x22 DIK_G", "configurable binding slot 18"),
    RecordContract(19, 0x004052E4, "keyboard_bit_0x80", "0x004B73D0", 0x11, "0x3B DIK_F1", "configurable binding slot 19"),
)

EXPECTED_NORMALIZER_CALLS = tuple(
    record.updater for record in sorted((item for item in RECORDS if item.updater), key=lambda item: item.updater)
)
EXPECTED_SYNTHETIC_CALLS = (0x00406EC9, 0x00440A7D, 0x00443013, 0x0044580B, 0x0044B149)
EXPECTED_SYNTHETIC_OPERANDS = ("1", "edx", "edx", "eax", "0Eh")
EXPECTED_REBASE_CALLS = (
    0x00424BD8, 0x00451E44, 0x0046287E, 0x004628D1,
    0x00462996, 0x00462AA6, 0x00462ACF, 0x004644D5,
)
EXPECTED_REBASE_OPERANDS = (
    ("1E0h", "168h"),
    ("ecx", "eax"),
    ("0F0h", "0C6h"),
    ("0F0h", "0C6h"),
    ("0F0h", "0C6h"),
    ("0F0h", "0C6h"),
    ("0F0h", "0C6h"),
    ("edi", "edx"),
)
EXPECTED_DIRECT_ACCESS_COUNT = 242


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(text: str) -> str:
    return " ".join(text.split())


def load_assembly() -> tuple[list[Instruction], dict[int, str]]:
    instructions: list[Instruction] = []
    by_address: dict[int, str] = {}
    owner = ""
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        owner_match = re.search(r"\b(sub_[0-9A-F]{6}|WinMain)\s+proc near\b", raw)
        if owner_match:
            owner = owner_match.group(1)
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        text = match.group(2).split(";", 1)[0].rstrip()
        if not text or text.endswith(":"):
            continue
        address = int(match.group(1), 16)
        text = normalize(text)
        instructions.append(Instruction(address, text, owner))
        by_address[address] = text
    return instructions, by_address


def verify_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(f"locked input changed: {path}: expected {expected}, got {actual}")


def verify_assembly(instructions: list[Instruction], by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: expected {expected!r}, got {actual!r}"
            )

    def calls(target: str) -> tuple[int, ...]:
        return tuple(item.address for item in instructions if item.text == f"call {target}")

    if calls("sub_4053C0") != EXPECTED_NORMALIZER_CALLS:
        raise SystemExit(f"normalized record update calls changed: {calls('sub_4053C0')}")
    if calls("sub_437300") != EXPECTED_SYNTHETIC_CALLS:
        raise SystemExit(f"synthetic raw-key calls changed: {calls('sub_437300')}")
    if calls("sub_437430") != EXPECTED_REBASE_CALLS:
        raise SystemExit(f"mouse coordinate rebase calls changed: {calls('sub_437430')}")
    raw_queries = calls("sub_4372D0")
    if len(raw_queries) != 77:
        raise SystemExit(f"raw key query count changed: {len(raw_queries)}")


def build_record_rows() -> list[tuple[object, ...]]:
    env = ENV_PATH.read_bytes()
    rows: list[tuple[object, ...]] = []
    for record in RECORDS:
        base = 0x004B7CB0 + record.index * 0x10
        if record.env_offset is not None:
            expected_code = int(record.current_binding[2:4], 16)
            actual_code = env[record.env_offset]
            if actual_code != expected_code:
                raise SystemExit(
                    f"Env.dat binding changed at 0x{record.env_offset:02X}: "
                    f"expected 0x{expected_code:02X}, got 0x{actual_code:02X}"
                )
        rows.append(
            (
                record.index,
                f"0x{base:08X}",
                f"0x{record.updater:08X}" if record.updater else "none",
                record.source,
                record.binding_global,
                f"0x{record.env_offset:02X}" if record.env_offset is not None else "none",
                record.current_binding,
                record.logical_role,
                "+0 current rapid-press multiplicity (clears on second released sample)",
                "+4 release timestamp; zero while continuously held",
                "+8 rapid-press chain stage 0..3; resets after >150 clock units released",
                "+0x0C continuous raw-down sample count; zero on release",
            )
        )
    return rows


def transition_rows() -> list[tuple[str, str, str, str]]:
    return [
        ("raw=0 and +4=0", "+0x0C=0; +4=current_input_clock; preserve +0/+8", "release is represented immediately by hold_count=0; click multiplicity can survive this first released sample", "0"),
        ("raw=0 and +4!=0 and elapsed<=150", "+0x0C=0; +0=0; preserve +4/+8", "second and later released samples clear current multiplicity but keep the rapid-press chain", "0"),
        ("raw=0 and +4!=0 and elapsed>150", "+0x0C=0; +0=0; +8=0; preserve +4", "rapid-press window expires only when elapsed is strictly greater than 150", "0"),
        ("raw!=0 and +4!=0", "v=table[+8] where table={1,2,3,3}; +0=v; +8=v; +4=0; +0x0C=1", "new press/re-press; multiplicity caps at three", "new +0x0C value"),
        ("raw!=0 and +4=0", "+0x0C += 1; preserve +0/+8", "continuous hold; no centralized repeat pulse", "new +0x0C value"),
        ("left suppression counter old value >0", "counter -= 1; clear all four dwords of record 15 after it was updated", "exactly the next N normalization calls suppress left click; a still-held raw button is erased each time", "record update return is discarded"),
        ("x/y unchanged and raw mouse button mask=0", "clear bit 9; increment inactivity counter; set bit 9 only when new counter>0x1C2", "bit 9 first becomes set on stable sample 451 and is cleared/re-set on later stable samples", "not applicable"),
        ("x/y changed or raw mouse button mask!=0", "clear bit 9; inactivity counter=0; save current x/y", "raw mouse activity resets inactivity even when left normalized record is suppressed", "not applicable"),
    ]


def classify_access(text: str, symbol: str) -> str:
    mnemonic, _, operands = text.partition(" ")
    destination = operands.split(",", 1)[0].strip()
    if "offset " + symbol in text or mnemonic == "lea":
        return "address_taken"
    if symbol == destination:
        if mnemonic == "mov":
            return "write"
        if mnemonic in {"cmp", "push"}:
            return "read"
        return "read_write"
    return "read"


def build_access_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    rows: list[tuple[object, ...]] = []
    for item in instructions:
        if item.address >= 0x00499000:
            continue
        for match in STATE_SYMBOL_RE.finditer(item.text):
            symbol = match.group(0)
            address = int("4B7" + match.group(1), 16)
            if not 0x004B7CB0 <= address < 0x004B7DF0:
                continue
            delta = address - 0x004B7CB0
            record_index, field_offset = divmod(delta, 0x10)
            field = {
                0x0: "rapid_press_multiplicity",
                0x4: "release_timestamp",
                0x8: "rapid_press_chain_stage",
                0xC: "continuous_down_count",
            }.get(field_offset, f"noncanonical_offset_0x{field_offset:X}")
            rows.append(
                (
                    f"0x{item.address:08X}", item.owner, symbol,
                    record_index, f"0x{field_offset:02X}", field,
                    classify_access(item.text, symbol), item.text,
                )
            )
    return rows


def preceding_pushes(instructions: list[Instruction], call_address: int, limit: int = 10) -> list[str]:
    index_by_address = {item.address: index for index, item in enumerate(instructions)}
    index = index_by_address[call_address]
    return [
        item.text[5:]
        for item in instructions[max(0, index - limit):index]
        if item.text.startswith("push ")
    ]


def call_rows(instructions: list[Instruction], target: str) -> list[tuple[object, ...]]:
    rows = []
    for item in instructions:
        if item.text != f"call {target}":
            continue
        pushes = preceding_pushes(instructions, item.address)
        rows.append((f"0x{item.address:08X}", item.owner, pushes[-1] if pushes else "", item.text))
    return rows


def build_rebase_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    rows = []
    for address in EXPECTED_REBASE_CALLS:
        item = next(item for item in instructions if item.address == address)
        pushes = preceding_pushes(instructions, address, 14)
        args = pushes[-2:] if len(pushes) >= 2 else pushes
        rows.append(
            (
                f"0x{address:08X}", item.owner,
                args[-1] if len(args) >= 1 else "",
                args[-2] if len(args) >= 2 else "",
                "clamp requested logical x/y to 0..639/0..479, then change absolute-axis baseline; does not write normalized records",
            )
        )
    return rows


def write_table(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    verify_inputs()
    instructions, by_address = load_assembly()
    verify_assembly(instructions, by_address)
    records = build_record_rows()
    transitions = transition_rows()
    accesses = build_access_rows(instructions)
    raw_queries = call_rows(instructions, "sub_4372D0")
    synthetic = call_rows(instructions, "sub_437300")
    rebases = build_rebase_rows(instructions)

    if len(records) != 20 or len(transitions) != 8:
        raise SystemExit(
            f"normalized table size changed: {len(records)} records, "
            f"{len(transitions)} transitions"
        )
    if len(accesses) != EXPECTED_DIRECT_ACCESS_COUNT:
        raise SystemExit(
            f"direct normalized-state access count changed: {len(accesses)}"
        )
    synthetic_operands = tuple(row[2] for row in synthetic)
    if synthetic_operands != EXPECTED_SYNTHETIC_OPERANDS:
        raise SystemExit(f"synthetic raw-key operands changed: {synthetic_operands}")
    rebase_operands = tuple((row[2], row[3]) for row in rebases)
    if rebase_operands != EXPECTED_REBASE_OPERANDS:
        raise SystemExit(f"mouse coordinate rebase operands changed: {rebase_operands}")

    write_table(RECORD_OUTPUT, ("record_index", "base_address", "updater_call", "physical_source", "binding_global", "env_file_offset", "current_env_binding", "logical_role", "field_0x00", "field_0x04", "field_0x08", "field_0x0C"), records)
    write_table(TRANSITION_OUTPUT, ("condition", "exact_writes", "observable_semantics", "return_value"), transitions)
    write_table(ACCESS_OUTPUT, ("instruction_address", "owner", "symbol", "record_index", "field_offset", "field_role", "access", "instruction"), accesses)
    write_table(RAW_QUERY_OUTPUT, ("call_address", "owner", "key_operand", "instruction"), raw_queries)
    write_table(SYNTHETIC_OUTPUT, ("call_address", "owner", "key_operand", "instruction"), synthetic)
    write_table(REBASE_OUTPUT, ("call_address", "owner", "x_operand", "y_operand", "contract"), rebases)
    print(
        f"wrote {len(records)} records, {len(transitions)} transitions, "
        f"{len(accesses)} direct state accesses, {len(raw_queries)} raw key queries, "
        f"{len(synthetic)} synthetic key writes, and {len(rebases)} mouse rebases"
    )


if __name__ == "__main__":
    main()
