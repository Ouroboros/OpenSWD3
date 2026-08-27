#include "openswd3/battle/legacy_battle_vertical_shift.hpp"

#include <array>
#include <bit>
#include <optional>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

constexpr std::array<i32, 16> kSignedRowOffsets{
    4,
    3,
    4,
    3,
    4,
    3,
    2,
    3,
    2,
    3,
    2,
    1,
    2,
    1,
    2,
    0,
};

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 signed_remainder_two(const u32 value) noexcept {
    u32 remainder = value & 0x80000001U;
    if (signed_bits(remainder) < 0) {
        remainder -= 1U;
        remainder |= 0xFFFFFFFEU;
        remainder += 1U;
    }
    return remainder;
}

[[nodiscard]] std::optional<i32> read_signed_offset(
    const LegacyBattleVerticalShiftState& state,
    LegacyBattleVerticalShiftResult& result
) noexcept {
    if (state.phase_index >= kSignedRowOffsets.size()) {
        result.status = LegacyBattleVerticalShiftStatus::phase_table_typed_stop;
        return std::nullopt;
    }
    const i32 value = kSignedRowOffsets[state.phase_index];
    result.signed_offsets[result.table_reads] = value;
    ++result.table_reads;
    return value;
}

[[nodiscard]] bool resolve_and_blit(
    LegacyBattleVerticalShiftPort& port,
    LegacyBattleVerticalShiftResult& result,
    const compat::u32 operation_index,
    const LegacyBattleSurfaceBlendRectangle& destination,
    const LegacyBattleSurfaceBlendRectangle& source
) {
    const u32 surface = port.resolve_vertical_shift_surface(
        kLegacyBattleVerticalShiftOwnerToken,
        kLegacyBattleVerticalShiftSurfaceSelector
    );
    ++result.surface_resolve_calls;
    if (surface == 0U) {
        result.status =
            LegacyBattleVerticalShiftStatus::primary_surface_typed_stop;
        return false;
    }
    result.operations[operation_index] = {
        .kind = LegacyBattleSurfaceBlendOperationKind::vertical_shift_frame,
        .object_token = surface,
        .destination_rectangle = destination,
        .source_token = kLegacyBattleVerticalShiftSourceToken,
        .source_rectangle = source,
        .flags = kLegacyBattleVerticalShiftFlags,
    };
    static_cast<void>(
        port.blit_vertical_shift(result.operations[operation_index])
    );
    ++result.surface_blit_calls;
    return true;
}

[[nodiscard]] bool clear_exposed_rows(
    rendering::LegacyFramebuffer& framebuffer,
    const i32 signed_offset,
    LegacyBattleVerticalShiftResult& result
) noexcept {
    const u32 offset = static_cast<u32>(signed_offset);
    const u32 bytes = offset * kLegacyBattleVerticalShiftRowBytes;
    const u32 dword_count = bytes >> 2U;
    auto pixels = framebuffer.physical_pixels();
    for (u32 index = 0U; index < dword_count; ++index) {
        const std::size_t pixel_index = static_cast<std::size_t>(index) * 2U;
        if (pixel_index + 1U >= pixels.size()) {
            result.status =
                LegacyBattleVerticalShiftStatus::framebuffer_typed_stop;
            return false;
        }
        pixels[pixel_index] = u16{0U};
        pixels[pixel_index + 1U] = u16{0U};
        result.cleared_bytes += 4U;
        result.cleared_pixels += 2U;
    }
    return true;
}

}  // namespace

LegacyBattleVerticalShiftResult run_legacy_battle_vertical_shift(
    LegacyBattleVerticalShiftPort& port,
    u32& completion_gate,
    const u32& battle_mode_flags,
    rendering::LegacyFramebuffer& framebuffer
) {
    LegacyBattleVerticalShiftResult result;
    auto& state = port.battle_vertical_shift_state();

    const u32 first_phase = state.phase_index;
    const u32 first_parity = signed_remainder_two(first_phase);
    const auto first_offset = read_signed_offset(state, result);
    if (!first_offset.has_value()) {
        return result;
    }
    const i32 first_bottom = kLegacyBattleVerticalShiftHeight - *first_offset;
    const LegacyBattleSurfaceBlendRectangle first_source{
        .left = 0,
        .top = 0,
        .right = kLegacyBattleVerticalShiftWidth,
        .bottom = first_bottom,
    };
    const LegacyBattleSurfaceBlendRectangle first_destination =
        first_parity == 0U
        ? LegacyBattleSurfaceBlendRectangle{
              .left = 0,
              .top = 0,
              .right = kLegacyBattleVerticalShiftWidth,
              .bottom = first_bottom,
          }
        : LegacyBattleSurfaceBlendRectangle{
              .left = 0,
              .top = *first_offset,
              .right = kLegacyBattleVerticalShiftWidth,
              .bottom = kLegacyBattleVerticalShiftHeight,
          };
    if (!resolve_and_blit(port, result, 0U, first_destination, first_source)) {
        return result;
    }

    const auto clear_offset = read_signed_offset(state, result);
    if (!clear_offset.has_value()) {
        return result;
    }
    if (!clear_exposed_rows(framebuffer, *clear_offset, result)) {
        return result;
    }

    const u32 second_phase = state.phase_index;
    const u32 second_parity = signed_remainder_two(second_phase);
    const auto second_offset = read_signed_offset(state, result);
    if (!second_offset.has_value()) {
        return result;
    }
    const LegacyBattleSurfaceBlendRectangle second_source{
        .left = 0,
        .top = 0,
        .right = kLegacyBattleVerticalShiftWidth,
        .bottom = *second_offset,
    };
    const LegacyBattleSurfaceBlendRectangle second_destination =
        second_parity == 0U
        ? LegacyBattleSurfaceBlendRectangle{
              .left = 0,
              .top = kLegacyBattleVerticalShiftHeight - *second_offset,
              .right = kLegacyBattleVerticalShiftWidth,
              .bottom = kLegacyBattleVerticalShiftHeight,
          }
        : LegacyBattleSurfaceBlendRectangle{
              .left = 0,
              .top = 0,
              .right = kLegacyBattleVerticalShiftWidth,
              .bottom = *second_offset,
          };
    if (!resolve_and_blit(
            port, result, 1U, second_destination, second_source
        )) {
        return result;
    }

    const u32 next_tick = state.tick_counter + 1U;
    state.tick_counter = next_tick;
    if (signed_bits(next_tick) <= signed_bits(state.tick_limit)) {
        result.return_value = state.phase_index;
        return result;
    }

    state.tick_counter = 0U;
    const u32 next_phase = (battle_mode_flags & 0x00000100U) != 0U
        ? signed_remainder_two(state.phase_index + 1U)
        : state.phase_index + 1U;
    state.phase_index = next_phase;
    result.return_value = next_phase;
    if (next_phase == 10U) {
        completion_gate = 0U;
        state.phase_index = 0U;
    }
    return result;
}

}  // namespace openswd3::battle
