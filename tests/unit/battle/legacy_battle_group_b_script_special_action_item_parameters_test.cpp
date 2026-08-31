#include "openswd3/battle/legacy_battle_group_b_script_special_action_item_parameters.hpp"

#include <array>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::
    LegacyBattleGroupBScriptSpecialActionItemParametersRequest;
using openswd3::battle::
    LegacyBattleGroupBScriptSpecialActionItemParametersStatus;
using openswd3::compat::u16;
using openswd3::compat::u32;

constexpr u32 kActorToken = 0x0052AB58U;
constexpr u32 kResourceToken = 0x73ABCDEFU;
constexpr u32 kEntryEax = 0xA5A51234U;
constexpr u32 kEntryEdx = 0xCAFEBABEU;
constexpr std::array<u32, 4> kTargetOffsets{0x72U, 0x74U, 0x76U, 0x74U};

[[nodiscard]] LegacyBattleGroupBScriptSpecialActionItemParametersRequest
request_for(const std::array<u16, 4>& parameters) {
    return {
        .parameters = parameters,
        .actor_token = kActorToken,
        .entry_eax = kEntryEax,
        .entry_edx = kEntryEdx,
    };
}

[[nodiscard]] u16 resource_word(
    const LegacyBattleActorGroupBElementState& actor, const u32 offset
) {
    return static_cast<u16>(
        static_cast<u16>(actor.resource_bytes[offset]) |
        (static_cast<u16>(actor.resource_bytes[offset + 1U]) << 8U)
    );
}

}  // namespace

void test_battle_group_b_script_special_action_item_parameters(
    openswd3::test::Context& test
) {
    {
        const std::array<u16, 4> parameters{};
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_special_action_item_parameters(
                nullptr, request_for(parameters)
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        completed &&
                result.parameter_reads == 4U &&
                result.resource_pointer_loads == 0U &&
                result.resource_writes == 0U &&
                result.return_eax == 0xA5A50000U &&
                result.return_ecx == kActorToken &&
                result.return_edx == kEntryEdx,
            "four zero special parameters skip every actor and resource access"
        );
    }

    for (u32 selected = 0U; selected < 4U; ++selected) {
        std::array<u16, 4> parameters{};
        parameters[selected] = static_cast<u16>(0x8000U + selected);
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = kResourceToken;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_special_action_item_parameters(
                &actor, request_for(parameters)
            );
        const u32 expected_eax = selected == 3U ? 0xA5A58003U : 0xA5A50000U;
        const u32 expected_ecx = selected == 3U ? kResourceToken : kActorToken;
        const u32 expected_edx = selected < 3U ? kResourceToken : kEntryEdx;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        completed &&
                result.parameter_reads == 4U &&
                result.resource_pointer_loads == 1U &&
                result.resource_writes == 1U &&
                resource_word(actor, kTargetOffsets[selected]) ==
                    parameters[selected] &&
                actor.resource_bytes[0x71U] == 0xEEU &&
                actor.resource_bytes[0x78U] == 0xEEU &&
                result.return_eax == expected_eax &&
                result.return_ecx == expected_ecx &&
                result.return_edx == expected_edx,
            "each nonzero special parameter writes its original target and selects the original pointer register"
        );
    }

    {
        const std::array<u16, 4> parameters{0x1111U, 0x8000U, 0xFFFFU, 0x4444U};
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = kResourceToken;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_special_action_item_parameters(
                &actor, request_for(parameters)
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        completed &&
                result.parameter_reads == 4U &&
                result.resource_pointer_loads == 4U &&
                result.resource_writes == 4U &&
                resource_word(actor, 0x72U) == 0x1111U &&
                resource_word(actor, 0x74U) == 0x4444U &&
                resource_word(actor, 0x76U) == 0xFFFFU &&
                actor.resource_bytes[0x78U] == 0xEEU &&
                result.return_eax == 0xA5A54444U &&
                result.return_ecx == kResourceToken &&
                result.return_edx == kResourceToken,
            "the fourth parameter preserves the legacy overwrite of the second target word"
        );
    }

    for (u32 stopped = 0U; stopped < 4U; ++stopped) {
        std::array<u16, 4> parameters{};
        parameters[stopped] = 0x7F01U;
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_special_action_item_parameters(
                nullptr, request_for(parameters)
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        actor_state_typed_stop &&
                result.stopped_offset == 0x0CU &&
                result.stopped_parameter_index == stopped &&
                result.parameter_reads == stopped + 1U &&
                result.resource_pointer_loads == 0U &&
                result.resource_writes == 0U &&
                result.return_eax == 0xA5A57F01U &&
                result.return_ecx == kActorToken &&
                result.return_edx == kEntryEdx,
            "leading zero special parameters defer the actor stop to the first nonzero access"
        );
    }

    for (u32 stopped = 0U; stopped < 4U; ++stopped) {
        std::array<u16, 4> parameters{};
        parameters[stopped] = 0x7F02U;
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0U;
        actor.resource_bytes.fill(0xEEU);
        const auto result = openswd3::battle::
            write_legacy_battle_group_b_script_special_action_item_parameters(
                &actor, request_for(parameters)
            );
        const u32 expected_ecx = stopped == 3U ? 0U : kActorToken;
        const u32 expected_edx = stopped < 3U ? 0U : kEntryEdx;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBScriptSpecialActionItemParametersStatus::
                        resource_write_typed_stop &&
                result.stopped_offset == kTargetOffsets[stopped] &&
                result.stopped_parameter_index == stopped &&
                result.parameter_reads == stopped + 1U &&
                result.resource_pointer_loads == 1U &&
                result.resource_writes == 0U &&
                result.return_eax == 0xA5A57F02U &&
                result.return_ecx == expected_ecx &&
                result.return_edx == expected_edx &&
                resource_word(actor, kTargetOffsets[stopped]) == 0xEEEEU,
            "a zero resource token stops at the aliased target after publishing the selected pointer register"
        );
    }
}
