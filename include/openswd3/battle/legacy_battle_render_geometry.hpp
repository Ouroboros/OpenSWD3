#pragma once

#include "openswd3/compat/types.hpp"

#include <memory>

namespace openswd3::battle {

struct LegacyBattleRenderGeometry {
    std::unique_ptr<compat::u32[]> primary_row_offsets{};
    std::unique_ptr<compat::u32[]> surface_row_offsets{};
    compat::i32 primary_row_stride{};
    compat::i32 primary_row_count{};
    compat::i32 surface_width{};
    compat::i32 surface_height{};
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};
};

struct LegacyBattleRowOffsetAllocation {
    std::unique_ptr<compat::u32[]> words{};
    compat::u32 word_capacity{};
};

class LegacyBattleRowOffsetAllocator {
public:
    virtual ~LegacyBattleRowOffsetAllocator() = default;
    [[nodiscard]] virtual LegacyBattleRowOffsetAllocation
    allocate(compat::u32 requested_bytes) noexcept = 0;
};

enum class LegacyBattleRowOffsetStatus : compat::u8 {
    completed,
    allocation_failed,
    write_out_of_range,
};

struct LegacyBattleRowOffsetResult {
    LegacyBattleRowOffsetStatus status{LegacyBattleRowOffsetStatus::completed};
    compat::u32 requested_bytes{};
    compat::u32 legacy_return_value{};
};

// sub_433E20.
[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count
) noexcept;

// sub_433E90.
[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_surface_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleRowOffsetResult
rebuild_legacy_battle_surface_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 row_stride,
    compat::i32 row_count
) noexcept;

// sub_4342E0. The final two parameters are dimensions, not absolute edges.
compat::i32 set_legacy_battle_render_rectangle(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 left,
    compat::i32 top,
    compat::i32 width,
    compat::i32 height
) noexcept;

}  // namespace openswd3::battle
