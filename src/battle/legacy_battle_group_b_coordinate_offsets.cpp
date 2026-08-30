#include "openswd3/battle/legacy_battle_group_b_coordinate_offsets.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] u16 read_word(
    const std::array<u8, 0xA4>& bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 replacement) noexcept {
    return (value & 0xFFFF0000U) | replacement;
}

}  // namespace

LegacyBattleGroupBCoordinateOffsetResult
read_legacy_battle_group_b_coordinate_offsets(
    const LegacyBattleActorGroupBElementState* const actor,
    compat::u16* const first_output,
    compat::u16* const second_output,
    const LegacyBattleGroupBCoordinateOffsetRequest& request
) noexcept {
    LegacyBattleGroupBCoordinateOffsetResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
    };
    if (actor == nullptr) {
        result.status =
            LegacyBattleGroupBCoordinateOffsetStatus::actor_state_typed_stop;
        return result;
    }

    result.return_eax = actor->resource_token;
    if (actor->resource_token == 0U) {
        result.status =
            LegacyBattleGroupBCoordinateOffsetStatus::resource_read_typed_stop;
        return result;
    }

    result.first_value = read_word(actor->resource_bytes, 0x62U);
    result.return_edx = replace_low_word(result.return_edx, result.first_value);
    result.return_eax = request.first_output_token;
    if (first_output == nullptr) {
        result.status =
            LegacyBattleGroupBCoordinateOffsetStatus::first_output_typed_stop;
        return result;
    }
    *first_output = result.first_value;
    result.outputs_written = 1U;

    result.return_ecx = actor->resource_token;
    result.return_eax = request.second_output_token;
    result.second_value = read_word(actor->resource_bytes, 0x8AU);
    result.return_edx =
        replace_low_word(result.return_edx, result.second_value);
    if (second_output == nullptr) {
        result.status =
            LegacyBattleGroupBCoordinateOffsetStatus::second_output_typed_stop;
        return result;
    }
    *second_output = result.second_value;
    result.outputs_written = 2U;
    return result;
}

}  // namespace openswd3::battle
