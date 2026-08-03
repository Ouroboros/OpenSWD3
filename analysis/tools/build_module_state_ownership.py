#!/usr/bin/env python3
"""Build the Stage A3 module state-ownership inventory."""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
GLOBAL_PATH = INVENTORY_ROOT / "global-ownership.tsv"
FUNCTION_PATH = INVENTORY_ROOT / "module-function-ownership.tsv"
OUTPUT_PATH = INVENTORY_ROOT / "module-state-ownership.tsv"

EXPECTED_SHA256 = {
    GLOBAL_PATH: "a774555262d77485b7ab62fc78a0f840607226f1dd6cb7232805c7ef1ed93d8c",
    FUNCTION_PATH: "7e396e2b636a7f5f6afbd61478b6ad807202992edd49f0eac794458f2173f24f",
}


@dataclass(frozen=True)
class Policy:
    state_kind: str
    owner_module: str
    owner_basis: str
    initialization_or_creation: str
    destruction_or_end: str
    sharing_contract: str


GLOBAL_POLICIES: dict[str, Policy] = {
    "frame_execution_gate": Policy("scalar_gate", "runtime_platform", "top-level frame coordinator", "0x00409EC0", "process lifetime", "input, story and modal UI may request; runtime_platform alone gates the frame"),
    "battle_request_value": Policy("tagged_request", "runtime_platform", "single-frame request consumption", "static zero; reset by 0x0040A570", "process lifetime", "world/story produce; runtime_platform consumes and clears; battle receives the low value"),
    "battle_active_gate": Policy("scalar_gate", "runtime_platform", "single-frame transition authority", "static zero; set on battle entry", "cleared by 0x0040A570", "world may use the immediate-entry shortcut; battle and display lifecycle only observe"),
    "special_mode_request_and_active_state": Policy("tagged_request", "runtime_platform", "single-frame mode dispatch authority", "static zero; reset by 0x00424B90", "process lifetime", "world/story request; runtime_platform dispatches; special_modes advances tagged transitions"),
    "talk_and_story_execution_state": Policy("static_structure", "story_scene", "VM owns script cursor and terminal reset", "0x00425B50; populated by world/story producers", "0x0042D47F terminal reset; process lifetime storage", "world/special modes populate trigger fields; story_scene owns execution and invalidation"),
    "controlled_role_index": Policy("scalar_index", "world_map", "map loader is the single writer", "0x0040C130", "replaced on map load", "story, rendering and audio borrow the selected index"),
    "role_records_256_by_216": Policy("static_structure_array", "world_map", "map/session lifecycle and role simulation", "0x00425B50; 0x0040C130", "dynamic fields released by world teardown", "story and asset runtime borrow records; battle transition may update documented bridge fields"),
    "world_camera_and_player_x_component": Policy("frame_scalar", "world_map", "world movement producer/consumer loop", "world-session reset", "process lifetime", "story helpers may inject; world simulation consumes"),
    "world_camera_and_player_y_component": Policy("frame_scalar", "world_map", "world movement producer/consumer loop", "world-session reset", "process lifetime", "story helpers may inject; world simulation consumes"),
    "world_player_only_x_component": Policy("frame_scalar", "world_map", "world movement producer/consumer loop", "world-session reset", "process lifetime", "story helpers may inject; world simulation consumes"),
    "world_player_only_y_component": Policy("frame_scalar", "world_map", "world movement producer/consumer loop", "world-session reset", "process lifetime", "story helpers may inject; world simulation consumes"),
    "party_item_sentinel_heads_4": Policy("owned_pointer_array", "world_map", "world-session create/destroy lifecycle", "0x0040E0B0", "0x0040F410", "special_modes and battle modify nodes through item operations; persistence only serializes"),
    "world_movement_step": Policy("scalar_parameter", "world_map", "world movement rule", "0x0040DD10 called by world initialization", "process lifetime", "world simulation and world rendering borrow the value"),
    "global_state_bitset_8192": Policy("persistent_bitset", "story_scene", "gameplay flag semantics and accessor contract", "0x0040E0B0; persistence may replace contents", "process lifetime", "world/menu modules use centralized bit access; persistence transports the block"),
    "global_integer_variables_64_element0_money": Policy("persistent_scalar_array", "story_scene", "VM owns general variable rules", "0x0040E0B0; persistence may replace contents", "process lifetime", "special_modes owns only transactional operations on element zero; persistence transports all elements"),
    "process_lifecycle_flags": Policy("bit_flags", "runtime_platform", "process close/media/suppression coordination", "platform initialization", "process teardown", "other modules submit documented bit transitions; runtime_platform combines them"),
    "platform_or_async_transition_frame_suppression": Policy("scalar_gate", "runtime_platform", "frame scheduler suppression", "0x00424B90", "process lifetime", "display lifecycle completes zero/one restoration; world/story only request suppression"),
    "display_active_and_restored_state": Policy("scalar_gate", "runtime_platform", "display activation lifecycle", "0x00424EF0", "display teardown", "rendering establishes resources; message/frame code observes activation"),
    "directdraw_common_source_surface": Policy("platform_resource_handle", "rendering", "render target creation and release", "0x00424EF0 via rendering surface factory", "0x004251B0 release sequence", "all modes borrow one presentation source; no mode owns the handle"),
    "optional_text_input_object_pointer": Policy("owned_optional_pointer", "special_modes", "menu stage owns object lifetime", "0x00448EE0; 0x004490C0", "0x004490C0", "input_time_rng/platform message code forwards events without taking ownership"),
    "miles_audio_manager_object": Policy("static_backend_object", "audio_video", "audio backend lifecycle", "0x00424EF0", "0x004251B0 audio release sequence", "world/story/battle submit audio commands; audio_video owns buffers and reclamation"),
}


SUPPLEMENTAL_ROWS: tuple[tuple[str, ...], ...] = (
    ("process_instance_handle", "0x004C8BD0", "4", "platform_handle", "runtime_platform", "WinMain platform lifecycle", "0x00409EC0", "0x00409EC0", "runtime_platform", "platform consumers", "runtime_platform", "process teardown", "borrowed only at the platform boundary", "confirmed global-state assembly evidence"),
    ("main_window_handle", "0x004C9A1C", "4", "platform_handle", "runtime_platform", "window lifecycle", "0x00409EC0", "0x00409EC0", "runtime_platform", "window, rendering and input consumers", "runtime_platform;rendering;input_time_rng", "WM_DESTROY/process teardown", "business modules never own the native window handle", "confirmed global-state assembly evidence"),
    ("crt_rand_seed_state", "0x004A833C", "4", "rng_state", "input_time_rng", "observable RNG stream authority", "0x00489B10", "0x00489B10;0x00489B20", "external_crt", "0x00489B20", "external_crt", "process lifetime", "game modules consume values only through the RNG boundary", "confirmed global-state assembly evidence"),
    ("secondary_rng_state_250", "0x004A6610", "1000", "rng_state_array", "input_time_rng", "second RNG stream authority", "0x00438FA0", "0x00438FA0;0x00439070", "input_time_rng", "0x00439070", "input_time_rng", "process lifetime", "call order is observable and remains centralized", "confirmed global-state assembly evidence"),
    ("previous_accepted_frame_time", "0x004CC2B0", "4", "time_state", "input_time_rng", "frame-clock rule authority", "first accepted frame", "0x0040A570", "runtime_platform", "0x0040A570", "runtime_platform", "process lifetime", "runtime_platform samples; input_time_rng specifies unsigned delta behavior", "confirmed frame-clock assembly evidence"),
    ("frame_interval_threshold_ms", "0x004B7BCC", "4", "time_parameter", "input_time_rng", "frame-clock rule authority", "0x00424B90;0x0040DD20;0x0040DD30", "0x00424B90;0x0040DD20;0x0040DD30;0x00427920", "input_time_rng;runtime_platform;story_scene", "0x0040A570", "runtime_platform", "process lifetime", "story may request 70/35; frame scheduler applies the exact threshold", "confirmed frame-clock assembly evidence"),
    ("current_frame_time_sample", "0x004AAECC", "4", "time_state", "input_time_rng", "single-sample-per-frame contract", "accepted frame sampling", "0x0040A570", "runtime_platform", "frame/input/story/action consumers", "runtime_platform;input_time_rng;story_scene;asset_runtime", "overwritten on the next accepted frame", "all consumers borrow the same sampled value", "confirmed frame-clock assembly evidence"),
    ("configurable_key_binding_block", "0x004B7384..0x004B7403", "128", "compatibility_block_with_sparse_binding_dwords", "input_time_rng", "raw DIK binding semantics and normalization consumers", "0x00424390 defaults; 0x00423FB0 Env.dat load", "0x00423FB0;0x00424390;0x0044BDA0", "input_time_rng;resource_io;special_modes", "0x004050E0;0x00423AF0;0x00423E00;0x0044BDA0", "input_time_rng;resource_io;special_modes", "process lifetime", "input_time_rng owns sixteen known dword fields; resource_io serializes only their low bytes in explicit file order; special_modes copies the full 0x80-byte block, so holes and high bytes cannot be collapsed into the file record", "confirmed 0x80-byte clear/copy and sixteen dword fields"),
    ("battle_instruction_pointer", "0x0053CE84", "4", "vm_cursor", "battle", "battle VM instruction authority", "0x00469D20 battle setup/dispatch", "battle opcode handlers", "battle", "battle opcode handlers", "battle", "battle teardown", "no non-battle module may advance the cursor", "confirmed global-state assembly evidence"),
    ("battle_external_result_state", "0x0053BDC8", "4", "result_state", "battle", "battle core return contract", "0x00453200", "0x00453200;0x0045E580", "battle", "0x00453200", "battle", "battle return", "runtime_platform receives only the four-value return contract", "confirmed global-state assembly evidence"),
    ("battle_multipurpose_temporary", "0x0053CCE8", "4", "battle_scratch", "battle", "battle-local reuse", "battle handlers", "battle handlers", "battle", "battle handlers", "battle", "battle teardown", "must remain battle-private; one opcode temporarily uses it for the outward result", "confirmed global-state assembly evidence"),
    ("win32_file_mapping_object", "0x00438000 entry family", "80", "resource_object", "resource_io", "file/mapping/view lifecycle", "0x00438000;0x004380B0", "0x00438000..0x00438640", "resource_io", "resource callers", "resource_io;asset_runtime;story_scene;world_map;persistence", "0x00438030;0x00438150", "callers borrow bytes/views; resource_io owns handles and close order", "confirmed structure evidence"),
    ("generic_action_record", "0x0040DC00 layout family", "152", "shared_runtime_record", "asset_runtime", "ACT/action update semantics", "0x0040DC00", "0x0040DC00;0x004321E0 and documented producers", "asset_runtime;story_scene;world_map;battle", "render/update consumers", "asset_runtime;rendering;story_scene;world_map;battle", "parent-object lifecycle", "parent owns storage; asset_runtime owns update semantics and borrowed stream pointers", "confirmed structure/action evidence"),
    ("item_node", "0x0044D0F0 layout family", "176", "owned_list_node", "special_modes", "item operation and deep-copy semantics", "0x0044D0F0..0x0044D6D3", "item, world and battle operations", "special_modes;world_map;battle", "menu/world/battle/persistence consumers", "special_modes;world_map;battle;persistence", "0x0044D5A0;0x0040F410", "list/root owner destroys; persistence transports snapshots without taking ownership", "confirmed structure/item evidence"),
    ("battle_actor_group_a_array", "0x005029D0", "120840", "static_object_array", "battle", "battle actor construction/destruction", "0x004517B0", "battle functions", "battle", "battle functions", "battle", "0x004517E0", "embedded action records borrow asset_runtime semantics", "confirmed structure/battle evidence"),
    ("battle_actor_group_b_array", "0x00525508", "88384", "static_object_array", "battle", "battle actor construction/destruction", "0x00451810", "battle functions", "battle", "battle functions", "battle", "0x00451840", "embedded action records borrow asset_runtime semantics", "confirmed structure/battle evidence"),
    ("picture_action_list", "0x0042B1F1 allocation family", "164", "owned_dynamic_list", "story_scene", "story picture-action production", "0x0042B1F1", "story producers; rendering updater", "story_scene;rendering", "rendering updater", "rendering", "0x004147E0 terminal removal", "story owns scene intent; rendering borrows and retires completed presentation nodes", "confirmed structure/action evidence"),
    ("picpaint_action_lists", "0x004AD3E8;0x004BA6E0", "180", "owned_dynamic_lists", "story_scene", "story PicPaint production", "0x0042A0A6;0x0042A200", "story producers; rendering updater", "story_scene;rendering", "0x00414B60;0x00414CE0", "rendering", "terminal removal in rendering update", "story owns scene intent; rendering owns per-frame update and node retirement", "confirmed structure/action evidence"),
    ("battle_status_action_nodes", "battle actor +0x2584", "172", "owned_dynamic_list", "battle", "battle status lifecycle", "0x0047DCB9", "battle functions", "battle", "battle functions", "battle", "battle status removal/actor teardown", "asset_runtime semantics are borrowed for the embedded action record", "confirmed structure/battle evidence"),
    ("battle_effect_action_nodes", "battle actor +0x236C", "476", "owned_dynamic_list", "battle", "battle effect lifecycle", "0x0048010D", "battle functions", "battle", "battle functions", "battle", "battle effect removal/actor teardown", "asset_runtime semantics are borrowed for two embedded action records", "confirmed structure/battle evidence"),
    ("static_action_pool_4", "0x004AD0D8", "608", "static_record_array", "asset_runtime", "action runtime initialization/update", "asset runtime initialization", "0x004161C0 and related asset functions", "asset_runtime", "world/render consumers", "world_map;rendering", "process lifetime", "consumers borrow records; asset_runtime owns action transitions", "confirmed structure/action evidence"),
    ("act_stream_cache_nodes", "0x00432D82 allocation family", "24", "owned_cache_nodes", "asset_runtime", "selected ACT stream cache", "0x00432D82", "asset runtime cache functions", "asset_runtime", "action update consumers", "asset_runtime;world_map;story_scene;battle", "0x00432EE0", "action records borrow selected streams; cache retains storage ownership", "confirmed structure/ACT evidence"),
    ("act_index_cache_nodes", "0x004330B0 allocation family", "20", "owned_cache_nodes", "asset_runtime", "ACT data-block index cache", "0x004330B0", "asset runtime cache functions", "asset_runtime", "ACT loaders", "asset_runtime", "asset cache teardown", "cache-private nodes do not escape; loaded blocks follow their documented parent lifetime", "confirmed structure/ACT evidence"),
    ("snd_runtime_index_table", "Miles object +0x67C", "48000", "owned_runtime_index", "audio_video", "SND/Miles buffer and reference lifecycle", "0x00486335", "audio functions", "audio_video", "audio functions", "audio_video", "audio backend teardown", "asset bytes are loaded through resource_io; audio_video owns live buffers and reference counts", "confirmed structure/SND evidence"),
    ("map_cache_directory_entries", "0x0042696D record family", "16", "cache_directory", "world_map", "map cache selection and eviction", "map cache load/rebuild", "map cache functions", "world_map;resource_io", "world map loaders", "world_map", "map cache teardown/rebuild", "resource_io owns bytes; world_map owns map-id/use-age/slot policy", "confirmed structure/map-cache evidence"),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def normalize_functions(value: str) -> str:
    tokens = [token for token in re.split(r"[,/]", value) if token]
    return ";".join(f"0x{token.removeprefix('0x').upper()}" for token in tokens)


def modules_for_functions(value: str, module_by_address: dict[str, str]) -> str:
    modules: set[str] = set()
    for address in normalize_functions(value).split(";"):
        if not address:
            continue
        if address == "0x004010B0":
            modules.add("external_crt")
            continue
        module = module_by_address.get(address)
        if module is None:
            raise SystemExit(f"state reference is not an A2 function entry: {address}")
        modules.add(module)
    return ";".join(sorted(modules))


def main() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(f"input hash mismatch for {path.name}: {actual} != {expected}")

    function_rows = read_tsv(FUNCTION_PATH)
    module_by_address = {
        row["address"]: row["module_candidate"] for row in function_rows
    }
    global_rows = read_tsv(GLOBAL_PATH)
    if len(global_rows) != 21:
        raise SystemExit(f"expected 21 established global ownership rows, got {len(global_rows)}")
    if set(GLOBAL_POLICIES) != {row["semantic_name"] for row in global_rows}:
        raise SystemExit("global state policy set does not match global-ownership.tsv")

    output_rows: list[tuple[str, ...]] = []
    for row in global_rows:
        policy = GLOBAL_POLICIES[row["semantic_name"]]
        output_rows.append(
            (
                row["semantic_name"],
                f"0x{row['address']}",
                row["size_bytes"],
                policy.state_kind,
                policy.owner_module,
                policy.owner_basis,
                policy.initialization_or_creation,
                normalize_functions(row["writers"]),
                modules_for_functions(row["writers"], module_by_address),
                normalize_functions(row["readers"]),
                modules_for_functions(row["readers"], module_by_address),
                policy.destruction_or_end,
                policy.sharing_contract,
                row["status"],
                "manual_reviewed_current_assembly",
            )
        )
    output_rows.extend(SUPPLEMENTAL_ROWS)
    if len(output_rows) != 46 or len({row[0] for row in output_rows}) != 46:
        raise SystemExit("expected 46 unique A3 state ownership rows")

    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "state_id",
                "address_or_origin",
                "size_bytes",
                "state_kind",
                "owner_module",
                "owner_basis",
                "initialization_or_creation",
                "writer_functions",
                "writer_modules",
                "reader_functions",
                "reader_modules",
                "destruction_or_end",
                "sharing_contract",
                "evidence_status",
                "review_status",
            )
        )
        writer.writerows(output_rows)

    cross_writer_rows = sum(
        len(set(filter(None, row[8].split(";"))) - {row[4]}) > 0 for row in output_rows
    )
    print(f"wrote {len(output_rows)} state rows to {OUTPUT_PATH.relative_to(RESEARCH_ROOT)}")
    print(f"cross_owner_writer_rows={cross_writer_rows}")


if __name__ == "__main__":
    main()
