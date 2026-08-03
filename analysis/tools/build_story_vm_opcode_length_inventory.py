#!/usr/bin/env python3
"""Build the first assembly-locked story opcode encoding/length layer.

The output is intentionally narrower than a full semantic opcode catalog.  It
records how the interpreter locates the next executed bytes (or transfers to a
new script window), including variable terminators and assembly bugs.  Generic
fixed-width operand fields remain unnamed until their handler data flow is
audited.
"""

from __future__ import annotations

import csv
import hashlib
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
TRIAGE_INPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "story-vm-opcode-static-triage.tsv"
)
OUTPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "story-vm-opcode-length-rules.tsv"
)

EXPECTED_ASM_SHA256 = "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def special_rule(opcode: int) -> tuple[str, str, str, str, str, str, str] | None:
    percent_q = (
        "byte_string_percent_q",
        "4 + byte_count_before_0x5125",
        "",
        "byte string begins at +2; terminating bytes 25 51 are consumed",
        "sequential_after_delimiter",
        "0x00427DFF-0x00427E2A;0x00484814-0x00484850;0x0042BD51-0x0042BD92",
        "no bounds check; odd byte lengths are accepted",
    )
    rules: dict[int, tuple[str, str, str, str, str, str, str]] = {}
    for value in (1, 2, 3, 4, 5, 6, 89, 90):
        rules[value] = percent_q
    rules.update(
        {
            0: (
                "default_no_advance", "0", "0", "none", "beep_and_yield_same_ip",
                "0x0042D230-0x0042D249", "default error behavior, not a NOP",
            ),
            15: (
                "absolute_stream_transfer_u32", "control_transfer; physical footprint 6", "6",
                "u32 target_relative at +2", "sub_42E430_same_file_target_ip_zero",
                "0x00428310-0x00428313;0x0042E430-0x0042E47B", "no sequential advance",
            ),
            20: (
                "counted_records_6byte", "4 + 6*u14_count", "",
                "u16 count/flags at +2; handler reads count records at 6-byte stride",
                "wait_until_ready_then_sequential", "0x0042ADB7-0x0042B06E",
                "first pass sets operand bit 14 and yields; ready pass clears flags and advances with the opcode-20 6-byte stride selected at 0x0042B030",
            ),
            23: (
                "u16_list_ff00_then_u32_target", "8 + 2*item_count", "",
                "u16 items at +2; u16 FF00; u32 branch target", "conditional_transfer_or_sequential",
                "0x0042857F-0x004285E8", "item scan has no bounds check",
            ),
            24: (
                "u16_list_ff00_then_u32_target", "8 + 2*item_count", "",
                "u16 items at +2; u16 FF00; u32 branch target", "conditional_transfer_or_sequential",
                "0x004285ED-0x00428656", "item scan has no bounds check",
            ),
            29: ("fixed_shared_numeric", "6", "6", "s16 index; s16 value", "sequential", "0x0042B074-0x0042B1EC", "shared handler refines by opcode"),
            30: ("fixed_shared_numeric", "6", "6", "s16 index; s16 value", "sequential", "0x0042B074-0x0042B1EC", "shared handler refines by opcode"),
            31: ("fixed_shared_numeric", "6", "6", "s16 index; s16 value", "sequential", "0x0042B074-0x0042B1EC", "shared handler refines by opcode"),
            32: ("fixed_shared_numeric", "10", "10", "s16 index; s16 value; u32 branch target", "conditional_transfer_or_sequential", "0x0042B074-0x0042B1EC", "shared handler refines by opcode"),
            33: ("fixed_shared_numeric", "10", "10", "s16 index; s16 value; u32 branch target", "conditional_transfer_or_sequential", "0x0042B074-0x0042B1EC", "shared handler refines by opcode"),
            41: (
                "dword_target_list_ff00ff00", "control_transfer; footprint 6 + 4*target_count", "",
                "u32 targets at +2 followed by u32 FF00FF00", "indexed_sub_42E430_transfer",
                "0x00428C9F-0x00428D13", "global index selects one target; no sequential advance",
            ),
            50: ("fixed_opcode_refined", "10", "10", "four u16 operands", "sequential_after_setup", "0x00429066-0x0042935D", "shared entry computes length from opcode-specific constant 3"),
            63: (
                "u16_prefix_list_ff00", "6 + 2*item_count", "",
                "u16 prefix at +2; u16 items at +4; u16 FF00", "sequential",
                "0x00429A1B-0x00429ACD", "list is copied into a fixed global array; separate overflow behavior pending",
            ),
            70: ("fixed_opcode_refined", "10", "10", "four u16 operands", "sequential_after_setup", "0x00429066-0x0042935D", "shared entry computes length from opcode-specific constant 3"),
            73: ("fixed_opcode_refined", "8", "8", "three u16 operands", "sequential_after_setup", "0x00429066-0x0042935D", "shared entry computes length from opcode-specific constant 2"),
            77: (
                "fixed_with_stale_failure_advance", "6 on successful role lookup", "6",
                "u16 role/context; u16 value", "sequential", "0x00429F7B-0x0042A0A1",
                "failed lookup reaches advance through uninitialized stack var_40; preserve as abnormal bug",
            ),
            78: (
                "fixed_with_stale_failure_advance", "4 on successful role lookup", "4",
                "u16 role/context", "sequential", "0x00429F7B-0x0042A0A1",
                "failed lookup reaches advance through uninitialized stack var_40; preserve as abnormal bug",
            ),
            85: (
                "byte_string_percent_q_external", "4 + byte_count_before_0x5125", "",
                "byte string begins at +2; terminating bytes 25 51 are consumed", "sub_484730_advances_caller_ip_and_pointer",
                "0x0042A611-0x0042A66E;0x0048480C-0x00484850", "video filename extension may be rewritten after parsing",
            ),
            87: (
                "dword_target_list_ff00ff00", "control_transfer; footprint 6 + 4*target_count", "",
                "u32 targets at +2 followed by u32 FF00FF00", "random_sub_42E430_transfer",
                "0x0042A6CB-0x0042A722", "random index selects one target; no sequential advance",
            ),
            96: (
                "byte_string_percent_q_with_prefix_flags", "4 + byte_count_before_0x5125", "",
                "byte string at +2; optional leading 25/2A set flags; terminating 25 51 consumed",
                "sequential_then_open_custom_ani", "0x0042A80E-0x0042AD37",
                "scan is byte-wise and unbounded; copied staging area is only 0x400 bytes",
            ),
            109: (
                "counted_u16_list", "4 + 2*u16_count", "", "u16 count; count u16 ids",
                "sequential", "0x0042B63C-0x0042B6A0", "zero count is accepted",
            ),
            116: (
                "counted_6byte_records", "4 + 6*u16_count", "", "u16 count; count records of 3*u16",
                "sequential", "0x0042B83A-0x0042B8E1", "state IP receives the low 16 bits of the product; current samples use small counts",
            ),
            125: (
                "byte_string_percent_q", "4 + byte_count_before_0x5125", "",
                "byte string begins at +2; terminating bytes 25 51 are consumed", "sequential_after_delimiter",
                "0x0042BD51-0x0042BDB7", "copied into 0x100-byte allocation without a bound check",
            ),
            133: (
                "u16_zero_terminated", "4 + 2*item_count", "", "nonzero u16 items at +2; terminating u16 zero",
                "sequential", "0x0042C234-0x0042C2C1", "copied into 0x100-byte allocation without a count bound",
            ),
            158: (
                "u16_prefix_then_byte_string_percent_q", "6 + byte_count_before_0x5125", "",
                "u16 prefix at +2; byte string at +4; terminating bytes 25 51 consumed", "sequential_after_file_operation",
                "0x0042CA7C-0x0042CBAB", "byte string copied to fixed FileName buffer without local bound check",
            ),
            159: (
                "u16_prefix_then_byte_string_percent_q", "6 + byte_count_before_0x5125", "",
                "u16 prefix at +2; byte string at +4; terminating bytes 25 51 consumed", "sequential_after_file_operation",
                "0x0042CA7C-0x0042CBAB", "byte string copied to fixed FileName buffer without local bound check",
            ),
            161: (
                "story_id_transfer", "control transfer; physical footprint 4", "4", "s16 story id at +2",
                "sub_42E480_select_file_and_reset_ip", "0x0042CBCC-0x0042CBF5;0x0042E480-0x0042E594",
                "next bytes come from newly selected TALK file/slot, not sequential stream",
            ),
            169: (
                "counted_records_12byte", "4 + 12*u14_count", "",
                "u16 count/flags at +2; count records of 12 bytes", "wait_until_ready_then_sequential",
                "0x0042ADB7-0x0042B06E", "count is low 14 bits; bit 14 selects wait/preflight branch",
            ),
            170: ("fixed_even_variant", "2", "2", "none", "sequential", "0x0042CCF7-0x0042CDA1", "even opcode skips embedded string"),
            171: (
                "byte_string_percent_q", "4 + byte_count_before_0x5125", "",
                "byte string at +2; terminating bytes 25 51 consumed", "sequential",
                "0x0042CCF7-0x0042CDA1", "odd opcode embeds string",
            ),
            172: ("fixed_even_variant", "2", "2", "none", "sequential", "0x0042CCF7-0x0042CDA1", "even opcode skips embedded string"),
            173: (
                "byte_string_percent_q", "4 + byte_count_before_0x5125", "",
                "byte string at +2; terminating bytes 25 51 consumed", "sequential",
                "0x0042CCF7-0x0042CDA1", "odd opcode embeds string",
            ),
            181: ("fixed_shared_numeric", "8", "8", "s16 index; s32 value", "sequential", "0x0042B070-0x0042B1EC", "shared handler refines by opcode"),
            182: ("fixed_shared_numeric", "8", "8", "s16 index; s32 value", "sequential", "0x0042B070-0x0042B1EC", "shared handler refines by opcode"),
            183: ("fixed_shared_numeric", "8", "8", "s16 index; s32 value", "sequential", "0x0042B070-0x0042B1EC", "shared handler refines by opcode"),
            184: ("fixed_shared_numeric", "12", "12", "s16 index; s32 value; u32 branch target", "conditional_transfer_or_sequential", "0x0042B070-0x0042B1EC", "shared handler refines by opcode"),
            185: ("fixed_shared_numeric", "12", "12", "s16 index; s32 value; u32 branch target", "conditional_transfer_or_sequential", "0x0042B070-0x0042B1EC", "shared handler refines by opcode"),
            1024: ("fixed_control", "2", "2", "none", "advance_and_continue_same_call", "0x0042D200-0x0042D214", "table-external special"),
            1025: ("fixed_control", "2", "2", "none", "advance_and_yield", "0x0042D49F-0x0042D4B1", "table-external special"),
            1026: ("fixed_control", "2", "2", "none", "advance_and_continue_same_call", "0x0042D1EA-0x0042D1FB", "table-external special"),
            16383: ("terminator", "2", "2", "none", "TalkEnd_cleanup", "0x0042D24E-0x0042D49A", "does not need a sequential successor"),
        }
    )
    return rules.get(opcode)


def load_triage() -> list[dict[str, str]]:
    if not TRIAGE_INPUT.exists():
        raise SystemExit("run build_story_vm_opcode_static_triage.py first")
    with TRIAGE_INPUT.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    if len(rows) != 198:
        raise SystemExit(f"unexpected triage row count: {len(rows)}")
    return rows


def build_rows() -> list[tuple[object, ...]]:
    rows = []
    for triage in load_triage():
        opcode = int(triage["effective_opcode"])
        rule = special_rule(opcode)
        if rule is None:
            fixed_values = [value for value in triage["fixed_add_values"].split(",") if value]
            if len(fixed_values) != 1:
                raise SystemExit(
                    f"opcode {opcode} needs an explicit length rule: "
                    f"triage={triage['triage']}, fixed={fixed_values}"
                )
            length = fixed_values[0]
            rule = (
                "fixed_raw_operands",
                length,
                length,
                f"{int(length) - 2} raw operand bytes; field names pending data-flow audit",
                "handler_specific_sequential_or_wait",
                triage["ip_mutation_sites"],
                "width confirmed; parameter meaning/state effects not yet complete",
            )
        encoding, formula, fixed, operands, control, evidence, caveat = rule
        rows.append(
            (
                opcode,
                triage["entry_target"],
                encoding,
                formula,
                fixed,
                operands,
                control,
                evidence,
                "assembly_confirmed",
                caveat,
            )
        )
    if len(rows) != 198 or len({row[0] for row in rows}) != 198:
        raise SystemExit("opcode length rows are incomplete")
    return rows


def write_tsv(rows: list[tuple[object, ...]]) -> None:
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "effective_opcode", "entry_target", "encoding_class",
                "sequential_length_rule", "normal_fixed_length", "operand_shape",
                "control_effect", "assembly_evidence", "confidence", "caveat",
            )
        )
        writer.writerows(rows)


def main() -> None:
    actual = sha256(ASM_PATH)
    if actual != EXPECTED_ASM_SHA256:
        raise SystemExit(f"assembly changed: {actual}")
    rows = build_rows()
    write_tsv(rows)
    variable = sum(not row[4] for row in rows)
    transfers = sum("transfer" in str(row[6]) for row in rows)
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(rows)} rows)")
    print(f"variable/control-derived widths: {variable}")
    print(f"explicit transfer rules: {transfers}")


if __name__ == "__main__":
    main()
