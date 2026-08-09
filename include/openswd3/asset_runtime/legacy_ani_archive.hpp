#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/resource_io/legacy_file.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyAniHeaderSize = 0x24U;
inline constexpr std::size_t kLegacyAniFrameTrailerSize = 0x0CU;
inline constexpr std::size_t kLegacyAniScratchCapacity = 0x9C400U;
inline constexpr std::size_t kLegacyAniPaletteColorCount = 0x100U;
inline constexpr std::size_t kLegacyAniPaletteBytes = 0x300U;
inline constexpr std::size_t kLegacyAniPaletteEventTableBytes = 0x400U;
inline constexpr compat::u32 kLegacyAniViewportHeight = 480U;

struct LegacyAniHeader {
    std::array<compat::u8, 4> magic{};
    compat::u32 frame_count{};
    compat::u16 storage_bpp{};
    compat::u16 canvas_width{};
    compat::u16 canvas_height{};
    compat::u16 display_width{};
    compat::u16 display_height{};
    compat::u16 flags{};
    compat::u16 palette_count{};
    compat::u16 field_16{};
    compat::u32 initial_declared_total_size{};
    compat::u32 initial_span_count{};
    compat::u32 initial_record_size{};
};

static_assert(sizeof(LegacyAniHeader) == kLegacyAniHeaderSize);
static_assert(offsetof(LegacyAniHeader, frame_count) == 0x04U);
static_assert(offsetof(LegacyAniHeader, storage_bpp) == 0x08U);
static_assert(offsetof(LegacyAniHeader, display_height) == 0x10U);
static_assert(offsetof(LegacyAniHeader, palette_count) == 0x14U);
static_assert(offsetof(LegacyAniHeader, initial_declared_total_size) == 0x18U);
static_assert(offsetof(LegacyAniHeader, initial_record_size) == 0x20U);

struct LegacyAniFrameNode {
    compat::u32 record_seek_base{};
    compat::u32 record_size{};
    compat::u32 span_count{};
    compat::u32 declared_total_size{};
    compat::u32 next_record_size{};
    compat::u32 next_span_count{};
    compat::u32 next_declared_total_size{};
    compat::u32 one_based_frame{};
    compat::u32 next_pointer_32{};
};

static_assert(sizeof(LegacyAniFrameNode) == 0x24U);
static_assert(offsetof(LegacyAniFrameNode, record_seek_base) == 0x00U);
static_assert(offsetof(LegacyAniFrameNode, record_size) == 0x04U);
static_assert(offsetof(LegacyAniFrameNode, span_count) == 0x08U);
static_assert(offsetof(LegacyAniFrameNode, declared_total_size) == 0x0CU);
static_assert(offsetof(LegacyAniFrameNode, next_record_size) == 0x10U);
static_assert(offsetof(LegacyAniFrameNode, one_based_frame) == 0x1CU);
static_assert(offsetof(LegacyAniFrameNode, next_pointer_32) == 0x20U);

enum class LegacyAniOpenStatus {
    ready,
    file_open_failed,
    header_read_failed,
    invalid_magic,
    palette_event_count_out_of_range,
    palette_event_table_out_of_file_range,
    palette_event_table_seek_failed,
    palette_event_table_read_failed,
    palette_out_of_file_range,
    palette_seek_failed,
    palette_read_failed,
    allocation_failed,
};

enum class LegacyAniFrameLoadStatus {
    ready,
    archive_not_open,
    frame_index_zero,
    frame_past_end,
    record_size_below_trailer,
    record_out_of_file_range,
    record_seek_failed,
    record_read_failed,
    declared_size_below_trailer,
    scratch_capacity_exceeded,
    invalid_zero_payload_frame,
    decompression_failed,
    decompressed_size_mismatch,
    palette_update_failed,
    allocation_failed,
};

enum class LegacyAniFrameLoadMode {
    first_load,
    cached_reload,
    first_load_zero_payload,
    cached_zero_payload_isolated,
    past_end,
};

struct LegacyAniFrameLoadResult {
    LegacyAniFrameLoadStatus status{LegacyAniFrameLoadStatus::archive_not_open};
    LegacyAniFrameLoadMode mode{LegacyAniFrameLoadMode::first_load};
    LegacyAniFrameNode node{};
    std::span<const compat::u8> command_stream;
    std::span<const compat::u16> palette;
    resource_io::LegacyLzo1xStatus decompression_status{
        resource_io::LegacyLzo1xStatus::success};
    compat::u32 decompressed_size{};
    compat::u32 legacy_return_value{};
    bool has_palette{};
    bool palette_changed{};
};

class LegacyAniArchive final {
public:
    [[nodiscard]] LegacyAniOpenStatus
    open(const std::filesystem::path& archive_path,
         rendering::LegacyPixelConversionState pixel_conversion = {});
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] const LegacyAniHeader& header() const noexcept;
    [[nodiscard]] std::size_t cached_frame_count() const noexcept;

    [[nodiscard]] LegacyAniFrameLoadResult
    load_frame(compat::u32 one_based_frame);

private:
    [[nodiscard]] LegacyAniOpenStatus load_palette(compat::u16 palette_index);
    [[nodiscard]] LegacyAniFrameLoadStatus
    read_record(const LegacyAniFrameNode& node) noexcept;
    [[nodiscard]] LegacyAniFrameLoadStatus
    update_palette_for_request(compat::u32 one_based_frame, bool& changed);
    [[nodiscard]] LegacyAniFrameLoadResult
    finish_frame(const LegacyAniFrameNode& node, LegacyAniFrameLoadMode mode);

    resource_io::LegacyFile file_;
    compat::u32 file_size_{};
    LegacyAniHeader header_{};
    rendering::LegacyPixelConversionState pixel_conversion_{};
    std::array<compat::u16, kLegacyAniPaletteColorCount> palette_{};
    std::array<compat::u16, kLegacyAniPaletteEventTableBytes / 2U>
        palette_event_frames_{};
    compat::u16 next_palette_index_{};
    std::vector<LegacyAniFrameNode> frames_;
    std::vector<compat::u8> compressed_scratch_;
    std::vector<compat::u8> decompressed_scratch_;
    bool has_palette_{};
    bool open_{};
};

enum class LegacyAniSpanStatus {
    completed,
    invalid_display_height,
    invalid_pitch,
    destination_out_of_bounds,
    command_stream_exhausted,
    invalid_record_size,
    indexed_palette_too_small,
};

struct LegacyAniSpanResult {
    LegacyAniSpanStatus status{LegacyAniSpanStatus::completed};
    compat::u32 spans_written{};
    compat::u32 source_pixels{};
};

[[nodiscard]] LegacyAniSpanResult apply_legacy_ani_spans(
    std::span<const compat::u8> command_stream, compat::u32 span_count,
    std::span<const compat::u16> palette, std::span<compat::u8> destination,
    compat::u32 pitch_bytes, compat::u16 display_height,
    const rendering::LegacyPixelConversionState& pixel_conversion =
        {}) noexcept;

}  // namespace openswd3::asset_runtime
