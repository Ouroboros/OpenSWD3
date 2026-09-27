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

RouteStepOutcome advance_phase_3(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
) {
    switch (prefix.status) {
        case LegacyBattleActorFrameEntryStatus::
            case_eleven_second_draw_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_eleven_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fifteen_second_draw_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifteen_second_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eleven_phase_decrement_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fifteen_phase_decrement_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fifty_phase_increment_ready:
            prefix = continue_legacy_battle_actor_frame_case_six_finish(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_active_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_fifty_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_fifty_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_draw_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_fifty_draw_call(
                *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_count_short_reset_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_short_reset(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_short_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_short_return(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_terminal_reset_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_release_gate(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_release_call_ready:
            if (ports.release == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_release_call(
                    *ports.release, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_reset_prefix_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_reset_prefix(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_particle_gate_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_particle_gate(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_particle_existing_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_hundred_particle_tail_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_particle_phase(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_particle_init_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_initial_clear(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_audio_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_hundred_post_rectangle_sample_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_hundred_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_hundred_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_draw_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_one_draw_call(
                *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_draw_return_ready:
            prefix = continue_legacy_battle_actor_frame_case_hundred_draw_phase(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_count_increment_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_count_increment(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_increment_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_increment_return(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_particle_decoder_arguments_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_decoder_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_decoder_call_ready:
            if (ports.decoder == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_decoder_call(
                *ports.decoder, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_decoder_token_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_decoder_publish(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_post_decoder_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_hundred_dimensions(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_geometry_ready:
            prefix = continue_legacy_battle_actor_frame_case_hundred_geometry(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_geometry_gate_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_configuration(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_attribute_call_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_property(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_metrics_iat_read_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_metrics_prepare(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_metrics_first_call_ready:
            if (ports.metrics == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_metrics_calls(
                    *ports.metrics, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_rectangle_call_ready:
            if (ports.rectangle == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_rectangle_call(
                *ports.rectangle, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_hundred_post_rectangle_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_hundred_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_hundred_phase_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_hundred_phase_write(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifty_one_reset_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifty_one_reset_prefix(
                    actor, request, prefix
                );
            break;

        default:
            return RouteStepOutcome::unhandled;
    }
    return RouteStepOutcome::advance;
}

}  // namespace openswd3::battle::actor_frame_route_detail
