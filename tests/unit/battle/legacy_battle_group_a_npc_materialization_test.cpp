#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_a_npc_materialization.hpp"
#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupASummonMaterializationCall;
using openswd3::battle::LegacyBattleGroupASummonMaterializationCallReply;
using openswd3::battle::LegacyBattleGroupASummonMaterializationCallRequest;
using openswd3::battle::LegacyBattleGroupASummonMaterializationPort;
using openswd3::battle::LegacyBattleGroupASummonProfileRecord;
using openswd3::compat::i16;
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

void set_record_byte(
    std::array<u32, 14>& record, const std::size_t offset, const u8 value
) noexcept {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    const u32 mask = 0xFFU << shift;
    record[index] =
        (record[index] & ~mask) | (static_cast<u32>(value) << shift);
}

void set_record_word(
    std::array<u32, 14>& record, const std::size_t offset, const u16 value
) noexcept {
    set_record_byte(record, offset, static_cast<u8>(value));
    set_record_byte(record, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] u8 record_byte(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u8>(
        record[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] u16 record_word(
    const std::array<u32, 14>& record, const std::size_t offset
) noexcept {
    return static_cast<u16>(record_byte(record, offset)) |
        static_cast<u16>(
               static_cast<u16>(record_byte(record, offset + 1U)) << 8U
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
            return {.eax = 1U, .profile_record = loaded_profile};
        }
        return {.profile_record = request.profile_record};
    }

    u32 allocation_token{0x71000000U};
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
        return true;
    }
};

}  // namespace

void test_battle_group_a_npc_materialization(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::LegacyBattleGroupANpcMaterializationStatus;
    using openswd3::battle::LegacyBattleGroupAPlacementRecord;
    using openswd3::battle::materialize_legacy_battle_group_a_npc;

    {
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x72001234U,
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
        std::array<u32, 14> modifier{};
        set_record_word(modifier, 0x0AU, std::bit_cast<u16>(i16{-100}));
        set_record_word(modifier, 0x26U, 60000U);
        set_record_word(modifier, 0x28U, 1000U);
        set_record_word(modifier, 0x14U, 0xFFFFU);
        set_record_word(modifier, 0x16U, 50000U);
        set_record_byte(modifier, 0x2CU, 250U);
        MaterializationPort port;
        set_profile_word(port.loaded_profile, 0x24U, 0xFFFDU);
        set_profile_word(port.loaded_profile, 0x2CU, 2U);
        set_profile_word(port.loaded_profile, 0x60U, 0x5566U);
        set_profile_byte(port.loaded_profile, 0x90U, 0xABU);
        for (std::size_t index = 0U; index < 9U; ++index) {
            set_profile_byte(
                port.loaded_profile,
                0x92U + index,
                static_cast<u8>(0x20U + index)
            );
        }

        const auto result = materialize_legacy_battle_group_a_npc(
            &state,
            &source,
            &modifier,
            0x005029D0U,
            0x0053AF70U,
            0x004AB790U,
            0x12340000U,
            port
        );

        bool name_matches = true;
        for (std::size_t index = 0U; index < 9U; ++index) {
            name_matches = name_matches &&
                record_byte(state.actor_record, 0x2DU + index) == 0x20U + index;
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupANpcMaterializationStatus::completed &&
                result.port_calls == 3U && result.allocation_calls == 1U &&
                result.load_calls == 1U && result.release_calls == 1U &&
                result.diagnostic_calls == 0U &&
                result.profile_dwords_zeroed == 0x29U &&
                result.placement_dwords_copied == 16U &&
                result.adjusted_word_writes == 5U &&
                result.adjusted_byte_writes == 1U &&
                result.profile_name_bytes_copied == 9U &&
                port.calls.size() == 2U &&
                port.requested_definition_ids == std::vector<u32>{0x123U} &&
                state.profile_token == 0x71000000U &&
                state.placement_primary == state.placement_secondary &&
                state.placement_primary[5U] == 0x23450123U &&
                record_word(state.actor_record, 0x0AU) == 0xFFBAU &&
                record_word(state.actor_record, 0x26U) == 42000U &&
                record_word(state.actor_record, 0x28U) == 700U &&
                record_word(state.actor_record, 0x14U) == 45875U &&
                record_byte(state.actor_record, 0x2CU) == 175U &&
                record_word(state.actor_record, 0x16U) == 60000U &&
                record_word(state.actor_record, 0x1EU) == 0x00ABU &&
                record_word(state.actor_record, 0x04U) == 0xFFBAU &&
                name_matches && state.field_2a93 == 0xABU &&
                state.source_record_token == state.actor_record_token &&
                state.profile_field_f2 == 0x5566U &&
                result.return_eax == 0x72005566U &&
                result.return_edx == 0x71000000U && port.open_calls == 1U &&
                port.read_calls == 3U && port.release_calls == 1U,
            "NPC materialization preserves signed and unsigned percentage adjustments, projection order, and final registers"
        );
    }

    {
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0x73000000U,
        };
        LegacyBattleGroupAPlacementRecord source{.role_id = 0U};
        std::array<u32, 14> modifier{};
        MaterializationPort port;
        const auto result = materialize_legacy_battle_group_a_npc(
            &state,
            &source,
            &modifier,
            0x00505904U,
            0x0053AF90U,
            0x004AB790U,
            0x76543210U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupANpcMaterializationStatus::completed &&
                result.diagnostic_calls == 1U && result.port_calls == 4U &&
                port.calls[2U].call ==
                    LegacyBattleGroupASummonMaterializationCall::
                        report_missing_role &&
                port.calls[2U].window_token == 0x76543210U &&
                port.calls[2U].diagnostic_text_token == 0x004A7C7CU &&
                port.calls[2U].diagnostic_source_token == 0x004A7C44U &&
                port.calls[2U].diagnostic_source_line == 0x14EU,
            "zero NPC role reports fixed NPC diagnostic arguments after profile release and source copies"
        );
    }

    {
        LegacyBattleGroupAPlacementRecord source{.role_id = 1U};
        std::array<u32, 14> modifier{};
        MaterializationPort actor_port;
        const auto actor_stop = materialize_legacy_battle_group_a_npc(
            nullptr,
            &source,
            &modifier,
            0x005029D0U,
            0x0053AF70U,
            0x004AB790U,
            0U,
            actor_port
        );

        LegacyBattleGroupAConfigurationState allocation_state{
            .profile_token = 0xAAAAAAAAU,
        };
        MaterializationPort allocation_port;
        allocation_port.allocation_token = 0U;
        const auto allocation_stop = materialize_legacy_battle_group_a_npc(
            &allocation_state,
            &source,
            &modifier,
            0x005029D0U,
            0x0053AF70U,
            0x004AB790U,
            0U,
            allocation_port
        );

        test.expect_true(
            actor_stop.status ==
                    LegacyBattleGroupANpcMaterializationStatus::
                        actor_state_typed_stop &&
                actor_stop.port_calls == 1U &&
                actor_stop.profile_dwords_zeroed == 0U &&
                allocation_stop.status ==
                    LegacyBattleGroupANpcMaterializationStatus::
                        allocation_typed_stop &&
                allocation_stop.port_calls == 1U &&
                allocation_stop.profile_dwords_zeroed == 0U &&
                allocation_state.profile_token == 0U,
            "NPC actor and allocation stops preserve the write-before-clear ordering"
        );
    }

    {
        LegacyBattleGroupAConfigurationState source_state{
            .actor_record_token = 0x74000000U,
        };
        LegacyBattleGroupAPlacementRecord source{.role_id = 2U};
        std::array<u32, 14> modifier{};
        MaterializationPort source_port;
        const auto source_stop = materialize_legacy_battle_group_a_npc(
            &source_state,
            &source,
            &modifier,
            0x005029D0U,
            0U,
            0x004AB790U,
            0U,
            source_port
        );

        LegacyBattleGroupAConfigurationState modifier_state{
            .actor_record_token = 0x75000000U,
        };
        MaterializationPort modifier_port;
        const auto modifier_stop = materialize_legacy_battle_group_a_npc(
            &modifier_state,
            &source,
            nullptr,
            0x005029D0U,
            0x0053AF70U,
            0U,
            0U,
            modifier_port
        );

        test.expect_true(
            source_stop.status ==
                    LegacyBattleGroupANpcMaterializationStatus::
                        source_record_typed_stop &&
                source_stop.port_calls == 1U &&
                source_stop.profile_dwords_zeroed == 0x29U &&
                modifier_stop.status ==
                    LegacyBattleGroupANpcMaterializationStatus::
                        modifier_record_typed_stop &&
                modifier_stop.port_calls == 3U &&
                modifier_stop.placement_dwords_copied == 16U &&
                modifier_state.actor_record[0U] == 0U,
            "NPC source and modifier stops retain the exact allocation, clear, callee, and source-copy prefixes"
        );
    }

    {
        LegacyBattleGroupAConfigurationState state{
            .actor_record_token = 0U,
        };
        state.actor_record.fill(0xDDDDDDDDU);
        LegacyBattleGroupAPlacementRecord source{.role_id = 3U};
        std::array<u32, 14> modifier{};
        set_record_word(modifier, 0x0AU, 100U);
        MaterializationPort port;
        const auto result = materialize_legacy_battle_group_a_npc(
            &state,
            &source,
            &modifier,
            0x005029D0U,
            0x0053AF70U,
            0x004AB790U,
            0U,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupANpcMaterializationStatus::
                        actor_record_typed_stop &&
                result.port_calls == 3U &&
                result.placement_dwords_copied == 16U &&
                state.actor_record[0U] == 0xDDDDDDDDU,
            "missing NPC actor record stops at the first adjusted word write without partial projection"
        );
    }
}
