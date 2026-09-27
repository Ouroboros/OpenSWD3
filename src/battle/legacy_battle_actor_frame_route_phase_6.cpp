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

RouteStepOutcome advance_phase_6(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
) {
    switch (prefix.status) {
        case LegacyBattleActorFrameEntryStatus::
            case_three_second_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_second_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_second_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_second_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_second_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_second_rectangle_call_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_rectangle_entry(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_second_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_four_second_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_second_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_second_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_second_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_second_rectangle_child_typed_stop:
            if (ports.clip_raster == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
                    *ports.clip_raster, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_second_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_second_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_second_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_second_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_second_rectangle_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_post_rectangle_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_second_post_rectangle_globals_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_second_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_four_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_second_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_second_rectangle_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_second_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_second_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_second_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_late_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_seven_second_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_first_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_second_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_seven_first_draw_call(
                    *ports.draw, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_second_draw_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_four_second_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_four_shared_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_second_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_third_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_seven_second_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_third_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_early_second_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_early_phase_tail(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_late_second_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_late_phase_tail(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_shared_rectangle_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_shared_rectangle_arguments(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_four_shared_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_third_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_fourth_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_shared_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_shared_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_third_rectangle_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_fourth_rectangle_call_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_rectangle_entry(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_four_shared_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_third_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_fourth_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_shared_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_shared_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_third_rectangle_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_fourth_rectangle_child_typed_stop:
            if (ports.clip_raster == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
                    *ports.clip_raster, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_three_four_shared_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_shared_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_shared_rectangle_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_shared_return(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_third_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_fourth_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_third_rectangle_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_fourth_rectangle_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_post_rectangle_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_third_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_third_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_seven_fourth_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_fourth_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_third_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_fourth_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_third_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_fourth_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_seven_first_draw_call(
                    *ports.draw, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_third_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_fourth_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_fourth_post_rectangle_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_thirteen_fourth_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_thirteen_fourth_draw_return_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_seven_fourth_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_shared_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_seven_third_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_seven_fourth_rectangle_arguments(
                    actor, request, prefix
                );
            break;

        default:
            return RouteStepOutcome::unhandled;
    }
    return RouteStepOutcome::advance;
}

}  // namespace openswd3::battle::actor_frame_route_detail
