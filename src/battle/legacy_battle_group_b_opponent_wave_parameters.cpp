#include "openswd3/battle/legacy_battle_group_b_opponent_wave_parameters.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::u16 read_word(
    const std::array<std::byte, 0x28>& bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(std::to_integer<compat::u8>(bytes[offset])) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(
                std::to_integer<compat::u8>(bytes[offset + 1U])
            )
            << 8U
        )
    );
}

void replace_low_word(compat::u32& value, const compat::u16 low) noexcept {
    value = (value & 0xFFFF0000U) | low;
}

}  // namespace

LegacyBattleGroupBOpponentWaveParametersResult
read_legacy_battle_group_b_opponent_wave_parameters(
    const LegacyBattleActorGroupBElementState* actor,
    compat::u16* special_action_output,
    compat::u16* spawn_count_output,
    const LegacyBattleGroupBOpponentWaveParametersRequest& request
) {
    LegacyBattleGroupBOpponentWaveParametersResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = request.actor_token;
    result.return_edx = request.first_output_token;

    if (actor == nullptr) {
        result.status = LegacyBattleGroupBOpponentWaveParametersStatus::
            actor_state_typed_stop;
        return result;
    }

    const auto& profile = actor->action_configuration.profile_buffer;
    result.special_action = read_word(profile, 0x20U);
    replace_low_word(result.return_eax, result.special_action);
    if (special_action_output == nullptr) {
        result.status = LegacyBattleGroupBOpponentWaveParametersStatus::
            first_output_write_typed_stop;
        return result;
    }

    *special_action_output = result.special_action;
    ++result.first_output_writes;

    result.spawn_count = std::to_integer<compat::u8>(profile[0x24U]);
    replace_low_word(result.return_eax, result.spawn_count);
    result.return_ecx = request.second_output_token;
    if (spawn_count_output == nullptr) {
        result.status = LegacyBattleGroupBOpponentWaveParametersStatus::
            second_output_write_typed_stop;
        return result;
    }

    *spawn_count_output = result.spawn_count;
    ++result.second_output_writes;
    return result;
}

}  // namespace openswd3::battle
