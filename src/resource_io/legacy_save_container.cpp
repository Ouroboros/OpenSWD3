#include "openswd3/resource_io/legacy_save_container.hpp"

#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <limits>
#include <new>

namespace openswd3::resource_io {
namespace {

[[nodiscard]] compat::u32 read_word(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool read_raw(
    const std::span<const compat::u8> bytes,
    std::size_t& offset,
    const std::span<compat::u8> destination
) noexcept {
    if (offset > bytes.size() || destination.size() > bytes.size() - offset) {
        return false;
    }
    std::copy_n(bytes.data() + offset, destination.size(), destination.data());
    offset += destination.size();
    return true;
}

[[nodiscard]] LegacySaveContainerStatus read_block(
    const std::span<const compat::u8> bytes,
    std::size_t& offset,
    LegacySaveDecodedBlock& block,
    const bool embedded_fame,
    const std::size_t destination_multiplier,
    const bool mapped_preview = false
) {
    const std::size_t prefix = embedded_fame ? 2U : 0U;
    if (offset > bytes.size() || prefix + 8U > bytes.size() - offset) {
        return LegacySaveContainerStatus::truncated;
    }
    const compat::u32 packed_size =
        read_word(bytes, offset + prefix + (embedded_fame ? 4U : 0U));
    const compat::u32 declared_size =
        read_word(bytes, offset + prefix + (embedded_fame ? 0U : 4U));
    const std::size_t header_size = prefix + 8U;
    if (!mapped_preview && packed_size > bytes.size() - offset - header_size) {
        return LegacySaveContainerStatus::truncated;
    }
    if (!mapped_preview && destination_multiplier != 0U &&
        declared_size >
            std::numeric_limits<std::size_t>::max() / destination_multiplier) {
        return LegacySaveContainerStatus::invalid_length;
    }

    block.declared_size = declared_size;
    block.bytes.resize(
        mapped_preview
            ? static_cast<std::size_t>(declared_size * compat::u32{2U})
            : destination_multiplier == 0U
            ? 0xA8U
            : static_cast<std::size_t>(declared_size) * destination_multiplier
    );
    // The native decoder only compares the packed end after reaching EOF.
    // Preview callers ignore that return code and keep the decoded prefix.
    const auto packed = mapped_preview
        ? bytes.subspan(offset + header_size)
        : bytes.subspan(offset + header_size, packed_size);
    const LegacyLzo1xResult decoded =
        decompress_legacy_lzo1x(packed, block.bytes);
    offset += header_size + packed_size;
    if ((decoded.status != LegacyLzo1xStatus::success &&
         !(mapped_preview &&
           decoded.status == LegacyLzo1xStatus::input_not_consumed)) ||
        (!mapped_preview && decoded.bytes_written != declared_size)) {
        return LegacySaveContainerStatus::decompression_failed;
    }
    block.bytes.resize(decoded.bytes_written);
    return LegacySaveContainerStatus::ready;
}

}  // namespace

LegacySaveContainerResult
read_legacy_save_container(const std::span<const compat::u8> bytes) {
    LegacySaveContainerResult result;
    LegacySaveContainer& save = result.container;
    std::size_t offset{};
    if (!read_raw(bytes, offset, save.timestamp) ||
        !read_raw(bytes, offset, save.preview) ||
        !read_raw(bytes, offset, save.label)) {
        return result;
    }

    const auto decode = [&](const std::size_t index,
                            const bool fame = false,
                            const std::size_t capacity = 1U) {
        return read_block(bytes, offset, save.blocks[index], fame, capacity);
    };
    result.status = decode(0U);
    if (result.status != LegacySaveContainerStatus::ready) {
        return result;
    }
    save.block_present[0U] = true;
    result.status = decode(1U);
    if (result.status != LegacySaveContainerStatus::ready ||
        !read_raw(bytes, offset, save.raw_after_primary)) {
        if (result.status == LegacySaveContainerStatus::ready) {
            result.status = LegacySaveContainerStatus::truncated;
        }
        return result;
    }
    save.block_present[1U] = true;
    // 0x00408507 compares the end of the seven raw words with the file size.
    if (offset == bytes.size()) {
        save.consumed_bytes = offset;
        return result;
    }

    result.status = decode(2U, false, 2U);
    if (result.status != LegacySaveContainerStatus::ready) {
        return result;
    }
    save.block_present[2U] = true;
    if (offset == bytes.size()) {
        save.consumed_bytes = offset;
        return result;
    }

    // Each subsequent EOF gate skips the remaining version extensions.
    if (!read_raw(bytes, offset, save.extension_a)) {
        result.status = LegacySaveContainerStatus::truncated;
        return result;
    }
    save.extension_a_present = true;
    if (offset == bytes.size()) {
        save.consumed_bytes = offset;
        return result;
    }

    if (!read_raw(bytes, offset, save.extension_b)) {
        result.status = LegacySaveContainerStatus::truncated;
        return result;
    }
    save.extension_b_present = true;
    if (offset == bytes.size()) {
        save.consumed_bytes = offset;
        return result;
    }

    if (offset > bytes.size() || 4U > bytes.size() - offset) {
        result.status = LegacySaveContainerStatus::truncated;
        return result;
    }
    const compat::u32 fame_outer_size = read_word(bytes, offset);
    offset += 4U;
    const std::size_t fame_start = offset;
    result.status = decode(3U, true);
    if (result.status != LegacySaveContainerStatus::ready) {
        return result;
    }
    if (offset - fame_start != fame_outer_size) {
        result.status = LegacySaveContainerStatus::invalid_length;
        return result;
    }
    save.block_present[3U] = true;
    if (offset == bytes.size()) {
        save.consumed_bytes = offset;
        return result;
    }

    if (!read_raw(bytes, offset, save.extension_c)) {
        result.status = LegacySaveContainerStatus::truncated;
        return result;
    }
    save.extension_c_present = true;
    if (offset == bytes.size()) {
        save.consumed_bytes = offset;
        return result;
    }

    result.status = decode(4U, false, 0U);
    if (result.status == LegacySaveContainerStatus::ready) {
        save.block_present[4U] = true;
        save.consumed_bytes = offset;
    }
    return result;
}

LegacySavePreviewPayloadResult
read_legacy_save_preview_payload(const std::span<const compat::u8> bytes) {
    LegacySavePreviewPayloadResult result;
    auto& preview = result.payload;
    std::size_t offset{};
    try {
        if (!read_raw(bytes, offset, preview.timestamp)) {
            return result;
        }

        result.next_read = LegacySavePreviewReadStage::pixels;
        preview.preview_bytes_read =
            std::min(preview.preview.size(), bytes.size() - offset) &
            ~std::size_t{3U};
        std::copy_n(
            bytes.data() + offset,
            preview.preview_bytes_read,
            preview.preview.data()
        );
        if (preview.preview_bytes_read != preview.preview.size()) {
            return result;
        }

        offset += preview.preview.size();
        result.next_read = LegacySavePreviewReadStage::label;
        // lstrlenA scans the mapped bytes, not a bounded 32-byte string.
        const auto label = bytes.subspan(offset);
        const auto terminator = std::find(label.begin(), label.end(), 0U);
        if (terminator == label.end()) {
            return result;
        }

        preview.nul_terminated_label.assign(label.begin(), terminator + 1);
        offset += 0x20U;
        result.next_read = LegacySavePreviewReadStage::flags;
        result.status =
            read_block(bytes, offset, preview.flags, false, 2U, true);
        if (result.status != LegacySaveContainerStatus::ready) {
            return result;
        }

        result.next_read = LegacySavePreviewReadStage::primary;
        // 004097DA..004097E6 advances by block 1's packed length. Its
        // declared output length and compressed contents are not consumed.
        result.status = LegacySaveContainerStatus::truncated;
        if (offset > bytes.size() || bytes.size() - offset < 8U) {
            return result;
        }

        const auto skipped_size = read_word(bytes, offset);
        if (skipped_size > bytes.size() - offset - 8U) {
            return result;
        }

        offset += 8U + static_cast<std::size_t>(skipped_size);
        if (offset > bytes.size() || bytes.size() - offset < 8U) {
            return result;
        }

        // Only the word at fixed-prefix +4 is read before advancing 0x1C.
        // Missing later bytes must not erase that already-available map value.
        std::copy_n(
            bytes.data() + offset,
            std::min(preview.raw_after_primary.size(), bytes.size() - offset),
            preview.raw_after_primary.data()
        );
        offset += preview.raw_after_primary.size();
        result.next_read = LegacySavePreviewReadStage::party;
        result.status =
            read_block(bytes, offset, preview.party, false, 2U, true);
        if (result.status != LegacySaveContainerStatus::ready) {
            return result;
        }

        result.status = LegacySaveContainerStatus::truncated;
        result.next_read = LegacySavePreviewReadStage::role_names;
        preview.role_name_bytes_read =
            std::min(preview.role_names.size(), bytes.size() - offset) &
            ~std::size_t{3U};
        std::copy_n(
            bytes.data() + offset,
            preview.role_name_bytes_read,
            preview.role_names.data()
        );
        if (preview.role_name_bytes_read != preview.role_names.size()) {
            return result;
        }

        offset += preview.role_names.size();
        result.next_read = LegacySavePreviewReadStage::elapsed_seconds;
        if (offset > bytes.size() || bytes.size() - offset < 4U) {
            return result;
        }

        preview.elapsed_seconds = read_word(bytes, offset);
        offset += 4U;
        preview.consumed_bytes = offset;
        result.next_read = LegacySavePreviewReadStage::complete;
        result.status = LegacySaveContainerStatus::ready;
        return result;
    } catch (const std::bad_alloc&) {
        result.status = LegacySaveContainerStatus::allocation_failed;
        return result;
    }
}

LegacySaveFameGroupsResult
read_legacy_save_fame_groups(const LegacySaveContainer& save) {
    LegacySaveFameGroupsResult result;
    const auto& source = save.blocks[3U].bytes;
    std::size_t offset{};
    for (auto& group : result.groups) {
        if (offset > source.size() || 10U > source.size() - offset) {
            return result;
        }
        group.declared_span = read_word(source, offset);
        group.count = static_cast<compat::u16>(
            static_cast<compat::u16>(source[offset + 4U]) |
            (static_cast<compat::u16>(source[offset + 5U]) << 8U)
        );
        std::copy_n(
            source.begin() + offset + 6U,
            group.header_tail.size(),
            group.header_tail.begin()
        );
        offset += 10U;
        if (group.count > (source.size() - offset) / 14U) {
            return result;
        }
        group.records.reserve(group.count);
        for (std::size_t index = 0U; index < group.count; ++index) {
            std::array<compat::u8, 14U> record{};
            std::copy_n(source.begin() + offset, record.size(), record.begin());
            group.records.push_back(record);
            offset += record.size();
        }
    }
    result.consumed_bytes = offset;
    result.complete = true;
    return result;
}

}  // namespace openswd3::resource_io
