#include "openswd3/battle/legacy_battle_surface_blend.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(left) + static_cast<u32>(right));
}

[[nodiscard]] constexpr i32 row_delta(const u32 random_value) noexcept {
    return std::bit_cast<i32>(static_cast<u32>(-30) - (random_value << 1U));
}

[[nodiscard]] constexpr bool
signed_less_equal(const u32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(left) <= right;
}

[[nodiscard]] u32 random_below(
    LegacyBattleSurfaceBlendPort& port,
    LegacyBattleSurfaceBlendResult& result,
    const u32 bound
) {
    ++result.random_calls;
    return port.random_below(bound);
}

[[nodiscard]] u32 operate(
    LegacyBattleSurfaceBlendPort& port,
    const LegacyBattleSurfaceBlendOperation& operation
) {
    return port.operate_surface(operation);
}

}  // namespace

LegacyBattleSurfaceBlendResult run_legacy_battle_surface_blend(
    LegacyBattleSurfaceBlendState& state,
    LegacyBattleSurfaceBlendPort& port,
    const LegacyBattleSurfaceBlendRequest& request
) {
    LegacyBattleSurfaceBlendResult result;

    state.completed_row_observations = 0U;
    state.screen_height = port.query_system_metric(1);
    ++result.metric_queries;
    state.screen_width = port.query_system_metric(0);
    ++result.metric_queries;
    state.screen_surface_token = port.create_screen_surface(
        kLegacyBattleSurfaceBlendOwnerToken,
        state.screen_width,
        state.screen_height
    );

    state.row_offsets.fill(0);
    for (u32 index = 0U; index < kLegacyBattleSurfaceBlendRows; ++index) {
        state.unused_random_table[index] =
            random_below(port, result, 20U) + 15U;
        ++result.unused_random_writes;
    }

    if (state.screen_surface_token == 0U) {
        result.status =
            LegacyBattleSurfaceBlendStatus::screen_surface_typed_stop;
        return result;
    }

    do {
        ++result.outer_passes;
        static_cast<void>(operate(
            port,
            LegacyBattleSurfaceBlendOperation{
                .kind =
                    LegacyBattleSurfaceBlendOperationKind::capture_secondary,
                .object_token = state.screen_surface_token,
                .destination_rectangle = std::nullopt,
                .source_token = request.secondary_surface_token,
                .source_rectangle = std::nullopt,
                .trailing_zero_a = 0U,
                .trailing_zero_b = 0U,
            }
        ));
        ++result.secondary_capture_calls;

        for (i32 row = 479; row >= 0; --row) {
            const std::size_t index = static_cast<std::size_t>(row);
            state.row_offsets[index] = wrapping_add(
                state.row_offsets[index],
                row_delta(random_below(port, result, 20U))
            );
            if (state.row_offsets[index] <= 0) {
                state.row_offsets[index] = 0;
                ++state.completed_row_observations;
            }

            const LegacyBattleSurfaceBlendRectangle rectangle{
                .left = state.row_offsets[index],
                .top = row,
                .right = static_cast<i32>(kLegacyBattleSurfaceBlendWidth),
                .bottom = row + 1,
            };
            static_cast<void>(operate(
                port,
                LegacyBattleSurfaceBlendOperation{
                    .kind = LegacyBattleSurfaceBlendOperationKind::
                        blend_primary_row,
                    .object_token = state.screen_surface_token,
                    .destination_rectangle = rectangle,
                    .source_token = request.primary_surface_token,
                    .source_rectangle = rectangle,
                    .trailing_zero_a = 0U,
                    .trailing_zero_b = 0U,
                }
            ));
            ++result.row_operation_calls;
            ++result.rectangle_pairs;
        }

        const u32 temporary_surface = port.create_temporary_surface(
            kLegacyBattleSurfaceBlendOwnerToken, kLegacyBattleSurfaceBlendFormat
        );
        ++result.temporary_surface_calls;
        if (temporary_surface == 0U) {
            result.status =
                LegacyBattleSurfaceBlendStatus::temporary_surface_typed_stop;
            return result;
        }
        static_cast<void>(operate(
            port,
            LegacyBattleSurfaceBlendOperation{
                .kind = LegacyBattleSurfaceBlendOperationKind::
                    copy_screen_to_temporary,
                .object_token = temporary_surface,
                .destination_rectangle = std::nullopt,
                .source_token = state.screen_surface_token,
                .source_rectangle = std::nullopt,
                .trailing_zero_a = 0U,
                .trailing_zero_b = 0U,
            }
        ));
        ++result.temporary_copy_calls;
    } while (signed_less_equal(state.completed_row_observations, 480));

    result.return_value = port.release_surface(state.screen_surface_token);
    ++result.release_calls;
    return result;
}

}  // namespace openswd3::battle
