#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <utility>

namespace openswd3::world_map {

LegacyLmfWorldMapSource::LegacyLmfWorldMapSource(
    std::filesystem::path archive_path
) : archive_path_{std::move(archive_path)} {}

resource_io::LegacyLmfMapLookupResult LegacyLmfWorldMapSource::lookup_map(
    const compat::u32 map_id
) {
    return resource_io::legacy_lmf_lookup_map(archive_path_, map_id);
}

resource_io::LegacyLmfMapHeader LegacyLmfWorldMapSource::read_map_header(
    const compat::u32 map_offset
) {
    return resource_io::legacy_lmf_read_map_header(archive_path_, map_offset);
}

resource_io::LegacyLmfSurfaceGrid LegacyLmfWorldMapSource::read_surface_grid(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_surface_grid(
        archive_path_,
        map_offset,
        header
    );
}

resource_io::LegacyLmfPostSurfaceRecords
LegacyLmfWorldMapSource::read_post_surface_records(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfSurfaceGrid& surface_grid
) {
    return resource_io::legacy_lmf_read_post_surface_records(
        archive_path_,
        map_offset,
        surface_grid
    );
}

resource_io::LegacyLmfReferencedRecordDirectory
LegacyLmfWorldMapSource::read_referenced_record_directory(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records
) {
    return resource_io::legacy_lmf_read_referenced_record_directory(
        archive_path_,
        map_offset,
        post_surface_records
    );
}

resource_io::LegacyLmfOffset14Directory
LegacyLmfWorldMapSource::read_offset14_directory(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_offset14_directory(
        archive_path_,
        map_offset,
        header
    );
}

resource_io::LegacyLmfIndexedObjectDirectory
LegacyLmfWorldMapSource::read_indexed_object_directory(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_indexed_object_directory(
        archive_path_,
        map_offset,
        header
    );
}

resource_io::LegacyLmfOffset1cDirectory
LegacyLmfWorldMapSource::read_offset1c_directory(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_offset1c_directory(
        archive_path_,
        map_offset,
        header
    );
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const compat::u32 map_id,
    LegacyWorldMapSource& source
) {
    return load_legacy_world_map(map_id, source, {});
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage
) {
    return load_legacy_world_map(map_id, source, pre_surface_stage, {});
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage
) {
    LegacyWorldMapLoadResult result;
    result.session.map_id = map_id;

    result.session.lookup = source.lookup_map(map_id);
    if (result.session.lookup.status !=
        resource_io::LegacyLmfMapLookupStatus::ready) {
        return result;
    }
    const compat::u32 map_offset = result.session.lookup.map_offset;

    result.session.header = source.read_map_header(map_offset);
    if (result.session.header.status !=
        resource_io::LegacyLmfMapHeaderStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::map_header_failed;
        return result;
    }

    if (pre_surface_stage && !pre_surface_stage(result.session)) {
        result.status = LegacyWorldMapLoadStatus::pre_surface_stage_failed;
        return result;
    }

    result.session.surface_grid =
        source.read_surface_grid(map_offset, result.session.header);
    if (result.session.surface_grid.status !=
        resource_io::LegacyLmfSurfaceGridStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::surface_grid_failed;
        return result;
    }

    result.session.post_surface_records = source.read_post_surface_records(
        map_offset,
        result.session.surface_grid
    );
    if (result.session.post_surface_records.status !=
        resource_io::LegacyLmfPostSurfaceRecordsStatus::ready) {
        result.status =
            LegacyWorldMapLoadStatus::post_surface_records_failed;
        return result;
    }

    result.session.referenced_records =
        source.read_referenced_record_directory(
            map_offset,
            result.session.post_surface_records
        );
    if (result.session.referenced_records.status !=
        resource_io::LegacyLmfReferencedRecordDirectoryStatus::ready) {
        result.status =
            LegacyWorldMapLoadStatus::referenced_record_directory_failed;
        return result;
    }

    result.session.offset14_directory =
        source.read_offset14_directory(map_offset, result.session.header);
    if (result.session.offset14_directory.status !=
        resource_io::LegacyLmfOffset14DirectoryStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::offset14_directory_failed;
        return result;
    }

    result.session.indexed_objects =
        source.read_indexed_object_directory(map_offset, result.session.header);
    if (result.session.indexed_objects.status !=
        resource_io::LegacyLmfIndexedObjectDirectoryStatus::ready) {
        result.status =
            LegacyWorldMapLoadStatus::indexed_object_directory_failed;
        return result;
    }

    result.session.offset1c_directory =
        source.read_offset1c_directory(map_offset, result.session.header);
    if (result.session.offset1c_directory.status !=
        resource_io::LegacyLmfOffset1cDirectoryStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::offset1c_directory_failed;
        return result;
    }

    result.session.business = build_legacy_world_map_business_state(
        result.session.header,
        result.session.post_surface_records,
        result.session.offset14_directory,
        result.session.offset1c_directory
    );
    if (result.session.business.status !=
        LegacyWorldMapBusinessStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::business_state_failed;
        return result;
    }

    if (pre_role_binding_stage &&
        !pre_role_binding_stage(result.session)) {
        result.status =
            LegacyWorldMapLoadStatus::pre_role_binding_stage_failed;
        return result;
    }

    auto& roles = result.session.business.state.roles;
    result.session.role_cell_binding = bind_legacy_world_role_cells(
        roles,
        1U,
        static_cast<compat::u32>(roles.size()),
        result.session.header.width,
        result.session.surface_grid.surface_grid
    );
    if (result.session.role_cell_binding.status !=
        LegacyWorldRoleCellBindingStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::role_cell_binding_failed;
        return result;
    }

    result.status = LegacyWorldMapLoadStatus::ready;
    return result;
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const std::filesystem::path& archive_path,
    const compat::u32 map_id
) {
    LegacyLmfWorldMapSource source{archive_path};
    return load_legacy_world_map(map_id, source);
}

}  // namespace openswd3::world_map
