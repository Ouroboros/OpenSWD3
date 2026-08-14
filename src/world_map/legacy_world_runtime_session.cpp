#include "openswd3/world_map/legacy_world_runtime_session.hpp"

#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include <algorithm>
#include <array>
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

constexpr std::array<i32, 16U> kDescriptorColorOffsets{
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    8,
    -7,
    -6,
    -5,
    -4,
    -3,
    -2,
    -1,
};

constexpr std::array<u8, 7U> kUnknownMapName{
    0xA4U,
    0xA3U,
    0xAAU,
    0xBEU,
    0xB9U,
    0x44U,
    0x00U,
};

struct RoleAssemblyState {
    LegacyWorldRuntimeSessionStatus status{
        LegacyWorldRuntimeSessionStatus::ready
    };
    u32 selected_role_index{};
    u32 maps_role_count{};
    u32 out_of_bounds_role_count{};
    u32 action_update_failure_count{};
};

[[nodiscard]] u32 shift_tile_coordinate(const u16 value) noexcept {
    return static_cast<u32>(value) << 4U;
}

[[nodiscard]] u32 sign_extend_word(const compat::i16 value) noexcept {
    return static_cast<u32>(static_cast<i32>(value));
}

[[nodiscard]] u32 normalize_movement_step(const u32 value) noexcept {
    switch (value) {
    case 1U:
    case 2U:
    case 4U:
    case 8U:
    case 16U:
        return value;
    default:
        return 4U;
    }
}

[[nodiscard]] LegacyWorldMapDescriptorRuntimeState
make_descriptor_runtime_state(
    const LegacyMapsMapDescriptor& descriptor,
    const std::size_t encounter_group_count
) noexcept {
    const u32 descriptor_flags = descriptor.field_04;
    u32 enabled_service_bits = 0U;
    const auto enable_service = [&](const u32 descriptor_bit,
                                    const u32 service_id) noexcept {
        if ((descriptor_flags & descriptor_bit) != 0U) {
            enabled_service_bits |= 1U << service_id;
        }
    };
    enable_service(0x8000U, 15U);
    enable_service(0x4000U, 5U);
    enable_service(0x2000U, 5U);
    enable_service(0x1000U, 6U);
    enable_service(0x0800U, 7U);
    enable_service(0x0400U, 8U);
    enable_service(0x0200U, 19U);
    enable_service(0x0100U, 22U);

    LegacyWorldMapDescriptorRuntimeState state{
        .behavior_index = descriptor_flags & 0x0FU,
        .base_movement_step =
            normalize_movement_step(descriptor.field_06 & 0x0FU),
        .tile_animation_interval = std::max<u32>(descriptor.field_08, 1U),
        .encounter_table_index = descriptor.field_0a,
        .map_state_flags = descriptor.field_0c,
        .role_red_offset =
            kDescriptorColorOffsets[(descriptor.field_06 >> 12U) & 0x0FU],
        .role_green_offset =
            kDescriptorColorOffsets[(descriptor.field_06 >> 8U) & 0x0FU],
        .role_blue_offset =
            kDescriptorColorOffsets[(descriptor.field_06 >> 4U) & 0x0FU],
        .enabled_service_bits = enabled_service_bits,
        .environment_enabled = (descriptor_flags & 0x0010U) == 0U,
        .directional_effect_enabled = (enabled_service_bits & (1U << 5U)) != 0U,
        .directional_variant_count =
            (descriptor_flags & 0x2000U) != 0U ? 8U : 4U,
    };
    if (state.encounter_table_index > encounter_group_count) {
        state.encounter_table_index = 1U;
        state.encounter_table_index_repaired = true;
    }
    return state;
}

void initialize_directional_points(
    const LegacyWorldRuntimeSessionRequest& request,
    const LegacyWorldMapDescriptorRuntimeState& descriptor,
    const u32 map_width,
    const u32 map_height,
    std::array<LegacyWorldDirectionalPoint, 4U>& points
) {
    if (request.random == nullptr || !descriptor.directional_effect_enabled) {
        return;
    }
    for (auto& point : points) {
        point.target_interval = request.random->next_bounded(3U) + 1U;
        point.variant =
            request.random->next_bounded(descriptor.directional_variant_count) +
            descriptor.directional_base_variant;
        point.world_x = request.random->next_bounded(map_width << 4U);
        point.world_y = request.random->next_bounded(map_height << 4U);
        point.velocity_x =
            static_cast<i32>(request.random->next_bounded(2U)) - 2;
        point.velocity_y =
            static_cast<i32>(request.random->next_bounded(2U)) - 2;
    }
}

void initialize_maps_role(
    LegacyWorldRoleRecord& role,
    const LegacyMapsRoleSourceRecord& source,
    const LegacyMapsWorldDatabase& database
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
    static_cast<void>(apply_legacy_maps_role_defaults(database, role));
}

[[nodiscard]] bool assemble_maps_roles(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const std::span<const u32> materialization_source_indices,
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
        for (const u32 source_index : materialization_source_indices) {
            if (source_index >= database.role_sources.size()) {
                assembly.status =
                    LegacyWorldRuntimeSessionStatus::maps_load_apply_failed;
                return false;
            }
            auto& source = database.role_sources[source_index];
            if (source.logical_map_id != request.logical_map_id) {
                continue;
            }

            if (static_cast<u32>(source.tile_x) > map_session.header.width ||
                static_cast<u32>(source.tile_y) > map_session.header.height) {
                source.tile_x = static_cast<u16>(map_session.header.width - 1U);
                source.tile_y =
                    static_cast<u16>(map_session.header.height - 1U);
                if (!write_legacy_maps_role_source_record(payload, source)) {
                    assembly.status = LegacyWorldRuntimeSessionStatus::
                        role_source_write_failed;
                    return false;
                }
                ++assembly.out_of_bounds_role_count;
                continue;
            }
            if (roles.size() >= kLegacyWorldRoleCapacity) {
                assembly.status =
                    LegacyWorldRuntimeSessionStatus::role_capacity_exceeded;
                return false;
            }

            LegacyWorldRoleRecord role;
            initialize_maps_role(role, source, database);
            roles.push_back(role);
            const u32 role_index = static_cast<u32>(roles.size() - 1U);
            if (!insert_legacy_role_spatially(
                    business.spatial_index, roles, role_index
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
        assembly.status =
            LegacyWorldRuntimeSessionStatus::selected_role_not_materialized;
        return false;
    }

    map_session.role_cell_binding = {
        .status = LegacyWorldRoleCellBindingStatus::ready,
    };
    const LegacyWorldRoleSurfaceContext surface_context{
        .map_width = map_session.header.width,
        .selected_guid = request.selected_guid,
        .surface_grid = map_session.surface_grid.surface_grid,
    };
    for (std::size_t role_index = 1U; role_index < roles.size(); ++role_index) {
        auto& role = roles[role_index];
        role.action.mode_flags = 0U;
        if (action_initializer.initialize_action(role.action) == 0U) {
            ++assembly.action_update_failure_count;
        }

        const auto binding = bind_legacy_world_role_cells(
            roles,
            static_cast<u32>(role_index),
            static_cast<u32>(role_index + 1U),
            map_session.header.width,
            map_session.surface_grid.surface_grid
        );
        map_session.role_cell_binding.roles_bound += binding.roles_bound;
        map_session.role_cell_binding.out_of_bounds_indices +=
            binding.out_of_bounds_indices;
        if (binding.status != LegacyWorldRoleCellBindingStatus::ready) {
            map_session.role_cell_binding.status = binding.status;
            assembly.status =
                LegacyWorldRuntimeSessionStatus::role_cell_binding_failed;
            return false;
        }

        if (role.action.action_id != 0U &&
            (role.flags & 0x00008400U) == 0x00008000U) {
            const auto occupancy =
                mark_legacy_world_role_surface_occupancy(role, surface_context);
            if (occupancy.status != LegacyWorldRoleSurfaceStatus::ready) {
                assembly.status = LegacyWorldRuntimeSessionStatus::
                    role_surface_occupancy_failed;
                return false;
            }
        }
    }
    map_session.role_cell_binding_completed = true;
    return true;
}

}  // namespace

LegacyWorldActionUpdaterInitializer::LegacyWorldActionUpdaterInitializer(
    asset_runtime::LegacyActionUpdater& updater
) noexcept
    : updater_{updater} {}

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

    result.session.encounter_thresholds =
        load_legacy_encounter_thresholds(maps_payload);
    if (result.session.encounter_thresholds.status !=
        LegacyEncounterSourceStatus::ready) {
        result.status =
            LegacyWorldRuntimeSessionStatus::encounter_thresholds_failed;
        return result;
    }

    const auto* const descriptor = find_legacy_maps_map_descriptor(
        result.session.maps_database, request.load.logical_map_id
    );
    if (descriptor == nullptr) {
        result.status =
            LegacyWorldRuntimeSessionStatus::map_descriptor_not_found;
        return result;
    }
    result.session.map_descriptor = *descriptor;
    result.session.map_descriptor_runtime = make_descriptor_runtime_state(
        *descriptor, result.session.encounter_thresholds.groups.size()
    );
    result.session.map_name_lookup = copy_legacy_maps_map_name(
        maps_payload, request.load.logical_map_id, result.session.map_name
    );
    if (result.session.map_name_lookup.status !=
        LegacyMapsMapNameLookupStatus::found) {
        std::ranges::copy(kUnknownMapName, result.session.map_name.begin());
    }

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
            initialize_directional_points(
                request,
                result.session.map_descriptor_runtime,
                map_session.header.width,
                map_session.header.height,
                result.session.directional_points
            );
            result.session.encounter_regions = load_legacy_encounter_regions(
                maps_payload, request.load.logical_map_id
            );
            if (result.session.encounter_regions.status !=
                LegacyEncounterSourceStatus::ready) {
                assembly.status =
                    LegacyWorldRuntimeSessionStatus::encounter_regions_failed;
                return false;
            }
            result.session.maps_load_apply = apply_legacy_maps_world_load(
                maps_payload, result.session.maps_database, request.load
            );
            if (result.session.maps_load_apply.status !=
                LegacyMapsWorldLoadApplyStatus::ready) {
                assembly.status =
                    LegacyWorldRuntimeSessionStatus::maps_load_apply_failed;
                return false;
            }
            return assemble_maps_roles(
                maps_payload,
                result.session.maps_database,
                result.session.maps_load_apply.materialization_source_indices,
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
    result.session.duplicate_role_count =
        result.session.maps_load_apply.duplicate_records_skipped;
    result.session.out_of_bounds_role_count = assembly.out_of_bounds_role_count;
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
        result.status =
            LegacyWorldRuntimeSessionStatus::selected_role_not_materialized;
        return result;
    }
    recenter_legacy_world_camera(
        roles[result.session.selected_role_index],
        map_session.header.width,
        map_session.header.height,
        result.session.camera
    );

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
        maps_payload, request, action_initializer, map_source, cm_cache_source
    );
}

}  // namespace openswd3::world_map
