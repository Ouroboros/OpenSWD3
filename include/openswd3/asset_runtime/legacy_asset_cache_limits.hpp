#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyMinimumTswCacheBytes = 0x00400000U;
inline constexpr compat::u32 kLegacyMaximumTswCacheBytes = 0x01000000U;
inline constexpr compat::u32 kLegacyActCacheBytes = 0x00080000U;

struct LegacyAssetCacheLimits {
    compat::u32 tsw_bytes{};
    compat::u32 act_bytes{};

    bool operator==(const LegacyAssetCacheLimits&) const = default;
};

class LegacyAssetCacheLimitPorts {
public:
    virtual ~LegacyAssetCacheLimitPorts() = default;

    [[nodiscard]] virtual compat::u32
    total_physical_memory_bytes_32() = 0;
    virtual void set_tsw_cache_limit(compat::u32 bytes) = 0;
    virtual void set_act_cache_limit(compat::u32 bytes) = 0;
};

[[nodiscard]] LegacyAssetCacheLimits calculate_legacy_asset_cache_limits(
    compat::u32 total_physical_memory_bytes
) noexcept;

[[nodiscard]] bool configure_legacy_asset_cache_limits(
    LegacyAssetCacheLimitPorts& ports
);

}  // namespace openswd3::asset_runtime
