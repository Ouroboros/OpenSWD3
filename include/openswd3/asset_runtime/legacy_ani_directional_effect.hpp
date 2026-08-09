#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <cstddef>

namespace openswd3::input_time_rng {
class LegacySecondaryRng;
}

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyAniDirectionalServiceId = 5U;
inline constexpr compat::u32 kLegacyAniDirectionalActionId = 0x232BU;
inline constexpr std::size_t kLegacyAniDirectionalPhysicalSlotCount = 4U;
inline constexpr std::size_t kLegacyAniDirectionalUpdatedSlotCount = 2U;
inline constexpr compat::u32 kLegacyAniDirectionalResetWord = 0x0F0F0F0FU;

// The original keeps three parallel arrays, each addressed with a 0x10-byte
// stride. Keeping them separate preserves those physical groups instead of
// inventing one contiguous aggregate that never existed in the executable.
struct LegacyAniDirectionalMotionSlot {
  compat::i32 world_x{};
  compat::i32 world_y{};
  compat::i32 velocity_x{};
  compat::i32 velocity_y{};
};

static_assert(sizeof(LegacyAniDirectionalMotionSlot) == 0x10U);

struct LegacyAniDirectionalColorSlot {
  compat::i32 current_offset{};
  compat::i32 reserved_04{};
  compat::i32 target_offset{};
  compat::i32 reserved_0c{};
};

static_assert(sizeof(LegacyAniDirectionalColorSlot) == 0x10U);

struct LegacyAniDirectionalTimingSlot {
  compat::i32 frame_counter{};
  compat::i32 variant{};
  compat::i32 target_interval{};
  compat::i32 current_interval{};
};

static_assert(sizeof(LegacyAniDirectionalTimingSlot) == 0x10U);

struct LegacyAniDirectionalState {
  std::array<LegacyAniDirectionalMotionSlot,
             kLegacyAniDirectionalPhysicalSlotCount>
      motion{};
  std::array<LegacyAniDirectionalColorSlot,
             kLegacyAniDirectionalPhysicalSlotCount>
      color{};
  std::array<LegacyAniDirectionalTimingSlot,
             kLegacyAniDirectionalPhysicalSlotCount>
      timing{};
};

struct LegacyAniDirectionalConfiguration {
  compat::i32 map_width_tiles{};
  compat::i32 map_height_tiles{};
  compat::u16 base_variant{};
  compat::u16 variant_count{};
  compat::u32 spawn_direction{};
};

struct LegacyAniDirectionalFrameInput {
  compat::i32 movement_scale{};
  compat::i32 player_delta_x{};
  compat::i32 player_delta_y{};
  compat::i32 camera_x{};
  compat::i32 camera_y{};
};

enum class LegacyAniDirectionalInitializationStatus {
  ready,
  invalid_random_bound,
};

struct LegacyAniDirectionalInitializationResult {
  LegacyAniDirectionalInitializationStatus status{
      LegacyAniDirectionalInitializationStatus::ready};
  compat::u32 random_call_count{};
  compat::u32 initialized_slot_count{};
  compat::u32 failed_slot{};
};

class LegacyAniDirectionalServicePort {
public:
  virtual ~LegacyAniDirectionalServicePort() = default;

  [[nodiscard]] virtual bool service_enabled(compat::u32 service_id) = 0;
};

class LegacyAniDirectionalPorts {
public:
  virtual ~LegacyAniDirectionalPorts() = default;

  [[nodiscard]] virtual LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) = 0;
  [[nodiscard]] virtual bool
  load_frame_piece(compat::u16 resource_id, compat::u16 frame_index,
                   rendering::LegacyFramePiece &piece) = 0;
  [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus
  draw_frame_piece(const rendering::LegacyFramePiece &piece,
                   compat::i32 destination_x, compat::i32 destination_y,
                   compat::u32 flags, compat::i32 opacity_step,
                   compat::i32 color_offset) noexcept = 0;
};

class LegacyAniDirectionalRuntimePorts final
    : public LegacyAniDirectionalPorts {
public:
  LegacyAniDirectionalRuntimePorts(
      LegacyActionUpdater &action_updater, LegacyTswRuntime &tsw_runtime,
      rendering::LegacyFramebuffer &framebuffer,
      rendering::LegacyRasterGeometryState &raster,
      rendering::LegacyBlitEffectState &effects,
      rendering::LegacyRleRowJitterState &jitter) noexcept;

  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override;
  [[nodiscard]] bool
  load_frame_piece(compat::u16 resource_id, compat::u16 frame_index,
                   rendering::LegacyFramePiece &piece) override;
  [[nodiscard]] rendering::LegacyBlitExecutionStatus
  draw_frame_piece(const rendering::LegacyFramePiece &piece,
                   compat::i32 destination_x, compat::i32 destination_y,
                   compat::u32 flags, compat::i32 opacity_step,
                   compat::i32 color_offset) noexcept override;

private:
  LegacyActionUpdater &action_updater_;
  LegacyTswRuntime &tsw_runtime_;
  rendering::LegacyFramebuffer &framebuffer_;
  rendering::LegacyRasterGeometryState &raster_;
  rendering::LegacyBlitEffectState &effects_;
  rendering::LegacyRleRowJitterState &jitter_;
};

enum class LegacyAniDirectionalStatus {
  ready,
  disabled,
  invalid_random_bound,
  action_update_failed,
  frame_load_failed,
};

struct LegacyAniDirectionalResult {
  LegacyAniDirectionalStatus status{LegacyAniDirectionalStatus::ready};
  compat::u32 service_query_count{};
  compat::u32 random_call_count{};
  compat::u32 moved_slot_count{};
  compat::u32 respawned_slot_count{};
  compat::u32 skipped_outside_slot_count{};
  compat::u32 invalid_direction_count{};
  compat::u32 action_update_count{};
  compat::u32 frame_request_count{};
  compat::u32 draw_count{};
  compat::u32 blit_failure_count{};
  compat::u32 failed_slot{};
  rendering::LegacyBlitExecutionStatus last_blit_status{
      rendering::LegacyBlitExecutionStatus::completed};
};

class LegacyAniDirectionalEffect final {
public:
  LegacyAniDirectionalEffect() noexcept;

  // 0x0040C1B9/0x0040E606 fill the full four-slot motion block with
  // 0x0F0F0F0F but leave the two parallel state groups untouched.
  void reset_motion_block() noexcept;

  // This is the service-5-enabled producer at 0x0040C7D3. It initializes all
  // four physical slots; the per-frame function at 0x00415B70 updates only
  // the first two.
  [[nodiscard]] LegacyAniDirectionalInitializationResult
  initialize_slots(const LegacyAniDirectionalConfiguration &configuration,
                   input_time_rng::LegacySecondaryRng &random) noexcept;

  [[nodiscard]] LegacyAniDirectionalResult
  update(const LegacyAniDirectionalConfiguration &configuration,
         const LegacyAniDirectionalFrameInput &frame,
         input_time_rng::LegacySecondaryRng &random,
         LegacyAniDirectionalServicePort &services,
         LegacyActionRecord &shared_action_record,
         LegacyAniDirectionalPorts &ports);

  [[nodiscard]] LegacyAniDirectionalState &state() noexcept;
  [[nodiscard]] const LegacyAniDirectionalState &state() const noexcept;

private:
  LegacyAniDirectionalState state_{};
};

} // namespace openswd3::asset_runtime
