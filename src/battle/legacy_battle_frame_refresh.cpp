#include "openswd3/battle/legacy_battle_frame_refresh.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_effect_frame.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <initializer_list>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;

constexpr u32 kCallServeAudio = 0x00485330U;
constexpr u32 kCallLockSurface = 0x00416F10U;
constexpr u32 kCallUnlockSurface = 0x00416F60U;
constexpr u32 kCallPrepareViewport = 0x004170E0U;
constexpr u32 kCallApplyRed = 0x00420560U;
constexpr u32 kCallApplyGreen = 0x00420600U;
constexpr u32 kCallApplyBlue = 0x004206F0U;

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

[[nodiscard]] constexpr u32
replace_low_word(const u32 original, const u16 value) noexcept {
    return (original & 0xFFFF0000U) | value;
}

[[nodiscard]] constexpr i32 signed_half(const u16 value) noexcept {
    const i32 signed_value = static_cast<i32>(std::bit_cast<i16>(value));
    return signed_value >= 0 ? signed_value / 2 : -((-signed_value + 1) / 2);
}

[[nodiscard]] constexpr u32 to_bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

template <typename Call>
[[nodiscard]] LegacyBattleFrameRefreshResult refresh_impl(
    LegacyBattleFrameRefreshState& state,
    Call&& call,
    const u16 current_word_36,
    const u16 current_word_38,
    const u16 current_word_3a
) {
    LegacyBattleFrameRefreshResult result;
    Registers registers{
        .eax = replace_low_word(state.entry_eax, state.snapshot_word_36),
        .ecx = state.entry_ecx,
        .edx = state.entry_edx,
    };
    if (state.snapshot_word_36 == current_word_36) {
        registers.ecx = replace_low_word(registers.ecx, state.snapshot_word_38);
        if (state.snapshot_word_38 == current_word_38) {
            registers.edx =
                replace_low_word(registers.edx, state.snapshot_word_3a);
            if (state.snapshot_word_3a == current_word_3a) {
                result.return_value = registers.eax;
                result.final_ecx = registers.ecx;
                result.final_edx = registers.edx;
                state.entry_eax = registers.eax;
                state.entry_ecx = registers.ecx;
                state.entry_edx = registers.edx;
                return result;
            }
        }
    }

    result.refreshed = true;
    for (u32 factor = 1U; factor <= 2U; ++factor) {
        registers = call(kCallServeAudio, {}, registers, result);

        registers.eax = state.surface_tokens[factor - 1U];
        registers = call(kCallLockSurface, {registers.eax}, registers, result);
        state.last_lock_token = registers.eax;

        registers.ecx = state.surface_tokens[factor - 1U];
        registers = call(
            kCallUnlockSurface,
            {registers.ecx, state.last_lock_token},
            registers,
            result
        );

        registers.edx = state.source_pitch;
        state.captured_pitch = registers.edx;
        registers = call(
            kCallPrepareViewport,
            {0U, 0U, 640U, 480U, 0U, 0U},
            registers,
            result
        );

        registers.eax = to_bits(signed_half(current_word_36)) * factor;
        registers.ecx = state.last_lock_token;
        registers = call(
            kCallApplyRed,
            {registers.ecx, 0x0003C000U, registers.eax},
            registers,
            result
        );

        registers.edx = to_bits(signed_half(current_word_38)) * factor;
        registers.eax = state.last_lock_token;
        registers = call(
            kCallApplyGreen,
            {registers.eax, 0x0003C000U, registers.edx},
            registers,
            result
        );

        registers.ecx = to_bits(signed_half(current_word_3a)) * factor;
        registers.edx = state.last_lock_token;
        registers = call(
            kCallApplyBlue,
            {registers.edx, 0x0003C000U, registers.ecx},
            registers,
            result
        );
        ++result.surface_iterations;
    }

    registers.ecx = replace_low_word(registers.ecx, current_word_38);
    registers.eax = replace_low_word(registers.eax, current_word_36);
    registers.edx = replace_low_word(registers.edx, current_word_3a);
    state.snapshot_word_38 = current_word_38;
    state.snapshot_word_36 = current_word_36;
    state.snapshot_word_3a = current_word_3a;

    registers.ecx = state.viewport_token;
    registers.eax = state.final_surface_token;
    state.refresh_pending = 1U;
    state.active_surface_token = registers.eax;
    registers = call(kCallLockSurface, {registers.ecx}, registers, result);
    state.last_lock_token = registers.eax;

    registers.edx = state.viewport_token;
    registers = call(
        kCallUnlockSurface,
        {registers.edx, state.last_lock_token},
        registers,
        result
    );
    result.return_value = registers.eax;
    result.final_ecx = registers.ecx;
    result.final_edx = registers.edx;
    state.entry_eax = registers.eax;
    state.entry_ecx = registers.ecx;
    state.entry_edx = registers.edx;
    return result;
}

[[nodiscard]] std::array<u32, 8>
action_arguments(const std::initializer_list<u32> values) noexcept {
    std::array<u32, 8> result{};
    std::copy(values.begin(), values.end(), result.begin());
    return result;
}

[[nodiscard]] std::array<u32, 12>
effect_arguments(const std::initializer_list<u32> values) noexcept {
    std::array<u32, 12> result{};
    std::copy(values.begin(), values.end(), result.begin());
    return result;
}

}  // namespace

LegacyBattleFrameRefreshResult refresh_legacy_battle_frame(
    LegacyBattleActionDispatchPort& port,
    const compat::u16 current_word_36,
    const compat::u16 current_word_38,
    const compat::u16 current_word_3a
) {
    auto call = [&port](
                    const u32 callee,
                    const std::initializer_list<u32> arguments,
                    const Registers registers,
                    LegacyBattleFrameRefreshResult& result
                ) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = action_arguments(arguments),
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        return Registers{
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
        };
    };
    return refresh_impl(
        port.frame_refresh_state(),
        call,
        current_word_36,
        current_word_38,
        current_word_3a
    );
}

LegacyBattleFrameRefreshResult refresh_legacy_battle_frame(
    LegacyBattleEffectCallPort& port,
    const compat::u16 current_word_36,
    const compat::u16 current_word_38,
    const compat::u16 current_word_3a
) {
    auto call = [&port](
                    const u32 callee,
                    const std::initializer_list<u32> arguments,
                    const Registers registers,
                    LegacyBattleFrameRefreshResult& result
                ) {
        ++result.port_calls;
        const auto reply = port.invoke({
            .callee_token = callee,
            .arguments = effect_arguments(arguments),
            .eax = registers.eax,
            .ecx = registers.ecx,
            .edx = registers.edx,
        });
        return Registers{
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
        };
    };
    return refresh_impl(
        port.frame_refresh_state(),
        call,
        current_word_36,
        current_word_38,
        current_word_3a
    );
}

}  // namespace openswd3::battle
