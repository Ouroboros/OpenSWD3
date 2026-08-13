#include "openswd3/world_map/legacy_maps_world_database.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <new>
#include <stdexcept>

namespace openswd3::world_map {
namespace {

using compat::i16;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kRoleDirectoryOffsetField = 0x04U;
constexpr std::size_t kMapDescriptorDirectoryOffsetField = 0x0CU;
constexpr std::size_t kInitialLoadOffsetField = 0x10U;
constexpr std::size_t kPartyAttributeDirectoryOffsetField = 0x18U;
constexpr std::size_t kRoleDefaultsDirectoryOffsetField = 0x54U;
constexpr u16 kDirectoryTerminator = 0xFFFFU;
constexpr u16 kRoleDefaultsTerminator = 0U;
constexpr u16 kLegacySelectedRoleFlags = 0xD100U;

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

void write_u16_le(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) noexcept {
    bytes[offset] = static_cast<u8>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] bool decode_map_descriptors(
    const std::span<const u8> payload,
    const u32 source_offset,
    LegacyMapsWorldDatabaseResult& result
) {
    std::size_t offset = source_offset;
    if (!range_available(payload, offset, sizeof(u16))) {
        result.status =
            LegacyMapsWorldDatabaseStatus::map_descriptor_offset_out_of_range;
        return false;
    }

    while (range_available(payload, offset, sizeof(u16))) {
        const u16 logical_map_id = read_u16_le(payload, offset);
        if (logical_map_id == kDirectoryTerminator) {
            return true;
        }
        if (!range_available(
                payload, offset, kLegacyMapsMapDescriptorRecordSize
            )) {
            result.status =
                LegacyMapsWorldDatabaseStatus::map_descriptor_record_truncated;
            return false;
        }

        result.database.map_descriptors.push_back({
            static_cast<u32>(offset),
            logical_map_id,
            read_u16_le(payload, offset + 0x02U),
            read_u16_le(payload, offset + 0x04U),
            read_u16_le(payload, offset + 0x06U),
            read_u16_le(payload, offset + 0x08U),
            read_u16_le(payload, offset + 0x0AU),
            read_u16_le(payload, offset + 0x0CU),
        });
        offset += kLegacyMapsMapDescriptorRecordSize;
    }

    result.status =
        LegacyMapsWorldDatabaseStatus::map_descriptor_directory_unterminated;
    return false;
}

[[nodiscard]] bool decode_role_sources(
    const std::span<const u8> payload,
    const u32 source_offset,
    LegacyMapsWorldDatabaseResult& result
) {
    std::size_t offset = source_offset;
    if (!range_available(payload, offset, sizeof(u16))) {
        result.status =
            LegacyMapsWorldDatabaseStatus::role_source_offset_out_of_range;
        return false;
    }

    while (range_available(payload, offset, sizeof(u16))) {
        const u16 logical_map_id = read_u16_le(payload, offset);
        if (logical_map_id == kDirectoryTerminator) {
            return true;
        }
        if (!range_available(
                payload, offset, kLegacyMapsRoleSourceRecordSize
            )) {
            result.status =
                LegacyMapsWorldDatabaseStatus::role_source_record_truncated;
            return false;
        }

        result.database.role_sources.push_back({
            static_cast<u32>(offset),
            logical_map_id,
            read_u16_le(payload, offset + 0x02U),
            read_u16_le(payload, offset + 0x04U),
            read_u16_le(payload, offset + 0x06U),
            read_u16_le(payload, offset + 0x08U),
            read_u16_le(payload, offset + 0x0AU),
            read_u16_le(payload, offset + 0x0CU),
            read_u16_le(payload, offset + 0x0EU),
            read_u16_le(payload, offset + 0x10U),
            std::bit_cast<i16>(read_u16_le(payload, offset + 0x12U)),
            read_u16_le(payload, offset + 0x14U),
        });
        offset += kLegacyMapsRoleSourceRecordSize;
    }

    result.status =
        LegacyMapsWorldDatabaseStatus::role_source_directory_unterminated;
    return false;
}

[[nodiscard]] bool decode_role_defaults(
    const std::span<const u8> payload,
    const u32 source_offset,
    LegacyMapsWorldDatabaseResult& result
) {
    std::size_t offset = source_offset;
    if (!range_available(payload, offset, sizeof(u16))) {
        result.status =
            LegacyMapsWorldDatabaseStatus::role_defaults_offset_out_of_range;
        return false;
    }

    while (range_available(payload, offset, sizeof(u16))) {
        const u16 guid = read_u16_le(payload, offset);
        if (guid == kRoleDefaultsTerminator) {
            return true;
        }
        if (!range_available(
                payload, offset, kLegacyMapsRoleDefaultsRecordSize
            )) {
            result.status =
                LegacyMapsWorldDatabaseStatus::role_defaults_record_truncated;
            return false;
        }

        result.database.role_defaults.push_back({
            guid,
            read_u16_le(payload, offset + 0x02U),
            read_u16_le(payload, offset + 0x04U),
        });
        offset += kLegacyMapsRoleDefaultsRecordSize;
    }

    result.status =
        LegacyMapsWorldDatabaseStatus::role_defaults_directory_unterminated;
    return false;
}

}  // namespace

u8 materialize_legacy_maps_party_attribute_record(
    const std::span<u8, kLegacyMapsPartyAttributeRuntimeRecordSize> destination,
    const std::span<const u8, kLegacyMapsPartyAttributeSourceRecordSize> source
) noexcept {
    std::copy_n(source.begin() + 0x00U, 0x04U, destination.begin() + 0x00U);
    std::copy_n(source.begin() + 0x04U, 0x06U, destination.begin() + 0x0AU);
    std::copy_n(source.begin() + 0x0AU, 0x06U, destination.begin() + 0x04U);
    std::copy_n(source.begin() + 0x10U, 0x10U, destination.begin() + 0x10U);
    std::copy_n(source.begin() + 0x20U, 0x02U, destination.begin() + 0x2AU);
    destination[0x20U] = source[0x22U];
    destination[0x21U] = source[0x23U];
    destination[0x22U] = 0U;
    destination[0x23U] = 0U;
    std::copy_n(source.begin() + 0x24U, 0x06U, destination.begin() + 0x24U);
    std::copy_n(source.begin() + 0x2AU, 0x0AU, destination.begin() + 0x2CU);
    return source[0x33U];
}

LegacyMapsWorldDatabaseResult
decode_legacy_maps_world_database(const std::span<const u8> payload) {
    LegacyMapsWorldDatabaseResult result;
    if (payload.size() < kLegacyMapsWorldHeaderMinimumSize) {
        return result;
    }

    result.database.header = {
        read_u32_le(payload, kRoleDirectoryOffsetField),
        read_u32_le(payload, kMapDescriptorDirectoryOffsetField),
        read_u32_le(payload, kInitialLoadOffsetField),
        read_u32_le(payload, kPartyAttributeDirectoryOffsetField),
        read_u32_le(payload, kRoleDefaultsDirectoryOffsetField),
    };

    const std::size_t initial_offset =
        result.database.header.initial_load_offset;
    if (!range_available(
            payload, initial_offset, kLegacyMapsInitialLoadRecordSize
        )) {
        result.status =
            LegacyMapsWorldDatabaseStatus::initial_load_record_out_of_range;
        return result;
    }
    result.database.initial_load = {
        read_u16_le(payload, initial_offset + 0x00U),
        read_u16_le(payload, initial_offset + 0x02U),
        read_u16_le(payload, initial_offset + 0x04U),
        read_u16_le(payload, initial_offset + 0x06U),
        read_u16_le(payload, initial_offset + 0x08U),
        read_u16_le(payload, initial_offset + 0x0AU),
        read_u16_le(payload, initial_offset + 0x0CU),
        0U,
    };

    const std::size_t party_attribute_offset =
        result.database.header.party_attribute_directory_offset;
    constexpr std::size_t kPartyAttributeSourceBytes =
        kLegacyMapsPartyAttributeRecordCount *
        kLegacyMapsPartyAttributeSourceRecordSize;
    if (!range_available(
            payload, party_attribute_offset, kPartyAttributeSourceBytes
        )) {
        result.status =
            LegacyMapsWorldDatabaseStatus::party_attribute_records_out_of_range;
        return result;
    }
    for (std::size_t index = 0U; index < kLegacyMapsPartyAttributeRecordCount;
         ++index) {
        const std::size_t source_offset = party_attribute_offset +
            index * kLegacyMapsPartyAttributeSourceRecordSize;
        static_cast<void>(materialize_legacy_maps_party_attribute_record(
            result.database.party_attributes[index],
            std::span<const u8, kLegacyMapsPartyAttributeSourceRecordSize>{
                payload.subspan(
                    source_offset, kLegacyMapsPartyAttributeSourceRecordSize
                )
            }
        ));
    }

    try {
        if (!decode_map_descriptors(
                payload,
                result.database.header.map_descriptor_directory_offset,
                result
            ) ||
            !decode_role_sources(
                payload, result.database.header.role_directory_offset, result
            ) ||
            !decode_role_defaults(
                payload,
                result.database.header.role_defaults_directory_offset,
                result
            )) {
            return result;
        }
    } catch (const std::bad_alloc&) {
        result.status = LegacyMapsWorldDatabaseStatus::allocation_failed;
        return result;
    } catch (const std::length_error&) {
        result.status = LegacyMapsWorldDatabaseStatus::allocation_failed;
        return result;
    }

    result.status = LegacyMapsWorldDatabaseStatus::ready;
    return result;
}

const LegacyMapsMapDescriptor* find_legacy_maps_map_descriptor(
    const LegacyMapsWorldDatabase& database, const u16 logical_map_id
) noexcept {
    const auto found = std::ranges::find(
        database.map_descriptors,
        logical_map_id,
        &LegacyMapsMapDescriptor::logical_map_id
    );
    return found == database.map_descriptors.end() ? nullptr : &*found;
}

const LegacyMapsRoleDefaultsRecord* find_legacy_maps_role_defaults(
    const LegacyMapsWorldDatabase& database, const u16 guid
) noexcept {
    const auto found = std::ranges::find(
        database.role_defaults, guid, &LegacyMapsRoleDefaultsRecord::guid
    );
    return found == database.role_defaults.end() ? nullptr : &*found;
}

bool apply_legacy_maps_role_defaults(
    const LegacyMapsWorldDatabase& database, LegacyWorldRoleRecord& role
) noexcept {
    const auto* const defaults =
        find_legacy_maps_role_defaults(database, role.guid);
    if (defaults == nullptr) {
        return false;
    }

    role.field_2c = (role.field_2c & 0xFFFF0000U) | defaults->field_2c;
    const u32 repeated = defaults->repeated_field_30_word;
    role.field_30 = repeated | (repeated << 16U);
    return true;
}

u32 load_legacy_maps_role_source_record(
    const LegacyMapsWorldDatabase& database,
    const u16 guid,
    LegacyWorldRoleRecord& role
) noexcept {
    const auto found = std::ranges::find(
        database.role_sources, guid, &LegacyMapsRoleSourceRecord::guid
    );
    if (found == database.role_sources.end()) {
        return std::numeric_limits<u32>::max();
    }

    role.guid = guid;
    role.action.action_id = found->action_id;
    role.action.base_variant = found->base_variant;
    role.action.variant_delta = found->variant_delta;
    role.world_x = static_cast<u32>(found->tile_x) << 4U;
    role.world_y = static_cast<u32>(found->tile_y) << 4U;
    role.talk_script_id = found->talk_script_id;
    role.path_data_id = found->path_data_id;
    role.path_word_index = 0U;
    role.flags = found->flags;
    return found->logical_map_id;
}

LegacyMapsRolePatchStatus patch_legacy_maps_role_source_record(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const LegacyMapsRolePatchRequest& request
) noexcept {
    const auto found = std::ranges::find(
        database.role_sources, request.guid, &LegacyMapsRoleSourceRecord::guid
    );
    if (found == database.role_sources.end()) {
        return LegacyMapsRolePatchStatus::guid_not_found;
    }
    if (!range_available(
            payload, found->payload_offset, kLegacyMapsRoleSourceRecordSize
        )) {
        return LegacyMapsRolePatchStatus::source_record_out_of_range;
    }

    auto apply = [](u16& field, const u16 value) noexcept {
        if (value != kLegacyMapsPreserveRoleField) {
            field = value;
        }
    };

    apply(found->action_id, request.action_id);
    apply(found->base_variant, request.base_variant);
    apply(found->variant_delta, request.variant_delta);
    apply(found->tile_x, request.tile_x);
    apply(found->tile_y, request.tile_y);
    apply(found->talk_script_id, request.talk_script_id);
    if (request.path_data_id != kLegacyMapsPreserveRoleField) {
        found->path_data_id = request.path_data_id;
        found->path_word_index = 0;
    }

    if (request.flags_and_mask != kLegacyMapsPreserveRoleField) {
        found->flags &= request.flags_and_mask;
    }

    if (request.flags_or_mask != kLegacyMapsPreserveRoleField) {
        found->flags |= request.flags_or_mask;
    }

    apply(found->logical_map_id, request.logical_map_id);

    if (!write_legacy_maps_role_source_record(payload, *found)) {
        return LegacyMapsRolePatchStatus::source_record_out_of_range;
    }

    return LegacyMapsRolePatchStatus::ready;
}

LegacyMapsRolePatchStatus synchronize_legacy_maps_role_source_record(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const LegacyWorldRoleRecord& role
) noexcept {
    const auto found = std::ranges::find(
        database.role_sources, role.guid, &LegacyMapsRoleSourceRecord::guid
    );
    if (found == database.role_sources.end()) {
        return LegacyMapsRolePatchStatus::guid_not_found;
    }

    found->action_id = static_cast<u16>(role.action.action_id);
    found->base_variant = static_cast<u16>(role.action.base_variant);
    found->variant_delta = static_cast<u16>(role.action.variant_delta);
    found->tile_x = static_cast<u16>(role.world_x) >> 4U;
    found->tile_y = static_cast<u16>(role.world_y) >> 4U;
    found->talk_script_id = role.talk_script_id;
    found->path_data_id = role.path_data_id;
    found->path_word_index =
        std::bit_cast<i16>(static_cast<u16>(role.path_word_index));
    found->flags = static_cast<u16>(role.flags);
    if (!write_legacy_maps_role_source_record(payload, *found)) {
        return LegacyMapsRolePatchStatus::source_record_out_of_range;
    }
    return LegacyMapsRolePatchStatus::ready;
}

LegacyMapsWorldLoadApplyResult apply_legacy_maps_world_load(
    const std::span<u8> payload,
    LegacyMapsWorldDatabase& database,
    const LegacyWorldLoadRequest& request
) noexcept {
    LegacyMapsWorldLoadApplyResult result;
    for (const auto& role : database.role_sources) {
        if (!range_available(
                payload, role.payload_offset, kLegacyMapsRoleSourceRecordSize
            )) {
            result.status =
                LegacyMapsWorldLoadApplyStatus::source_record_out_of_range;
            return result;
        }
    }

    bool selected_found = false;
    std::size_t source_index = 0U;
    try {
        while (source_index < database.role_sources.size()) {
            std::size_t comparison_index = 0U;
            while (comparison_index < source_index) {
                if (database.role_sources[comparison_index].guid ==
                    database.role_sources[source_index].guid) {
                    ++result.duplicate_records_skipped;
                    ++source_index;
                    ++comparison_index;
                    if (source_index >= database.role_sources.size()) {
                        break;
                    }
                    continue;
                }
                ++comparison_index;
            }
            if (source_index >= database.role_sources.size()) {
                break;
            }

            result.materialization_source_indices.push_back(
                static_cast<u32>(source_index)
            );
            auto& role = database.role_sources[source_index];
            bool changed = false;
            if (role.guid == 0U || role.guid == 10000U || role.guid == 10001U) {
                role.logical_map_id = request.logical_map_id;
                ++result.reserved_records_moved;
                changed = true;
            }
            if (role.guid == request.selected_guid) {
                role.logical_map_id = request.logical_map_id;
                role.action_id = request.action_id;
                role.base_variant = request.base_variant;
                role.variant_delta = request.variant_delta;
                role.tile_x = request.tile_x;
                role.tile_y = request.tile_y;
                role.talk_script_id = 0U;
                role.path_data_id = 0U;
                role.path_word_index = 0;
                role.flags = kLegacySelectedRoleFlags;
                result.selected_source_index = static_cast<u32>(source_index);
                selected_found = true;
                changed = true;
            }
            if (changed) {
                static_cast<void>(
                    write_legacy_maps_role_source_record(payload, role)
                );
            }
            ++source_index;
        }
    } catch (const std::bad_alloc&) {
        result.materialization_source_indices.clear();
        result.status = LegacyMapsWorldLoadApplyStatus::allocation_failed;
        return result;
    } catch (const std::length_error&) {
        result.materialization_source_indices.clear();
        result.status = LegacyMapsWorldLoadApplyStatus::allocation_failed;
        return result;
    }

    if (!selected_found) {
        return result;
    }

    result.status = LegacyMapsWorldLoadApplyStatus::ready;
    return result;
}

bool write_legacy_maps_role_source_record(
    const std::span<u8> payload, const LegacyMapsRoleSourceRecord& role
) noexcept {
    const std::size_t offset = role.payload_offset;
    if (!range_available(payload, offset, kLegacyMapsRoleSourceRecordSize)) {
        return false;
    }

    write_u16_le(payload, offset + 0x00U, role.logical_map_id);
    write_u16_le(payload, offset + 0x02U, role.guid);
    write_u16_le(payload, offset + 0x04U, role.action_id);
    write_u16_le(payload, offset + 0x06U, role.base_variant);
    write_u16_le(payload, offset + 0x08U, role.variant_delta);
    write_u16_le(payload, offset + 0x0AU, role.tile_x);
    write_u16_le(payload, offset + 0x0CU, role.tile_y);
    write_u16_le(payload, offset + 0x0EU, role.talk_script_id);
    write_u16_le(payload, offset + 0x10U, role.path_data_id);
    write_u16_le(
        payload, offset + 0x12U, std::bit_cast<u16>(role.path_word_index)
    );
    write_u16_le(payload, offset + 0x14U, role.flags);
    return true;
}

}  // namespace openswd3::world_map
