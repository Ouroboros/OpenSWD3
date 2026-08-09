#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::asset_runtime {

class LegacyActionDrawPorts {
public:
  virtual ~LegacyActionDrawPorts() = default;

  [[nodiscard]] virtual LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) = 0;
  [[nodiscard]] virtual bool
  load_frame_piece(compat::u16 resource_id, compat::u16 frame_index,
                   rendering::LegacyFramePiece &piece) = 0;
  [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus
  draw_frame_piece(const rendering::LegacyFramePiece &piece,
                   compat::i32 destination_x, compat::i32 destination_y,
                   compat::u32 flags, compat::i32 opacity_step) noexcept = 0;
};

class LegacyActionDrawRuntimePorts final : public LegacyActionDrawPorts {
public:
  LegacyActionDrawRuntimePorts(
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
                   compat::u32 flags,
                   compat::i32 opacity_step) noexcept override;

private:
  LegacyActionUpdater &action_updater_;
  LegacyTswRuntime &tsw_runtime_;
  rendering::LegacyFramebuffer &framebuffer_;
  rendering::LegacyRasterGeometryState &raster_;
  const rendering::LegacyBlitEffectState &effects_;
  rendering::LegacyRleRowJitterState &jitter_;
};

enum class LegacyActionDrawStatus {
  ready,
  action_update_failed,
  frame_load_failed,
};

struct LegacyActionDrawResult {
  LegacyActionDrawStatus status{LegacyActionDrawStatus::ready};
  compat::u32 action_update_count{};
  compat::u32 frame_request_count{};
  compat::u32 draw_count{};
  compat::u32 blit_failure_count{};
  rendering::LegacyBlitExecutionStatus last_blit_status{
      rendering::LegacyBlitExecutionStatus::completed};
};

// sub_40EBF0: update the action, resolve its current TSW frame, subtract the
// action offsets, and draw with the action flags and byte opacity.
[[nodiscard]] LegacyActionDrawResult
update_draw_legacy_action(LegacyActionRecord &record, compat::i32 x,
                          compat::i32 y, LegacyActionDrawPorts &ports);

// sub_40EC80: resolve and draw a TSW frame directly with zero flags/opacity.
[[nodiscard]] LegacyActionDrawResult
draw_legacy_tsw_frame(compat::u32 resource_id_slot,
                      compat::u32 frame_index_slot, compat::i32 x,
                      compat::i32 y, LegacyActionDrawPorts &ports);

// sub_40ECC0: the special-mode bridge keeps only bits 31, 1 and 0 from the
// action flags, then ORs the caller flags after updating the action.
[[nodiscard]] LegacyActionDrawResult
update_draw_legacy_action_with_flags(LegacyActionRecord &record, compat::i32 x,
                                     compat::i32 y, compat::u32 caller_flags,
                                     LegacyActionDrawPorts &ports);

} // namespace openswd3::asset_runtime
