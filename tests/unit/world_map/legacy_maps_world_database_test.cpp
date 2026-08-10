#include "test.hpp"

#include "openswd3/world_map/legacy_maps_world_database.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::apply_legacy_maps_world_load;
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::find_legacy_maps_map_descriptor;
using openswd3::world_map::find_legacy_maps_role_defaults;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyMapsWorldLoadApplyStatus;
using openswd3::world_map::LegacyWorldLoadRequest;

void write_u16(const std::span<u8> bytes, const std::size_t offset,
               const u16 value) {
  bytes[offset] = static_cast<u8>(value & 0xFFU);
  bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_i16(const std::span<u8> bytes, const std::size_t offset,
               const i16 value) {
  write_u16(bytes, offset, std::bit_cast<u16>(value));
}

void write_u32(const std::span<u8> bytes, const std::size_t offset,
               const u32 value) {
  bytes[offset] = static_cast<u8>(value & 0xFFU);
  bytes[offset + 1U] = static_cast<u8>((value >> 8U) & 0xFFU);
  bytes[offset + 2U] = static_cast<u8>((value >> 16U) & 0xFFU);
  bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

void write_map_descriptor(const std::span<u8> bytes, const std::size_t offset,
                          const u16 logical_map_id, const u16 archive_map_id) {
  write_u16(bytes, offset + 0x00U, logical_map_id);
  write_u16(bytes, offset + 0x02U, archive_map_id);
  write_u16(bytes, offset + 0x04U, 0x1234U);
  write_u16(bytes, offset + 0x06U, 0x5678U);
  write_u16(bytes, offset + 0x08U, 0x9ABCU);
  write_u16(bytes, offset + 0x0AU, 0xDEF0U);
  write_u16(bytes, offset + 0x0CU, 0x1357U);
}

void write_role_source(const std::span<u8> bytes, const std::size_t offset,
                       const u16 logical_map_id, const u16 guid) {
  write_u16(bytes, offset + 0x00U, logical_map_id);
  write_u16(bytes, offset + 0x02U, guid);
  write_u16(bytes, offset + 0x04U, 2U);
  write_u16(bytes, offset + 0x06U, 3U);
  write_u16(bytes, offset + 0x08U, 4U);
  write_u16(bytes, offset + 0x0AU, 5U);
  write_u16(bytes, offset + 0x0CU, 6U);
  write_u16(bytes, offset + 0x0EU, 7U);
  write_u16(bytes, offset + 0x10U, 8U);
  write_i16(bytes, offset + 0x12U, -9);
  write_u16(bytes, offset + 0x14U, 0xA100U);
}

std::vector<u8> make_database() {
  std::vector<u8> bytes(0xC0U, 0U);
  write_u32(bytes, 0x04U, 0x80U);
  write_u32(bytes, 0x0CU, 0x70U);
  write_u32(bytes, 0x10U, 0x60U);
  write_u32(bytes, 0x54U, 0xB0U);

  constexpr LegacyWorldLoadRequest initial{
      5U, 11U, 12U, 13U, 14U, 15U, 7U, 0U,
  };
  write_u16(bytes, 0x60U, initial.logical_map_id);
  write_u16(bytes, 0x62U, initial.tile_x);
  write_u16(bytes, 0x64U, initial.tile_y);
  write_u16(bytes, 0x66U, initial.action_id);
  write_u16(bytes, 0x68U, initial.base_variant);
  write_u16(bytes, 0x6AU, initial.variant_delta);
  write_u16(bytes, 0x6CU, initial.selected_guid);

  write_map_descriptor(bytes, 0x70U, 5U, 9U);
  write_u16(bytes, 0x7EU, 0xFFFFU);
  write_role_source(bytes, 0x80U, 1U, 10000U);
  write_role_source(bytes, 0x96U, 2U, 7U);
  write_u16(bytes, 0xACU, 0xFFFFU);
  write_u16(bytes, 0xB0U, 7U);
  write_u16(bytes, 0xB2U, 0x2468U);
  write_u16(bytes, 0xB4U, 0xACE0U);
  write_u16(bytes, 0xB6U, 0U);
  return bytes;
}

u16 read_u16(const std::span<const u8> bytes, const std::size_t offset) {
  return static_cast<u16>(bytes[offset]) |
         static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

void test_decode_and_lookup(openswd3::test::Context &test) {
  const std::vector<u8> bytes = make_database();
  const auto result = decode_legacy_maps_world_database(bytes);
  test.expect_equal(result.status, LegacyMapsWorldDatabaseStatus::ready,
                    "all four MAPS world directories reach their terminators");
  test.expect_true(
      result.database.header.role_directory_offset == 0x80U &&
          result.database.header.map_descriptor_directory_offset == 0x70U &&
          result.database.header.initial_load_offset == 0x60U &&
          result.database.header.role_defaults_directory_offset == 0xB0U,
      "payload-relative pointers come from header +04/+0c/+10/+54");
  const auto &initial = result.database.initial_load;
  test.expect_true(
      initial.logical_map_id == 5U && initial.tile_x == 11U &&
          initial.tile_y == 12U && initial.action_id == 13U &&
          initial.base_variant == 14U && initial.variant_delta == 15U &&
          initial.selected_guid == 7U && initial.load_flags == 0U,
      "the seven initial words preserve the sub_40F160 push order");

  const auto *descriptor = find_legacy_maps_map_descriptor(result.database, 5U);
  test.expect_true(
      descriptor != nullptr && descriptor->archive_map_id == 9U &&
          descriptor->field_04 == 0x1234U && descriptor->field_06 == 0x5678U &&
          descriptor->field_08 == 0x9ABCU && descriptor->field_0a == 0xDEF0U &&
          descriptor->field_0c == 0x1357U,
      "the selected 14-byte descriptor retains all source words");
  test.expect_true(
      find_legacy_maps_map_descriptor(result.database, 6U) == nullptr,
      "an absent logical map is not confused with its archive map id");

  const auto *defaults = find_legacy_maps_role_defaults(result.database, 7U);
  test.expect_true(defaults != nullptr && defaults->field_2c == 0x2468U &&
                       defaults->repeated_field_30_word == 0xACE0U,
                   "the six-byte sub_40D060 directory is decoded by GUID");
}

void test_apply_load_mutates_owned_payload(openswd3::test::Context &test) {
  std::vector<u8> bytes = make_database();
  auto decoded = decode_legacy_maps_world_database(bytes);
  const LegacyWorldLoadRequest request{
      21U, 31U, 32U, 41U, 42U, 43U, 7U, 1U,
  };
  const auto applied =
      apply_legacy_maps_world_load(bytes, decoded.database, request);
  test.expect_true(
      applied.status == LegacyMapsWorldLoadApplyStatus::ready &&
          applied.selected_source_index == 1U &&
          applied.reserved_records_moved == 1U,
      "selected and reserved source records follow 0x0040C914..0x0040C9A7");

  const auto &reserved = decoded.database.role_sources[0];
  const auto &selected = decoded.database.role_sources[1];
  test.expect_true(
      reserved.logical_map_id == 21U && reserved.action_id == 2U &&
          selected.logical_map_id == 21U && selected.guid == 7U &&
          selected.action_id == 41U && selected.base_variant == 42U &&
          selected.variant_delta == 43U && selected.tile_x == 31U &&
          selected.tile_y == 32U && selected.talk_script_id == 0U &&
          selected.path_data_id == 0U && selected.path_word_index == 0 &&
          selected.flags == 0xD100U,
      "selected role receives the exact load arguments and reset fields");
  test.expect_true(
      read_u16(bytes, 0x80U) == 21U && read_u16(bytes, 0x96U) == 21U &&
          read_u16(bytes, 0x9AU) == 41U && read_u16(bytes, 0xA8U) == 0U &&
          read_u16(bytes, 0xAAU) == 0xD100U,
      "the mutable payload remains authoritative after load preparation");
}

void test_checked_boundaries(openswd3::test::Context &test) {
  const std::vector<u8> short_header(0x57U, 0U);
  test.expect_equal(decode_legacy_maps_world_database(short_header).status,
                    LegacyMapsWorldDatabaseStatus::payload_header_truncated,
                    "header must expose the +54 pointer");

  std::vector<u8> bytes = make_database();
  write_u32(bytes, 0x10U, static_cast<u32>(bytes.size() - 13U));
  test.expect_equal(
      decode_legacy_maps_world_database(bytes).status,
      LegacyMapsWorldDatabaseStatus::initial_load_record_out_of_range,
      "partial initial records stop at the modern ownership boundary");

  bytes = make_database();
  write_u16(bytes, 0x7EU, 6U);
  bytes.resize(0x85U);
  test.expect_equal(
      decode_legacy_maps_world_database(bytes).status,
      LegacyMapsWorldDatabaseStatus::map_descriptor_record_truncated,
      "a nonterminating partial map record is rejected");

  bytes = make_database();
  auto decoded = decode_legacy_maps_world_database(bytes);
  const auto missing = apply_legacy_maps_world_load(
      bytes, decoded.database,
      LegacyWorldLoadRequest{1U, 0U, 0U, 0U, 0U, 0U, 99U, 0U});
  test.expect_equal(
      missing.status, LegacyMapsWorldLoadApplyStatus::selected_guid_not_found,
      "a missing selected GUID is surfaced before creating a world session");
}

void test_real_maps_dat(openswd3::test::Context &test,
                        const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  const bool opened = input.is_open();
  std::vector<u8> file_bytes{
      std::istreambuf_iterator<char>{input},
      std::istreambuf_iterator<char>{},
  };
  test.expect_true(opened && file_bytes.size() > 0x200U,
                   "current MAPS.DAT exposes its post-prefix world database");
  if (!opened || file_bytes.size() <= 0x200U) {
    return;
  }

  std::vector<u8> payload(file_bytes.begin() + 0x200, file_bytes.end());
  auto decoded = decode_legacy_maps_world_database(payload);
  test.expect_true(
      decoded.status == LegacyMapsWorldDatabaseStatus::ready &&
          decoded.database.map_descriptors.size() == 345U &&
          decoded.database.role_sources.size() == 1371U,
      "current DVD map and role directories are structurally complete");
  if (decoded.status != LegacyMapsWorldDatabaseStatus::ready) {
    return;
  }

  const auto &initial = decoded.database.initial_load;
  const auto *descriptor =
      find_legacy_maps_map_descriptor(decoded.database, 81U);
  test.expect_true(
      initial.logical_map_id == 81U && initial.tile_x == 13U &&
          initial.tile_y == 28U && initial.action_id == 1U &&
          initial.base_variant == 0U && initial.variant_delta == 3U &&
          initial.selected_guid == 1U && descriptor != nullptr &&
          descriptor->archive_map_id == 81U && descriptor->field_04 == 16U &&
          descriptor->field_06 == 4U && descriptor->field_08 == 8U &&
          descriptor->field_0a == 0U && descriptor->field_0c == 10U,
      "current DVD new game selects logical/archive map 81 at tile 13,28");

  const std::size_t before = static_cast<std::size_t>(std::count_if(
      decoded.database.role_sources.begin(),
      decoded.database.role_sources.end(),
      [](const auto &role) { return role.logical_map_id == 81U; }));
  const auto applied = apply_legacy_maps_world_load(
      payload, decoded.database, decoded.database.initial_load);
  const std::size_t after = static_cast<std::size_t>(std::count_if(
      decoded.database.role_sources.begin(),
      decoded.database.role_sources.end(),
      [](const auto &role) { return role.logical_map_id == 81U; }));
  test.expect_true(
      applied.status == LegacyMapsWorldLoadApplyStatus::ready && before == 9U &&
          after == 12U && applied.reserved_records_moved == 2U,
      "new game moves GUID 1/10000/10001 onto the nine existing map-81 roles");
}

} // namespace

int main(const int argc, char **argv) {
  openswd3::test::Context test;
  test_decode_and_lookup(test);
  test_apply_load_mutates_owned_payload(test);
  test_checked_boundaries(test);
  if (argc == 2) {
    test_real_maps_dat(test, argv[1]);
  }
  return test.exit_code();
}
