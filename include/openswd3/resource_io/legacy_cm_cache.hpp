#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>

namespace openswd3::resource_io {

struct LegacyCmCachePixelMasks {
    compat::u32 red{};
    compat::u32 green{};
    compat::u32 blue{};
};

[[nodiscard]] compat::u32 legacy_cm_cache_total_size(
    const std::filesystem::path& cache_directory
);

[[nodiscard]] compat::u32 legacy_cm_cache_validate_session_marker(
    const std::filesystem::path& environment_file,
    const std::filesystem::path& cache_directory
);

[[nodiscard]] compat::u32 legacy_cm_cache_validate_pixel_masks(
    const std::filesystem::path& environment_file,
    const std::filesystem::path& cache_directory,
    const LegacyCmCachePixelMasks& stored_masks,
    const LegacyCmCachePixelMasks& current_masks
);

}  // namespace openswd3::resource_io
