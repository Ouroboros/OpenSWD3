#include "openswd3/world_map/legacy_world_map_business.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <bit>
#include <cstddef>
#include <limits>
#include <new>
#include <stdexcept>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kLegacyRoleMapFlagMask = 0xDF0FFFFFU;
constexpr u32 kLegacyRoleMapNibbleMask = 0x0000F000U;
constexpr u32 kLegacyRoleMappedNibbleClearMask = 0xFF0FFFFFU;
constexpr u32 kLegacyRoleCellBit11 = 0x00000800U;
constexpr u32 kLegacyRoleMappedBit11 = 0x20000000U;
constexpr u32 kLegacyRoleReadsMapCellFlag = 0x00000100U;

[[nodiscard]] u32 shift_signed_coordinate(const compat::i16 value) noexcept {
    return static_cast<u32>(static_cast<i32>(value)) << 4U;
}

[[nodiscard]] u16 shift_word_coordinate(const u16 value) noexcept {
    return static_cast<u16>(static_cast<u32>(value) << 4U);
}

[[nodiscard]] u32 read_u32_le(
    const std::span<const u8> bytes,
    const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void initialize_role(LegacyWorldRoleRecord& role) noexcept {
    role = {};
    asset_runtime::initialize_legacy_action_record(role.action);
}

void apply_packed_role_fields(
    LegacyWorldRoleRecord& role,
    const u16 packed
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
         static_cast<u32>(source.field_04)) << 4U;
    role.action.field_4c = static_cast<u16>(source.field_00 - 1U);

    const u32 upper_y = (map_height + kLegacySpatialRowPadding) << 4U;
    if (role.world_y >= upper_y ||
        std::bit_cast<i32>(role.world_y) <= -320) {
        return true;
    }

    state.roles.push_back(role);
    const u32 role_index = static_cast<u32>(state.roles.size() - 1U);
    if (!insert_legacy_role_spatially(
            state.spatial_index,
            state.roles,
            role_index
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
            role_index
        )) {
        state.roles.pop_back();
        return false;
    }
    ++state.offset1c_role_count;
    return true;
}

}  // namespace

bool insert_legacy_role_spatially(
    LegacyRoleSpatialIndex& spatial_index,
    const std::span<LegacyWorldRoleRecord> roles,
    const u32 role_index
) noexcept {
    if (role_index == kLegacySpatialNoRole || role_index >= roles.size()) {
        return false;
    }

    LegacyWorldRoleRecord& inserted = roles[role_index];
    const u32 group = inserted.flags & 3U;
    if (group >= kLegacySpatialGroupCount) {
        return false;
    }
    const i32 row = std::bit_cast<i32>(inserted.world_y) / 16;
    const i32 padded_row = row + static_cast<i32>(kLegacySpatialRowPadding);
    if (padded_row < 0 ||
        static_cast<std::size_t>(padded_row) >=
            spatial_index.row_heads[group].size()) {
        return false;
    }

    u32& head = spatial_index.row_heads[group][
        static_cast<std::size_t>(padded_row)
    ];
    inserted.spatial_next_link_32 = kLegacySpatialNoRole;
    if (head == kLegacySpatialNoRole) {
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
            current->guid >= inserted.guid &&
            next.guid <= inserted.guid) {
            inserted.spatial_next_link_32 = next_index;
            current->spatial_next_link_32 = role_index;
            return true;
        }

        current_index = next_index;
        current = &next;
        next_index = current->spatial_next_link_32;
        if (next_index == kLegacySpatialNoRole) {
            current->spatial_next_link_32 = role_index;
            return true;
        }
    }
}

LegacyWorldMapBusinessResult build_legacy_world_map_business_state(
    const resource_io::LegacyLmfMapHeader& header,
    const resource_io::LegacyLmfPostSurfaceRecords& post_surface_records,
    const resource_io::LegacyLmfOffset14Directory& offset14_directory,
    const resource_io::LegacyLmfOffset1cDirectory& offset1c_directory
) {
    LegacyWorldMapBusinessResult result;
    if (header.status != resource_io::LegacyLmfMapHeaderStatus::ready ||
        post_surface_records.status !=
            resource_io::LegacyLmfPostSurfaceRecordsStatus::ready ||
        offset14_directory.status !=
            resource_io::LegacyLmfOffset14DirectoryStatus::ready ||
        offset1c_directory.status !=
            resource_io::LegacyLmfOffset1cDirectoryStatus::ready) {
        return result;
    }

    if (header.height >
        std::numeric_limits<u32>::max() - 2U * kLegacySpatialRowPadding) {
        result.status =
            LegacyWorldMapBusinessStatus::spatial_index_size_overflow;
        return result;
    }
    const std::size_t spatial_rows = static_cast<std::size_t>(
        header.height + 2U * kLegacySpatialRowPadding
    );

    try {
        result.state.events.reserve(post_surface_records.records.size());
        for (auto record = post_surface_records.records.crbegin();
             record != post_surface_records.records.crend(); ++record) {
            result.state.events.push_back({
                record->field_00,
                record->field_02,
                record->field_06,
                record->field_0a,
                record->name_bytes_with_terminator,
            });
        }

        result.state.roles.reserve(kLegacyWorldRoleCapacity);
        result.state.roles.emplace_back();
        initialize_role(result.state.roles.front());
        result.state.spatial_index.map_height = header.height;
        for (auto& group : result.state.spatial_index.row_heads) {
            group.resize(spatial_rows, kLegacySpatialNoRole);
        }

        for (const auto& record : offset14_directory.records) {
            if (!append_offset14_role(result.state, record, header.height)) {
                if (result.state.roles.size() >= kLegacyWorldRoleCapacity) {
                    result.status =
                        LegacyWorldMapBusinessStatus::role_capacity_exceeded;
                } else if ((record.field_0a & 3U) >=
                           kLegacySpatialGroupCount) {
                    result.status = LegacyWorldMapBusinessStatus::
                        unsupported_spatial_group;
                } else {
                    result.status = LegacyWorldMapBusinessStatus::
                        spatial_row_out_of_range;
                }
                return result;
            }
        }
        for (const auto& record : offset1c_directory.records) {
            if (!append_offset1c_role(result.state, record, header.height)) {
                if (result.state.roles.size() >= kLegacyWorldRoleCapacity) {
                    result.status =
                        LegacyWorldMapBusinessStatus::role_capacity_exceeded;
                } else if ((record.packed_field_08 & 3U) >=
                           kLegacySpatialGroupCount) {
                    result.status = LegacyWorldMapBusinessStatus::
                        unsupported_spatial_group;
                } else {
                    result.status = LegacyWorldMapBusinessStatus::
                        spatial_row_out_of_range;
                }
                return result;
            }
        }
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
        role.action.mode_flags = 0U;
        role.flags &= kLegacyRoleMapFlagMask;

        const u32 cell_index =
            (role.world_y >> 4U) * map_width + (role.world_x >> 4U);
        role.map_cell_pointer_32 = cell_index;
        ++result.roles_bound;

        const bool in_bounds = static_cast<std::size_t>(cell_index) <
            surface_grid.size() / 4U;
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

        const std::size_t byte_offset =
            static_cast<std::size_t>(cell_index) * 4U;
        const u32 cell = read_u32_le(surface_grid, byte_offset);
        if ((cell & kLegacyRoleCellBit11) != 0U) {
            role.flags |= kLegacyRoleMappedBit11;
        }
        const u32 cell_nibble = cell & kLegacyRoleMapNibbleMask;
        if (cell_nibble != 0U) {
            role.flags = (role.flags & kLegacyRoleMappedNibbleClearMask) |
                (cell_nibble << 8U);
        }
    }
    return result;
}

}  // namespace openswd3::world_map
