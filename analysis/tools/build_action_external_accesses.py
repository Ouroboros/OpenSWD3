#!/usr/bin/env python3
"""Extract ActionRecord accesses from regions with a proven action base register.

The updater itself is intentionally excluded.  Each region either receives an
already-classified ActionRecord pointer argument or forms the role's embedded
ActionRecord address explicitly.  This avoids unsafe global [register+offset]
matching across unrelated structures.
"""

from __future__ import annotations

import csv
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASSEMBLY = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
OUTPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "action-external-accesses.tsv"
)

ACTION_SIZE = 0x98
EXPECTED_REGION_COUNT = 14
EXPECTED_ACCESS_COUNT = 201
EXPECTED_RAW_OFFSET_COUNT = 26

INSTRUCTION_RE = re.compile(r"^([0-9A-F]{8})\s+([a-z][a-z0-9]*)\s*(.*)$")
PTR_WIDTH_RE = re.compile(r"\b(byte|word|dword|qword) ptr\b", re.IGNORECASE)
PTR_WIDTHS = {"byte": 1, "word": 2, "dword": 4, "qword": 8}
REGISTER_WIDTHS = {
    **{name: 1 for name in ("al", "ah", "bl", "bh", "cl", "ch", "dl", "dh")},
    **{name: 2 for name in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp")},
    **{name: 4 for name in ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")},
}


@dataclass(frozen=True)
class ProvenRegion:
    owner_function: str
    start: int
    end: int
    register: str
    proof_anchor: int
    context: str


REGIONS = (
    ProvenRegion(
        "0040EBF0:sub_40EBF0",
        0x0040EBF1,
        0x0040EC75,
        "esi",
        0x0040EBF1,
        "函数参数已由全调用点目录确认是 ActionRecord*；ESI 到函数结束前未改写",
    ),
    ProvenRegion(
        "0040ECC0:sub_40ECC0",
        0x0040ECC1,
        0x0040ED53,
        "esi",
        0x0040ECC1,
        "函数参数已由全调用点目录确认是 ActionRecord*；ESI 到函数结束前未改写",
    ),
    ProvenRegion(
        "0040F6D0:sub_40F6D0",
        0x0040F805,
        0x0040F861,
        "edi",
        0x0040F805,
        "EDI 由 0x004BABE8 加角色索引乘 0xD8 形成角色内嵌 ActionRecord 基址",
    ),
    ProvenRegion(
        "0043B080:sub_43B080",
        0x0043B081,
        0x0043B10C,
        "esi",
        0x0043B081,
        "函数参数已由全调用点目录确认是 ActionRecord*；ESI 到函数结束前未改写",
    ),
    ProvenRegion(
        "00449C30:sub_449C30",
        0x00449C34,
        0x00449D71,
        "edi",
        0x00449C34,
        "函数参数已由全调用点目录确认是 ActionRecord*；EDI 到函数结束前未改写",
    ),
    ProvenRegion(
        "00451420:sub_451420",
        0x00451436,
        0x0045153B,
        "edi",
        0x00451436,
        "ECX 指向以 ActionRecord 为首成员、至少使用到 +0xC0 的包装记录；EDI 保持对象起点，扫描严格截断于 0x98",
    ),
    ProvenRegion(
        "00451540:sub_451540",
        0x00451541,
        0x004515D7,
        "esi",
        0x00451541,
        "ECX 指向以 ActionRecord 为首成员、至少使用到 +0xC0 的包装记录；ESI 保持对象起点，扫描严格截断于 0x98",
    ),
    ProvenRegion(
        "004515E0:sub_4515E0",
        0x004515E9,
        0x0045171F,
        "esi",
        0x004515E9,
        "ECX 指向以 ActionRecord 为首成员、至少使用到 +0xC0 的包装记录；ESI 保持对象起点，扫描严格截断于 0x98",
    ),
    ProvenRegion(
        "0047F940:sub_47F940",
        0x0047F945,
        0x0047FC31,
        "ebp",
        0x0047F945,
        "函数第二参数已由全调用点目录确认是 ActionRecord*；EBP 到函数结束前未改写",
    ),
    ProvenRegion(
        "004831C0:sub_4831C0",
        0x004831C7,
        0x004836F7,
        "edi",
        0x004831C7,
        "函数第二参数已由全调用点目录确认是 ActionRecord*；EDI 到函数结束前未改写",
    ),
    ProvenRegion(
        "004838D0:sub_4838D0",
        0x004838D7,
        0x00483B27,
        "edi",
        0x004838D7,
        "函数第二参数已由全调用点目录确认是 ActionRecord*；EDI 到函数结束前未改写",
    ),
    ProvenRegion(
        "00483B30:sub_483B30",
        0x00483B33,
        0x00483DA3,
        "edi",
        0x00483B33,
        "函数参数已由全调用点目录确认是 ActionRecord*；EDI 到函数结束前未改写",
    ),
    ProvenRegion(
        "00483DB0:sub_483DB0",
        0x00483DB7,
        0x00483FBF,
        "edi",
        0x00483DB7,
        "函数第二参数已由全调用点目录确认是 ActionRecord*；EDI 到函数结束前未改写",
    ),
    ProvenRegion(
        "00484230:sub_484230",
        0x00484232,
        0x004844F3,
        "edi",
        0x00484232,
        "函数第三参数已由全调用点目录确认是 ActionRecord*；EDI 到函数结束前未改写",
    ),
)


def split_operands(text: str) -> list[str]:
    return [operand.strip() for operand in text.split(",") if operand.strip()]


def parse_displacement(expression: str, register: str) -> int | None:
    compact = re.sub(r"\s+", "", expression.lower())
    match = re.fullmatch(rf"{register}(?:([+-])([0-9a-f]+h?))?", compact)
    if not match:
        return None
    if match.group(2) is None:
        return 0
    token = match.group(2)
    value = int(token[:-1], 16) if token.endswith("h") else int(token, 10)
    return -value if match.group(1) == "-" else value


def find_register_displacement(operand: str, register: str) -> int | None:
    for expression in re.findall(r"\[([^]]+)\]", operand):
        displacement = parse_displacement(expression, register)
        if displacement is not None:
            return displacement
    return None


def infer_width(operand: str, operands: list[str], mnemonic: str) -> int | None:
    if mnemonic == "lea":
        return None
    ptr_width = PTR_WIDTH_RE.search(operand)
    if ptr_width:
        return PTR_WIDTHS[ptr_width.group(1).lower()]
    for other_operand in operands:
        if other_operand == operand:
            continue
        for token in re.findall(r"\b[a-z][a-z0-9]*\b", other_operand.lower()):
            if token in REGISTER_WIDTHS:
                return REGISTER_WIDTHS[token]
    raise SystemExit(f"cannot infer memory width: {operand!r} in {operands!r}")


def classify(mnemonic: str, operand_index: int, operand: str) -> str:
    if mnemonic == "lea" or re.search(r"\boffset\b", operand):
        return "address"
    if operand_index > 0 or mnemonic in {"cmp", "test", "push", "call"}:
        return "read"
    if mnemonic in {
        "mov",
        "movzx",
        "movsx",
        "seta",
        "setae",
        "setb",
        "setbe",
        "sete",
        "setne",
    }:
        return "write"
    return "read_write"


def main() -> None:
    if len(REGIONS) != EXPECTED_REGION_COUNT:
        raise SystemExit("unexpected action external proof-region count")

    lines = ASSEMBLY.read_text(encoding="utf-8", errors="replace").replace("\r", "").splitlines()
    instructions: dict[int, tuple[str, str, str]] = {}
    for line in lines:
        code = line.split(";", 1)[0].rstrip()
        instruction = INSTRUCTION_RE.match(code)
        if instruction:
            address_text, mnemonic, operand_text = instruction.groups()
            instructions[int(address_text, 16)] = (mnemonic, operand_text, code)

    missing_anchors = [region.proof_anchor for region in REGIONS if region.proof_anchor not in instructions]
    if missing_anchors:
        rendered = ", ".join(f"{address:08X}" for address in missing_anchors)
        raise SystemExit(f"proof anchors missing from assembly: {rendered}")

    rows: list[tuple[object, ...]] = []
    for region in REGIONS:
        for address in sorted(instructions):
            if not region.start <= address <= region.end:
                continue
            mnemonic, operand_text, code = instructions[address]
            operands = split_operands(operand_text)
            for operand_index, operand in enumerate(operands):
                offset = find_register_displacement(operand, region.register)
                if offset is None or not 0 <= offset < ACTION_SIZE:
                    continue
                width = infer_width(operand, operands, mnemonic)
                rows.append(
                    (
                        f"{offset:02X}",
                        "" if width is None else width,
                        classify(mnemonic, operand_index, operand),
                        f"{address:08X}",
                        region.owner_function,
                        region.register.upper(),
                        f"{region.proof_anchor:08X}",
                        region.context,
                        code,
                    )
                )

    rows.sort(key=lambda row: (int(str(row[0]), 16), str(row[3]), str(row[4])))
    offsets = {row[0] for row in rows}
    if len(rows) != EXPECTED_ACCESS_COUNT or len(offsets) != EXPECTED_RAW_OFFSET_COUNT:
        raise SystemExit(
            "unexpected external ActionRecord matrix size: "
            f"{len(rows)} accesses across {len(offsets)} raw offsets"
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "action_offset_hex",
                "access_width_bytes",
                "access_kind",
                "instruction_address",
                "owner_function",
                "base_register",
                "proof_anchor",
                "proof_note",
                "assembly_instruction",
            )
        )
        writer.writerows(rows)

    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(
        f"wrote {len(rows)} proven external ActionRecord accesses across "
        f"{len(offsets)} offsets and {len(REGIONS)} proof regions to {relative_output}"
    )


if __name__ == "__main__":
    main()
