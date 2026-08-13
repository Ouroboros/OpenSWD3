#include "openswd3/rendering/legacy_glyph_cache.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace openswd3::rendering {
namespace {

struct CheckedGlyphGeometry {
    std::size_t row_bytes{};
    std::size_t raster_words{};
    std::size_t slot_bytes{};
};

[[nodiscard]] bool checked_glyph_geometry(
    const compat::i32 glyph_width,
    const compat::i32 glyph_height,
    CheckedGlyphGeometry& geometry
) noexcept {
    if (glyph_width <= 0 || glyph_height <= 0) {
        return false;
    }

    const auto width = static_cast<std::uint64_t>(glyph_width);
    const auto height = static_cast<std::uint64_t>(glyph_height);
    const std::uint64_t row_bytes = (width + 7U) / 8U;
    const std::uint64_t raster_words = width * height;
    const std::uint64_t slot_bytes = row_bytes * height;
    const std::uint64_t maximum_size =
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());

    if (row_bytes > maximum_size || raster_words > maximum_size ||
        slot_bytes > maximum_size) {
        return false;
    }

    geometry = CheckedGlyphGeometry{
        .row_bytes = static_cast<std::size_t>(row_bytes),
        .raster_words = static_cast<std::size_t>(raster_words),
        .slot_bytes = static_cast<std::size_t>(slot_bytes),
    };
    return true;
}

[[nodiscard]] std::size_t checked_cache_storage_bytes(
    const compat::i32 glyph_width,
    const compat::i32 glyph_height,
    CheckedGlyphGeometry& geometry
) {
    if (!checked_glyph_geometry(glyph_width, glyph_height, geometry) ||
        geometry.slot_bytes > std::numeric_limits<std::size_t>::max() /
                kLegacyGlyphCacheSlotCapacity) {
        throw std::invalid_argument("invalid legacy glyph cache geometry");
    }

    return geometry.slot_bytes * kLegacyGlyphCacheSlotCapacity;
}

}  // namespace

LegacyGlyphMaskPackStatus pack_legacy_glyph_mask(
    const std::span<const compat::u16> tightly_packed_raster,
    const compat::i32 glyph_width,
    const compat::i32 glyph_height,
    const std::span<compat::u8> destination
) noexcept {
    CheckedGlyphGeometry geometry{};
    if (!checked_glyph_geometry(glyph_width, glyph_height, geometry)) {
        return LegacyGlyphMaskPackStatus::invalid_geometry;
    }
    if (tightly_packed_raster.size() < geometry.raster_words) {
        return LegacyGlyphMaskPackStatus::source_out_of_bounds;
    }
    if (destination.size() < geometry.slot_bytes) {
        return LegacyGlyphMaskPackStatus::destination_out_of_bounds;
    }

    const auto width = static_cast<std::size_t>(glyph_width);
    const auto height = static_cast<std::size_t>(glyph_height);
    for (std::size_t row = 0U; row < height; ++row) {
        const std::size_t source_row = row * width;
        const std::size_t destination_row = row * geometry.row_bytes;
        for (std::size_t column = 0U; column < width; ++column) {
            if (tightly_packed_raster[source_row + column] == 0U) {
                continue;
            }

            const auto bit = static_cast<compat::u8>(
                0x80U >> static_cast<unsigned int>(column & 7U)
            );
            destination[destination_row + column / 8U] |= bit;
        }
    }

    return LegacyGlyphMaskPackStatus::completed;
}

LegacyGlyphCache::LegacyGlyphCache(
    const compat::i32 glyph_width, const compat::i32 glyph_height
)
    : glyph_width_(glyph_width), glyph_height_(glyph_height) {
    CheckedGlyphGeometry geometry{};
    const std::size_t storage_bytes =
        checked_cache_storage_bytes(glyph_width, glyph_height, geometry);
    mask_row_bytes_ = geometry.row_bytes;
    mask_slot_bytes_ = geometry.slot_bytes;
    masks_.resize(storage_bytes);
}

compat::i32 LegacyGlyphCache::glyph_width() const noexcept {
    return glyph_width_;
}

compat::i32 LegacyGlyphCache::glyph_height() const noexcept {
    return glyph_height_;
}

std::size_t LegacyGlyphCache::mask_row_bytes() const noexcept {
    return mask_row_bytes_;
}

std::size_t LegacyGlyphCache::mask_slot_bytes() const noexcept {
    return mask_slot_bytes_;
}

compat::u32 LegacyGlyphCache::count() const noexcept {
    return count_;
}

compat::i32 LegacyGlyphCache::find(const compat::u16 key) const noexcept {
    compat::i32 low = 0;
    compat::i32 high = static_cast<compat::i32>(count_) - 1;

    while (low <= high) {
        const compat::i32 middle = (low + high) >> 1;
        const compat::u16 current = keys_[static_cast<std::size_t>(middle)];
        if (key < current) {
            high = middle - 1;
        } else if (key > current) {
            low = middle + 1;
        } else {
            return middle;
        }
    }

    return -1;
}

compat::i32 LegacyGlyphCache::insert_empty(const compat::u16 key) noexcept {
    if (count_ >= kLegacyGlyphCacheSlotCapacity) {
        return -1;
    }

    if (count_ == 0U) {
        keys_[0] = key;
        return 0;
    }

    std::size_t insertion = 0U;
    const std::size_t live_count = static_cast<std::size_t>(count_);
    while (insertion < live_count && key > keys_[insertion]) {
        ++insertion;
    }

    for (std::size_t cursor = live_count; cursor > insertion; --cursor) {
        keys_[cursor] = keys_[cursor - 1U];
        const std::size_t destination_offset = cursor * mask_slot_bytes_;
        const std::size_t source_offset = (cursor - 1U) * mask_slot_bytes_;
        std::copy_n(
            masks_.begin() + static_cast<std::ptrdiff_t>(source_offset),
            static_cast<std::ptrdiff_t>(mask_slot_bytes_),
            masks_.begin() + static_cast<std::ptrdiff_t>(destination_offset)
        );
    }

    keys_[insertion] = key;
    std::ranges::fill(
        mask_slot(static_cast<compat::u32>(insertion)), compat::u8{}
    );
    return static_cast<compat::i32>(insertion);
}

void LegacyGlyphCache::finish_miss_after_draw() noexcept {
    ++count_;
    if (count_ < kLegacyGlyphCacheCountThreshold) {
        return;
    }

    --count_;
    std::ranges::fill(mask_slot(count_), compat::u8{});
}

std::span<compat::u8>
LegacyGlyphCache::mask_slot(const compat::u32 slot) noexcept {
    if (slot >= kLegacyGlyphCacheSlotCapacity) {
        return {};
    }

    return std::span<compat::u8>{masks_}.subspan(
        static_cast<std::size_t>(slot) * mask_slot_bytes_, mask_slot_bytes_
    );
}

std::span<const compat::u8>
LegacyGlyphCache::mask_slot(const compat::u32 slot) const noexcept {
    if (slot >= kLegacyGlyphCacheSlotCapacity) {
        return {};
    }

    return std::span<const compat::u8>{masks_}.subspan(
        static_cast<std::size_t>(slot) * mask_slot_bytes_, mask_slot_bytes_
    );
}

std::span<const compat::u16>
LegacyGlyphCache::physical_key_slots() const noexcept {
    return keys_;
}

}  // namespace openswd3::rendering
