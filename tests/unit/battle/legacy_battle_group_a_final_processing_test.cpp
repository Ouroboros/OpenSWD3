#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing.hpp"

#include "test.hpp"

#include <type_traits>
#include <vector>

static_assert(std::is_aggregate_v<
              openswd3::battle::LegacyBattleGroupAActionExecutionState>);

namespace {

using openswd3::battle::LegacyBattleGroupAFinalProcessingPort;
using openswd3::battle::LegacyBattleGroupAItemEffectApplicationCallReply;
using openswd3::battle::LegacyBattleGroupAItemEffectApplicationCallRequest;
using openswd3::battle::LegacyBattleGroupAProfileModeRandomReply;
using openswd3::compat::u16;
using openswd3::compat::u32;

class FinalPort final : public LegacyBattleGroupAFinalProcessingPort,
                        public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    u32 item_calls{};
    u32 random_calls{};

    [[nodiscard]] LegacyBattleGroupAItemEffectApplicationCallReply
    invoke_group_a_item_effect_application(
        const LegacyBattleGroupAItemEffectApplicationCallRequest& request
    ) override {
        ++item_calls;
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] LegacyBattleGroupAProfileModeRandomReply random_below(
        const u32, const u32 eax, const u32 ecx, const u32 edx
    ) override {
        ++random_calls;
        return {.eax = eax, .ecx = ecx, .edx = edx};
    }
};

}  // namespace

void test_battle_group_a_final_processing(openswd3::test::Context& test) {
    using namespace openswd3::battle;

    LegacyBattleActorProgressState progress;
    LegacyBattleGroupAConfigurationState configuration;
    configuration.actor_record_token = 0x00600000U;
    configuration.actor_record[9] = 0x45670000U;
    LegacyBattleGroupAAttributeAggregationState aggregation;
    LegacyBattleGroupAActionExecutionSharedState shared;

    {
        LegacyBattleGroupAFinalProcessingState state;
        LegacyBattleGroupAItemEffectApplicationState item;
        LegacyBattleGroupAWorkspaceState workspace;
        state.replacement_action_kind = 0x1234U;
        workspace.early_workspace.fill(0xFFFFFFFFU);
        const auto result = finalize_legacy_battle_actor_mode_four(
            &state, &item, &workspace, 0x005029D0U, 0xA5A50000U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorModeFourFinalizationStatus::completed &&
                item.mode_flags == 0x02U && item.action_kind == 0x1234U &&
                state.completion_latch == 1U &&
                result.workspace_dwords_zeroed == 0x4CU &&
                workspace.early_workspace[0U] == 0U &&
                workspace.early_workspace.back() == 0U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x005029D0U,
            "mode-four finalization publishes flags and latch, conditionally copies action kind, then clears 304 bytes"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState state;
        LegacyBattleGroupAActionExecutionState action;
        LegacyBattleGroupAItemEffectApplicationState item;
        FinalPort port;
        const auto result = process_legacy_battle_group_a_final(
            &state,
            &action,
            progress,
            &configuration,
            aggregation,
            &item,
            shared,
            0x005029D0U,
            0U,
            0U,
            port,
            port,
            {.entry_eax = 0xAABBCCDDU, .entry_edx = 0x11223344U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAFinalProcessingStatus::completed &&
                result.return_eax == 1U && result.item_effect_calls == 0U &&
                result.profile_load_calls == 0U,
            "zero action returns one before every later side effect"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState state;
        state.replacement_action_kind = 26U;
        LegacyBattleGroupAActionExecutionState action;
        action.profile_buffer[3U] = 0x28U;
        LegacyBattleGroupAItemEffectApplicationState item;
        item.action_kind = 7U;
        item.mode_flags = 0x02U;
        FinalPort port;
        const auto result = process_legacy_battle_group_a_final(
            &state,
            &action,
            progress,
            &configuration,
            aggregation,
            &item,
            shared,
            0x005029D0U,
            0U,
            0U,
            port,
            port
        );
        test.expect_true(
            result.return_eax == 1U && state.completion_latch == 1U &&
                item.display_kind == 26U && item.action_kind == 200U &&
                result.action_kind_writes == 2U &&
                result.profile_load_calls == 0U,
            "mode bit replaces the action then publishes the flagged transition"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState state;
        state.pre_effect_words.fill(0xFFFFFFFFU);
        LegacyBattleGroupAActionExecutionState action;
        action.profile_buffer.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item;
        item.action_kind = 5U;
        item.cached_profile_item_id = 1U;
        FinalPort port;
        const auto result = process_legacy_battle_group_a_final(
            &state,
            &action,
            progress,
            &configuration,
            aggregation,
            &item,
            shared,
            0x005029D0U,
            1U,
            0U,
            port,
            port,
            {.entry_eax = 0xABCD0000U, .entry_edx = 0x12340000U}
        );
        test.expect_true(
            result.return_eax == 0U && result.item_effect_calls == 1U &&
                result.profile_mode_calls == 1U &&
                result.profile_load_calls == 1U && port.read_calls == 3U &&
                item.derived_words[0] == 0x4567U &&
                result.pre_effect_dwords_zeroed == 4U &&
                result.profile_buffer_dwords_zeroed == 10U,
            "normal path clears both blocks, derives the record word, and loads the zero profile"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState state;
        state.profile_record_id = 9U;
        LegacyBattleGroupAActionExecutionState action;
        LegacyBattleGroupAItemEffectApplicationState item;
        item.action_kind = 23U;
        item.cached_profile_item_id = 1U;
        aggregation.embedded_profile_application.status_bits = 0x20U;
        FinalPort port;
        const auto result = process_legacy_battle_group_a_final(
            &state,
            &action,
            progress,
            &configuration,
            aggregation,
            &item,
            shared,
            0x005029D0U,
            1U,
            0U,
            port,
            port
        );
        test.expect_true(
            result.return_eax == 1U && result.profile_load_calls == 2U &&
                port.open_calls == 1U && port.read_calls == 6U &&
                action.profile_buffer[1] == 0x00000100U &&
                item.display_kind == 23U && item.action_kind == 200U,
            "nonzero profile performs the second load and publishes an unblocked transition"
        );
    }

    for (const u32 flags : {0U, 8U, 0x0800U}) {
        LegacyBattleGroupAFinalProcessingState state;
        state.profile_record_id = 9U;
        LegacyBattleGroupAActionExecutionState action;
        action.profile_buffer.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item;
        item.action_kind = 26U;
        item.cached_profile_item_id = 1U;
        FinalPort port;
        port.set_profile_dword(0x0CU, 0xCAFE0000U | flags);
        port.set_profile_dword(0x04U, 2U);
        port.set_profile_word(0x14U, 0x1234U);
        port.set_profile_word(0x16U, 0xABCDU);
        port.set_profile_word(0x22U, 0x8001U);
        const auto result = process_legacy_battle_group_a_final(
            &state,
            &action,
            progress,
            &configuration,
            aggregation,
            &item,
            shared,
            0x005029D0U,
            1U,
            0U,
            port,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAFinalProcessingStatus::completed &&
                result.profile_load_calls == 2U &&
                action.profile_buffer[3U] == (0xCAFE0000U | flags) &&
                action.special_effect_direct_mode() ==
                    static_cast<openswd3::compat::u8>(flags) &&
                (action.special_particle_coordinate_suppression() & 2U) != 0U &&
                action.copied_runtime_word() == 0x1234U &&
                action.source_y() == 0x8001U &&
                item.action_kind == (flags == 8U ? 200U : 26U) &&
                item.display_kind == (flags == 8U ? 26U : 0U),
            "loaded MON bytes immediately drive finalization flags and all overlapping action views"
        );
        action.write_profile_word(0x14U, 0x9876U);
        test.expect_true(
            action.profile_buffer[5U] == 0xABCD9876U &&
                action.profile_buffer[3U] == (0xCAFE0000U | flags),
            "profile action WORD writes preserve the adjacent loaded WORD and flag DWORD"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState state;
        state.pre_effect_words.fill(7U);
        LegacyBattleGroupAActionExecutionState action;
        LegacyBattleGroupAItemEffectApplicationState item;
        item.action_kind = 5U;
        FinalPort port;
        const auto result = process_legacy_battle_group_a_final(
            &state,
            &action,
            progress,
            nullptr,
            aggregation,
            &item,
            shared,
            0x005029D0U,
            0U,
            0U,
            port,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAFinalProcessingStatus::
                        actor_record_typed_stop &&
                result.pre_effect_dwords_zeroed == 4U &&
                state.pre_effect_words[0] == 0U && item.derived_words[0] == 0U,
            "missing actor record stops after the original sixteen-byte clear"
        );
    }
}
