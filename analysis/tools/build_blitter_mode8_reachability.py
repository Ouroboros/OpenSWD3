#!/usr/bin/env python3
"""Build assembly- and asset-locked evidence for blitter mode 0x08.

This tool answers a deliberately narrow question: can the current TSW/ACT
asset path supply the byte plane consumed by the RLE mode-0x08 kernels?  The
assembly remains the sole behavioral authority; asset scans only establish
reachability for the six archives shipped in this workspace.
"""

from __future__ import annotations

import csv
import hashlib
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXE_PATH = WORKSPACE_ROOT / "swd3.exe"
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
FRAME_INPUT = INVENTORY_ROOT / "tsw-frame-descriptors.tsv"
ACT_COMMAND_INPUT = INVENTORY_ROOT / "act-command-words.tsv"
ACTION_EXTERNAL_INPUT = INVENTORY_ROOT / "action-external-accesses.tsv"
TSW_OUTPUT = INVENTORY_ROOT / "tsw-blitter-family-and-aux.tsv"
REACHABILITY_OUTPUT = INVENTORY_ROOT / "blitter-mode8-reachability.tsv"

EXPECTED_SHA256 = {
    EXE_PATH: "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    FRAME_INPUT: "481cd14a026a3d7a7ad208e7c69fcb844dbc06f604f9038518ad0064894ba6f6",
    ACT_COMMAND_INPUT: "2e8371a9463f5ef396455993cacd754b187c2065541d737e0b3e846f5b434140",
}

EXPECTED_FRAME_COUNT = 20091
EXPECTED_RLE_FIRST_WORD_COUNT = 20091
EXPECTED_AUX_METADATA_FRAMES = 7316
EXPECTED_AUX_PHYSICAL_PAYLOAD_FRAMES = 7148
EXPECTED_AUX_PHYSICAL_PAYLOAD_BYTES = 8463829
EXPECTED_ACTION_MODE_READS = 11

EXPECTED_DECOMPRESSED_HASHES = {
    "all_char.tsw": "c6c401cfdd33d047ad16afb0e3af047f5f24a203a76fe2b0679c70e07235fff3",
    "all_item.tsw": "27a624cd08659b0723839b8b4ae8673192346fea8455aca0cdd631c2b76c2402",
    "all_magic.tsw": "0c797e28826a8da9cd18dcf265126b6b9ff86726520f509a07237dc89b8b59b1",
    "all_map1.tsw": "888680220725bee08718b6203c8f63cad8f9ed84c09e6e4dc913e80bca101934",
    "all_map2.tsw": "79c8f249ce2fb2a17e677f05078f88b70736ada3b554eec05bc5c8a4e453f1f8",
    "all_sys.tsw": "45a68c036ed196e91414502c24b1ea12f826832c0a25e98929f7bb8628a27c17",
}

EXPECTED_SNIPPETS = {
    # Both TSW wrapper records begin as six zero dwords.  Their runtime +4 is
    # copied from local var_3C, i.e. temporary loader-record +8.
    0x00433383: "mov ecx, 6",
    0x00433391: "lea edi, [esp+5Ch+var_44]",
    0x00433395: "rep stosd",
    0x0043351A: "mov edx, [esp+58h+var_3C]",
    0x00433526: "mov [eax+4], edx",
    0x00433723: "mov ecx, 6",
    0x00433731: "lea edi, [esp+5Ch+var_44]",
    0x00433735: "rep stosd",
    0x004338A5: "mov edx, [esp+5Ch+var_3C]",
    0x004338AD: "mov [eax+4], edx",
    # The two low-level loaders only clear temporary +8 when descriptor +0x0C
    # is nonzero.  There is no nonzero producer in either function.
    0x004336BC: "mov ecx, [esp+3Ch+var_18]",
    0x004336C0: "cmp ecx, ebp",
    0x004336C7: "mov [eax+8], ebp",
    0x00433A49: "mov ecx, [esp+3Ch+var_18]",
    0x00433A4D: "cmp ecx, ebp",
    0x00433A54: "mov [eax+8], ebp",
    # 8-bit source conversion clears runtime +8 (palette), not runtime +4.
    0x00401E42: "mov dword ptr [eax+8], 0",
    # Current world rendering re-enables the source-header RLE selection.
    0x00412A70: "mov dword_4CD764, 0",
    0x004170E8: "cmp word ptr [eax], 0FFFFh",
    0x004170FB: "or [esp+10h+arg_10], 80000000h",
    # The only explicit ACT producer for mode 0x08.
    0x0043250C: "mov ecx, [esi+18h]",
    0x0043250F: "and ecx, 8000000Bh",
    0x00432515: "or ecx, 8",
    0x00432518: "mov [esi+18h], ecx",
    # The sole immediate mode-8 dispatcher call is the raw constant-color path.
    0x00450A58: "lea eax, [esp+arg_10]",
    0x00450A5E: "mov dword_4CD730, eax",
    0x00450A67: "push 8",
    0x00450A71: "call sub_4170E0",
    # Reverse RLE special-run lookup.
    0x0041B1E7: "mov esi, dword_4CDC00",
    # The adjacent initialized tables begin at +0x04 and are only written
    # forward; their fills do not cover the pointer-sized hole at 0x4CDC00.
    0x004237A3: "mov edi, offset unk_4CDC04",
    0x004237C7: "mov ecx, 20h",
    0x004237CC: "rep stosd",
    0x00423860: "mov edi, offset unk_4CDD04",
    0x00423865: "rep stosd",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(instruction: str) -> str:
    return " ".join(instruction.split())


def load_assembly() -> dict[int, str]:
    instructions: dict[int, str] = {}
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        instruction = match.group(2).split(";", 1)[0].rstrip()
        if not instruction or instruction.endswith(":"):
            continue
        instructions[int(match.group(1), 16)] = normalize(instruction)
    return instructions


def verify_locked_inputs() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"locked input changed: {path} expected {expected}, got {actual}"
            )


def verify_assembly(instructions: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = instructions.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: "
                f"expected {expected!r}, got {actual!r}"
            )

    reverse_table_refs = [
        (address, instruction)
        for address, instruction in instructions.items()
        if address < 0x00490000 and "dword_4CDC00" in instruction
    ]
    if reverse_table_refs != [(0x0041B1E7, "mov esi, dword_4CDC00")]:
        raise SystemExit(f"unexpected dword_4CDC00 reference set: {reverse_table_refs}")

    encoded_address_offsets: list[int] = []
    image = EXE_PATH.read_bytes()
    encoded_address = struct.pack("<I", 0x004CDC00)
    search_from = 0
    while True:
        offset = image.find(encoded_address, search_from)
        if offset < 0:
            break
        encoded_address_offsets.append(offset)
        search_from = offset + 1
    if encoded_address_offsets != [0x0001B1E9]:
        raise SystemExit(
            "unexpected encoded 0x004CDC00 address set in PE: "
            f"{encoded_address_offsets}"
        )

    for start, end, function_name in (
        (0x00433540, 0x0043371D, "sub_433540"),
        (0x004338F0, 0x00433A9D, "sub_4338F0"),
    ):
        writes_to_temp_aux = [
            (address, instruction)
            for address, instruction in instructions.items()
            if start <= address < end
            and re.fullmatch(r"mov \[eax\+8\], .+", instruction)
        ]
        expected_address = 0x004336C7 if start == 0x00433540 else 0x00433A54
        expected = [(expected_address, "mov [eax+8], ebp")]
        if writes_to_temp_aux != expected:
            raise SystemExit(
                f"{function_name} temporary auxiliary writes changed: "
                f"{writes_to_temp_aux}"
            )


def load_frames() -> list[dict[str, str]]:
    with FRAME_INPUT.open(encoding="utf-8", newline="") as source:
        frames = list(csv.DictReader(source, delimiter="\t"))
    if len(frames) != EXPECTED_FRAME_COUNT:
        raise SystemExit(f"unexpected TSW frame count: {len(frames)}")
    return frames


def verify_act_mode8_command() -> tuple[int, str]:
    with ACT_COMMAND_INPUT.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    matches = [row for row in rows if row["command_word_hex"] == "4148"]
    if len(matches) != 1:
        raise SystemExit(f"expected one ACT command row for 0x4148, got {len(matches)}")
    row = matches[0]
    archive_counts = [
        int(row[f"{archive}_occurrences"])
        for archive in (
            "all_char.act",
            "all_item.act",
            "all_magic.act",
            "all_sys.act",
            "all_map1.act",
            "all_map2.act",
        )
    ]
    total = int(row["total_occurrences"])
    if total != 0 or any(archive_counts):
        raise SystemExit(
            f"current ACT archives unexpectedly contain mode-8 command: {archive_counts}"
        )
    return total, ",".join(str(value) for value in archive_counts)


def verify_action_external_accesses() -> list[str]:
    with ACTION_EXTERNAL_INPUT.open(encoding="utf-8", newline="") as source:
        rows = list(csv.DictReader(source, delimiter="\t"))
    mode_rows = [row for row in rows if row["action_offset_hex"] == "18"]
    if len(mode_rows) != EXPECTED_ACTION_MODE_READS:
        raise SystemExit(
            f"unexpected proven external action+0x18 access count: {len(mode_rows)}"
        )
    if {row["access_kind"] for row in mode_rows} != {"read"}:
        raise SystemExit("proven external action+0x18 set now contains a write")
    return [row["instruction_address"] for row in mode_rows]


def scan_tsw_frames(
    frames: list[dict[str, str]],
) -> tuple[list[tuple[object, ...]], int, int, int]:
    # Reuse the assembly-faithful valid-stream transliteration.  Importing here
    # avoids coupling that module's output files to this separate inventory.
    sys.path.insert(0, str(RESEARCH_ROOT / "tools"))
    from verify_tsw_decompression import (  # pylint: disable=import-outside-toplevel
        DecompressionError,
        decompress_sub_4399e0,
    )

    counts: Counter[tuple[str, int]] = Counter()
    rle_counts: Counter[tuple[str, int]] = Counter()
    raw_counts: Counter[tuple[str, int]] = Counter()
    aux_metadata_counts: Counter[tuple[str, int]] = Counter()
    aux_payload_counts: Counter[tuple[str, int]] = Counter()
    aux_payload_bytes: Counter[tuple[str, int]] = Counter()
    aux_extent_counts: Counter[tuple[str, int]] = Counter()
    archive_hashes: dict[str, object] = defaultdict(hashlib.sha256)

    open_files = {
        archive: (WORKSPACE_ROOT / archive).open("rb")
        for archive in sorted({frame["archive"] for frame in frames})
    }
    try:
        for frame in frames:
            archive = frame["archive"]
            bpp = int(frame["storage_bpp"])
            key = (archive, bpp)
            compressed_size = int(frame["compressed_size_bytes"])
            declared_size = int(frame["declared_decompressed_size_bytes"])
            offset = int(frame["payload_absolute_offset_hex"], 16)
            source = open_files[archive]
            source.seek(offset)
            compressed = source.read(compressed_size)
            if len(compressed) != compressed_size:
                raise SystemExit(
                    f"{archive} resource {frame['lookup_resource_id']} variant "
                    f"{frame['variant_index']}: short primary read"
                )
            try:
                decompressed = decompress_sub_4399e0(compressed, declared_size)
            except DecompressionError as error:
                raise SystemExit(
                    f"{archive} resource {frame['lookup_resource_id']} variant "
                    f"{frame['variant_index']}: {error}"
                ) from error
            if len(decompressed) < 2:
                raise SystemExit(
                    f"{archive} resource {frame['lookup_resource_id']} variant "
                    f"{frame['variant_index']}: decompressed stream shorter than first word"
                )

            first_word = struct.unpack_from("<H", decompressed)[0]
            counts[key] += 1
            if first_word == 0xFFFF:
                rle_counts[key] += 1
            else:
                raw_counts[key] += 1
            archive_hashes[archive].update(decompressed)

            metadata_0c = int(frame["metadata_0c"])
            physical_aux_bytes = int(frame["descriptor_10_trailing_span_bytes"])
            metadata_14 = int(frame["metadata_14"])
            if metadata_0c or metadata_14:
                if not metadata_0c or metadata_14 != int(frame["width"]) * int(frame["height"]):
                    raise SystemExit(
                        f"inconsistent TSW auxiliary metadata in {archive} resource "
                        f"{frame['lookup_resource_id']} variant {frame['variant_index']}"
                    )
                aux_metadata_counts[key] += 1
                aux_extent_counts[key] += 1
            if physical_aux_bytes:
                if not metadata_0c or not metadata_14:
                    raise SystemExit("physical TSW auxiliary payload lacks metadata")
                aux_payload_counts[key] += 1
                aux_payload_bytes[key] += physical_aux_bytes
    finally:
        for source in open_files.values():
            source.close()

    actual_hashes = {
        archive: digest.hexdigest() for archive, digest in archive_hashes.items()
    }
    if actual_hashes != EXPECTED_DECOMPRESSED_HASHES:
        raise SystemExit(f"unexpected decompressed TSW hashes: {actual_hashes}")

    total_rle = sum(rle_counts.values())
    total_aux_metadata = sum(aux_metadata_counts.values())
    total_aux_payload_frames = sum(aux_payload_counts.values())
    total_aux_payload_bytes = sum(aux_payload_bytes.values())
    if total_rle != EXPECTED_RLE_FIRST_WORD_COUNT or sum(raw_counts.values()) != 0:
        raise SystemExit(
            f"unexpected TSW blitter-family split: rle={total_rle}, "
            f"raw={sum(raw_counts.values())}"
        )
    observed_aux = (
        total_aux_metadata,
        total_aux_payload_frames,
        total_aux_payload_bytes,
    )
    expected_aux = (
        EXPECTED_AUX_METADATA_FRAMES,
        EXPECTED_AUX_PHYSICAL_PAYLOAD_FRAMES,
        EXPECTED_AUX_PHYSICAL_PAYLOAD_BYTES,
    )
    if observed_aux != expected_aux:
        raise SystemExit(
            f"unexpected TSW auxiliary metadata totals: "
            f"observed={observed_aux}, expected={expected_aux}"
        )

    rows: list[tuple[object, ...]] = []
    for archive, bpp in sorted(counts):
        key = (archive, bpp)
        rows.append(
            (
                archive,
                bpp,
                counts[key],
                rle_counts[key],
                raw_counts[key],
                aux_metadata_counts[key],
                aux_payload_counts[key],
                aux_payload_bytes[key],
                aux_extent_counts[key],
                "always_zero",
                "sub_433380/sub_433720 runtime +0x04 <- zero-initialized temporary +0x08",
            )
        )
    rows.append(
        (
            "TOTAL",
            "all",
            sum(counts.values()),
            total_rle,
            sum(raw_counts.values()),
            total_aux_metadata,
            total_aux_payload_frames,
            total_aux_payload_bytes,
            sum(aux_extent_counts.values()),
            "always_zero",
            "descriptor auxiliary metadata and bytes are not loaded into runtime +0x04",
        )
    )
    return rows, total_rle, total_aux_metadata, total_aux_payload_frames


def write_outputs(
    tsw_rows: list[tuple[object, ...]],
    total_rle: int,
    total_aux_metadata: int,
    total_aux_payload_frames: int,
    act_mode8_total: int,
    act_archive_counts: str,
    external_read_addresses: list[str],
) -> None:
    INVENTORY_ROOT.mkdir(parents=True, exist_ok=True)
    with TSW_OUTPUT.open("w", encoding="utf-8", newline="") as target:
        writer = csv.writer(target, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "archive",
                "storage_bpp",
                "frame_count",
                "decompressed_first_word_ffff_count",
                "decompressed_other_first_word_count",
                "descriptor_aux_metadata_frame_count",
                "physical_aux_payload_frame_count",
                "physical_aux_payload_bytes",
                "metadata_14_equals_width_times_height_count",
                "runtime_record_plus_04_contract",
                "assembly_basis",
            )
        )
        writer.writerows(tsw_rows)

    reachability_rows = [
        (
            "tsw_primary_family",
            "six current TSW archives",
            total_rle,
            "all current decompressed primary streams begin with 0xFFFF and select the RLE/span family when dword_4CD764 is zero",
            "closed_current_assets",
        ),
        (
            "tsw_auxiliary_metadata",
            "six current TSW archives",
            total_aux_metadata,
            f"{total_aux_metadata} descriptors advertise width*height auxiliary extent; {total_aux_payload_frames} have physical bytes after the primary stream",
            "present_but_not_consumed_by_current_loader",
        ),
        (
            "tsw_runtime_auxiliary",
            "sub_433380 and sub_433720",
            0,
            "runtime record +0x04 is copied from zero-initialized temporary +0x08; both low-level loaders only write zero there",
            "closed_always_zero",
        ),
        (
            "act_explicit_mode8_producer",
            "command 0x4148/HA",
            act_mode8_total,
            f"per-archive occurrences={act_archive_counts}",
            "closed_absent_current_assets",
        ),
        (
            "action_mode_external_accesses",
            "currently proven ActionRecord +0x18 external set",
            len(external_read_addresses),
            "all are reads: " + ",".join(external_read_addresses),
            "closed_no_external_writer_in_proven_set",
        ),
        (
            "immediate_mode8_path",
            "sub_450A50 -> sub_4170E0",
            1,
            "uses address of its own argument slot as source and therefore takes raw +0x88 constant-color fade, not RLE coverage",
            "closed_safe_noncoverage_path",
        ),
        (
            "reverse_special_table",
            "dword_4CDC00",
            1,
            "the PE contains exactly one encoded 0x004CDC00 address, the reverse RLE 0x8000-run read; adjacent table initialization starts at 0x004CDC04 and writes forward",
            "no_normal_producer_dangerous_only_if_malformed_state_forces_it",
        ),
        (
            "current_asset_driven_rle_mode8",
            "TSW + ACT combined contract",
            0,
            "no current ACT command selects mode 0x08 and current TSW loaders cannot supply its required byte plane",
            "not_reachable_from_demonstrated_asset_path",
        ),
    ]
    with REACHABILITY_OUTPUT.open("w", encoding="utf-8", newline="") as target:
        writer = csv.writer(target, delimiter="\t", lineterminator="\n")
        writer.writerow(("evidence_class", "scope", "observed_count", "evidence", "status"))
        writer.writerows(reachability_rows)


def main() -> None:
    verify_locked_inputs()
    instructions = load_assembly()
    verify_assembly(instructions)
    act_mode8_total, act_archive_counts = verify_act_mode8_command()
    external_read_addresses = verify_action_external_accesses()
    frames = load_frames()
    tsw_rows, total_rle, total_aux_metadata, total_aux_payload_frames = scan_tsw_frames(frames)
    write_outputs(
        tsw_rows,
        total_rle,
        total_aux_metadata,
        total_aux_payload_frames,
        act_mode8_total,
        act_archive_counts,
        external_read_addresses,
    )
    print(
        f"classified {total_rle} TSW frames as RLE/span; "
        f"{total_aux_metadata} carry unused auxiliary metadata, "
        f"ACT command 0x4148 occurs {act_mode8_total} times"
    )


if __name__ == "__main__":
    main()
