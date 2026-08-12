#include "test.hpp"

#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_path_script.hpp"

#include <array>
#include <bit>
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
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::kLegacyWorldActiveObjectSlotCount;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldPathNodePool;
using openswd3::world_map::LegacyWorldPathScriptStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::run_legacy_world_path_script;
using openswd3::world_map::run_legacy_world_path_scripts;

constexpr u32 kMapWidth = 20U;
constexpr u32 kMapHeight = 20U;

void write_u16(const std::span<u8> bytes, const std::size_t offset,
               const u16 value) noexcept {
  bytes[offset] = static_cast<u8>(value);
  bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(const std::span<u8> bytes, const std::size_t offset,
               const u32 value) noexcept {
  bytes[offset] = static_cast<u8>(value);
  bytes[offset + 1U] = static_cast<u8>(value >> 8U);
  bytes[offset + 2U] = static_cast<u8>(value >> 16U);
  bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u16 read_slot_u16(const LegacyWorldObjectSlot &slot,
                                const std::size_t offset) noexcept {
  return static_cast<u16>(slot.bytes[offset]) |
         static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

struct Fixture {
  std::vector<u8> path_database = std::vector<u8>(0x280U, 0U);
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  std::vector<u8> surface =
      std::vector<u8>(kMapWidth * kMapHeight * sizeof(u32), 0U);
  std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
  LegacyWorldPathNodePool node_pool;

  Fixture() {
    write_u32(path_database, 0x204U, 0x20U);
    auto &role = roles[1];
    role.world_x = 4U << 4U;
    role.world_y = 5U << 4U;
    role.map_cell_pointer_32 = 5U * kMapWidth + 4U;
    role.flags = 0x00008000U;
    role.path_data_id = 1U;
    role.action.field_2c = 1U;
    role.action.field_30 = 1U;
  }

  void set_script(const std::span<const u16> words) noexcept {
    for (std::size_t index = 0U; index < words.size(); ++index) {
      write_u16(path_database, 0x220U + index * sizeof(u16), words[index]);
    }
  }

  [[nodiscard]] auto run() {
    return run_legacy_world_path_script(1U, path_database, roles,
                                        LegacyWorldRoleSurfaceContext{
                                            .map_width = kMapWidth,
                                            .selected_guid = 1U,
                                            .surface_grid = surface,
                                        },
                                        kMapHeight, slots, node_pool);
  }
};

void test_wait_opcode_preserves_frame_boundaries(
    openswd3::test::Context &test) {
  Fixture fixture;
  constexpr std::array<u16, 4U> script{4U, 2U, 5U, 0U};
  fixture.set_script(script);

  const auto first = fixture.run();
  test.expect_true(first.status == LegacyWorldPathScriptStatus::completed &&
                       first.opcodes_dispatched == 2U &&
                       first.waits_set == 1U && first.waits_decremented == 1U &&
                       first.cursor_words_advanced == 2U &&
                       fixture.roles[1].path_word_index == 2U &&
                       fixture.roles[1].path_wait_remaining == 1U,
                   "opcode 4 falls through to opcode 5, which consumes one "
                   "wait frame");

  const auto second = fixture.run();
  test.expect_true(second.opcodes_dispatched == 1U &&
                       second.waits_decremented == 1U &&
                       fixture.roles[1].path_word_index == 2U &&
                       fixture.roles[1].path_wait_remaining == 0U,
                   "a positive opcode-5 wait decrements without advancing");

  const auto third = fixture.run();
  test.expect_true(third.opcodes_dispatched == 1U &&
                       third.cursor_words_advanced == 1U &&
                       fixture.roles[1].path_word_index == 3U,
                   "a zero opcode-5 wait advances one word and still yields");

  const auto fourth = fixture.run();
  test.expect_true(fourth.opcodes_dispatched == 1U &&
                       fixture.roles[1].path_word_index == 0U,
                   "opcode 0 resets the path cursor and returns");
}

void test_request_and_movement_handshake(openswd3::test::Context &test) {
  Fixture fixture;
  constexpr std::array<u16, 5U> script{7U, 5U, 5U, 8U, 0U};
  fixture.set_script(script);

  const auto request = fixture.run();
  const auto &slot = fixture.slots[0];
  test.expect_true(
      request.status == LegacyWorldPathScriptStatus::completed &&
          request.opcodes_dispatched == 2U && request.path_requests == 1U &&
          request.movement_slots_advanced == 1U &&
          fixture.roles[1].path_word_index == 3U &&
          read_slot_u16(slot, 0x00U) == 1U &&
          read_slot_u16(slot, 0x02U) == 0U &&
          read_slot_u16(slot, 0x16U) == 4U && read_slot_u16(slot, 0x18U) == 0U,
      "opcode 7 immediately falls through to opcode 8 and arms four-pixel "
      "movement");

  fixture.roles[1].world_x = 5U << 4U;
  write_u16(fixture.slots[0].bytes, 0x02U, 0x8001U);
  const auto arrival = fixture.run();
  test.expect_true(arrival.status == LegacyWorldPathScriptStatus::completed &&
                       arrival.movement_slots_completed == 1U &&
                       arrival.cursor_words_advanced == 1U &&
                       arrival.opcodes_dispatched == 2U &&
                       fixture.roles[1].path_word_index == 0U &&
                       read_slot_u16(fixture.slots[0], 0x00U) == 0xFFFFU,
                   "terminal direction clears the object slot, advances "
                   "opcode 8, and reaches opcode 0 in the same call");
}

void test_movement_gates_and_checked_boundaries(openswd3::test::Context &test) {
  {
    Fixture fixture;
    constexpr std::array<u16, 2U> script{8U, 0U};
    fixture.set_script(script);
    auto &slot = fixture.slots[0];
    write_u16(slot.bytes, 0x00U, 1U);
    write_u16(slot.bytes, 0x02U, 0U);
    slot.bytes[0x1AU] = 3U;
    slot.bytes[0x1BU] = 1U;
    slot.bytes[0x1CU] = 7U;
    fixture.roles[1].interaction_gate = 1U;

    const auto result = fixture.run();
    test.expect_true(result.status == LegacyWorldPathScriptStatus::completed &&
                         result.cursor_words_advanced == 0U &&
                         read_slot_u16(slot, 0x02U) == 0x8000U &&
                         slot.bytes[0x1AU] == 3U,
                     "interaction gate only sets the path cursor frame bit");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 2U> script{8U, 0U};
    fixture.set_script(script);
    auto &slot = fixture.slots[0];
    write_u16(slot.bytes, 0x00U, 1U);
    write_u16(slot.bytes, 0x02U, 0U);
    slot.bytes[0x1AU] = 8U;
    slot.bytes[0x1BU] = 1U;
    slot.bytes[0x1CU] = 7U;
    for (std::size_t offset = 0U; offset < fixture.surface.size();
         offset += sizeof(u32)) {
      write_u32(fixture.surface, offset, 0x60000000U);
    }

    const auto result = fixture.run();
    test.expect_true(result.status == LegacyWorldPathScriptStatus::completed &&
                         result.movement_slots_advanced == 1U &&
                         read_slot_u16(slot, 0x02U) == 0x8000U &&
                         slot.bytes[0x1AU] == 9U &&
                         read_slot_u16(slot, 0x16U) == 0U &&
                         read_slot_u16(slot, 0x18U) == 0U,
                     "blocked opcode-8 movement preserves the exact stall and "
                     "cursor updates");
  }

  {
    Fixture fixture;
    fixture.roles[1].path_data_id = 0xFFFFU;
    test.expect_equal(
        fixture.run().status,
        LegacyWorldPathScriptStatus::path_directory_entry_out_of_range,
        "invalid PATH directory access is an explicit modern boundary");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 1U> script{36U};
    fixture.set_script(script);
    test.expect_equal(fixture.run().status,
                      LegacyWorldPathScriptStatus::unsupported_opcode,
                      "an unrestored opcode stops without guessed effects");
  }
}

void test_sub_405430_scan_gates(openswd3::test::Context &test) {
  Fixture fixture;
  constexpr std::array<u16, 2U> script{5U, 0U};
  fixture.set_script(script);
  std::array<LegacyWorldRoleRecord, 6U> roles{};
  for (std::size_t index = 1U; index < roles.size(); ++index) {
    roles[index] = fixture.roles[1];
  }
  roles[2].flags |= 0x80U;
  roles[3].flags |= 0x80000000U;
  roles[4].interaction_gate = 1U;
  roles[5].path_data_id = 0U;

  const auto result = run_legacy_world_path_scripts(
      fixture.path_database, roles,
      LegacyWorldRoleSurfaceContext{
          .map_width = kMapWidth,
          .selected_guid = 1U,
          .surface_grid = fixture.surface,
      },
      kMapHeight, fixture.slots, fixture.node_pool);
  test.expect_true(
      result.status == LegacyWorldPathScriptStatus::completed &&
          result.roles_scanned == 5U && result.eligible_roles == 1U &&
          result.scripts_completed == 1U && roles[1].path_word_index == 1U &&
          roles[2].path_word_index == 0U && roles[3].path_word_index == 0U &&
          roles[4].path_word_index == 0U && roles[5].path_word_index == 0U,
      "the sub_405430 ordinary-role gates precede PATH dispatch");
}

void test_current_path_entries(openswd3::test::Context &test,
                               const std::filesystem::path &maps_path,
                               const std::filesystem::path &path_path) {
  std::ifstream maps_input{maps_path, std::ios::binary};
  std::ifstream path_input{path_path, std::ios::binary};
  const std::vector<u8> maps_file{std::istreambuf_iterator<char>{maps_input},
                                  std::istreambuf_iterator<char>{}};
  const std::vector<u8> path{std::istreambuf_iterator<char>{path_input},
                             std::istreambuf_iterator<char>{}};
  test.expect_true(maps_input.is_open() && maps_file.size() > 0x200U &&
                       path_input.is_open() && path.size() > 0x200U,
                   "current MAPS and PATH databases are readable");
  if (!maps_input.is_open() || maps_file.size() <= 0x200U ||
      !path_input.is_open() || path.size() <= 0x200U) {
    return;
  }

  std::vector<u8> payload{maps_file.begin() + 0x200, maps_file.end()};
  const auto decoded = decode_legacy_maps_world_database(payload);
  test.expect_equal(decoded.status, LegacyMapsWorldDatabaseStatus::ready,
                    "current MAPS database decodes");
  if (decoded.status != LegacyMapsWorldDatabaseStatus::ready) {
    return;
  }

  u32 verified = 0U;
  for (const auto &source : decoded.database.role_sources) {
    if (source.path_data_id == 0U) {
      continue;
    }
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].path_data_id = source.path_data_id;
    roles[1].path_word_index = static_cast<u32>(
        static_cast<int>(static_cast<i16>(source.path_word_index)));
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    std::vector<u8> surface(sizeof(u32), 0U);
    LegacyWorldPathNodePool node_pool;
    const auto result = run_legacy_world_path_script(
        1U, path, roles,
        LegacyWorldRoleSurfaceContext{
            .map_width = 1U, .selected_guid = 0U, .surface_grid = surface},
        1U, slots, node_pool);
    test.expect_true(result.status == LegacyWorldPathScriptStatus::completed &&
                         result.last_opcode == 5U &&
                         result.opcodes_dispatched == 1U &&
                         roles[1].path_word_index ==
                             static_cast<u32>(source.path_word_index) + 1U,
                     "every current nonzero initial PATH entry executes the "
                     "restored opcode-5 cursor rule");
    ++verified;
  }
  test.expect_equal(verified, u32{136U},
                    "all 136 current nonzero initial PATH entries verified");
}

} // namespace

int main(const int argc, char **argv) {
  openswd3::test::Context test;
  test_wait_opcode_preserves_frame_boundaries(test);
  test_request_and_movement_handshake(test);
  test_movement_gates_and_checked_boundaries(test);
  test_sub_405430_scan_gates(test);
  if (argc == 3) {
    test_current_path_entries(test, argv[1], argv[2]);
  }
  return test.exit_code();
}
