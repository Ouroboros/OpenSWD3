#include "openswd3/battle/legacy_battle_available_actor_cycle.hpp"

#include "test.hpp"

#include <optional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::compat::u32;

class AvailableActorCyclePort final
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
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    AvailableActorCyclePort port;
};

}  // namespace

void test_battle_available_actor_cycle(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleAvailableActorCycleStatus;
    using openswd3::battle::cycle_legacy_battle_available_actor;

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 4U;
        fixture.final_actor.actor_order = {10U, 9U, 8U, 11U};
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = 7U, .ecx = 0xAAU, .edx = 0xBBU
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 0U, .ecx = 0xCCU, .edx = 0xDDU
            },
        };
        const auto result = cycle_legacy_battle_available_actor(
            {.final_actor = fixture.final_actor, .metrics = fixture.metrics},
            fixture.port,
            {
                .starting_actor_code = 10U,
                .entry_eax = 0x11U,
                .entry_ecx = 0x22U,
                .entry_edx = 0x33U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleAvailableActorCycleStatus::completed &&
                result.candidate_calls == 2U && result.port_calls == 2U &&
                result.candidate_codes[0U] == 10U &&
                result.candidate_codes[1U] == 9U && result.return_eax == 9U &&
                result.return_ecx == 0xCCU && result.return_edx == 0xDDU,
            "candidate ten falls through to available candidate nine and preserves the winning callee registers"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 4U;
        fixture.final_actor.actor_order = {10U, 9U, 8U, 11U};
        fixture.port.replies.resize(4U);
        for (u32 index = 0U; index < fixture.port.replies.size(); ++index) {
            fixture.port.replies[index] = LegacyBattleInputDispatchCallReply{
                .eax = 1U,
                .ecx = 0x100U + index,
                .edx = 0x200U + index,
            };
        }
        const auto result = cycle_legacy_battle_available_actor(
            {.final_actor = fixture.final_actor, .metrics = fixture.metrics},
            fixture.port,
            {.starting_actor_code = 8U}
        );
        test.expect_true(
            result.status == LegacyBattleAvailableActorCycleStatus::completed &&
                result.candidate_calls == 4U && result.port_calls == 4U &&
                result.candidate_codes ==
                    std::array<u32, 4>{8U, 11U, 10U, 9U} &&
                result.return_eax == 0U && result.return_ecx == 0x103U &&
                result.return_edx == 0x203U,
            "four unavailable candidates wrap only when the post-read index equals four and then return zero"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 4U;
        fixture.final_actor.actor_order.fill(99U);
        const auto result = cycle_legacy_battle_available_actor(
            {.final_actor = fixture.final_actor, .metrics = fixture.metrics},
            fixture.port,
            {
                .starting_actor_code = 99U,
                .entry_ecx = 0x22U,
                .entry_edx = 0x33U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleAvailableActorCycleStatus::completed &&
                result.candidate_calls == 4U && result.port_calls == 0U &&
                result.candidate_codes == std::array<u32, 4>{2U, 1U, 0U, 3U} &&
                result.return_eax == 0U && result.return_edx == 4U,
            "unknown start preserves the original adjacent-table reads two one zero three"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.final_actor.actor_order[0U] = 2U;
        const auto result = cycle_legacy_battle_available_actor(
            {.final_actor = fixture.final_actor, .metrics = fixture.metrics},
            fixture.port,
            {.starting_actor_code = 99U, .entry_edx = 0x44U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleAvailableActorCycleStatus::
                        candidate_availability_typed_stop &&
                result.candidate_calls == 1U && result.port_calls == 0U &&
                result.candidate_codes[0U] == 2U &&
                result.candidate_availability.status ==
                    openswd3::battle::
                        LegacyBattleActorActionCandidateAvailabilityStatus::
                            group_a_actor_typed_stop &&
                fixture.port.calls.empty(),
            "adjacent candidate two stops at its first real group-A actor access without modern sanitization"
        );
    }
}
