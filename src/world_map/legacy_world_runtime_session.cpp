#include "openswd3/world_map/legacy_world_runtime_session.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <new>
#include <stdexcept>
#include <utility>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

struct RoleAssemblyState {
    LegacyWorldRuntimeSessionStatus status{
        LegacyWorldRuntimeSessionStatus::ready
    };
    u32 selected_role_index{};
    u32 maps_role_count{};
    u32 duplicate_role_count{};
    u32 out_of_bounds_role_count{};
    u32 action_update_failure_count{};
};

[[nodiscard]] u32 shift_tile_coordinate(const u16 value) noexcept {
    return static_cast<u32>(value) << 4U;
}

[[nodiscard]] u32 sign_extend_word(const compat::i16 value) noexcept {
    return static_cast<u32>(static_cast<i32>(value));
}

void initialize_maps_role(
    LegacyWorldRoleRecord& role,
    const LegacyMapsRoleSourceRecord& source,
    const LegacyMapsRoleDefaultsRecord* const defaults
) noexcept {
    role = {};
    asset_runtime::initialize_legacy_action_record(role.action);
    role.guid = source.guid;
    role.action.action_id = source.action_id;
    role.action.base_variant = source.base_variant;
    role.action.variant_delta = source.variant_delta;
    role.action.field_88 = 0U;
    role.action.field_62 = 0U;
    role.action.field_8a = 0U;
    role.world_x = shift_tile_coordinate(source.tile_x);
    role.world_y = shift_tile_coordinate(source.tile_y);
    role.talk_script_id = source.talk_script_id;
    role.path_data_id = source.path_data_id;
    role.path_word_index = sign_extend_word(source.path_word_index);
    role.flags = source.flags;
    role.interaction_gate = 0U;
    role.talk_data_offset = 0U;
    role.talk_initial_offset = 0U;
    if (defaults != nullptr) {
        role.field_2c = defaults->field_2c;
        const u32 repeated = defaults->repeated_field_30_word;
        role.field_30 = repeated | (repeated << 16U);
    }
}

[[nodiscard]] bool guid_previously_seen(
    const std::span<const LegacyMapsRoleSourceRecord> roles,
    const std::size_t current
) noexcept {
    return std::any_of(
        roles.begin(),
        roles.begin() + static_cast<std::ptrdiff_t>(current),
        [&](const LegacyMapsRoleSourceRecord& role) {
            return role.guid == roles[current].guid;
        }
    );
}

[[nodiscard]] bool assemble_maps_roles(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const LegacyMapsMapDescriptor& map_descriptor,
    const LegacyWorldLoadRequest& request,
    LegacyWorldMapSession& map_session,
    LegacyWorldRoleActionInitializer& action_initializer,
    const LegacyWorldRolePostMaterializationContext*
        post_materialization_context,
    LegacyWorldRolePostMaterializationState& post_materialization_state,
    LegacyWorldRolePostMaterializationStatus& post_materialization_status,
    RoleAssemblyState& assembly
) {
    auto& business = map_session.business.state;
    auto& roles = business.roles;
    LegacyWorldRolePostMaterializationContext effective_post_context;
    if (post_materialization_context != nullptr) {
        effective_post_context = *post_materialization_context;
    }
    effective_post_context.spatial_index = &business.spatial_index;
    effective_post_context.surface_grid = map_session.surface_grid.surface_grid;
    effective_post_context.map_width = map_session.header.width;

    try {
        for (std::size_t source_index = 0U;
             source_index < database.role_sources.size();
             ++source_index) {
            auto& source = database.role_sources[source_index];
            if (guid_previously_seen(database.role_sources, source_index)) {
                ++assembly.duplicate_role_count;
                continue;
            }
            if (source.logical_map_id != request.logical_map_id) {
                continue;
            }

            if (static_cast<u32>(source.tile_x) > map_session.header.width ||
                static_cast<u32>(source.tile_y) > map_session.header.height) {
                source.tile_x = static_cast<u16>(
                    map_session.header.width - 1U
                );
                source.tile_y = static_cast<u16>(
                    map_session.header.height - 1U
                );
                if (!write_legacy_maps_role_source_record(payload, source)) {
                    assembly.status = LegacyWorldRuntimeSessionStatus::
                        role_source_write_failed;
                    return false;
                }
                ++assembly.out_of_bounds_role_count;
                continue;
            }
            if (roles.size() >= kLegacyWorldRoleCapacity) {
                assembly.status = LegacyWorldRuntimeSessionStatus::
                    role_capacity_exceeded;
                return false;
            }

            LegacyWorldRoleRecord role;
            initialize_maps_role(
                role,
                source,
                find_legacy_maps_role_defaults(database, source.guid)
            );
            roles.push_back(role);
            const u32 role_index = static_cast<u32>(roles.size() - 1U);
            if (!insert_legacy_role_spatially(
                    business.spatial_index,
                    roles,
                    role_index
                )) {
                roles.pop_back();
                assembly.status = LegacyWorldRuntimeSessionStatus::
                    role_spatial_insertion_failed;
                return false;
            }
            post_materialization_status = post_materialize_legacy_world_role(
                payload,
                database,
                map_descriptor,
                request,
                roles,
                role_index,
                &effective_post_context,
                post_materialization_state
            );
            if (post_materialization_status !=
                LegacyWorldRolePostMaterializationStatus::ready) {
                assembly.status = LegacyWorldRuntimeSessionStatus::
                    role_post_materialization_failed;
                return false;
            }
            if (source.guid == request.selected_guid) {
                assembly.selected_role_index = role_index;
            }
            ++assembly.maps_role_count;
        }
    } catch (const std::bad_alloc&) {
        assembly.status = LegacyWorldRuntimeSessionStatus::allocation_failed;
        return false;
    } catch (const std::length_error&) {
        assembly.status = LegacyWorldRuntimeSessionStatus::allocation_failed;
        return false;
    }

    if (assembly.selected_role_index == 0U) {
        assembly.status = LegacyWorldRuntimeSessionStatus::
            selected_role_not_materialized;
        return false;
    }

    for (std::size_t role_index = 1U; role_index < roles.size(); ++role_index) {
        if (action_initializer.initialize_action(roles[role_index].action) ==
            0U) {
            ++assembly.action_update_failure_count;
        }
    }
    return true;
}

}  // namespace

LegacyWorldActionUpdaterInitializer::LegacyWorldActionUpdaterInitializer(
    asset_runtime::LegacyActionUpdater& updater
) noexcept : updater_{updater} {}

u32 LegacyWorldActionUpdaterInitializer::initialize_action(
    asset_runtime::LegacyActionRecord& action
) {
    return updater_.update(action).return_value;
}

LegacyWorldRuntimeSessionResult load_legacy_world_runtime_session(
    const std::span<u8> maps_payload,
    const LegacyWorldRuntimeSessionRequest& request,
    LegacyWorldRoleActionInitializer& action_initializer,
    LegacyWorldMapSource& map_source,
    LegacyWorldCmCacheSource& cm_cache_source
) {
    LegacyWorldRuntimeSessionResult result;
    auto decoded = decode_legacy_maps_world_database(maps_payload);
    result.maps_database_status = decoded.status;
    if (decoded.status != LegacyMapsWorldDatabaseStatus::ready) {
        return result;
    }
    result.session.maps_database = std::move(decoded.database);
    result.session.logical_map_id = request.load.logical_map_id;

    const auto* const descriptor = find_legacy_maps_map_descriptor(
        result.session.maps_database,
        request.load.logical_map_id
    );
    if (descriptor == nullptr) {
        result.status =
            LegacyWorldRuntimeSessionStatus::map_descriptor_not_found;
        return result;
    }
    result.session.map_descriptor = *descriptor;

    if ((request.load.load_flags & 1U) != 0U) {
        if (request.preload_context == nullptr) {
            result.status = LegacyWorldRuntimeSessionStatus::
                preload_coordinate_stage_required;
            return result;
        }

        result.session.role_preload = preload_legacy_world_roles_before_load(
            maps_payload,
            result.session.maps_database,
            request.load,
            *request.preload_context
        );
        result.session.role_preload_applied = true;
        if (result.session.role_preload.status !=
            LegacyWorldRolePreloadStatus::ready) {
            result.status = LegacyWorldRuntimeSessionStatus::
                preload_role_synchronization_failed;
            return result;
        }
    }

    RoleAssemblyState assembly;
    auto render_result = load_legacy_world_render_session(
        LegacyWorldRenderSessionRequest{
            .archive_path = request.archive_path,
            .cache_directory = request.cache_directory,
            .map_id = descriptor->archive_map_id,
            .cache_limit_megabytes = request.cache_limit_megabytes,
            .pixel_conversion = request.pixel_conversion,
        },
        map_source,
        cm_cache_source,
        [&](LegacyWorldMapSession& map_session) {
            result.session.maps_load_apply = apply_legacy_maps_world_load(
                maps_payload,
                result.session.maps_database,
                request.load
            );
            if (result.session.maps_load_apply.status !=
                LegacyMapsWorldLoadApplyStatus::ready) {
                assembly.status = LegacyWorldRuntimeSessionStatus::
                    maps_load_apply_failed;
                return false;
            }
            return assemble_maps_roles(
                maps_payload,
                result.session.maps_database,
                result.session.map_descriptor,
                request.load,
                map_session,
                action_initializer,
                request.post_materialization_context,
                result.session.role_post_materialization,
                result.session.role_post_materialization_status,
                assembly
            );
        }
    );
    result.render_status = render_result.status;
    result.session.render = std::move(render_result.session);

    result.session.selected_role_index = assembly.selected_role_index;
    result.session.maps_role_count = assembly.maps_role_count;
    result.session.duplicate_role_count = assembly.duplicate_role_count;
    result.session.out_of_bounds_role_count =
        assembly.out_of_bounds_role_count;
    result.session.action_update_failure_count =
        assembly.action_update_failure_count;

    if (assembly.status != LegacyWorldRuntimeSessionStatus::ready) {
        result.status = assembly.status;
        return result;
    }
    if (result.render_status != LegacyWorldRenderSessionStatus::ready) {
        result.status = LegacyWorldRuntimeSessionStatus::render_session_failed;
        return result;
    }

    const auto& map_session = result.session.render.map_load.session;
    const auto& roles = map_session.business.state.roles;
    if (result.session.selected_role_index == 0U ||
        result.session.selected_role_index >= roles.size()) {
        result.status = LegacyWorldRuntimeSessionStatus::
            selected_role_not_materialized;
        return result;
    }
    recenter_legacy_world_camera(
        roles[result.session.selected_role_index], map_session.header.width,
        map_session.header.height, result.session.camera);

    result.status = LegacyWorldRuntimeSessionStatus::ready;
    return result;
}

LegacyWorldRuntimeSessionResult load_legacy_world_runtime_session(
    const std::span<u8> maps_payload,
    const LegacyWorldRuntimeSessionRequest& request,
    LegacyWorldRoleActionInitializer& action_initializer
) {
    LegacyLmfWorldMapSource map_source{request.archive_path};
    LegacyFileWorldCmCacheSource cm_cache_source;
    return load_legacy_world_runtime_session(
        maps_payload,
        request,
        action_initializer,
        map_source,
        cm_cache_source
    );
}

}  // namespace openswd3::world_map
