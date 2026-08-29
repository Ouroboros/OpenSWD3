#include "openswd3/battle/legacy_battle_group_a_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
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
    [[nodiscard]] u32 random_bounded(u32) override {
        return 0U;
    }
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
        port.push(0x0047CE80U, {.eax = 1U});
        port.push(0x0047CE80U, {.eax = 0U});
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
                port.count(0x00439070U) == 1U && port.count(0x0047CE80U) >= 3U,
            "AI coordination counts terminals and retries one based target until live"
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
                port.count(0x00478330U) == 1U && port.count(0x00464CC0U) == 0U,
            "group-A queue caller directly prepares the shared actor target without invoking the closed boundary"
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
                openswd3::compat::u8(state.action.packed_actor_counter) == 1U &&
                port.battle_message_state() == 0x67U &&
                port.count(0x00471540U) == 2U &&
                port.count(0x004714B0U) == 1U &&
                result.text_message_calls == 1U &&
                fixture.startup_reset.block_5214f8[0U] == 0x72000000U &&
                fixture.text_messages.allocations[0U].record.value_04 == 0x118U,
            "turn completion then takes the final actor terminal suffix in the same frame"
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
        port.push(0x004714B0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_a_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                has_call_argument(port, 0x004714B0U, 1U, 50U) &&
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
