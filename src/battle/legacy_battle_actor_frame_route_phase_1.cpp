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

RouteStepOutcome advance_phase_1(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
) {
    switch (prefix.status) {
        case LegacyBattleActorFrameEntryStatus::reset_call_ready:
            if (ports.random == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_reset(
                actor, *ports.random, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::reset_release_call_ready:
            prefix = continue_legacy_battle_actor_frame_release(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::linked_node_read_ready:
            if (ports.linked_nodes == nullptr || ports.release == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_release_node(
                ports.linked_nodes->resolve_linked_nodes(prefix.eax),
                *ports.release,
                request,
                prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_ready:
            if (ports.updater == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_update(
                actor, *ports.updater, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_frame_read_ready:
            if (ports.updater == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_lookup(
                actor, *ports.updater, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_post_lookup_ready:
            prefix = continue_legacy_battle_actor_frame_resource_gate(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_resource_byte_read_ready:
            prefix = continue_legacy_battle_actor_frame_selector_header(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_selector_dec_ready:
            prefix = continue_legacy_battle_actor_frame_selector_dispatch(
                request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_selector_default_ready:
            prefix = continue_legacy_battle_actor_frame_default_return(
                request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::update_selector_case_ready:
            if (prefix.eip == 0x004799DCU) {
                prefix = continue_legacy_battle_actor_frame_case_one_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x00479B06U) {
                prefix = continue_legacy_battle_actor_frame_case_two_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047A5FEU) {
                prefix = continue_legacy_battle_actor_frame_case_eight_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047A752U) {
                prefix = continue_legacy_battle_actor_frame_case_nine_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047A1A0U) {
                prefix = continue_legacy_battle_actor_frame_case_six_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047A94DU) {
                prefix = continue_legacy_battle_actor_frame_case_eleven_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047B2E8U) {
                prefix = continue_legacy_battle_actor_frame_case_fifteen_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047B747U) {
                prefix = continue_legacy_battle_actor_frame_case_fifty_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047B409U) {
                prefix = continue_legacy_battle_actor_frame_case_hundred_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047B83EU) {
                prefix =
                    continue_legacy_battle_actor_frame_case_fifty_one_header(
                        actor, request, prefix
                    );
            } else if (prefix.eip == 0x0047A083U || prefix.eip == 0x0047A815U) {
                prefix = continue_legacy_battle_actor_frame_case_five_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047A935U) {
                prefix =
                    continue_legacy_battle_actor_frame_case_five_ten_terminal_writes(
                        actor, request, prefix
                    );
            } else if (prefix.eip == 0x00479CA6U) {
                prefix = continue_legacy_battle_actor_frame_case_three_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x00479EAAU || prefix.eip == 0x0047ABADU) {
                prefix = continue_legacy_battle_actor_frame_case_four_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047A266U) {
                prefix = continue_legacy_battle_actor_frame_case_seven_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047AF24U) {
                prefix =
                    continue_legacy_battle_actor_frame_case_fourteen_header(
                        actor, request, prefix
                    );
            } else if (prefix.eip == 0x0047AA7BU) {
                prefix = continue_legacy_battle_actor_frame_case_twelve_header(
                    actor, request, prefix
                );
            } else if (prefix.eip == 0x0047AB9AU) {
                prefix =
                    continue_legacy_battle_actor_frame_case_twelve_terminal_writes(
                        actor, request, prefix
                    );
            } else if (prefix.eip == 0x0047B801U) {
                prefix = continue_legacy_battle_actor_frame_common_reset_prefix(
                    actor, request, prefix
                );
            } else {
                return RouteStepOutcome::stop;
            }
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_fourteen_progress_reset_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_fourteen_progress_reset(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_reset_progress_write_ready:
            prefix = continue_legacy_battle_actor_frame_common_reset_prefix(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_reset_call_ready:
        case LegacyBattleActorFrameEntryStatus::case_hundred_reset_call_ready:
            if (ports.random == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_common_reset_return(
                actor, *ports.random, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_active_ready:
        case LegacyBattleActorFrameEntryStatus::case_one_source_token_ready:
            prefix = continue_legacy_battle_actor_frame_case_one_active_prefix(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_audio_call_ready:
            if (ports.sound == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_one_audio(
                *ports.sound, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_source_read_ready:
            prefix = continue_legacy_battle_actor_frame_case_one_source_read(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_global_write_ready:
            prefix = continue_legacy_battle_actor_frame_case_one_motion_globals(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_height_ready:
            prefix = continue_legacy_battle_actor_frame_case_one_height(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_draw_args_ready:
            prefix = continue_legacy_battle_actor_frame_case_one_draw_arguments(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_draw_call_ready:
            if (ports.draw == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_one_draw_call(
                *ports.draw, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_draw_return_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_one_phase_increment(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_one_stack_cleanup_ready:
            prefix = continue_legacy_battle_actor_frame_case_one_return(
                request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_release_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_release_prefix(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_emitter_clear_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_emitter_clear(
                prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_release_call_ready:
            if (ports.release == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_release_call(
                *ports.release, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_emitter_reset_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_emitter_reset(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_particle_init_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_initial_clear(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_decoder_prepare_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_decoder_arguments(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_decoder_call_ready:
            if (ports.decoder == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_decoder_call(
                *ports.decoder, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_two_decoder_token_write_ready:
            prefix =
                continue_legacy_battle_actor_frame_case_two_decoder_publish(
                    actor, request, prefix
                );
            break;

        case LegacyBattleActorFrameEntryStatus::
            case_two_post_decoder_frame_read_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_dimensions(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_geometry_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_geometry(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_emitter_flags_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_emitter_fields(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_property_call_ready:
            prefix = continue_legacy_battle_actor_frame_case_two_property(
                actor, request, prefix
            );
            break;

        case LegacyBattleActorFrameEntryStatus::case_two_metrics_iat_read_ready:
            if (ports.metrics == nullptr) {
                return RouteStepOutcome::stop;
            }
            prefix = continue_legacy_battle_actor_frame_case_two_metrics(
                *ports.metrics, request, prefix
            );
            break;

        default:
            return RouteStepOutcome::unhandled;
    }
    return RouteStepOutcome::advance;
}

}  // namespace openswd3::battle::actor_frame_route_detail
