#include "openswd3/battle/legacy_battle_actor_action_cycle.hpp"

#include <optional>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorActionCycleBindings;
using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::compat::u32;

class ActorActionCyclePort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        const std::size_t index = calls.size() - 1U;
        if (index < replies.size() && replies[index].has_value()) {
            return *replies[index];
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::vector<std::optional<LegacyBattleInputDispatchCallReply>> replies;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupResetBlocks startup;
    openswd3::battle::LegacyBattleFinalActorStepState actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    u32 message{};
    ActorActionCyclePort port;

    [[nodiscard]] LegacyBattleActorActionCycleBindings bindings() {
        return {
            .startup_reset = startup,
            .final_actor = actor,
            .metrics = metrics,
            .input_dispatch = input,
            .message_state = message,
        };
    }
};

}  // namespace

void test_battle_actor_action_cycle(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorActionCycleStatus;
    using openswd3::battle::cycle_legacy_battle_actor_action;

    {
        Fixture fixture;
        fixture.actor.queued_actor_code = 7U;
        fixture.actor.pre_frame_gate_b = 9U;
        const auto result = cycle_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            fixture.actor.pre_frame_gate_b == 0U &&
                result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0x22U && result.return_edx == 0x33U &&
                result.port_calls == 0U && fixture.port.calls.empty(),
            "actor code seven clears the shared gate then returns the wrapped switch index"
        );
    }

    {
        Fixture fixture;
        fixture.actor.queued_actor_code = 12U;
        fixture.actor.pre_frame_gate_b = 9U;
        const auto result = cycle_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.entry_ecx = 0x44U, .entry_edx = 0x55U}
        );
        test.expect_true(
            fixture.actor.pre_frame_gate_b == 0U && result.return_eax == 4U &&
                result.return_ecx == 0x44U && result.return_edx == 0x55U &&
                result.port_calls == 0U && fixture.port.calls.empty(),
            "actor code twelve takes the unsigned default branch with transformed EAX"
        );
    }

    constexpr std::array<u32, 4> expected_starts{11U, 8U, 9U, 10U};
    for (u32 index = 0U; index < expected_starts.size(); ++index) {
        Fixture fixture;
        fixture.actor.queued_actor_code = index + 8U;
        fixture.actor.pre_frame_gate_b = 9U;
        fixture.actor.actor_order[0U] = index + 8U;
        fixture.metrics.group_a_count = 2U;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = index + 8U,
                .ecx = 0x90U + index,
                .edx = 0xA0U + index,
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 0U,
                .ecx = 0xC0U + index,
                .edx = 0xD0U + index,
            },
        };
        const auto result = cycle_legacy_battle_actor_action(
            fixture.bindings(),
            fixture.port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.status == LegacyBattleActorActionCycleStatus::completed &&
                fixture.actor.pre_frame_gate_b == 0U &&
                result.port_calls == 2U && result.resolve_calls == 1U &&
                result.commit_calls == 1U && fixture.port.calls.size() == 2U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::
                        actor_action_resolve_available &&
                fixture.port.calls[0U].arguments[0U] ==
                    expected_starts[index] &&
                fixture.port.calls[0U].eax == index &&
                fixture.port.calls[0U].ecx == 0x22U &&
                fixture.port.calls[0U].edx == 0x33U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleInputDispatchCall::query_active_actor &&
                fixture.port.calls[1U].eax == index * 0x3EFU &&
                fixture.port.calls[1U].ecx == 0x005029D0U + index * 0x2F34U &&
                fixture.port.calls[1U].edx == 0xA0U + index &&
                result.return_eax == index + 8U && result.return_ecx == 0U &&
                result.return_edx == 0xD0U + index,
            "actor codes eight through eleven resolve their rotated starts then use the typed queue commit"
        );
    }

    {
        Fixture fixture;
        fixture.actor.queued_actor_code = 8U;
        fixture.actor.actor_order.fill(1U);
        fixture.metrics.group_a_count = 12U;
        fixture.startup.value_4ff0b0 = 9U;
        fixture.port.replies.push_back(
            LegacyBattleInputDispatchCallReply{
                .eax = 2U,
                .ecx = 0x90U,
                .edx = 0xA0U,
            }
        );
        const auto result = cycle_legacy_battle_actor_action(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionCycleStatus::
                        action_commit_typed_stop &&
                result.resolve_calls == 1U && result.commit_calls == 1U &&
                result.port_calls == 1U && fixture.port.calls.size() == 1U &&
                result.return_eax == 11U && result.return_ecx == 0x90U &&
                result.return_edx == 11U && fixture.startup.value_4ff0b0 == 9U,
            "typed queue stop propagates through the forward cycle after preserving the resolver result"
        );
    }
}
