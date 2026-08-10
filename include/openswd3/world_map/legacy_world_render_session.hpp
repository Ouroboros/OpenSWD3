#pragma once

#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/world_map/legacy_cm_cache_loader.hpp"
#include "openswd3/world_map/legacy_world_background.hpp"
#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <filesystem>
#include <vector>

namespace openswd3::world_map {

enum class LegacyWorldRenderSessionStatus {
    ready,
    map_load_failed,
    cache_directory_failed,
    cm_cache_load_failed,
    indexed_palette_too_short,
    unsupported_pixel_bits,
};

struct LegacyWorldRenderSessionRequest {
    std::filesystem::path archive_path;
    std::filesystem::path cache_directory;
    compat::u32 map_id{};
    compat::u32 cache_limit_megabytes{60U};
    rendering::LegacyPixelConversionState pixel_conversion;
};

struct LegacyWorldRenderSession {
    LegacyWorldMapLoadResult map_load;
    LegacyCmCacheLoadResult cm_cache;
    std::vector<compat::u16> indexed_palette;
    LegacyWorldBackgroundPixelLayout pixel_layout{
        LegacyWorldBackgroundPixelLayout::direct_16
    };
    compat::u16 transparent_pixel{};

    [[nodiscard]] LegacyWorldBackgroundSource background_source(
        compat::u32 tile_layer_offset = 0U
    ) const noexcept;
};

struct LegacyWorldRenderSessionResult {
    LegacyWorldRenderSessionStatus status{
        LegacyWorldRenderSessionStatus::map_load_failed
    };
    LegacyWorldRenderSession session;
};

class LegacyWorldCmCacheSource {
public:
    virtual ~LegacyWorldCmCacheSource() = default;

    [[nodiscard]] virtual LegacyCmCacheLoadResult load_cm_cache(
        const LegacyCmCacheRequest& request
    ) = 0;
};

class LegacyFileWorldCmCacheSource final : public LegacyWorldCmCacheSource {
public:
    [[nodiscard]] LegacyCmCacheLoadResult load_cm_cache(
        const LegacyCmCacheRequest& request
    ) override;
};

[[nodiscard]] LegacyWorldRenderSessionResult
load_legacy_world_render_session(
    const LegacyWorldRenderSessionRequest& request,
    LegacyWorldMapSource& map_source,
    LegacyWorldCmCacheSource& cm_cache_source
);

[[nodiscard]] LegacyWorldRenderSessionResult
load_legacy_world_render_session(
    const LegacyWorldRenderSessionRequest& request
);

}  // namespace openswd3::world_map
