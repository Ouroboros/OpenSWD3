#include "openswd3/battle/legacy_battle_group_b_action_six_target_availability.hpp"

#include <array>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionSixTargetAvailabilityStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_dword(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] LegacyBattleActorGroupBElementState
prepared_actor(const u32 flags, const u16 threshold) {
    LegacyBattleActorGroupBElementState actor;
    actor.resource_token = 0x73001234U;
    write_dword(actor.resource_bytes, 0x20U, flags);
    write_word(actor.resource_bytes, 0x52U, threshold);
    return actor;
}

}  // namespace

void test_battle_group_b_action_six_target_availability(
    openswd3::test::Context& test
) {
    {
        const auto result = openswd3::battle::
            query_legacy_battle_group_b_action_six_target_availability(
                nullptr,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0xAAAAAAAAU,
                    .entry_edx = 0xBBBBBBBBU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSixTargetAvailabilityStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0xAAAAAAAAU &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0xBBBBBBBBU,
            "missing actor stops at the first resource-token access with entry registers intact"
        );
    }

    {
        auto actor = prepared_actor(0x00000800U, 0x15U);
        actor.resource_token = 0U;
        const auto result = openswd3::battle::
            query_legacy_battle_group_b_action_six_target_availability(
                &actor,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0xAAAAAAAAU,
                    .entry_edx = 0xBBBBBBBBU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSixTargetAvailabilityStatus::
                        resource_read_typed_stop &&
                result.return_eax == 0xAAAAAAAAU && result.return_ecx == 0U &&
                result.return_edx == 0xBBBBBBBBU,
            "missing resource stops at the first flags access after publishing the null ECX"
        );
    }

    {
        auto actor = prepared_actor(0xFFFF0820U, 0x15U);
        const auto result = openswd3::battle::
            query_legacy_battle_group_b_action_six_target_availability(
                &actor,
                {
                    .actor_token = 0x00525508U,
                    .entry_edx = 0xBBBBBBBBU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSixTargetAvailabilityStatus::
                        completed &&
                result.resource_flags == 0xFFFF0820U &&
                result.resource_threshold == 0U && result.return_eax == 0U &&
                result.return_ecx == 0x73001234U &&
                result.return_edx == 0xBBBBBBBBU,
            "low-byte bit five fails before the threshold read even when high-byte bit three is set"
        );
    }

    {
        auto actor = prepared_actor(0xFFFF0000U, 0x15U);
        const auto result = openswd3::battle::
            query_legacy_battle_group_b_action_six_target_availability(
                &actor,
                {
                    .actor_token = 0x00525508U,
                    .entry_edx = 0xBBBBBBBBU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSixTargetAvailabilityStatus::
                        completed &&
                result.resource_flags == 0xFFFF0000U &&
                result.resource_threshold == 0U && result.return_eax == 0U,
            "absent high-byte bit three fails before the threshold read"
        );
    }

    for (const auto [threshold, expected] : std::array<std::array<u16, 2>, 4>{
             {{0U, 1U}, {0x15U, 1U}, {0x16U, 0U}, {0xFFFFU, 0U}}
         }) {
        auto actor = prepared_actor(0xABCD0800U, threshold);
        const auto result = openswd3::battle::
            query_legacy_battle_group_b_action_six_target_availability(
                &actor,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0xAAAAAAAAU,
                    .entry_edx = 0xBBBBBBBBU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSixTargetAvailabilityStatus::
                        completed &&
                result.resource_flags == 0xABCD0800U &&
                result.resource_threshold == threshold &&
                result.return_eax == expected &&
                result.return_ecx == 0x73001234U &&
                result.return_edx == 0xBBBBBBBBU,
            "enabled gates use an unsigned less-than-or-equal twenty-one threshold and canonical full-EAX return"
        );
    }
}
