#include "openswd3/battle/legacy_battle_group_a_profile_mode_selection.hpp"

#include "test.hpp"

#include <array>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupAProfileModeRandomReply;
using openswd3::battle::LegacyBattleGroupAProfileModeSelectionPort;
using openswd3::compat::u32;

struct RandomPort final : LegacyBattleGroupAProfileModeSelectionPort {
    [[nodiscard]] LegacyBattleGroupAProfileModeRandomReply random_below(
        const u32 bound, const u32 eax, const u32 ecx, const u32 edx
    ) override {
        bounds.push_back(bound);
        entries.push_back({eax, ecx, edx});
        return reply;
    }

    LegacyBattleGroupAProfileModeRandomReply reply{};
    std::vector<u32> bounds;
    std::vector<std::array<u32, 3>> entries;
};

}  // namespace

void test_battle_group_a_profile_mode_selection(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionState;
    using openswd3::battle::LegacyBattleGroupAEmbeddedProfileApplicationState;
    using openswd3::battle::LegacyBattleGroupAItemEffectApplicationState;
    using openswd3::battle::LegacyBattleGroupAProfileModeSelectionStatus;
    using openswd3::battle::select_legacy_battle_group_a_profile_mode;

    constexpr u32 actor_token = 0x005029D0U;

    {
        LegacyBattleGroupAActionExecutionSharedState shared;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded;
        LegacyBattleGroupAItemEffectApplicationState item;
        RandomPort port;
        const auto result = select_legacy_battle_group_a_profile_mode(
            nullptr,
            shared,
            embedded,
            item,
            0U,
            0U,
            0U,
            port,
            {.entry_eax = 0x12345678U, .entry_edx = 0xABCDEF01U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAProfileModeSelectionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x12345678U && result.return_ecx == 0U &&
                result.return_edx == 0xABCDEF01U && port.bounds.empty(),
            "profile mode selection stops at the first actor field read with entry registers intact"
        );
    }

    for (const u32 gate_case : {0U, 1U, 2U, 3U}) {
        LegacyBattleGroupAActionExecutionState state;
        state.identity_word = 7U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        shared.last_identity = gate_case == 3U ? 7U : 0U;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded;
        LegacyBattleGroupAItemEffectApplicationState item;
        if (gate_case == 2U) {
            item.effect_flags = 1U;
        }
        RandomPort port;
        const auto result = select_legacy_battle_group_a_profile_mode(
            &state,
            shared,
            embedded,
            item,
            actor_token,
            gate_case == 0U ? 1U : 0U,
            gate_case == 1U ? 1U : 0U,
            port
        );
        test.expect_true(
            result.return_eax == 0U && state.profile_mode == 0U &&
                port.bounds.empty(),
            "skip fields effect flags and repeated identity each return zero before counter logic"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.identity_word = 9U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        shared.completion_counter = 12U;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded{
            .status_bits = 1U,
        };
        LegacyBattleGroupAItemEffectApplicationState item;
        RandomPort port;
        const auto result = select_legacy_battle_group_a_profile_mode(
            &state, shared, embedded, item, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.return_eax == 1U && state.profile_mode == 1U &&
                shared.completion_counter == 12U &&
                shared.last_identity == 0U && result.counter_clears == 0U &&
                port.bounds.empty(),
            "embedded status bit selects profile mode immediately without clearing counters or publishing identity"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.identity_word = 0x3456U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        shared.profile_threshold = 2U;
        shared.completion_counter = 10U;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded;
        LegacyBattleGroupAItemEffectApplicationState item;
        RandomPort port;
        const auto result = select_legacy_battle_group_a_profile_mode(
            &state, shared, embedded, item, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 10U &&
                result.return_edx == 10U && state.profile_mode == 1U &&
                shared.completion_counter == 0U &&
                shared.last_identity == 0x3456U && port.bounds.empty(),
            "completion counter at threshold plus eight deterministically selects and publishes identity"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.identity_word = 3U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        shared.profile_threshold = 100U;
        shared.completion_counter = 7U;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded;
        LegacyBattleGroupAItemEffectApplicationState item;
        RandomPort port;
        const auto result = select_legacy_battle_group_a_profile_mode(
            &state, shared, embedded, item, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 108U &&
                result.return_edx == 7U && state.profile_mode == 0U &&
                port.bounds.empty(),
            "completion counter below eight returns zero without random selection"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.identity_word = 4U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        shared.profile_threshold = 100U;
        shared.completion_counter = 8U;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded;
        LegacyBattleGroupAItemEffectApplicationState item;
        RandomPort port;
        port.reply = {.eax = 5U, .ecx = 0x11111111U, .edx = 0x22222222U};
        const auto result = select_legacy_battle_group_a_profile_mode(
            &state, shared, embedded, item, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0x11111111U &&
                result.return_edx == 0x22222222U && state.profile_mode == 0U &&
                shared.completion_counter == 8U &&
                port.bounds == std::vector<u32>{10U} &&
                port.entries[0U] == std::array<u32, 3>{4U, 108U, 8U},
            "random low word five returns zero and preserves random callee ecx edx"
        );
    }

    {
        LegacyBattleGroupAActionExecutionState state;
        state.identity_word = 0x6789U;
        LegacyBattleGroupAActionExecutionSharedState shared;
        shared.profile_threshold = 100U;
        shared.completion_counter = 8U;
        LegacyBattleGroupAEmbeddedProfileApplicationState embedded;
        LegacyBattleGroupAItemEffectApplicationState item;
        RandomPort port;
        port.reply = {
            .eax = 0xABCD0006U,
            .ecx = 0x11111111U,
            .edx = 0x22222222U,
        };
        const auto result = select_legacy_battle_group_a_profile_mode(
            &state, shared, embedded, item, actor_token, 0U, 0U, port
        );
        test.expect_true(
            result.return_eax == 1U && result.return_ecx == 0x6789U &&
                result.return_edx == 0x22222222U && state.profile_mode == 1U &&
                shared.completion_counter == 0U &&
                shared.last_identity == 0x6789U && result.random_calls == 1U,
            "random low word six selects profile mode clears the counter and replaces ecx with identity"
        );
    }
}
