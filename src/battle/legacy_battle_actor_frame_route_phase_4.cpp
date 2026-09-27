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

RouteStepOutcome advance_phase_4(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
) {
    switch (prefix.status) {
        case LegacyBattleActorFrameEntryStatus::case_fifty_one_reset_call_ready:
            if (ports.random == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_reset_return(
                    actor, *ports.random, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_one_init_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_initialize(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_one_geometry_ready:
            prefix = continue_legacy_battle_actor_frame_case_fifty_one_geometry(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fifty_one_particle_call_ready:
            prefix = continue_legacy_battle_actor_frame_case_fifty_one_property(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fifty_one_decoder_prepare_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_decoder_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fifty_one_decoder_call_ready:
            if (ports.decoder == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_decoder_call(
                    *ports.decoder, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fifty_one_decoder_token_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_one_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_audio_call(
                    *ports.sound, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_one_tail_ready:
            prefix = continue_legacy_battle_actor_frame_case_fifty_one_tail(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_one_spawn_call_ready:
            if (ports.directional_scan_owners == nullptr) {
                prefix =
                    continue_legacy_battle_actor_frame_case_fifty_one_spawn_entry(
                        request, prefix
                    );
            } else {
                prefix =
                    continue_legacy_battle_actor_frame_case_fifty_one_spawn_call(
                        actor, *ports.directional_scan_owners, request, prefix
                    );
            }
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_five_ten_terminal_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_ten_terminal_return(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_audio_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_audio_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_three_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_three_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_source_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_five_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_source_base_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_source_base_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_five_draw_call(
                actor, *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_five_after_first_draw_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_after_first_draw_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_after_first_draw(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_five_second_draw_prepare_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_second_draw_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_ten_second_draw_prepare_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_ten_second_draw_globals(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_five_second_draw_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_ten_second_draw_globals_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_ten_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_five_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_second_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_five_second_draw_call(
                    *ports.draw, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_clip_arguments_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_clip_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_clip_arguments(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_clip_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_ten_clip_call_ready:
            prefix = continue_legacy_battle_actor_frame_case_five_clip_entry(
                request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_clip_child_typed_stop:
        case LegacyBattleActorFrameEntryStatus::case_ten_clip_child_typed_stop:
            if (ports.clip_raster == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
                    *ports.clip_raster, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_five_ten_clip_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_five_ten_active_return(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_audio_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_twelve_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_globals_ready:
            prefix = continue_legacy_battle_actor_frame_case_twelve_globals(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_global_values_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_source_route(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_forward_args_ready:
        case LegacyBattleActorFrameEntryStatus::case_twelve_reverse_args_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_raster_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_raster_call_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_raster_entry(
                    actor, request, prefix, ports.scaled_rle
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_raster_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_active_phase(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_stack_cleanup_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_active_return(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_reset_tail_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_twelve_reset_prefix(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_twelve_reset_call_ready:
            if (ports.random == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_common_reset_return(
                actor, *ports.random, request, prefix
            );
            break;

        default:
            return RouteStepOutcome::unhandled;
    }
    return RouteStepOutcome::advance;
}

}  // namespace openswd3::battle::actor_frame_route_detail
