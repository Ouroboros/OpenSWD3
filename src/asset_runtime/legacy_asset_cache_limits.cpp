#include "openswd3/asset_runtime/legacy_asset_cache_limits.hpp"

#include <algorithm>

namespace openswd3::asset_runtime {

LegacyAssetCacheLimits calculate_legacy_asset_cache_limits(
    const compat::u32 total_physical_memory_bytes
) noexcept {
    const compat::u32 one_sixth = total_physical_memory_bytes / 6U;
    return {
        .tsw_bytes = std::clamp(
            one_sixth,
            kLegacyMinimumTswCacheBytes,
            kLegacyMaximumTswCacheBytes
        ),
        .act_bytes = kLegacyActCacheBytes,
    };
}

bool configure_legacy_asset_cache_limits(
    LegacyAssetCacheLimitPorts& ports
) {
    const LegacyAssetCacheLimits limits =
        calculate_legacy_asset_cache_limits(
            ports.total_physical_memory_bytes_32()
        );
    ports.set_tsw_cache_limit(limits.tsw_bytes);
    ports.set_act_cache_limit(limits.act_bytes);
    return true;
}

}  // namespace openswd3::asset_runtime
