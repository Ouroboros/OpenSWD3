#include "openswd3/world_map/legacy_world_role_preload.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kSkippedRoleFlagMask = 0x18000000U;
constexpr u32 kFlaggedRoleMask = 0x00000080U;
constexpr u16 kMissingRoleGuid = 0xFFFFU;
constexpr u16 kPathCoordinateCommand = 8U;
constexpr u32 kCoordinateAlignmentMask = 0xFFFFFFF0U;
constexpr std::size_t kPathDirectoryOffset = 0x200U;

[[nodiscard]] bool range_available(
    const std::span<const u8> bytes,
    const std::size_t offset,
    const std::size_t size
) noexcept {
    return offset <= bytes.size() && size <= bytes.size() - offset;
}

[[nodiscard]] u16 read_u16_le(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_u32_le(
    const std::span<const u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool synchronize_ordinary_role(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const LegacyWorldRoleRecord& role,
    LegacyWorldRolePreloadResult& result
) noexcept {
    const auto status =
        synchronize_legacy_maps_role_source_record(payload, database, role);
    if (status == LegacyMapsRolePatchStatus::guid_not_found) {
        ++result.missing_role_sources;
        return true;
    }
    if (status != LegacyMapsRolePatchStatus::ready) {
        return false;
    }

    ++result.ordinary_roles_synchronized;
    return true;
}

enum class PathCommandReadStatus {
    ready,
    directory_entry_out_of_range,
    command_out_of_range,
};

struct PathCommandReadResult {
    PathCommandReadStatus status{
        PathCommandReadStatus::directory_entry_out_of_range
    };
    u16 command{};
};

[[nodiscard]] PathCommandReadResult read_path_command(
    const std::span<const u8> path_database,
    const u16 path_data_id,
    const u32 path_word_index
) noexcept {
    const u32 directory_offset = static_cast<u32>(
        kPathDirectoryOffset + static_cast<u32>(path_data_id) * sizeof(u32)
    );
    if (!range_available(path_database, directory_offset, sizeof(u32))) {
        return {PathCommandReadStatus::directory_entry_out_of_range, 0U};
    }

    const u32 relative = read_u32_le(path_database, directory_offset);
    const std::int64_t command_offset = static_cast<std::int64_t>(relative) +
        static_cast<std::int64_t>(kPathDirectoryOffset) +
        static_cast<std::int64_t>(std::bit_cast<compat::i32>(path_word_index)) *
            static_cast<std::int64_t>(sizeof(u16));
    if (command_offset < 0 ||
        !range_available(
            path_database, static_cast<std::size_t>(command_offset), sizeof(u16)
        )) {
        return {PathCommandReadStatus::command_out_of_range, 0U};
    }

    return {
        PathCommandReadStatus::ready,
        read_u16_le(path_database, static_cast<std::size_t>(command_offset)),
    };
}

[[nodiscard]] bool apply_type_eight_coordinates(
    LegacyWorldRoleRecord& role,
    const u32 role_index,
    const LegacyWorldRolePreloadContext& context,
    LegacyWorldRolePreloadResult& result
) noexcept {
    if (context.object_slots.size() < kLegacyWorldObjectSlotCount) {
        result.status = LegacyWorldRolePreloadStatus::object_slots_required;
        return false;
    }

    const auto slots = context.object_slots.first(kLegacyWorldObjectSlotCount);
    const auto found = std::ranges::find(
        slots,
        static_cast<u16>(role_index),
        &LegacyWorldObjectSlotPrefix::role_index
    );
    if (found != slots.end()) {
        role.world_x = found->world_x;
        role.world_y = found->world_y;
        ++result.object_coordinate_overrides;

        const u32 map_width = context.current_map_width << 4U;
        const u32 map_height = context.current_map_height << 4U;
        if (role.world_x > map_width || role.world_y > map_height) {
            ++result.out_of_bounds_coordinates;
        }
    }

    ++role.path_word_index;
    ++result.path_type_eight_roles;
    return true;
}

[[nodiscard]] bool patch_flagged_role(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const LegacyWorldRoleRecord& role,
    const LegacyWorldLoadRequest& target,
    LegacyWorldRolePreloadResult& result
) noexcept {
    const auto status = patch_legacy_maps_role_source_record(
        payload,
        database,
        LegacyMapsRolePatchRequest{
            .guid = role.guid,
            .action_id = static_cast<u16>(role.action.action_id),
            .base_variant = static_cast<u16>(role.action.base_variant),
            .variant_delta = static_cast<u16>(role.action.variant_delta),
            .tile_x = static_cast<u16>(target.tile_x),
            .tile_y = static_cast<u16>(target.tile_y),
            .talk_script_id = role.talk_script_id,
            .path_data_id = role.path_data_id,
            .flags_or_mask = static_cast<u16>(role.flags),
            .flags_and_mask = 0U,
            .logical_map_id = static_cast<u16>(target.logical_map_id),
        }
    );
    if (status == LegacyMapsRolePatchStatus::guid_not_found) {
        ++result.missing_role_sources;
        return true;
    }
    if (status != LegacyMapsRolePatchStatus::ready) {
        return false;
    }

    ++result.flagged_roles_patched;
    return true;
}

}  // namespace

LegacyWorldRolePreloadResult preload_legacy_world_roles_before_load(
    const std::span<u8> maps_payload,
    LegacyMapsWorldDatabase& maps_database,
    const LegacyWorldLoadRequest& target,
    const LegacyWorldRolePreloadContext& context
) noexcept {
    LegacyWorldRolePreloadResult result;
    for (std::size_t role_index = 1U; role_index < context.roles.size();
         ++role_index) {
        ++result.roles_visited;
        const LegacyWorldRoleRecord& source_role = context.roles[role_index];
        if ((source_role.flags & kSkippedRoleFlagMask) != 0U ||
            source_role.guid == kMissingRoleGuid ||
            static_cast<u32>(role_index) == context.controlled_role_index) {
            ++result.roles_skipped;
            continue;
        }

        LegacyWorldRoleRecord role = source_role;
        if ((role.flags & kFlaggedRoleMask) != 0U) {
            if (!patch_flagged_role(
                    maps_payload, maps_database, role, target, result
                )) {
                result.status =
                    LegacyWorldRolePreloadStatus::role_source_write_failed;
                return result;
            }

            continue;
        }

        if (role.path_data_id != 0U) {
            role.world_x &= kCoordinateAlignmentMask;
            role.world_y &= kCoordinateAlignmentMask;
            const auto path = read_path_command(
                context.path_database, role.path_data_id, role.path_word_index
            );
            if (path.status ==
                PathCommandReadStatus::directory_entry_out_of_range) {
                result.status = LegacyWorldRolePreloadStatus::
                    path_directory_entry_out_of_range;
                return result;
            }
            if (path.status == PathCommandReadStatus::command_out_of_range) {
                result.status =
                    LegacyWorldRolePreloadStatus::path_command_out_of_range;
                return result;
            }
            if (path.command == kPathCoordinateCommand &&
                !apply_type_eight_coordinates(
                    role, static_cast<u32>(role_index), context, result
                )) {
                return result;
            }
        }

        if (!synchronize_ordinary_role(
                maps_payload, maps_database, role, result
            )) {
            result.status =
                LegacyWorldRolePreloadStatus::role_source_write_failed;
            return result;
        }
    }

    return result;
}

}  // namespace openswd3::world_map
