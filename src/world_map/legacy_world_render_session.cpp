#include "openswd3/world_map/legacy_world_render_session.hpp"

#include <cstddef>
#include <system_error>

namespace openswd3::world_map {
namespace {

constexpr std::size_t kIndexedPaletteEntries = 256U;
constexpr std::size_t kIndexedPaletteBytes =
    kIndexedPaletteEntries * sizeof(compat::u16);

[[nodiscard]] bool cm_cache_is_ready(
    const LegacyCmCacheLoadStatus status
) noexcept {
    return status == LegacyCmCacheLoadStatus::ready_hit ||
        status == LegacyCmCacheLoadStatus::ready_generated;
}

[[nodiscard]] bool ensure_cache_directory(
    const std::filesystem::path& directory
) noexcept {
    if (directory.empty()) {
        return false;
    }

    std::error_code error;
    if (!std::filesystem::exists(directory, error)) {
        if (error || !std::filesystem::create_directories(directory, error)) {
            return false;
        }
    }
    if (error) {
        return false;
    }
    return std::filesystem::is_directory(directory, error) && !error;
}

void decode_indexed_palette(
    const std::vector<compat::u8>& cache_bytes,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    std::vector<compat::u16>& palette
) {
    palette.resize(kIndexedPaletteEntries);
    for (std::size_t index = 0U;
         index < kIndexedPaletteEntries;
         ++index) {
        const std::size_t offset = index * 2U;
        palette[index] = static_cast<compat::u16>(
            static_cast<compat::u16>(cache_bytes[offset]) |
            static_cast<compat::u16>(
                static_cast<compat::u16>(cache_bytes[offset + 1U]) << 8U
            )
        );
    }
    rendering::legacy_convert_pixels_forward(
        pixel_conversion,
        palette.data(),
        static_cast<compat::i32>(palette.size())
    );
}

}  // namespace

LegacyWorldBackgroundSource LegacyWorldRenderSession::background_source(
    const compat::u32 tile_layer_offset
) const noexcept {
    return LegacyWorldBackgroundSource{
        .map_width = map_load.session.header.width,
        .map_height = map_load.session.header.height,
        .tile_layer_offset = tile_layer_offset,
        .tile_indices = map_load.session.surface_grid.raw_table_values,
        .cell_flags = map_load.session.surface_grid.surface_grid,
        .tile_bytes = cm_cache.cache_bytes,
        .pixel_layout = pixel_layout,
        .palette = indexed_palette,
        .transparent_pixel = transparent_pixel,
    };
}

LegacyCmCacheLoadResult LegacyFileWorldCmCacheSource::load_cm_cache(
    const LegacyCmCacheRequest& request
) {
    return load_legacy_cm_cache(request);
}

LegacyWorldRenderSessionResult load_legacy_world_render_session(
    const LegacyWorldRenderSessionRequest& request,
    LegacyWorldMapSource& map_source,
    LegacyWorldCmCacheSource& cm_cache_source
) {
    LegacyWorldRenderSessionResult result;
    bool cache_directory_failed = false;
    bool cm_cache_attempted = false;

    result.session.map_load = load_legacy_world_map(
        request.map_id,
        map_source,
        [&](const LegacyWorldMapSession& map_session) {
            if (!ensure_cache_directory(request.cache_directory)) {
                cache_directory_failed = true;
                return false;
            }

            cm_cache_attempted = true;
            result.session.cm_cache = cm_cache_source.load_cm_cache(
                LegacyCmCacheRequest{
                    .archive_path = request.archive_path,
                    .cache_directory = request.cache_directory,
                    .map_id = request.map_id,
                    .map_offset = map_session.lookup.map_offset,
                    .cm_relative_offset = map_session.header.offset_20,
                    .cache_limit_megabytes = request.cache_limit_megabytes,
                    .map_pixel_bits = map_session.header.field_88,
                    .pixel_conversion = request.pixel_conversion,
                }
            );
            return cm_cache_is_ready(result.session.cm_cache.status);
        }
    );

    if (result.session.map_load.status != LegacyWorldMapLoadStatus::ready) {
        if (cache_directory_failed) {
            result.status =
                LegacyWorldRenderSessionStatus::cache_directory_failed;
        } else if (cm_cache_attempted &&
                   !cm_cache_is_ready(result.session.cm_cache.status)) {
            result.status =
                LegacyWorldRenderSessionStatus::cm_cache_load_failed;
        } else {
            result.status = LegacyWorldRenderSessionStatus::map_load_failed;
        }
        return result;
    }

    const compat::u32 pixel_bits =
        result.session.map_load.session.header.field_88;
    if (pixel_bits == 16U) {
        result.session.pixel_layout =
            LegacyWorldBackgroundPixelLayout::direct_16;
        result.session.transparent_pixel = 0x026BU;
        rendering::legacy_convert_pixels_forward(
            request.pixel_conversion,
            &result.session.transparent_pixel,
            1
        );
    } else if (pixel_bits == 8U) {
        if (result.session.cm_cache.cache_bytes.size() <
            kIndexedPaletteBytes) {
            result.status =
                LegacyWorldRenderSessionStatus::indexed_palette_too_short;
            return result;
        }
        result.session.pixel_layout =
            LegacyWorldBackgroundPixelLayout::indexed_8;
        decode_indexed_palette(
            result.session.cm_cache.cache_bytes,
            request.pixel_conversion,
            result.session.indexed_palette
        );
    } else {
        result.status =
            LegacyWorldRenderSessionStatus::unsupported_pixel_bits;
        return result;
    }

    result.status = LegacyWorldRenderSessionStatus::ready;
    return result;
}

LegacyWorldRenderSessionResult load_legacy_world_render_session(
    const LegacyWorldRenderSessionRequest& request
) {
    LegacyLmfWorldMapSource map_source{request.archive_path};
    LegacyFileWorldCmCacheSource cm_cache_source;
    return load_legacy_world_render_session(
        request,
        map_source,
        cm_cache_source
    );
}

}  // namespace openswd3::world_map
