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

}  // namespace

LegacyBattleRowOffsetResult rebuild_legacy_battle_primary_row_offsets(
    LegacyBattleRenderGeometry& geometry,
    const compat::i32 row_stride,
    const compat::i32 row_count,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept {
    geometry.primary_row_offsets.reset();

    const compat::u32 requested_bytes =
        std::bit_cast<compat::u32>(row_count) * 4U;
    LegacyBattleRowOffsetAllocation allocation =
        allocator.allocate(requested_bytes);
    geometry.primary_row_offsets = std::move(allocation.words);
    if (geometry.primary_row_offsets == nullptr) {
        return {
            .status = LegacyBattleRowOffsetStatus::allocation_failed,
            .requested_bytes = requested_bytes,
            .legacy_return_value = 0U,
        };
    }

    geometry.primary_row_stride = row_stride;
    geometry.primary_row_count = row_count;
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
        geometry.primary_row_offsets[row_index - 1U] = byte_offset;
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
