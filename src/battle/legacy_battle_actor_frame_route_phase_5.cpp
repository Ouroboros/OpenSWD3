#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include "legacy_battle_actor_frame_io_helpers.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_directional_scan.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_scaled_rle_writer.hpp"

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>

#include "legacy_battle_actor_frame_route_internal.hpp"

namespace openswd3::battle::actor_frame_route_detail {

RouteStepOutcome advance_phase_5(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
) {
    switch (prefix.status) {
        case LegacyBattleActorFrameEntryStatus::case_three_audio_ready:
        case LegacyBattleActorFrameEntryStatus::case_four_audio_ready:
        case LegacyBattleActorFrameEntryStatus::case_seven_audio_ready:
        case LegacyBattleActorFrameEntryStatus::case_thirteen_audio_ready:
        case LegacyBattleActorFrameEntryStatus::case_fourteen_audio_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_three_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_four_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_seven_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_thirteen_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_fourteen_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_three_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_three_source_ready:
        case LegacyBattleActorFrameEntryStatus::case_four_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_three_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_seven_source_ready:
        case LegacyBattleActorFrameEntryStatus::case_thirteen_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_seven_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fourteen_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_fourteen_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_seven_initial_source_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_geometry_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_first_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_geometry_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_late_first_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_three_initial_source_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_four_initial_source_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_four_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_initial_source_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_first_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_three_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_four_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_seven_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_first_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_first_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_first_rectangle_call_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_rectangle_entry(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_four_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_first_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_first_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_first_rectangle_child_typed_stop:
            if (ports.clip_raster == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
                    *ports.clip_raster, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_first_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_first_rectangle_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_post_rectangle_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_post_rectangle_globals_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_post_rectangle_globals_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_first_post_rectangle_globals_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_first_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_four_first_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_first_rectangle_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_first_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_first_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_first_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_seven_post_rectangle_globals_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_third_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_first_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_first_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_four_first_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_first_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_seven_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_first_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_first_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_seven_first_draw_call(
                    *ports.draw, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_second_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_four_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_four_second_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_second_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_seven_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_second_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_second_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_late_second_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        default:
            return RouteStepOutcome::unhandled;
    }
    return RouteStepOutcome::advance;
}

}  // namespace openswd3::battle::actor_frame_route_detail
