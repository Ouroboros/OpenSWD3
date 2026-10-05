#include "openswd3/world_map/legacy_world_save_restore.hpp"

namespace openswd3::world_map {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] bool available(
    const std::span<const u8> bytes,
    const std::size_t offset,
    const std::size_t size
) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] u16
read_half(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        (static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] u32
read_word(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void write_word(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

}  // namespace

LegacySaveMapOverridesResult read_legacy_save_map_overrides(
    const resource_io::LegacySaveContainer& save, const std::size_t start
) {
    LegacySaveMapOverridesResult result;
    const std::span<const u8> bytes = save.blocks[2U].bytes;
    std::size_t offset = start;
    for (;;) {
        if (!available(bytes, offset, 2U)) {
            return result;
        }
        const u16 key = read_half(bytes, offset);
        if (key == 0U) {
            result.consumed_bytes = offset + 2U;
            result.complete = true;
            return result;
        }
        if (!available(bytes, offset, 8U)) {
            return result;
        }
        result.records.push_back({
            .key = key,
            .upper_word = read_half(bytes, offset + 2U),
            .value = read_word(bytes, offset + 4U),
        });
        offset += 8U;
    }
}

LegacySaveMapOverrideApplyResult apply_legacy_save_map_overrides(
    const std::span<u8> maps_payload,
    const std::span<const LegacySaveMapOverride> records
) noexcept {
    LegacySaveMapOverrideApplyResult result;
    if (!available(maps_payload, 8U, 4U)) {
        result.status =
            LegacySaveMapOverrideApplyStatus::directory_out_of_range;
        return result;
    }
    const std::size_t root_offset = read_word(maps_payload, 8U);
    if (!available(maps_payload, root_offset, 8U)) {
        result.status =
            LegacySaveMapOverrideApplyStatus::directory_out_of_range;
        return result;
    }
    // 0x00408915–0x0040891C dereferences the root's second dword before
    // traversing relative record offsets. The dword at root+4 is not itself
    // the first record offset.
    const std::size_t table_offset = read_word(maps_payload, root_offset + 4U);
    for (const auto& saved : records) {
        ++result.records_seen;
        std::size_t table_cursor = table_offset;
        for (;;) {
            if (!available(maps_payload, table_cursor, 4U)) {
                result.status =
                    LegacySaveMapOverrideApplyStatus::directory_out_of_range;
                return result;
            }
            const std::size_t record_offset =
                read_word(maps_payload, table_cursor);
            if (!available(maps_payload, record_offset, 2U)) {
                result.status =
                    LegacySaveMapOverrideApplyStatus::record_out_of_range;
                return result;
            }
            const u16 key = read_half(maps_payload, record_offset);
            if (key == 0U) {
                break;
            }
            if (key == saved.key) {
                if (!available(maps_payload, record_offset, 8U)) {
                    result.status =
                        LegacySaveMapOverrideApplyStatus::record_out_of_range;
                    return result;
                }
                ++result.records_matched;
                if ((read_half(maps_payload, record_offset + 6U) & 0x0800U) !=
                    0U) {
                    ++result.records_skipped_flag;
                } else {
                    write_word(
                        maps_payload,
                        record_offset,
                        static_cast<u32>(saved.key) |
                            (static_cast<u32>(saved.upper_word) << 16U)
                    );
                    write_word(maps_payload, record_offset + 4U, saved.value);
                    ++result.records_written;
                }
                break;
            }
            table_cursor += 4U;
        }
    }
    return result;
}

}  // namespace openswd3::world_map
