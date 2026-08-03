#!/usr/bin/env python3
"""Extract RoleRecord +0x00..+0x3F accesses from proven register aliases.

This intentionally does not perform global register-name matching.  Every scanned
interval starts at an assembly instruction that proves the register's relationship
to a 0xD8-byte RoleRecord, and ends before that relationship is invalidated.
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
    / "role-front-alias-accesses.tsv"
)

ROLE_FRONT_END = 0x40
EXPECTED_ACCESS_COUNT = 221
EXPECTED_OFFSET_COUNT = 15

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
    register_role_delta: int
    proof_anchor: int
    proof_note: str


REGIONS = (
    ProvenRegion(
        "00402F80:sub_402F80",
        0x00402FA8,
        0x00403762,
        "ebx",
        0,
        0x00402FA8,
        "角色索引先乘 0xD8，再由 0x004BABA8 形成精确角色基址；到 0x00403763 改写 EBX 前为止",
    ),
    ProvenRegion(
        "00402F80:sub_402F80",
        0x004037BF,
        0x0040387E,
        "edi",
        0,
        0x004037BF,
        "从已保存的受控角色指针 var_B4 恢复精确角色基址",
    ),
    ProvenRegion(
        "00405430:sub_405430",
        0x00405445,
        0x004054FA,
        "esi",
        0x10,
        0x00405445,
        "ESI 从第二条角色记录 +0x10 开始并按 0xD8 遍历；字段偏移需加上该基址偏移",
    ),
    ProvenRegion(
        "00405500:sub_405500",
        0x00405522,
        0x004062B8,
        "ebp",
        0,
        0x00405521,
        "角色索引先乘 0xD8，再加 0x004BABA8；函数内没有后续 EBP 赋值",
    ),
    ProvenRegion(
        "0040D610:sub_40D610",
        0x0040D626,
        0x0040D784,
        "esi",
        0,
        0x0040D626,
        "角色索引乘 0xD8 后由 0x004BABA8 形成精确角色基址，ESI 保持到函数尾",
    ),
    ProvenRegion(
        "0040F280:sub_40F280",
        0x0040F281,
        0x0040F33B,
        "esi",
        0,
        0x0040F281,
        "函数参数直接装入 ESI；调用者传入角色记录，且 ESI 在函数内保持不变",
    ),
    ProvenRegion(
        "004120B0:sub_4120B0",
        0x00412121,
        0x0041214D,
        "ebx",
        0,
        0x00412121,
        "主角色索引乘 0xD8 后由 0x004BABA8 形成精确角色基址",
    ),
    ProvenRegion(
        "004120B0:sub_4120B0",
        0x004121D4,
        0x004124C2,
        "esi",
        0,
        0x004121D4,
        "事件角色编号乘 0xD8 后由 0x004BABA8 形成精确角色基址",
    ),
    ProvenRegion(
        "004120B0:sub_4120B0",
        0x0041246F,
        0x004128DF,
        "ebx",
        0,
        0x0041246F,
        "从 var_C 恢复 0x00412121 保存的主角色基址，后续无 EBX 改写",
    ),
    ProvenRegion(
        "004120B0:sub_4120B0",
        0x00412542,
        0x0041266B,
        "esi",
        0,
        0x00412542,
        "队友角色编号乘 0xD8 后由 0x004BABA8 形成精确角色基址",
    ),
    ProvenRegion(
        "00427300:sub_427300",
        0x0042732D,
        0x00427912,
        "esi",
        0,
        0x0042732D,
        "主角色索引乘 0xD8 后由 0x004BABA8 形成精确角色基址，ESI 保持到函数尾",
    ),
    ProvenRegion(
        "00427300:sub_427300",
        0x00427366,
        0x00427419,
        "edi",
        0x1E,
        0x00427366,
        "EDI 从第二条角色记录 +0x1E 开始并按 0xD8 遍历；字段偏移需减去该基址偏移",
    ),
    ProvenRegion(
        "00427300:sub_427300",
        0x004274D2,
        0x00427681,
        "edi",
        0,
        0x004274D2,
        "候选角色索引乘 0xD8 后由 0x004BABA8 形成精确角色基址；到返回或 0x00427682 改写前为止",
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
        tokens = re.findall(r"\b[a-z][a-z0-9]*\b", other_operand.lower())
        for token in tokens:
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
    seen_accesses: set[tuple[int, str, int]] = set()
    for region in REGIONS:
        for address in sorted(instructions):
            if not region.start <= address <= region.end:
                continue
            mnemonic, operand_text, code = instructions[address]
            operands = split_operands(operand_text)
            for operand_index, operand in enumerate(operands):
                displacement = find_register_displacement(operand, region.register)
                if displacement is None:
                    continue
                role_offset = region.register_role_delta + displacement
                if not 0 <= role_offset < ROLE_FRONT_END:
                    continue
                identity = (address, region.register, role_offset)
                if identity in seen_accesses:
                    continue
                seen_accesses.add(identity)
                width = infer_width(operand, operands, mnemonic)
                rows.append(
                    (
                        f"{role_offset:02X}",
                        "" if width is None else width,
                        classify(mnemonic, operand_index, operand),
                        f"{address:08X}",
                        region.owner_function,
                        region.register.upper(),
                        f"{region.register_role_delta:+#x}",
                        f"{region.proof_anchor:08X}",
                        region.proof_note,
                        code,
                    )
                )

    rows.sort(key=lambda row: (int(str(row[0]), 16), str(row[3]), str(row[5])))
    if not rows:
        raise SystemExit("no proven role-front alias accesses found")
    offsets = {row[0] for row in rows}
    if len(rows) != EXPECTED_ACCESS_COUNT or len(offsets) != EXPECTED_OFFSET_COUNT:
        raise SystemExit(
            "unexpected proven role-front matrix size: "
            f"{len(rows)} accesses across {len(offsets)} offsets"
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "role_offset_hex",
                "access_width_bytes",
                "access_kind",
                "instruction_address",
                "owner_function",
                "base_register",
                "register_role_delta_hex",
                "proof_anchor",
                "proof_note",
                "assembly_instruction",
            )
        )
        writer.writerows(rows)

    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(
        f"wrote {len(rows)} proven alias accesses across {len(offsets)} role-front offsets "
        f"and {len(REGIONS)} proof regions to {relative_output}"
    )


if __name__ == "__main__":
    main()
