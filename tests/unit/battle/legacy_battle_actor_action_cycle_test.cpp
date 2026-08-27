#include "openswd3/battle/legacy_battle_actor_action_cycle.hpp"

#include <optional>
#include <vector>

#include "test.hpp"

namespace {

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

}  // namespace

void test_battle_actor_action_cycle(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorActionCycleBindings;
    using openswd3::battle::cycle_legacy_battle_actor_action;

    {
        openswd3::battle::LegacyBattleFinalActorStepState actor;
        actor.queued_actor_code = 7U;
        actor.pre_frame_gate_b = 9U;
        ActorActionCyclePort port;
        const auto result = cycle_legacy_battle_actor_action(
            LegacyBattleActorActionCycleBindings{actor},
            port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            actor.pre_frame_gate_b == 0U && result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0x22U && result.return_edx == 0x33U &&
                result.port_calls == 0U && port.calls.empty(),
            "actor code seven clears the shared gate then returns the wrapped switch index"
        );
    }

    {
        openswd3::battle::LegacyBattleFinalActorStepState actor;
        actor.queued_actor_code = 12U;
        actor.pre_frame_gate_b = 9U;
        ActorActionCyclePort port;
        const auto result = cycle_legacy_battle_actor_action(
            LegacyBattleActorActionCycleBindings{actor},
            port,
            {.entry_ecx = 0x44U, .entry_edx = 0x55U}
        );
        test.expect_true(
            actor.pre_frame_gate_b == 0U && result.return_eax == 4U &&
                result.return_ecx == 0x44U && result.return_edx == 0x55U &&
                result.port_calls == 0U && port.calls.empty(),
            "actor code twelve takes the unsigned default branch with transformed EAX"
        );
    }

    constexpr std::array<u32, 4> expected_starts{11U, 8U, 9U, 10U};
    for (u32 index = 0U; index < expected_starts.size(); ++index) {
        openswd3::battle::LegacyBattleFinalActorStepState actor;
        actor.queued_actor_code = index + 8U;
        actor.pre_frame_gate_b = 9U;
        ActorActionCyclePort port;
        port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = 0x80U + index,
                .ecx = 0x90U + index,
                .edx = 0xA0U + index,
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 0xB0U + index,
                .ecx = 0xC0U + index,
                .edx = 0xD0U + index,
            },
        };
        const auto result = cycle_legacy_battle_actor_action(
            LegacyBattleActorActionCycleBindings{actor},
            port,
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            actor.pre_frame_gate_b == 0U && result.port_calls == 2U &&
                result.resolve_calls == 1U && result.commit_calls == 1U &&
                port.calls.size() == 2U &&
                port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::
                        actor_action_resolve_available &&
                port.calls[0U].arguments[0U] == expected_starts[index] &&
                port.calls[0U].eax == index && port.calls[0U].ecx == 0x22U &&
                port.calls[0U].edx == 0x33U &&
                port.calls[1U].call ==
                    LegacyBattleInputDispatchCall::
                        actor_action_commit_candidate &&
                port.calls[1U].arguments[0U] == 0x80U + index &&
                port.calls[1U].eax == 0x80U + index &&
                port.calls[1U].ecx == 0x90U + index &&
                port.calls[1U].edx == 0xA0U + index &&
                result.return_eax == 0xB0U + index &&
                result.return_ecx == 0xC0U + index &&
                result.return_edx == 0xD0U + index,
            "actor codes eight through eleven resolve and commit their rotated action starts"
        );
    }
}
