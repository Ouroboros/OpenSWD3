#!/usr/bin/env python3
"""Inventory the DAT families according to their assembly loaders.

The nine MZ-prefixed files use a 0x200-byte DOS-MZ-style wrapper.  The
remaining DAT files are deliberately kept separate: sharing a suffix does not
make them one container format.
"""

from __future__ import annotations

import csv
import hashlib
import struct
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FILES_OUTPUT = INVENTORY_ROOT / "dat-files.tsv"
INDEX_OUTPUT = INVENTORY_ROOT / "dat-offset-index.tsv"
INDEX_SUMMARY_OUTPUT = INVENTORY_ROOT / "dat-offset-index-summary.tsv"
LOADER_OUTPUT = INVENTORY_ROOT / "dat-loader-contexts.tsv"
PLAIN_FIELDS_OUTPUT = INVENTORY_ROOT / "dat-plain-fields.tsv"

MZ_FILES = (
    ("FIGTALK.dat", 20612),
    ("LEVEL.DAT", 8206),
    ("MAPS.DAT", 162929),
    ("MON.DAT", 163731),
    ("PATH.DAT", 23114),
    ("TALK1.DAT", 371450),
    ("TALK2.DAT", 209206),
    ("TALK3.DAT", 211381),
    ("TALK4.DAT", 222652),
)

PLAIN_FILES = (
    ("Env.dat", 63, "environment_record"),
    ("Fame.dat", 1024, "fixed_512_u16_table"),
    ("Data/mcache.dat", 384, "fixed_24_x_16_cache_table"),
    ("swd3_dvd.dat", 384, "presence_marker_content_not_read"),
)

EXPECTED_SHA256 = {
    "FIGTALK.dat": "1ace07e58ed09f6875f5d9cacaa5a7be0bf8b83af4e0668d2a26cc421d873d1c",
    "LEVEL.DAT": "858faceb6778d8d4612e134fb8514e5aacf45039ff417fd44d0464ebf447492b",
    "MAPS.DAT": "a6faee71c0cb41c1be94f29152deed12ed19eb3e1842fb0f10e13e00132a20ba",
    "MON.DAT": "05abe60f3e3c0a92d0c345835014271019b6d1bdac6a9bb7e33e3009290a4980",
    "PATH.DAT": "8b295815fc2e311fde2caf0557cfe3de137b12105339fc797428f99ddfae47a3",
    "TALK1.DAT": "e85520a8158ec9d01364f3b00dde7965f3dd07c5d34829f380ce8d446cf38b6f",
    "TALK2.DAT": "cfe8059f8899eb5bd255885510e4b26fb3ec35793fb386b270e1b9e97dd5880e",
    "TALK3.DAT": "369d5c03b63957c4c240a465d5cb9dba8dc7290a36ad8a480f28d41af9398959",
    "TALK4.DAT": "5f6cf8d31d7b25fba78797b28c64f33f3c1ffbdc16b70930c083140bd90929a7",
    "Env.dat": "2c55dddc9a6808afda5d69688f2c27ac268caf2b9155ae82b18596ed593ed9a4",
    "Fame.dat": "52047d64faba2cca72bcfa872ba455144eca5d750bbd9f249b6bd877c3b49ff9",
    "Data/mcache.dat": "998599462e4f8828ef4e8da4143b980c346613e426331be4a809070d559f0472",
    "swd3_dvd.dat": "21d50c950c93e82e9619eaef1e1d7d27ad20dc77266bde4565ffc0afe2e82351",
}

# These are sample table spans, not safety bounds invented for the original
# functions.  The basis is emitted for every row and explained in the evidence
# document.  Original lookup expressions perform no range check.
OFFSET_TABLES = (
    ("FIGTALK.dat", "figtalk_primary", 0x0000, 301, "payload[0]/4"),
    ("LEVEL.DAT", "level_grouped_100", 0x0000, 402, "payload[0]/4"),
    ("PATH.DAT", "path_primary", 0x0000, 802, "payload[1]/4"),
    ("TALK1.DAT", "talk_remainder", 0x0000, 1202, "payload[1]/4"),
    ("TALK2.DAT", "talk_remainder", 0x0000, 701, "payload[1]/4"),
    ("TALK3.DAT", "talk_remainder", 0x0000, 2000, "assembly_modulo_2000_contract"),
    ("TALK4.DAT", "talk_remainder", 0x0000, 1101, "payload[1]/4"),
    ("MON.DAT", "mon_primary_before_aux_root", 0x0000, 1723, "payload[1]/4"),
    ("MON.DAT", "mon_auxiliary", 0x1AEC, 470, "(payload[0x1AEC]-0x1AEC)/4"),
)

LOADER_ROWS = (
    (
        "sub_40F160",
        "0040F160",
        "MAPS.DAT",
        "read_payload_whole",
        "file_size-0x200; seek 0x200; read unchanged tail",
        "exact remaining file size",
        "none",
        "0040F174-0040F1B5",
    ),
    (
        "sub_40F040 / path consumers",
        "0040F040",
        "PATH.DAT",
        "map_whole_file_then_index",
        "base + 0x200 + u32(base+0x200+4*path_id)",
        "whole-file mapping",
        "none",
        "0040F094-0040F0F3; 00405529-00405535",
    ),
    (
        "sub_42E480",
        "0042E480",
        "TALK1.DAT..TALK4.DAT",
        "select_file_and_read_script_window",
        "file=(story_id/2000)+1; slot=story_id%2000; target=0x200+u32(0x200+4*slot)",
        "0x8000 requested",
        "none",
        "0042E490-0042E582",
    ),
    (
        "sub_46E0B0",
        "0046E0B0",
        "FIGTALK.dat",
        "read_battle_talk_window",
        "target=0x200+u32(0x200+4*id)",
        "0x8000 zero-filled allocation then fixed read",
        "none",
        "0046E129-0046E1C6",
    ),
    (
        "sub_476DB0",
        "00476DB0",
        "MON.DAT",
        "read_definition_tag_stream",
        "target=0x200+u32(0x200+4*id)",
        "0x400 zero-filled allocation then fixed read",
        "none",
        "00476E68-00476EFA",
    ),
    (
        "sub_476A80",
        "00476A80",
        "MON.DAT",
        "read_auxiliary_tag_stream",
        "root=u32(0x204); target=0x200+u32(0x200+root+4*id)",
        "0x400 zero-filled allocation then fixed read",
        "none",
        "00476AF0-00476B78",
    ),
    (
        "sub_477290 / sub_477400",
        "00477290 / 00477400",
        "LEVEL.DAT",
        "read_grouped_level_tag_stream",
        "entry=0x70+4*(level+100*group); target=0x200+u32(entry)",
        "0x400 zero-filled allocation then fixed read",
        "none",
        "00477300-00477387; 00477472-004774F9",
    ),
    (
        "sub_423AF0 / sub_423FB0",
        "00423AF0 / 00423FB0",
        "Env.dat",
        "rewrite_or_read_environment_record",
        "raw record begins with optional 0xFFFFFFFF current-layout marker",
        "up to 0x1000 temporary buffer",
        "none",
        "00423B74-00423DB7; 00423FB0 reader",
    ),
    (
        "sub_409CE0 / sub_409DD0",
        "00409CE0 / 00409DD0",
        "Fame.dat",
        "write_or_read_fixed_table",
        "u16 value for ids 1..500 at byte offset 2*id",
        "exactly 0x400 bytes",
        "none",
        "00409D03-00409D97; 00409DF3-00409E92",
    ),
    (
        "sub_426840",
        "00426840",
        "Data/mcache.dat",
        "read_update_cache_directory",
        "file_size/16 records; each record is 16 bytes",
        "whole file, normally 24 records / 0x180 bytes",
        "none",
        "004268B9-0042699D",
    ),
    (
        "sub_4118B0",
        "004118B0",
        "swd3_dvd.dat",
        "presence_check_only",
        "construct drive path and test existence; file bytes are never read",
        "none",
        "none",
        "0041197C-004119E5",
    ),
)


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def mz_declared_size(cblp: int, cp: int) -> int:
    if cp == 0:
        return 0
    return (cp - 1) * 512 + (cblp if cblp else 512)


def special_slot(file_name: str, table_name: str, slot: int, raw_offset: int) -> str:
    if raw_offset == 0xFFFFFFFF:
        return "sentinel_minus_one"
    if slot == 0 and table_name in {"figtalk_primary", "path_primary", "talk_remainder"}:
        return "slot_zero_not_used_by_normal_one_based_or_sample_lookup"
    if file_name == "MON.DAT" and table_name == "mon_primary_before_aux_root" and slot == 1:
        return "auxiliary_table_root"
    return "ordinary_sample_offset"


def main() -> None:
    asm = ASM_PATH.read_text(encoding="utf-8", errors="replace")
    required_asm = (
        "0040F17E                 sub     eax, 200h",
        "0040F19A                 push    200h",
        "0042E520                 mov     ecx, 7D0h",
        "0042E52D                 lea     edx, ds:200h[edx*4]",
        "0046E136                 push    204h",
        "00476E72                 push    204h",
        "0047731C                 lea     edx, ds:70h[ecx*4]",
        "00477F79                 call    sub_4399E0",
    )
    for needle in required_asm:
        if needle not in asm:
            raise SystemExit(f"assembly evidence changed or missing: {needle}")

    INVENTORY_ROOT.mkdir(parents=True, exist_ok=True)
    file_rows: list[tuple[object, ...]] = []
    loaded: dict[str, bytes] = {}
    for file_name, expected_size in MZ_FILES:
        data = (WORKSPACE_ROOT / file_name).read_bytes()
        loaded[file_name] = data
        if len(data) != expected_size:
            raise SystemExit(f"unexpected {file_name} size: {len(data)}")
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXPECTED_SHA256[file_name]:
            raise SystemExit(f"unexpected {file_name} SHA-256: {digest}")
        fields = tuple(u16(data, offset) for offset in range(0, 0x1C, 2))
        (
            magic,
            cblp,
            cp,
            crlc,
            cparhdr,
            minalloc,
            maxalloc,
            ss,
            sp,
            csum,
            ip,
            cs,
            lfarlc,
            ovno,
        ) = fields
        if magic != 0x5A4D:
            raise SystemExit(f"{file_name} does not have MZ magic")
        if mz_declared_size(cblp, cp) != len(data):
            raise SystemExit(f"{file_name} MZ page fields do not reproduce file size")
        if (crlc, cparhdr, minalloc, maxalloc, ss, sp, csum, ip, cs, lfarlc, ovno) != (
            0,
            0x20,
            0,
            0xFFFF,
            0,
            0,
            0,
            0,
            0,
            0x1E,
            0,
        ):
            raise SystemExit(f"unexpected shared MZ header fields in {file_name}")
        if u16(data, 0x1C) != 1 or any(data[0x1E:0x200]):
            raise SystemExit(f"{file_name} has nonzero bytes in padded MZ header")
        payload_base = cparhdr * 16
        file_rows.append(
            (
                file_name,
                len(data),
                digest,
                "mz_wrapped_raw_payload",
                f"{magic:04X}",
                cblp,
                cp,
                crlc,
                cparhdr,
                f"{payload_base:08X}",
                minalloc,
                f"{maxalloc:04X}",
                ss,
                sp,
                csum,
                ip,
                cs,
                f"{lfarlc:04X}",
                ovno,
                u16(data, 0x1C),
                mz_declared_size(cblp, cp),
                len(data) - payload_base,
                "none",
            )
        )

    for file_name, expected_size, kind in PLAIN_FILES:
        data = (WORKSPACE_ROOT / file_name).read_bytes()
        loaded[file_name] = data
        if len(data) != expected_size:
            raise SystemExit(f"unexpected {file_name} size: {len(data)}")
        digest = hashlib.sha256(data).hexdigest()
        if digest != EXPECTED_SHA256[file_name]:
            raise SystemExit(f"unexpected {file_name} SHA-256: {digest}")
        file_rows.append(
            (
                file_name,
                len(data),
                digest,
                kind,
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                "",
                len(data),
                "none",
            )
        )

    with FILES_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "file_size_bytes",
                "sha256",
                "container_class",
                "mz_magic_hex",
                "mz_e_cblp",
                "mz_e_cp",
                "mz_e_crlc",
                "mz_e_cparhdr_paragraphs",
                "payload_base_hex",
                "mz_e_minalloc",
                "mz_e_maxalloc_hex",
                "mz_e_ss",
                "mz_e_sp",
                "mz_e_csum",
                "mz_e_ip",
                "mz_e_cs",
                "mz_e_lfarlc_hex",
                "mz_e_ovno",
                "mz_e_res0_custom_value",
                "size_reconstructed_from_mz_pages",
                "payload_size_bytes",
                "sub_4399e0_used_by_file_loader",
            )
        )
        writer.writerows(file_rows)

    index_rows: list[tuple[object, ...]] = []
    summary_rows: list[tuple[object, ...]] = []
    for file_name, table_name, table_relative, slot_count, range_basis in OFFSET_TABLES:
        data = loaded[file_name]
        payload = data[0x200:]
        table_end = table_relative + slot_count * 4
        if table_end > len(payload):
            raise SystemExit(f"{file_name} {table_name} table is out of bounds")
        valid_targets = 0
        invalid_targets = 0
        unique_targets: set[int] = set()
        for slot in range(slot_count):
            table_payload_offset = table_relative + slot * 4
            raw_offset = u32(payload, table_payload_offset)
            target_file_offset = 0x200 + raw_offset
            in_bounds = raw_offset != 0xFFFFFFFF and target_file_offset < len(data)
            if in_bounds:
                valid_targets += 1
                unique_targets.add(raw_offset)
                first_word = u16(data, target_file_offset) if target_file_offset + 2 <= len(data) else 0
                first_word_hex = f"{first_word:04X}"
            else:
                invalid_targets += 1
                first_word_hex = ""
            if table_name == "level_grouped_100":
                logical_group = slot // 100 + 1
                logical_id = slot % 100
            else:
                logical_group = ""
                logical_id = slot
            index_rows.append(
                (
                    file_name,
                    table_name,
                    range_basis,
                    slot,
                    logical_group,
                    logical_id,
                    f"{0x200 + table_payload_offset:08X}",
                    f"{raw_offset:08X}",
                    f"{target_file_offset:08X}" if raw_offset != 0xFFFFFFFF else "",
                    int(in_bounds),
                    first_word_hex,
                    special_slot(file_name, table_name, slot, raw_offset),
                )
            )
        summary_rows.append(
            (
                file_name,
                table_name,
                f"{table_relative:08X}",
                slot_count,
                range_basis,
                valid_targets,
                invalid_targets,
                len(unique_targets),
                f"{min(unique_targets):08X}" if unique_targets else "",
                f"{max(unique_targets):08X}" if unique_targets else "",
            )
        )

    with INDEX_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "table_name",
                "sample_range_basis",
                "physical_slot",
                "logical_group_if_level",
                "logical_id",
                "table_entry_file_offset_hex",
                "raw_payload_relative_offset_hex",
                "target_file_offset_hex",
                "target_in_file",
                "target_first_u16_hex",
                "slot_note",
            )
        )
        writer.writerows(index_rows)

    with INDEX_SUMMARY_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "table_name",
                "table_payload_offset_hex",
                "sample_slots",
                "sample_range_basis",
                "in_file_targets",
                "out_of_file_or_sentinel_targets",
                "unique_in_file_targets",
                "minimum_in_file_payload_offset_hex",
                "maximum_in_file_payload_offset_hex",
            )
        )
        writer.writerows(summary_rows)

    with LOADER_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "function",
                "function_address",
                "files",
                "operation",
                "lookup_or_layout",
                "read_or_mapping_window",
                "decoder_call",
                "assembly_evidence",
            )
        )
        writer.writerows(LOADER_ROWS)

    plain_field_rows: list[tuple[object, ...]] = []
    env = loaded["Env.dat"]
    first_end = env.find(b"\0", 0x2E)
    second_end = env.find(b"\0", first_end + 1)
    if first_end < 0 or second_end < 0:
        raise SystemExit("Env.dat current strings are not NUL terminated")
    env_fields = (
        ("current_layout_marker", 0x00, 4, env[0x00:0x04], "0xFFFFFFFF selects current layout"),
        ("sixteen_global_bytes", 0x04, 16, env[0x04:0x14], "copied to/from sixteen byte globals"),
        ("integer_parameter", 0x14, 4, env[0x14:0x18], "32-bit caller parameter"),
        ("six_option_bytes", 0x18, 6, env[0x18:0x1E], "six byte-sized caller parameters"),
        ("preserved_or_legacy_area", 0x1E, 16, env[0x1E:0x2E], "zeroed only during old-layout migration"),
        ("string_1", 0x2E, first_end - 0x2E + 1, env[0x2E:first_end + 1], "first NUL-terminated string"),
        (
            "string_2",
            first_end + 1,
            second_end - first_end,
            env[first_end + 1:second_end + 1],
            "second NUL-terminated string",
        ),
        (
            "post_string_bytes",
            second_end + 1,
            len(env) - second_end - 1,
            env[second_end + 1:],
            "first byte is read as byte_4C97F4 value; remaining byte is not consumed by sub_423FB0",
        ),
    )
    for name, offset, size, value, use in env_fields:
        text_value = ""
        if name.startswith("string_"):
            text_value = value[:-1].decode("cp950", errors="replace")
        plain_field_rows.append(
            ("Env.dat", "environment_field", name, f"{offset:08X}", size, value.hex().upper(), text_value, use)
        )

    fame = loaded["Fame.dat"]
    if len(fame) != 512 * 2:
        raise SystemExit("Fame.dat is not exactly 512 u16 slots")
    for slot in range(512):
        value = u16(fame, slot * 2)
        if slot == 0 or slot > 500:
            use = "outside sub_409CE0/sub_409DD0 id loop"
        else:
            use = "fame id 1..500 raw u16 value"
        plain_field_rows.append(
            (
                "Fame.dat",
                "fame_u16_slot",
                slot,
                f"{slot * 2:08X}",
                2,
                f"{value:04X}",
                value,
                use,
            )
        )

    mcache = loaded["Data/mcache.dat"]
    if len(mcache) != 24 * 16:
        raise SystemExit("Data/mcache.dat is not exactly 24 x 16 bytes")
    for slot in range(24):
        offset = slot * 16
        map_id, byte_size, use_counter, stored_slot = struct.unpack_from("<4I", mcache, offset)
        value_hex = " ".join(f"{value:08X}" for value in (map_id, byte_size, use_counter, stored_slot))
        value_text = (
            f"map_id={map_id if map_id != 0xFFFFFFFF else -1};byte_size={byte_size};"
            f"use_counter={use_counter};stored_slot={stored_slot}"
        )
        plain_field_rows.append(
            (
                "Data/mcache.dat",
                "cache_record",
                slot,
                f"{offset:08X}",
                16,
                value_hex,
                value_text,
                "four u32 fields consumed and rewritten by sub_426840",
            )
        )

    plain_field_rows.append(
        (
            "swd3_dvd.dat",
            "presence_marker",
            "whole_file",
            "00000000",
            len(loaded["swd3_dvd.dat"]),
            hashlib.sha256(loaded["swd3_dvd.dat"]).hexdigest(),
            "",
            "sub_4118B0 tests path existence and never reads file contents",
        )
    )

    with PLAIN_FIELDS_OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "file",
                "record_kind",
                "logical_index_or_field",
                "file_offset_hex",
                "size_bytes",
                "value_hex_or_hash",
                "value_text_or_integer",
                "assembly_use",
            )
        )
        writer.writerows(plain_field_rows)

    print(
        f"wrote {len(file_rows)} DAT file rows, {len(index_rows)} offset slots, "
        f"{len(summary_rows)} table summaries, {len(LOADER_ROWS)} loader contexts and "
        f"{len(plain_field_rows)} plain-DAT field rows"
    )


if __name__ == "__main__":
    main()
