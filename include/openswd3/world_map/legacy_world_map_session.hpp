#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_lmf_archive.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"

#include <filesystem>
#include <functional>

namespace openswd3::world_map {

enum class LegacyWorldMapLoadStatus {
    ready,
    map_lookup_failed,
    map_header_failed,
    pre_surface_stage_failed,
    surface_grid_failed,
    post_surface_records_failed,
    referenced_record_directory_failed,
    offset14_directory_failed,
    indexed_object_directory_failed,
    indexed_object_stage_failed,
    offset1c_directory_failed,
    business_state_failed,
    pre_role_binding_stage_failed,
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
    bool role_cell_binding_completed{};
};

struct LegacyWorldMapLoadResult {
    LegacyWorldMapLoadStatus status{
        LegacyWorldMapLoadStatus::map_lookup_failed
    };
    LegacyWorldMapSession session;
};

// sub_425BE0 reports 15/60/65/70/75/80/85 after the corresponding
// successful stages.  Supplying the session makes the ordering independently
// testable without exposing resource-layer internals to the progress renderer.
using LegacyWorldMapLoadProgressStage = std::function<
    void(compat::i32 progress, const LegacyWorldMapSession& session)>;
using LegacyWorldMapAudioMaintenanceStage = std::function<void()>;

// 0x0042660E passes each decompressed +0x18 object to sub_401B70 before
// progress 80 and before the +0x1C directory.  The hook keeps that consumer
// in the rendering owner while preserving its exact orchestration slot.
using LegacyWorldMapIndexedObjectStage = std::function<bool(
    resource_io::LegacyLmfIndexedObjectDirectory& directory,
    std::size_t physical_index
)>;

// sub_425BE0 does not read the LMF body in one uninterrupted pass.  After the
// map header has been copied into runtime state, and before the surface-grid
// stream is read, 0x00426044 calls sub_426840 to acquire the CM tile cache.
// The hook preserves that externally observable slot without coupling the LMF
// parser to cache storage policy.  Returning false stops before the surface
// read.  Other operations in that interval, including sub_411620's workspace
// allocation, remain owned by their corresponding runtime components.
using LegacyWorldMapPreSurfaceStage =
    std::function<bool(const LegacyWorldMapSession& session)>;

// sub_40C130 appends MAPS.DAT roles and runs sub_40F280 after sub_425BE0 has
// assembled the LMF business state but before the final cell projection.
using LegacyWorldMapPreRoleBindingStage =
    std::function<bool(LegacyWorldMapSession& session)>;

class LegacyWorldMapSource {
public:
    virtual ~LegacyWorldMapSource() = default;

    // Returns true when the source accepts the synchronous observer.  The
    // loader always clears an accepted observer before returning.
    [[nodiscard]] virtual bool
    set_read_observer(const resource_io::LegacyLmfReadObserver*) noexcept {
        return false;
    }

    [[nodiscard]] virtual resource_io::LegacyLmfMapLookupResult
    lookup_map(compat::u32 map_id) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfMapHeader
    read_map_header(compat::u32 map_offset) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfSurfaceGrid read_surface_grid(
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
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
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfIndexedObjectDirectory
    read_indexed_object_directory(
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
    ) = 0;

    [[nodiscard]] virtual resource_io::LegacyLmfOffset1cDirectory
    read_offset1c_directory(
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
    ) = 0;
};

class LegacyLmfWorldMapSource final : public LegacyWorldMapSource {
public:
    explicit LegacyLmfWorldMapSource(std::filesystem::path archive_path);

    [[nodiscard]] bool set_read_observer(
        const resource_io::LegacyLmfReadObserver* observer
    ) noexcept override;

    [[nodiscard]] resource_io::LegacyLmfMapLookupResult
    lookup_map(compat::u32 map_id) override;

    [[nodiscard]] resource_io::LegacyLmfMapHeader
    read_map_header(compat::u32 map_offset) override;

    [[nodiscard]] resource_io::LegacyLmfSurfaceGrid read_surface_grid(
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
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
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
    ) override;

    [[nodiscard]] resource_io::LegacyLmfIndexedObjectDirectory
    read_indexed_object_directory(
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
    ) override;

    [[nodiscard]] resource_io::LegacyLmfOffset1cDirectory
    read_offset1c_directory(
        compat::u32 map_offset, const resource_io::LegacyLmfMapHeader& header
    ) override;

private:
    std::filesystem::path archive_path_;
    const resource_io::LegacyLmfReadObserver* read_observer_{};
};

[[nodiscard]] LegacyWorldMapLoadResult
load_legacy_world_map(compat::u32 map_id, LegacyWorldMapSource& source);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage
);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage
);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage,
    const LegacyWorldMapLoadProgressStage& progress_stage
);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage,
    const LegacyWorldMapLoadProgressStage& progress_stage,
    const LegacyWorldMapIndexedObjectStage& indexed_object_stage
);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    compat::u32 map_id,
    LegacyWorldMapSource& source,
    const LegacyWorldMapPreSurfaceStage& pre_surface_stage,
    const LegacyWorldMapPreRoleBindingStage& pre_role_binding_stage,
    const LegacyWorldMapLoadProgressStage& progress_stage,
    const LegacyWorldMapIndexedObjectStage& indexed_object_stage,
    const LegacyWorldMapAudioMaintenanceStage& audio_maintenance_stage
);

[[nodiscard]] LegacyWorldMapLoadResult load_legacy_world_map(
    const std::filesystem::path& archive_path, compat::u32 map_id
);

}  // namespace openswd3::world_map
