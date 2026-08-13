#include "openswd3/world_map/legacy_world_dialog_runtime.hpp"

#include "openswd3/rendering/legacy_packed_row.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <limits>
#include <vector>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr std::array<u32, 16U> kLegacyPrimaryBgr888{
    0x00FFFFFFU, 0x00000000U, 0x000C31ECU, 0x000080FFU,
    0x002C577BU, 0x00FFE6E6U, 0x00ACCFE9U, 0x00002CECU,
    0x00FF0000U, 0x00800000U, 0x00606060U, 0x002C577BU,
    0x00E9C8C0U, 0x00ACCFE9U, 0x000D31ECU, 0x00002CECU,
};

constexpr std::array<u32, 16U> kLegacySecondaryBgr888{
    0x00606060U, 0x00808080U, 0x007DA9AEU, 0x00003080U,
    0x007DA9AEU, 0x002C4F24U, 0x00334D64U, 0x002C4F24U,
    0x00800080U, 0x00FF0000U, 0x00606060U, 0x007DA9AEU,
    0x002C4F24U, 0x00334D64U, 0x007DA9AEU, 0x002C4F24U,
};

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
  return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i32 wrapping_subtract(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(std::bit_cast<u32>(left) - std::bit_cast<u32>(right));
}

[[nodiscard]] constexpr i32 wrapping_add(const i32 left,
                                         const i32 right) noexcept {
  return from_bits(std::bit_cast<u32>(left) + std::bit_cast<u32>(right));
}

[[nodiscard]] constexpr i32 wrapping_multiply(const i32 left,
                                              const i32 right) noexcept {
  return from_bits(std::bit_cast<u32>(left) * std::bit_cast<u32>(right));
}

[[nodiscard]] u16 pack_bgr888(
    const u32 color,
    const rendering::LegacyPixelConversionState &format) noexcept {
  return static_cast<u16>(rendering::legacy_pack_color_pair(
      format, static_cast<i32>((color >> 3U) & 0x1FU),
      static_cast<i32>((color >> 11U) & 0x1FU),
      static_cast<i32>((color >> 19U) & 0x1FU)));
}

[[nodiscard]] bool color_at(
    const std::array<u32, 16U> &source, const u16 index,
    const rendering::LegacyPixelConversionState &format,
    u16 &destination) noexcept {
  if (index >= source.size()) {
    return false;
  }
  destination = pack_bgr888(source[index], format);
  return true;
}

[[nodiscard]] constexpr rendering::LegacyBlitClipRectangle current_clip(
    const rendering::LegacyRasterGeometryState &raster) noexcept {
  return rendering::LegacyBlitClipRectangle{
      .left = raster.clip_left,
      .top = raster.clip_top,
      .width = raster.clip_width,
      .height = raster.clip_height,
  };
}

[[nodiscard]] constexpr bool accepted_text_status(
    const rendering::LegacyTextDrawStatus status) noexcept {
  return status == rendering::LegacyTextDrawStatus::completed;
}

class ActionFramePieceProvider final
    : public rendering::LegacyFramePieceProvider {
public:
  explicit ActionFramePieceProvider(
      asset_runtime::LegacyActionDrawPorts &ports) noexcept
      : ports_(ports) {}

  [[nodiscard]] bool load_frame_piece(
      const u32 resource_id, const u32 piece_index,
      rendering::LegacyFramePiece &piece) noexcept override {
    if (resource_id > std::numeric_limits<u16>::max() ||
        piece_index > std::numeric_limits<u16>::max()) {
      return false;
    }
    try {
      return ports_.load_frame_piece(static_cast<u16>(resource_id),
                                     static_cast<u16>(piece_index), piece);
    } catch (...) {
      return false;
    }
  }

private:
  asset_runtime::LegacyActionDrawPorts &ports_;
};

[[nodiscard]] bool update_action(
    asset_runtime::LegacyActionRecord &action,
    asset_runtime::LegacyActionDrawPorts &ports) noexcept {
  try {
    return ports.update_action_record(action) ==
           asset_runtime::LegacyActionUpdateStatus::completed;
  } catch (...) {
    return false;
  }
}

[[nodiscard]] std::size_t caption_length(
    const std::span<const u8> text) noexcept {
  const auto terminator = std::ranges::find(text, u8{});
  return static_cast<std::size_t>(terminator - text.begin());
}

} // namespace

LegacyWorldDialogRuntimeState::LegacyWorldDialogRuntimeState()
    : text_surface(rendering::LegacySurfaceGeometry{
          .pitch_bytes = story_scene::kLegacyDialogSurfaceWidth *
                         static_cast<i32>(sizeof(u16)),
          .width = story_scene::kLegacyDialogSurfaceWidth,
          .height = story_scene::kLegacyDialogSurfaceHeight,
      }) {}

LegacyWorldDialogPrimeResult prime_legacy_world_dialog_runtime(
    LegacyWorldDialogRuntimeState &state,
    asset_runtime::LegacyActionDrawPorts &action_ports) noexcept {
  state.end_dialog_action = {};
  state.next_page_action = {};
  asset_runtime::initialize_legacy_action_record(state.end_dialog_action);
  asset_runtime::initialize_legacy_action_record(state.next_page_action);
  state.end_dialog_action.action_id = 0x2329U;
  state.end_dialog_action.base_variant = 0x0CU;
  state.next_page_action.action_id = 0x2329U;
  state.next_page_action.base_variant = 0x0EU;

  constexpr std::array<u32, 4U> kFrameActionIds{
      0x232DU, 0x232FU, 0x2330U, 0x2331U};
  constexpr std::array<u32, 4U> kCaptionActionIds{
      0x2337U, 0x2339U, 0x233AU, 0x233BU};
  for (std::size_t index = 0U; index < state.frame_actions.size(); ++index) {
    auto &frame = state.frame_actions[index];
    auto &caption = state.caption_actions[index];
    frame = {};
    caption = {};
    asset_runtime::initialize_legacy_action_record(frame);
    asset_runtime::initialize_legacy_action_record(caption);
    frame.action_id = kFrameActionIds[index];
    caption.action_id = kCaptionActionIds[index];
  }

  LegacyWorldDialogPrimeResult result;
  ++result.action_update_count;
  if (!update_action(state.end_dialog_action, action_ports)) {
    ++result.action_update_failure_count;
  }
  ++result.action_update_count;
  if (!update_action(state.next_page_action, action_ports)) {
    ++result.action_update_failure_count;
  }
  for (std::size_t index = 0U; index < state.frame_actions.size(); ++index) {
    ++result.action_update_count;
    if (!update_action(state.frame_actions[index], action_ports)) {
      ++result.action_update_failure_count;
    }
    ++result.action_update_count;
    if (!update_action(state.caption_actions[index], action_ports)) {
      ++result.action_update_failure_count;
    }
  }
  return result;
}

LegacyWorldDialogRuntimePorts::LegacyWorldDialogRuntimePorts(
    LegacyWorldDialogRuntimeState &state,
    rendering::LegacyFramebuffer &framebuffer,
    rendering::LegacyRasterGeometryState &raster,
    const rendering::LegacyPixelConversionState &pixel_conversion,
    const rendering::LegacyBlitEffectState &blit_effects,
    rendering::LegacyRleRowJitterState &jitter,
    const std::span<LegacyWorldRoleRecord> roles,
    asset_runtime::LegacyActionDrawPorts &action_ports,
    rendering::LegacyTextRendererBinding text_20,
    rendering::LegacyTextRendererBinding text_16,
    LegacyWorldDialogExternalPorts *external_ports,
    LegacyWorldTalkContext *talk_context) noexcept
    : state_(state), framebuffer_(framebuffer), raster_(raster),
      pixel_conversion_(pixel_conversion), blit_effects_(blit_effects),
      jitter_(jitter), roles_(roles), action_ports_(action_ports),
      text_20_(text_20), text_16_(text_16), external_ports_(external_ports),
      talk_context_(talk_context) {}

bool LegacyWorldDialogRuntimePorts::begin_text_surface(
    const i32 width, const i32 height) noexcept {
  surface_active_ = width == story_scene::kLegacyDialogSurfaceWidth &&
                    height == story_scene::kLegacyDialogSurfaceHeight;
  return surface_active_;
}

void LegacyWorldDialogRuntimePorts::clear_text_surface() noexcept {
  if (surface_active_) {
    u16 transparent_pixel = 0x026BU;
    rendering::legacy_convert_pixels_forward(
        pixel_conversion_, &transparent_pixel, 1U);
    std::ranges::fill(state_.text_surface.physical_pixels(),
                      transparent_pixel);
  }
}

void LegacyWorldDialogRuntimePorts::end_text_surface() noexcept {
  surface_active_ = false;
}

bool LegacyWorldDialogRuntimePorts::resolve_role_anchor(
    const u16 role_index, i32 &world_x, i32 &world_y) noexcept {
  if (role_index >= roles_.size()) {
    return false;
  }
  world_x = from_bits(roles_[role_index].world_x);
  world_y = from_bits(roles_[role_index].world_y);
  return true;
}

void LegacyWorldDialogRuntimePorts::set_dialog_clip(
    const story_scene::LegacyDialogRectangle &rectangle) noexcept {
  rendering::set_legacy_clip_rectangle(raster_, rectangle.left, rectangle.top,
                                       rectangle.right, rectangle.bottom);
}

void LegacyWorldDialogRuntimePorts::draw_dialog_panel(
    const story_scene::LegacyDialogPanelDrawRequest &request) noexcept {
  if (request.action == nullptr) {
    return;
  }
  try {
    ActionFramePieceProvider provider{action_ports_};
    static_cast<void>(rendering::draw_legacy_tiled_frame(
        framebuffer_, raster_, provider,
        rendering::LegacyTiledFrameRequest{
            .resource_id = request.action->field_4a,
            .left = request.rectangle.left,
            .top = request.rectangle.top,
            .right = request.rectangle.right,
            .bottom = request.rectangle.bottom,
            .opacity_step = request.opacity_step,
            .flags = 0x10U,
        },
        blit_effects_, jitter_));
  } catch (...) {
  }
}

void LegacyWorldDialogRuntimePorts::composite_text_surface(
    const story_scene::LegacyDialogCompositeRequest &request) noexcept {
  if (!surface_active_) {
    return;
  }
  auto pixels = state_.text_surface.physical_pixels();
  if (!pixels.empty() && pixels.front() == 0xFFFFU) {
    pixels.front() = 0xFFFEU;
  }
  const auto *bytes = reinterpret_cast<const u8 *>(pixels.data());
  const rendering::LegacyBlitSource source{
      .bytes = {bytes, pixels.size_bytes()},
      .layout = rendering::LegacyBlitSourceLayout::direct_16,
  };
  static_cast<void>(rendering::blit_legacy_copy_paths(
      framebuffer_, current_clip(raster_), source,
      rendering::LegacyBlitRequest{
          .destination_x = request.destination_x,
          .destination_y = request.destination_y,
          .source_width = request.source_width,
          .source_height = request.source_height,
          .target_height = request.source_height,
          .flags = request.flags,
          .opacity_step = request.opacity_step,
      },
      blit_effects_, jitter_));
}

void LegacyWorldDialogRuntimePorts::draw_dialog_indicator(
    const story_scene::LegacyDialogIndicatorRequest &request) noexcept {
  auto &action = request.kind ==
                         story_scene::LegacyDialogIndicatorKind::end_dialog
                     ? state_.end_dialog_action
                     : state_.next_page_action;
  rendering::LegacyFramePiece piece;
  try {
    if (!action_ports_.load_frame_piece(action.field_4a, action.field_4c,
                                        piece)) {
      return;
    }
    const i32 x = wrapping_subtract(
        wrapping_subtract(request.panel.right,
                          from_bits(action.draw_offset_x)),
        4);
    const i32 y = wrapping_subtract(
        wrapping_subtract(request.panel.bottom,
                          from_bits(action.draw_offset_y)),
        8);
    static_cast<void>(action_ports_.draw_frame_piece(piece, x, y, 0U, 0));
  } catch (...) {
  }
}

void LegacyWorldDialogRuntimePorts::draw_dialog_caption(
    const story_scene::LegacyDialogCaptionRequest &request) noexcept {
  const std::size_t length = caption_length(request.text);
  if (length > static_cast<std::size_t>(std::numeric_limits<i32>::max())) {
    return;
  }
  const i32 byte_length = static_cast<i32>(length);
  const i32 background_x = wrapping_add(request.panel.left, 16);
  const i32 background_width = wrapping_add(wrapping_multiply(byte_length, 8),
                                            0x1C);
  const auto &surface = framebuffer_.geometry().surface;
  for (i32 row = 0; row < 15; ++row) {
    const i32 y = wrapping_add(wrapping_add(request.panel.top, row), -16);
    if (background_x < 0 || y < 0 || y >= surface.height ||
        background_width < 2 || background_x > surface.width ||
        background_width > surface.width - background_x) {
      continue;
    }
    auto destination = framebuffer_.row_pixels(static_cast<u32>(y)).subspan(
        static_cast<std::size_t>(background_x),
        static_cast<std::size_t>(background_width));
    static_cast<void>(rendering::blend_legacy_packed_row(
        destination, 0U, background_width, pixel_conversion_));
  }

  if (request.action != nullptr) {
    try {
      ActionFramePieceProvider provider{action_ports_};
      static_cast<void>(rendering::draw_legacy_tiled_frame(
          framebuffer_, raster_, provider,
          rendering::LegacyTiledFrameRequest{
              .resource_id = request.action->field_4a,
              .left = wrapping_add(request.panel.left, 6),
              .top = wrapping_add(request.panel.top, -27),
              .right = wrapping_add(
                  wrapping_add(request.panel.left,
                               wrapping_multiply(byte_length, 9)),
                  16),
              .bottom = wrapping_add(request.panel.top, -8),
              .opacity_step = request.opacity_step,
              .flags = 4U,
          },
          blit_effects_, jitter_));
    } catch (...) {
    }
  }

  if (!text_16_.ready()) {
    return;
  }
  std::vector<u8> caption;
  try {
    const auto visible = request.text.first(length);
    caption.assign(visible.begin(), visible.end());
    caption.push_back(0U);
  } catch (...) {
    return;
  }
  u16 foreground{};
  u16 secondary{};
  if (!color_at(kLegacyPrimaryBgr888, 4U, pixel_conversion_, foreground) ||
      !color_at(kLegacySecondaryBgr888, 4U, pixel_conversion_, secondary)) {
    return;
  }
  rendering::LegacyTextRendererState text_state = *text_16_.state;
  text_state.background_color = 0xFFFEU;
  text_state.secondary_color = secondary;
  text_state.clip = rendering::LegacyGlyphClipRectangle{
      .left = 0,
      .top = 0,
      .width = surface.width,
      .height = surface.height,
  };
  static_cast<void>(rendering::draw_legacy_text(
      framebuffer_, *text_16_.glyph_cache, *text_16_.glyph_provider,
      text_state,
      rendering::LegacyTextDrawRequest{
          .destination_x = wrapping_add(request.panel.left, 12),
          .destination_y = wrapping_add(request.panel.top, -26),
          .nul_terminated_text = caption,
          .foreground_color = foreground,
          .flags = 4U,
      }));
}

void LegacyWorldDialogRuntimePorts::release_message_owner(
    const u16 role_index) noexcept {
  if (role_index < roles_.size()) {
    roles_[role_index].interaction_gate = 0U;
  } else if (role_index == kLegacyWorldTalkMapEventSource &&
             talk_context_ != nullptr) {
    talk_context_->field_26 = 0U;
  }
}

bool LegacyWorldDialogRuntimePorts::update_end_dialog_action() noexcept {
  return update_action(state_.end_dialog_action, action_ports_);
}

bool LegacyWorldDialogRuntimePorts::update_next_page_action() noexcept {
  return update_action(state_.next_page_action, action_ports_);
}

void LegacyWorldDialogRuntimePorts::restore_text_destination(i32, i32) noexcept {
}

bool LegacyWorldDialogRuntimePorts::draw_segment(
    const story_scene::LegacyDialogSegmentDrawRequest &request) noexcept {
  if (!surface_active_ || !text_20_.ready()) {
    return false;
  }
  u16 foreground{};
  u16 secondary{};
  if (!color_at(kLegacyPrimaryBgr888, request.foreground_index,
                pixel_conversion_, foreground) ||
      !color_at(kLegacySecondaryBgr888, request.secondary_index,
                pixel_conversion_, secondary)) {
    return false;
  }
  rendering::LegacyTextRendererState text_state = *text_20_.state;
  text_state.background_color = 0xFFFEU;
  text_state.secondary_color = secondary;
  text_state.clip = rendering::LegacyGlyphClipRectangle{
      .left = 0,
      .top = 0,
      .width = story_scene::kLegacyDialogSurfaceWidth,
      .height = story_scene::kLegacyDialogSurfaceHeight,
  };
  try {
    return accepted_text_status(rendering::draw_legacy_text(
                                    state_.text_surface,
                                    *text_20_.glyph_cache,
                                    *text_20_.glyph_provider, text_state,
                                    rendering::LegacyTextDrawRequest{
                                        .destination_x = request.destination_x,
                                        .destination_y = request.destination_y,
                                        .nul_terminated_text =
                                            request.nul_terminated_text,
                                        .foreground_color = foreground,
                                        .flags = request.style,
                                    })
                                    .status);
  } catch (...) {
    return false;
  }
}

void LegacyWorldDialogRuntimePorts::draw_selected_choice_background(
    const story_scene::LegacyDialogChoiceBackgroundRequest &request) noexcept {
  if (!surface_active_ ||
      request.surface_width != story_scene::kLegacyDialogSurfaceWidth) {
    return;
  }
  static_cast<void>(rendering::draw_legacy_animated_border(
      state_.choice_border, pixel_conversion_,
      rendering::LegacyAnimatedBorderRequest{
          .destination = state_.text_surface.physical_pixels(),
          .x = request.destination_x,
          .y = request.destination_y,
          .width = request.width,
          .height = request.height,
          .pitch_pixels = story_scene::kLegacyDialogSurfaceWidth,
      }));
}

void LegacyWorldDialogRuntimePorts::play_choice_sound() noexcept {
  if (external_ports_ != nullptr) {
    external_ports_->play_dialog_choice_sound();
  }
}

bool LegacyWorldDialogRuntimePorts::close_role_dialog_action(
    const u16 role_index) noexcept {
  if (role_index >= roles_.size()) {
    return false;
  }
  auto &role = roles_[role_index];
  if ((role.flags & 0x00000800U) == 0U) {
    return true;
  }
  return update_action(role.action, action_ports_);
}

void LegacyWorldDialogRuntimePorts::close_detached_dialog() noexcept {
  state_.detached_dialog_active = false;
  if (talk_context_ != nullptr) {
    talk_context_->field_26 = 0U;
  }
}

} // namespace openswd3::world_map
