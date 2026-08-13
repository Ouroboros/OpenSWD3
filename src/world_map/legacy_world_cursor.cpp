#include "openswd3/world_map/legacy_world_cursor.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] constexpr compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::u32 right) noexcept {
    return std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(left) - right);
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

[[nodiscard]] constexpr bool
edge_visible_for_talk(const LegacyWorldCursorFrameInput& input) noexcept {
    return input.talk_target == 0xFFFFU || input.talk_phase < 8U;
}

}  // namespace

LegacyWorldCursorState::LegacyWorldCursorState() noexcept {
    asset_runtime::initialize_legacy_action_record(cursor_action);
    cursor_action.action_id = kLegacyWorldCursorActionId;
    cursor_action.base_variant = 0U;

    asset_runtime::initialize_legacy_action_record(edge_action);
    edge_action.action_id = kLegacyWorldCursorActionId;
    edge_action.base_variant = 8U;
}

asset_runtime::LegacyActionUpdateStatus prime_legacy_world_cursor_state(
    LegacyWorldCursorState& state, asset_runtime::LegacyActionDrawPorts& ports
) {
    return ports.update_action_record(state.cursor_action);
}

LegacyWorldCursorResult update_draw_legacy_world_cursor(
    LegacyWorldCursorState& state,
    const LegacyWorldCursorFrameInput& input,
    compat::u32& special_mode_state,
    asset_runtime::LegacyActionDrawPorts& ports
) {
    LegacyWorldCursorResult result;

    if (input.delete_key_pressed) {
        state.cursor_action.base_variant = 15U;
        result.delete_variant_selected = true;
    }

    if (special_mode_state == 0U) {
        if ((std::bit_cast<compat::u32>(input.movement_x) |
             std::bit_cast<compat::u32>(input.movement_y)) != 0U) {
            state.edge_x = wrapping_add(state.edge_x, 1);
            if (state.edge_x > 0) {
                state.edge_x = 0;
            }
        } else {
            state.edge_idle_frames = wrapping_add(state.edge_idle_frames, 1);
            if (state.edge_idle_frames > 16) {
                state.edge_x = wrapping_add(state.edge_x, -1);
                if (state.edge_x < -32) {
                    state.edge_x = -32;
                }

                if (input.left_press_multiplicity != 0U &&
                    std::bit_cast<compat::u32>(input.mouse_x) > 610U &&
                    std::bit_cast<compat::u32>(input.mouse_y) < 24U &&
                    input.talk_target == 0xFFFFU) {
                    special_mode_state = 0x80000001U;
                    result.special_mode_requested = true;
                }
            }
        }

        if (edge_visible_for_talk(input)) {
            result.edge_draw_requested = true;
            result.edge_action = asset_runtime::update_draw_legacy_action(
                state.edge_action, wrapping_add(state.edge_x, 642), 0, ports
            );
        }
    }

    if (state.previous_cursor_base_variant !=
        state.cursor_action.base_variant) {
        state.cursor_action.wait_remaining = 0U;
    }
    state.previous_cursor_base_variant = state.cursor_action.base_variant;

    ++result.cursor_update_count;
    if (ports.update_action_record(state.cursor_action) !=
        asset_runtime::LegacyActionUpdateStatus::completed) {
        ++result.cursor_update_failure_count;
    }

    rendering::LegacyFramePiece piece;
    ++result.cursor_frame_request_count;
    if (!ports.load_frame_piece(
            state.cursor_action.field_4a, state.cursor_action.field_4c, piece
        )) {
        result.status = LegacyWorldCursorStatus::cursor_frame_unavailable;
        return result;
    }

    result.last_cursor_blit_status = ports.draw_frame_piece(
        piece,
        wrapping_subtract(input.mouse_x, state.cursor_action.draw_offset_x),
        wrapping_subtract(input.mouse_y, state.cursor_action.draw_offset_y),
        state.cursor_action.mode_flags,
        static_cast<compat::i32>(state.cursor_action.field_8a)
    );
    ++result.cursor_draw_count;
    if (!accepted_blit_status(result.last_cursor_blit_status)) {
        ++result.cursor_blit_failure_count;
    }
    return result;
}

}  // namespace openswd3::world_map
