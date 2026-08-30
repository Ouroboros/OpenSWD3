#include "openswd3/battle/legacy_battle_group_b_action_profile_flag.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>

namespace {

using openswd3::compat::u32;

void write_dword(
    std::array<std::byte, 0x28>& bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<std::byte>(value);
    bytes[offset + 1U] = static_cast<std::byte>(value >> 8U);
    bytes[offset + 2U] = static_cast<std::byte>(value >> 16U);
    bytes[offset + 3U] = static_cast<std::byte>(value >> 24U);
}

}  // namespace

void test_battle_group_b_action_profile_flag(
    openswd3::test::Context& test
) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBActionProfileFlagStatus;
    using openswd3::battle::query_legacy_battle_group_b_action_profile_flag;

    {
        const auto result = query_legacy_battle_group_b_action_profile_flag(
            nullptr,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0xA1B2C3D4U,
                .entry_edx = 0x55667788U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileFlagStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0xA1B2C3D4U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x55667788U,
            "action profile flag preserves entry registers at actor stop"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        write_dword(
            actor.action_configuration.profile_buffer, 0x08U, 0x10000000U
        );
        const auto result = query_legacy_battle_group_b_action_profile_flag(
            &actor,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0xFFFFFFFFU,
                .entry_edx = 0x12345678U,
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionProfileFlagStatus::completed &&
                result.return_eax == 1U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x12345678U,
            "primary profile bit returns one before reading the fallback bit"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_execution.special_particle_coordinate_suppression = 0xFFU;
        write_dword(
            actor.action_configuration.profile_buffer, 0x04U, 0x00001000U
        );
        const auto result = query_legacy_battle_group_b_action_profile_flag(
            &actor, {.actor_token = 0x00525508U}
        );
        test.expect_true(
            result.return_eax == 1U,
            "fallback profile bit uses the group B profile buffer owner"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        write_dword(
            actor.action_configuration.profile_buffer, 0x04U, 0xFFFFEFFFU
        );
        write_dword(
            actor.action_configuration.profile_buffer, 0x08U, 0xEFFFFFFFU
        );
        const auto result = query_legacy_battle_group_b_action_profile_flag(
            &actor,
            {
                .actor_token = 0x00525508U,
                .entry_eax = 0xFFFFFFFFU,
                .entry_edx = 0xCAFEBABEU,
            }
        );
        test.expect_true(
            result.return_eax == 0U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0xCAFEBABEU,
            "unrelated profile bits return zero and preserve ECX and EDX"
        );
    }
}
