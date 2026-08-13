#pragma once

#include "openswd3/resource_io/legacy_resource_databases.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"
#include "openswd3/story_scene/legacy_dialog_runtime.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_picture_actions.hpp"
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

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldStoryFlagBytes = 0x400U;

struct LegacyWorldStoryVmState {
  std::array<compat::u8, resource_io::kLegacyTalkWindowSize> window{};
  std::array<compat::u8, kLegacyWorldStoryFlagBytes> flags{};
  std::array<compat::u8, 32U> speaker_name{};
  compat::u32 text_control_flags{0xFFFFFFFFU};
  compat::u32 next_text_aux_value{60U};
  compat::u32 music_request{0xFFFFFFFFU};
  compat::u32 music_first_stream{};
  compat::u32 music_second_stream{};
  compat::u32 music_control_flags{};
  compat::u32 current_first_stream{};
  compat::u32 current_second_stream{};
  compat::u32 wait_duration{};
  compat::u32 wait_started_at{};
  compat::u32 loaded_file_number{};
  compat::u32 loaded_data_offset{};
  bool next_text_aux_pending{};
  bool window_loaded{};
};

struct LegacyWorldStoryVmRuntime {
  LegacyRoleSpatialIndex *spatial_index{};
  LegacyWorldRoleSurfaceContext role_surface{};
  LegacyWorldCameraRect *camera{};
  LegacyWorldCameraPanState *camera_pan{};
  LegacyWorldMovementRuntimeState *movement{};
  LegacyPictureActionLists *picture_actions{};
  rendering::LegacyFrameColorTransitionState *frame_color{};
  LegacyWorldStoryPathRuntime *story_paths{};
  compat::u8 *scene_render_flags{};
  compat::u32 map_height{};
  compat::u32 current_tick{};
};

void initialize_legacy_world_story_vm(
    LegacyWorldStoryVmState &state) noexcept;

[[nodiscard]] bool query_legacy_world_story_flag(
    const LegacyWorldStoryVmState &state, compat::u16 bit_index) noexcept;
void set_legacy_world_story_flag(LegacyWorldStoryVmState &state,
                                 compat::u16 bit_index) noexcept;
void clear_legacy_world_story_flag(LegacyWorldStoryVmState &state,
                                   compat::u16 bit_index) noexcept;

class LegacyWorldStoryVmPorts {
public:
  virtual ~LegacyWorldStoryVmPorts() = default;

  [[nodiscard]] virtual resource_io::LegacyTalkWindowLoadResult
  load_story_window(
      compat::i32 story_id,
      std::span<compat::u8, resource_io::kLegacyTalkWindowSize> destination,
      bool clear_before_read) = 0;
  [[nodiscard]] virtual resource_io::LegacyTalkWindowLoadResult
  load_data_window(
      compat::u32 file_number, compat::u32 data_offset,
      std::span<compat::u8, resource_io::kLegacyTalkWindowSize> destination,
      bool clear_before_read) = 0;
  [[nodiscard]] virtual compat::u32 update_action(
      asset_runtime::LegacyActionRecord &action) = 0;
  virtual void patch_role_source(
      const LegacyMapsRolePatchRequest &request) noexcept = 0;
  virtual void clear_story_framebuffer() noexcept = 0;
  virtual void present_story_framebuffer() noexcept = 0;
  virtual void begin_story_video(std::span<const compat::u8> filename) = 0;
  [[nodiscard]] virtual compat::i32 query_story_video_progress() = 0;
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
      resource_io::LegacyTalkWindowStatus::ready};
  compat::u16 raw_word{};
  compat::u16 opcode{};
  compat::u32 executed_instruction_count{};
  compat::u32 action_update_count{};
  compat::u32 action_update_failure_count{};
  compat::u32 dialog_enqueue_count{};
  compat::u32 role_one_shot_clear_count{};
  compat::u32 active_object_reset_count{};
};

// sub_427920, restricted to the assembly-audited opcode closure reachable
// from the map-81 new-game entry through the current TALK100 opcode-45
// boundary:
// 6,7,8,9,10,11,14,20,21,22,25,26,38,39,40,42,43,51,52,60,61,67,
// 70,71,72,76,77,78,85,89,91,94,95,114,120,141,153,161,193,0x402 and
// 0x3FFF. Each
// handler preserves its individual advance/continue/yield contract;
// unsupported opcodes deliberately do not advance the IP.
[[nodiscard]] LegacyWorldStoryVmResult step_legacy_world_story_vm(
    LegacyWorldTalkContext &context, LegacyWorldStoryVmState &state,
    std::span<LegacyWorldRoleRecord> roles, compat::u32 controlled_role_index,
    std::span<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount>
        active_object_slots,
    std::span<const compat::u8> maps_payload,
    story_scene::LegacyDialogRuntimeState &dialogs,
    LegacyWorldDialogRuntimeState &dialog_resources,
    std::span<const compat::u8, 16U> first_name,
    std::span<const compat::u8, 16U> second_name,
    LegacyWorldStoryVmRuntime runtime,
    LegacyWorldStoryVmPorts &ports) noexcept;

} // namespace openswd3::world_map
