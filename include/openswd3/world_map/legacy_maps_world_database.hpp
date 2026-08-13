#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyMapsWorldHeaderMinimumSize = 0x58U;
inline constexpr std::size_t kLegacyMapsInitialLoadRecordSize = 0x0EU;
inline constexpr std::size_t kLegacyMapsMapDescriptorRecordSize = 0x0EU;
inline constexpr std::size_t kLegacyMapsRoleSourceRecordSize = 0x16U;
inline constexpr std::size_t kLegacyMapsRoleDefaultsRecordSize = 0x06U;
inline constexpr compat::u16 kLegacyMapsPreserveRoleField = 0xFFFFU;

struct LegacyMapsWorldHeader {
  compat::u32 role_directory_offset{};
  compat::u32 map_descriptor_directory_offset{};
  compat::u32 initial_load_offset{};
  compat::u32 role_defaults_directory_offset{};
};

struct LegacyWorldLoadRequest {
  compat::u16 logical_map_id{};
  compat::u16 tile_x{};
  compat::u16 tile_y{};
  compat::u16 action_id{};
  compat::u16 base_variant{};
  compat::u16 variant_delta{};
  compat::u16 selected_guid{};
  compat::u32 load_flags{};
};

struct LegacyMapsMapDescriptor {
  compat::u32 payload_offset{};
  compat::u16 logical_map_id{};
  compat::u16 archive_map_id{};
  compat::u16 field_04{};
  compat::u16 field_06{};
  compat::u16 field_08{};
  compat::u16 field_0a{};
  compat::u16 field_0c{};
};

struct LegacyMapsRoleSourceRecord {
  compat::u32 payload_offset{};
  compat::u16 logical_map_id{};
  compat::u16 guid{};
  compat::u16 action_id{};
  compat::u16 base_variant{};
  compat::u16 variant_delta{};
  compat::u16 tile_x{};
  compat::u16 tile_y{};
  compat::u16 talk_script_id{};
  compat::u16 path_data_id{};
  compat::i16 path_word_index{};
  compat::u16 flags{};
};

struct LegacyMapsRoleDefaultsRecord {
  compat::u16 guid{};
  compat::u16 field_2c{};
  compat::u16 repeated_field_30_word{};
};

struct LegacyMapsRolePatchRequest {
  compat::u16 guid{};
  compat::u16 action_id{kLegacyMapsPreserveRoleField};
  compat::u16 base_variant{kLegacyMapsPreserveRoleField};
  compat::u16 variant_delta{kLegacyMapsPreserveRoleField};
  compat::u16 tile_x{kLegacyMapsPreserveRoleField};
  compat::u16 tile_y{kLegacyMapsPreserveRoleField};
  compat::u16 talk_script_id{kLegacyMapsPreserveRoleField};
  compat::u16 path_data_id{kLegacyMapsPreserveRoleField};
  compat::u16 flags_or_mask{kLegacyMapsPreserveRoleField};
  compat::u16 flags_and_mask{kLegacyMapsPreserveRoleField};
  compat::u16 logical_map_id{kLegacyMapsPreserveRoleField};
};

struct LegacyMapsWorldDatabase {
  LegacyMapsWorldHeader header;
  LegacyWorldLoadRequest initial_load;
  std::vector<LegacyMapsMapDescriptor> map_descriptors;
  std::vector<LegacyMapsRoleSourceRecord> role_sources;
  std::vector<LegacyMapsRoleDefaultsRecord> role_defaults;
};

enum class LegacyMapsWorldDatabaseStatus {
  ready,
  payload_header_truncated,
  initial_load_record_out_of_range,
  map_descriptor_offset_out_of_range,
  map_descriptor_record_truncated,
  map_descriptor_directory_unterminated,
  role_source_offset_out_of_range,
  role_source_record_truncated,
  role_source_directory_unterminated,
  role_defaults_offset_out_of_range,
  role_defaults_record_truncated,
  role_defaults_directory_unterminated,
  allocation_failed,
};

struct LegacyMapsWorldDatabaseResult {
  LegacyMapsWorldDatabaseStatus status{
      LegacyMapsWorldDatabaseStatus::payload_header_truncated};
  LegacyMapsWorldDatabase database;
};

[[nodiscard]] LegacyMapsWorldDatabaseResult
decode_legacy_maps_world_database(std::span<const compat::u8> payload);

[[nodiscard]] const LegacyMapsMapDescriptor *
find_legacy_maps_map_descriptor(const LegacyMapsWorldDatabase &database,
                                compat::u16 logical_map_id) noexcept;

[[nodiscard]] const LegacyMapsRoleDefaultsRecord *
find_legacy_maps_role_defaults(const LegacyMapsWorldDatabase &database,
                               compat::u16 guid) noexcept;

// sub_40D060: apply the optional six-byte defaults record. The original
// writes only the low word at role+0x2c and duplicates the other word into
// both halves of role+0x30.
[[nodiscard]] bool apply_legacy_maps_role_defaults(
    const LegacyMapsWorldDatabase &database,
    LegacyWorldRoleRecord &role) noexcept;

// sub_40D560: materialize the fields owned by a 22-byte MAPS role source.
// Returns its logical map id or 0xffffffff when the GUID is absent. A miss
// leaves the destination record untouched.
[[nodiscard]] compat::u32 load_legacy_maps_role_source_record(
    const LegacyMapsWorldDatabase &database,
    compat::u16 guid,
    LegacyWorldRoleRecord &role) noexcept;

enum class LegacyMapsWorldLoadApplyStatus {
  ready,
  selected_guid_not_found,
  source_record_out_of_range,
};

struct LegacyMapsWorldLoadApplyResult {
  LegacyMapsWorldLoadApplyStatus status{
      LegacyMapsWorldLoadApplyStatus::selected_guid_not_found};
  compat::u32 selected_source_index{};
  compat::u32 reserved_records_moved{};
};

enum class LegacyMapsRolePatchStatus {
  ready,
  guid_not_found,
  source_record_out_of_range,
};

[[nodiscard]] LegacyMapsRolePatchStatus patch_legacy_maps_role_source_record(
    std::span<compat::u8> payload,
    LegacyMapsWorldDatabase &database,
    const LegacyMapsRolePatchRequest &request) noexcept;

// sub_40D3C0: write all persistent runtime-role fields back to the matching
// MAPS role source.
[[nodiscard]] LegacyMapsRolePatchStatus
synchronize_legacy_maps_role_source_record(
    std::span<compat::u8> payload,
    LegacyMapsWorldDatabase &database,
    const LegacyWorldRoleRecord &role) noexcept;

[[nodiscard]] LegacyMapsWorldLoadApplyResult
apply_legacy_maps_world_load(std::span<compat::u8> payload,
                             LegacyMapsWorldDatabase &database,
                             const LegacyWorldLoadRequest &request) noexcept;

[[nodiscard]] bool write_legacy_maps_role_source_record(
    std::span<compat::u8> payload,
    const LegacyMapsRoleSourceRecord &role) noexcept;

} // namespace openswd3::world_map
