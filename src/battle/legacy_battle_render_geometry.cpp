#include "openswd3/battle/legacy_battle_render_geometry.hpp"

#include <bit>
#include <new>
#include <utility>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

class DefaultRowOffsetAllocator final : public LegacyBattleRowOffsetAllocator {
public:
    [[nodiscard]] LegacyBattleRowOffsetAllocation
    allocate(const compat::u32 requested_bytes) noexcept override {
        const compat::u32 word_capacity = requested_bytes / 4U;
        std::unique_ptr<compat::u32[]> words{new (std::nothrow)
                                                 compat::u32[word_capacity]};
        if (words == nullptr) {
            return {};
        }
        return {
            .words = std::move(words),
            .word_capacity = word_capacity,
        };
    }
};

[[nodiscard]] LegacyBattleRowOffsetResult rebuild_row_offsets(
    std::unique_ptr<compat::u32[]>& row_offsets,
    compat::i32& published_stride,
    compat::i32& published_count,
    const compat::i32 row_stride,
    const compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept {
    row_offsets.reset();

    const compat::u32 requested_bytes =
        std::bit_cast<compat::u32>(row_count) * 4U;
    LegacyBattleRowOffsetAllocation allocation =
        allocator.allocate(requested_bytes);
    row_offsets = std::move(allocation.words);
    if (row_offsets == nullptr) {
        return {
            .status = LegacyBattleRowOffsetStatus::allocation_failed,
            .requested_bytes = requested_bytes,
            .legacy_return_value = 0U,
        };
    }

    published_stride = row_stride;
    published_count = row_count;
    if (row_count <= 0) {
        return {
            .status = LegacyBattleRowOffsetStatus::completed,
            .requested_bytes = requested_bytes,
            .legacy_return_value = 0U,
        };
    }

    compat::u32 row_index{};
    compat::u32 byte_offset{};
    while (true) {
        ++row_index;
        if (row_index - 1U >= allocation.word_capacity) {
            return {
                .status = LegacyBattleRowOffsetStatus::write_out_of_range,
                .requested_bytes = requested_bytes,
                .legacy_return_value = row_index,
            };
        }
        row_offsets[row_index - 1U] = byte_offset;
        byte_offset += std::bit_cast<compat::u32>(row_stride);
        if (static_cast<compat::i32>(row_index) >= row_count) {
            break;
        }
    }

    return {
        .status = LegacyBattleRowOffsetStatus::completed,
        .requested_bytes = requested_bytes,
        .legacy_return_value = row_index,
    };
}

}  // namespace

LegacyBattleRowOffsetResult rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 row_stride,
    const compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept {
    return rebuild_row_offsets(
        geometry.primary_row_offsets,
        geometry.primary_row_stride,
        geometry.primary_row_count,
        row_stride,
        row_count,
        allocator
    );
}

LegacyBattleRowOffsetResult rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 row_stride,
    const compat::i32 row_count
) noexcept {
    DefaultRowOffsetAllocator allocator;
    return rebuild_legacy_battle_primary_row_offsets(
        geometry, row_stride, row_count, allocator
    );
}

LegacyBattleRowOffsetResult rebuild_legacy_battle_surface_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 row_stride,
    const compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept {
    return rebuild_row_offsets(
        geometry.surface_row_offsets,
        geometry.surface_width,
        geometry.surface_height,
        row_stride,
        row_count,
        allocator
    );
}

LegacyBattleRowOffsetResult rebuild_legacy_battle_surface_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 row_stride,
    const compat::i32 row_count
) noexcept {
    DefaultRowOffsetAllocator allocator;
    return rebuild_legacy_battle_surface_row_offsets(
        geometry, row_stride, row_count, allocator
    );
}

LegacyBattleHostSurfaceResult set_legacy_battle_host_surface(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 surface_width,
    const compat::i32 surface_height,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept {
    geometry.surface_width = surface_width;
    geometry.surface_height = surface_height;

    LegacyBattleRowOffsetResult row_offsets =
        rebuild_legacy_battle_surface_row_offsets(
            geometry, surface_width, surface_height, allocator
        );
    if (row_offsets.status == LegacyBattleRowOffsetStatus::write_out_of_range) {
        return {
            .row_offsets = row_offsets,
            .rectangle_published = false,
            .legacy_return_value =
                std::bit_cast<compat::i32>(row_offsets.legacy_return_value),
        };
    }

    const compat::i32 bottom = set_legacy_battle_render_rectangle(
        geometry, 0, 0, surface_width, surface_height
    );
    return {
        .row_offsets = row_offsets,
        .rectangle_published = true,
        .legacy_return_value = bottom,
    };
}

LegacyBattleHostSurfaceResult set_legacy_battle_host_surface(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 surface_width,
    const compat::i32 surface_height
) noexcept {
    DefaultRowOffsetAllocator allocator;
    return set_legacy_battle_host_surface(
        geometry, surface_width, surface_height, allocator
    );
}

compat::i32 set_legacy_battle_render_rectangle(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 left,
    compat::i32 top,
    compat::i32 width,
    compat::i32 height
) noexcept {
    if (left < 0) {
        width = wrapping_add(width, left);
        left = 0;
    }
    if (top < 0) {
        height = wrapping_add(height, top);
        top = 0;
    }

    if (wrapping_add(left, width) >= geometry.surface_width) {
        left = wrapping_subtract(geometry.surface_width, width);
    }
    if (wrapping_add(top, height) >= geometry.surface_height) {
        top = wrapping_subtract(geometry.surface_height, height);
    }

    geometry.top = top;
    geometry.left = left;
    geometry.right = wrapping_add(left, width);
    geometry.bottom = wrapping_add(top, height);
    return geometry.bottom;
}

}  // namespace openswd3::battle
