#include "openswd3/battle/legacy_battle_message_phase.hpp"

#include <array>
#include <bit>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleMessagePhaseCall;
using openswd3::battle::LegacyBattleMessagePhaseCallReply;
using openswd3::battle::LegacyBattleMessagePhaseCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

class VictoryActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(const u32, const u32, const bool) override {
        constexpr std::array<u16, 8> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
};

class VictoryFrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        if (fail || piece_index >= pixels.size()) {
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = pixels[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::array<u8, 2>, 9> pixels{{
        {1U, 0U},
        {2U, 0U},
        {3U, 0U},
        {4U, 0U},
        {5U, 0U},
        {6U, 0U},
        {7U, 0U},
        {8U, 0U},
        {9U, 0U},
    }};
    bool fail{};
};

class MessagePort final
    : public openswd3::battle::LegacyBattleMessagePhasePort {
public:
    [[nodiscard]] LegacyBattleMessagePhaseCallReply invoke_message_phase(
        const LegacyBattleMessagePhaseCallRequest& request
    ) override {
        message_calls.push_back(request);
        auto& index = message_reply_indices[request.call];
        const auto found = message_replies.find(request.call);
        if (found == message_replies.end() || index >= found->second.size()) {
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }
        return found->second[index++];
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionCallReply invoke(
        const openswd3::battle::LegacyBattleActionCallRequest& request
    ) override {
        action_calls.push_back(request);
        if (action_reply_index >= action_replies.size()) {
            return {};
        }
        return action_replies[action_reply_index++];
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(
        const openswd3::battle::LegacyBattleInputDispatchCallRequest& request
    ) override {
        input_calls.push_back(request);
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        sample_calls.push_back({sound_id, std::bit_cast<u32>(mix_level)});
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }

    void reply(
        const LegacyBattleMessagePhaseCall call,
        const LegacyBattleMessagePhaseCallReply& reply
    ) {
        message_replies[call].push_back(reply);
    }

    [[nodiscard]] u32 count(const LegacyBattleMessagePhaseCall call) const {
        u32 value = 0U;
        for (const auto& request : message_calls) {
            if (request.call == call) {
                ++value;
            }
        }
        return value;
    }

    std::vector<LegacyBattleMessagePhaseCallRequest> message_calls;
    std::map<
        LegacyBattleMessagePhaseCall,
        std::vector<LegacyBattleMessagePhaseCallReply>>
        message_replies;
    std::map<LegacyBattleMessagePhaseCall, std::size_t> message_reply_indices;
    std::vector<openswd3::battle::LegacyBattleActionCallRequest> action_calls;
    std::vector<openswd3::battle::LegacyBattleActionCallReply> action_replies;
    std::size_t action_reply_index{};
    std::vector<openswd3::battle::LegacyBattleInputDispatchCallRequest>
        input_calls;
    std::vector<std::array<u32, 2>> sample_calls;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        port.battle_victory_reward_state().committed_money_word = 0x8000U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleMessagePhaseBindings
    bindings() {
        return {
            .state = state,
            .startup = startup,
            .final_actor = final_actor,
            .action = action,
            .metrics = metrics,
            .debug_hotkeys = debug_hotkeys,
            .input_dispatch = input_dispatch,
            .frame_input_resolution = frame_input,
            .selection_frame = selection_frame,
            .target_selection = target_selection,
            .target_ready_gate = target_ready_gate,
            .message_state = message,
            .dialogs = dialogs,
            .one_shot_interaction_state = one_shot_interaction_state,
            .outcome_darkening_gate = outcome_darkening_gate,
            .input_records = input.records,
            .action_profile_bytes = action_profiles,
            .victory_rewards = {
                .state = port.battle_victory_reward_state(),
                .startup = startup,
                .metrics = metrics,
                .input_dispatch = input_dispatch,
                .target_selection = target_selection,
                .party_member_resources = party_member_resources,
                .script_variables = script_variables,
                .framebuffer = framebuffer,
                .raster = raster,
                .shared_effects = effects,
                .jitter = jitter,
                .action_updater = action_updater,
                .frame_provider = frame_provider,
            },
        };
    }

    openswd3::battle::LegacyBattleMessagePhaseState state;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleDebugHotkeyState debug_hotkeys;
    openswd3::battle::LegacyBattleInputDispatchState input_dispatch;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame_input;
    openswd3::battle::LegacyBattleSelectionFrameState selection_frame;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target_selection;
    u32 target_ready_gate{};
    u32 message{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    u32 one_shot_interaction_state{};
    u32 outcome_darkening_gate{};
    openswd3::input_time_rng::LegacyInputNormalizationState input;
    std::vector<u8> action_profiles = std::vector<u8>(600U, 0U);
    std::array<openswd3::world_map::LegacyWorldStoryPartyMemberResources, 4>
        party_member_resources{};
    std::array<u32, 64> script_variables{};
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    VictoryActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    VictoryFrameProvider frame_provider;
    MessagePort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleMessagePhaseResult
run(Fixture& fixture,
    const u32 ecx = 0x11111111U,
    const u32 edx = 0x22222222U) {
    return openswd3::battle::advance_legacy_battle_message_phase(
        fixture.bindings(), fixture.port, {.entry_ecx = ecx, .entry_edx = edx}
    );
}

}  // namespace

void test_battle_message_phase(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.message = 0x60U;
        fixture.state.entry_list_gate = 0x12345678U;
        fixture.input_dispatch.selection_cache_gate_b = 3U;
        const auto result = run(fixture);
        test.expect_true(
            result.return_eax == 0x12345678U && result.port_calls == 0U &&
                fixture.message == 0x60U &&
                fixture.input_dispatch.selection_cache_gate_b == 3U,
            "message phase entry list gate returns before reading the battle message"
        );

        fixture.state.entry_list_gate = 0U;
        fixture.startup.reset.block_5214f8[0U] = 1U;
        const auto secondary = run(fixture);
        test.expect_true(
            secondary.return_eax == 0U && fixture.message == 0x60U,
            "message phase secondary object gate returns with the zero entry EAX"
        );

        fixture.startup.reset.block_5214f8[0U] = 0U;
        fixture.message = 0x5FU;
        const auto below = run(fixture);
        fixture.message = 0x72U;
        const auto above = run(fixture);
        test.expect_true(
            below.return_eax == 0xFFFFFFFFU && above.return_eax == 0x12U,
            "message phase preserves unsigned switch rejection below and above 96 through 113"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x60U;
        fixture.input_dispatch.selection_cache_gate_b = 5U;
        fixture.target_selection.selection_input_gate = 6U;
        const auto result = run(fixture);
        test.expect_true(
            result.return_eax == 0U && fixture.message == 0U &&
                fixture.input_dispatch.selection_cache_gate_b == 0U &&
                fixture.target_selection.selection_input_gate == 0U,
            "message 96 clears cache B, selection input and the message in order"
        );

        fixture.message = 0x62U;
        const auto message_98 = run(fixture);
        test.expect_true(
            message_98.port_calls == 1U &&
                fixture.input_dispatch.selection_cache_gate_a == 1U &&
                fixture.port.message_calls.back().call ==
                    LegacyBattleMessagePhaseCall::prepare_message_98,
            "message 98 publishes cache A before its opaque preparation call"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x61U;
        fixture.metrics.group_a_count = 0U;
        fixture.startup.party[0U].position_x = 0xFFFFU;
        fixture.startup.party[0U].position_y = 0x8000U;
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::resolve_group_a_position,
            {.eax = 1U, .ecx = 0xABCDEF01U, .edx = 0xABCDEF02U}
        );
        const auto result = run(fixture);
        const auto& call = fixture.port.message_calls[0U];
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        completed &&
                result.return_eax == 1U &&
                fixture.target_selection.transition_actor_index == 1U &&
                call.actor_token ==
                    openswd3::battle::
                        kLegacyBattleMessagePhaseGroupABaseToken &&
                call.eax == 0U && call.ecx == call.actor_token &&
                call.edx == 0xFFFF8000U && call.arguments[0U] == 0xFFFFFFFFU &&
                call.arguments[1U] == 0xFFFF8000U,
            "message 97 sign-extends the next group-A display position and publishes AL on exact success"
        );

        fixture.metrics.group_a_count = 10U;
        const auto stopped = run(fixture);
        test.expect_true(
            stopped.status ==
                openswd3::battle::LegacyBattleMessagePhaseStatus::
                    group_a_position_typed_stop,
            "message 97 stops at the first display-slot read beyond ten group-A records"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x63U;
        fixture.metrics.group_b_count = 2U;
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 1U}
        );
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 0U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        completed &&
                result.group_b_reset_calls == 2U &&
                result.group_b_completion_calls == 2U &&
                fixture.message == 0U && result.return_eax == 2U &&
                fixture.input_dispatch.retreat_block_word == 0U &&
                fixture.input_dispatch.selected_actor_cleanup_gate == 0U &&
                fixture.frame_input.target_selection_suppression == 1U,
            "message 99 clears its prefix then abandons setup when not every live group-B actor completes"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x63U;
        fixture.metrics.group_b_count = 1U;
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::reset_actor_state,
            {.publish_group_b_count = true, .group_b_count = 9U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        group_b_actor_typed_stop &&
                result.group_b_reset_calls == 8U &&
                result.group_b_completion_calls == 8U,
            "message 99 reloads the live group-B bound until the ninth actor stops at its first real call"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x63U;
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::prepare_transition_control,
            {.eax = 0xA5A50000U, .publish_transition_control_words = true}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.return_eax == 0xA5A50000U && fixture.message == 0x64U &&
                fixture.target_selection.transition_aux_byte == 0U,
            "message 99 sends an empty control pair to message 100 while preserving EAX high word"
        );

        Fixture completed;
        completed.message = 0x63U;
        completed.port.reply(
            LegacyBattleMessagePhaseCall::prepare_transition_control,
            {
                .eax = 0xB6B60000U,
                .publish_transition_control_words = true,
                .transition_control_words = 1U,
            }
        );
        completed.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 1U}
        );
        const auto completed_result = run(completed);
        test.expect_true(
            completed_result.return_eax == 1U && completed.message == 0x62U &&
                completed.target_selection.transition_aux_byte == 2U,
            "message 99 sends an already completed selected actor back to message 98 and aux two"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x63U;
        fixture.metrics.group_a_count = 2U;
        fixture.target_ready_gate = 9U;
        fixture.selection_frame.display_gate = 9U;
        fixture.target_selection.special_action_count = 5U;
        fixture.startup.action_mode_source.actor_label_indices[1U] = 2U;
        fixture.action_profiles[112U] = 0x7AU;
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::prepare_transition_control,
            {
                .publish_transition_control_words = true,
                .transition_control_words = 1U,
            }
        );
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 0U}
        );
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_resource, {.eax = 0x55U}
        );
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::resolve_action_item, {.eax = 0x1234U}
        );
        fixture.port.action_replies = {
            {.eax = 0x00900000U},
            {.eax = 0x11111111U},
        };
        const auto result = run(fixture);
        const auto resolved = fixture.port.message_calls.back();
        const auto committed = fixture.port.message_calls[10U];
        const auto configured = fixture.port.message_calls[11U];
        const auto resource = fixture.port.message_calls[12U];
        bool records_reset = true;
        for (const auto& record : fixture.startup.reset.records_524788) {
            records_reset = records_reset && record.value_00 == 0xFFFFFFFFU &&
                record.value_04 == 0U && record.value_18 == 0U;
        }
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        completed &&
                result.player_item_quantity_calls == 1U &&
                result.player_item_quantity.created &&
                fixture.port.battle_victory_reward_state()
                        .player_item_tokens[0U] == 0x0090000CU &&
                fixture.target_selection.transition_aux_byte == 1U &&
                fixture.target_selection.special_action_count == 4U &&
                result.return_eax == 4U && records_reset &&
                fixture.metrics.priority_actor_index == 9U &&
                fixture.metrics.priority_actor_record_tail[0U] == 0U &&
                fixture.debug_hotkeys.committed_actor_code == 9U &&
                fixture.final_actor.published_actor_code == 0U &&
                fixture.startup.reset.block_520e90[5U] == 1U &&
                fixture.selection_frame.display_gate == 0U &&
                fixture.target_ready_gate == 0U,
            "message 99 rebuilds records, publishes actor one and consumes one resolved player item"
        );
        test.expect_true(
            resolved.call ==
                    LegacyBattleMessagePhaseCall::resolve_action_item &&
                resolved.actor_token ==
                    openswd3::battle::
                        kLegacyBattleMessagePhaseGroupBBaseToken &&
                resolved.arguments[0U] == 0x55U &&
                resolved.arguments[1U] == 0x7AU,
            "message 99 passes the selected resource and low-byte profile through the group-B item resolver"
        );
        test.expect_true(
            committed.call ==
                    LegacyBattleMessagePhaseCall::commit_active_actor &&
                committed.eax == 3021U && committed.edx == 0U &&
                configured.call ==
                    LegacyBattleMessagePhaseCall::configure_actor_action &&
                configured.eax == 1007U && configured.edx == 3021U &&
                resource.call ==
                    LegacyBattleMessagePhaseCall::query_actor_resource &&
                resource.eax == 3021U && resource.edx == 5U,
            "message 99 preserves each distinct group-A address-multiply register snapshot"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x63U;
        fixture.metrics.group_b_count = 8U;
        fixture.metrics.group_a_count = 10U;
        fixture.action_profiles.clear();
        for (u32 index = 0U; index < 8U; ++index) {
            fixture.port.reply(
                LegacyBattleMessagePhaseCall::query_actor_completion,
                {.eax = 1U}
            );
        }
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::prepare_transition_control,
            {
                .publish_transition_control_words = true,
                .transition_control_words = 9U,
            }
        );
        fixture.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 0U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        action_profile_typed_stop &&
                result.group_b_reset_calls == 8U &&
                result.group_a_reset_calls == 10U &&
                result.group_a_prepare_calls == 10U &&
                result.call_trace_count > 40U &&
                result.call_trace.size() == result.call_trace_count,
            "message 99 preserves unbounded dynamic call tracing across all eight and ten actor loops"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x64U;
        fixture.state.transition_mode_gate = 1U;
        const auto gated = run(fixture);
        test.expect_true(
            gated.return_eax == 1U &&
                fixture.target_selection.completion_gate == 1U &&
                gated.port_calls == 0U,
            "message 100 completes immediately when its mode gate is exactly one"
        );

        Fixture timed;
        timed.message = 0x64U;
        timed.target_selection.transition_timer = 149U;
        timed.input_dispatch.retreat_block_word = 1U;
        const auto advanced = run(timed, 0x12340000U, 0x56780000U);
        test.expect_true(
            advanced.target_selection_entry_calls == 1U &&
                advanced.victory_reward_calls == 1U &&
                advanced.victory_rewards.status ==
                    openswd3::battle::LegacyBattleVictoryRewardStatus::
                        completed &&
                timed.port.count(
                    LegacyBattleMessagePhaseCall::
                        reserved_advance_message_100_slot
                ) == 0U &&
                timed.target_selection.transition_timer == 150U &&
                timed.debug_hotkeys.actor_retarget_gate_53bf64 == 0U &&
                timed.input_dispatch.selection_cache_gate_a == 1U &&
                timed.input_dispatch.selection_cache_gate_b == 1U &&
                timed.target_ready_gate == 1U &&
                timed.final_actor.queued_actor_code == 0U,
            "message 100 directly distributes victory rewards before publishing setup gates and entering target selection"
        );

        Fixture stopped;
        stopped.message = 0x64U;
        stopped.debug_hotkeys.actor_retarget_gate_53bf64 = 9U;
        stopped.frame_provider.fail = true;
        const auto stopped_result = run(stopped);
        test.expect_true(
            stopped_result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        victory_rewards_typed_stop &&
                stopped_result.victory_reward_calls == 1U &&
                stopped_result.victory_rewards.status ==
                    openswd3::battle::LegacyBattleVictoryRewardStatus::
                        title_frame_typed_stop &&
                stopped.debug_hotkeys.actor_retarget_gate_53bf64 == 9U &&
                stopped.input_dispatch.selection_cache_gate_a == 0U &&
                stopped.target_selection.transition_timer == 0U,
            "message 100 propagates victory panel failure before all caller-owned setup writes"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x65U;
        fixture.target_selection.transition_actor_index = 0xFFU;
        const auto missing = run(fixture);
        test.expect_true(
            missing.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        completed &&
                fixture.message == 0x70U &&
                fixture.target_selection.transition_timer == 0U &&
                fixture.port.count(
                    LegacyBattleMessagePhaseCall::select_message_101_actor
                ) == 1U,
            "message 101 transitions to 112 when actor selection remains minus one"
        );

        Fixture selected;
        selected.message = 0x65U;
        selected.target_selection.transition_actor_index = 0xFFU;
        selected.port.reply(
            LegacyBattleMessagePhaseCall::select_message_101_actor,
            {
                .edx = 0x22222222U,
                .publish_transition_actor_index = true,
                .transition_actor_index = 2U,
            }
        );
        selected.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 0U}
        );
        selected.port.reply(
            LegacyBattleMessagePhaseCall::allocate_actor_transition,
            {.eax = 0x88U}
        );
        const auto selected_result = run(selected);
        test.expect_true(
            selected_result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        completed &&
                selected.target_selection.transition_state == 0x88U &&
                selected.target_selection.transition_timer == 1U &&
                selected.port.message_calls[1U].eax == 2014U &&
                selected.port.message_calls[1U].edx == 0x22222222U &&
                selected.port.message_calls[2U].eax == 2014U &&
                selected.port.message_calls[2U].edx == 6042U &&
                selected.port.count(
                    LegacyBattleMessagePhaseCall::advance_message_101
                ) == 1U,
            "message 101 allocates a transition and advances the selected actor timer"
        );

        Fixture invalid;
        invalid.message = 0x65U;
        invalid.target_selection.transition_actor_index = 0xFEU;
        const auto invalid_result = run(invalid);
        test.expect_true(
            invalid_result.status ==
                openswd3::battle::LegacyBattleMessagePhaseStatus::
                    group_a_actor_typed_stop,
            "message 101 sign-extends actor FE and stops at the first real actor query"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x6EU;
        fixture.target_selection.transition_actor_index = 0xFFU;
        fixture.target_selection.transition_state = 1U;
        const auto nonzero = run(fixture);
        test.expect_true(
            nonzero.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        completed &&
                fixture.port.count(
                    LegacyBattleMessagePhaseCall::advance_message_110
                ) == 1U,
            "message 110 with existing transition state calls its frame even for actor FF"
        );

        Fixture zero;
        zero.message = 0x6EU;
        zero.target_selection.transition_actor_index = 0xFFU;
        const auto stopped = run(zero);
        test.expect_true(
            stopped.status ==
                openswd3::battle::LegacyBattleMessagePhaseStatus::
                    group_a_actor_typed_stop,
            "message 110 with no transition state stops at the signed actor minus-one query"
        );

        Fixture message_111;
        message_111.message = 0x6FU;
        message_111.target_selection.transition_timer = 0xFFFFFFFFU;
        const auto wrapped = run(message_111);
        test.expect_true(
            wrapped.return_eax == 0U &&
                message_111.target_selection.transition_timer == 0U &&
                message_111.port.count(
                    LegacyBattleMessagePhaseCall::advance_message_111
                ) == 1U,
            "message 111 advances its frame then wraps the full timer dword"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x70U;
        fixture.target_selection.transition_actor_index = 0xFFU;
        const auto missing = run(fixture);
        test.expect_true(
            fixture.message == 0x71U &&
                fixture.target_selection.transition_timer == 0U &&
                missing.sample_calls == 0U,
            "message 112 transitions to 113 without a sample when selection remains FF"
        );

        Fixture selected;
        selected.message = 0x70U;
        selected.target_selection.transition_actor_index = 0xFFU;
        selected.input_dispatch.sample_mix_level = -4;
        selected.port.reply(
            LegacyBattleMessagePhaseCall::select_message_112_actor,
            {
                .publish_transition_actor_index = true,
                .transition_actor_index = 2U,
            }
        );
        selected.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 0U}
        );
        const auto advanced = run(selected);
        test.expect_true(
            advanced.sample_calls == 1U &&
                selected.port.sample_calls[0U][0U] == 0x160U &&
                selected.port.sample_calls[0U][1U] == 0xFFFFFFFCU &&
                selected.port.count(
                    LegacyBattleMessagePhaseCall::allocate_actor_transition
                ) == 1U &&
                selected.port.message_calls[1U].eax == 6042U &&
                selected.port.message_calls[2U].eax == 2014U &&
                selected.port.message_calls[2U].edx == 0U &&
                selected.port.count(
                    LegacyBattleMessagePhaseCall::advance_message_112
                ) == 1U &&
                selected.target_selection.transition_timer == 1U,
            "message 112 samples, queries, allocates and advances a newly selected actor"
        );
    }

    {
        Fixture fixture;
        fixture.message = 0x71U;
        fixture.target_selection.transition_actor_index = 0xFFU;
        fixture.target_selection.transition_sample_word = 1U;
        fixture.input_dispatch.sample_mix_level = 6;
        const auto fallback = run(fixture);
        test.expect_true(
            fixture.message == 0x66U &&
                fixture.target_selection.transition_timer == 0U &&
                fallback.sample_calls == 1U,
            "message 113 falls back to 102 and samples only when the transition word is nonzero"
        );

        Fixture selected;
        selected.message = 0x71U;
        selected.target_selection.transition_actor_index = 0xFFU;
        selected.port.reply(
            LegacyBattleMessagePhaseCall::select_message_113_actor,
            {
                .publish_transition_actor_index = true,
                .transition_actor_index = 3U,
            }
        );
        selected.port.reply(
            LegacyBattleMessagePhaseCall::query_actor_completion, {.eax = 1U}
        );
        const auto advanced = run(selected);
        test.expect_true(
            advanced.sample_calls == 1U &&
                selected.port.count(
                    LegacyBattleMessagePhaseCall::advance_message_113
                ) == 1U &&
                selected.target_selection.transition_timer == 1U,
            "message 113 samples and advances a selected actor without allocating when its query is nonzero"
        );
    }

    {
        Fixture zero;
        zero.message = 0x66U;
        zero.target_selection.transition_sample_word = 0U;
        const auto completed = run(zero);
        test.expect_true(
            zero.target_selection.completion_gate == 1U &&
                completed.target_selection_entry_calls == 0U,
            "message 102 completes immediately when its sample word is zero"
        );

        Fixture timed;
        timed.message = 0x66U;
        timed.target_selection.transition_sample_word = 1U;
        timed.target_selection.transition_timer = 149U;
        timed.input_dispatch.retreat_block_word = 1U;
        const auto advanced = run(timed);
        test.expect_true(
            advanced.target_selection_entry_calls == 1U &&
                timed.target_selection.transition_timer == 150U &&
                timed.port.count(
                    LegacyBattleMessagePhaseCall::advance_message_102
                ) == 1U,
            "message 102 enters target selection at signed timer 150 then always advances its frame"
        );

        Fixture debug;
        debug.message = 0x67U;
        debug.debug_hotkeys.battle_mode_flags_53bc24 = 8U;
        debug.target_selection.transition_timer = 29U;
        const auto debug_result = run(debug);
        test.expect_true(
            debug_result.port_calls == 0U && debug_result.return_ecx == 1U &&
                debug.target_selection.transition_timer == 0U &&
                debug.target_selection.completion_gate == 1U &&
                debug.input_dispatch.selection_cache_gate_a == 1U &&
                debug.input_dispatch.selection_cache_gate_b == 1U,
            "message 103 debug bit completes at signed timer 30 without the normal frame"
        );

        Fixture message_104;
        message_104.message = 0x68U;
        message_104.target_selection.transition_timer = 20U;
        message_104.input_dispatch.retreat_block_word = 1U;
        const auto over = run(message_104);
        test.expect_true(
            over.target_selection_entry_calls == 1U &&
                message_104.target_selection.transition_timer == 21U &&
                message_104.input_dispatch.selection_cache_gate_a == 1U &&
                message_104.input_dispatch.selection_cache_gate_b == 1U,
            "message 104 enters target selection only after signed timer exceeds twenty"
        );

        Fixture stopped;
        stopped.message = 0x68U;
        stopped.target_selection.transition_timer = 20U;
        stopped.target_ready_gate = 1U;
        stopped.final_actor.queued_actor_code = 7U;
        stopped.input_dispatch.selected_option_word = 0U;
        const auto stopped_result = run(stopped);
        test.expect_true(
            stopped_result.status ==
                    openswd3::battle::LegacyBattleMessagePhaseStatus::
                        target_selection_entry_typed_stop &&
                stopped_result.target_selection_entry.status ==
                    openswd3::battle::LegacyBattleTargetSelectionEntryStatus::
                        active_group_a_actor_typed_stop &&
                stopped.input_dispatch.selection_cache_gate_a == 1U &&
                stopped.input_dispatch.selection_cache_gate_b == 1U,
            "message 104 propagates target-selection actor failure after preserving both cache writes"
        );
    }
}
