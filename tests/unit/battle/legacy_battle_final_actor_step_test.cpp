#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
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
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::compat::u16;
using openswd3::compat::u32;

class FinalStepPort final : public LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found == replies.end() || found->second.empty()) {
            if (request.callee_token == 0x00487C10U) {
                const u32 token = next_fixed_count_token;
                next_fixed_count_token += 0x20U;
                return {
                    .eax = token,
                    .ecx = request.ecx,
                    .edx = request.edx,
                };
            }
            if (request.callee_token == 0x004783B0U) {
                return {
                    .publish_metric_word = true,
                    .metric_word = 1U,
                };
            }
            return {};
        }
        const auto reply = found->second.front();
        found->second.pop_front();
        return reply;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(
            std::ranges::count_if(calls, [callee](const auto& request) {
                return request.callee_token == callee;
            })
        );
    }

    [[nodiscard]] openswd3::battle::LegacyBattleAttackOrderRemoveBindings
    attack_order() {
        return {
            .records = attack_order_records,
            .adjacent_intensity_record = &attack_order_adjacent_record,
        };
    }

    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    u32 next_fixed_count_token{0x75000000U};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 18>
        attack_order_records{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};
};

void bind_group_b_coordinate_resource(
    FinalStepPort& port, const u32 actor_index, const u16 x, const u16 y
) {
    if (port.startup->group_b_lifecycle == nullptr) {
        port.startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }
    auto& actor = (*port.startup->group_b_lifecycle)[actor_index];
    actor.object_token = openswd3::battle::kLegacyBattleActorGroupBBaseToken +
        actor_index * openswd3::battle::kLegacyBattleActorGroupBElementSize;
    actor.resource_token =
        openswd3::battle::kLegacyBattleActorGroupBResourceStateBaseToken +
        actor_index * 0xA4U;
    actor.resource_bytes[0x62U] = static_cast<openswd3::compat::u8>(x);
    actor.resource_bytes[0x63U] = static_cast<openswd3::compat::u8>(x >> 8U);
    actor.resource_bytes[0x8AU] = static_cast<openswd3::compat::u8>(y);
    actor.resource_bytes[0x8BU] = static_cast<openswd3::compat::u8>(y >> 8U);
}

}  // namespace

void test_battle_final_actor_step(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::advance_legacy_battle_final_actor_step;

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 99U, 1U
        );
        test.expect_true(
            result.return_value == 0U && result.port_calls == 1U &&
                result.status == LegacyBattleActionDispatchStatus::completed,
            "group A validity failure returns before the first indexed flag access"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.group_a_completion_flags[1] = 1U;
        state.queued_actor_code = 9U;
        state.active_actor_code = 9U;
        state.actor_order = {9U, 77U, 88U};
        action.group_a_count = 2;
        action.group_b_count = 1;
        action.phase_counter = 0x00010000U;
        action.opponent_workspace.fill(0xFFFFFFFFU);
        state.actor_runtime_records[1].fill(0xFFFFFFFFU);
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x0047F340U, {.eax = 1U});
        port.attack_order_records[0].value_00 = 9U;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 1U, 1U, port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && action.group_a_count == 1 &&
                ((action.packed_actor_counter >> 8U) & 0xFFU) == 1U &&
                action.opponent_workspace[16] == 0U &&
                action.opponent_workspace[23] == 0U &&
                action.opponent_workspace[15] == 0xFFFFFFFFU &&
                state.queued_actor_code == 0U &&
                state.active_actor_code == 0xFFFFFFFFU &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.actor_writes == 1U &&
                state.group_a_availability_blocks[1U].value == 1U &&
                port.battle_message_state() == 1U &&
                state.actor_order[0] == 77U && state.actor_order[9] == 0U &&
                action.opponent_workspace[3] == 1U &&
                state.actor_runtime_records[1][4] == 0U &&
                result.group_a_iterations == 1U &&
                result.group_b_iterations == 1U &&
                port.count(0x0045B0E0U) == 0U &&
                port.count(0x0045EFB0U) == 0U &&
                result.attack_order_remove.matched &&
                port.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                port.count(0x004783B0U) == 2U && port.count(0x0047C660U) == 2U,
            "group A completion clears the counted workspace and runs common actor cleanup"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.removed_group_a_count = 0U;
        action.group_a_count = 1;
        action.opponent_workspace.fill(0xFFFFFFFFU);
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 1U, port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && state.removed_group_a_count == 1U &&
                state.frame_gate_a == 1U && state.frame_gate_b == 1U &&
                port.battle_message_state() == 0x67U &&
                std::ranges::all_of(
                    action.opponent_workspace,
                    [](const u32 value) { return value == 0U; }
                ) &&
                port.count(0x0047F340U) == 0U,
            "group A removed-count threshold takes the fixed workspace finalization return"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.group_a_slot_values[0] = 9U;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        port.attack_order_records[17].value_00 = 8U;
        auto attack_order = port.attack_order();
        attack_order.adjacent_intensity_record = nullptr;

        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, attack_order, 0U, 1U, port.startup.get()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        attack_order_remove_typed_stop &&
                state.removed_group_a_count == 1U &&
                state.group_a_slot_values[0] == 9U &&
                result.group_a_actor_cleanup_calls == 1U &&
                port.count(0x004750C0U) == 0U &&
                port.count(0x0045EFB0U) == 0U &&
                result.attack_order_remove.status ==
                    openswd3::battle::LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop,
            "final actor removal stop preserves validation and actor cleanup prefix then blocks slot clearing"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.group_a_completion_flags[0] = 1U;
        action.group_a_count = 20;
        action.phase_counter = 0x00010000U;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 1U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        final_actor_workspace_typed_stop &&
                ((action.packed_actor_counter >> 8U) & 0xFFU) == 1U &&
                action.group_a_count == 20 && port.count(0x0045B0E0U) == 0U,
            "group A oversized workspace stops at the first real zero write"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_a_count = 2;
        state.removed_group_a_count = 0U;
        state.group_a_availability_blocks[0U].write_accessible = false;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x0047F340U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 1U, port.startup.get()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block_calls == 1U &&
                result.actor_availability_block.return_eax == 1U &&
                result.actor_availability_block.return_ecx == 0x005029D0U &&
                result.actor_availability_block.return_edx == 0U &&
                state.action_execution_active == 0U &&
                action.opponent_workspace[2] == 0U,
            "group A typed write stop preserves the completed prefix and suppresses the caller suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0xFFFFFFFFU, 0U
        );
        test.expect_true(
            result.return_value == 0U && result.port_calls == 0U,
            "group B all-one index returns before object validation"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_coordinate_offset_typed_stop &&
                result.port_calls == 1U && state.coordinate_x == 0U &&
                state.coordinate_y == 0U && port.count(0x00480AD0U) == 0U,
            "group B coordinate owner stop preserves validation then blocks descriptor lookup"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_b_count = 2;
        state.coordinate_x = 0xFFFEU;
        state.coordinate_y = 3U;
        state.group_b_reset_word = 9U;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 2U, 5U, 6U);
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x12345678U, .object_flags = 0x20U});
        port.push(0x0047F910U, {.eax = 0x55U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.attack_order_records[0].value_00 = 2U;
        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            port.attack_order(),
            2U,
            0xFFFFFFFFU,
            port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && state.coordinate_x == 3U &&
                state.coordinate_y == 9U && state.action_delay == 0x14U &&
                state.actor_descriptor_token == 0x12345678U &&
                action.group_b_count == 1 &&
                (action.packed_actor_counter & 0xFFU) == 0U &&
                state.group_b_reset_word == 0U &&
                port.count(0x0045B0E0U) == 0U &&
                port.count(0x0045EFB0U) == 0U &&
                result.attack_order_remove.matched &&
                port.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                port.count(0x004783B0U) == 1U &&
                result.fixed_count_calls == 1U &&
                result.fixed_count.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::
                        allocated_node &&
                port.count(0x00477710U) == 0U &&
                port.count(0x00487C10U) == 1U &&
                port.legacy_battle_fixed_object_state()
                        .fixed_count_nodes.front()
                        .words[1U] == 0x00010055U &&
                port.count(0x00475870U) == 0U,
            "every non-one selector reads group B coordinates before descriptor, action and reset suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_b_count = 1;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 0U, 0U, 0U);
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x12345678U});
        port.push(0x0047F910U, {.eax = 0x55U});
        port.attack_order_records[0].value_00 = 0U;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 0U, port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && action.group_b_count == 1 &&
                state.frame_gate_a == 1U && state.frame_gate_b == 1U &&
                port.battle_message_state() == 0x63U &&
                port.battle_terminal_latch() == 0U,
            "final group-B actor publishes shared message 99 and clears the terminal latch"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.coordinate_x = 7U;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 0U, 2U, 0U);
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 0U, port.startup.get()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        final_actor_descriptor_typed_stop &&
                state.coordinate_x == 9U && result.port_calls == 2U &&
                port.count(0x00475870U) == 0U && port.count(0x0047F910U) == 0U,
            "group B null descriptor stops at the first object-field access after coordinates"
        );
    }
}
