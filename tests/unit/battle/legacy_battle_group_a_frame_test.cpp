#include "openswd3/battle/legacy_battle_group_a_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        if (request.callee_token == 0x004786B0U) {
            return {.eax = action};
        }
        if (request.callee_token == 0x004786E0U) {
            return {.eax = action_target};
        }
        return default_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageCallReply
    invoke_text_message(
        const openswd3::battle::LegacyBattleTextMessageCallRequest& request
    ) override {
        text_message_calls.push_back(request);
        if (request.call ==
            openswd3::battle::LegacyBattleTextMessageCall::allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token};
        }
        return {.eax = 4U};
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleActionCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    u16 action{};
    u16 action_target{};
    LegacyBattleActionCallReply default_reply{.eax = 1U};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<openswd3::battle::LegacyBattleTextMessageCallRequest>
        text_message_calls;
    u32 next_text_message_token{0x72000000U};
};

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece&
    ) noexcept override {
        return false;
    }
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        return value;
    }

    u32 value{};
    std::vector<u32> bounds;
};

class SoundPort final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(u16, u16) override {}
};

class CountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagPorts {
public:
    [[nodiscard]] bool query_internal_flag(u32) noexcept override {
        return false;
    }
    void set_internal_flag(u32) noexcept override {}
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    ActionStreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    CountdownFlags countdown_flags;
    std::array<u8, 16> flags{};
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleStartupResetBlocks startup_reset;
    openswd3::battle::LegacyBattleTextMessageState text_messages;
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};
    std::array<u32, 0x32> attack_order_party_sources{};
    u32 attack_order_primary_gate{};
    u32 attack_order_secondary_gate{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};
    openswd3::battle::LegacyBattleActionDispatchState shared_action;
    openswd3::battle::LegacyBattleFinalActorStepState shared_final_actor;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target_runtime;

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
        for (auto& actor : startup.party) {
            actor.configuration.source_record_token = 0x004AB790U;
            actor.configuration.actor_record_token = 0x005029D0U;
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionDispatchContext
    context() {
        return {
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = flags,
            .startup = &startup,
            .startup_reset = &startup_reset,
            .text_messages = &text_messages,
            .attack_order_records = attack_order_records,
            .attack_order_party_sources = attack_order_party_sources,
            .attack_order_primary_gate = &attack_order_primary_gate,
            .attack_order_secondary_gate = &attack_order_secondary_gate,
            .attack_order_adjacent_record = &attack_order_adjacent_record,
            .status_indicator_action_eax_snapshot = 0U,
            .shared_action_dispatch = &shared_action,
            .shared_final_actor = &shared_final_actor,
            .target_selection_runtime = &target_runtime,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
            .scripted_resource_release_test_compat = true,
        };
    }
};

[[nodiscard]] bool has_call_argument(
    const DispatchPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleActionCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_group_a_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::LegacyBattleGroupAFrameState;
    using openswd3::battle::LegacyBattleTurnAdvanceStatus;

    {
        LegacyBattleGroupAFrameState state;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 10U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                result.port_calls == 0U,
            "group A frame stops at first actor object query"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.actor_enabled[0U] = 1U;
        state.actors[0U].mode_gate = 1U;
        state.selected_actor_one_based = 2U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        context.actor_turn_completion_request.access.latch_readable = false;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        const u32 selected =
            openswd3::battle::kLegacyBattleActionGroupABaseToken +
            openswd3::battle::kLegacyBattleActionGroupAStride;
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_turn_completion_typed_stop &&
                result.actor_turn_completion_calls == 1U &&
                result.actor_turn_completion.status ==
                    openswd3::battle::LegacyBattleActorTurnCompletionStatus::
                        latch_read_typed_stop &&
                result.actor_turn_completion.return_eax == 0x7DEU &&
                result.actor_turn_completion.return_ecx == selected &&
                result.actor_turn_completion.return_edx == 0x179AU &&
                result.actor_turn_completion.return_eip == 0x00478690U &&
                result.actor_turn_completion.field_token ==
                    selected + 0x2AACU &&
                result.actor_turn_completion.flags_known &&
                !result.actor_turn_completion.flags.carry &&
                result.actor_turn_completion.flags.parity &&
                result.actor_turn_completion.flags.auxiliary_carry_defined &&
                result.actor_turn_completion.flags.auxiliary_carry &&
                !result.actor_turn_completion.flags.zero &&
                !result.actor_turn_completion.flags.sign &&
                !result.actor_turn_completion.flags.overflow &&
                result.actor_turn_completion.return_esp ==
                    context.actor_turn_completion_request.entry_esp &&
                result.return_value == 0x7DEU &&
                port.count(0x00478690U) == 0U && port.count(0x004786A0U) == 0U,
            "one-based Group-A target query propagates its typed field stop before the post-call branch"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.active_effect_target = 8U;
        state.final_actor_step.action_execution_active = 0U;
        Fixture fixture;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        DispatchPort port;
        port.push(0x004786E0U, {.eax = 2U, .edx = 0xA5A55A5AU});
        auto context = fixture.context();
        context.actor_turn_completion_request.access.latch_readable = false;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        const u32 target =
            openswd3::battle::kLegacyBattleActionGroupBBaseToken +
            2U * openswd3::battle::kLegacyBattleActionGroupBStride;
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_turn_completion_typed_stop &&
                result.actor_turn_completion_calls == 1U &&
                result.actor_turn_completion.status ==
                    openswd3::battle::LegacyBattleActorTurnCompletionStatus::
                        latch_read_typed_stop &&
                result.actor_turn_completion.return_eax == 0xACAU &&
                result.actor_turn_completion.return_ecx == target &&
                result.actor_turn_completion.return_edx == 0xA5A55A5AU &&
                result.actor_turn_completion.return_eip == 0x00478690U &&
                result.actor_turn_completion.field_token == target + 0x2AACU &&
                result.actor_turn_completion.flags_known &&
                !result.actor_turn_completion.flags.carry &&
                result.actor_turn_completion.flags.parity &&
                result.actor_turn_completion.flags.auxiliary_carry_defined &&
                result.actor_turn_completion.flags.auxiliary_carry &&
                !result.actor_turn_completion.flags.zero &&
                !result.actor_turn_completion.flags.sign &&
                !result.actor_turn_completion.flags.overflow &&
                result.actor_turn_completion.return_esp ==
                    context.actor_turn_completion_request.entry_esp &&
                result.return_value == 0xACAU &&
                port.count(0x00478690U) == 0U && port.count(0x00478B40U) == 0U,
            "active Group-A actor resolves the Group-B lifecycle latch and stops before action-start suffixes"
        );
    }

    {
        LegacyBattleGroupAFrameState zero_state;
        zero_state.actor_enabled[0U] = 1U;
        zero_state.actors[0U].mode_gate = 1U;
        zero_state.selected_actor_one_based = 2U;
        zero_state.action.group_a_action_execution[1U].turn_completion_latch =
            0U;
        Fixture zero_fixture;
        DispatchPort zero_port;
        auto zero_context = zero_fixture.context();
        const auto zero = openswd3::battle::advance_legacy_battle_group_a_frame(
            zero_state, zero_port, zero_context, 0U
        );

        LegacyBattleGroupAFrameState nonzero_state;
        nonzero_state.actor_enabled[0U] = 1U;
        nonzero_state.actors[0U].mode_gate = 1U;
        nonzero_state.selected_actor_one_based = 2U;
        nonzero_state.action.group_a_action_execution[1U]
            .turn_completion_latch = 7U;
        Fixture nonzero_fixture;
        DispatchPort nonzero_port;
        auto nonzero_context = nonzero_fixture.context();
        const auto nonzero =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                nonzero_state, nonzero_port, nonzero_context, 0U
            );

        test.expect_true(
            zero.actor_turn_completion_calls == 1U &&
                zero.actor_turn_completion.returned &&
                zero.actor_turn_completion.return_eax == 0U &&
                zero.actor_turn_completion.return_eip == 0x00456DDBU &&
                zero_port.count(0x004786A0U) == 1U &&
                nonzero.actor_turn_completion_calls == 1U &&
                nonzero.actor_turn_completion.returned &&
                nonzero.actor_turn_completion.return_eax == 7U &&
                nonzero.actor_turn_completion.return_eip == 0x00456DDBU &&
                nonzero_port.count(0x004786A0U) == 0U &&
                zero_port.count(0x00478690U) == 0U &&
                nonzero_port.count(0x00478690U) == 0U,
            "one-based Group-A caller TEST enters the idle suffix only for a zero canonical latch"
        );
    }

    {
        LegacyBattleGroupAFrameState zero_state;
        zero_state.action.active_effect_target = 8U;
        zero_state.action_side = 1U;
        Fixture zero_fixture;
        zero_fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        (*zero_fixture.startup.group_b_lifecycle)[2U]
            .action_execution.turn_completion_latch = 0U;
        DispatchPort zero_port;
        zero_port.push(0x004786E0U, {.eax = 2U});
        auto zero_context = zero_fixture.context();
        const auto zero = openswd3::battle::advance_legacy_battle_group_a_frame(
            zero_state, zero_port, zero_context, 0U
        );

        LegacyBattleGroupAFrameState nonzero_state;
        nonzero_state.action.active_effect_target = 8U;
        nonzero_state.action_side = 1U;
        Fixture nonzero_fixture;
        nonzero_fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        (*nonzero_fixture.startup.group_b_lifecycle)[2U]
            .action_execution.turn_completion_latch = 9U;
        DispatchPort nonzero_port;
        nonzero_port.push(0x004786E0U, {.eax = 2U});
        auto nonzero_context = nonzero_fixture.context();
        const auto nonzero =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                nonzero_state, nonzero_port, nonzero_context, 0U
            );

        test.expect_true(
            zero.actor_turn_completion_calls == 1U &&
                zero.actor_turn_completion.returned &&
                zero.actor_turn_completion.return_eax == 0U &&
                zero.actor_turn_completion.return_eip == 0x00456EF0U,
            "Group-B target caller returns the zero lifecycle latch"
        );
        test.expect_true(
            zero_port.count(0x00478B40U) == 1U,
            "zero lifecycle latch queries selection completion"
        );
        test.expect_true(
            zero.group_a_actor_list_action_calls == 1U,
            "zero lifecycle latch begins the typed current-actor list action"
        );
        test.expect_true(
            nonzero.actor_turn_completion_calls == 1U &&
                nonzero.actor_turn_completion.returned &&
                nonzero.actor_turn_completion.return_eax == 9U &&
                nonzero.actor_turn_completion.return_eip == 0x00456EF0U,
            "Group-B target caller returns the nonzero lifecycle latch"
        );
        test.expect_true(
            nonzero_port.count(0x00478B40U) == 0U &&
                nonzero.group_a_actor_list_action_calls == 0U,
            "nonzero lifecycle latch skips the Group-B action preparation suffix"
        );
        test.expect_true(
            zero_port.count(0x00478690U) == 0U &&
                nonzero_port.count(0x00478690U) == 0U,
            "Group-B target caller uses no generic turn-completion call"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.frame_effect.primary_suppression = 1U;
        Fixture fixture;
        DispatchPort port;
        port.actor_metric_state().pending_action_activation_latch = 9U;
        port.push(0x004786D0U, {.eax = 1U});
        port.push(0x00479850U, {.eax = 1U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                has_call_argument(port, 0x00478B60U, 1U, 1U) &&
                has_call_argument(port, 0x00479850U, 0U, 0x005029D0U) &&
                port.actor_metric_state().pending_action_activation_latch ==
                    0U &&
                state.final_selected_word == 0xFFFFU,
            "group A frame directly composes the final actor step"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0] = 1U;
        state.action.group_b_count = 2;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 1U, .edx = 0x11112222U});
        port.push(0x0047CE80U, {.eax = 0U, .edx = 0x33334444U});
        port.push(0x00439070U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                result.group_a_attribute_effect_calls == 1U &&
                result.group_a_attribute_effect.status ==
                    openswd3::battle::LegacyBattleGroupAAttributeEffectStatus::
                        completed &&
                port.count(0x0046EE60U) == 0U &&
                state.selected_opponent_one_based == 1U &&
                state.final_actor_step.selection_gate == 1U &&
                state.actors[0].special_ready == 1U &&
                state.actors[0].action_complete == 1U &&
                state.actors[0].update_ready == 1U &&
                port.count(0x0046E520U) == 0U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                result.actor_availability_block.return_edx == 0x33334444U &&
                state.final_actor_step.group_a_availability_blocks[0U].value ==
                    1U &&
                port.count(0x00439070U) == 1U && port.count(0x0047CE80U) >= 3U,
            "AI coordination counts terminals and retries one based target until live"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0U] = 1U;
        state.action.group_b_count = 1;
        state.final_actor_step.group_a_availability_blocks[0U]
            .write_accessible = false;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U, .edx = 0x55667788U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0x55667788U &&
                result.return_value == 1U && port.count(0x0047CE80U) == 1U &&
                port.count(0x00439070U) == 0U &&
                state.selected_opponent_one_based == 1U &&
                state.final_actor_step.selection_gate == 0U,
            "AI typed write stop preserves the last terminal-query EDX and suppresses the random-selection suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0U] = 1U;
        Fixture fixture;
        fixture.startup.party[0U].workspace.tail_words[7U] = 200U;
        fixture.startup.group_a_configuration_sources[0U].dwords[2U] = 25U
            << 16U;
        DispatchPort port;
        auto context = fixture.context();

        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.group_a_attribute_effect_calls == 1U &&
                result.group_a_attribute_effect.active_channels == 1U &&
                result.group_a_attribute_effect.computed_words[0U] == 0xFFCEU &&
                fixture.startup.party[0U]
                        .attribute_effect.temporary_values[0U] == 0U &&
                port.count(0x0046EE60U) == 0U &&
                port.count(0x0047F150U) == 1U &&
                port.count(0x004787D0U) == 1U &&
                port.count(0x0047D640U) == 1U &&
                port.count(0x0047CF00U) == 1U &&
                port.count(0x0047CEC0U) == 1U &&
                has_call_argument(port, 0x0047F150U, 0U, 0xFFFFFFCEU) &&
                has_call_argument(port, 0x004787D0U, 0U, 0x246FU) &&
                has_call_argument(port, 0x0047D640U, 0U, 0xFFFFFFCEU) &&
                has_call_argument(port, 0x0047CF00U, 0U, 0U) &&
                has_call_argument(port, 0x0047CEC0U, 0U, 1U),
            "completed progress directly applies the shared group-A attribute channel before the AI suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        state.actor_ai_primary[0U] = 1U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        context.startup = nullptr;

        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_attribute_effect_typed_stop &&
                result.return_value == 1U &&
                result.group_a_attribute_effect_calls == 0U &&
                state.actors[0U].action_complete == 1U &&
                state.actors[0U].update_ready == 1U &&
                port.count(0x0046EE60U) == 0U && port.count(0x0047F150U) == 0U,
            "missing startup actor owner stops after progress completion and before the reclaimed attribute call"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.final_actor_step.actor_order[0] = 9U;
        state.final_actor_step.actor_order[1] = 10U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                state.final_actor_step.queued_actor_code == 9U &&
                state.final_actor_step.actor_order[0] == 10U &&
                state.final_actor_step.actor_order[1] == 0U,
            "actor queue publishes first unfinished entry then shifts fixed tail left"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.return_value == 1U && state.actors[2].frame_started == 1U &&
                state.final_actor_step.active_actor_code == 0xFFFFFFFFU &&
                result.attack_order_insert_calls == 1U &&
                result.attack_order_insert.record_written &&
                result.attack_order_remove_calls == 1U &&
                result.attack_order_remove.matched &&
                fixture.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                fixture.attack_order_records[0].value_08 == 0U &&
                port.count(0x0045EE70U) == 0U && port.count(0x0045EFB0U) == 0U,
            "started actor is registered then removed directly by the composed final actor suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.actor_target_preparation_calls == 1U &&
                fixture.shared_action.opponent_workspace[12U] == 1U &&
                fixture.shared_final_actor.published_actor_code == 0U &&
                fixture.target_runtime.selected_action_kind == 1U &&
                fixture.target_runtime.actor_commit_gate == 1U &&
                port.battle_debug_hotkey_state().committed_actor_code == 10U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                fixture.shared_final_actor.group_a_availability_blocks[2U]
                        .value == 1U &&
                port.count(0x00478330U) == 0U && port.count(0x00464CC0U) == 0U,
            "group-A queue caller directly prepares the shared actor target through the typed owner"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        Fixture fixture;
        fixture.shared_final_actor.group_a_availability_blocks[2U]
            .write_accessible = false;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_target_preparation_calls == 1U &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 0U &&
                result.return_value == 1U &&
                result.actor_availability_block.return_ecx == 0x00508838U &&
                result.actor_availability_block.return_edx == 10U &&
                fixture.shared_action.opponent_workspace[12U] == 1U &&
                port.battle_debug_hotkey_state().committed_actor_code == 10U &&
                fixture.target_runtime.selected_action_kind == 1U &&
                fixture.target_runtime.actor_commit_gate == 1U &&
                fixture.shared_final_actor.published_actor_code == 0U,
            "group-A queue typed write stop preserves target preparation and suppresses every nested suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.ai_coordination_enabled = 1U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        context.target_selection_runtime = nullptr;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_target_preparation_typed_stop &&
                result.actor_target_preparation_calls == 0U &&
                port.count(0x00464CC0U) == 0U,
            "group-A queue caller stops at the reclaimed boundary when the shared target owner is unavailable"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        auto context = fixture.context();
        context.attack_order_party_sources = {};

        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 2U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        attack_order_insert_typed_stop &&
                state.actors[2].frame_started == 1U &&
                state.final_actor_step.active_actor_code == 10U &&
                fixture.attack_order_records[0].value_00 == 10U &&
                fixture.attack_order_records[0].value_08 == 1U &&
                result.return_value == 0U,
            "attack-order source stop preserves actor start and record prefix then blocks the final actor suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.actor_enabled[0] = 1U;
        state.actors[0].action_complete = 1U;
        state.action.group_b_count = 2;
        state.action.group_a_to_actor[0] = 0xFFFFFFFFU;
        state.action.group_a_to_actor[1] = 0xFFFFFFFFU;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.actors[0].progress == 2U &&
                port.count(0x00478B30U) == 1U &&
                has_call_argument(port, 0x00478A70U, 0U, 0U),
            "completed actor scans unmapped live opponents and selects first live index"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.active_effect_target = 8U;
        state.final_actor_step.action_execution_active = 1U;
        state.action.group_a_count = 0;
        state.action.group_b_count = 0;
        state.action.selected_target_index = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 5U;
        port.action_target = 0U;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U && port.count(0x004539B0U) == 0U &&
                port.count(0x004786B0U) == 1U &&
                has_call_argument(port, 0x00478850U, 0U, 0x00525508U) &&
                state.final_actor_step.action_execution_active == 0U &&
                state.action.active_effect_target == 0xFFFFFFFFU &&
                state.shared_gate_4ff578 == 1U &&
                state.shared_gate_4ff57c == 1U &&
                state.shared_gate_4ff580 == 1U &&
                state.shared_gate_4ff584 == 1U,
            "active actor directly composes action dispatch and post-action cleanup suffixes"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.action.active_effect_target = 8U;
        state.final_actor_step.action_execution_active = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action_target = 0xFFFFU;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop &&
                port.count(0x00478690U) == 0U,
            "action preparation stops when queried target first forms invalid group B object"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAConfigurationState first_actor;
        first_actor.actor_record_token = 0x005029D0U;
        first_actor.actor_record[11U] = 20U;
        RandomPort random;
        const auto zero =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                nullptr, random, {.candidate = 0U}
            );
        random.value = 35U;
        const auto below_true =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 21U}
            );
        random.value = 36U;
        const auto below_false =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 21U}
            );
        const auto equal =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 20U}
            );
        random.value = 70U;
        const auto near =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 19U}
            );
        random.value = 90U;
        const auto middle =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 12U}
            );
        const auto far =
            openswd3::battle::evaluate_legacy_battle_turn_commit_chance(
                &first_actor, random, {.candidate = 7U}
            );
        test.expect_true(
            zero.return_eax == 0U && zero.random_calls == 0U &&
                below_true.return_eax == 1U && below_true.difference == -1 &&
                below_false.return_eax == 0U && equal.return_eax == 0U &&
                equal.random_calls == 0U && near.return_eax == 1U &&
                near.difference == 1 && middle.return_eax == 1U &&
                middle.difference == 8 && far.return_eax == 1U &&
                far.difference == 13 && far.random_calls == 0U &&
                random.bounds == std::vector<u32>({100U, 100U, 100U, 100U}),
            "turn commit chance preserves zero, equal, three inclusive random bands and deterministic far success"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_completion_latch = 9U;
        progress.special_ready = 1U;
        DispatchPort port;
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .entry_eax = 0x11111111U,
             .entry_ecx = 0x005029D0U,
             .entry_edx = 0x22222222U}
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x22222222U &&
                actor.turn_completion_latch == 0U && result.port_calls == 0U,
            "turn gate special-ready path clears the latch before returning without any call"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_countdown = 7;
        DispatchPort port;
        port.push(
            0x0047F920U, {.eax = 1U, .ecx = 0x33333333U, .edx = 0x44444444U}
        );
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U, .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x33333333U &&
                result.return_edx == 0x44444444U &&
                actor.turn_threshold == 2U && actor.turn_countdown == 6 &&
                result.queue_completion_calls == 1U && result.port_calls == 1U,
            "turn gate decrements the signed countdown while queue completion remains above the mode-zero threshold"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_countdown = 6;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 1U});
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.return_eax == 1U && actor.turn_threshold == 6U &&
                actor.turn_countdown == 15 && actor.turn_completion_latch == 0U,
            "turn gate resets the countdown to fifteen at the inclusive mode-one completion threshold"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_action_record.action_id = 0xFFFFFFFFU;
        actor.turn_action_record.field_94 = 0xFFFFFFFFU;
        actor.turn_completion_latch = 9U;
        actor.turn_countdown = 2;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        const auto first = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U, .entry_ecx = 0x005029D0U}
        );
        const u32 first_latch = actor.turn_completion_latch;
        actor.turn_countdown = 6;
        actor.turn_completion_latch = 9U;
        port.push(0x0047F920U, {.eax = 0U});
        const auto second = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            first.return_eax == 1U && first.action_record_clears == 1U &&
                first_latch == 9U && actor.turn_action_record.action_id == 0U &&
                actor.turn_action_record.field_94 == 0U &&
                second.return_eax == 1U && second.action_record_clears == 1U &&
                actor.turn_completion_latch == 1U && actor.turn_countdown == 15,
            "turn gate clears exactly the action record and only mode one sets the completion latch"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x1234U;
        actor.special_mode = 1U;
        actor.turn_countdown = 7;
        actor.turn_action_record.field_4a = 0x1122U;
        actor.turn_action_record.field_4c = 0x3344U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(
            0x004321E0U, {.eax = 0U, .ecx = 0xABCDEF01U, .edx = 0x12345678U}
        );
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0xABCDEF01U &&
                result.return_edx == 0x12345678U &&
                actor.turn_completion_latch == 1U &&
                actor.turn_action_record.action_id == 0x1234U &&
                actor.turn_action_record.base_variant == 0x2AU &&
                actor.turn_action_record.external_mode == 1U &&
                has_call_argument(port, 0x004321E0U, 0U, 0x00502E38U) &&
                port.count(0x004315D0U) == 0U,
            "turn gate preserves the initialized prefix and returns one when the action updater returns zero"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x55AAU;
        actor.turn_countdown = 15;
        actor.position_x = 200U;
        actor.position_y = 300U;
        actor.turn_action_record.draw_offset_x = 3U;
        actor.turn_action_record.draw_offset_y = 5U;
        actor.turn_action_record.mode_flags = 4U;
        actor.turn_action_record.field_4a = 0x1122U;
        actor.turn_action_record.field_4c = 0x3344U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(
            0x004321E0U,
            {.eax = 0xAAAA0001U, .ecx = 0xBBBB0002U, .edx = 0xCCCC0003U}
        );
        LegacyBattleActionCallReply frame{
            .eax = 0x70000000U,
            .ecx = 0xDDDD0004U,
            .edx = 0xEEEE0005U,
        };
        frame.outputs = {0x71000000U, 40U, 20U, 0x72000000U};
        port.push(0x004315D0U, frame);
        port.push(
            0x00485610U,
            {.eax = 0x11110000U, .ecx = 0x22220000U, .edx = 0x33330000U}
        );
        port.push(
            0x00485650U,
            {.eax = 0x44440000U, .ecx = 0x55550000U, .edx = 0x66660000U}
        );
        port.push(0x004170E0U, {.edx = 0x88880000U});
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .sample_handle = 0x12345678U,
             .coordinate_output_x_token = 0xABCD0100U,
             .coordinate_output_y_token = 0xDCBA0200U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status == LegacyBattleTurnAdvanceStatus::completed &&
                result.return_eax == 0U && result.return_edx == 0x88880000U &&
                actor.turn_countdown == 14 && actor.turn_render_flags == 5U &&
                actor.turn_target_x_offset == 3U &&
                actor.turn_sample_word == 0U &&
                shared.turn_frame_source_token == 0x71000000U &&
                has_call_argument(port, 0x004315D0U, 0U, 0xAAAA1122U) &&
                has_call_argument(port, 0x004315D0U, 1U, 0xCCCC3344U) &&
                has_call_argument(port, 0x00485650U, 0U, 0x2222002FU) &&
                has_call_argument(port, 0x00485650U, 1U, 0x10U) &&
                actor.position_x == 216U && actor.position_y == 300U &&
                actor.alternate_position_x == 216U &&
                actor.alternate_position_y == 300U &&
                result.coordinate_query_calls == 1U &&
                result.current_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorCurrentCoordinateQueryStatus::
                            completed &&
                result.current_coordinate_query.return_eax == 0xDCBA012CU &&
                result.current_coordinate_query.return_ecx == 0xDCBA0200U &&
                result.current_coordinate_query.return_edx == 0xABCD0100U &&
                !result.current_coordinate_query.flags.carry &&
                result.current_coordinate_query.flags.parity &&
                result.current_coordinate_query.flags.auxiliary_carry &&
                result.current_coordinate_query.flags.auxiliary_carry_defined &&
                !result.current_coordinate_query.flags.zero &&
                result.current_coordinate_query.flags.sign &&
                !result.current_coordinate_query.flags.overflow &&
                result.coordinate_publish_calls == 1U &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            completed &&
                result.coordinate_publication.argument_x == 216U &&
                result.coordinate_publication.argument_y == 300U &&
                result.coordinate_publication.return_eax == 216U &&
                result.coordinate_publication.return_ecx == 0U &&
                result.coordinate_publication.return_edx == 300U &&
                result.coordinate_publication.return_esi == 0x005029D0U &&
                result.coordinate_publication.return_edi == 0U &&
                !result.coordinate_publication.flags.carry &&
                result.coordinate_publication.flags.parity &&
                !result.coordinate_publication.flags.auxiliary_carry &&
                !result.coordinate_publication.flags.zero &&
                !result.coordinate_publication.flags.sign &&
                !result.coordinate_publication.flags.overflow &&
                port.count(0x004785C0U) == 0U &&
                has_call_argument(port, 0x004170E0U, 0U, 213U) &&
                has_call_argument(port, 0x004170E0U, 1U, 295U) &&
                has_call_argument(port, 0x004170E0U, 2U, 40U) &&
                has_call_argument(port, 0x004170E0U, 3U, 20U) &&
                has_call_argument(port, 0x004170E0U, 4U, 5U) &&
                has_call_argument(port, 0x004170E0U, 5U, 0x72000000U) &&
                port.count(0x00478600U) == 0U && result.port_calls == 6U,
            "turn gate directly publishes shifted coordinates before rendering"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x55AAU;
        actor.turn_countdown = 7;
        actor.turn_action_record.field_4a = 1U;
        actor.turn_action_record.field_4c = 2U;
        actor.alternate_position_x = 0x1111U;
        actor.alternate_position_y = 0x2222U;
        actor.publication_destination_dword_write_accessible[6U] = false;
        progress.post_action_value = 1U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x004315D0U, {.eax = 0x70000000U});
        actor.position_x = 8U;
        actor.position_y = 0xCCDDU;
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .coordinate_y_initial = 0xAABB0000U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTurnAdvanceStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            destination_dword_write_typed_stop &&
                result.coordinate_publication.stopped_dword_index == 6U &&
                result.coordinate_publication.source_dword_reads == 7U &&
                result.coordinate_publication.destination_dword_writes == 6U &&
                result.coordinate_publication.return_eax == 0xFFFFFFF8U &&
                result.coordinate_publication.return_ecx == 2U &&
                result.coordinate_publication.return_edx == 0xAABBCCDDU &&
                result.coordinate_publication.flags.carry &&
                !result.coordinate_publication.flags.parity &&
                !result.coordinate_publication.flags.auxiliary_carry &&
                !result.coordinate_publication.flags.zero &&
                result.coordinate_publication.flags.sign &&
                !result.coordinate_publication.flags.overflow &&
                actor.position_x == 0xFFF8U && actor.position_y == 0xCCDDU &&
                actor.alternate_position_x == 0xFFF8U &&
                actor.alternate_position_y == 0x2222U &&
                result.render_calls == 0U && actor.turn_countdown == 7 &&
                shared.turn_frame_source_token == 0U &&
                port.count(0x004785C0U) == 0U && port.count(0x004170E0U) == 0U,
            "turn gate publication fault preserves the copied prefix and suppresses the frame suffix"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x55AAU;
        actor.turn_countdown = 7;
        actor.turn_action_record.field_4a = 1U;
        actor.turn_action_record.field_4c = 2U;
        actor.position_x = 0x1111U;
        actor.position_y = 0x2222U;
        actor.alternate_position_x = 0x3333U;
        actor.alternate_position_y = 0x4444U;
        actor.position_x_write_accessible = false;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x004315D0U, {.eax = 0x70000000U});

        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 1U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTurnAdvanceStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            position_x_write_typed_stop &&
                result.coordinate_publication.coordinate_writes == 0U &&
                result.coordinate_publication.source_dword_reads == 0U &&
                result.coordinate_publication.destination_dword_writes == 0U &&
                actor.position_x == 0x1111U && actor.position_y == 0x2222U &&
                actor.alternate_position_x == 0x3333U &&
                actor.alternate_position_y == 0x4444U &&
                result.render_calls == 0U && actor.turn_countdown == 7 &&
                shared.turn_frame_source_token == 0U &&
                port.count(0x004170E0U) == 0U,
            "turn gate X publication stop preserves all actor coordinates and suppresses rendering"
        );
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.profile_value = 0x55AAU;
        actor.turn_countdown = 7;
        actor.turn_action_record.field_4a = 1U;
        actor.turn_action_record.field_4c = 2U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x004315D0U, {.eax = 0x70000000U});
        actor.position_x = 0x5678U;
        actor.position_y = 0x4321U;
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            nullptr,
            &progress,
            port,
            {.actor_token = 0x005029D0U,
             .argument = 0U,
             .coordinate_y_initial = 0x87650000U,
             .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTurnAdvanceStatus::shared_state_typed_stop &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            completed &&
                result.coordinate_publication.argument_x == 0x5678U &&
                result.coordinate_publication.argument_y == 0x4321U &&
                result.coordinate_publication.flags.carry &&
                result.coordinate_publication.flags.parity &&
                result.coordinate_publication.flags.auxiliary_carry &&
                !result.coordinate_publication.flags.zero &&
                result.coordinate_publication.flags.sign &&
                !result.coordinate_publication.flags.overflow &&
                actor.position_x == 0x5678U && actor.position_y == 0x4321U &&
                result.render_calls == 0U && actor.turn_countdown == 7 &&
                port.count(0x004785C0U) == 0U,
            "turn gate no-adjust branch preserves the secondary-index compare flags"
        );
    }

    {
        using QueryStatus =
            openswd3::battle::LegacyBattleActorCurrentCoordinateQueryStatus;
        const std::array expected_statuses{
            QueryStatus::first_output_pointer_read_typed_stop,
            QueryStatus::position_x_read_typed_stop,
            QueryStatus::first_output_write_typed_stop,
            QueryStatus::position_y_read_typed_stop,
            QueryStatus::second_output_pointer_read_typed_stop,
            QueryStatus::second_output_write_typed_stop,
        };
        for (std::size_t stage = 0U; stage < expected_statuses.size();
             ++stage) {
            openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
            openswd3::battle::LegacyBattleGroupAActionExecutionSharedState
                shared;
            openswd3::battle::LegacyBattleActorProgressState progress;
            actor.profile_value = 0x55AAU;
            actor.turn_countdown = 7;
            actor.turn_action_record.field_4a = 1U;
            actor.turn_action_record.field_4c = 2U;
            actor.position_x = 100U;
            actor.position_y = 200U;
            if (stage == 1U) {
                actor.position_x_read_accessible = false;
            }
            if (stage == 3U) {
                actor.position_y_read_accessible = false;
            }
            DispatchPort port;
            port.push(0x0047F920U, {.eax = 0U});
            port.push(0x004321E0U, {.eax = 1U});
            port.push(
                0x004315D0U,
                {.eax = 0x70000000U, .ecx = 0xBBBB0002U, .edx = 0xCCCC0003U}
            );
            openswd3::battle::LegacyBattleTurnAdvanceRequest request{
                .actor_token = 0x005029D0U,
                .argument = 0U,
                .coordinate_output_x_token = 0xAAAA0100U,
                .coordinate_output_y_token = 0xBBBB0200U,
                .coordinate_y_initial = 0xAABB0000U,
                .entry_ecx = 0x005029D0U,
            };
            if (stage == 0U) {
                request.current_coordinate_access
                    .first_output_pointer_readable = false;
            } else if (stage == 2U) {
                request.current_coordinate_access.first_output_writable = false;
            } else if (stage == 4U) {
                request.current_coordinate_access
                    .second_output_pointer_readable = false;
            } else if (stage == 5U) {
                request.current_coordinate_access.second_output_writable =
                    false;
            }
            const auto result =
                openswd3::battle::advance_legacy_battle_turn_gate(
                    &actor, &shared, &progress, port, request
                );
            const u32 expected_eax = stage < 2U
                ? 0xBBBB0200U
                : (stage < 4U ? 0xBBBB0064U : 0xBBBB00C8U);
            const u32 expected_ecx = stage < 5U ? 0x005029D0U : 0xBBBB0200U;
            const u32 expected_edx = stage == 0U ? 0xCCCC0003U : 0xAAAA0100U;
            test.expect_true(
                result.status ==
                        LegacyBattleTurnAdvanceStatus::
                            actor_current_coordinate_typed_stop &&
                    result.current_coordinate_query.status ==
                        expected_statuses[stage] &&
                    result.current_coordinate_query.output_writes ==
                        (stage >= 3U ? 1U : 0U) &&
                    result.current_coordinate_query.return_eax ==
                        expected_eax &&
                    result.current_coordinate_query.return_ecx ==
                        expected_ecx &&
                    result.current_coordinate_query.return_edx ==
                        expected_edx &&
                    result.current_coordinate_query.flags.carry &&
                    !result.current_coordinate_query.flags.parity &&
                    result.current_coordinate_query.flags.auxiliary_carry &&
                    !result.current_coordinate_query.flags.zero &&
                    result.current_coordinate_query.flags.sign &&
                    !result.current_coordinate_query.flags.overflow &&
                    result.coordinate_x == (stage >= 3U ? 100U : 0U) &&
                    result.coordinate_y == 0xAABB0000U &&
                    result.coordinate_publish_calls == 0U &&
                    result.render_calls == 0U && actor.turn_countdown == 7 &&
                    shared.turn_frame_source_token == 0U &&
                    port.count(0x00478600U) == 0U &&
                    port.count(0x004785C0U) == 0U &&
                    port.count(0x004170E0U) == 0U && result.port_calls == 3U,
                "turn gate current-coordinate stop preserves exact registers, flags and stack-local prefix"
            );
        }
    }

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;
        openswd3::battle::LegacyBattleActorProgressState progress;
        actor.turn_countdown = 7;
        actor.turn_action_record.draw_offset_x = 3U;
        actor.turn_action_record.mode_flags = 5U;
        actor.turn_action_record.field_4a = 1U;
        actor.turn_action_record.field_4c = 2U;
        progress.post_action_value = 1U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x004315D0U, {.eax = 0U});
        const auto result = openswd3::battle::advance_legacy_battle_turn_gate(
            &actor,
            &shared,
            &progress,
            port,
            {.actor_token = 0x005029D0U, .entry_ecx = 0x005029D0U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTurnAdvanceStatus::frame_owner_typed_stop &&
                actor.turn_render_flags == 5U &&
                port.count(0x00485610U) == 0U && port.count(0x00478600U) == 0U,
            "turn gate stops at the first mirrored frame dereference after preserving the updater prefix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x4000U;
        state.action.group_a_count = 1;
        state.action.group_b_count = 0;
        state.action.coordinate_output_x_token = 0xABCD0100U;
        state.action.coordinate_output_y_token = 0xDCBA0200U;
        state.action.turn_coordinate_y_stack_initial = 0xAABB0000U;
        auto& action_actor = state.action.group_a_action_execution[0U];
        action_actor.profile_value = 0x55AAU;
        action_actor.turn_countdown = 7;
        action_actor.position_x = 0xAAAAU;
        action_actor.position_y = 0xBBBBU;
        action_actor.turn_action_record.field_4a = 1U;
        action_actor.turn_action_record.field_4c = 2U;
        Fixture fixture;
        fixture.startup.party[0U].position_x = 0x1234U;
        fixture.startup.party[0U].position_y = 0x5678U;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 1U});
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        LegacyBattleActionCallReply frame{.eax = 0x70000000U};
        frame.outputs = {0x71000000U, 1U, 1U, 0x72000000U};
        port.push(0x004315D0U, frame);
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.turn_advance_calls == 1U &&
                result.turn_advance.status ==
                    LegacyBattleTurnAdvanceStatus::completed &&
                result.turn_advance.current_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorCurrentCoordinateQueryStatus::
                            completed &&
                result.turn_advance.current_coordinate_query.return_eax ==
                    0xDCBA5678U &&
                result.turn_advance.current_coordinate_query.return_ecx ==
                    0xDCBA0200U &&
                result.turn_advance.current_coordinate_query.return_edx ==
                    0xABCD0100U &&
                result.turn_advance.coordinate_x == 0x00001234U &&
                result.turn_advance.coordinate_y == 0xAABB5678U &&
                result.turn_advance.coordinate_publication.argument_x ==
                    0x00001234U &&
                result.turn_advance.coordinate_publication.argument_y ==
                    0x00005678U &&
                action_actor.position_x == 0x1234U &&
                action_actor.position_y == 0x5678U &&
                fixture.startup.party[0U].position_x == 0x1234U &&
                fixture.startup.party[0U].position_y == 0x5678U &&
                port.count(0x00478600U) == 0U,
            "group A frame turn gate reads startup-party current coordinates and preserves its Y stack high word"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x4000U;
        state.action.group_a_count = 1;
        state.action.group_b_count = 0;
        state.action.coordinate_output_x_token = 0xABCD0100U;
        state.action.coordinate_output_y_token = 0xDCBA0200U;
        state.action.turn_coordinate_y_stack_initial = 0xAABB0000U;
        auto& action_actor = state.action.group_a_action_execution[0U];
        action_actor.profile_value = 0x55AAU;
        action_actor.turn_countdown = 7;
        action_actor.position_x = 0xAAAAU;
        action_actor.position_y = 0xBBBBU;
        action_actor.turn_action_record.field_4a = 1U;
        action_actor.turn_action_record.field_4c = 2U;
        Fixture fixture;
        fixture.startup.party[0U].position_x = 0x1234U;
        fixture.startup.party[0U].position_y = 0x5678U;
        fixture.startup.party[0U].position_y_read_accessible = false;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 1U});
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x004321E0U, {.eax = 1U});
        LegacyBattleActionCallReply frame{.eax = 0x70000000U};
        frame.outputs = {0x71000000U, 1U, 1U, 0x72000000U};
        port.push(0x004315D0U, frame);
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::turn_advance_typed_stop &&
                result.turn_advance.status ==
                    LegacyBattleTurnAdvanceStatus::
                        actor_current_coordinate_typed_stop &&
                result.turn_advance.current_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorCurrentCoordinateQueryStatus::
                            position_y_read_typed_stop &&
                result.turn_advance.current_coordinate_query.output_writes ==
                    1U &&
                result.turn_advance.current_coordinate_query.return_eax ==
                    0xDCBA1234U &&
                result.turn_advance.coordinate_x == 0x00001234U &&
                result.turn_advance.coordinate_y == 0xAABB0000U &&
                result.turn_advance.coordinate_publish_calls == 0U &&
                result.turn_advance.render_calls == 0U &&
                action_actor.position_x == 0xAAAAU &&
                action_actor.position_y == 0xBBBBU &&
                state.action.group_a_action_shared.turn_frame_source_token ==
                    0U &&
                state.action.action_pending_aux == 1U &&
                port.outcome_resolution_state().resolution_latch == 1U &&
                port.count(0x00478600U) == 0U &&
                port.count(0x004785C0U) == 0U && port.count(0x004170E0U) == 0U,
            "group A frame propagates startup-party query stops after the X stack prefix and blocks the parent suffix"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x4000U;
        state.action.group_a_count = 1;
        state.action.group_b_count = 0;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.turn_resolution_bits == 0U &&
                openswd3::compat::u8(state.action.packed_actor_counter) == 0U &&
                port.count(0x00471540U) == 0U &&
                port.count(0x0047F920U) == 2U &&
                result.turn_advance_calls == 1U &&
                result.turn_advance.queue_completion_calls == 1U &&
                result.turn_advance.return_eax == 1U &&
                port.count(0x004714B0U) == 0U &&
                result.turn_commit_chance_calls == 1U &&
                result.turn_commit_chance.return_eax == 0U &&
                result.turn_commit_chance.random_calls == 0U &&
                result.text_message_calls == 1U &&
                fixture.startup_reset.block_5214f8[0U] == 0x72000000U &&
                fixture.text_messages.allocations[0U].record.value_04 == 0x118U,
            "zero turn candidate takes the deterministic failure reset without consuming random state"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x8000U;
        state.action.group_a_count = 2;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U && result.turn_advance_calls == 1U &&
                result.turn_advance.return_eax == 1U &&
                port.count(0x00471540U) == 0U &&
                openswd3::compat::u8(state.action.packed_actor_counter) == 1U &&
                state.action.overlay_gate == 1U &&
                state.turn_resolution_bits == 0x8001U &&
                result.text_message_calls == 0U,
            "negative turn path advances through the typed mode-one gate and marks the current actor"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.turn_resolution_bits = 0x4000U;
        state.action.group_a_count = 1;
        state.action.group_b_count = 1;
        state.action.group_a_to_actor[0] = 0xFFFFFFFFU;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x00480AD0U, {.eax = 0xA0000000U, .object_flags = 50U});
        fixture.random.value = 36U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                result.turn_commit_chance_calls == 1U &&
                result.turn_commit_chance.actor_level == 0U &&
                result.turn_commit_chance.difference == -50 &&
                result.turn_commit_chance.random_calls == 1U &&
                result.turn_commit_chance.return_eax == 0U &&
                fixture.random.bounds == std::vector<u32>{100U} &&
                port.count(0x004714B0U) == 0U &&
                port.count(0x00483FD0U) == 1U &&
                port.count(0x00485610U) == 1U &&
                state.action.action_pending_aux == 0U &&
                port.outcome_resolution_state().resolution_latch == 0U,
            "turn resolution preserves resolved maximum in stale low word and executes failure reset"
        );
    }

    {
        LegacyBattleGroupAFrameState state;
        state.final_actor_step.actor_order[0] = 7U;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                port.count(0x0047F920U) == 0U,
            "queued actor below eight stops at first derived group A object query"
        );
    }
}
