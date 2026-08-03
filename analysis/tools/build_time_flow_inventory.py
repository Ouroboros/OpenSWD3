#!/usr/bin/env python3
"""Build assembly-locked inventories for SWD3 clock and frame timing.

The complete assembly is the only behavioral authority. Pseudocode is not
read. The import list is checked to distinguish timeGetTime from timer APIs.
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
IMPORT_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "imports.txt"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"

SOURCE_OUTPUT = INVENTORY_ROOT / "time-source-callsites.tsv"
INTERVAL_OUTPUT = INVENTORY_ROOT / "frame-interval-mutations.tsv"
GLOBAL_OUTPUT = INVENTORY_ROOT / "time-global-accesses.tsv"
SLEEP_OUTPUT = INVENTORY_ROOT / "sleep-callsites.tsv"
WAIT_OUTPUT = INVENTORY_ROOT / "time-wait-rules.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    IMPORT_PATH: "32f24fe9cde5b40b0dbb003363f427ac8420a3cdceac2b88b0f402b46577e375",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")

TIME_GET_CALLS = (0x0040A5A3, 0x00411958, 0x00411C10)
CRT_TIME_CALLS = (
    0x00407EF3, 0x00407F26, 0x004089B2, 0x0040A00C,
    0x0040A018, 0x0040A596, 0x004347BA, 0x004492C3,
)
INTERVAL_CLEAR_CALLS = (
    0x00403135, 0x0040AB6D, 0x0040C147,
    0x0042A815, 0x0042AD61, 0x0044CBA7,
)
INTERVAL_SET_CALLS = (
    0x0040ACFC, 0x0040CDBB, 0x00415737,
    0x00424E0C, 0x0042A81C, 0x0042AD68,
)
SLEEP_CALLS = (
    0x00403050, 0x004030D8, 0x0040318F, 0x004031E7,
    0x00403263, 0x004032C6, 0x004033CF, 0x004034B7,
    0x004034FC, 0x00406EF3, 0x00406F1D, 0x00408190,
    0x00409565, 0x0040A09C, 0x0040A58C, 0x00424C53,
    0x0042AAA2, 0x0045D95F, 0x0045D988, 0x0045D9E7,
    0x0045DA65, 0x0045DAB5, 0x0045DB0B, 0x0045DB4D,
    0x0045DC9C, 0x0045F72E, 0x0045F7CD,
)
EXPECTED_SLEEP_OPERANDS = (
    "1F4h", "15Eh", "1F4h", "1F4h", "12Ch", "12Ch", "0FAh",
    "1F4h", "0C8h", "1F4h", "1F4h", "258h", "96h", "ebp",
    "ebp", "400h", "0FAh", "0C8h", "0C8h", "0C8h", "0C8h",
    "64h", "0C8h", "0C8h", "0C8h", "32h", "14h",
)
EXPECTED_INTERVAL_CLEAR_OPERANDS = ("edx", "eax", "ecx", "edx", "ecx", "edx")
EXPECTED_INTERVAL_SET_OPERANDS = ("23h", "23h", "23h", "23h", "46h", "23h")
EXPECTED_TIME_GLOBAL_ACCESS_COUNT = 42

TIME_GLOBALS = {
    "dword_4A99FC": "current CRT epoch-second sample",
    "dword_4AAECC": "current timeGetTime sample",
    "dword_4B7BCC": "frame interval threshold",
    "dword_4C8444": "previous accepted-frame time sample for delta",
    "dword_4C8448": "ordinary-world end subtraction result",
    "dword_4C844C": "current accepted-frame time snapshot",
    "dword_4CB224": "input normalization clock snapshot",
    "dword_4CB23C": "last normalized input clock mirror",
    "dword_4CC2B0": "previous accepted-frame gate time",
    "dword_4CF6B0": "story wait duration",
    "dword_4CF6B4": "story wait start time",
    "dword_49E1C8": "accepted-frame delta used by debug FPS division",
}
TIME_GLOBAL_RE = re.compile(r"\b(" + "|".join(map(re.escape, TIME_GLOBALS)) + r")\b")

EXPECTED_SNIPPETS = {
    # Main frame gate and accepted-frame snapshots.
    0x0040A596: "call _time",
    0x0040A5A3: "call ds:timeGetTime",
    0x0040A5B7: "mov dword_4AAECC, eax",
    0x0040A5BC: "sub ecx, esi",
    0x0040A5BE: "cmp ecx, edx",
    0x0040A5C0: "jb loc_40AB2C",
    0x0040A5CC: "mov dword_4CC2B0, eax",
    0x0040A72D: "sub ecx, edx",
    0x0040A72F: "mov dword_4C844C, eax",
    0x0040A734: "mov dword_49E1C8, ecx",
    0x0040A73F: "mov dword_4C8444, eax",
    0x0040AAA0: "sub ecx, eax",
    0x0040AAA2: "mov dword_4C8448, ecx",
    # Interval helpers are plain stores, not multimedia timer creation.
    0x0040DD20: "mov eax, [esp+arg_0]",
    0x0040DD24: "mov dword_4B7BCC, eax",
    0x0040DD29: "mov eax, 1",
    0x0040DD30: "mov dword_4B7BCC, 0",
    0x0040DD3A: "mov eax, 1",
    0x00424E0A: "push 23h",
    0x00424E0C: "call sub_40DD20",
    0x00424E14: "test eax, eax",
    # Input chain and story/action waits all use the accepted-frame sample.
    0x004053ED: "sub esi, ecx",
    0x004053EF: "cmp esi, 96h",
    0x004053F6: "jbe short loc_405420",
    0x00429B9C: "sub edx, ecx",
    0x00429B9E: "cmp edx, eax",
    0x00429BA0: "jbe loc_42B0AE",
    0x0042F266: "sub edx, eax",
    0x0042F268: "mov eax, 51EB851Fh",
    0x0042F26F: "shr edx, 5",
    0x0042F274: "jbe loc_42F310",
    # Direct CD-check polling thresholds.
    0x0041196A: "sub ecx, edx",
    0x0041196C: "cmp ecx, eax",
    0x0041196E: "jbe loc_411B13",
    0x00411B1B: "cmp eax, 64h",
    0x00411B1E: "jbe short loc_411B29",
    0x00411C18: "sub ecx, edi",
    0x00411C1A: "cmp ecx, 1F4h",
    0x00411C20: "ja short loc_411C34",
    0x00411C24: "sub edx, esi",
    0x00411C26: "cmp edx, 5Ah",
    0x00411C29: "jbe short loc_411C10",
}


@dataclass(frozen=True)
class Instruction:
    address: int
    text: str
    owner: str


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

    imports = IMPORT_PATH.read_text(encoding="utf-8", errors="replace")
    forbidden = ("timeBeginPeriod", "timeEndPeriod", "timeSetEvent", "timeKillEvent")
    if "timeGetTime" not in imports or any(name in imports for name in forbidden):
        raise SystemExit("multimedia time import set changed")


def calls(instructions: list[Instruction], target: str) -> tuple[int, ...]:
    return tuple(item.address for item in instructions if item.text == f"call {target}")


def verify_assembly(instructions: list[Instruction], by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: expected {expected!r}, got {actual!r}"
            )
    if calls(instructions, "ds:timeGetTime") != TIME_GET_CALLS:
        raise SystemExit(f"timeGetTime calls changed: {calls(instructions, 'ds:timeGetTime')}")
    if calls(instructions, "_time") != CRT_TIME_CALLS:
        raise SystemExit(f"CRT time calls changed: {calls(instructions, '_time')}")
    if calls(instructions, "sub_40DD30") != INTERVAL_CLEAR_CALLS:
        raise SystemExit(f"interval clear calls changed: {calls(instructions, 'sub_40DD30')}")
    if calls(instructions, "sub_40DD20") != INTERVAL_SET_CALLS:
        raise SystemExit(f"interval set calls changed: {calls(instructions, 'sub_40DD20')}")


def nearest_push(instructions: list[Instruction], call_address: int) -> str:
    index_by_address = {item.address: index for index, item in enumerate(instructions)}
    index = index_by_address[call_address]
    for item in reversed(instructions[max(0, index - 14):index]):
        if item.text.startswith("call "):
            break
        if item.text.startswith("push "):
            return item.text[5:]
    return ""


def build_source_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    by_address = {item.address: item for item in instructions}
    contracts = {
        0x0040A5A3: ("timeGetTime", "u32 milliseconds", "main-frame attempt; stored in 0x004AAECC before interval gate"),
        0x00411958: ("timeGetTime", "u32 milliseconds", "CD/file checker polling loop and AIL service cadence"),
        0x00411C10: ("timeGetTime", "u32 milliseconds", "CD/file checker success hold busy-wait"),
        0x00407EF3: ("CRT time", "epoch seconds", "save path difftime against Time2 and accumulate play seconds"),
        0x00407F26: ("CRT time", "epoch seconds", "save path refreshes Time2 after accumulation"),
        0x004089B2: ("CRT time", "epoch seconds", "load path initializes Time2"),
        0x0040A00C: ("CRT time", "epoch seconds", "seed CRT-compatible random state"),
        0x0040A018: ("CRT time", "epoch seconds", "seed independent 250-dword xor random state"),
        0x0040A596: ("CRT time", "epoch seconds", "main-frame attempt writes 0x004A99FC before millisecond gate"),
        0x004347BA: ("CRT time", "epoch seconds", "reseed CRT-compatible random state on resource path"),
        0x004492C3: ("CRT time", "epoch seconds", "mode path resets play-time origin Time2"),
    }
    rows = []
    for address in sorted(contracts):
        item = by_address[address]
        source, unit, contract = contracts[address]
        rows.append((f"0x{address:08X}", item.owner, source, unit, contract))
    return rows


def build_interval_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    by_address = {item.address: item for item in instructions}
    roles = {
        0x00403135: "world/debug shutdown path; clear throttle before close request",
        0x0040AB6D: "display deactivate/reactivate entry; old value push is discarded",
        0x0040ACFC: "display reactivation completion restores literal 35",
        0x0040C147: "map/session synchronous setup entry; old value push is discarded",
        0x0040CDBB: "map/session synchronous setup completion restores literal 35",
        0x00415737: "state cleanup completion writes literal 35",
        0x00424E0C: "main initialization writes literal 35; helper always returns 1",
        0x0042A815: "story opcode 96 clears before changing interval",
        0x0042A81C: "story opcode 96 writes literal 70",
        0x0042AD61: "story opcode 97 clears before changing interval",
        0x0042AD68: "story opcode 97 writes literal 35",
        0x0044CBA7: "special-mode exit clears throttle; old value push is discarded",
    }
    rows = []
    for address in sorted(INTERVAL_CLEAR_CALLS + INTERVAL_SET_CALLS):
        item = by_address[address]
        operand = nearest_push(instructions, address)
        if address in INTERVAL_CLEAR_CALLS:
            helper = "sub_40DD30"
            effect = "0"
            stack_contract = "preceding push is not consumed; helper has plain retn"
        else:
            helper = "sub_40DD20"
            effect = {"23h": "35", "46h": "70"}.get(operand, operand)
            stack_contract = "one u32 argument; helper has plain retn and caller cleans stack"
        rows.append(
            (f"0x{address:08X}", item.owner, helper, operand, effect,
             stack_contract, roles[address])
        )
    clear_operands = tuple(row[3] for row in rows if row[2] == "sub_40DD30")
    set_operands = tuple(row[3] for row in rows if row[2] == "sub_40DD20")
    if clear_operands != EXPECTED_INTERVAL_CLEAR_OPERANDS:
        raise SystemExit(f"interval clear stack operands changed: {clear_operands}")
    if set_operands != EXPECTED_INTERVAL_SET_OPERANDS:
        raise SystemExit(f"interval set operands changed: {set_operands}")
    return rows


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


def build_global_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    rows = []
    for item in instructions:
        if item.address >= 0x00499000:
            continue
        for match in TIME_GLOBAL_RE.finditer(item.text):
            symbol = match.group(1)
            rows.append(
                (f"0x{item.address:08X}", item.owner, symbol,
                 TIME_GLOBALS[symbol], classify_access(item.text, symbol), item.text)
            )
    return rows


def build_sleep_rows(instructions: list[Instruction]) -> list[tuple[object, ...]]:
    by_address = {item.address: item for item in instructions}
    rows = []
    for address in SLEEP_CALLS:
        item = by_address[address]
        operand = nearest_push(instructions, address)
        milliseconds = {"ebp": "0"}.get(operand, str(int(operand[:-1], 16)) if operand.endswith("h") else operand)
        contract = "scheduler yield; zero delay" if operand == "ebp" else "blocking wall-clock delay"
        rows.append((f"0x{address:08X}", item.owner, operand, milliseconds, contract, item.text))
    operands = tuple(row[2] for row in rows)
    if operands != EXPECTED_SLEEP_OPERANDS:
        raise SystemExit(f"Sleep operands changed: {operands}")
    return rows


def wait_rows() -> list[tuple[object, ...]]:
    return [
        ("0x0040A5BC", "sub_40A570", "timeGetTime", "u32(now - previous_gate)", ">= frame_interval", "accept exactly one frame; previous_gate=now; no catch-up"),
        ("0x004053ED", "sub_4053C0", "accepted-frame time sample", "u32(now - release_time)", "> 150", "reset rapid-press chain only while raw remains released"),
        ("0x00429B9C", "sub_427920 opcode 67", "accepted-frame time sample", "u32(now - wait_start)", "> u16 duration", "clear in-stream active bit and advance story instruction"),
        ("0x0042F266", "sub_42ED40", "accepted-frame time sample", "floor(u32(now - object_start) / 100)", "> object threshold", "advance object/UI timed state; 0xFFFF is sentinel"),
        ("0x0041196A", "sub_4118B0", "direct timeGetTime", "u32(now - last_scan)", "> 1000 or > 3000", "perform next CD/file scan; threshold depends on sequence length"),
        ("0x00411B19", "sub_4118B0", "direct timeGetTime", "u32(now - last_AIL_service)", "> 100", "call AIL_serve while waiting between scans"),
        ("0x00411C18", "sub_4118B0", "direct timeGetTime", "u32(now - success_start)", "> 500", "leave success-message busy-wait"),
        ("0x00411C24", "sub_4118B0", "direct timeGetTime", "u32(now - last_AIL_service)", "> 90", "call AIL_serve during success-message busy-wait"),
        ("0x00405372", "sub_4050E0", "normalization sample count", "counter += 1 when x/y stable and raw mouse mask zero", "> 450", "set inactivity bit 9 first on sample 451; not wall-clock based"),
    ]


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
    sources = build_source_rows(instructions)
    intervals = build_interval_rows(instructions)
    globals_ = build_global_rows(instructions)
    sleeps = build_sleep_rows(instructions)
    waits = wait_rows()

    if len(sources) != 11 or len(intervals) != 12 or len(sleeps) != 27 or len(waits) != 9:
        raise SystemExit(
            f"timing inventory size changed: {len(sources)} sources, "
            f"{len(intervals)} interval mutations, {len(sleeps)} Sleep calls, "
            f"{len(waits)} wait rules"
        )
    if len(globals_) != EXPECTED_TIME_GLOBAL_ACCESS_COUNT:
        raise SystemExit(f"time-global access count changed: {len(globals_)}")

    write_table(SOURCE_OUTPUT, ("call_address", "owner", "source", "unit", "contract"), sources)
    write_table(INTERVAL_OUTPUT, ("call_address", "owner", "helper", "stack_operand", "resulting_interval_ms", "stack_contract", "role"), intervals)
    write_table(GLOBAL_OUTPUT, ("instruction_address", "owner", "symbol", "role", "access", "instruction"), globals_)
    write_table(SLEEP_OUTPUT, ("call_address", "owner", "stack_operand", "milliseconds", "contract", "instruction"), sleeps)
    write_table(WAIT_OUTPUT, ("instruction_address", "owner", "clock", "calculation", "predicate", "effect"), waits)
    print(
        f"wrote {len(sources)} time sources, {len(intervals)} interval mutations, "
        f"{len(globals_)} time-global accesses, {len(sleeps)} Sleep calls, "
        f"and {len(waits)} wait rules"
    )


if __name__ == "__main__":
    main()
