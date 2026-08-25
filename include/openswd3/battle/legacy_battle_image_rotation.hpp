#pragma once

#include "openswd3/compat/types.hpp"

#include <memory>
#include <span>

namespace openswd3::battle {

enum class LegacyBattleImageRotationMode : compat::u32 {
    rows_up = 0U,
    rows_down = 1U,
    pixels_left = 2U,
    pixels_right = 3U,
};

struct LegacyBattleImageRotationAllocation {
    std::unique_ptr<compat::u8[]> bytes{};
    compat::u32 byte_capacity{};
};

class LegacyBattleImageRotationAllocator {
public:
    virtual ~LegacyBattleImageRotationAllocator() = default;

    [[nodiscard]] virtual LegacyBattleImageRotationAllocation
    allocate(compat::u32 requested_bytes) noexcept = 0;
    virtual void
    release(LegacyBattleImageRotationAllocation& allocation) noexcept = 0;
};

enum class LegacyBattleImageRotationStatus : compat::u8 {
    completed,
    shift_not_positive,
    header_read_out_of_range,
    magic_mismatch,
    first_row_header_read_out_of_range,
    first_row_flags_unsupported,
    mode_out_of_range,
    image_read_out_of_range,
    image_write_out_of_range,
    temporary_read_out_of_range,
    temporary_write_out_of_range,
};

struct LegacyBattleImageRotationResult {
    LegacyBattleImageRotationStatus status{
        LegacyBattleImageRotationStatus::completed
    };
    compat::u16 width{};
    compat::u16 height{};
    compat::u32 row_bytes{};
    compat::u32 requested_temporary_bytes{};
    compat::u32 image_bytes_written{};
    compat::u32 temporary_bytes_written{};
    bool first_row_header_written{};
    bool allocation_failed{};
    bool temporary_released{};
    LegacyBattleImageRotationAllocation stopped_temporary{};
};

// sub_433F70.
[[nodiscard]] LegacyBattleImageRotationResult
rotate_legacy_battle_literal_image(
    std::span<compat::u8> image,
    LegacyBattleImageRotationMode mode,
    compat::i32 shift,
    LegacyBattleImageRotationAllocator& allocator
) noexcept;

[[nodiscard]] LegacyBattleImageRotationResult
rotate_legacy_battle_literal_image(
    std::span<compat::u8> image,
    LegacyBattleImageRotationMode mode,
    compat::i32 shift
) noexcept;

}  // namespace openswd3::battle
