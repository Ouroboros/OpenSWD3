#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldCursorActionId = 0x2329U;

struct LegacyWorldCursorState {
    LegacyWorldCursorState() noexcept;

    asset_runtime::LegacyActionRecord cursor_action{};
    asset_runtime::LegacyActionRecord edge_action{};
    compat::i32 edge_x{2};
    compat::i32 edge_idle_frames{};
    compat::u32 previous_cursor_base_variant{};
};

struct LegacyWorldCursorFrameInput {
    bool delete_key_pressed{};
    compat::i32 mouse_x{};
    compat::i32 mouse_y{};
    compat::u32 left_press_multiplicity{};
    compat::i32 movement_x{};
    compat::i32 movement_y{};
    compat::u16 talk_target{0xFFFFU};
    compat::u16 talk_phase{};
};

enum class LegacyWorldCursorStatus : compat::u8 {
    completed,
    edge_frame_unavailable,
    cursor_frame_unavailable,
};

struct LegacyWorldCursorResult {
    LegacyWorldCursorStatus status{LegacyWorldCursorStatus::completed};
    asset_runtime::LegacyActionDrawResult edge_action;
    rendering::LegacyBlitExecutionStatus last_cursor_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
    compat::u32 cursor_update_count{};
    compat::u32 cursor_update_failure_count{};
    compat::u32 cursor_frame_request_count{};
    compat::u32 cursor_draw_count{};
    compat::u32 cursor_blit_failure_count{};
    bool delete_variant_selected{};
    bool edge_draw_requested{};
    bool special_mode_requested{};
};

// 0x0040E0B0 primes the main 0x2329 record once after initialization. Its
// diagnostic failure is nonfatal in the original process.
[[nodiscard]] asset_runtime::LegacyActionUpdateStatus
prime_legacy_world_cursor_state(
    LegacyWorldCursorState& state, asset_runtime::LegacyActionDrawPorts& ports
);

// sub_4149B0: maintain the right-edge trigger and draw the software cursor.
// The main action update's diagnostic branch deliberately continues to its
// current frame, matching the assembly rather than the stricter 0x0040EBF0
// bridge used by the edge action.
[[nodiscard]] LegacyWorldCursorResult update_draw_legacy_world_cursor(
    LegacyWorldCursorState& state,
    const LegacyWorldCursorFrameInput& input,
    compat::u32& special_mode_state,
    asset_runtime::LegacyActionDrawPorts& ports
);

}  // namespace openswd3::world_map
