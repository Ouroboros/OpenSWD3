#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_role_surface_occupancy.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kLegacyRoleMapFlagMask = 0xDF0FFFFFU;
constexpr u32 kLegacyRoleReadsMapCellFlag = 0x00000100U;

[[nodiscard]] u32 shift_signed_coordinate(const compat::i16 value) noexcept {
    return static_cast<u32>(static_cast<i32>(value)) << 4U;
}

[[nodiscard]] u16 shift_word_coordinate(const u16 value) noexcept {
    return static_cast<u16>(static_cast<u32>(value) << 4U);
}

void initialize_role(LegacyWorldRoleRecord& role) noexcept {
    role = {};
    asset_runtime::initialize_legacy_action_record(role.action);
}

void apply_packed_role_fields(
    LegacyWorldRoleRecord& role, const u16 packed
) noexcept {
    role.flags = 0x00009000U | (static_cast<u32>(packed) & 3U);
    role.field_28 = static_cast<u16>(packed >> 12U);
    role.field_2a = static_cast<u16>((packed >> 8U) & 0x000FU);
    role.guid = 0xFFFFU;
}

[[nodiscard]] bool append_offset14_role(
    LegacyWorldMapBusinessState& state,
    const resource_io::LegacyLmfOffset14Record& source,
    const u32 map_height
) {
    if (state.roles.size() >= kLegacyWorldRoleCapacity) {
        return false;
    }

    LegacyWorldRoleRecord role;
    initialize_role(role);
    apply_packed_role_fields(role, source.field_0a);
    role.world_x = shift_signed_coordinate(source.field_02);
    role.world_y = shift_signed_coordinate(source.field_08);
    role.action.action_id = 0U;
    role.action.draw_offset_x = 0U;
    role.action.draw_offset_y =
        (static_cast<u32>(static_cast<u16>(source.field_08)) -
         static_cast<u32>(source.field_04))
        << 4U;
    role.action.field_4c = static_cast<u16>(source.field_00 - 1U);

    const u32 upper_y = (map_height + kLegacySpatialRowPadding) << 4U;
    if (role.world_y >= upper_y || std::bit_cast<i32>(role.world_y) <= -320) {
        return true;
    }

    state.roles.push_back(role);
    const u32 role_index = static_cast<u32>(state.roles.size() - 1U);
    if (!insert_legacy_role_spatially(
            state.spatial_index,
            state.roles,
            role_index,
            state.roles[role_index].flags & 3U
        )) {
        state.roles.pop_back();
        return false;
    }
    ++state.offset14_role_count;
    return true;
}

[[nodiscard]] bool append_offset1c_role(
    LegacyWorldMapBusinessState& state,
    const resource_io::LegacyLmfOffset1cRecord& source,
    const u32 map_height
) {
    if (state.roles.size() >= kLegacyWorldRoleCapacity) {
        return false;
    }

    LegacyWorldRoleRecord role;
    initialize_role(role);
    role.action.action_id = source.field_00;
    role.action.base_variant = source.field_02;
    role.world_x = static_cast<u32>(shift_word_coordinate(source.field_04));
    role.world_y = static_cast<u32>(shift_word_coordinate(source.field_06));
    apply_packed_role_fields(role, source.packed_field_08);
    role.action.field_88 = 0U;

    if (role.world_y >= (map_height << 4U)) {
        return true;
    }

    state.roles.push_back(role);
    const u32 role_index = static_cast<u32>(state.roles.size() - 1U);
    if (!insert_legacy_role_spatially(
            state.spatial_index,
            state.roles,
            role_index,
            state.roles[role_index].flags & 3U
        )) {
        state.roles.pop_back();
        return false;
    }
    ++state.offset1c_role_count;
    return true;
}

}  // namespace

const LegacyWorldMapEvent* find_legacy_world_map_event(
    const std::span<const LegacyWorldMapEvent> events, const u32 event_code
) noexcept {
    for (const LegacyWorldMapEvent& event : events) {
        if (event.field_04 == event_code) {
            return &event;
        }
    }
    return nullptr;
}

LegacyRoleSpatialIndexRebuildStatus rebuild_legacy_role_spatial_index(
    LegacyRoleSpatialIndex& spatial_index, const u32 map_height
) noexcept {
    constexpr u32 kRowBytes = static_cast<u32>(sizeof(u32));
    constexpr u32 kPrefixAndSuffixBytes =
        2U * kLegacySpatialRowPadding * kRowBytes;
    constexpr u32 kMaximumHeightWithoutByteCountWrap =
        (std::numeric_limits<u32>::max() - kPrefixAndSuffixBytes) / kRowBytes;
    if (map_height > kMaximumHeightWithoutByteCountWrap) {
        return LegacyRoleSpatialIndexRebuildStatus::allocation_size_overflow;
    }

    LegacyRoleSpatialIndex empty;
    spatial_index = std::move(empty);
    spatial_index.map_height = map_height;
    const std::size_t row_count = static_cast<std::size_t>(map_height) +
        2U * static_cast<std::size_t>(kLegacySpatialRowPadding);
    try {
        for (auto& group : spatial_index.row_heads) {
            group.assign(row_count, kLegacySpatialNoRole);
        }
    } catch (const std::bad_alloc&) {
        return LegacyRoleSpatialIndexRebuildStatus::allocation_failed;
    } catch (const std::length_error&) {
        return LegacyRoleSpatialIndexRebuildStatus::allocation_failed;
    }

    return LegacyRoleSpatialIndexRebuildStatus::ready;
}

bool insert_legacy_role_spatially(
    LegacyRoleSpatialIndex& spatial_index,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index,
    const u32 group
) noexcept {
    if (role_index == kLegacySpatialNoRole || role_index >= roles.size()) {
        return false;
    }

    if (group >= kLegacySpatialGroupCount) {
        return false;
    }
    LegacyWorldRoleRecord& inserted = roles[role_index];
    const i32 row = std::bit_cast<i32>(inserted.world_y) / 16;
    const i32 padded_row = row + static_cast<i32>(kLegacySpatialRowPadding);
    if (padded_row < 0 ||
        static_cast<std::size_t>(padded_row) >=
            spatial_index.row_heads[group].size()) {
        return false;
    }

    u32& head =
        spatial_index.row_heads[group][static_cast<std::size_t>(padded_row)];
    if (head == kLegacySpatialNoRole) {
        inserted.spatial_next_link_32 = kLegacySpatialNoRole;
        head = role_index;
        return true;
    }
    if (head >= roles.size()) {
        return false;
    }

    u32 current_index = head;
    LegacyWorldRoleRecord* current = &roles[current_index];
    u32 next_index = current->spatial_next_link_32;
    if (next_index == kLegacySpatialNoRole) {
        if (current->world_y > inserted.world_y ||
            current->guid < inserted.guid) {
            inserted.spatial_next_link_32 = current_index;
            head = role_index;
        } else {
            inserted.spatial_next_link_32 = kLegacySpatialNoRole;
            current->spatial_next_link_32 = role_index;
        }
        return true;
    }

    if (current->world_y <= inserted.world_y &&
        inserted.guid >= current->guid) {
        inserted.spatial_next_link_32 = current_index;
        head = role_index;
        return true;
    }

    while (true) {
        if (next_index >= roles.size()) {
            return false;
        }
        LegacyWorldRoleRecord& next = roles[next_index];
        if (current->world_y <= inserted.world_y &&
            current->guid >= inserted.guid && next.guid <= inserted.guid) {
            inserted.spatial_next_link_32 = next_index;
            current->spatial_next_link_32 = role_index;
            return true;
        }

        current_index = next_index;
        current = &next;
        next_index = current->spatial_next_link_32;
        if (next_index == kLegacySpatialNoRole) {
            inserted.spatial_next_link_32 = kLegacySpatialNoRole;
            current->spatial_next_link_32 = role_index;
            return true;
        }
    }
}

LegacyRoleSpatialRelocationResult relocate_legacy_role_spatially_by_guid(
    LegacyRoleSpatialIndex& spatial_index,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 guid,
    const u32 group,
    const i32 first_row,
    const bool reinsert
) noexcept {
    if (group >= kLegacySpatialGroupCount) {
        return {.status = LegacyRoleSpatialRelocationStatus::invalid_group};
    }

    const i32 map_height = std::bit_cast<i32>(spatial_index.map_height);
    if (first_row >= map_height) {
        return {};
    }

    const std::int64_t first_padded_row = static_cast<std::int64_t>(first_row) +
        static_cast<std::int64_t>(kLegacySpatialRowPadding);
    if (first_padded_row < 0 ||
        static_cast<std::size_t>(first_padded_row) >=
            spatial_index.row_heads[group].size()) {
        return {
            .status = LegacyRoleSpatialRelocationStatus::first_row_out_of_range,
        };
    }

    for (i32 row = first_row; row < map_height; ++row) {
        const std::int64_t padded_row = static_cast<std::int64_t>(row) +
            static_cast<std::int64_t>(kLegacySpatialRowPadding);
        if (padded_row < 0 ||
            static_cast<std::size_t>(padded_row) >=
                spatial_index.row_heads[group].size()) {
            return {.status = LegacyRoleSpatialRelocationStatus::broken_link};
        }

        u32* link =
            &spatial_index
                 .row_heads[group][static_cast<std::size_t>(padded_row)];
        std::size_t traversed = 0U;
        while (*link != kLegacySpatialNoRole) {
            if (*link >= roles.size() || traversed++ >= roles.size()) {
                return {
                    .status = LegacyRoleSpatialRelocationStatus::broken_link,
                };
            }

            const u32 role_index = *link;
            LegacyWorldRoleRecord& role = roles[role_index];
            if (role.guid == guid) {
                *link = role.spatial_next_link_32;
                role.spatial_next_link_32 = kLegacySpatialNoRole;
                if (reinsert &&
                    !insert_legacy_role_spatially(
                        spatial_index, roles, role_index, role.flags & 3U
                    )) {
                    return {
                        .status = LegacyRoleSpatialRelocationStatus::
                            reinsertion_failed,
                    };
                }
                return {
                    .status = LegacyRoleSpatialRelocationStatus::ready,
                    .legacy_return_role_index = reinsert ? 0U : role_index,
                };
            }
            link = &role.spatial_next_link_32;
        }
    }

    return {};
}

LegacyWorldMapBusinessResult begin_legacy_world_map_business_state(
    const resource_io::LegacyLmfMapHeader& header
) {
    LegacyWorldMapBusinessResult result;
    if (header.status != resource_io::LegacyLmfMapHeaderStatus::ready) {
        return result;
    }

    const auto spatial_rebuild_status = rebuild_legacy_role_spatial_index(
        result.state.spatial_index, header.height
    );
    if (spatial_rebuild_status != LegacyRoleSpatialIndexRebuildStatus::ready) {
        result.status = spatial_rebuild_status ==
                LegacyRoleSpatialIndexRebuildStatus::allocation_size_overflow
            ? LegacyWorldMapBusinessStatus::spatial_index_size_overflow
            : LegacyWorldMapBusinessStatus::allocation_failed;
        return result;
    }

    try {
        result.state.roles.reserve(kLegacyWorldRoleCapacity);
        result.state.roles.emplace_back();
        initialize_role(result.state.roles.front());
    } catch (const std::bad_alloc&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
        return result;
    } catch (const std::length_error&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
        return result;
    }

    result.status = LegacyWorldMapBusinessStatus::ready;
    return result;
}

LegacyWorldMapBusinessStatus append_legacy_world_map_events(
    LegacyWorldMapBusinessResult& result,
    const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records
) {
    if (result.status != LegacyWorldMapBusinessStatus::ready ||
        post_surface_records.status !=
            resource_io::LegacyLmfPostSurfaceRecordsStatus::ready) {
        result.status = LegacyWorldMapBusinessStatus::invalid_physical_state;
        return result.status;
    }

    try {
        result.state.events.reserve(post_surface_records.records.size());
        for (auto record = post_surface_records.records.crbegin();
             record != post_surface_records.records.crend();
             ++record) {
            result.state.events.push_back({
                record->field_00,
                record->field_02,
                record->field_06,
                record->field_0a,
                record->name_bytes_with_terminator,
            });
        }
    } catch (const std::bad_alloc&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
    } catch (const std::length_error&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
    }
    return result.status;
}

LegacyWorldMapBusinessStatus append_legacy_world_map_offset14_roles(
    LegacyWorldMapBusinessResult& result,
    const resource_io::LegacyLmfMapHeader& header,
    const resource_io::LegacyLmfOffset14Directory& offset14_directory
) {
    if (result.status != LegacyWorldMapBusinessStatus::ready ||
        header.status != resource_io::LegacyLmfMapHeaderStatus::ready ||
        offset14_directory.status !=
            resource_io::LegacyLmfOffset14DirectoryStatus::ready) {
        result.status = LegacyWorldMapBusinessStatus::invalid_physical_state;
        return result.status;
    }

    try {
        for (const auto& record : offset14_directory.records) {
            if (append_offset14_role(result.state, record, header.height)) {
                continue;
            }
            if (result.state.roles.size() >= kLegacyWorldRoleCapacity) {
                result.status =
                    LegacyWorldMapBusinessStatus::role_capacity_exceeded;
            } else if ((record.field_0a & 3U) >= kLegacySpatialGroupCount) {
                result.status =
                    LegacyWorldMapBusinessStatus::unsupported_spatial_group;
            } else {
                result.status =
                    LegacyWorldMapBusinessStatus::spatial_row_out_of_range;
            }
            return result.status;
        }
    } catch (const std::bad_alloc&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
    } catch (const std::length_error&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
    }
    return result.status;
}

LegacyWorldMapBusinessStatus append_legacy_world_map_offset1c_roles(
    LegacyWorldMapBusinessResult& result,
    const resource_io::LegacyLmfMapHeader& header,
    const resource_io::LegacyLmfOffset1cDirectory& offset1c_directory
) {
    if (result.status != LegacyWorldMapBusinessStatus::ready ||
        header.status != resource_io::LegacyLmfMapHeaderStatus::ready ||
        offset1c_directory.status !=
            resource_io::LegacyLmfOffset1cDirectoryStatus::ready) {
        result.status = LegacyWorldMapBusinessStatus::invalid_physical_state;
        return result.status;
    }

    try {
        for (const auto& record : offset1c_directory.records) {
            if (append_offset1c_role(result.state, record, header.height)) {
                continue;
            }
            if (result.state.roles.size() >= kLegacyWorldRoleCapacity) {
                result.status =
                    LegacyWorldMapBusinessStatus::role_capacity_exceeded;
            } else if (
                (record.packed_field_08 & 3U) >= kLegacySpatialGroupCount
            ) {
                result.status =
                    LegacyWorldMapBusinessStatus::unsupported_spatial_group;
            } else {
                result.status =
                    LegacyWorldMapBusinessStatus::spatial_row_out_of_range;
            }
            return result.status;
        }
    } catch (const std::bad_alloc&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
    } catch (const std::length_error&) {
        result.status = LegacyWorldMapBusinessStatus::allocation_failed;
    }
    return result.status;
}

LegacyWorldMapBusinessResult build_legacy_world_map_business_state(
    const resource_io::LegacyLmfMapHeader& header,
    const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records,
    const resource_io::LegacyLmfOffset14Directory& offset14_directory,
    const resource_io::LegacyLmfOffset1cDirectory& offset1c_directory
) {
    auto result = begin_legacy_world_map_business_state(header);
    if (result.status != LegacyWorldMapBusinessStatus::ready ||
        append_legacy_world_map_events(result, post_surface_records) !=
            LegacyWorldMapBusinessStatus::ready ||
        append_legacy_world_map_offset14_roles(
            result, header, offset14_directory
        ) != LegacyWorldMapBusinessStatus::ready ||
        append_legacy_world_map_offset1c_roles(
            result, header, offset1c_directory
        ) != LegacyWorldMapBusinessStatus::ready) {
        return result;
    }
    return result;
}

LegacyWorldRoleCellBindingResult bind_legacy_world_role_cells(
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 first_role_index,
    const u32 role_count,
    const u32 map_width,
    const std::span<const u8> surface_grid
) noexcept {
    LegacyWorldRoleCellBindingResult result;
    if (first_role_index > role_count || role_count > roles.size()) {
        result.status =
            LegacyWorldRoleCellBindingStatus::role_range_out_of_bounds;
        return result;
    }

    result.status = LegacyWorldRoleCellBindingStatus::ready;
    for (u32 index = first_role_index; index < role_count; ++index) {
        LegacyWorldRoleRecord& role = roles[index];
        role.flags &= kLegacyRoleMapFlagMask;

        const u32 cell_index =
            (role.world_y >> 4U) * map_width + (role.world_x >> 4U);
        role.map_cell_pointer_32 = cell_index;
        ++result.roles_bound;

        const bool in_bounds =
            static_cast<std::size_t>(cell_index) < surface_grid.size() / 4U;
        if (!in_bounds) {
            ++result.out_of_bounds_indices;
            if ((role.flags & kLegacyRoleReadsMapCellFlag) != 0U) {
                result.status = LegacyWorldRoleCellBindingStatus::
                    flagged_cell_out_of_bounds;
            }
            continue;
        }
        if ((role.flags & kLegacyRoleReadsMapCellFlag) == 0U) {
            continue;
        }

        if (refresh_legacy_world_role_cell_flags(role, surface_grid) !=
            LegacyWorldRoleCellFlagRefreshStatus::ready) {
            ++result.out_of_bounds_indices;
            result.status =
                LegacyWorldRoleCellBindingStatus::flagged_cell_out_of_bounds;
        }
    }
    return result;
}

}  // namespace openswd3::world_map
