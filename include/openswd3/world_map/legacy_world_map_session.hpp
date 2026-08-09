#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_lmf_archive.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <filesystem>

namespace openswd3::world_map {

enum class LegacyWorldMapLoadStatus {
    ready,
    map_lookup_failed,
    map_header_failed,
    surface_grid_failed,
    post_surface_records_failed,
    referenced_record_directory_failed,
    offset14_directory_failed,
    indexed_object_directory_failed,
    offset1c_directory_failed,
    business_state_failed,
    role_cell_binding_failed,
};

struct LegacyWorldMapSession {
    compat::u32 map_id{};
    resource_io::LegacyLmfMapLookupResult lookup;
    resource_io::LegacyLmfMapHeader header;
    resource_io::LegacyLmfSurfaceGrid surface_grid;
    resource_io::LegacyLmfPostSurfaceRecords post_surface_records;
    resource_io::LegacyLmfReferencedRecordDirectory referenced_records;
    resource_io::LegacyLmfOffset14Directory offset14_directory;
    resource_io::LegacyLmfIndexedObjectDirectory indexed_objects;
    resource_io::LegacyLmfOffset1cDirectory offset1c_directory;
    LegacyWorldMapBusinessResult business;
    LegacyWorldRoleCellBindingResult role_cell_binding;
};

struct LegacyWorldMapLoadResult {
    LegacyWorldMapLoadStatus status{LegacyWorldMapLoadStatus::map_lookup_failed};
    LegacyWorldMapSession session;
};

class LegacyWorldMapSource {
public:
    virtual ~LegacyWorldMapSource() = default;

    [[nodiscard]] virtual resource_io::LegacyLmfMapLookupResult
    lookup_map(compat::u32 map_id) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfMapHeader
    read_map_header(compat::u32 map_offset) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfSurfaceGrid
    read_surface_grid(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfPostSurfaceRecords
    read_post_surface_records(
        compat::u32 map_offset,
        const resource_io::LegacyLmfSurfaceGrid& surface_grid
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfReferencedRecordDirectory
    read_referenced_record_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfOffset14Directory
    read_offset14_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfIndexedObjectDirectory
    read_indexed_object_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfOffset1cDirectory
    read_offset1c_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) = 0;
};

class LegacyLmfWorldMapSource final : public LegacyWorldMapSource {
public:
    explicit LegacyLmfWorldMapSource(std::filesystem::path archive_path);

    [[nodiscard]] resource_io::LegacyLmfMapLookupResult
    lookup_map(compat::u32 map_id) override;

    [[nodiscard]] resource_io::LegacyLmfMapHeader
    read_map_header(compat::u32 map_offset) override;

    [[nodiscard]] resource_io::LegacyLmfSurfaceGrid read_surface_grid(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) override;

    [[nodiscard]] resource_io::LegacyLmfPostSurfaceRecords
    read_post_surface_records(
        compat::u32 map_offset,
        const resource_io::LegacyLmfSurfaceGrid& surface_grid
    ) override;

    [[nodiscard]] resource_io::LegacyLmfReferencedRecordDirectory
    read_referenced_record_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records
    ) override;

    [[nodiscard]] resource_io::LegacyLmfOffset14Directory
    read_offset14_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) override;

    [[nodiscard]] resource_io::LegacyLmfIndexedObjectDirectory
    read_indexed_object_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) override;

    [[nodiscard]] resource_io::LegacyLmfOffset1cDirectory
    read_offset1c_directory(
        compat::u32 map_offset,
        const resource_io::LegacyLmfMapHeader& header
    ) override;

private:
    std::filesystem::path archive_path_;
};

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    compat::u32 map_id,
    LegacyWorldMapSource& source
);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    const std::filesystem::path& archive_path,
    compat::u32 map_id
);

}  // namespace openswd3::world_map
