#include "openswd3/world_map/legacy_picture_actions.hpp"

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

[[nodiscard]] compat::u32 release_picture_action_list(
    std::list<LegacyPictureActionNode>& nodes
) noexcept {
    compat::u32 release_count = 0U;
    std::list<LegacyPictureActionNode> detached;
    while (!nodes.empty()) {
        detached.splice(detached.end(), nodes, nodes.begin());
        detached.pop_front();
        ++release_count;
    }
    return release_count;
}

}  // namespace

LegacyPictureActionReleaseResult
release_legacy_picture_actions(LegacyPictureActionLists& lists) noexcept {
    LegacyPictureActionReleaseResult result;
    result.primary_release_count = release_picture_action_list(lists.primary);
    result.secondary_release_count =
        release_picture_action_list(lists.secondary);
    return result;
}

LegacyPictureActionResult update_draw_legacy_picture_actions(
    std::list<LegacyPictureActionNode>& nodes,
    const compat::i32 camera_left,
    const compat::i32 camera_top,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    LegacyPictureActionAudioPorts& audio_ports
) {
    LegacyPictureActionResult result;
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
            result.status = LegacyPictureActionStatus::frame_load_failed;
            return result;
        }
        result.last_blit_status = action_ports.draw_frame_piece(
            piece,
            wrapping_subtract(
                static_cast<compat::i32>(current->screen_x),
                current->action.draw_offset_x
            ),
            wrapping_subtract(
                static_cast<compat::i32>(current->screen_y),
                current->action.draw_offset_y
            ),
            current->action.mode_flags,
            static_cast<compat::i32>(current->action.field_8a)
        );
        ++result.draw_count;
        if (!accepted_blit_status(result.last_blit_status)) {
            ++result.blit_failure_count;
        }

        if (current->action.field_58 != 0U) {
            audio_ports.play_positional_sample(
                current->action.field_58,
                wrapping_add(
                    static_cast<compat::i32>(current->screen_x), camera_left
                ),
                wrapping_add(
                    static_cast<compat::i32>(current->screen_y), camera_top
                )
            );
            current->action.field_58 = 0U;
            ++result.positional_sample_count;
        }

        if (current->action.field_8c == 1U) {
            current = nodes.erase(current);
            ++result.removed_count;
        } else {
            ++current;
        }
    }
    return result;
}

}  // namespace openswd3::world_map
