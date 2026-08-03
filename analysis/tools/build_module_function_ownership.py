#!/usr/bin/env python3
"""Build the Stage A2 module ownership candidate inventory.

The output is deliberately a candidate map, not recovered function semantics.
Assembly supplies function entries and direct calls.  Existing manually reviewed
ABI and top-level evidence supplies the fixed boundary seeds.  Address locality,
call-graph propagation and decompiler tokens are navigation-only fallbacks.
"""

from __future__ import annotations

import bisect
import csv
import hashlib
import re
from collections import Counter, defaultdict
from dataclasses import dataclass, replace
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
EXPORT_ROOT = WORKSPACE_ROOT / "swd3.exe_export_for_ai"
ASM_PATH = EXPORT_ROOT / "swd3.exe.asm"
CATALOG_PATH = RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "function-catalog.tsv"
ABI_PATH = RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "function-abi-candidates.tsv"
CRITICAL_PATH = RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "critical-abi-contracts.tsv"
OUTPUT_PATH = RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "module-function-ownership.tsv"
CROSS_CALL_OUTPUT_PATH = RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "module-cross-calls.tsv"
BOUNDARY_FUNCTION_OUTPUT_PATH = (
    RESEARCH_ROOT / "04-reverse-engineering" / "inventory" / "module-boundary-functions.tsv"
)

EXPECTED_SHA256 = {
    ASM_PATH: "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    CATALOG_PATH: "d8b707b5550c64dee539f25c7032fc700f48ae70fa0661e50faabf7a45df5c73",
    ABI_PATH: "cd85c44a4b03f395199d7de82f1e520263fe3aec0d9a8528657624484b681bda",
    CRITICAL_PATH: "0f3329f36be47ef897660df17fd723455c9c65231386385bedcc1a32a20475ef",
}

EXPECTED_REVIEWED_GAME_BOUNDARY_COUNT = 307
EXPECTED_REVIEWED_GAME_BOUNDARY_SHA256 = (
    "8d5abed4ebfd4e0ca97bd65e79d97de3e160a1580edac85b5765304824cf7938"
)

GAME_MODULES = (
    "runtime_platform",
    "resource_io",
    "input_time_rng",
    "rendering",
    "audio_video",
    "asset_runtime",
    "story_scene",
    "world_map",
    "special_modes",
    "battle",
    "persistence",
)
EXTERNAL_MODULES = ("external_crt", "external_third_party")

PROC_RE = re.compile(r"^([0-9A-F]{8})\s+(\S+)\s+proc\s+(?:near|far)\b")
ENDP_RE = re.compile(r"^[0-9A-F]{8}\s+(\S+)\s+endp\b")
CHUNK_START_RE = re.compile(r"^[0-9A-F]{8}\s+; START OF FUNCTION CHUNK FOR (\S+)\s*$")
CHUNK_END_RE = re.compile(r"^[0-9A-F]{8}\s+; END OF FUNCTION CHUNK FOR (\S+)\s*$")
CALL_RE = re.compile(r"^([0-9A-F]{8}) {17}call\s+(.+?)(?:\s+;.*)?$")


@dataclass(frozen=True)
class FunctionRow:
    address: int
    ida_name: str
    export_status: str
    asm_body_status: str
    exported_file: str


@dataclass(frozen=True)
class Ownership:
    code_origin: str
    module_candidate: str
    follow_up_module: str
    confidence: str
    assignment_basis: str
    review_scope: str
    review_status: str


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def load_functions() -> list[FunctionRow]:
    catalog_rows = read_tsv(CATALOG_PATH)
    catalog_files = {
        int(row["address"], 16): row["exported_file"] for row in catalog_rows
    }
    abi_rows = read_tsv(ABI_PATH)
    functions = [
        FunctionRow(
            address=int(row["address"], 16),
            ida_name=row["ida_name"],
            export_status=row["export_status"],
            asm_body_status=row["asm_body_status"],
            exported_file=catalog_files.get(int(row["address"], 16), ""),
        )
        for row in abi_rows
    ]
    if len(catalog_rows) != 1486:
        raise SystemExit(f"unexpected export catalog size: {len(catalog_rows)}")
    if len(functions) != 1510 or len({row.address for row in functions}) != 1510:
        raise SystemExit("ownership input is not the locked 1510-address ABI union")
    if sum(row.export_status == "assembly-proc-only-not-in-export-manifests" for row in functions) != 24:
        raise SystemExit("expected 24 assembly-only PROC entries")
    return functions


def target_name(operand: str) -> str:
    value = operand.strip()
    for prefix in ("near ptr ", "far ptr ", "short "):
        if value.startswith(prefix):
            value = value[len(prefix) :]
    return value.split()[0]


def parse_assembly_graph(
    lines: list[str], functions: list[FunctionRow]
) -> tuple[
    dict[int, Counter[int]],
    dict[int, Counter[int]],
    dict[tuple[int, int], list[int]],
    int,
]:
    name_to_address = {row.ida_name: row.address for row in functions}
    proc_names: set[str] = set()
    for line in lines:
        match = PROC_RE.match(line)
        if match:
            proc_names.add(match.group(2))
            name_to_address[match.group(2)] = int(match.group(1), 16)

    callees: dict[int, Counter[int]] = defaultdict(Counter)
    callers: dict[int, Counter[int]] = defaultdict(Counter)
    callsites: dict[tuple[int, int], list[int]] = defaultdict(list)
    current_owner: int | None = None
    owner_stack: list[int | None] = []
    proc_target_call_count = 0
    all_proc_target_call_count = 0

    for line in lines:
        chunk_start = CHUNK_START_RE.match(line)
        if chunk_start:
            owner_stack.append(current_owner)
            current_owner = name_to_address.get(chunk_start.group(1))
            continue
        chunk_end = CHUNK_END_RE.match(line)
        if chunk_end:
            current_owner = owner_stack.pop() if owner_stack else None
            continue

        proc_start = PROC_RE.match(line)
        if proc_start:
            current_owner = int(proc_start.group(1), 16)
            continue
        proc_end = ENDP_RE.match(line)
        if proc_end:
            current_owner = None
            continue

        call = CALL_RE.match(line)
        if not call:
            continue
        raw_target = target_name(call.group(2))
        destination = name_to_address.get(raw_target)
        if destination is None:
            continue
        if raw_target in proc_names:
            all_proc_target_call_count += 1
        if current_owner is None:
            continue
        callees[current_owner][destination] += 1
        callers[destination][current_owner] += 1
        callsites[(current_owner, destination)].append(int(call.group(1), 16))
        if raw_target in proc_names:
            proc_target_call_count += 1

    if all_proc_target_call_count != 7159 or proc_target_call_count != 7151:
        raise SystemExit(
            "unexpected direct calls to expanded PROC targets: "
            f"all={all_proc_target_call_count}, owned={proc_target_call_count}"
        )

    function_by_address = {row.address: row for row in functions}
    expected_top_sub_counts = {0x00409EC0: 8, 0x0040A0D0: 20, 0x0040A570: 37}
    for root, expected in expected_top_sub_counts.items():
        actual = sum(
            function_by_address[target].ida_name.startswith("sub_")
            for target in callees[root]
            if target in function_by_address
        )
        if actual != expected:
            raise SystemExit(
                f"top-level sub_* target mismatch at 0x{root:08X}: {actual} != {expected}"
            )
    return callees, callers, callsites, proc_target_call_count


def critical_module(subsystem: str) -> str:
    if subsystem in {"startup", "startup_platform", "platform_messages", "frame_coordinator", "platform_lifecycle"}:
        return "runtime_platform"
    if subsystem == "platform_graphics" or subsystem in {"pause_render", "world_render"}:
        return "rendering" if subsystem != "world_render" else "world_map"
    if subsystem == "mode_render" or subsystem in {"menu_modes", "shop"}:
        return "special_modes"
    if subsystem.startswith("audio") or subsystem.startswith("video"):
        return "audio_video"
    if subsystem.startswith("resource_tsw") or subsystem.startswith("resource_act") or subsystem == "action_runtime":
        return "asset_runtime"
    if subsystem.startswith("resource_"):
        return "resource_io"
    if subsystem.startswith("input"):
        return "input_time_rng"
    if subsystem.startswith("story"):
        return "story_scene"
    if subsystem.startswith("world") or subsystem in {"map", "map_cache"}:
        return "world_map"
    if subsystem.startswith("battle"):
        return "battle"
    if subsystem == "save":
        return "persistence"
    raise SystemExit(f"no module mapping for critical subsystem {subsystem!r}")


def load_critical_seeds() -> dict[int, str]:
    result: dict[int, str] = {}
    for row in read_tsv(CRITICAL_PATH):
        address = int(row["address"], 16)
        result[address] = critical_module(row["subsystem"])
    if len(result) != 39:
        raise SystemExit(f"expected 39 critical ABI seeds, got {len(result)}")
    return result


# All 59 unique sub_* targets of WinMain, WndProc and the frame root.  Values are
# the manually established boundary owner from top-level-direct-call-coverage.md.
TOP_LEVEL_SEEDS: dict[int, str] = {
    0x00402F80: "world_map",
    0x004050E0: "input_time_rng",
    0x00405430: "world_map",
    0x00406D30: "special_modes",
    0x00406E30: "special_modes",
    0x00409A10: "persistence",
    0x00409C10: "persistence",
    0x0040A570: "runtime_platform",
    0x0040AB50: "runtime_platform",
    0x0040CDD0: "audio_video",
    0x0040CF10: "audio_video",
    0x0040DBC0: "world_map",
    0x0040DC50: "story_scene",
    0x0040DC80: "story_scene",
    0x0040DCB0: "story_scene",
    0x0040F340: "rendering",
    0x0040F500: "world_map",
    0x0040F540: "world_map",
    0x0040F570: "world_map",
    0x0040F5A0: "world_map",
    0x0040F5E0: "world_map",
    0x0040F6D0: "world_map",
    0x00411C90: "runtime_platform",
    0x00411D90: "resource_io",
    0x00411F90: "runtime_platform",
    0x00411FA0: "rendering",
    0x004120B0: "world_map",
    0x00424910: "runtime_platform",
    0x00425040: "resource_io",
    0x004251B0: "runtime_platform",
    0x00425B40: "runtime_platform",
    0x00427300: "world_map",
    0x00427920: "story_scene",
    0x004303D0: "rendering",
    0x00431960: "asset_runtime",
    0x00433010: "asset_runtime",
    0x00435650: "rendering",
    0x004372B0: "input_time_rng",
    0x00438000: "resource_io",
    0x00438030: "resource_io",
    0x004380B0: "resource_io",
    0x00438150: "resource_io",
    0x004381E0: "resource_io",
    0x00438230: "resource_io",
    0x00438290: "resource_io",
    0x004382E0: "resource_io",
    0x00438640: "resource_io",
    0x00438B50: "input_time_rng",
    0x00438FA0: "input_time_rng",
    0x00439FD0: "special_modes",
    0x0044EA60: "special_modes",
    0x00451B10: "battle",
    0x00469D20: "battle",
    0x00484920: "audio_video",
    0x00484950: "audio_video",
    0x00485710: "audio_video",
    0x00485740: "audio_video",
    0x004885A0: "resource_io",
    0x00489B10: "input_time_rng",
}

RANGE_HINTS: tuple[tuple[int, int, str, str], ...] = (
    (0x00401190, 0x004011D0, "asset_runtime", "early_asset_initializer"),
    (0x004011D0, 0x00406D30, "world_map", "early_gameplay_cluster"),
    (0x00406D30, 0x004070A0, "special_modes", "high_priority_cluster"),
    (0x004070A0, 0x00408CF0, "persistence", "save_state_machine_cluster"),
    (0x00408CF0, 0x00409EC0, "persistence", "save_ui_and_preview_cluster"),
    (0x00409EC0, 0x0040C130, "runtime_platform", "host_and_lifecycle_cluster"),
    (0x0040C130, 0x0040CDD0, "world_map", "world_load_cluster"),
    (0x0040CDD0, 0x0040D050, "audio_video", "frame_audio_cluster"),
    (0x0040D050, 0x0040E0B0, "runtime_platform", "shared_runtime_state_cluster"),
    (0x0040E0B0, 0x004154A0, "world_map", "world_role_render_coordination_cluster"),
    (0x004154A0, 0x00416D30, "asset_runtime", "ani_runtime_cluster"),
    (0x00416D30, 0x00423A10, "rendering", "software_render_cluster"),
    (0x00423A10, 0x00424390, "resource_io", "configuration_and_path_cluster"),
    (0x00424390, 0x00425570, "runtime_platform", "startup_init_release_cluster"),
    (0x00425570, 0x00425BE0, "resource_io", "resource_initialization_cluster"),
    (0x00425BE0, 0x00427920, "world_map", "map_loader_and_cache_cluster"),
    (0x00427920, 0x004303D0, "story_scene", "story_vm_cluster"),
    (0x004303D0, 0x00430C60, "rendering", "bitmap_output_cluster"),
    (0x00430C60, 0x004351F0, "asset_runtime", "tsw_act_action_cluster"),
    (0x004351F0, 0x00436FB0, "rendering", "font_and_glyph_cluster"),
    (0x00436FB0, 0x00437570, "input_time_rng", "directinput_cluster"),
    (0x00437570, 0x00438000, "rendering", "directdraw_wrapper_cluster"),
    (0x00438000, 0x00438B50, "resource_io", "file_object_cluster"),
    (0x00438B50, 0x004399E0, "input_time_rng", "ime_and_rng_cluster"),
    (0x004399E0, 0x00439FD0, "resource_io", "decompressor_cluster"),
    (0x00439FD0, 0x00451730, "special_modes", "menu_and_shop_cluster"),
    (0x00451730, 0x004841B0, "battle", "battle_cluster"),
    (0x004841B0, 0x00486AB0, "audio_video", "bink_and_miles_cluster"),
)

TOKEN_HINTS: tuple[tuple[str, tuple[str, ...], int, str], ...] = (
    ("persistence", (".sav", "save\\\\%d.sav", "save\\%d.sav"), 30, "save_token"),
    ("battle", ("battle.ffd", "figtalk.dat"), 30, "battle_asset_token"),
    (
        "audio_video",
        (
            "ail_open_",
            "ail_stream_",
            "ail_sample_",
            "ail_sequence_",
            "ail_allocate_",
            "ail_start_",
            "ail_end_",
            "ail_set_",
            "bink",
            "all.snd",
            ".mp3",
        ),
        28,
        "media_api_or_asset_token",
    ),
    ("input_time_rng", ("directinput", "getdevicestate", "immget", "immset"), 28, "input_api_token"),
    ("asset_runtime", (".tsw", ".act", ".ani"), 26, "asset_container_token"),
    ("rendering", ("directdraw", "ddraw", "->blt", " blt("), 3, "render_api_token"),
    ("world_map", ("huge.lmf", "mcache.dat"), 26, "map_asset_token"),
)

STRONG_UTILITY_SEEDS: dict[int, str] = {
    0x00439070: "input_time_rng",  # second RNG value producer
    0x0044A240: "runtime_platform",  # inert diagnostic sink used across modules
    0x00485330: "audio_video",  # AIL_serve wrapper; callers do not own audio
}

# Additional module decisions resolved from complete assembly bodies, direct
# callers and existing assembly evidence.  This scope is intentionally separate
# from the locked critical-ABI/top-level scopes.
MANUAL_OWNERSHIP_SEEDS: dict[int, str] = {
    0x004014F0: "rendering",  # raw framebuffer to packed draw-command stream encoder
    0x004019A0: "rendering",  # packed draw-command stream to raw framebuffer decoder
    0x00401B70: "rendering",  # packed draw-command pixel-format conversion
    0x00401C70: "rendering",  # indexed draw-command stream to 16-bit conversion
    0x00401E50: "rendering",  # embedded-palette draw-command conversion
    0x004053C0: "input_time_rng",  # input repeat/hold-state timer update
    0x0040C020: "world_map",  # role table scan by selector
    0x0040C060: "world_map",  # role index to selector, including original error path
    0x0040C0D0: "world_map",  # role selector/current-role to role index
    0x0040C100: "world_map",  # role lookup wrapper
    0x0040AD10: "asset_runtime",  # compressed indexed-image load and conversion
    0x0040AE20: "world_map",  # map occupancy-bit clearing over an object footprint
    0x0040AEC0: "world_map",  # map occupancy-bit setting over an object footprint
    0x0040AF70: "story_scene",  # dialog object construction/setup
    0x0040AFF0: "story_scene",  # dialog object construction/setup
    0x0040B7F0: "story_scene",  # dialog control-code/text expansion
    0x0040BAA0: "story_scene",  # dialog text replacement helper
    0x0040BB20: "story_scene",  # dialog-object list append
    0x0040BB50: "world_map",  # map-neighbour collision-bit summary
    0x0040D060: "world_map",  # apply map-role record data
    0x0040D0C0: "world_map",  # controlled-role camera viewport clamp
    0x0040D160: "world_map",  # clamped camera rectangle calculation
    0x0040D200: "world_map",  # role map-transfer coordination
    0x0040D3C0: "world_map",  # copy live role fields into the world role table
    0x0040D460: "world_map",  # role/map request-table partial update
    0x0040D560: "world_map",  # role request-table lookup/populate
    0x0040D610: "world_map",  # role/map object removal
    0x0040D790: "world_map",  # role update and map transfer
    0x0040D9E0: "world_map",  # map-data linked-list load
    0x0040DA60: "world_map",  # world encounter/talk/region selection
    0x0040DB40: "world_map",  # world-region list count
    0x0040DB60: "world_map",  # current world-region coordinate lookup
    0x0040DC00: "asset_runtime",  # 0x98-byte ActionRecord initializer
    0x0040DC30: "world_map",  # world linked-list lookup
    0x0040DCE0: "story_scene",  # exact-one set / otherwise clear story-bit wrapper
    0x0040DD10: "world_map",  # world movement-step setter
    0x0040DD20: "input_time_rng",  # frame-interval threshold setter
    0x0040DD30: "input_time_rng",  # frame-interval threshold clear
    0x0040DD40: "world_map",  # 0x21c-byte map-object reset to 0xff
    0x0040DD60: "world_map",  # world-record copy
    0x0040DE50: "rendering",  # 16-bit framebuffer border/effect writer
    0x0040E030: "world_map",  # controlled-role collision/path query
    0x0040E080: "rendering",  # save-thumbnail framebuffer downsample
    0x0040EB60: "audio_video",  # Music path and MP3 filename construction
    0x0040EBF0: "asset_runtime",  # ActionRecord update/TSW lookup/blitter adapter
    0x0040EC80: "asset_runtime",  # TSW frame lookup/blitter adapter
    0x0040ECC0: "asset_runtime",  # ActionRecord update/TSW lookup/blitter adapter
    0x0040F040: "resource_io",  # maps/path/talk database open and mapping setup
    0x0040F890: "special_modes",  # in-game role/item modal dialog procedure
    0x004103C0: "special_modes",  # role/item dialog control setup
    0x00410490: "special_modes",  # role/item dialog row formatting
    0x00410600: "special_modes",  # role/item dialog row formatting
    0x00410730: "special_modes",  # role/item dialog page population
    0x00411030: "special_modes",  # role/item dialog level-data update helper
    0x004112B0: "special_modes",  # role/item dialog state helper
    0x00411700: "special_modes",  # special-mode object label/panel drawing
    0x004117F0: "rendering",  # software digit-string drawing helper
    0x004118B0: "resource_io",  # optical-media resource check and prompt loop
    0x00412050: "resource_io",  # file-existence probe used by media resource checks
    0x00414B60: "rendering",  # shared PicPaint action-list update and drawing
    0x00414CE0: "rendering",  # shared role-head picture/effect list drawing
    0x00414E50: "rendering",  # shared packed-pixel effect update and drawing
    0x004153D0: "rendering",  # transient text-message presentation list
    0x00416FF0: "rendering",  # software blitter clip rectangle
    0x00416F10: "rendering",  # DirectDraw surface description query
    0x00416F60: "rendering",  # DirectDraw surface binding wrapper
    0x004170E0: "rendering",  # software blitter clipping and dispatch
    0x00417DE0: "rendering",  # packed-pixel row effect
    0x00420490: "rendering",  # packed-pixel RGB channel adjustment
    0x004238B0: "rendering",  # selected pixel-format conversion wrapper
    0x004239D0: "rendering",  # packed 16-bit color conversion/update
    0x00424EF0: "runtime_platform",  # input/audio/display subsystem initialization coordinator
    0x00425150: "resource_io",  # ensure and select resource directory
    0x00425570: "resource_io",  # env.dat and CM/mcache cache invalidation/rebuild
    0x004267E0: "resource_io",  # allocated-buffer decompression wrapper
    0x0042E850: "rendering",  # tiled/nine-slice panel drawing
    0x00430BE0: "runtime_platform",  # shared signed decimal parser
    0x00435160: "rendering",  # font renderer object initialization
    0x00435660: "rendering",  # font renderer state setter
    0x00435670: "rendering",  # font renderer state setter
    0x00436AD0: "rendering",  # byte-string glyph cache and software text draw
    0x004372D0: "input_time_rng",  # DirectInput keyboard-state high-bit query
    0x00437B60: "rendering",  # DirectDraw surface creation wrapper
    0x00438340: "resource_io",  # file size query
    0x00438360: "resource_io",  # truncate file at current position
    0x00438380: "resource_io",  # file read wrapper
    0x004383E0: "resource_io",  # file write wrapper
    0x004384B0: "resource_io",  # relative file seek wrapper
    0x00439070: "input_time_rng",  # bounded RNG with original rejection loop
    0x00439120: "resource_io",  # compressor implementation used by resource wrapper
    0x00439210: "resource_io",  # compressor match-search stage
    0x00439580: "resource_io",  # compressor implementation used by Fame persistence
    0x00439670: "resource_io",  # compressor match-search stage
    0x0043B110: "rendering",  # clipped framebuffer rectangle/effect fill
    0x0043BAB0: "rendering",  # generic bordered-panel drawing composition
    0x0044A240: "runtime_platform",  # one-byte retn diagnostic/no-op sink
    0x0044D2D0: "special_modes",  # shared item-list quantity/update/create operation
    0x0044D680: "special_modes",  # item-list lookup
    0x0044FFC0: "battle",  # battle presentation/state helper cluster
    0x0044FFE0: "battle",  # battle presentation/state helper cluster
    0x00450270: "battle",  # battle presentation/state helper cluster
    0x004502B0: "battle",  # battle presentation/state helper cluster
    0x00450400: "battle",  # battle presentation/state helper cluster
    0x00450490: "battle",  # battle presentation/state helper cluster
    0x004504E0: "battle",  # battle presentation/state helper cluster
    0x00450530: "battle",  # battle presentation/state helper cluster
    0x004505B0: "battle",  # battle presentation/state helper cluster
    0x00450630: "battle",  # battle presentation/state helper cluster
    0x004506B0: "battle",  # battle presentation/state helper cluster
    0x004507A0: "battle",  # battle presentation/state helper cluster
    0x00450900: "battle",  # battle presentation/state helper cluster
    0x004509D0: "battle",  # battle presentation/state helper cluster
    0x00450A50: "battle",  # battle presentation/state helper cluster
    0x00450A80: "battle",  # battle presentation/state helper cluster
    0x00450B40: "battle",  # battle presentation/state helper cluster
    0x00450B60: "battle",  # battle presentation/state helper cluster
    0x00450BD0: "battle",  # battle presentation/state helper cluster
    0x00450C50: "battle",  # battle presentation/state helper cluster
    0x00450F90: "battle",  # battle presentation/state helper cluster
    0x00451100: "battle",  # battle presentation/state helper cluster
    0x004512B0: "battle",  # battle presentation/state helper cluster
    0x00451420: "battle",  # battle presentation/state helper cluster
    0x00451540: "battle",  # battle presentation/state helper cluster
    0x004515E0: "battle",  # battle presentation/state helper cluster
    0x00451940: "battle",  # battle-only all_map2.tsw background load/orchestration
    0x00451A90: "rendering",  # battle auxiliary DirectDraw surface creation
    0x00451AE0: "rendering",  # battle auxiliary DirectDraw surface release
    0x004527E0: "battle",  # battle transition/effect/music state coordinator
    0x00476DB0: "battle",  # mon.dat definition snapshot loader
    0x00477800: "battle",  # battle/item definition list value lookup
    0x00477C90: "persistence",  # embedded Fame save-record serialization/compression
    0x00477F10: "persistence",  # embedded Fame save-record decompression/load
    0x00478110: "persistence",  # persistent Fame list teardown/reset
    0x00485330: "audio_video",  # Miles AIL_serve import wrapper
    0x00485610: "audio_video",  # sound-effect playback wrapper
    0x00485650: "audio_video",  # sound playback-state wrapper
    0x00485850: "audio_video",  # music volume wrapper
    0x00485880: "audio_video",  # music playback-state/volume transition
    0x00411530: "world_map",  # map-cell linked-object detach/reconcile
    0x00412930: "world_map",  # complete ordinary-world render coordinator
    0x00426820: "resource_io",  # common decompressor argument-order wrapper
}

# External routines whose generic contract and actual game consumer groups were
# checked during A2.  They remain external; this set only records review state.
MANUAL_EXTERNAL_BOUNDARY_REVIEWS: set[int] = {
    0x00424390,  # compiler floating-conversion initialization glue
    0x004272D0,  # compiler global file-object constructor thunk
    0x00451870,  # compiler global battle-object constructor thunk
    0x004518B0,  # compiler global asset-object constructor thunk
    0x00451910,  # compiler global file-object constructor thunk
    0x00484590,  # Concurrency ReaderWriterLock constructor
    0x00486AB0,  # DirectDrawCreate
    0x00487AD8,  # DirectInputCreateA
    0x00487ADE,  # ImmIsIME
    0x00487BA0,  # atexit
    0x00487C10,  # malloc
    0x004889B0,  # msize
    0x00489654,  # compiler float-to-integer helper
    0x00489B20,  # rand
    0x00489B50,  # difftime
    0x00489B70,  # time
    0x00489D00,  # operator delete
    0x00489D90,  # vsprintf
    0x00489E90,  # operator new
    0x00489EB0,  # memcpy
    0x0048A1F0,  # floor
    0x0048A4C0,  # compiler vector constructor iterator
    0x0048A560,  # compiler vector destructor iterator
    0x0048A6C0,  # strstr
}


def range_hint(address: int) -> tuple[str, str] | None:
    for start, end, module, label in RANGE_HINTS:
        if start <= address < end:
            return module, label
    return None


def code_origin(row: FunctionRow) -> str:
    compiler_glue = {
        0x004011D0,
        0x00401210,
        0x00401250,
        0x00401290,
        0x004012D0,
        0x00401310,
        0x00401350,
        0x00401390,
        0x004013D0,
        0x00401410,
        0x00401450,
        0x004014B0,
    }
    if row.address in compiler_glue:
        return "crt"
    if row.export_status == "skipped" or "unknown_libname" in row.ida_name:
        return "crt"
    if 0x00487AF0 <= row.address:
        return "crt"
    if 0x00486AB0 <= row.address < 0x00487AF0:
        return "third_party"
    return "game"


def source_tokens(row: FunctionRow) -> str:
    if not row.exported_file:
        return ""
    path = EXPORT_ROOT / row.exported_file
    if not path.is_file():
        return ""
    return path.read_text(encoding="utf-8", errors="replace").lower()


def build_base_scores(
    functions: list[FunctionRow], fixed_seeds: dict[int, str]
) -> tuple[dict[int, Counter[str]], dict[int, list[str]], dict[int, str]]:
    scores: dict[int, Counter[str]] = defaultdict(Counter)
    bases: dict[int, list[str]] = defaultdict(list)
    range_modules: dict[int, str] = {}
    for row in functions:
        if code_origin(row) != "game":
            continue
        hint = range_hint(row.address)
        if hint:
            module, label = hint
            scores[row.address][module] += 20
            bases[row.address].append(f"address_cluster:{label}")
            range_modules[row.address] = module
        text = source_tokens(row)
        for module, tokens, weight, label in TOKEN_HINTS:
            if text and any(token in text for token in tokens):
                scores[row.address][module] += weight
                bases[row.address].append(f"decompile_token_navigation_only:{label}")
        if row.address in fixed_seeds:
            scores[row.address][fixed_seeds[row.address]] += 1000
        if row.address in STRONG_UTILITY_SEEDS:
            scores[row.address][STRONG_UTILITY_SEEDS[row.address]] += 200
            bases[row.address].append("assembly_cross_cutting_utility_role")
    return scores, bases, range_modules


def select_label(scores: Counter[str], preferred: str | None = None) -> str:
    if not scores:
        return "unresolved"
    maximum = max(scores.values())
    tied = sorted(module for module, score in scores.items() if score == maximum)
    if preferred in tied:
        return preferred
    return tied[0]


def classify_game_functions(
    functions: list[FunctionRow],
    callees: dict[int, Counter[int]],
    callers: dict[int, Counter[int]],
    fixed_seeds: dict[int, str],
) -> tuple[dict[int, str], dict[int, Counter[str]], dict[int, list[str]], dict[int, str]]:
    base_scores, bases, range_modules = build_base_scores(functions, fixed_seeds)
    game_addresses = {row.address for row in functions if code_origin(row) == "game"}
    labels = {
        address: fixed_seeds.get(
            address,
            select_label(base_scores[address], range_modules.get(address)),
        )
        for address in game_addresses
    }

    final_scores: dict[int, Counter[str]] = {}
    for _ in range(6):
        updated: dict[int, str] = {}
        final_scores = {}
        for address in game_addresses:
            if address in fixed_seeds:
                updated[address] = fixed_seeds[address]
                final_scores[address] = Counter(base_scores[address])
                continue
            score = Counter(base_scores[address])
            neighbour_score: Counter[str] = Counter()
            for destination, count in callees.get(address, {}).items():
                module = labels.get(destination)
                if module in GAME_MODULES:
                    neighbour_score[module] += min(count, 3) * 2
            for source, count in callers.get(address, {}).items():
                module = labels.get(source)
                if module in GAME_MODULES:
                    neighbour_score[module] += min(count, 3)
            for module, value in neighbour_score.items():
                score[module] += min(value, 6)
            updated[address] = select_label(score, range_modules.get(address))
            final_scores[address] = score
        labels = updated
    return labels, final_scores, bases, range_modules


def external_follow_up(
    row: FunctionRow,
    callers: dict[int, Counter[int]],
    game_labels: dict[int, str],
    fixed_seeds: dict[int, str],
) -> str:
    if row.address in fixed_seeds:
        return fixed_seeds[row.address]
    exact = {
        0x00486AB0: "rendering",
        0x00487AD8: "input_time_rng",
        0x00487ADE: "input_time_rng",
        0x00489B10: "input_time_rng",
        0x00489B20: "input_time_rng",
        0x00489B70: "input_time_rng",
        0x00489EB0: "resource_io",
        0x0048A930: "resource_io",
    }
    if row.address in exact:
        return exact[row.address]
    scores: Counter[str] = Counter()
    for source, count in callers.get(row.address, {}).items():
        module = game_labels.get(source)
        if module in GAME_MODULES:
            scores[module] += count
    if scores:
        return select_label(scores)
    return "runtime_platform"


def ownership_rows(
    functions: list[FunctionRow],
    callees: dict[int, Counter[int]],
    callers: dict[int, Counter[int]],
    critical_seeds: dict[int, str],
) -> dict[int, Ownership]:
    fixed_seeds = dict(TOP_LEVEL_SEEDS)
    for address, module in critical_seeds.items():
        previous = fixed_seeds.get(address)
        if previous is not None and previous != module:
            raise SystemExit(
                f"critical/top-level module conflict at 0x{address:08X}: {previous} vs {module}"
            )
        fixed_seeds[address] = module
    for address, module in MANUAL_OWNERSHIP_SEEDS.items():
        previous = fixed_seeds.get(address)
        if previous is not None and previous != module:
            raise SystemExit(
                f"manual/fixed module conflict at 0x{address:08X}: {previous} vs {module}"
            )
        fixed_seeds[address] = module

    labels, final_scores, bases, range_modules = classify_game_functions(
        functions, callees, callers, fixed_seeds
    )
    result: dict[int, Ownership] = {}
    for row in functions:
        origin = code_origin(row)
        scopes: list[str] = []
        if row.address in critical_seeds:
            scopes.append("critical_abi")
        if row.address in TOP_LEVEL_SEEDS:
            scopes.append("top_level_direct")
        if row.address in MANUAL_OWNERSHIP_SEEDS:
            scopes.append("module_candidate_manual")
        if row.address in MANUAL_EXTERNAL_BOUNDARY_REVIEWS:
            scopes.append("external_contract")
        review_scope = ";".join(scopes)

        if origin != "game":
            external_module = "external_crt" if origin == "crt" else "external_third_party"
            result[row.address] = Ownership(
                code_origin=origin,
                module_candidate=external_module,
                follow_up_module=external_follow_up(row, callers, labels, fixed_seeds),
                confidence="confirmed_boundary" if review_scope else "mechanical_boundary",
                assignment_basis=(
                    f"code_origin:{origin}"
                    + (";manual_reviewed_boundary_seed" if review_scope else "")
                ),
                review_scope=review_scope,
                review_status=(
                    "manual_reviewed_current_assembly"
                    if row.address in MANUAL_EXTERNAL_BOUNDARY_REVIEWS
                    else "manual_reviewed_existing_assembly_evidence"
                    if review_scope
                    else "external_boundary_mechanical"
                ),
            )
            continue

        module = labels.get(row.address, "unresolved")
        score = final_scores.get(row.address, Counter())
        ordered = sorted(score.values(), reverse=True)
        difference = ordered[0] - ordered[1] if len(ordered) > 1 else (ordered[0] if ordered else 0)
        if row.address in fixed_seeds:
            confidence = "confirmed_boundary"
            basis = []
            if row.address in critical_seeds:
                basis.append("manual_critical_abi_seed")
            if row.address in critical_seeds and row.address in TOP_LEVEL_SEEDS:
                basis.append("manual_top_level_seed")
            elif row.address in TOP_LEVEL_SEEDS:
                basis.append("manual_top_level_seed")
            if row.address in MANUAL_OWNERSHIP_SEEDS:
                basis.append("manual_module_assembly_review")
                review_status = "manual_reviewed_current_assembly"
            else:
                review_status = "manual_reviewed_existing_assembly_evidence"
        else:
            basis = list(bases.get(row.address, []))
            neighbour_count = sum(callees.get(row.address, {}).values()) + sum(
                callers.get(row.address, {}).values()
            )
            if neighbour_count:
                basis.append(f"assembly_direct_call_graph:{neighbour_count}_sites")
            if module == "unresolved":
                confidence = "unresolved"
            elif difference >= 6:
                confidence = "medium"
            else:
                confidence = "low"
            review_status = "mechanical_candidate_not_manually_reviewed"
        if row.export_status == "assembly-proc-only-not-in-export-manifests":
            basis.append("assembly_proc_only_entry")
        if not basis:
            basis.append("no_positive_mechanical_evidence")

        follow_up = module
        if module == "unresolved":
            follow_up = range_modules.get(row.address, "runtime_platform")
        result[row.address] = Ownership(
            code_origin=origin,
            module_candidate=module,
            follow_up_module=follow_up,
            confidence=confidence,
            assignment_basis=";".join(basis),
            review_scope=review_scope,
            review_status=review_status,
        )

    if len(result) != len(functions):
        raise SystemExit("ownership rows do not cover the function union")
    return result


def mark_reviewed_game_boundaries(
    functions: list[FunctionRow],
    ownership: dict[int, Ownership],
    callsites: dict[tuple[int, int], list[int]],
) -> dict[int, Ownership]:
    """Lock and mark the complete A2 game-to-game boundary review set."""
    function_addresses = {row.address for row in functions}
    caller_modules_by_callee: dict[int, set[str]] = defaultdict(set)
    for caller, callee in callsites:
        if caller not in function_addresses or callee not in function_addresses:
            continue
        caller_owner = ownership[caller]
        callee_owner = ownership[callee]
        if caller_owner.code_origin != "game" or callee_owner.code_origin != "game":
            continue
        if caller_owner.module_candidate == callee_owner.module_candidate:
            continue
        caller_modules_by_callee[callee].add(caller_owner.module_candidate)

    digest_rows = [
        "\t".join(
            (
                f"0x{callee:08X}",
                ownership[callee].module_candidate,
                ";".join(sorted(caller_modules)),
            )
        )
        for callee, caller_modules in sorted(caller_modules_by_callee.items())
    ]
    digest = hashlib.sha256(("\n".join(digest_rows) + "\n").encode("utf-8")).hexdigest()
    if len(digest_rows) != EXPECTED_REVIEWED_GAME_BOUNDARY_COUNT:
        raise SystemExit(
            "reviewed game boundary count changed: "
            f"{len(digest_rows)} != {EXPECTED_REVIEWED_GAME_BOUNDARY_COUNT}"
        )
    if digest != EXPECTED_REVIEWED_GAME_BOUNDARY_SHA256:
        raise SystemExit(
            "reviewed game boundary digest changed: "
            f"{digest} != {EXPECTED_REVIEWED_GAME_BOUNDARY_SHA256}"
        )

    result = dict(ownership)
    for address in caller_modules_by_callee:
        item = result[address]
        scopes = [scope for scope in item.review_scope.split(";") if scope]
        if "cross_module_boundary_a2" not in scopes:
            scopes.append("cross_module_boundary_a2")
        bases = [basis for basis in item.assignment_basis.split(";") if basis]
        if "manual_a2_cross_module_boundary_review" not in bases:
            bases.append("manual_a2_cross_module_boundary_review")
        result[address] = replace(
            item,
            confidence="confirmed_boundary",
            assignment_basis=";".join(bases),
            review_scope=";".join(scopes),
            review_status="manual_reviewed_current_assembly",
        )
    return result


def format_edges(edges: Counter[int]) -> str:
    return ";".join(f"0x{address:08X}:{count}" for address, count in sorted(edges.items()))


def write_output(
    functions: list[FunctionRow],
    ownership: dict[int, Ownership],
    callees: dict[int, Counter[int]],
    callers: dict[int, Counter[int]],
) -> None:
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "address",
                "ida_name_navigation_only",
                "catalog_source",
                "code_origin",
                "module_candidate",
                "candidate_confidence",
                "assignment_basis",
                "assembly_direct_callers_address_count",
                "assembly_direct_callees_address_count",
                "follow_up_module",
                "manual_review_scope",
                "review_status",
            )
        )
        for row in sorted(functions, key=lambda item: item.address):
            item = ownership[row.address]
            writer.writerow(
                (
                    f"0x{row.address:08X}",
                    row.ida_name,
                    (
                        "assembly_proc_only"
                        if row.export_status == "assembly-proc-only-not-in-export-manifests"
                        else "ida_export_catalog"
                    ),
                    item.code_origin,
                    item.module_candidate,
                    item.confidence,
                    item.assignment_basis,
                    format_edges(callers.get(row.address, Counter())),
                    format_edges(callees.get(row.address, Counter())),
                    item.follow_up_module,
                    item.review_scope,
                    item.review_status,
                )
            )


def write_cross_calls(
    functions: list[FunctionRow],
    ownership: dict[int, Ownership],
    callsites: dict[tuple[int, int], list[int]],
) -> Counter[str]:
    function_by_address = {row.address: row for row in functions}
    counts: Counter[str] = Counter()
    with CROSS_CALL_OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "caller_address",
                "caller_name_navigation_only",
                "caller_code_origin",
                "caller_module_candidate",
                "callee_address",
                "callee_name_navigation_only",
                "callee_code_origin",
                "callee_module_candidate",
                "callsite_count",
                "assembly_callsite_addresses",
                "boundary_kind",
                "review_status",
            )
        )
        for (caller, callee), sites in sorted(callsites.items()):
            if caller not in function_by_address or callee not in function_by_address:
                continue
            caller_owner = ownership[caller]
            callee_owner = ownership[callee]
            if caller_owner.code_origin == "game" and callee_owner.code_origin == "game":
                if caller_owner.module_candidate == callee_owner.module_candidate:
                    continue
                boundary_kind = "game_cross_module"
                review_status = (
                    callee_owner.review_status
                    if callee_owner.review_status.startswith("manual_reviewed")
                    else "pending_manual_boundary_review"
                )
            elif caller_owner.code_origin == "game" and callee_owner.code_origin != "game":
                boundary_kind = "game_to_external"
                review_status = "external_boundary_consumer_contract_review"
            elif caller_owner.code_origin != "game" and callee_owner.code_origin == "game":
                boundary_kind = "external_to_game"
                review_status = "external_boundary_entry_contract_review"
            else:
                continue
            counts[boundary_kind] += 1
            writer.writerow(
                (
                    f"0x{caller:08X}",
                    function_by_address[caller].ida_name,
                    caller_owner.code_origin,
                    caller_owner.module_candidate,
                    f"0x{callee:08X}",
                    function_by_address[callee].ida_name,
                    callee_owner.code_origin,
                    callee_owner.module_candidate,
                    len(sites),
                    ";".join(f"0x{site:08X}" for site in sorted(sites)),
                    boundary_kind,
                    review_status,
                )
            )
    return counts


def write_boundary_functions(
    functions: list[FunctionRow],
    ownership: dict[int, Ownership],
    callsites: dict[tuple[int, int], list[int]],
) -> Counter[str]:
    """Aggregate cross-boundary edges by actual callee contract."""
    function_by_address = {row.address: row for row in functions}
    abi_by_address = {
        int(row["address"], 16): row for row in read_tsv(ABI_PATH)
    }
    boundary_edges: dict[int, list[tuple[int, list[int], str]]] = defaultdict(list)
    for (caller, callee), sites in sorted(callsites.items()):
        if caller not in function_by_address or callee not in function_by_address:
            continue
        caller_owner = ownership[caller]
        callee_owner = ownership[callee]
        if caller_owner.code_origin == "game" and callee_owner.code_origin == "game":
            if caller_owner.module_candidate == callee_owner.module_candidate:
                continue
            kind = "game_cross_module"
        elif caller_owner.code_origin == "game" and callee_owner.code_origin != "game":
            kind = "game_to_external"
        elif caller_owner.code_origin != "game" and callee_owner.code_origin == "game":
            kind = "external_to_game"
        else:
            continue
        boundary_edges[callee].append((caller, sites, kind))

    counts: Counter[str] = Counter()
    with BOUNDARY_FUNCTION_OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "callee_address",
                "callee_name_navigation_only",
                "callee_code_origin",
                "callee_module_candidate",
                "boundary_kinds",
                "caller_modules",
                "caller_function_count",
                "assembly_callsite_count",
                "positive_stack_argument_hint_count",
                "retn_pop_values_hex",
                "assembly_cleanup_class",
                "eax_consumed_before_clobber_heuristic",
                "ownership_review_scope",
                "review_priority",
                "review_status",
            )
        )
        for callee, edges in sorted(boundary_edges.items()):
            owner = ownership[callee]
            abi = abi_by_address[callee]
            callers = {caller for caller, _, _ in edges}
            caller_modules = sorted(
                {ownership[caller].module_candidate for caller in callers}
            )
            kinds = sorted({kind for _, _, kind in edges})
            callsite_count = sum(len(sites) for _, sites, _ in edges)
            manually_reviewed = owner.review_status.startswith("manual_reviewed")
            if manually_reviewed:
                priority = "reviewed"
                review_status = owner.review_status
            elif len(callers) >= 10 or len(caller_modules) >= 3:
                priority = "high_fanout"
                review_status = "pending_manual_boundary_review"
            else:
                priority = "normal"
                review_status = "pending_manual_boundary_review"
            counts["unique_boundary_functions"] += 1
            counts[f"unique_{'+'.join(kinds)}"] += 1
            counts[f"priority_{priority}"] += 1
            writer.writerow(
                (
                    f"0x{callee:08X}",
                    function_by_address[callee].ida_name,
                    owner.code_origin,
                    owner.module_candidate,
                    ";".join(kinds),
                    ";".join(caller_modules),
                    len(callers),
                    callsite_count,
                    abi["positive_stack_argument_hint_count"],
                    abi["retn_pop_values_hex"],
                    abi["assembly_cleanup_class"],
                    abi["eax_consumed_before_clobber_heuristic"],
                    owner.review_scope,
                    priority,
                    review_status,
                )
            )
    return counts


def main() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"input hash mismatch for {path.relative_to(WORKSPACE_ROOT)}: {actual} != {expected}"
            )
    functions = load_functions()
    lines = ASM_PATH.read_text(encoding="utf-8", errors="replace").replace("\r", "").splitlines()
    callees, callers, callsites, _ = parse_assembly_graph(lines, functions)
    critical_seeds = load_critical_seeds()
    ownership = ownership_rows(functions, callees, callers, critical_seeds)
    ownership = mark_reviewed_game_boundaries(functions, ownership, callsites)
    write_output(functions, ownership, callees, callers)
    cross_call_counts = write_cross_calls(functions, ownership, callsites)
    boundary_function_counts = write_boundary_functions(functions, ownership, callsites)

    origin_counts = Counter(item.code_origin for item in ownership.values())
    module_counts = Counter(item.module_candidate for item in ownership.values())
    confidence_counts = Counter(item.confidence for item in ownership.values())
    print(f"wrote {len(functions)} ownership rows to {OUTPUT_PATH.relative_to(RESEARCH_ROOT)}")
    print("origins " + " ".join(f"{key}={value}" for key, value in sorted(origin_counts.items())))
    print("modules " + " ".join(f"{key}={value}" for key, value in sorted(module_counts.items())))
    print("confidence " + " ".join(f"{key}={value}" for key, value in sorted(confidence_counts.items())))
    print("cross_calls " + " ".join(f"{key}={value}" for key, value in sorted(cross_call_counts.items())))
    print(
        "boundary_functions "
        + " ".join(f"{key}={value}" for key, value in sorted(boundary_function_counts.items()))
    )


if __name__ == "__main__":
    main()
