#include "openswd3/world_map/legacy_world_map_session.hpp"

#include <utility>

namespace openswd3::world_map {

LegacyLmfWorldMapSource::LegacyLmfWorldMapSource(
    std::filesystem::path archive_path
)
    : archive_path_{std::move(archive_path)} {}

bool LegacyLmfWorldMapSource::set_read_observer(
    const resource_io::LegacyLmfReadObserver* const observer
) noexcept {
    read_observer_ = observer;
    return true;
}

resource_io::LegacyLmfMapLookupResult
LegacyLmfWorldMapSource::lookup_map(const compat::u32 map_id) {
    return resource_io::legacy_lmf_lookup_map(
        archive_path_, map_id, read_observer_
    );
}

resource_io::LegacyLmfMapHeader
LegacyLmfWorldMapSource::read_map_header(const compat::u32 map_offset) {
    return resource_io::legacy_lmf_read_map_header(
        archive_path_, map_offset, read_observer_
    );
}

resource_io::LegacyLmfSurfaceGrid LegacyLmfWorldMapSource::read_surface_grid(
    const compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_surface_grid(
        archive_path_, map_offset, header, read_observer_
    );
}

resource_io::LegacyLmfPostSurfaceRecords
LegacyLmfWorldMapSource::read_post_surface_records(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfSurfaceGrid& surface_grid
) {
    return resource_io::legacy_lmf_read_post_surface_records(
        archive_path_, map_offset, surface_grid, read_observer_
    );
}

resource_io::LegacyLmfReferencedRecordDirectory
LegacyLmfWorldMapSource::read_referenced_record_directory(
    const compat::u32 map_offset,
    const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records
) {
    return resource_io::legacy_lmf_read_referenced_record_directory(
        archive_path_, map_offset, post_surface_records, read_observer_
    );
}

resource_io::LegacyLmfOffset14Directory
LegacyLmfWorldMapSource::read_offset14_directory(
    const compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_offset14_directory(
        archive_path_, map_offset, header, read_observer_
    );
}

resource_io::LegacyLmfIndexedObjectDirectory
LegacyLmfWorldMapSource::read_indexed_object_directory(
    const compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_indexed_object_directory(
        archive_path_, map_offset, header, read_observer_
    );
}

resource_io::LegacyLmfOffset1cDirectory
LegacyLmfWorldMapSource::read_offset1c_directory(
    const compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
) {
    return resource_io::legacy_lmf_read_offset1c_directory(
        archive_path_, map_offset, header, read_observer_
    );
}

LegacyWorldMapLoadResult
load_legacy_world_map(const compat::u32 map_id, LegacyWorldMapSource& source) {
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
    return load_legacy_world_map(
        map_id, source, pre_surface_stage, pre_role_binding_stage, {}
    );
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage,
    const LegacyWorldMapLoadProgressStage& progress_stage
) {
    return load_legacy_world_map(
        map_id,
        source,
        pre_surface_stage,
        pre_role_binding_stage,
        progress_stage,
        {}
    );
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage,
    const LegacyWorldMapLoadProgressStage& progress_stage,
    const LegacyWorldMapIndexedObjectStage& indexed_object_stage
) {
    return load_legacy_world_map(
        map_id,
        source,
        pre_surface_stage,
        pre_role_binding_stage,
        progress_stage,
        indexed_object_stage,
        {}
    );
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage,
    const LegacyWorldMapLoadProgressStage& progress_stage,
    const LegacyWorldMapIndexedObjectStage& indexed_object_stage,
    const LegacyWorldMapAudioMaintenanceStage& audio_maintenance_stage
) {
    LegacyWorldMapLoadResult result;
    result.session.map_id = map_id;
    const auto report_progress = [&](const compat::i32 progress) {
        if (progress_stage) {
            progress_stage(progress, result.session);
        }
    };
    const auto maintain_audio = [&]() {
        if (audio_maintenance_stage) {
            audio_maintenance_stage();
        }
    };

    bool indexed_object_consumer_failed = false;
    resource_io::LegacyLmfReadObserver read_observer{
        .maintain_audio = maintain_audio,
        .map_header_signature_ready = [&]() { report_progress(15); },
        .indexed_object_ready =
            [&](resource_io::LegacyLmfIndexedObjectDirectory& directory,
                const std::size_t physical_index) {
                if (!indexed_object_stage) {
                    return true;
                }
                const bool ready =
                    indexed_object_stage(directory, physical_index);
                indexed_object_consumer_failed = !ready;
                return ready;
            },
    };
    const bool observer_installed = source.set_read_observer(&read_observer);
    struct ReadObserverReset {
        LegacyWorldMapSource& source;
        bool installed{};

        ~ReadObserverReset() {
            if (installed) {
                static_cast<void>(source.set_read_observer(nullptr));
            }
        }
    } observer_reset{source, observer_installed};

    maintain_audio();
    maintain_audio();
    result.session.lookup = source.lookup_map(map_id);
    if (result.session.lookup.status !=
        resource_io::LegacyLmfMapLookupStatus::ready) {
        return result;
    }
    maintain_audio();
    const compat::u32 map_offset = result.session.lookup.map_offset;

    result.session.header = source.read_map_header(map_offset);
    if (result.session.header.status !=
        resource_io::LegacyLmfMapHeaderStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::map_header_failed;
        return result;
    }
    if (!observer_installed) {
        report_progress(15);
    }

    result.session.business =
        begin_legacy_world_map_business_state(result.session.header);
    if (result.session.business.status != LegacyWorldMapBusinessStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::business_state_failed;
        return result;
    }
    maintain_audio();

    if (pre_surface_stage && !pre_surface_stage(result.session)) {
        result.status = LegacyWorldMapLoadStatus::pre_surface_stage_failed;
        return result;
    }
    report_progress(60);
    maintain_audio();

    result.session.surface_grid =
        source.read_surface_grid(map_offset, result.session.header);
    if (result.session.surface_grid.status !=
        resource_io::LegacyLmfSurfaceGridStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::surface_grid_failed;
        return result;
    }

    result.session.post_surface_records = source.read_post_surface_records(
        map_offset, result.session.surface_grid
    );
    if (result.session.post_surface_records.status !=
        resource_io::LegacyLmfPostSurfaceRecordsStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::post_surface_records_failed;
        return result;
    }
    if (append_legacy_world_map_events(
            result.session.business, result.session.post_surface_records
        ) != LegacyWorldMapBusinessStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::business_state_failed;
        return result;
    }
    report_progress(65);
    maintain_audio();

    result.session.referenced_records = source.read_referenced_record_directory(
        map_offset, result.session.post_surface_records
    );
    if (result.session.referenced_records.status !=
        resource_io::LegacyLmfReferencedRecordDirectoryStatus::ready) {
        result.status =
            LegacyWorldMapLoadStatus::referenced_record_directory_failed;
        return result;
    }
    report_progress(70);
    maintain_audio();

    result.session.offset14_directory =
        source.read_offset14_directory(map_offset, result.session.header);
    if (result.session.offset14_directory.status !=
        resource_io::LegacyLmfOffset14DirectoryStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::offset14_directory_failed;
        return result;
    }
    if (append_legacy_world_map_offset14_roles(
            result.session.business,
            result.session.header,
            result.session.offset14_directory
        ) != LegacyWorldMapBusinessStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::business_state_failed;
        return result;
    }
    report_progress(75);
    maintain_audio();

    result.session.indexed_objects =
        source.read_indexed_object_directory(map_offset, result.session.header);
    if (result.session.indexed_objects.status !=
        resource_io::LegacyLmfIndexedObjectDirectoryStatus::ready) {
        result.status = indexed_object_consumer_failed
            ? LegacyWorldMapLoadStatus::indexed_object_stage_failed
            : LegacyWorldMapLoadStatus::indexed_object_directory_failed;
        return result;
    }
    if (!observer_installed) {
        for (std::size_t object = 0U;
             object < result.session.indexed_objects.objects.size();
             ++object) {
            const bool ready = !indexed_object_stage ||
                indexed_object_stage(result.session.indexed_objects, object);
            maintain_audio();
            if (!ready) {
                result.status =
                    LegacyWorldMapLoadStatus::indexed_object_stage_failed;
                return result;
            }
        }
    }
    report_progress(80);

    result.session.offset1c_directory =
        source.read_offset1c_directory(map_offset, result.session.header);
    if (result.session.offset1c_directory.status !=
        resource_io::LegacyLmfOffset1cDirectoryStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::offset1c_directory_failed;
        return result;
    }
    if (append_legacy_world_map_offset1c_roles(
            result.session.business,
            result.session.header,
            result.session.offset1c_directory
        ) != LegacyWorldMapBusinessStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::business_state_failed;
        return result;
    }
    report_progress(85);
    maintain_audio();

    if (pre_role_binding_stage && !pre_role_binding_stage(result.session)) {
        result.status = LegacyWorldMapLoadStatus::pre_role_binding_stage_failed;
        return result;
    }

    auto& roles = result.session.business.state.roles;
    if (!result.session.role_cell_binding_completed) {
        result.session.role_cell_binding = bind_legacy_world_role_cells(
            roles,
            1U,
            static_cast<compat::u32>(roles.size()),
            result.session.header.width,
            result.session.surface_grid.surface_grid
        );
        result.session.role_cell_binding_completed = true;
    }
    if (result.session.role_cell_binding.status !=
        LegacyWorldRoleCellBindingStatus::ready) {
        result.status = LegacyWorldMapLoadStatus::role_cell_binding_failed;
        return result;
    }

    result.status = LegacyWorldMapLoadStatus::ready;
    return result;
}

LegacyWorldMapLoadResult load_legacy_world_map(
    const std::filesystem::path& archive_path, const compat::u32 map_id
) {
    LegacyLmfWorldMapSource source{archive_path};
    return load_legacy_world_map(map_id, source);
}

}  // namespace openswd3::world_map
