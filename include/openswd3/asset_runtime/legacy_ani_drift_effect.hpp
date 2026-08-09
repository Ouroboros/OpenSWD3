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

inline constexpr compat::u32 kLegacyAniDriftServiceId = 6U;
inline constexpr compat::u32 kLegacyAniDriftActionId = 0x232BU;
inline constexpr std::size_t kLegacyAniDriftSlotCount = 4U;
inline constexpr compat::i32 kLegacyAniDriftInactiveX = 0x7FFFFFFF;

struct LegacyAniDriftSlot {
  compat::i32 x{kLegacyAniDriftInactiveX};
  compat::i32 y{};
  compat::i32 velocity_x{};
  compat::i32 velocity_y{};
};

static_assert(sizeof(LegacyAniDriftSlot) == 0x10U);

struct LegacyAniDriftState {
  std::array<LegacyAniDriftSlot, kLegacyAniDriftSlotCount> slots{};
};

class LegacyAniDriftServicePort {
public:
  virtual ~LegacyAniDriftServicePort() = default;

  [[nodiscard]] virtual bool service_enabled(compat::u32 service_id) = 0;
};

class LegacyAniDriftPorts {
public:
  virtual ~LegacyAniDriftPorts() = default;

  [[nodiscard]] virtual LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) = 0;
  [[nodiscard]] virtual bool
  load_frame_piece(compat::u16 resource_id, compat::u16 frame_index,
                   rendering::LegacyFramePiece &piece) = 0;
  [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus
  draw_frame_piece(const rendering::LegacyFramePiece &piece,
                   compat::i32 destination_x, compat::i32 destination_y,
                   compat::u32 flags) noexcept = 0;
};

class LegacyAniDriftRuntimePorts final : public LegacyAniDriftPorts {
public:
  LegacyAniDriftRuntimePorts(
      LegacyActionUpdater &action_updater, LegacyTswRuntime &tsw_runtime,
      rendering::LegacyFramebuffer &framebuffer,
      rendering::LegacyRasterGeometryState &raster,
      const rendering::LegacyBlitEffectState &effects,
      rendering::LegacyRleRowJitterState &jitter) noexcept;

  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override;
  [[nodiscard]] bool
  load_frame_piece(compat::u16 resource_id, compat::u16 frame_index,
                   rendering::LegacyFramePiece &piece) override;
  [[nodiscard]] rendering::LegacyBlitExecutionStatus
  draw_frame_piece(const rendering::LegacyFramePiece &piece,
                   compat::i32 destination_x, compat::i32 destination_y,
                   compat::u32 flags) noexcept override;

private:
  LegacyActionUpdater &action_updater_;
  LegacyTswRuntime &tsw_runtime_;
  rendering::LegacyFramebuffer &framebuffer_;
  rendering::LegacyRasterGeometryState &raster_;
  const rendering::LegacyBlitEffectState &effects_;
  rendering::LegacyRleRowJitterState &jitter_;
};

enum class LegacyAniDriftStatus {
  ready,
  disabled,
  action_update_failed,
  frame_load_failed,
  invalid_vertical_velocity,
};

struct LegacyAniDriftResult {
  LegacyAniDriftStatus status{LegacyAniDriftStatus::ready};
  compat::u32 service_query_count{};
  compat::u32 respawn_count{};
  compat::u32 perturbation_count{};
  compat::u32 action_update_count{};
  compat::u32 frame_request_count{};
  compat::u32 draw_count{};
  compat::u32 blit_failure_count{};
  compat::u32 failed_slot{};
  rendering::LegacyBlitExecutionStatus last_blit_status{
      rendering::LegacyBlitExecutionStatus::completed};
};

class LegacyAniDriftEffect final {
public:
  LegacyAniDriftEffect() noexcept;

  // The original scene reset writes only the four x sentinels. It preserves
  // y, both velocity fields, and all four action records.
  void reset_positions() noexcept;

  [[nodiscard]] LegacyAniDriftResult
  update(compat::i32 map_width_tiles, compat::i32 map_height_tiles,
         compat::i32 camera_x, compat::i32 camera_y,
         input_time_rng::LegacySecondaryRng &random,
         LegacyAniDriftServicePort &services, LegacyAniDriftPorts &ports);

  [[nodiscard]] LegacyAniDriftState &state() noexcept;
  [[nodiscard]] const LegacyAniDriftState &state() const noexcept;
  [[nodiscard]] std::array<LegacyActionRecord, kLegacyAniDriftSlotCount> &
  action_records() noexcept;
  [[nodiscard]] const std::array<LegacyActionRecord, kLegacyAniDriftSlotCount> &
  action_records() const noexcept;

private:
  LegacyAniDriftState state_{};
  std::array<LegacyActionRecord, kLegacyAniDriftSlotCount> action_records_{};
};

} // namespace openswd3::asset_runtime
