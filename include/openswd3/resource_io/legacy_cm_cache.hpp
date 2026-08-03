#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>

namespace openswd3::resource_io {

[[nodiscard]] compat::u32 legacy_cm_cache_total_size(
    const std::filesystem::path& cache_directory
);

}  // namespace openswd3::resource_io
