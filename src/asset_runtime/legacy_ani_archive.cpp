#include "openswd3/asset_runtime/legacy_ani_archive.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>

namespace openswd3::asset_runtime {
namespace {

[[nodiscard]] compat::u16 read_u16(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(bytes[offset + 1U]) << 8U
        )
    );
}

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

void write_u16(
    const std::span<compat::u8> bytes,
    const std::size_t offset,
    const compat::u16 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
}

[[nodiscard]] bool read_exact(
    resource_io::LegacyFile& file, const std::span<compat::u8> bytes
) noexcept {
    compat::u32 requested = static_cast<compat::u32>(bytes.size());
    return file.read(bytes, requested) && requested == bytes.size();
}

[[nodiscard]] bool range_fits(
    const compat::u32 offset, const compat::u32 size, const compat::u32 limit
) noexcept {
    return static_cast<std::uint64_t>(offset) + size <= limit;
}

[[nodiscard]] bool
seek_exact(resource_io::LegacyFile& file, const compat::u32 offset) noexcept {
    return offset <=
        static_cast<compat::u32>(std::numeric_limits<compat::i32>::max()) &&
        file.seek_begin_one_based(static_cast<compat::i32>(offset)) ==
        offset + 1U;
}

void parse_header(
    const std::span<const compat::u8> bytes, LegacyAniHeader& header
) noexcept {
    std::ranges::copy(bytes.first(header.magic.size()), header.magic.begin());
    header.frame_count = read_u32(bytes, 0x04U);
    header.storage_bpp = read_u16(bytes, 0x08U);
    header.canvas_width = read_u16(bytes, 0x0AU);
    header.canvas_height = read_u16(bytes, 0x0CU);
    header.display_width = read_u16(bytes, 0x0EU);
    header.display_height = read_u16(bytes, 0x10U);
    header.flags = read_u16(bytes, 0x12U);
    header.palette_count = read_u16(bytes, 0x14U);
    header.field_16 = read_u16(bytes, 0x16U);
    header.initial_declared_total_size = read_u32(bytes, 0x18U);
    header.initial_span_count = read_u32(bytes, 0x1CU);
    header.initial_record_size = read_u32(bytes, 0x20U);
}

[[nodiscard]] bool magic_is_ani(const LegacyAniHeader& header) noexcept {
    constexpr std::array<compat::u8, 4> kMagic{'A', 'N', 'I', 0U};
    return header.magic == kMagic;
}

[[nodiscard]] LegacyAniFrameNode
make_sentinel(const LegacyAniHeader& header) noexcept {
    LegacyAniFrameNode sentinel;
    sentinel.record_size = 0x18U;
    sentinel.span_count = 1U;
    sentinel.declared_total_size = 0x18U;
    sentinel.next_record_size = header.initial_record_size;
    sentinel.next_span_count = header.initial_span_count;
    sentinel.next_declared_total_size = header.initial_declared_total_size;
    return sentinel;
}

[[nodiscard]] LegacyAniFrameNode
make_next_node(const LegacyAniFrameNode& previous) noexcept {
    LegacyAniFrameNode node;
    node.record_seek_base = previous.record_seek_base + previous.record_size;
    node.record_size = previous.next_record_size;
    node.span_count = previous.next_span_count;
    node.declared_total_size = previous.next_declared_total_size;
    node.one_based_frame = previous.one_based_frame + 1U;
    return node;
}

[[nodiscard]] LegacyAniSpanResult span_failure(
    const LegacyAniSpanResult& base, const LegacyAniSpanStatus status
) noexcept {
    LegacyAniSpanResult result = base;
    result.status = status;
    return result;
}

}  // namespace

LegacyAniOpenStatus LegacyAniArchive::open(
    const std::filesystem::path& archive_path,
    const rendering::LegacyPixelConversionState pixel_conversion
) {
    close();
    if (!file_.open(
            archive_path,
            resource_io::LegacyFileCreation::open_existing,
            resource_io::LegacyFileAccess::read,
            resource_io::LegacyFileSharing::exclusive
        )) {
        return LegacyAniOpenStatus::file_open_failed;
    }

    file_size_ = file_.size();
    if (file_size_ < kLegacyAniHeaderSize || !seek_exact(file_, 0U)) {
        close();
        return LegacyAniOpenStatus::header_read_failed;
    }
    std::array<compat::u8, kLegacyAniHeaderSize> header_bytes{};
    if (!read_exact(file_, header_bytes)) {
        close();
        return LegacyAniOpenStatus::header_read_failed;
    }
    parse_header(header_bytes, header_);
    if (!magic_is_ani(header_)) {
        close();
        return LegacyAniOpenStatus::invalid_magic;
    }

    try {
        compressed_scratch_.assign(kLegacyAniScratchCapacity, 0U);
        decompressed_scratch_.assign(kLegacyAniScratchCapacity, 0U);
        frames_.reserve(header_.frame_count);
    } catch (const std::bad_alloc&) {
        close();
        return LegacyAniOpenStatus::allocation_failed;
    }

    pixel_conversion_ = pixel_conversion;
    if (header_.storage_bpp == 8U) {
        if (header_.palette_count > palette_event_frames_.size()) {
            close();
            return LegacyAniOpenStatus::palette_event_count_out_of_range;
        }
        if (header_.palette_count != 0U) {
            if (file_size_ < kLegacyAniPaletteEventTableBytes) {
                close();
                return LegacyAniOpenStatus::
                    palette_event_table_out_of_file_range;
            }
            const compat::u32 table_offset =
                file_size_ - kLegacyAniPaletteEventTableBytes;
            if (!seek_exact(file_, table_offset)) {
                close();
                return LegacyAniOpenStatus::palette_event_table_seek_failed;
            }
            std::array<compat::u8, kLegacyAniPaletteEventTableBytes> bytes{};
            if (!read_exact(file_, bytes)) {
                close();
                return LegacyAniOpenStatus::palette_event_table_read_failed;
            }
            for (std::size_t index = 0U; index < palette_event_frames_.size();
                 ++index) {
                palette_event_frames_[index] = read_u16(bytes, index * 2U);
            }
        }

        const LegacyAniOpenStatus palette_status = load_palette(0U);
        if (palette_status != LegacyAniOpenStatus::ready) {
            close();
            return palette_status;
        }
        has_palette_ = true;
        next_palette_index_ = header_.palette_count == 0U ? 0U : 1U;
    }

    open_ = true;
    return LegacyAniOpenStatus::ready;
}

void LegacyAniArchive::close() noexcept {
    static_cast<void>(file_.close());
    file_size_ = 0U;
    header_ = LegacyAniHeader{};
    pixel_conversion_ = rendering::LegacyPixelConversionState{};
    palette_.fill(0U);
    palette_event_frames_.fill(0U);
    next_palette_index_ = 0U;
    frames_.clear();
    compressed_scratch_.clear();
    decompressed_scratch_.clear();
    has_palette_ = false;
    open_ = false;
}

bool LegacyAniArchive::is_open() const noexcept {
    return open_;
}

const LegacyAniHeader& LegacyAniArchive::header() const noexcept {
    return header_;
}

std::size_t LegacyAniArchive::cached_frame_count() const noexcept {
    return frames_.size();
}

LegacyAniOpenStatus
LegacyAniArchive::load_palette(const compat::u16 palette_index) {
    const std::uint64_t trailing_bytes = header_.palette_count == 0U
        ? kLegacyAniPaletteBytes
        : static_cast<std::uint64_t>(kLegacyAniPaletteEventTableBytes) +
            static_cast<std::uint64_t>(header_.palette_count - palette_index) *
                kLegacyAniPaletteBytes;
    if (trailing_bytes > file_size_) {
        return LegacyAniOpenStatus::palette_out_of_file_range;
    }
    const compat::u32 offset =
        file_size_ - static_cast<compat::u32>(trailing_bytes);
    if (!seek_exact(file_, offset)) {
        return LegacyAniOpenStatus::palette_seek_failed;
    }

    std::array<compat::u8, kLegacyAniPaletteBytes> bytes{};
    if (!read_exact(file_, bytes)) {
        return LegacyAniOpenStatus::palette_read_failed;
    }
    for (std::size_t index = 0U; index < palette_.size(); ++index) {
        const std::size_t source = index * 3U;
        const compat::u16 rgb555 = static_cast<compat::u16>(
            (static_cast<compat::u16>(bytes[source] & 0xF8U) << 7U) |
            (static_cast<compat::u16>(bytes[source + 1U] & 0xF8U) << 2U) |
            static_cast<compat::u16>(bytes[source + 2U] >> 3U)
        );
        palette_[index] = rgb555;
    }
    rendering::legacy_convert_pixels_forward(
        pixel_conversion_,
        palette_.data(),
        static_cast<compat::i32>(palette_.size())
    );
    return LegacyAniOpenStatus::ready;
}

LegacyAniFrameLoadStatus
LegacyAniArchive::read_record(const LegacyAniFrameNode& node) noexcept {
    if (node.record_size < kLegacyAniFrameTrailerSize) {
        return LegacyAniFrameLoadStatus::record_size_below_trailer;
    }
    if (node.record_size > compressed_scratch_.size()) {
        return LegacyAniFrameLoadStatus::scratch_capacity_exceeded;
    }
    const std::uint64_t actual_offset =
        static_cast<std::uint64_t>(node.record_seek_base) +
        kLegacyAniFrameTrailerSize;
    if (actual_offset > std::numeric_limits<compat::u32>::max() ||
        !range_fits(
            static_cast<compat::u32>(actual_offset),
            node.record_size,
            file_size_
        )) {
        return LegacyAniFrameLoadStatus::record_out_of_file_range;
    }
    if (!seek_exact(file_, static_cast<compat::u32>(actual_offset))) {
        return LegacyAniFrameLoadStatus::record_seek_failed;
    }
    if (!read_exact(
            file_,
            std::span<compat::u8>{compressed_scratch_}.first(node.record_size)
        )) {
        return LegacyAniFrameLoadStatus::record_read_failed;
    }
    return LegacyAniFrameLoadStatus::ready;
}

LegacyAniFrameLoadStatus LegacyAniArchive::update_palette_for_request(
    const compat::u32 one_based_frame, bool& changed
) {
    changed = false;
    if (!has_palette_ || next_palette_index_ >= header_.palette_count ||
        palette_event_frames_[next_palette_index_] != one_based_frame) {
        return LegacyAniFrameLoadStatus::ready;
    }
    if (load_palette(next_palette_index_) != LegacyAniOpenStatus::ready) {
        return LegacyAniFrameLoadStatus::palette_update_failed;
    }
    next_palette_index_ = static_cast<compat::u16>(next_palette_index_ + 1U);
    changed = true;
    return LegacyAniFrameLoadStatus::ready;
}

LegacyAniFrameLoadResult LegacyAniArchive::finish_frame(
    const LegacyAniFrameNode& node, const LegacyAniFrameLoadMode mode
) {
    LegacyAniFrameLoadResult result;
    result.node = node;
    result.mode = mode;
    result.has_palette = has_palette_;
    if (has_palette_) {
        result.palette = palette_;
    }

    if (node.declared_total_size < kLegacyAniFrameTrailerSize) {
        result.status = LegacyAniFrameLoadStatus::declared_size_below_trailer;
        return result;
    }
    const compat::u32 expected_output =
        node.declared_total_size - kLegacyAniFrameTrailerSize;
    const compat::u32 compressed_size =
        mode == LegacyAniFrameLoadMode::cached_reload
        ? node.record_size
        : node.record_size - kLegacyAniFrameTrailerSize;

    if (compressed_size == 0U) {
        if (expected_output != 0U || node.span_count != 0U) {
            result.status =
                LegacyAniFrameLoadStatus::invalid_zero_payload_frame;
            return result;
        }
        result.status = LegacyAniFrameLoadStatus::ready;
        result.mode = LegacyAniFrameLoadMode::first_load_zero_payload;
        return result;
    }
    if (mode == LegacyAniFrameLoadMode::cached_reload &&
        node.record_size == kLegacyAniFrameTrailerSize) {
        if (expected_output != 0U || node.span_count != 0U) {
            result.status =
                LegacyAniFrameLoadStatus::invalid_zero_payload_frame;
            return result;
        }
        result.status = LegacyAniFrameLoadStatus::ready;
        result.mode = LegacyAniFrameLoadMode::cached_zero_payload_isolated;
        return result;
    }

    const resource_io::LegacyLzo1xResult decompressed =
        resource_io::decompress_legacy_lzo1x(
            std::span<const compat::u8>{compressed_scratch_}.first(
                compressed_size
            ),
            decompressed_scratch_
        );
    result.decompression_status = decompressed.status;
    result.decompressed_size = decompressed.bytes_written;
    const bool acceptable_status =
        decompressed.status == resource_io::LegacyLzo1xStatus::success ||
        (mode == LegacyAniFrameLoadMode::cached_reload &&
         decompressed.status ==
             resource_io::LegacyLzo1xStatus::input_not_consumed);
    if (!acceptable_status) {
        result.status = LegacyAniFrameLoadStatus::decompression_failed;
        return result;
    }
    if (decompressed.bytes_written != expected_output) {
        result.status = LegacyAniFrameLoadStatus::decompressed_size_mismatch;
        return result;
    }
    result.command_stream =
        std::span<const compat::u8>{decompressed_scratch_}.first(
            decompressed.bytes_written
        );
    result.status = LegacyAniFrameLoadStatus::ready;
    return result;
}

LegacyAniFrameLoadResult
LegacyAniArchive::load_frame(const compat::u32 one_based_frame) {
    LegacyAniFrameLoadResult result;
    if (!open_) {
        return result;
    }
    if (one_based_frame == 0U) {
        result.status = LegacyAniFrameLoadStatus::frame_index_zero;
        return result;
    }

    const auto cached = std::ranges::find(
        frames_, one_based_frame, &LegacyAniFrameNode::one_based_frame
    );
    if (cached != frames_.end()) {
        result.status = read_record(*cached);
        if (result.status != LegacyAniFrameLoadStatus::ready) {
            return result;
        }
        return finish_frame(*cached, LegacyAniFrameLoadMode::cached_reload);
    }

    LegacyAniFrameNode previous =
        frames_.empty() ? make_sentinel(header_) : frames_.back();
    bool any_palette_changed{};
    while (true) {
        const LegacyAniFrameNode candidate = make_next_node(previous);
        if (candidate.one_based_frame > header_.frame_count) {
            result.status = LegacyAniFrameLoadStatus::frame_past_end;
            result.mode = LegacyAniFrameLoadMode::past_end;
            result.legacy_return_value = 1U;
            return result;
        }

        result.status = read_record(candidate);
        if (result.status != LegacyAniFrameLoadStatus::ready) {
            return result;
        }

        LegacyAniFrameNode inserted = candidate;
        const std::size_t trailer =
            inserted.record_size - kLegacyAniFrameTrailerSize;
        const std::span<const compat::u8> record{compressed_scratch_};
        inserted.next_declared_total_size = read_u32(record, trailer);
        inserted.next_span_count = read_u32(record, trailer + 4U);
        inserted.next_record_size = read_u32(record, trailer + 8U);

        bool palette_changed{};
        result.status =
            update_palette_for_request(one_based_frame, palette_changed);
        if (result.status != LegacyAniFrameLoadStatus::ready) {
            return result;
        }
        any_palette_changed = any_palette_changed || palette_changed;

        try {
            frames_.push_back(inserted);
            if (frames_.size() > 1U) {
                frames_[frames_.size() - 2U].next_pointer_32 = 1U;
            }
        } catch (const std::bad_alloc&) {
            result.status = LegacyAniFrameLoadStatus::allocation_failed;
            return result;
        }
        if (inserted.one_based_frame == one_based_frame) {
            result = finish_frame(inserted, LegacyAniFrameLoadMode::first_load);
            result.palette_changed = any_palette_changed;
            return result;
        }
        previous = inserted;
    }
}

LegacyAniSpanResult apply_legacy_ani_spans(
    const std::span<const compat::u8> command_stream,
    const compat::u32 span_count,
    const std::span<const compat::u16> palette,
    const std::span<compat::u8> destination,
    const compat::u32 pitch_bytes,
    const compat::u16 display_height,
    const rendering::LegacyPixelConversionState& pixel_conversion
) noexcept {
    LegacyAniSpanResult result;
    if (display_height > kLegacyAniViewportHeight) {
        return span_failure(
            result, LegacyAniSpanStatus::invalid_display_height
        );
    }
    if (pitch_bytes == 0U) {
        return span_failure(result, LegacyAniSpanStatus::invalid_pitch);
    }
    const std::uint64_t top = (kLegacyAniViewportHeight - display_height) / 2U;
    const std::uint64_t base_offset = top * pitch_bytes;
    if (base_offset > destination.size()) {
        return span_failure(
            result, LegacyAniSpanStatus::destination_out_of_bounds
        );
    }

    std::size_t cursor{};
    const bool indexed = !palette.empty();
    if (indexed && palette.size() < kLegacyAniPaletteColorCount) {
        return span_failure(
            result, LegacyAniSpanStatus::indexed_palette_too_small
        );
    }
    for (compat::u32 ordinal = 0U; ordinal < span_count; ++ordinal) {
        if (cursor > command_stream.size() ||
            command_stream.size() - cursor < 8U) {
            return span_failure(
                result, LegacyAniSpanStatus::command_stream_exhausted
            );
        }
        const compat::u32 record_size = read_u32(command_stream, cursor);
        const compat::u32 target_offset = read_u32(command_stream, cursor + 4U);
        if (record_size < 8U) {
            return span_failure(
                result, LegacyAniSpanStatus::invalid_record_size
            );
        }
        if (record_size > command_stream.size() - cursor) {
            return span_failure(
                result, LegacyAniSpanStatus::command_stream_exhausted
            );
        }

        const std::size_t source_offset = cursor + 8U;
        const std::size_t source_bytes = record_size - 8U;
        const std::uint64_t target = base_offset + target_offset;
        const std::uint64_t output_bytes = indexed
            ? static_cast<std::uint64_t>(source_bytes) * 2U
            : source_bytes;
        if (target > destination.size() ||
            output_bytes > destination.size() - target) {
            return span_failure(
                result, LegacyAniSpanStatus::destination_out_of_bounds
            );
        }

        const std::span<const compat::u8> source =
            command_stream.subspan(source_offset, source_bytes);
        std::span<compat::u8> output = destination.subspan(
            static_cast<std::size_t>(target),
            static_cast<std::size_t>(output_bytes)
        );
        if (indexed) {
            for (std::size_t index = 0U; index < source.size(); ++index) {
                write_u16(output, index * 2U, palette[source[index]]);
            }
            result.source_pixels += static_cast<compat::u32>(source.size());
        } else {
            std::size_t index{};
            for (; index + 1U < source.size(); index += 2U) {
                compat::u16 pixel = read_u16(source, index);
                rendering::legacy_convert_pixels_forward(
                    pixel_conversion, &pixel, 1
                );
                write_u16(output, index, pixel);
            }
            if (index < source.size()) {
                output[index] = source[index];
            }
            result.source_pixels +=
                static_cast<compat::u32>(source.size() / 2U);
        }
        cursor += record_size;
        ++result.spans_written;
    }
    return result;
}

}  // namespace openswd3::asset_runtime
