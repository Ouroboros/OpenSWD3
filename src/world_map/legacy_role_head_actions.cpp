#include "openswd3/world_map/legacy_role_head_actions.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] constexpr compat::i32
field_as_i32(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] constexpr compat::i16
wrapping_add_word(const compat::i16 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u32>(std::bit_cast<compat::u16>(left)) +
        static_cast<compat::u32>(right)
    ));
}

[[nodiscard]] constexpr compat::i16 wrapping_multiply_word(
    const compat::i16 value, const compat::u32 multiplier
) noexcept {
    return std::bit_cast<compat::i16>(static_cast<compat::u16>(
        static_cast<compat::u32>(std::bit_cast<compat::u16>(value)) * multiplier
    ));
}

[[nodiscard]] constexpr bool accepted_blit_status(
    const rendering::LegacyBlitExecutionStatus status
) noexcept {
    return status == rendering::LegacyBlitExecutionStatus::completed ||
        status == rendering::LegacyBlitExecutionStatus::clipped_out ||
        status == rendering::LegacyBlitExecutionStatus::opacity_disabled;
}

}  // namespace

LegacyRoleHeadActionResult update_draw_legacy_role_head_actions(
    LegacyRoleHeadActionList& nodes,
    asset_runtime::LegacyActionDrawPorts& action_ports
) {
    LegacyRoleHeadActionResult result;
    for (auto current = nodes.begin(); current != nodes.end();) {
        ++result.visited_count;
        if (action_ports.update_action_record(current->action) !=
            asset_runtime::LegacyActionUpdateStatus::completed) {
            ++result.action_update_failure_count;
        }

        rendering::LegacyFramePiece piece;
        ++result.frame_request_count;
        if (!action_ports.load_frame_piece(
                current->action.field_4a, current->action.field_4c, piece
            )) {
            ++result.frame_failure_count;
        } else {
            result.last_blit_status = action_ports.draw_frame_piece(
                piece,
                wrapping_subtract(
                    static_cast<compat::i32>(current->current_x),
                    field_as_i32(current->action.draw_offset_x)
                ),
                wrapping_subtract(
                    static_cast<compat::i32>(current->y),
                    field_as_i32(current->action.draw_offset_y)
                ),
                current->action.mode_flags,
                static_cast<compat::i32>(current->action.field_8a)
            );
            ++result.draw_count;
            if (!accepted_blit_status(result.last_blit_status)) {
                ++result.blit_failure_count;
            }
        }

        const compat::u16 motion_bits =
            std::bit_cast<compat::u16>(current->horizontal_motion);
        if ((motion_bits & 0x7FFFU) == 0U) {
            const compat::i32 distance =
                static_cast<compat::i32>(current->target_x) -
                static_cast<compat::i32>(current->current_x);
            const compat::i32 step = (distance * 2) / 3;
            current->current_x = wrapping_add_word(current->current_x, step);
            if (step >= -1 && step <= 1) {
                current->current_x = current->target_x;
            }
            ++current;
            continue;
        }

        current->current_x = wrapping_add_word(
            current->current_x,
            static_cast<compat::i32>(current->horizontal_motion)
        );
        current->horizontal_motion =
            wrapping_multiply_word(current->horizontal_motion, 3U);
        if (current->current_x <= -120 || current->current_x >= 760) {
            current = nodes.erase(current);
            ++result.removed_count;
        } else {
            ++current;
        }
    }
    return result;
}

}  // namespace openswd3::world_map
