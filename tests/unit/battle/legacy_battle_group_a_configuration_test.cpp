#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "test.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <array>
#include <bit>

namespace {

using openswd3::battle::LegacyBattleGroupAConfigurationDiagnosticPort;
using openswd3::battle::LegacyBattleGroupAConfigurationDiagnosticReply;
using openswd3::battle::LegacyBattleGroupAConfigurationDiagnosticRequest;
using openswd3::battle::LegacyBattleGroupAConfigurationSourceRecord;
using openswd3::compat::u16;
using openswd3::compat::u32;

[[nodiscard]] constexpr u16 word_at(
    const LegacyBattleGroupAConfigurationSourceRecord& record,
    const std::size_t byte_offset
) noexcept {
    const u32 shift = static_cast<u32>((byte_offset & 2U) * 8U);
    return static_cast<u16>(record.dwords[byte_offset / 4U] >> shift);
}

void set_word(
    LegacyBattleGroupAConfigurationSourceRecord& record,
    const std::size_t byte_offset,
    const u16 value
) noexcept {
    const std::size_t index = byte_offset / 4U;
    const u32 shift = static_cast<u32>((byte_offset & 2U) * 8U);
    const u32 mask = 0xFFFFU << shift;
    record.dwords[index] =
        (record.dwords[index] & ~mask) | (static_cast<u32>(value) << shift);
}

class DiagnosticPort final
    : public LegacyBattleGroupAConfigurationDiagnosticPort {
public:
    [[nodiscard]] LegacyBattleGroupAConfigurationDiagnosticReply
    report_missing_placement(
        const LegacyBattleGroupAConfigurationDiagnosticRequest& request
    ) override {
        ++calls;
        last_request = request;
        observed_source_word = source != nullptr ? word_at(*source, 0x04U) : 0U;
        return reply;
    }

    LegacyBattleGroupAConfigurationSourceRecord* source{};
    LegacyBattleGroupAConfigurationDiagnosticReply reply{};
    LegacyBattleGroupAConfigurationDiagnosticRequest last_request{};
    u16 observed_source_word{};
    u32 calls{};
};

class LiveSourceDiagnostic final
    : public LegacyBattleGroupAConfigurationDiagnosticPort {
public:
    explicit LiveSourceDiagnostic(
        openswd3::world_map::LegacyWorldStoryPartyMemberResources& source
    )
        : source_(source) {}

    LegacyBattleGroupAConfigurationDiagnosticReply report_missing_placement(
        const LegacyBattleGroupAConfigurationDiagnosticRequest&
    ) override {
        observed_current = source_.current_first;
        source_.current_first = 1U;
        return {};
    }

    u16 observed_current{};
    openswd3::world_map::LegacyWorldStoryPartyMemberResources& source_;
};

}  // namespace

void test_battle_group_a_configuration(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorProgressState;
    using openswd3::battle::LegacyBattleGroupAConfigurationSourceRecord;
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::LegacyBattleGroupAConfigurationStatus;
    using openswd3::battle::LegacyBattleGroupAPlacementRecord;
    using openswd3::battle::LegacyBattleGroupAWorkspaceState;
    using openswd3::battle::configure_legacy_battle_group_a_actor;

    for (const std::size_t readable : {0U, 3U, 4U, 55U}) {
        std::array<std::byte, 0x38U> source;
        source.fill(std::byte{0x44});
        LegacyBattleGroupAWorkspaceState workspace{.object_token = 0x005029D0U};
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x71110000U,
            .source_record_token = 0xAAAAAAAAU,
            .auxiliary_record_token = 0xBBBBBBBBU,
        };
        state.actor_record.fill(0xCCCCCCCCU);
        LegacyBattleActorProgressState progress;
        DiagnosticPort diagnostic;
        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            std::span{source}.first(readable),
            {.active = 0xABCD1234U},
            0x004AB790U,
            0x004ACF50U,
            0x0053AF70U,
            0U,
            diagnostic
        );
        bool prefix_matches = true;
        for (std::size_t index = 0U; index < state.actor_record.size();
             ++index) {
            prefix_matches &= state.actor_record[index] ==
                (index < readable / 4U ? 0x44444444U : 0xCCCCCCCCU);
        }

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAConfigurationStatus::
                        source_record_typed_stop &&
                result.placement_dwords_copied == 16U &&
                result.actor_record_dwords_copied == readable / 4U &&
                result.return_eax == 0x004AB790U &&
                result.return_ecx == 14U - readable / 4U &&
                result.return_edx == 0x0053AF70U && prefix_matches &&
                state.source_record_token == 0xAAAAAAAAU &&
                state.auxiliary_record_token == 0xBBBBBBBBU &&
                state.source_runtime_value == 0xABCD1234U &&
                diagnostic.calls == 0U,
            "short borrowed source retains each completed REP dword and stops before publishing source pointers"
        );
    }

    {
        openswd3::world_map::LegacyWorldStoryPartyMemberResources source{};
        source.current_first = 12000U;
        source.tail_2d_to_37.fill(0x44U);
        LegacyBattleGroupAWorkspaceState workspace{.object_token = 0x005029D0U};
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x71110000U
        };
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAPlacementRecord placement{.active = 0xABCD1234U};
        LiveSourceDiagnostic diagnostic{source};
        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            std::as_writable_bytes(std::span{&source, 1U}),
            placement,
            0x004AB790U,
            0x004ACF50U,
            0x0053AF70U,
            0U,
            diagnostic
        );
        test.expect_true(
            result.status == LegacyBattleGroupAConfigurationStatus::completed &&
                result.diagnostic_calls == 1U &&
                diagnostic.observed_current == 12000U &&
                static_cast<u16>(state.actor_record[1U]) == 12000U &&
                state.actor_record[13U] == 0x44444444U &&
                source.current_first == 1U &&
                result.source_clamp_writes == 0U &&
                state.source_runtime_value == 0xABCD1234U,
            "configuration copies the live world record before diagnostics and " "clamps the callback-modified source without stale copyback"
        );

        source.current_first = 12000U;
        placement.role_id = 101U;
        const auto repeated = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            std::as_writable_bytes(std::span{&source, 1U}),
            placement,
            0x004AB790U,
            0x004ACF50U,
            0x0053AF70U,
            0U,
            diagnostic
        );
        test.expect_true(
            repeated.status ==
                    LegacyBattleGroupAConfigurationStatus::completed &&
                repeated.source_clamp_writes == 1U &&
                static_cast<u16>(state.actor_record[1U]) == 12000U &&
                source.current_first == 9999U,
            "configuration clamps the world owner while retaining the earlier actor copy"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace{
            .object_token = 0x005029D0U,
        };
        workspace.early_workspace.fill(1U);
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x70000000U,
        };
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAConfigurationSourceRecord source;
        set_word(source, 0x04U, 10000U);
        set_word(source, 0x06U, 0xFFFFU);
        set_word(source, 0x08U, 32767U);
        set_word(source, 0x0AU, 9999U);
        set_word(source, 0x0CU, 10001U);
        set_word(source, 0x0EU, 0U);
        source.dwords[4U] = 0x56781234U;
        source.dwords[7U] = 0x00AB0000U;
        source.dwords[9U] = 0x00008000U;
        LegacyBattleGroupAPlacementRecord placement{
            .prefix = {1U, 2U, 3U, 4U, 5U},
            .role_id = 101U,
            .position_x = 0x2345U,
            .position_y = 0x3456U,
            .field_1a = 0x4567U,
            .active = 1U,
        };
        DiagnosticPort diagnostic;

        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            source,
            placement,
            0x004AB790U,
            0x004ACF50U,
            0x0053AF70U,
            0x12340000U,
            diagnostic
        );

        test.expect_true(
            result.status == LegacyBattleGroupAConfigurationStatus::completed &&
                result.workspace_reset.upper_workspace_dwords_zeroed == 0xBEU &&
                result.placement_dwords_copied == 16U &&
                result.actor_record_dwords_copied == 14U &&
                result.source_clamp_writes == 3U &&
                result.diagnostic_calls == 0U && diagnostic.calls == 0U &&
                state.placement_primary == state.placement_secondary &&
                state.placement_primary[5U] == 0x23450065U &&
                state.placement_primary[6U] == 0x45673456U &&
                state.source_runtime_value == 1U &&
                state.source_record_token == 0x004AB790U &&
                state.auxiliary_record_token == 0x004ACF50U &&
                state.field_2a93 == 0xABU && state.placement_word == 101U &&
                word_at(source, 0x04U) == 9999U &&
                word_at(source, 0x06U) == 0xFFFFU &&
                word_at(source, 0x08U) == 9999U &&
                word_at(source, 0x0AU) == 9999U &&
                word_at(source, 0x0CU) == 9999U &&
                word_at(source, 0x0EU) == 0U &&
                static_cast<u16>(state.actor_record[1U]) == 10000U &&
                static_cast<u16>(state.actor_record[9U] >> 16U) == 0x1234U &&
                static_cast<u16>(state.actor_record[10U]) == 0x5678U &&
                progress.special_ready == 1U &&
                result.return_eax == 0x70000000U &&
                result.return_ecx == 0x004AB790U &&
                result.return_edx == 0x00535678U,
            "group-A configuration copies unclamped records then clamps signed source fields and publishes state"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace{
            .object_token = 0x00505904U,
        };
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x71000000U,
        };
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAConfigurationSourceRecord source;
        set_word(source, 0x04U, 12000U);
        source.dwords[4U] = 0x56781234U;
        LegacyBattleGroupAPlacementRecord placement;
        DiagnosticPort diagnostic;
        diagnostic.source = &source;
        diagnostic.reply = {
            .eax = 0x11111111U,
            .ecx = 0x22222222U,
            .edx = 0xAABBCCDDU,
        };

        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            source,
            placement,
            0x004AB7C8U,
            0x004ACFB0U,
            0x0053AF90U,
            0x76543210U,
            diagnostic
        );

        test.expect_true(
            result.status == LegacyBattleGroupAConfigurationStatus::completed &&
                result.diagnostic_calls == 1U && diagnostic.calls == 1U &&
                diagnostic.observed_source_word == 12000U &&
                diagnostic.last_request.window_token == 0x76543210U &&
                diagnostic.last_request.text_token == 0x004A7C2CU &&
                diagnostic.last_request.flags == 0U &&
                diagnostic.last_request.source_token == 0x004A7C44U &&
                diagnostic.last_request.source_line == 0xDEU &&
                word_at(source, 0x04U) == 9999U &&
                result.return_edx == 0xAABB5678U,
            "zero placement word reports before source clamps and preserves diagnostic EDX high word"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace{
            .object_token = 0x00508838U,
        };
        workspace.late_workspace.fill(0xFFFFFFFFU);
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x72000000U,
        };
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAConfigurationSourceRecord source;
        LegacyBattleGroupAPlacementRecord placement{.role_id = 3U};
        DiagnosticPort diagnostic;
        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            source,
            placement,
            0x004AB800U,
            0x004AD010U,
            0U,
            0U,
            diagnostic
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAConfigurationStatus::
                        placement_typed_stop &&
                result.workspace_reset.lower_workspace_dwords_zeroed == 0x29U &&
                result.placement_dwords_copied == 0U &&
                state.placement_primary[0U] == 0U && diagnostic.calls == 0U,
            "zero placement token stops at the first source access after workspace reset"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace{
            .object_token = 0x0050B76CU,
        };
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x73000000U,
        };
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAConfigurationSourceRecord source;
        LegacyBattleGroupAPlacementRecord placement{.role_id = 4U};
        DiagnosticPort diagnostic;
        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            source,
            placement,
            0U,
            0x004AD070U,
            0x0053AFD0U,
            0U,
            diagnostic
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAConfigurationStatus::
                        source_record_typed_stop &&
                result.placement_dwords_copied == 16U &&
                result.actor_record_dwords_copied == 0U &&
                result.return_ecx == 14U && result.return_edx == 0x0053AFD0U,
            "zero source token stops after both placement copies and before actor record writes"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace{
            .object_token = 0x0050E6A0U,
        };
        LegacyBattleGroupAConfigurationState state;
        LegacyBattleActorProgressState progress;
        LegacyBattleGroupAConfigurationSourceRecord source;
        LegacyBattleGroupAPlacementRecord placement{.role_id = 5U};
        DiagnosticPort diagnostic;
        const auto result = configure_legacy_battle_group_a_actor(
            workspace,
            state,
            progress,
            source,
            placement,
            0x004AB838U,
            0x004AD0D0U,
            0x0053AFF0U,
            0U,
            diagnostic
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAConfigurationStatus::
                        actor_record_typed_stop &&
                result.placement_dwords_copied == 16U &&
                result.actor_record_dwords_copied == 0U &&
                result.return_eax == 0x004AB838U && result.return_ecx == 14U &&
                result.return_edx == 0x0053AFF0U,
            "zero actor record token stops at the first destination write after reading source"
        );
    }
}
