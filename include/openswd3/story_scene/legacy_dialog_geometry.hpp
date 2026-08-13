#pragma once

#include "openswd3/compat/types.hpp"

#include <cstddef>

namespace openswd3::story_scene {

inline constexpr std::size_t kLegacyDialogRecordSize = 0x4CU;

// Physical IA-32 record consumed by sub_40AFF0/sub_42ED40. Pointer-valued
// slots remain 32-bit tokens here; modern ownership lives outside this POD.
struct LegacyDialogRecord32 {
    compat::u32 frame_action_pointer_32{};
    compat::u32 caption_action_pointer_32{};
    compat::u32 flags{};
    compat::u32 lifetime_limit{};
    compat::u32 lifetime_started_at{};
    compat::u16 transition_step{};
    compat::u16 role_index{0xFFFDU};
    compat::u16 display_counter{};
    compat::u16 left{};
    compat::u16 top{};
    compat::u16 anchor_left{};
    compat::u16 anchor_top{};
    compat::u16 width{};
    compat::u16 height{};
    compat::u16 field_26{};
    compat::u16 character_delay{};
    compat::u16 character_countdown{};
    compat::u16 foreground_index{};
    compat::u16 secondary_index{};
    compat::u16 saved_foreground_index{};
    compat::u16 saved_secondary_index{};
    compat::u8 text_style{};
    compat::u8 saved_text_style{};
    compat::u16 field_36{};
    compat::u32 text_allocation_pointer_32{};
    compat::u32 text_cursor_pointer_32{};
    compat::u32 page_stop_pointer_32{};
    compat::u32 caption_pointer_32{};
    compat::u32 next_pointer_32{};
};

static_assert(sizeof(LegacyDialogRecord32) == kLegacyDialogRecordSize);
static_assert(offsetof(LegacyDialogRecord32, flags) == 0x08U);
static_assert(offsetof(LegacyDialogRecord32, transition_step) == 0x14U);
static_assert(offsetof(LegacyDialogRecord32, role_index) == 0x16U);
static_assert(offsetof(LegacyDialogRecord32, left) == 0x1AU);
static_assert(offsetof(LegacyDialogRecord32, width) == 0x22U);
static_assert(offsetof(LegacyDialogRecord32, character_delay) == 0x28U);
static_assert(offsetof(LegacyDialogRecord32, foreground_index) == 0x2CU);
static_assert(offsetof(LegacyDialogRecord32, text_style) == 0x34U);
static_assert(
    offsetof(LegacyDialogRecord32, text_allocation_pointer_32) == 0x38U
);
static_assert(offsetof(LegacyDialogRecord32, text_cursor_pointer_32) == 0x3CU);
static_assert(offsetof(LegacyDialogRecord32, page_stop_pointer_32) == 0x40U);
static_assert(offsetof(LegacyDialogRecord32, caption_pointer_32) == 0x44U);
static_assert(offsetof(LegacyDialogRecord32, next_pointer_32) == 0x48U);

inline constexpr compat::u32 kLegacyDialogFlagDirectRectangle = 0x00000040U;
inline constexpr compat::u32 kLegacyDialogFlagSuppressPanel = 0x00000080U;
inline constexpr compat::u32 kLegacyDialogFlagExplicitAnchor = 0x00000800U;
inline constexpr compat::u32 kLegacyDialogFlagAlternateDirectTransition =
    0x00008000U;

struct LegacyDialogAnchorInput {
    compat::i32 scale{1};
    compat::i32 camera_left{};
    compat::i32 camera_top{};
    compat::i32 role_world_x{};
    compat::i32 role_world_y{};
    bool role_anchor_available{};
};

enum class LegacyDialogGeometryStatus : compat::u8 {
    completed,
    role_anchor_unavailable,
};

struct LegacyDialogRectangle {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};

    [[nodiscard]] bool
    operator==(const LegacyDialogRectangle&) const noexcept = default;
};

struct LegacyDialogGeometryResult {
    LegacyDialogGeometryStatus status{LegacyDialogGeometryStatus::completed};
    LegacyDialogRectangle panel{};
    LegacyDialogRectangle text_clip{};
    compat::i32 opacity_step{};
    bool transition_in_progress{true};
    bool panel_draw_requested{true};
};

// 0x0042EDCF..0x0042F11E: advance one opening step, derive the panel
// rectangle and reproduce the following sub_416FF0 arguments. Arithmetic is
// IA-32 wrapping; signed /4 uses C++ truncation toward zero, matching the
// assembly's bias-and-SAR sequences.
[[nodiscard]] LegacyDialogGeometryResult prepare_legacy_dialog_geometry(
    LegacyDialogRecord32& record, const LegacyDialogAnchorInput& input
) noexcept;

}  // namespace openswd3::story_scene
