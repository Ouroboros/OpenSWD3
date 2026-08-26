#include "openswd3/battle/legacy_battle_animation_collision.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 unsigned_dword(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr u32
wrapping_add(const u32 left, const u32 right) noexcept {
    return left + right;
}

[[nodiscard]] constexpr u32
scaled_step_count(const u16 counter, const u32 multiplier) noexcept {
    return static_cast<u32>(counter) * multiplier;
}

}  // namespace

LegacyBattleAnimationCollisionResult advance_legacy_battle_animation_collision(
    LegacyBattleAnimationCollisionState& state,
    const LegacyBattleAnimationCollisionRequest& request
) noexcept {
    LegacyBattleAnimationCollisionResult result;
    if (request.counter_index >= state.animation_collision_counter.size()) {
        result.status =
            LegacyBattleAnimationCollisionStatus::counter_index_typed_stop;
        return result;
    }

    auto& counter = state.animation_collision_counter[request.counter_index];
    counter = static_cast<u16>(counter + 1U);
    result.counter_after = counter;

    LegacyBattleLineRaster raster{
        .start_x = signed_dword(request.start_x),
        .start_y = signed_dword(request.start_y),
        .end_x = signed_dword(request.end_x),
        .end_y = signed_dword(request.end_y),
    };

    u32 iteration_index = 0U;
    u32 product = scaled_step_count(counter, request.step_multiplier);
    if (signed_dword(product) > 0) {
        for (;;) {
            static_cast<void>(advance_legacy_battle_line_raster(raster));
            ++result.line_raster_calls;

            const u32 current_x =
                wrapping_add(unsigned_dword(raster.current_x), request.start_x);
            if (current_x == request.end_x) {
                result.target_reached = true;
                iteration_index = 0xFFFFFFFFU;
                break;
            }

            iteration_index += 1U;
            product = scaled_step_count(counter, request.step_multiplier);
            if (signed_dword(iteration_index) >= signed_dword(product)) {
                break;
            }
        }
    }

    const u32 output_x =
        wrapping_add(unsigned_dword(raster.current_x), request.start_x);
    const u32 output_y =
        wrapping_add(unsigned_dword(raster.current_y), request.start_y);
    state.shared_x = signed_dword(output_x);
    state.shared_y = signed_dword(output_y);

    result.iteration_index = iteration_index;
    result.return_ecx = output_y;
    result.return_edx = request.start_y;
    if (iteration_index == 0xFFFFFFFFU) {
        counter = 0U;
        result.counter_after = 0U;
        result.counter_cleared = true;
        result.return_eax = 1U;
        return result;
    }

    result.return_eax = 0U;
    return result;
}

}  // namespace openswd3::battle
