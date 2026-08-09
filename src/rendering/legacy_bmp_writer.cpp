#include "openswd3/rendering/legacy_bmp_writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace openswd3::rendering {
namespace {

void store_u16_le(
    const std::span<compat::u8> destination,
    const std::size_t offset,
    const compat::u16 value
) noexcept {
    destination[offset] = static_cast<compat::u8>(value);
    destination[offset + 1U] = static_cast<compat::u8>(value >> 8U);
}

void store_u32_le(
    const std::span<compat::u8> destination,
    const std::size_t offset,
    const compat::u32 value
) noexcept {
    destination[offset] = static_cast<compat::u8>(value);
    destination[offset + 1U] = static_cast<compat::u8>(value >> 8U);
    destination[offset + 2U] = static_cast<compat::u8>(value >> 16U);
    destination[offset + 3U] = static_cast<compat::u8>(value >> 24U);
}

[[nodiscard]] LegacyBmpWriteResult fail_after_open(
    const LegacyBmpWriteStatus status,
    const compat::u32 row_stride,
    LegacyBmpWriterPorts& ports
) {
    ports.close();
    ports.maintain_audio();
    return {status, row_stride, 0U};
}

}  // namespace

LegacyBmpWriteResult write_legacy_16bit_framebuffer_bmp(
    const std::span<const compat::u16> pixels,
    const compat::i32 width,
    const compat::i32 height,
    const std::string_view filename,
    const LegacyPixelConversionState& pixel_conversion,
    LegacyBmpWriterPorts& ports
) {
    if (width <= 0 || height <= 0 || filename.empty()) {
        return {};
    }

    const auto pixel_count_64 = static_cast<std::uint64_t>(width) *
        static_cast<std::uint64_t>(height);
    const auto unpadded_row_bytes_64 = static_cast<std::uint64_t>(width) * 3U;
    const auto row_stride_64 = (unpadded_row_bytes_64 + 3U) & ~UINT64_C(3);
    const auto file_size_64 = static_cast<std::uint64_t>(kLegacyBmpHeaderBytes) +
        row_stride_64 * static_cast<std::uint64_t>(height);
    if (pixel_count_64 >
            static_cast<std::uint64_t>(
                std::numeric_limits<compat::i32>::max()
            ) ||
        pixel_count_64 > pixels.size() ||
        row_stride_64 > std::numeric_limits<compat::u32>::max() ||
        file_size_64 > std::numeric_limits<compat::u32>::max()) {
        return {};
    }

    const auto pixel_count = static_cast<std::size_t>(pixel_count_64);
    const auto row_stride = static_cast<compat::u32>(row_stride_64);
    std::vector<compat::u16> rgb555_pixels(
        pixels.begin(),
        pixels.begin() + static_cast<std::ptrdiff_t>(pixel_count)
    );
    legacy_convert_pixels_reverse(
        pixel_conversion,
        rgb555_pixels.data(),
        static_cast<compat::i32>(pixel_count)
    );

    if (!ports.open_or_create_without_truncation(filename)) {
        return {LegacyBmpWriteStatus::open_failed, row_stride, 0U};
    }
    if (!ports.seek_absolute(0U)) {
        return fail_after_open(
            LegacyBmpWriteStatus::seek_failed,
            row_stride,
            ports
        );
    }

    std::array<compat::u8, 14U> file_header{};
    store_u16_le(file_header, 0U, 0x4D42U);
    if (!ports.write_bytes(file_header)) {
        return fail_after_open(
            LegacyBmpWriteStatus::write_failed,
            row_stride,
            ports
        );
    }

    std::array<compat::u8, 40U> information_header{};
    store_u32_le(information_header, 0U, 40U);
    store_u32_le(
        information_header,
        4U,
        static_cast<compat::u32>(width)
    );
    store_u32_le(
        information_header,
        8U,
        static_cast<compat::u32>(height)
    );
    store_u16_le(information_header, 12U, 1U);
    store_u16_le(information_header, 14U, 24U);
    if (!ports.write_bytes(information_header)) {
        return fail_after_open(
            LegacyBmpWriteStatus::write_failed,
            row_stride,
            ports
        );
    }

    std::vector<compat::u8> output_row(row_stride);
    for (compat::i32 output_row_index = 0;
         output_row_index < height;
         ++output_row_index) {
        const auto source_row = static_cast<std::size_t>(
            height - 1 - output_row_index
        );
        const auto source_offset =
            source_row * static_cast<std::size_t>(width);
        for (compat::i32 column = 0; column < width; ++column) {
            const compat::u16 pixel = rgb555_pixels[
                source_offset + static_cast<std::size_t>(column)
            ];
            const auto output_offset = static_cast<std::size_t>(column) * 3U;
            output_row[output_offset] =
                static_cast<compat::u8>(pixel << 3U);
            output_row[output_offset + 1U] =
                static_cast<compat::u8>((pixel >> 2U) & 0xF8U);
            output_row[output_offset + 2U] =
                static_cast<compat::u8>((pixel >> 7U) & 0xF8U);
        }

        if (!ports.write_bytes(output_row)) {
            return fail_after_open(
                LegacyBmpWriteStatus::write_failed,
                row_stride,
                ports
            );
        }
        if (output_row_index % 15 == 0) {
            ports.maintain_audio();
        }
    }

    const std::optional<compat::u32> logical_file_size =
        ports.current_position();
    if (!logical_file_size.has_value()) {
        return fail_after_open(
            LegacyBmpWriteStatus::position_failed,
            row_stride,
            ports
        );
    }

    std::array<compat::u8, 4U> patch{};
    store_u32_le(patch, 0U, *logical_file_size);
    if (!ports.seek_absolute(2U)) {
        return fail_after_open(
            LegacyBmpWriteStatus::seek_failed,
            row_stride,
            ports
        );
    }
    if (!ports.write_bytes(patch)) {
        return fail_after_open(
            LegacyBmpWriteStatus::write_failed,
            row_stride,
            ports
        );
    }
    store_u32_le(patch, 0U, kLegacyBmpHeaderBytes);
    if (!ports.seek_absolute(10U)) {
        return fail_after_open(
            LegacyBmpWriteStatus::seek_failed,
            row_stride,
            ports
        );
    }
    if (!ports.write_bytes(patch)) {
        return fail_after_open(
            LegacyBmpWriteStatus::write_failed,
            row_stride,
            ports
        );
    }

    ports.close();
    ports.maintain_audio();
    return {
        LegacyBmpWriteStatus::completed,
        row_stride,
        *logical_file_size,
    };
}

}  // namespace openswd3::rendering
