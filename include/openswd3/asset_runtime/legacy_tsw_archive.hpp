#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <array>
#include <filesystem>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyTswPhysicalSlotCount = 3000U;
inline constexpr compat::u32 kLegacyTswPaletteSize = 512U;

struct LegacyTswIndexRecord {
    std::array<compat::u8, 20> raw_name{};
    compat::u32 block_size{};
    compat::u32 block_offset{};
    compat::u32 metadata_id{};
    compat::u32 field_20{};
    compat::u32 field_24{};
    compat::u32 field_28{};
};

struct LegacyTswFrameDescriptor {
    compat::u32 primary_relative_offset{};
    compat::u32 primary_compressed_size{};
    compat::u32 primary_decompressed_size{};
    compat::u32 auxiliary_presence{};
    compat::u32 auxiliary_trailing_span{};
    compat::u32 auxiliary_pixel_count{};
    compat::u32 auxiliary_type{};
    compat::u32 field_1c{};
    compat::u16 width{};
    compat::u16 height{};
};

struct LegacyTswFrame {
    LegacyTswIndexRecord index;
    compat::u32 block_value{};
    compat::u16 frame_count{};
    compat::u16 storage_bpp{};
    compat::u16 header_size{};
    bool has_palette{};
    std::array<compat::u8, kLegacyTswPaletteSize> palette{};
    LegacyTswFrameDescriptor descriptor;
    std::vector<compat::u8> command_stream;
};

enum class LegacyTswOpenStatus {
    ready,
    file_open_failed,
};

enum class LegacyTswFrameStatus {
    ready,
    archive_not_open,
    physical_record_out_of_range,
    index_out_of_file_range,
    index_seek_failed,
    index_read_failed,
    empty_index_record,
    block_out_of_file_range,
    block_seek_failed,
    block_header_read_failed,
    invalid_block_magic,
    palette_read_failed,
    variant_out_of_range,
    descriptor_out_of_block_range,
    descriptor_seek_failed,
    descriptor_read_failed,
    primary_stream_out_of_block_range,
    invalid_primary_stream_size,
    allocation_failed,
    primary_stream_seek_failed,
    primary_stream_read_failed,
    decompression_failed,
    decompressed_size_mismatch,
};

struct LegacyTswFrameResult {
    LegacyTswFrameStatus status{LegacyTswFrameStatus::archive_not_open};
    LegacyTswFrame frame;
};

class LegacyTswArchive final {
public:
    [[nodiscard]] LegacyTswOpenStatus
    open(const std::filesystem::path& archive_path);
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept;
    [[nodiscard]] LegacyTswFrameResult read_frame(
        compat::u32 one_based_physical_record, compat::u32 variant_index
    ) noexcept;

private:
    resource_io::LegacyFile file_;
    compat::u32 file_size_{};
    bool open_{};
};

}  // namespace openswd3::asset_runtime
