#include "openswd3/battle/legacy_battle_group_b_script_action_item_parameters.hpp"

#include <array>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBScriptActionItemParametersRequest;
using openswd3::battle::LegacyBattleGroupBScriptActionItemParametersStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;

constexpr u32 kActorToken = 0x0052AB58U;
constexpr u32 kResourceToken = 0x73ABCDEFU;
constexpr u32 kEntryEax = 0xA5A51234U;
constexpr u32 kEntryEdx = 0xCAFEBABEU;

[[nodiscard]] LegacyBattleGroupBScriptActionItemParametersRequest
request_for(const std::array<u16, 6>& parameters) {
    return {
        .parameters = parameters,
        .actor_token = kActorToken,
        .entry_eax = kEntryEax,
        .entry_edx = kEntryEdx,
    };
}

[[nodiscard]] u16 resource_word(
    const LegacyBattleActorGroupBElementState& actor, const u32 index
) {
    const u32 offset = 0x66U + index * 2U;
    return static_cast<u16>(
        static_cast<u16>(actor.resource_bytes[offset]) |
        (static_cast<u16>(actor.resource_bytes[offset + 1U]) << 8U)
    );
}

}  // namespace

void test_battle_group_b_script_action_item_parameters(
    openswd3::test::Context& test
) {
    {
        const std::array<u16, 6> parameters{};
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_action_item_parameters(
                nullptr, request_for(parameters)
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        completed &&
                result.parameter_reads == 6U &&
                result.resource_pointer_loads == 0U &&
                result.resource_writes == 0U &&
                result.return_eax == 0xA5A50000U &&
                result.return_ecx == kActorToken &&
                result.return_edx == kEntryEdx,
            "six zero parameters skip every actor and resource access while preserving stale pointer registers"
        );
    }

    for (u32 selected = 0U; selected < 6U; ++selected) {
        std::array<u16, 6> parameters{};
        parameters[selected] = static_cast<u16>(0x8000U + selected);
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = kResourceToken;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_action_item_parameters(
                &actor, request_for(parameters)
            );
        bool slots_match = true;
        for (u32 index = 0U; index < 6U; ++index) {
            const u16 expected = index == selected ? parameters[index]
                                                   : static_cast<u16>(0xEEEEU);
            slots_match =
                slots_match && resource_word(actor, index) == expected;
        }
        const u32 expected_eax = selected == 5U ? 0xA5A58005U : 0xA5A50000U;
        const u32 expected_ecx = selected == 5U ? kResourceToken : kActorToken;
        const u32 expected_edx = selected < 5U ? kResourceToken : kEntryEdx;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        completed &&
                result.parameter_reads == 6U &&
                result.resource_pointer_loads == 1U &&
                result.resource_writes == 1U && slots_match &&
                result.return_eax == expected_eax &&
                result.return_ecx == expected_ecx &&
                result.return_edx == expected_edx,
            "each nonzero parameter writes only its paired resource word and selects the original pointer register"
        );
    }

    {
        const std::array<u16, 6> parameters{
            0x1111U, 0x8000U, 0xFFFFU, 0x4444U, 0x5555U, 0x6666U
        };
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = kResourceToken;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_action_item_parameters(
                &actor, request_for(parameters)
            );
        bool slots_match = true;
        for (u32 index = 0U; index < 6U; ++index) {
            slots_match =
                slots_match && resource_word(actor, index) == parameters[index];
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        completed &&
                result.parameter_reads == 6U &&
                result.resource_pointer_loads == 6U &&
                result.resource_writes == 6U && slots_match &&
                actor.resource_bytes[0x65U] == 0xEEU &&
                actor.resource_bytes[0x72U] == 0xEEU &&
                result.return_eax == 0xA5A56666U &&
                result.return_ecx == kResourceToken &&
                result.return_edx == kResourceToken,
            "all six nonzero parameters reload the resource and preserve unsigned word payloads"
        );
    }

    for (u32 stopped = 0U; stopped < 6U; ++stopped) {
        std::array<u16, 6> parameters{};
        parameters[stopped] = 0x7F01U;
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_action_item_parameters(
                nullptr, request_for(parameters)
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        actor_state_typed_stop &&
                result.stopped_offset == 0x0CU &&
                result.stopped_parameter_index == stopped &&
                result.parameter_reads == stopped + 1U &&
                result.resource_pointer_loads == 0U &&
                result.resource_writes == 0U &&
                result.return_eax == 0xA5A57F01U &&
                result.return_ecx == kActorToken &&
                result.return_edx == kEntryEdx,
            "leading zero parameters defer the actor stop until the first nonzero resource pointer read"
        );
    }

    for (u32 stopped = 0U; stopped < 6U; ++stopped) {
        std::array<u16, 6> parameters{};
        parameters[stopped] = 0x7F02U;
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_action_item_parameters(
                &actor, request_for(parameters)
            );
        const u32 expected_ecx = stopped == 5U ? 0U : kActorToken;
        const u32 expected_edx = stopped < 5U ? 0U : kEntryEdx;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptActionItemParametersStatus::
                        resource_write_typed_stop &&
                result.stopped_offset == 0x66U + stopped * 2U &&
                result.stopped_parameter_index == stopped &&
                result.parameter_reads == stopped + 1U &&
                result.resource_pointer_loads == 1U &&
                result.resource_writes == 0U &&
                result.return_eax == 0xA5A57F02U &&
                result.return_ecx == expected_ecx &&
                result.return_edx == expected_edx &&
                resource_word(actor, stopped) == 0xEEEEU,
            "a zero resource token stops at the selected word write after publishing the selected pointer register"
        );
    }
}
