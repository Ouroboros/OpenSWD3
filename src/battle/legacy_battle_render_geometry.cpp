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

[[nodiscard]] compat::i32 wrapping_negate(const compat::i32 value) noexcept {
    return std::bit_cast<compat::i32>(0U - std::bit_cast<compat::u32>(value));
}

[[nodiscard]] compat::i32
arithmetic_shift_right_one(const compat::i32 value) noexcept {
    compat::u32 shifted = std::bit_cast<compat::u32>(value) >> 1U;
    if (value < 0) {
        shifted |= 0x80000000U;
    }
    return std::bit_cast<compat::i32>(shifted);
}

// Exact low-dword results from the original x87 sequence:
// fild(index), fmul(0x3F91DCF4D98B0955), fptan, fmul(-1000.0f),
// then the truncation-control-word conversion at sub_489654.
inline constexpr std::array<compat::i32, 90> kNegativeTangentBase{
    0,     -17,   -34,   -52,   -69,   -87,    -105,   -122,   -140,   -158,
    -176,  -194,  -212,  -230,  -249,  -267,   -286,   -305,   -324,   -344,
    -363,  -383,  -403,  -424,  -444,  -466,   -487,   -509,   -531,   -553,
    -576,  -600,  -624,  -648,  -674,  -699,   -726,   -753,   -780,   -809,
    -838,  -868,  -899,  -931,  -964,  -999,   -1034,  -1071,  -1109,  -1149,
    -1190, -1233, -1278, -1325, -1374, -1426,  -1480,  -1538,  -1598,  -1662,
    -1729, -1801, -1878, -1959, -2047, -2141,  -2242,  -2351,  -2470,  -2600,
    -2742, -2898, -3071, -3263, -3478, -3722,  -3999,  -4318,  -4688,  -5125,
    -5647, -6284, -7078, -8095, -9446, -11331, -14145, -18804, -28010, -54816,
};

void publish_direction_vectors(LegacyBattleDirectionVectors& vectors) noexcept {
    for (std::size_t index = 0; index < kNegativeTangentBase.size(); ++index) {
        vectors.horizontal[index] = -1000;
        vectors.vertical[index] = kNegativeTangentBase[index];
    }

    for (std::size_t index = 0; index < 89U; ++index) {
        const std::size_t destination = 91U + index;
        vectors.horizontal[destination] = 1000;
        vectors.vertical[destination] = kNegativeTangentBase[88U - index];
    }

    for (std::size_t index = 0; index < kNegativeTangentBase.size(); ++index) {
        const std::size_t destination = 180U + index;
        vectors.horizontal[destination] = 1000;
        vectors.vertical[destination] =
            wrapping_negate(kNegativeTangentBase[index]);
    }

    for (std::size_t index = 0; index < 89U; ++index) {
        const std::size_t destination = 271U + index;
        vectors.horizontal[destination] = -1000;
        vectors.vertical[destination] =
            wrapping_negate(kNegativeTangentBase[88U - index]);
    }

    vectors.horizontal[90U] = 0;
    vectors.vertical[90U] = -100000;
    vectors.horizontal[180U] = 100000;
    vectors.vertical[180U] = 0;
    vectors.horizontal[270U] = 0;
    vectors.vertical[270U] = 100000;
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

bool release_legacy_battle_render_auxiliary_buffer(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRenderAuxiliaryBufferReleaser& releaser
) noexcept {
    const compat::u32 token = geometry.auxiliary_buffer_token;
    if (token == 0U) {
        return false;
    }

    releaser.release(token);
    geometry.auxiliary_buffer_token = 0U;
    return true;
}

bool advance_legacy_battle_line_raster(
    LegacyBattleLineRaster& raster
) noexcept {
    compat::i32 horizontal_distance =
        wrapping_subtract(raster.end_x, raster.start_x);
    compat::i32 vertical_distance =
        wrapping_subtract(raster.end_y, raster.start_y);
    compat::i32 horizontal_step = 1;
    compat::i32 vertical_step = 1;

    if (horizontal_distance < 0) {
        horizontal_step = -1;
        horizontal_distance = wrapping_negate(horizontal_distance);
    }
    if (vertical_distance < 0) {
        vertical_step = -1;
        vertical_distance = wrapping_negate(vertical_distance);
    }

    if (horizontal_distance == 0) {
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
    } else if (vertical_distance == 0) {
        raster.current_x = wrapping_add(raster.current_x, horizontal_step);
    } else if (horizontal_distance < vertical_distance) {
        raster.x_error = wrapping_add(raster.x_error, horizontal_distance);
        if (raster.x_error > arithmetic_shift_right_one(vertical_distance)) {
            raster.x_error =
                wrapping_subtract(raster.x_error, vertical_distance);
            raster.current_x = wrapping_add(raster.current_x, horizontal_step);
        }
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
    } else if (horizontal_distance == vertical_distance) {
        raster.current_x = wrapping_add(raster.current_x, horizontal_step);
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
    } else {
        raster.y_error = wrapping_add(raster.y_error, vertical_distance);
        if (raster.y_error > arithmetic_shift_right_one(horizontal_distance)) {
            raster.y_error =
                wrapping_subtract(raster.y_error, horizontal_distance);
            raster.current_y = wrapping_add(raster.current_y, vertical_step);
        }
        raster.current_x = wrapping_add(raster.current_x, horizontal_step);
    }

    return raster.current_x == raster.end_x && raster.current_y == raster.end_y;
}

LegacyBattleDirectionStepStatus advance_legacy_battle_direction_raster(
    const LegacyBattleDirectionVectors& vectors,
    LegacyBattleDirectionRaster& raster
) noexcept {
    const compat::u32 direction_index =
        std::bit_cast<compat::u32>(raster.direction_index);
    if (direction_index >= kLegacyBattleDirectionCount) {
        return LegacyBattleDirectionStepStatus::direction_index_out_of_range;
    }

    compat::i32 horizontal_distance = vectors.horizontal[direction_index];
    compat::i32 vertical_distance = vectors.vertical[direction_index];
    compat::i32 horizontal_step = 1;
    compat::i32 vertical_step = 1;

    if (horizontal_distance < 0) {
        horizontal_step = -1;
        horizontal_distance = wrapping_negate(horizontal_distance);
    }
    if (vertical_distance < 0) {
        vertical_step = -1;
        vertical_distance = wrapping_negate(vertical_distance);
    }

    if (horizontal_distance == 0) {
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
        return LegacyBattleDirectionStepStatus::completed;
    }
    if (vertical_distance == 0) {
        raster.current_x = wrapping_add(raster.current_x, horizontal_step);
        return LegacyBattleDirectionStepStatus::completed;
    }
    if (horizontal_distance == vertical_distance) {
        raster.current_x = wrapping_add(raster.current_x, horizontal_step);
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
        return LegacyBattleDirectionStepStatus::completed;
    }
    if (horizontal_distance < vertical_distance) {
        raster.x_error = wrapping_add(raster.x_error, horizontal_distance);
        if (raster.x_error > arithmetic_shift_right_one(vertical_distance)) {
            raster.x_error =
                wrapping_subtract(raster.x_error, vertical_distance);
            raster.current_x = wrapping_add(raster.current_x, horizontal_step);
        }
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
        return LegacyBattleDirectionStepStatus::completed;
    }

    raster.y_error = wrapping_add(raster.y_error, vertical_distance);
    if (raster.y_error > arithmetic_shift_right_one(horizontal_distance)) {
        raster.y_error = wrapping_subtract(raster.y_error, horizontal_distance);
        raster.current_y = wrapping_add(raster.current_y, vertical_step);
    }
    raster.current_x = wrapping_add(raster.current_x, horizontal_step);
    return LegacyBattleDirectionStepStatus::completed;
}

LegacyBattleRenderInitializationResult initialize_legacy_battle_render_geometry(
    LegacyBattleRenderGeometry& geometry,
    LegacyBattleRowOffsetAllocator& allocator
) noexcept {
    // The legacy routine zeroes both published pointers before calling either
    // rebuild helper, so the helpers cannot release any previous allocations.
    static_cast<void>(geometry.primary_row_offsets.release());
    static_cast<void>(geometry.surface_row_offsets.release());

    LegacyBattleRenderInitializationResult result;
    result.primary_row_offsets = rebuild_legacy_battle_primary_row_offsets(
        geometry, 0x500, 0x300, allocator
    );
    if (result.primary_row_offsets.status ==
        LegacyBattleRowOffsetStatus::write_out_of_range) {
        result.status = LegacyBattleRenderInitializationStatus::
            primary_row_offsets_write_out_of_range;
        return result;
    }

    result.surface_row_offsets = rebuild_legacy_battle_surface_row_offsets(
        geometry, 0x280, 0x1E0, allocator
    );
    if (result.surface_row_offsets.status ==
        LegacyBattleRowOffsetStatus::write_out_of_range) {
        result.status = LegacyBattleRenderInitializationStatus::
            surface_row_offsets_write_out_of_range;
        return result;
    }

    static_cast<void>(
        set_legacy_battle_render_rectangle(geometry, 0, 0, 0x280, 0x1E0)
    );
    result.rectangle_published = true;

    publish_direction_vectors(geometry.direction_vectors);
    result.direction_vectors_published = true;
    result.legacy_return_value = &geometry;
    return result;
}

LegacyBattleRenderInitializationResult initialize_legacy_battle_render_geometry(
    LegacyBattleRenderGeometry& geometry
) noexcept {
    DefaultRowOffsetAllocator allocator;
    return initialize_legacy_battle_render_geometry(geometry, allocator);
}

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
