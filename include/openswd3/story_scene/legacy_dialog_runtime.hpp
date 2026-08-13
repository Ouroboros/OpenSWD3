#pragma once

#include "openswd3/story_scene/legacy_dialog_control.hpp"

#include <list>
#include <span>

namespace openswd3::story_scene {

inline constexpr compat::i32 kLegacyDialogScale = 0x0B;
inline constexpr compat::i32 kLegacyDialogSurfaceColumns = 40;
inline constexpr compat::i32 kLegacyDialogSurfaceRows = 11;
inline constexpr compat::i32 kLegacyDialogSurfaceWidth =
    kLegacyDialogSurfaceColumns * kLegacyDialogScale;
inline constexpr compat::i32 kLegacyDialogSurfaceHeight =
    kLegacyDialogSurfaceRows * kLegacyDialogScale;

struct LegacyDialogRuntimeState {
  std::list<LegacyDialogMessage> messages;
  LegacyDialogControlState control;
  LegacyDialogCloseState close;
};

// sub_40DBC0: release the shared choice-hotspot chain and clear its five-dword
// sentinel node. The modern owner has no raw sentinel; vector::clear supplies
// the RAII ownership adaptation and unrelated dialog state remains untouched.
void clear_legacy_dialog_choice_chain(
    LegacyDialogRuntimeState &state
) noexcept;

struct LegacyDialogRuntimeInput {
  compat::u32 current_tick{};
  compat::u32 primary_press_state{};
  compat::u32 global_confirm_latch_state{};
  compat::u16 base_character_delay{};
  compat::i32 selected_choice_index{-1};
  compat::i32 camera_left{};
  compat::i32 camera_top{};
  compat::i32 destination_width{640};
  compat::i32 destination_height{480};
  bool choice_chain_active{};
};

struct LegacyDialogPanelDrawRequest {
  const LegacyDialogRecord32 *record{};
  const asset_runtime::LegacyActionRecord *action{};
  LegacyDialogRectangle rectangle{};
  compat::i32 opacity_step{};
};

struct LegacyDialogCompositeRequest {
  compat::i32 destination_x{};
  compat::i32 destination_y{};
  compat::i32 source_width{kLegacyDialogSurfaceWidth};
  compat::i32 source_height{kLegacyDialogSurfaceHeight};
  compat::u32 flags{4U};
  compat::i32 opacity_step{};
};

enum class LegacyDialogIndicatorKind : compat::u8 {
  next_page,
  end_dialog,
};

struct LegacyDialogIndicatorRequest {
  LegacyDialogIndicatorKind kind{LegacyDialogIndicatorKind::next_page};
  LegacyDialogRectangle panel{};
  compat::u32 flags{};
};

struct LegacyDialogCaptionRequest {
  std::span<const compat::u8> text{};
  LegacyDialogRectangle panel{};
  compat::u32 action_pointer_32{};
  const asset_runtime::LegacyActionRecord *action{};
  compat::i32 opacity_step{};
};

class LegacyDialogRuntimePorts : public LegacyDialogTextPorts,
                                 public LegacyDialogClosePorts {
public:
  ~LegacyDialogRuntimePorts() override = default;

  [[nodiscard]] virtual bool begin_text_surface(
      compat::i32 width, compat::i32 height) noexcept = 0;
  virtual void clear_text_surface() noexcept = 0;
  virtual void end_text_surface() noexcept = 0;

  [[nodiscard]] virtual bool resolve_role_anchor(
      compat::u16 role_index, compat::i32 &world_x,
      compat::i32 &world_y) noexcept = 0;
  virtual void set_dialog_clip(
      const LegacyDialogRectangle &rectangle) noexcept = 0;
  virtual void draw_dialog_panel(
      const LegacyDialogPanelDrawRequest &request) noexcept = 0;
  virtual void composite_text_surface(
      const LegacyDialogCompositeRequest &request) noexcept = 0;
  virtual void draw_dialog_indicator(
      const LegacyDialogIndicatorRequest &request) noexcept = 0;
  virtual void draw_dialog_caption(
      const LegacyDialogCaptionRequest &request) noexcept = 0;

  virtual void release_message_owner(compat::u16 role_index) noexcept = 0;
  [[nodiscard]] virtual bool update_end_dialog_action() noexcept = 0;
  [[nodiscard]] virtual bool update_next_page_action() noexcept = 0;
  virtual void restore_text_destination(
      compat::i32 width, compat::i32 height) noexcept = 0;
};

enum class LegacyDialogRuntimeStatus : compat::u8 {
  idle,
  completed,
  surface_unavailable,
  role_anchor_unavailable,
  text_failed,
  control_apply_failed,
};

struct LegacyDialogRuntimeResult {
  LegacyDialogRuntimeStatus status{LegacyDialogRuntimeStatus::idle};
  LegacyDialogTextFrameStatus text_status{
      LegacyDialogTextFrameStatus::completed};
  LegacyDialogControlApplyStatus control_apply_status{
      LegacyDialogControlApplyStatus::completed};
  compat::u32 message_count{};
  compat::u32 panel_draw_count{};
  compat::u32 composite_count{};
  compat::u32 indicator_draw_count{};
  compat::u32 caption_draw_count{};
  compat::u32 removed_message_count{};
  compat::u32 text_draw_failure_count{};
  compat::u32 nonfatal_action_failure_count{};
};

// sub_42ED40 (0x0042ED40..0x0043017C): update and draw the shared dialog
// chain against one reusable 440x121 text surface. The caller supplies the
// current scene's anchors, framebuffer operations and action owners.
[[nodiscard]] LegacyDialogRuntimeResult update_draw_legacy_dialogs(
    LegacyDialogRuntimeState &state, const LegacyDialogRuntimeInput &input,
    LegacyDialogRuntimePorts &ports) noexcept;

} // namespace openswd3::story_scene
