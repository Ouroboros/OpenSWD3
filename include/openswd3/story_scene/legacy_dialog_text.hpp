#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/story_scene/legacy_dialog_geometry.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {
struct LegacyActionRecord;
}

namespace openswd3::story_scene {

enum class LegacyDialogTextTokenKind : compat::u8 {
  text,
  terminator,
  line_break,
  delayed_page_break,
  page_break,
  character_delay,
  both_colors,
  secondary_color,
  text_style,
  choice,
  sound,
  close,
};

enum class LegacyDialogTextTokenStatus : compat::u8 {
  completed,
  end_of_buffer,
  truncated_control,
  dangling_double_byte,
  missing_period,
};

struct LegacyDialogTextToken {
  LegacyDialogTextTokenStatus status{
      LegacyDialogTextTokenStatus::end_of_buffer};
  LegacyDialogTextTokenKind kind{LegacyDialogTextTokenKind::text};
  std::size_t source_index{};
  std::size_t next_index{};
  std::span<const compat::u8> bytes{};
  std::span<const compat::u8> payload{};
  compat::u8 parameter{};
  bool uppercase_variant{};
};

// Tokenizes the byte protocol consumed by 0x0042F484..0x0042F9DE. Marker
// spelling follows memory order exactly. In particular the secondary-color
// command is the legacy byte sequence "D%<digit>", not "%D<digit>".
[[nodiscard]] LegacyDialogTextToken next_legacy_dialog_text_token(
    std::span<const compat::u8> text, std::size_t source_index) noexcept;

inline constexpr compat::u32 kLegacyDialogFlagFastPage = 0x00000100U;
inline constexpr compat::u32 kLegacyDialogFlagHasChoices = 0x00002000U;
inline constexpr compat::u32 kLegacyDialogFlagPageBoundary = 0x00004000U;
inline constexpr compat::u32 kLegacyDialogFlagTerminated = 0x00008000U;
inline constexpr compat::u32 kLegacyDialogFlagClosing = 0x40000000U;

struct LegacyDialogChoiceHotspot {
  compat::u32 kind{1U};
  compat::u16 left{};
  compat::u16 top{};
  compat::u16 right{};
  compat::u16 bottom{};
};

struct LegacyDialogMessage {
  LegacyDialogRecord32 record;
  // The physical record keeps the two original IA-32 pointer tokens. Live
  // owners are carried separately so a 64-bit build can still resolve the
  // exact current TSW resource without changing the 0x4C layout.
  asset_runtime::LegacyActionRecord *frame_action{};
  asset_runtime::LegacyActionRecord *caption_action{};
  std::vector<compat::u8> text;
  std::vector<compat::u8> caption;
  std::size_t text_cursor_index{};
  std::size_t page_stop_index{};
  std::vector<LegacyDialogChoiceHotspot> choices;
  bool active{true};
};

struct LegacyDialogSegmentDrawRequest {
  compat::i32 destination_x{};
  compat::i32 destination_y{};
  std::span<const compat::u8> nul_terminated_text{};
  compat::u16 foreground_index{};
  compat::u16 secondary_index{};
  compat::u8 style{};
  bool selected_choice{};
};

struct LegacyDialogChoiceBackgroundRequest {
  compat::i32 destination_x{};
  compat::i32 destination_y{};
  compat::i32 width{};
  compat::i32 height{};
  compat::i32 surface_width{};
};

class LegacyDialogTextPorts {
public:
  virtual ~LegacyDialogTextPorts() = default;

  [[nodiscard]] virtual bool draw_segment(
      const LegacyDialogSegmentDrawRequest &request) noexcept = 0;
  virtual void draw_selected_choice_background(
      const LegacyDialogChoiceBackgroundRequest &request) noexcept = 0;
  virtual void play_choice_sound() noexcept = 0;
};

struct LegacyDialogTextFrameInput {
  compat::i32 scale{1};
  compat::u16 base_character_delay{};
  compat::i32 selected_choice_index{-1};
  compat::u32 current_tick{};
  bool force_complete{};
};

enum class LegacyDialogTextFrameStatus : compat::u8 {
  completed,
  page_limit_reached,
  page_boundary_reached,
  terminator_reached,
  source_out_of_bounds,
  malformed_control,
  segment_buffer_overflow,
  choice_allocation_failed,
};

struct LegacyDialogTextFrameResult {
  LegacyDialogTextFrameStatus status{LegacyDialogTextFrameStatus::completed};
  LegacyDialogTextTokenStatus token_status{
      LegacyDialogTextTokenStatus::completed};
  compat::u32 segment_draw_count{};
  compat::u32 segment_draw_failure_count{};
  compat::u32 choice_draw_count{};
  compat::u32 choice_hotspot_count{};
  compat::u32 sound_count{};
  std::size_t consumed_byte_count{};
  std::size_t next_visible_index{};
  bool closing_requested{};
};

// 0x0042F43A..0x0042FE14: redraw the visible page and, when +0x2A has
// underflowed, reveal one additional raw character. The caller owns the
// outer input/timeout gates and opening geometry from sub_42ED40.
[[nodiscard]] LegacyDialogTextFrameResult update_legacy_dialog_text(
    LegacyDialogMessage &message, const LegacyDialogTextFrameInput &input,
    LegacyDialogTextPorts &ports) noexcept;

} // namespace openswd3::story_scene
