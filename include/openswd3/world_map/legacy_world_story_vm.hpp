#pragma once

#include "openswd3/asset_runtime/legacy_ani_activity.hpp"
#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/resource_io/legacy_resource_databases.hpp"
#include "openswd3/rendering/legacy_action_renderers.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_moving_actions.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
#include "openswd3/world_map/legacy_role_head_actions.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_camera_pan.hpp"
#include "openswd3/world_map/legacy_world_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_map_update.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_selection_scroll.hpp"
#include "openswd3/world_map/legacy_world_story_paths.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <array>
#include <list>
#include <span>
#include <vector>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldStoryFlagBytes = 0x400U;
inline constexpr std::size_t kLegacyWorldScriptVariableCount = 64U;

enum LegacyWorldStoryOpcode : compat::u16 {
    OP_07_CLEAR_DIALOG_CONTROL_BIT31 = 7U,
    OP_08_STAGE_DIALOG_LIFETIME = 8U,
    OP_09_CLEAR_DIALOG_CONTROL_BIT30 = 9U,
    OP_10_SET_ROLE_BASE_VARIANT = 10U,
    OP_11_SET_ROLE_VARIANT_DELTA = 11U,
    OP_12_SET_ROLE_POSITION = 12U,
    OP_13_STEP_ROLE = 13U,
    OP_14_WAIT_ROLE_ACTION_STATUS = 14U,
    OP_15_JUMP_SAME_FILE_OFFSET = 15U,
    OP_16_JUMP_IF_ROLE_PATH_UNPREPARED = 16U,
    OP_17_JUMP_IF_ROLE_PATH_PREPARED = 17U,
    OP_18_RELEASE_ROLE_PATH = 18U,
    OP_19_RELEASE_ROLE_PATHS = 19U,
    OP_20_SCHEDULE_ROLE_PATHS = 20U,
    OP_21_JUMP_IF_GLOBAL_BIT_SET = 21U,
    OP_22_JUMP_IF_GLOBAL_BIT_CLEAR = 22U,
    OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET = 23U,
    OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET = 24U,
    OP_25_SET_GLOBAL_BIT = 25U,
    OP_26_CLEAR_GLOBAL_BIT = 26U,
    OP_27_RELOAD_WORLD_SESSION = 27U,
    OP_28_CHANGE_ROLE_PATH_ID = 28U,
    OP_29_SET_GLOBAL_INTEGER = 29U,
    OP_30_ADD_GLOBAL_INTEGER = 30U,
    OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO = 31U,
    OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE = 32U,
    OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE = 33U,
    OP_34_SET_BOUNDED_SCRIPT_CLOCK = 34U,
    OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK = 35U,
    OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA = 36U,
    OP_37_SNAPSHOT_SCRIPT_CLOCK = 37U,
    OP_38_CLEAR_ROLE_FROM_SCENE = 38U,
    OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS = 39U,
    OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH = 40U,
    OP_41_RELOAD_INDEXED_TARGET = 41U,
    OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT = 42U,
    OP_43_CLEAR_INTERACTION_LOCK = 43U,
    OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE = 44U,
    OP_45_SET_ROLE_ACTION_ID = 45U,
    OP_46_RESTORE_ROLE_ACTION_OVERRIDES = 46U,
    OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE = 47U,
    OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE = 48U,
    OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF = 49U,
    OP_50_START_RELATIVE_CAMERA_MOVE = 50U,
    OP_51_WAIT_CAMERA_MOVE_COMPLETE = 51U,
    OP_52_START_FRAME_COLOR_TRANSITION = 52U,
    OP_53_WAIT_FRAME_COLOR_TRANSITION = 53U,
    OP_54_REPEAT_ROLE_ACTION_REFRESH = 54U,
    OP_55_SET_ROLE_SPATIAL_GROUP_1 = 55U,
    OP_56_SET_ROLE_SPATIAL_GROUP_0 = 56U,
    OP_57_SET_ROLE_SPATIAL_GROUP_2 = 57U,
    OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION = 58U,
    OP_59_PLAY_SOUND_EFFECT = 59U,
    OP_60_RESUME_WORLD_SCENE_RENDERING = 60U,
    OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING = 61U,
    OP_62_WRITE_MAP_ROLE = 62U,
    OP_63_SET_SELECTION_SCROLL = 63U,
    OP_64_CLEAR_SELECTION_SCROLL = 64U,
    OP_65_TRANSFER_ROLE_TO_PARTY = 65U,
    OP_66_UPDATE_ROLE_MAP_STATE = 66U,
    OP_67_WAIT_FRAME_CLOCK = 67U,
    OP_68_CLEAR_ROLE_FLAG_0400 = 68U,
    OP_69_SET_ROLE_FLAG_0400 = 69U,
    OP_70_START_ABSOLUTE_CAMERA_MOVE = 70U,
    OP_71_SET_ROLE_HEAD_SIGN = 71U,
    OP_72_CLEAR_ROLE_HEAD_SIGN = 72U,
    OP_73_START_CAMERA_MOVE_TO_ROLE = 73U,
    OP_74_CANCEL_FRAME_COLOR_TRANSITION = 74U,
    OP_75_SUSPEND_STORY_ROLE = 75U,
    OP_76_TURN_AND_SUSPEND_STORY_ROLE = 76U,
    OP_77_SET_ROLE_WAIT_OVERRIDE = 77U,
    OP_78_CLEAR_ROLE_WAIT_OVERRIDE = 78U,
    OP_79_ENQUEUE_MOVING_ACTION = 79U,
    OP_80_CLEAR_TEXT_CONTROL_BIT29 = 80U,
    OP_81_ENQUEUE_ROLE_HEAD_ACTION = 81U,
    OP_82_DISMISS_ROLE_HEAD_ACTION = 82U,
    OP_83_UPSERT_PACKED_ROW_EFFECT = 83U,
    OP_84_CONTROL_PACKED_ROW_EFFECT = 84U,
    OP_85_BEGIN_STORY_VIDEO = 85U,
    OP_86_REWRITE_ROLE_HEAD_ACTION_KEY = 86U,
    OP_87_RELOAD_RANDOM_TARGET = 87U,
    OP_88_REQUEST_BATTLE = 88U,
    OP_91_LOAD_NAME_RECORD = 91U,
    OP_92_SET_RESERVED_GLOBAL_BIT = 92U,
    OP_93_CLEAR_RESERVED_GLOBAL_BIT = 93U,
    OP_94_SET_SCENE_RENDER_BIT1 = 94U,
    OP_95_CLEAR_SCENE_RENDER_BIT1 = 95U,
    OP_96_BEGIN_CUSTOM_ANI = 96U,
    OP_97_WAIT_CUSTOM_ANI_COMPLETE = 97U,
    OP_98_CONSUME_FOUR_BYTE_NOOP = 98U,
    OP_99_WAIT_CUSTOM_ANI_PHASE = 99U,
    OP_100_SET_ROLE_TALK_SCRIPT = 100U,
    OP_101_SET_ROLE_STATUS_BIT26 = 101U,
    OP_102_SET_ROLE_STATUS_BIT6 = 102U,
    OP_103_SET_ROLE_STATUS_BIT5 = 103U,
    OP_104_SET_TEXT_LAYOUT_PAIR = 104U,
    OP_105_CLEAR_TEXT_CONTROL_BIT27 = 105U,
    OP_106_WAIT_PRIMARY_PICTURE_ACTION_BYTE = 106U,
    OP_117_SET_ROLE_STATUS_BIT4 = 117U,
    OP_136_SET_ROLE_STATUS_BIT12 = 136U,
    OP_140_SET_ROLE_STATUS_BIT11 = 140U,
    OP_145_SET_ROLE_STATUS_BIT13 = 145U,
    OP_146_SET_ROLE_STATUS_BIT8 = 146U,
    OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION = 153U,
    OP_154_WAIT_SECONDARY_PICTURE_ACTION_BYTE = 154U,
    OP_174_SET_ROLE_STATUS_BIT14 = 174U,
    OP_162_LOAD_DYNAMIC_NAME_RECORD = 162U,
    OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS = 169U,
    OP_1025 = 1025U,
};

struct LegacyWorldStoryVmState {
    std::array<compat::u8, resource_io::kLegacyTalkWindowSize> window{};
    std::array<compat::u8, kLegacyWorldStoryFlagBytes> flags{};
    std::array<compat::u32, kLegacyWorldScriptVariableCount> script_variables{};
    std::array<compat::u8, 32U> speaker_name{};
    compat::u32 text_control_flags{0xFFFFFFFFU};
    compat::i32 text_layout_first{};
    compat::i32 text_layout_second{};
    compat::u32 next_text_aux_value{60U};
    // sub_425040 establishes these process-level dialog metrics; sub_40E0B0
    // does not own them.
    compat::u32 dialog_scale{11U};
    compat::u32 dialog_character_delay_base{2U};
    // dword_4A135C packs the one-shot anchor override and 0x004CF73C is
    // the one-shot horizontal-centering latch. The shared text handler resets
    // them only after a dialog has been queued.
    compat::u16 dialog_anchor_left{0x8000U};
    compat::u16 dialog_anchor_top{0x8000U};
    bool dialog_center_pending{};
    // sub_40E0B0 clears the staged stream request at 0x004B7C80..88,
    // clears 0x004ACDBC and establishes current stream mode 1/argument 0 at
    // 0x004B7380/0x004B74F0.
    compat::u32 music_request{};
    compat::u32 music_first_stream{};
    compat::u32 music_second_stream{};
    compat::u32 music_control_flags{};
    compat::u32 current_first_stream{1U};
    compat::u32 current_second_stream{};
    compat::u32 wait_duration{};
    compat::u32 wait_started_at{};
    compat::u32 script_clock_frame_counter{};
    compat::u32 script_clock{};
    compat::u32 script_clock_origin{};
    // dword_4CF6D8 is updated at the common interpreter join and read by the
    // default-invalid diagnostic before that update. sub_40E0B0 does not own
    // or reset it.
    compat::u32 previous_opcode{};
    // sub_40E0B0 initializes the deferred map-load coordinates at
    // 0x004A9930/0x004A9938 to -1 and the map id at 0x004CAE88 to zero.
    // Story opcodes 155..157 retain these values across VM steps.
    compat::i32 deferred_map_tile_x{-1};
    compat::i32 deferred_map_tile_y{-1};
    compat::i32 deferred_map_id{};
    // dword_4C8BE0 is serialized beside the deferred map fields and lets
    // sub_40C130 override GUID 1's action when leaving logical map 22.
    compat::u32 guid_one_action_override{};
    compat::u32 loaded_file_number{};
    compat::u32 loaded_data_offset{};
    bool next_text_aux_pending{};
    bool window_loaded{};
};

struct LegacyWorldStoryVmRuntime {
    LegacyRoleSpatialIndex* spatial_index{};
    LegacyWorldRoleSurfaceContext role_surface{};
    std::span<compat::u8> mutable_maps_payload;
    LegacyMapsWorldDatabase* maps_database{};
    std::vector<LegacyWorldRoleRecord>* role_storage{};
    LegacyWorldRoleTransferState* role_transfer_state{};
    compat::u32* live_party_role_count{};
    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount>*
        live_party_object_slots{};
    asset_runtime::LegacyAniRoleParticleEffect* role_particles{};
    compat::u16 current_logical_map_id{};
    std::array<compat::i16, kLegacyWorldSelectionWordCount>* selection_words{};
    LegacyWorldSelectionScrollState* selection_scroll{};
    LegacyWorldCameraRect* camera{};
    LegacyWorldCameraPanState* camera_pan{};
    LegacyWorldMovementRuntimeState* movement{};
    LegacyPictureActionLists* picture_actions{};
    std::list<rendering::LegacyPackedRowEffect>* packed_row_effects{};
    LegacyMovingActionList* moving_actions{};
    LegacyRoleHeadActionList* role_head_actions{};
    compat::u32* battle_request_value{};
    rendering::LegacyFrameColorTransitionState* frame_color{};
    LegacyWorldStoryPathRuntime* story_paths{};
    compat::u32* indexed_target_selector{};
    compat::u8* scene_render_flags{};
    compat::u32 map_height{};
    compat::u32 current_tick{};
    input_time_rng::LegacySecondaryRng* secondary_rng{};
};

void initialize_legacy_world_story_vm(LegacyWorldStoryVmState& state) noexcept;
void advance_legacy_world_script_clock(LegacyWorldStoryVmState& state) noexcept;

[[nodiscard]] bool query_legacy_world_story_flag(
    const LegacyWorldStoryVmState& state, compat::u16 bit_index
) noexcept;
void set_legacy_world_story_flag(
    LegacyWorldStoryVmState& state, compat::u16 bit_index
) noexcept;
void clear_legacy_world_story_flag(
    LegacyWorldStoryVmState& state, compat::u16 bit_index
) noexcept;

class LegacyWorldStoryVmPorts {
public:
    virtual ~LegacyWorldStoryVmPorts() = default;

    [[nodiscard]] virtual resource_io::LegacyTalkWindowLoadResult
    load_story_window(
        compat::i32 story_id,
        std::span<compat::u8, resource_io::kLegacyTalkWindowSize> destination,
        bool clear_before_read
    ) = 0;
    [[nodiscard]] virtual resource_io::LegacyTalkWindowLoadResult
    load_data_window(
        compat::u32 file_number,
        compat::u32 data_offset,
        std::span<compat::u8, resource_io::kLegacyTalkWindowSize> destination,
        bool clear_before_read
    ) = 0;
    [[nodiscard]] virtual compat::u32
    update_action(asset_runtime::LegacyActionRecord& action) = 0;
    virtual void release_role_path_payload(compat::u32 role_index) noexcept = 0;
    virtual void begin_world_session_reload() noexcept = 0;
    [[nodiscard]] virtual bool reload_world_session(
        const LegacyWorldLoadRequest& request,
        std::span<LegacyWorldRoleRecord>& roles,
        compat::u32& controlled_role_index,
        LegacyWorldStoryVmRuntime& runtime
    ) = 0;
    virtual void
    patch_role_source(const LegacyMapsRolePatchRequest& request) noexcept = 0;
    virtual void play_sound_effect(compat::u16 sound_id) noexcept = 0;
    virtual void clear_story_framebuffer() noexcept = 0;
    virtual void present_story_framebuffer() noexcept = 0;
    [[nodiscard]] virtual bool prepare_story_video() noexcept = 0;
    virtual void begin_story_video(std::span<const compat::u8> filename) = 0;
    [[nodiscard]] virtual compat::i32 query_story_video_progress() = 0;
    virtual void
    set_story_frame_interval(compat::u32 milliseconds) noexcept = 0;
    [[nodiscard]] virtual bool prepare_story_ani() noexcept = 0;
    [[nodiscard]] virtual asset_runtime::LegacyAniActivityStartResult
    begin_story_ani(std::span<const compat::u8> filename, compat::u8 flags) = 0;
    [[nodiscard]] virtual bool is_story_ani_active() const noexcept = 0;
    [[nodiscard]] virtual compat::i32
    query_story_ani_phase() const noexcept = 0;
    virtual void beep() noexcept = 0;
    virtual void service_audio() = 0;
    // Models sub_40B7F0's optional %T/mon.dat expansion. Returning false
    // preserves the original resolver-failure path and leaves source intact.
    [[nodiscard]] virtual bool prepare_dialog_text(
        std::span<const compat::u8> source, std::vector<compat::u8>& destination
    ) = 0;
};

enum class LegacyWorldStoryVmStatus : compat::u8 {
    idle,
    yielded,
    terminated,
    load_failed,
    instruction_out_of_range,
    operand_out_of_range,
    unsupported_opcode,
    maps_payload_out_of_range,
    name_terminator_not_found,
    ani_filename_terminator_not_found,
    global_bit_index_out_of_range,
    role_not_found,
    runtime_unavailable,
    role_surface_failed,
    role_spatial_relocation_failed,
    role_path_completion_unavailable,
    role_path_failed,
    world_session_load_failed,
    script_variable_index_out_of_range,
    dialog_allocation_failed,
    picture_action_allocation_failed,
    moving_action_allocation_failed,
    role_head_action_allocation_failed,
    packed_row_effect_allocation_failed,
    unsupported_packed_row_effect_operation,
    role_allocation_failed,
    role_transfer_failed,
    role_map_update_failed,
    camera_step_divide_by_zero,
    random_target_divide_by_zero,
};

struct LegacyWorldStoryVmResult {
    LegacyWorldStoryVmStatus status{LegacyWorldStoryVmStatus::idle};
    resource_io::LegacyTalkWindowStatus load_status{
        resource_io::LegacyTalkWindowStatus::ready
    };
    LegacyWorldRoleTransferStatus role_transfer_status{
        LegacyWorldRoleTransferStatus::ready
    };
    LegacyWorldRoleMapUpdateResult role_map_update;
    compat::u16 instruction_offset{};
    compat::u16 raw_word{};
    compat::u16 opcode{};
    compat::u16 first_operand_word{};
    bool first_operand_available{};
    compat::u32 executed_instruction_count{};
    compat::u32 action_update_count{};
    compat::u32 action_update_failure_count{};
    compat::u32 dialog_enqueue_count{};
    compat::u32 role_one_shot_clear_count{};
    compat::u32 active_object_reset_count{};
    compat::u32 role_source_patch_failure_count{};
    compat::u32 role_materialization_count{};
    compat::u32 role_particle_emitter_write_count{};
    compat::u32 selection_overflow_diagnostic_count{};
    compat::u32 invalid_opcode_diagnostic_count{};
    compat::u32 invalid_opcode_current{};
    compat::u32 invalid_opcode_previous{};
    compat::u32 beep_count{};
    compat::u32 direct_audio_service_count{};
    compat::u32 dialog_text_prepare_count{};
    compat::u32 dialog_text_prepare_success_count{};
};

// sub_427920, currently restricted to the independently audited default-invalid
// and shared-dialog groups plus the earlier map-81/TALK100 implementation coverage:
// 1-40,42-43,45,51-53,58-72,74,76-87,
// 88-97,104,107,114,120,141,153,161,169,193,0x402 and 0x3FFF. Each
// handler preserves its individual advance/continue/yield contract;
// unsupported opcodes deliberately do not advance the IP.
[[nodiscard]] LegacyWorldStoryVmResult step_legacy_world_story_vm(
    LegacyWorldTalkContext& context,
    LegacyWorldStoryVmState& state,
    std::span<LegacyWorldRoleRecord> roles,
    compat::u32 controlled_role_index,
    std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    std::span<const compat::u8> maps_payload,
    story_scene::LegacyDialogRuntimeState& dialogs,
    LegacyWorldDialogRuntimeState& dialog_resources,
    std::span<const compat::u8, 16U> first_name,
    std::span<const compat::u8, 16U> second_name,
    LegacyWorldStoryVmRuntime runtime,
    LegacyWorldStoryVmPorts& ports
) noexcept;

}  // namespace openswd3::world_map
