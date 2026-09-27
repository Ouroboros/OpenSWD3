#pragma once

#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr bool even_parity(const u8 value) noexcept {
    return (std::popcount(value) & 1) == 0;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_result_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
logical_zero_flags() noexcept {
    return logical_result_flags(0U);
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left - right;
    return {
        .carry = left < right,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags_16(const u16 left, const u16 right) noexcept {
    const u16 value = static_cast<u16>(left - right);
    return {
        .carry = left < right,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x8000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags_16(const u16 left, const u16 right) noexcept {
    const u16 value = static_cast<u16>(left + right);
    return {
        .carry = static_cast<u32>(left) + right > 0xFFFFU,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x8000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left + right;
    return {
        .carry = value < left,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ value)) & 0x80000000U) != 0U,
    };
}

}  // namespace
}  // namespace openswd3::battle
