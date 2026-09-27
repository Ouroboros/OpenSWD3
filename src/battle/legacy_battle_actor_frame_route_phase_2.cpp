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

RouteStepOutcome advance_phase_2(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
) {
    switch (prefix.status) {
        case LegacyBattleActorFrameEntryStatus::case_two_rectangle_call_ready:
            if (ports.rectangle == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_rectangle_call(
                *ports.rectangle, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_two_sample_code_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_sample_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_sample_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_sample_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_two_sample_phase_write_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_sample_phase(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_particle_tail_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_particle_arguments(
                    request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_particle_call_ready:
            if (ports.particle == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_particle_call(
                *ports.particle, actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_two_particle_phase_100_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_particle_return(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_particle_init_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_initial_clear(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eight_decoder_prepare_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_decoder_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_decoder_call_ready:
            if (ports.decoder == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_decoder_call(
                *ports.decoder, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eight_decoder_token_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_decoder_publish(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eight_post_decoder_frame_read_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_dimensions(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_geometry_ready:
            prefix = continue_legacy_battle_actor_frame_case_eight_geometry(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_geometry_value_ready:
            prefix = continue_legacy_battle_actor_frame_case_eight_fields(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_property_call_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_property(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eight_metrics_iat_read_ready:
            if (ports.metrics == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_metrics(
                *ports.metrics, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_rectangle_call_ready:
            if (ports.rectangle == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_rectangle_call(
                *ports.rectangle, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eight_sample_handle_read_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_eight_sample_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eight_sample_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_sample_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eight_sample_phase_write_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_sample_phase(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_nine_active_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_nine_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_nine_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_nine_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_nine_source_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_nine_source_and_opacity(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_nine_draw_flags_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_nine_draw_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_nine_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_nine_draw_call(
                *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_nine_draw_globals_ready:
            prefix = continue_legacy_battle_actor_frame_case_nine_finish(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_six_active_ready:
        case LegacyBattleActorFrameEntryStatus::case_eleven_active_ready:
        case LegacyBattleActorFrameEntryStatus::case_fifteen_active_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_six_audio_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_six_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_eleven_audio_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_fifteen_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_six_audio_call(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_six_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_six_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_six_draw_parameters_ready:
            prefix = continue_legacy_battle_actor_frame_case_six_draw_arguments(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_six_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_six_draw_call(
                *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_six_phase_decrement_ready:
            prefix = continue_legacy_battle_actor_frame_case_six_finish(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_eleven_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_eleven_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_fifteen_source_ready:
            prefix = continue_legacy_battle_actor_frame_case_fifteen_source(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eleven_first_draw_arguments_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fifteen_first_draw_arguments_ready:
            prefix = continue_legacy_battle_actor_frame_case_six_draw_arguments(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eleven_first_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fifteen_first_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_eleven_second_draw_call_ready:
        case LegacyBattleActorFrameEntryStatus::
            case_fifteen_second_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_six_draw_call(
                *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_eleven_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_eleven_between_draws(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fifteen_first_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fifteen_between_draws(
                    actor, request, prefix
                );
            break;

        default:
            return RouteStepOutcome::unhandled;
    }
    return RouteStepOutcome::advance;
}

}  // namespace openswd3::battle::actor_frame_route_detail
