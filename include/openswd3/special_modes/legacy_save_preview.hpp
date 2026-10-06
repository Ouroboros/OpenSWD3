#pragma once

#include "openswd3/resource_io/legacy_save_container.hpp"
#include "openswd3/special_modes/legacy_standard_mode.hpp"

namespace openswd3::special_modes {

// sub_4099C0: release the five resources in order, then zero the record.
void reset_legacy_save_preview(LegacySavePreviewRecord& preview) noexcept;

// sub_409B80: three columns, four existing action records per column.
void refresh_legacy_save_preview_actions(
    LegacyInputMenuSavePreviewResetState& state
) noexcept;

enum class LegacySavePreviewPopulateStatus : compat::u8 {
    completed,
    flags_truncated,
    party_truncated,
    label_unterminated,
    allocation_failed,
    payload_unavailable,
};

// Apply sub_409600's available payload and preserve the prefix when a read
// stopped. File opening belongs to the calling loader. The first unavailable
// stage prevents later allocations, field writes and completion publication.
// This does not implicitly reset the destination record.
[[nodiscard]] LegacySavePreviewPopulateStatus populate_legacy_save_preview(
    LegacySavePreviewRecord& preview,
    const resource_io::LegacySavePreviewPayload& payload,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    resource_io::LegacySavePreviewReadStage next_read =
        resource_io::LegacySavePreviewReadStage::complete
);

}  // namespace openswd3::special_modes
