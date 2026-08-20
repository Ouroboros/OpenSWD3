#pragma once

#include "openswd3/resource_io/legacy_resource_databases.hpp"
#include "openswd3/rendering/legacy_action_renderers.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
#include "openswd3/world_map/legacy_role_head_actions.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_camera_pan.hpp"
#include "openswd3/world_map/legacy_world_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"
#include "openswd3/world_map/legacy_world_story_paths.hpp"
#include "openswd3/world_map/legacy_world_role_transfer.hpp"

#include <array>
#include <list>
#include <span>
#include <vector>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldStoryFlagBytes = 0x400U;
inline constexpr std::size_t kLegacyWorldScriptVariableCount = 64U;

inline constexpr compat::u16 OP_07_CLEAR_DIALOG_CONTROL_BIT31 = 7U;
inline constexpr compat::u16 OP_08_STAGE_DIALOG_LIFETIME = 8U;
inline constexpr compat::u16 OP_09_CLEAR_DIALOG_CONTROL_BIT30 = 9U;
inline constexpr compat::u16 OP_10_SET_ROLE_BASE_VARIANT = 10U;
inline constexpr compat::u16 OP_11_SET_ROLE_VARIANT_DELTA = 11U;
inline constexpr compat::u16 OP_12_SET_ROLE_POSITION = 12U;
inline constexpr compat::u16 OP_13_STEP_ROLE = 13U;
inline constexpr compat::u16 OP_14_WAIT_ROLE_ACTION_STATUS = 14U;
inline constexpr compat::u16 OP_15_JUMP_SAME_FILE_OFFSET = 15U;
inline constexpr compat::u16 OP_16_JUMP_IF_ROLE_PATH_UNPREPARED = 16U;
inline constexpr compat::u16 OP_17_JUMP_IF_ROLE_PATH_PREPARED = 17U;
inline constexpr compat::u16 OP_18 = 18U;
inline constexpr compat::u16 OP_19 = 19U;
inline constexpr compat::u16 OP_45 = 45U;

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
    compat::u32 loaded_file_number{};
    compat::u32 loaded_data_offset{};
    bool next_text_aux_pending{};
    bool window_loaded{};
};

struct LegacyWorldStoryVmRuntime {
    LegacyRoleSpatialIndex* spatial_index{};
    LegacyWorldRoleSurfaceContext role_surface{};
    LegacyWorldCameraRect* camera{};
    LegacyWorldCameraPanState* camera_pan{};
    LegacyWorldMovementRuntimeState* movement{};
    LegacyPictureActionLists* picture_actions{};
    std::list<rendering::LegacyPackedRowEffect>* packed_row_effects{};
    LegacyRoleHeadActionList* role_head_actions{};
    compat::u32* battle_request_value{};
    rendering::LegacyFrameColorTransitionState* frame_color{};
    LegacyWorldStoryPathRuntime* story_paths{};
    compat::u8* scene_render_flags{};
    compat::u32 map_height{};
    compat::u32 current_tick{};
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
    virtual void
    patch_role_source(const LegacyMapsRolePatchRequest& request) noexcept = 0;
    virtual void play_sound_effect(compat::u16 sound_id) noexcept = 0;
    virtual void clear_story_framebuffer() noexcept = 0;
    virtual void present_story_framebuffer() noexcept = 0;
    virtual void begin_story_video(std::span<const compat::u8> filename) = 0;
    [[nodiscard]] virtual compat::i32 query_story_video_progress() = 0;
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
    role_not_found,
    runtime_unavailable,
    role_surface_failed,
    role_spatial_relocation_failed,
    role_path_completion_unavailable,
    role_path_failed,
    dialog_allocation_failed,
    picture_action_allocation_failed,
};

struct LegacyWorldStoryVmResult {
    LegacyWorldStoryVmStatus status{LegacyWorldStoryVmStatus::idle};
    resource_io::LegacyTalkWindowStatus load_status{
        resource_io::LegacyTalkWindowStatus::ready
    };
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
// 1-18,20-22,25-26,38-40,42-43,45,51-53,58-61,67,70-72,74,76-78,
// 85,88-91,94-95,104,107,114,120,141,153,161,193,0x402 and 0x3FFF. Each
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
