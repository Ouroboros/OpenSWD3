#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupASummonMaterializationCall;
using openswd3::battle::LegacyBattleGroupASummonMaterializationCallReply;
using openswd3::battle::LegacyBattleGroupASummonMaterializationCallRequest;
using openswd3::battle::LegacyBattleGroupASummonMaterializationPort;
using openswd3::battle::LegacyBattleGroupASummonProfileRecord;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void set_profile_byte(
    LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset,
    const u8 value
) noexcept {
    record[offset] = static_cast<std::byte>(value);
}

void set_profile_word(
    LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset,
    const u16 value
) noexcept {
    set_profile_byte(record, offset, static_cast<u8>(value));
    set_profile_byte(record, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] u8 actor_byte(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u8>(
        record[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] u16 actor_word(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u16>(actor_byte(record, offset)) |
        static_cast<u16>(
               static_cast<u16>(actor_byte(record, offset + 1U)) << 8U
        );
}

class MaterializationPort final
    : public LegacyBattleGroupASummonMaterializationPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleGroupASummonMaterializationCallReply
    invoke_group_a_summon_materialization(
        const LegacyBattleGroupASummonMaterializationCallRequest& request
    ) override {
        calls.push_back(request);
        if (request.call ==
            LegacyBattleGroupASummonMaterializationCall::allocate_profile) {
            return {.eax = allocation_token};
        }
        if (request.call ==
            LegacyBattleGroupASummonMaterializationCall::load_profile) {
            return {.eax = load_eax, .profile_record = loaded_profile};
        }
        return {.profile_record = request.profile_record};
    }

    u32 allocation_token{0x71000000U};
    u32 load_eax{1U};
    LegacyBattleGroupASummonProfileRecord loaded_profile{};
    std::vector<LegacyBattleGroupASummonMaterializationCallRequest> calls;

protected:
    [[nodiscard]] std::optional<bool> prepare_definition_record(
        const std::span<u8> destination, const u32
    ) noexcept override {
        std::transform(
            loaded_profile.cbegin(),
            loaded_profile.cend(),
            destination.begin(),
            [](const std::byte value) { return std::to_integer<u8>(value); }
        );
        return load_eax != 0U;
    }
};

}  // namespace

void test_battle_group_a_summon_materialization(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::LegacyBattleGroupAPlacementRecord;
    using openswd3::battle::LegacyBattleGroupASummonMaterializationStatus;
    using openswd3::battle::materialize_legacy_battle_group_a_summon;

    {
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x72000000U,
            .actor_record = {},
            .source_record_token = 0x004AB790U,
        };
        state.actor_record.fill(0xCCCCCCCCU);
        LegacyBattleGroupAPlacementRecord source{
            .prefix = {1U, 2U, 3U, 4U, 5U},
            .role_id = 0x123U,
            .position_x = 0x2345U,
            .position_y = 0x3456U,
            .field_1a = 0x4567U,
            .active = 1U,
        };
        MaterializationPort port;
        set_profile_word(port.loaded_profile, 0x56U, 0x1111U);
        set_profile_word(port.loaded_profile, 0x58U, 0x2222U);
        set_profile_word(port.loaded_profile, 0x5AU, 0x3333U);
        set_profile_word(port.loaded_profile, 0x5CU, 0x4444U);
        set_profile_word(port.loaded_profile, 0x60U, 0x5566U);
        set_profile_word(port.loaded_profile, 0x64U, 0x7788U);
        set_profile_byte(port.loaded_profile, 0x90U, 0xABU);
        for (std::size_t index = 0U; index < 9U; ++index) {
            set_profile_byte(
                port.loaded_profile,
                0x92U + index,
                static_cast<u8>(0x0AU + index)
            );
        }

        const auto result = materialize_legacy_battle_group_a_summon(
            &state, &source, 0x005029D0U, 0x0053AF70U, 0x12340000U, port
        );

        bool name_matches = true;
        for (std::size_t index = 0U; index < 9U; ++index) {
            name_matches = name_matches &&
                actor_byte(state.actor_record, 0x2DU + index) == 0x0AU + index;
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupASummonMaterializationStatus::completed &&
                result.port_calls == 3U && result.allocation_calls == 1U &&
                result.load_calls == 1U && result.release_calls == 1U &&
                result.diagnostic_calls == 0U &&
                result.profile_dwords_zeroed == 0x29U &&
                result.placement_dwords_copied == 16U &&
                result.profile_name_bytes_copied == 9U &&
                port.calls.size() == 2U &&
                port.calls[0U].call ==
                    LegacyBattleGroupASummonMaterializationCall::
                        allocate_profile &&
                port.requested_definition_ids == std::vector<u32>{0x123U} &&
                port.calls[1U].call ==
                    LegacyBattleGroupASummonMaterializationCall::
                        release_profile_text &&
                state.profile_token == 0x71000000U &&
                state.placement_primary == state.placement_secondary &&
                state.placement_primary[5U] == 0x23450123U &&
                state.placement_primary[6U] == 0x45673456U &&
                state.placement_tail == 1U && state.placement_word == 0x123U &&
                actor_word(state.actor_record, 0x26U) == 0x1111U &&
                actor_word(state.actor_record, 0x28U) == 0x2222U &&
                actor_word(state.actor_record, 0x16U) == 0x3333U &&
                actor_word(state.actor_record, 0x14U) == 0x4444U &&
                actor_word(state.actor_record, 0x1EU) == 0x00ABU &&
                actor_word(state.actor_record, 0x04U) == 0x7788U &&
                actor_word(state.actor_record, 0x0AU) == 0x7788U &&
                name_matches && state.field_2a93 == 0xABU &&
                state.source_record_token == state.actor_record_token &&
                state.profile_field_f2 == 0x5566U &&
                result.return_eax == 0x71000000U &&
                result.return_edx == 0x72000000U && port.open_calls == 1U &&
                port.read_calls == 3U && port.release_calls == 1U,
            "summon materialization preserves allocation, profile projection, duplicate source copies, and final register shape"
        );
    }

    {
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x73000000U,
        };
        LegacyBattleGroupAPlacementRecord source{.role_id = 0U};
        MaterializationPort port;
        const auto result = materialize_legacy_battle_group_a_summon(
            &state, &source, 0x00505904U, 0x0053AF90U, 0x76543210U, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupASummonMaterializationStatus::completed &&
                result.diagnostic_calls == 1U && result.port_calls == 4U &&
                port.calls[2U].call ==
                    LegacyBattleGroupASummonMaterializationCall::
                        report_missing_role &&
                port.calls[2U].window_token == 0x76543210U,
            "zero summon role reports only after load, release, and duplicate source publication"
        );
    }

    {
        LegacyBattleGroupAConfigurationState state{
            .profile_token = 0xAAAAAAAAU,
        };
        LegacyBattleGroupAPlacementRecord source{.role_id = 1U};
        MaterializationPort port;
        port.allocation_token = 0U;
        const auto result = materialize_legacy_battle_group_a_summon(
            &state, &source, 0x005029D0U, 0x0053AF70U, 0U, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupASummonMaterializationStatus::
                        allocation_typed_stop &&
                result.port_calls == 1U && result.profile_dwords_zeroed == 0U &&
                state.profile_token == 0xAAAAAAAAU,
            "zero allocation stops at the first profile clear without attaching the record"
        );
    }

    {
        LegacyBattleGroupAConfigurationState state{
            .profile_token = 0xBBBBBBBBU,
        };
        LegacyBattleGroupAPlacementRecord source{.role_id = 2U};
        MaterializationPort port;
        const auto actor_stop = materialize_legacy_battle_group_a_summon(
            &state, &source, 0U, 0x0053AF70U, 0U, port
        );

        LegacyBattleGroupAConfigurationState source_state{
            .profile_token = 0xCCCCCCCCU,
        };
        MaterializationPort source_port;
        const auto source_stop = materialize_legacy_battle_group_a_summon(
            &source_state, &source, 0x005029D0U, 0U, 0U, source_port
        );

        test.expect_true(
            actor_stop.status ==
                    LegacyBattleGroupASummonMaterializationStatus::
                        actor_state_typed_stop &&
                actor_stop.port_calls == 1U &&
                actor_stop.profile_dwords_zeroed == 0x29U &&
                state.profile_token == 0xBBBBBBBBU &&
                source_stop.status ==
                    LegacyBattleGroupASummonMaterializationStatus::
                        source_record_typed_stop &&
                source_stop.port_calls == 1U &&
                source_state.profile_token == 0x71000000U,
            "actor and source stops preserve the exact allocation, clear, and attachment prefixes"
        );
    }

    {
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0U,
        };
        state.actor_record.fill(0xDDDDDDDDU);
        LegacyBattleGroupAPlacementRecord source{
            .role_id = 0U,
            .active = 7U,
        };
        MaterializationPort port;
        const auto result = materialize_legacy_battle_group_a_summon(
            &state, &source, 0x005029D0U, 0x0053AF70U, 0x11110000U, port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupASummonMaterializationStatus::
                        actor_record_typed_stop &&
                result.port_calls == 4U && result.diagnostic_calls == 1U &&
                result.placement_dwords_copied == 16U &&
                state.placement_tail == 7U &&
                std::ranges::all_of(
                    state.actor_record,
                    [](const u32 value) { return value == 0xDDDDDDDDU; }
                ),
            "missing actor record stops at the first projected word after all prior calls and source copies"
        );
    }
}
