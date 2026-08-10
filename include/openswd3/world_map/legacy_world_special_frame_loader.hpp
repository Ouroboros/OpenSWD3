#pragma once

#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/resource_io/legacy_lmf_archive.hpp"

#include <filesystem>
#include <span>

namespace openswd3::world_map {

enum class LegacyWorldSpecialFrameStatus {
    ready,
    variant_out_of_range,
    file_open_failed,
    record_range_invalid,
    record_seek_failed,
    record_read_failed,
    decompression_failed,
    conversion_failed,
    allocation_failed,
};

// sub_40AD10: resolves TSW resource 0xFFFF from the referenced-record
// directory of the currently loaded LMF map.
class LegacyWorldSpecialFrameLoader final
    : public asset_runtime::LegacyTswSpecialFrameLoader {
public:
    LegacyWorldSpecialFrameLoader(
        std::filesystem::path archive_path,
        compat::u32 map_offset,
        std::span<const resource_io::LegacyLmfReferencedRecord> records,
        rendering::LegacyPixelConversionState pixel_conversion
    );

    [[nodiscard]] bool load_special_frame(
        compat::u16 variant_index,
        asset_runtime::LegacyTswRuntimeFrame& frame
    ) override;

    [[nodiscard]] LegacyWorldSpecialFrameStatus last_status() const noexcept;

private:
    std::filesystem::path archive_path_;
    compat::u32 map_offset_{};
    std::span<const resource_io::LegacyLmfReferencedRecord> records_;
    rendering::LegacyPixelConversionState pixel_conversion_;
    LegacyWorldSpecialFrameStatus last_status_{
        LegacyWorldSpecialFrameStatus::variant_out_of_range
    };
};

}  // namespace openswd3::world_map
