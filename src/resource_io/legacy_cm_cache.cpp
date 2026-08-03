#include "openswd3/resource_io/legacy_cm_cache.hpp"

#include "openswd3/resource_io/legacy_file.hpp"

#include <string>

namespace openswd3::resource_io {

compat::u32 legacy_cm_cache_total_size(
    const std::filesystem::path& cache_directory
) {
    constexpr compat::u32 kCacheSlotCount = 24U;

    compat::u32 total_size{};
    for (compat::u32 slot = 0U; slot < kCacheSlotCount; ++slot) {
        LegacyFile file;
        const std::filesystem::path path =
            cache_directory / (std::to_string(slot) + ".cm");
        if (file.open(
                path,
                LegacyFileCreation::open_existing,
                LegacyFileAccess::read
            )) {
            total_size += file.size();
        }

        static_cast<void>(file.close());
    }

    return total_size;
}

}  // namespace openswd3::resource_io
