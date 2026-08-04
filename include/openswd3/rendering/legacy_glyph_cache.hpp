#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::rendering {

inline constexpr std::size_t kLegacyGlyphCacheSlotCapacity = 2000U;
inline constexpr compat::u32 kLegacyGlyphCacheCountThreshold = 1999U;

enum class LegacyGlyphMaskPackStatus : compat::u8 {
    completed,
    invalid_geometry,
    source_out_of_bounds,
    destination_out_of_bounds,
};

[[nodiscard]] LegacyGlyphMaskPackStatus pack_legacy_glyph_mask(
    std::span<const compat::u16> tightly_packed_raster,
    compat::i32 glyph_width,
    compat::i32 glyph_height,
    std::span<compat::u8> destination
) noexcept;

class LegacyGlyphCache final {
public:
    LegacyGlyphCache(compat::i32 glyph_width, compat::i32 glyph_height);

    LegacyGlyphCache(const LegacyGlyphCache&) = delete;
    LegacyGlyphCache& operator=(const LegacyGlyphCache&) = delete;
    LegacyGlyphCache(LegacyGlyphCache&&) = delete;
    LegacyGlyphCache& operator=(LegacyGlyphCache&&) = delete;

    [[nodiscard]] compat::i32 glyph_width() const noexcept;
    [[nodiscard]] compat::i32 glyph_height() const noexcept;
    [[nodiscard]] std::size_t mask_row_bytes() const noexcept;
    [[nodiscard]] std::size_t mask_slot_bytes() const noexcept;
    [[nodiscard]] compat::u32 count() const noexcept;

    [[nodiscard]] compat::i32 find(compat::u16 key) const noexcept;

    // Mirrors sub_4369C0: insert and clear/shift a physical slot without
    // changing the live count. The original caller draws the miss before
    // finish_miss_after_draw() increments and applies its count cap.
    [[nodiscard]] compat::i32 insert_empty(compat::u16 key) noexcept;
    void finish_miss_after_draw() noexcept;

    [[nodiscard]] std::span<compat::u8> mask_slot(
        compat::u32 slot
    ) noexcept;
    [[nodiscard]] std::span<const compat::u8> mask_slot(
        compat::u32 slot
    ) const noexcept;
    [[nodiscard]] std::span<const compat::u16> physical_key_slots(
    ) const noexcept;

private:
    compat::i32 glyph_width_{};
    compat::i32 glyph_height_{};
    std::size_t mask_row_bytes_{};
    std::size_t mask_slot_bytes_{};
    std::array<compat::u16, kLegacyGlyphCacheSlotCapacity> keys_{};
    std::vector<compat::u8> masks_{};
    compat::u32 count_{};
};

}  // namespace openswd3::rendering
