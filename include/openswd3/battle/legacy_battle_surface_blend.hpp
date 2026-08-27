#pragma once

#include <array>
#include <optional>

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleSurfaceBlendOwnerToken = 0x004AB870U;
inline constexpr compat::u32 kLegacyBattleSurfaceBlendFormat = 0x2711U;
inline constexpr compat::u32 kLegacyBattleSurfaceBlendWidth = 640U;
inline constexpr compat::u32 kLegacyBattleSurfaceBlendRows = 480U;

struct LegacyBattleSurfaceBlendRectangle {
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};

    [[nodiscard]] bool operator==(
        const LegacyBattleSurfaceBlendRectangle& other
    ) const noexcept = default;
};

enum class LegacyBattleSurfaceBlendOperationKind : compat::u8 {
    capture_secondary,
    blend_primary_row,
    copy_screen_to_temporary,
    vertical_shift_frame,
};

struct LegacyBattleSurfaceBlendOperation {
    LegacyBattleSurfaceBlendOperationKind kind{
        LegacyBattleSurfaceBlendOperationKind::capture_secondary
    };
    compat::u32 object_token{};
    std::optional<LegacyBattleSurfaceBlendRectangle> destination_rectangle;
    compat::u32 source_token{};
    std::optional<LegacyBattleSurfaceBlendRectangle> source_rectangle;
    compat::u32 flags{};
    compat::u32 trailing_zero_a{};
    compat::u32 trailing_zero_b{};
};

class LegacyBattleSurfaceBlendPort {
public:
    virtual ~LegacyBattleSurfaceBlendPort() = default;

    [[nodiscard]] virtual compat::i32
    query_system_metric(compat::i32 index) = 0;
    [[nodiscard]] virtual compat::u32 create_screen_surface(
        compat::u32 owner_token, compat::i32 width, compat::i32 height
    ) = 0;
    [[nodiscard]] virtual compat::u32
    create_temporary_surface(compat::u32 owner_token, compat::u32 format) = 0;
    [[nodiscard]] virtual compat::u32 random_below(compat::u32 bound) = 0;
    [[nodiscard]] virtual compat::u32
    operate_surface(const LegacyBattleSurfaceBlendOperation& operation) = 0;
    [[nodiscard]] virtual compat::u32
    release_surface(compat::u32 surface_token) = 0;
};

struct LegacyBattleSurfaceBlendRequest {
    compat::u32 primary_surface_token{};
    compat::u32 secondary_surface_token{};
    std::array<compat::u32, 4> ignored_arguments{};
};

struct LegacyBattleSurfaceBlendState {
    compat::u32 screen_surface_token{};
    std::array<compat::u32, kLegacyBattleSurfaceBlendRows>
        unused_random_table{};
    std::array<compat::i32, kLegacyBattleSurfaceBlendRows> row_offsets{};
    compat::u32 completed_row_observations{};
    compat::i32 screen_width{};
    compat::i32 screen_height{};
};

enum class LegacyBattleSurfaceBlendStatus : compat::u8 {
    completed,
    screen_surface_typed_stop,
    temporary_surface_typed_stop,
};

struct LegacyBattleSurfaceBlendResult {
    LegacyBattleSurfaceBlendStatus status{
        LegacyBattleSurfaceBlendStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 metric_queries{};
    compat::u32 random_calls{};
    compat::u32 unused_random_writes{};
    compat::u32 outer_passes{};
    compat::u32 secondary_capture_calls{};
    compat::u32 row_operation_calls{};
    compat::u32 rectangle_pairs{};
    compat::u32 temporary_surface_calls{};
    compat::u32 temporary_copy_calls{};
    compat::u32 release_calls{};
};

[[nodiscard]] LegacyBattleSurfaceBlendResult run_legacy_battle_surface_blend(
    LegacyBattleSurfaceBlendState& state,
    LegacyBattleSurfaceBlendPort& port,
    const LegacyBattleSurfaceBlendRequest& request
);

}  // namespace openswd3::battle
