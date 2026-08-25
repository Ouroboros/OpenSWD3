#include "openswd3/battle/legacy_battle_image_rotation.hpp"

#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <new>
#include <utility>

namespace openswd3::battle {
namespace {

enum class CopySpace : compat::u8 {
    image,
    temporary,
};

class DefaultRotationAllocator final
    : public LegacyBattleImageRotationAllocator {
public:
    [[nodiscard]] LegacyBattleImageRotationAllocation
    allocate(const compat::u32 requested_bytes) noexcept override {
        std::unique_ptr<compat::u8[]> bytes{new (std::nothrow)
                                                compat::u8[requested_bytes]};
        if (bytes == nullptr) {
            return {};
        }
        return {
            .bytes = std::move(bytes),
            .byte_capacity = requested_bytes,
        };
    }

    void
    release(LegacyBattleImageRotationAllocation& allocation) noexcept override {
        allocation.bytes.reset();
        allocation.byte_capacity = 0U;
    }
};

[[nodiscard]] bool range_available(
    const std::size_t size, const compat::u32 offset, const compat::u32 count
) noexcept {
    const std::size_t widened_offset = static_cast<std::size_t>(offset);
    const std::size_t widened_count = static_cast<std::size_t>(count);
    return widened_offset <= size && widened_count <= size - widened_offset;
}

[[nodiscard]] compat::u32
wrapping_add(const compat::u32 left, const compat::u32 right) noexcept {
    return left + right;
}

[[nodiscard]] compat::u32
wrapping_subtract(const compat::u32 left, const compat::u32 right) noexcept {
    return left - right;
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] bool read_u16(
    const std::span<const compat::u8> bytes,
    const compat::u32 offset,
    compat::u16& value
) noexcept {
    if (!range_available(bytes.size(), offset, 2U)) {
        return false;
    }
    value = static_cast<compat::u16>(
        bytes[static_cast<std::size_t>(offset)] |
        static_cast<compat::u16>(
            static_cast<compat::u16>(
                bytes[static_cast<std::size_t>(offset + 1U)]
            )
            << 8U
        )
    );
    return true;
}

void write_u16(
    const std::span<compat::u8> bytes,
    const compat::u32 offset,
    const compat::u16 value
) noexcept {
    bytes[static_cast<std::size_t>(offset)] =
        static_cast<compat::u8>(value & 0xFFU);
    bytes[static_cast<std::size_t>(offset + 1U)] =
        static_cast<compat::u8>(value >> 8U);
}

[[nodiscard]] LegacyBattleImageRotationStatus
read_failure_status(const CopySpace source) noexcept {
    return source == CopySpace::image
        ? LegacyBattleImageRotationStatus::image_read_out_of_range
        : LegacyBattleImageRotationStatus::temporary_read_out_of_range;
}

[[nodiscard]] LegacyBattleImageRotationStatus
write_failure_status(const CopySpace destination) noexcept {
    return destination == CopySpace::image
        ? LegacyBattleImageRotationStatus::image_write_out_of_range
        : LegacyBattleImageRotationStatus::temporary_write_out_of_range;
}

[[nodiscard]] bool copy_forward_like_x86(
    const std::span<compat::u8> image,
    const std::span<compat::u8> temporary,
    const CopySpace source_space,
    compat::u32 source_offset,
    const CopySpace destination_space,
    compat::u32 destination_offset,
    const compat::u32 count,
    LegacyBattleImageRotationResult& result
) noexcept {
    const auto source = source_space == CopySpace::image ? image : temporary;
    const auto destination =
        destination_space == CopySpace::image ? image : temporary;

    compat::u32 dword_count = count >> 2U;
    while (dword_count != 0U) {
        if (!range_available(source.size(), source_offset, 4U)) {
            result.status = read_failure_status(source_space);
            return false;
        }
        const std::array<compat::u8, 4> value{
            source[static_cast<std::size_t>(source_offset)],
            source[static_cast<std::size_t>(source_offset + 1U)],
            source[static_cast<std::size_t>(source_offset + 2U)],
            source[static_cast<std::size_t>(source_offset + 3U)],
        };
        if (!range_available(destination.size(), destination_offset, 4U)) {
            result.status = write_failure_status(destination_space);
            return false;
        }
        for (compat::u32 index = 0U; index < 4U; ++index) {
            destination[static_cast<std::size_t>(destination_offset + index)] =
                value[static_cast<std::size_t>(index)];
        }
        if (destination_space == CopySpace::image) {
            result.image_bytes_written += 4U;
        } else {
            result.temporary_bytes_written += 4U;
        }
        source_offset = wrapping_add(source_offset, 4U);
        destination_offset = wrapping_add(destination_offset, 4U);
        --dword_count;
    }

    compat::u32 byte_count = count & 3U;
    while (byte_count != 0U) {
        if (!range_available(source.size(), source_offset, 1U)) {
            result.status = read_failure_status(source_space);
            return false;
        }
        const compat::u8 value =
            source[static_cast<std::size_t>(source_offset)];
        if (!range_available(destination.size(), destination_offset, 1U)) {
            result.status = write_failure_status(destination_space);
            return false;
        }
        destination[static_cast<std::size_t>(destination_offset)] = value;
        if (destination_space == CopySpace::image) {
            ++result.image_bytes_written;
        } else {
            ++result.temporary_bytes_written;
        }
        source_offset = wrapping_add(source_offset, 1U);
        destination_offset = wrapping_add(destination_offset, 1U);
        --byte_count;
    }
    return true;
}

void retain_stopped_temporary(
    LegacyBattleImageRotationResult& result,
    LegacyBattleImageRotationAllocation& allocation
) noexcept {
    result.stopped_temporary = std::move(allocation);
}

void release_temporary(
    LegacyBattleImageRotationResult& result,
    LegacyBattleImageRotationAllocation& allocation,
    LegacyBattleImageRotationAllocator& allocator
) noexcept {
    allocator.release(allocation);
    result.temporary_released = true;
}

}  // namespace

LegacyBattleImageRotationResult rotate_legacy_battle_literal_image(
    const std::span<compat::u8> image,
    const LegacyBattleImageRotationMode mode,
    const compat::i32 shift,
    LegacyBattleImageRotationAllocator& allocator
) noexcept {
    LegacyBattleImageRotationResult result;
    if (shift <= 0) {
        result.status = LegacyBattleImageRotationStatus::shift_not_positive;
        return result;
    }

    compat::u16 magic{};
    if (!read_u16(image, 0U, magic)) {
        result.status =
            LegacyBattleImageRotationStatus::header_read_out_of_range;
        return result;
    }
    if (magic != rendering::kLegacyImageCommandStreamMagic) {
        result.status = LegacyBattleImageRotationStatus::magic_mismatch;
        return result;
    }
    if (!read_u16(image, 2U, result.width) ||
        !read_u16(image, 4U, result.height)) {
        result.status =
            LegacyBattleImageRotationStatus::header_read_out_of_range;
        return result;
    }

    compat::u16 first_row_header{};
    if (!read_u16(image, 8U, first_row_header)) {
        result.status =
            LegacyBattleImageRotationStatus::first_row_header_read_out_of_range;
        return result;
    }
    first_row_header = static_cast<compat::u16>(first_row_header & 0x7FFFU);
    write_u16(image, 8U, first_row_header);
    result.first_row_header_written = true;
    if ((first_row_header & 0xC000U) != 0U) {
        result.status =
            LegacyBattleImageRotationStatus::first_row_flags_unsupported;
        return result;
    }

    const compat::u32 mode_value = static_cast<compat::u32>(mode);
    if (mode_value >
        static_cast<compat::u32>(LegacyBattleImageRotationMode::pixels_right)) {
        result.status = LegacyBattleImageRotationStatus::mode_out_of_range;
        return result;
    }

    const compat::u32 row_words = static_cast<compat::u32>(result.width) + 3U;
    result.row_bytes = row_words * 2U;
    const compat::u32 unsigned_shift = static_cast<compat::u32>(shift);
    if (mode == LegacyBattleImageRotationMode::rows_up ||
        mode == LegacyBattleImageRotationMode::rows_down) {
        result.requested_temporary_bytes = row_words * unsigned_shift * 2U;
    } else {
        result.requested_temporary_bytes = result.row_bytes - 6U;
    }

    LegacyBattleImageRotationAllocation temporary_allocation =
        allocator.allocate(result.requested_temporary_bytes);
    result.allocation_failed = temporary_allocation.bytes == nullptr;
    const compat::u32 temporary_capacity = temporary_allocation.bytes == nullptr
        ? 0U
        : temporary_allocation.byte_capacity;
    const std::span<compat::u8> temporary{
        temporary_allocation.bytes.get(),
        static_cast<std::size_t>(temporary_capacity),
    };

    constexpr compat::u32 kBodyOffset = 8U;
    const compat::u32 width_bytes = result.row_bytes - 6U;

    if (mode == LegacyBattleImageRotationMode::rows_up) {
        if (!copy_forward_like_x86(
                image,
                temporary,
                CopySpace::image,
                kBodyOffset,
                CopySpace::temporary,
                0U,
                result.requested_temporary_bytes,
                result
            )) {
            retain_stopped_temporary(result, temporary_allocation);
            return result;
        }

        const compat::i32 remaining_rows =
            wrapping_subtract(static_cast<compat::i32>(result.height), shift);
        compat::u32 destination_offset = kBodyOffset;
        if (remaining_rows > 0) {
            compat::u32 source_offset =
                wrapping_add(kBodyOffset, result.requested_temporary_bytes);
            compat::i32 rows = remaining_rows;
            while (rows != 0) {
                if (!copy_forward_like_x86(
                        image,
                        temporary,
                        CopySpace::image,
                        source_offset,
                        CopySpace::image,
                        destination_offset,
                        result.row_bytes,
                        result
                    )) {
                    retain_stopped_temporary(result, temporary_allocation);
                    return result;
                }
                source_offset = wrapping_add(source_offset, result.row_bytes);
                destination_offset =
                    wrapping_add(destination_offset, result.row_bytes);
                --rows;
            }
        }
        if (!copy_forward_like_x86(
                image,
                temporary,
                CopySpace::temporary,
                0U,
                CopySpace::image,
                destination_offset,
                result.requested_temporary_bytes,
                result
            )) {
            retain_stopped_temporary(result, temporary_allocation);
            return result;
        }
    } else if (mode == LegacyBattleImageRotationMode::rows_down) {
        const compat::u32 remaining_rows = std::bit_cast<compat::u32>(
            wrapping_subtract(static_cast<compat::i32>(result.height), shift)
        );
        const compat::u32 saved_rows_offset = remaining_rows * row_words * 2U;
        if (!copy_forward_like_x86(
                image,
                temporary,
                CopySpace::image,
                wrapping_add(kBodyOffset, saved_rows_offset),
                CopySpace::temporary,
                0U,
                result.requested_temporary_bytes,
                result
            )) {
            retain_stopped_temporary(result, temporary_allocation);
            return result;
        }

        compat::i32 last_row = static_cast<compat::i32>(result.height) - 1;
        if (last_row >= shift) {
            compat::u32 destination_row_offset =
                static_cast<compat::u32>(last_row) * row_words * 2U;
            compat::i32 rows = wrapping_subtract(last_row, shift) + 1;
            while (rows != 0) {
                const compat::u32 source_row_offset = wrapping_subtract(
                    destination_row_offset, result.requested_temporary_bytes
                );
                if (!copy_forward_like_x86(
                        image,
                        temporary,
                        CopySpace::image,
                        wrapping_add(kBodyOffset, source_row_offset),
                        CopySpace::image,
                        wrapping_add(kBodyOffset, destination_row_offset),
                        result.row_bytes,
                        result
                    )) {
                    retain_stopped_temporary(result, temporary_allocation);
                    return result;
                }
                destination_row_offset =
                    wrapping_subtract(destination_row_offset, result.row_bytes);
                --rows;
            }
        }
        if (!copy_forward_like_x86(
                image,
                temporary,
                CopySpace::temporary,
                0U,
                CopySpace::image,
                kBodyOffset,
                result.requested_temporary_bytes,
                result
            )) {
            retain_stopped_temporary(result, temporary_allocation);
            return result;
        }
    } else if (result.height > 0U) {
        const compat::u32 shift_bytes = unsigned_shift * 2U;
        const compat::u32 remaining_bytes =
            wrapping_subtract(width_bytes, shift_bytes);
        compat::u32 row_offset = kBodyOffset;
        compat::u16 rows = result.height;
        while (rows != 0U) {
            if (!copy_forward_like_x86(
                    image,
                    temporary,
                    CopySpace::image,
                    wrapping_add(row_offset, 4U),
                    CopySpace::temporary,
                    0U,
                    width_bytes,
                    result
                )) {
                retain_stopped_temporary(result, temporary_allocation);
                return result;
            }

            if (mode == LegacyBattleImageRotationMode::pixels_left) {
                const compat::u32 tail_offset = wrapping_subtract(
                    wrapping_add(row_offset, result.row_bytes - 2U), shift_bytes
                );
                if (!copy_forward_like_x86(
                        image,
                        temporary,
                        CopySpace::temporary,
                        0U,
                        CopySpace::image,
                        tail_offset,
                        shift_bytes,
                        result
                    ) ||
                    !copy_forward_like_x86(
                        image,
                        temporary,
                        CopySpace::temporary,
                        shift_bytes,
                        CopySpace::image,
                        wrapping_add(row_offset, 4U),
                        remaining_bytes,
                        result
                    )) {
                    retain_stopped_temporary(result, temporary_allocation);
                    return result;
                }
            } else {
                if (!copy_forward_like_x86(
                        image,
                        temporary,
                        CopySpace::temporary,
                        remaining_bytes,
                        CopySpace::image,
                        wrapping_add(row_offset, 4U),
                        shift_bytes,
                        result
                    ) ||
                    !copy_forward_like_x86(
                        image,
                        temporary,
                        CopySpace::temporary,
                        0U,
                        CopySpace::image,
                        wrapping_add(wrapping_add(row_offset, 4U), shift_bytes),
                        remaining_bytes,
                        result
                    )) {
                    retain_stopped_temporary(result, temporary_allocation);
                    return result;
                }
            }
            row_offset = wrapping_add(row_offset, result.row_bytes);
            --rows;
        }
    }

    release_temporary(result, temporary_allocation, allocator);
    return result;
}

LegacyBattleImageRotationResult rotate_legacy_battle_literal_image(
    const std::span<compat::u8> image,
    const LegacyBattleImageRotationMode mode,
    const compat::i32 shift
) noexcept {
    DefaultRotationAllocator allocator;
    return rotate_legacy_battle_literal_image(image, mode, shift, allocator);
}

}  // namespace openswd3::battle
