#include "openswd3/battle/legacy_battle_group_b_opponent_wave_parameters.hpp"

#include <cstddef>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBOpponentWaveParametersStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;

[[nodiscard]] LegacyBattleActorGroupBElementState
prepared_actor(const u16 special_action, const u8 spawn_count) {
    LegacyBattleActorGroupBElementState actor;
    auto& profile = actor.action_configuration.profile_buffer;
    profile[0x1FU] = std::byte{0xA5};
    profile[0x20U] = static_cast<std::byte>(special_action);
    profile[0x21U] = static_cast<std::byte>(special_action >> 8U);
    profile[0x22U] = std::byte{0x5A};
    profile[0x23U] = std::byte{0xC3};
    profile[0x24U] = static_cast<std::byte>(spawn_count);
    return actor;
}

}  // namespace

void test_battle_group_b_opponent_wave_parameters(
    openswd3::test::Context& test
) {
    {
        u16 special_action = 0x1111U;
        u16 spawn_count = 0x2222U;
        const auto result = openswd3::battle::
            read_legacy_battle_group_b_opponent_wave_parameters(
                nullptr,
                &special_action,
                &spawn_count,
                {
                    .actor_token = 0x00525508U,
                    .first_output_token = 0x0053BF28U,
                    .second_output_token = 0x0053BF2AU,
                    .entry_eax = 0xABCD1234U,
                    .entry_edx = 0xDEADBEEFU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOpponentWaveParametersStatus::
                        actor_state_typed_stop &&
                special_action == 0x1111U && spawn_count == 0x2222U &&
                result.first_output_writes == 0U &&
                result.second_output_writes == 0U &&
                result.return_eax == 0xABCD1234U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x0053BF28U,
            "missing actor stops after publishing the first output pointer and preserves both outputs"
        );
    }

    {
        const auto actor = prepared_actor(0xBEEFU, 0x7AU);
        u16 spawn_count = 0x2222U;
        const auto result = openswd3::battle::
            read_legacy_battle_group_b_opponent_wave_parameters(
                &actor,
                nullptr,
                &spawn_count,
                {
                    .actor_token = 0x00525508U,
                    .first_output_token = 0U,
                    .second_output_token = 0x0053BF2AU,
                    .entry_eax = 0xABCD1234U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOpponentWaveParametersStatus::
                        first_output_write_typed_stop &&
                result.special_action == 0xBEEFU && result.spawn_count == 0U &&
                spawn_count == 0x2222U && result.first_output_writes == 0U &&
                result.second_output_writes == 0U &&
                result.return_eax == 0xABCDBEEFU &&
                result.return_ecx == 0x00525508U && result.return_edx == 0U,
            "invalid first output stops after the first actor read and before the spawn byte read"
        );
    }

    {
        const auto actor = prepared_actor(0xBEEFU, 0x7AU);
        u16 special_action = 0x1111U;
        const auto result = openswd3::battle::
            read_legacy_battle_group_b_opponent_wave_parameters(
                &actor,
                &special_action,
                nullptr,
                {
                    .actor_token = 0x00525508U,
                    .first_output_token = 0x0053BF28U,
                    .second_output_token = 0U,
                    .entry_eax = 0xABCD1234U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOpponentWaveParametersStatus::
                        second_output_write_typed_stop &&
                special_action == 0xBEEFU && result.special_action == 0xBEEFU &&
                result.spawn_count == 0x7AU &&
                result.first_output_writes == 1U &&
                result.second_output_writes == 0U &&
                result.return_eax == 0xABCD007AU && result.return_ecx == 0U &&
                result.return_edx == 0x0053BF28U,
            "invalid second output preserves the first write after replacing AX with the zero-extended byte"
        );
    }

    {
        const auto actor = prepared_actor(0xBEEFU, 0xFFU);
        u16 special_action = 0x1111U;
        u16 spawn_count = 0x2222U;
        const auto result = openswd3::battle::
            read_legacy_battle_group_b_opponent_wave_parameters(
                &actor,
                &special_action,
                &spawn_count,
                {
                    .actor_token = 0x00525508U,
                    .first_output_token = 0x0053BF28U,
                    .second_output_token = 0x0053BF2AU,
                    .entry_eax = 0xCAFEFFFFU,
                    .entry_edx = 0xDEADBEEFU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOpponentWaveParametersStatus::completed &&
                special_action == 0xBEEFU && spawn_count == 0x00FFU &&
                result.special_action == 0xBEEFU &&
                result.spawn_count == 0xFFU &&
                result.first_output_writes == 1U &&
                result.second_output_writes == 1U &&
                result.return_eax == 0xCAFE00FFU &&
                result.return_ecx == 0x0053BF2AU &&
                result.return_edx == 0x0053BF28U,
            "successful publication reads only the two tail fields and returns the original pointer registers"
        );
    }
}
