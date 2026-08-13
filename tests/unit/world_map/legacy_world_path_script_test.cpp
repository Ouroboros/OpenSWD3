#include "test.hpp"

#include "openswd3/input_time_rng/legacy_crt_rng.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_path_script.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <unordered_set>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::input_time_rng::LegacyCrtRng;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::world_map::decode_legacy_maps_world_database;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldActiveObjectSlotCount;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyMapsWorldDatabaseStatus;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldPathNodePool;
using openswd3::world_map::LegacyWorldRolePathRequestStatus;
using openswd3::world_map::LegacyWorldPathScriptPorts;
using openswd3::world_map::LegacyWorldPathScriptRuntime;
using openswd3::world_map::LegacyWorldPathScriptState;
using openswd3::world_map::LegacyWorldPathScriptStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::LegacyWorldStoryVmState;
using openswd3::world_map::run_legacy_world_path_script;
using openswd3::world_map::run_legacy_world_path_scripts;
using openswd3::world_map::advance_legacy_world_script_clock;
using openswd3::world_map::query_legacy_world_story_flag;
using openswd3::world_map::set_legacy_world_story_flag;

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

[[nodiscard]] u16 read_u16(const std::span<const u8> bytes,
                           const std::size_t offset) noexcept {
  return static_cast<u16>(bytes[offset]) |
         static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_u32(const std::span<const u8> bytes,
                           const std::size_t offset) noexcept {
  return static_cast<u32>(bytes[offset]) |
         (static_cast<u32>(bytes[offset + 1U]) << 8U) |
         (static_cast<u32>(bytes[offset + 2U]) << 16U) |
         (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] u16 read_slot_u16(const LegacyWorldObjectSlot &slot,
                                const std::size_t offset) noexcept {
  return static_cast<u16>(slot.bytes[offset]) |
         static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

[[nodiscard]] u32 read_surface_u32(const std::span<const u8> bytes,
                                   const u32 cell) noexcept {
  const std::size_t offset = static_cast<std::size_t>(cell) * sizeof(u32);
  return static_cast<u32>(bytes[offset]) |
         (static_cast<u32>(bytes[offset + 1U]) << 8U) |
         (static_cast<u32>(bytes[offset + 2U]) << 16U) |
         (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

struct FakePathPorts final : LegacyWorldPathScriptPorts {
  [[nodiscard]] u32 update_action(
      openswd3::asset_runtime::LegacyActionRecord &) override {
    ++action_updates;
    return action_result;
  }

  void play_positional_sample(const u16 sound_id, const openswd3::compat::i32 x,
                              const openswd3::compat::i32 y) noexcept override {
    ++sample_requests;
    last_sound_id = sound_id;
    last_sound_x = x;
    last_sound_y = y;
  }

  u32 action_result{1U};
  u32 action_updates{};
  u32 sample_requests{};
  u16 last_sound_id{};
  openswd3::compat::i32 last_sound_x{};
  openswd3::compat::i32 last_sound_y{};
};

struct Fixture {
  std::vector<u8> path_database = std::vector<u8>(0x280U, 0U);
  std::array<LegacyWorldRoleRecord, 3U> roles{};
  std::vector<u8> surface =
      std::vector<u8>(kMapWidth * kMapHeight * sizeof(u32), 0U);
  std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
  LegacyWorldPathNodePool node_pool;
  LegacyWorldPathScriptState path_state;
  LegacyWorldStoryVmState shared_state;
  LegacyRoleSpatialIndex spatial_index;
  LegacyCrtRng crt_rng;
  LegacySecondaryRng secondary_rng;
  FakePathPorts ports;

  Fixture() {
    write_u32(path_database, 0x204U, 0x20U);
    auto &role = roles[1];
    role.guid = 1U;
    role.world_x = 4U << 4U;
    role.world_y = 5U << 4U;
    role.map_cell_pointer_32 = 5U * kMapWidth + 4U;
    role.flags = 0x00008000U;
    role.path_data_id = 1U;
    role.action.field_2c = 1U;
    role.action.field_30 = 1U;

    auto &target = roles[2];
    target.guid = 2U;
    target.world_x = 8U << 4U;
    target.world_y = 5U << 4U;
    target.map_cell_pointer_32 = 5U * kMapWidth + 8U;
    target.action.field_2c = 1U;
    target.action.field_30 = 1U;

    spatial_index.map_height = kMapHeight;
    const std::size_t row_count =
        static_cast<std::size_t>(kMapHeight) + 2U * kLegacySpatialRowPadding;
    for (auto &rows : spatial_index.row_heads) {
      rows.assign(row_count, kLegacySpatialNoRole);
    }
    static_cast<void>(insert_legacy_role_spatially(spatial_index, roles, 1U));
    static_cast<void>(insert_legacy_role_spatially(spatial_index, roles, 2U));
    crt_rng.seed(1U);
    secondary_rng.seed(1U);
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
                                        kMapHeight, slots, node_pool,
                                        path_state,
                                        LegacyWorldPathScriptRuntime{
                                            .shared_script_state =
                                                &shared_state,
                                            .spatial_index = &spatial_index,
                                            .crt_rng = &crt_rng,
                                            .secondary_rng = &secondary_rng,
                                            .controlled_role_index = 1U,
                                        },
                                        ports);
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
    constexpr std::array<u16, 1U> script{37U};
    fixture.set_script(script);
    test.expect_equal(fixture.run().status,
                      LegacyWorldPathScriptStatus::unsupported_opcode,
                      "an out-of-table opcode stops without guessed effects");
  }
}

void test_action_wait_and_audio_handlers(openswd3::test::Context &test) {
  Fixture fixture;

  constexpr std::array<u16, 2U> base_variant{2U, 9U};
  fixture.set_script(base_variant);
  const auto base_result = fixture.run();
  test.expect_true(base_result.opcodes_dispatched == 1U &&
                       base_result.action_updates == 1U &&
                       fixture.roles[1].action.base_variant == 9U &&
                       fixture.roles[1].path_word_index == 2U,
                   "opcode 2 stores the base variant before one action update");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 2U> direction{3U, 7U};
  fixture.set_script(direction);
  const auto direction_result = fixture.run();
  test.expect_true(direction_result.opcodes_dispatched == 1U &&
                       direction_result.action_updates == 1U &&
                       fixture.roles[1].action.variant_delta == 7U &&
                       fixture.roles[1].path_word_index == 2U,
                   "opcode 3 stores the direction field before yielding");

  fixture.roles[1].path_word_index = 0U;
  fixture.roles[1].path_wait_remaining = 2U;
  constexpr std::array<u16, 1U> active_wait{6U};
  fixture.set_script(active_wait);
  const auto wait_result = fixture.run();
  test.expect_true(wait_result.waits_decremented == 1U &&
                       wait_result.action_updates == 1U &&
                       fixture.roles[1].path_wait_remaining == 1U &&
                       fixture.roles[1].path_word_index == 0U,
                   "opcode 6 decrements a positive wait and still updates action");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 4U> action_data{31U, 12U, 0xFFFFU, 5U};
  fixture.set_script(action_data);
  const auto action_result = fixture.run();
  test.expect_true(action_result.action_updates == 1U &&
                       fixture.roles[1].action.action_id == 12U &&
                       fixture.roles[1].action.base_variant == 9U &&
                       fixture.roles[1].action.variant_delta == 5U &&
                       fixture.roles[1].path_word_index == 4U,
                   "opcode 31 applies only non-FFFF action fields");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 2U> sample{32U, 77U};
  fixture.set_script(sample);
  const auto sample_result = fixture.run();
  test.expect_true(sample_result.opcodes_dispatched == 1U &&
                       fixture.ports.sample_requests == 1U &&
                       fixture.ports.last_sound_id == 77U &&
                       fixture.ports.last_sound_x == 64 &&
                       fixture.ports.last_sound_y == 80 &&
                       fixture.roles[1].path_word_index == 2U,
                   "opcode 32 yields after one positional sample request");
}

void test_clock_flag_and_transfer_handlers(openswd3::test::Context &test) {
  {
    Fixture fixture;
    fixture.roles[1].flags |= 0x00001000U;
    fixture.shared_state.script_clock = 9U;
    constexpr std::array<u16, 3U> script{9U, 10U, 5U};
    fixture.set_script(script);
    const auto waiting = fixture.run();
    test.expect_true(waiting.opcodes_dispatched == 1U &&
                         waiting.action_updates == 1U &&
                         fixture.roles[1].path_word_index == 0U,
                     "opcode 9 yields without advancing below its threshold");

    fixture.shared_state.script_clock = 10U;
    fixture.roles[1].flags &= ~0x00001000U;
    const auto elapsed = fixture.run();
    test.expect_true(elapsed.opcodes_dispatched == 2U &&
                         elapsed.action_updates == 0U &&
                         fixture.roles[1].path_word_index == 3U,
                     "opcode 9 advances and continues on threshold equality");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 2U> script{11U, 1U};
    fixture.set_script(script);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 2U &&
                         fixture.shared_state.script_clock == 1U &&
                         fixture.roles[1].path_data_id == 0U &&
                         fixture.roles[1].path_word_index == 0U,
                     "opcode 11 advances one word and reinterprets its operand");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 2U> script{11U, 1001U};
    fixture.set_script(script);
    const auto result = fixture.run();
    test.expect_true(result.status ==
                             LegacyWorldPathScriptStatus::unsupported_opcode &&
                         fixture.shared_state.script_clock == 0U &&
                         fixture.roles[1].path_word_index == 1U,
                     "opcode 11 clamps above 1000 before one-word fallthrough");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 10U> script{
        10U, 5U, 0x30U, 0U, 5U, 0U, 0U, 0U, 5U, 0U};
    fixture.set_script(script);
    fixture.shared_state.script_clock = 5U;
    const auto taken = fixture.run();
    test.expect_true(taken.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 9U,
                     "opcode 10 takes the unsigned equality edge");

    fixture.roles[1].path_word_index = 0U;
    fixture.shared_state.script_clock = 4U;
    const auto skipped = fixture.run();
    test.expect_true(skipped.conditional_transfers == 0U &&
                         fixture.roles[1].path_word_index == 5U,
                     "opcode 10 advances four words below the threshold");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 7U> script{12U, 0x2CU, 0U, 0U, 0U, 0U, 5U};
    fixture.set_script(script);
    const auto result = fixture.run();
    test.expect_true(result.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 7U,
                     "opcode 12 transfers to an absolute PATH payload offset");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 3U> script{12U, 0x10U, 0U};
    fixture.set_script(script);
    write_u16(fixture.path_database, 0x210U, 5U);
    const auto result = fixture.run();
    test.expect_true(
        result.conditional_transfers == 1U &&
            result.opcodes_dispatched == 2U &&
            fixture.roles[1].path_word_index == 0xFFFFFFF9U,
        "a transfer before the current PATH directory entry preserves the "
        "original signed 32-bit word cursor");
  }

  {
    Fixture fixture;
    set_legacy_world_story_flag(fixture.shared_state, 9U);
    constexpr std::array<u16, 9U> script{
        13U, 9U, 0x30U, 0U, 5U, 0U, 0U, 0U, 5U};
    fixture.set_script(script);
    const auto result = fixture.run();
    test.expect_true(result.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 9U,
                     "opcode 13 jumps when the selected flag is set");

    fixture.roles[1].path_word_index = 0U;
    constexpr std::array<u16, 9U> inverted{
        14U, 10U, 0x30U, 0U, 5U, 0U, 0U, 0U, 5U};
    fixture.set_script(inverted);
    const auto inverted_result = fixture.run();
    test.expect_true(inverted_result.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 9U,
                     "opcode 14 jumps when the selected flag is clear");
  }

  {
    Fixture fixture;
    set_legacy_world_story_flag(fixture.shared_state, 3U);
    set_legacy_world_story_flag(fixture.shared_state, 4U);
    constexpr std::array<u16, 9U> all_flags{
        15U, 3U, 4U, 0xFFFFU, 0x30U, 0U, 5U, 0U, 5U};
    fixture.set_script(all_flags);
    const auto result = fixture.run();
    test.expect_true(result.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 9U,
                     "opcode 15 jumps when every listed flag is set");

    fixture.roles[1].path_word_index = 0U;
    constexpr std::array<u16, 5U> empty_any{
        16U, 0xFFFFU, 0x28U, 0U, 5U};
    fixture.set_script(empty_any);
    const auto empty_result = fixture.run();
    test.expect_true(empty_result.conditional_transfers == 0U &&
                         fixture.roles[1].path_word_index == 5U,
                     "opcode 16 treats an empty list as no flag set");

    fixture.roles[1].path_word_index = 0U;
    set_legacy_world_story_flag(fixture.shared_state, 4U);
    constexpr std::array<u16, 9U> any_flags{
        16U, 3U, 4U, 0xFFFFU, 0x30U, 0U, 5U, 0U, 5U};
    fixture.set_script(any_flags);
    const auto any_result = fixture.run();
    test.expect_true(any_result.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 9U,
                     "opcode 16 jumps when any listed flag is set");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 5U> script{17U, 12U, 18U, 12U, 5U};
    fixture.set_script(script);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 3U &&
                         !query_legacy_world_story_flag(
                             fixture.shared_state, 12U) &&
                         fixture.roles[1].path_word_index == 5U,
                     "opcodes 17 and 18 set then clear one story flag");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 7U> script{
        25U, 3U, 0x2CU, 0U, 5U, 0U, 5U};
    fixture.set_script(script);
    fixture.shared_state.script_clock_origin = 7U;
    fixture.shared_state.script_clock = 10U;
    const auto equal = fixture.run();
    test.expect_true(equal.conditional_transfers == 0U &&
                         fixture.roles[1].path_word_index == 5U,
                     "opcode 25 does not jump on origin-plus-delta equality");

    fixture.roles[1].path_word_index = 0U;
    fixture.shared_state.script_clock = 11U;
    const auto greater = fixture.run();
    test.expect_true(greater.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 7U,
                     "opcode 25 jumps only when clock is strictly greater");

    fixture.roles[1].path_word_index = 0U;
    fixture.shared_state.script_clock = 37U;
    constexpr std::array<u16, 2U> save_origin{26U, 5U};
    fixture.set_script(save_origin);
    const auto saved = fixture.run();
    test.expect_true(saved.opcodes_dispatched == 2U &&
                         fixture.shared_state.script_clock_origin == 37U &&
                         fixture.roles[1].path_word_index == 2U,
                     "opcode 26 copies the current clock then continues");

    fixture.roles[1].path_word_index = 0U;
    fixture.shared_state.script_clock_origin = 0xFFFFFFFEU;
    fixture.shared_state.script_clock = 2U;
    constexpr std::array<u16, 7U> wrapped{
        25U, 3U, 0x2CU, 0U, 5U, 0U, 5U};
    fixture.set_script(wrapped);
    const auto wrapped_result = fixture.run();
    test.expect_true(wrapped_result.conditional_transfers == 1U &&
                         fixture.roles[1].path_word_index == 7U,
                     "opcode 25 compares after a wrapping 32-bit addition");
  }
}

void test_variable_handlers(openswd3::test::Context &test) {
  Fixture fixture;

  constexpr std::array<u16, 4U> assign{19U, 2U, 1001U, 5U};
  fixture.set_script(assign);
  static_cast<void>(fixture.run());
  test.expect_true(fixture.shared_state.script_variables[2] == 1001U &&
                       fixture.roles[1].path_word_index == 4U,
                   "opcode 19 preserves an above-1000 assignment value");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 4U> add{20U, 2U, 50U, 5U};
  fixture.set_script(add);
  static_cast<void>(fixture.run());
  test.expect_equal(fixture.shared_state.script_variables[2], u32{1000U},
                    "opcode 20 clamps an addition above 1000");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 4U> subtract{21U, 2U, 1001U, 5U};
  fixture.set_script(subtract);
  static_cast<void>(fixture.run());
  test.expect_equal(fixture.shared_state.script_variables[2], u32{0U},
                    "opcode 21 clamps a signed-negative wrapped result to zero");

  fixture.roles[1].path_word_index = 0U;
  fixture.shared_state.script_variables[2] = 500U;
  constexpr std::array<u16, 7U> greater_equal{
      22U, 2U, 500U, 0x2CU, 0U, 5U, 5U};
  fixture.set_script(greater_equal);
  const auto ge_result = fixture.run();
  test.expect_true(ge_result.conditional_transfers == 1U &&
                       fixture.roles[1].path_word_index == 7U,
                   "opcode 22 takes the variable equality edge");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 7U> less_equal{
      23U, 2U, 499U, 0x2CU, 0U, 5U, 5U};
  fixture.set_script(less_equal);
  const auto le_result = fixture.run();
  test.expect_true(le_result.conditional_transfers == 0U &&
                       fixture.roles[1].path_word_index == 6U,
                   "opcode 23 advances five words when variable is greater");

  fixture.roles[1].path_word_index = 0U;
  constexpr std::array<u16, 3U> invalid{19U, 64U, 1U};
  fixture.set_script(invalid);
  const auto invalid_result = fixture.run();
  test.expect_true(invalid_result.invalid_variable_indices == 1U &&
                       fixture.roles[1].path_word_index == 0U,
                   "an invalid variable index returns without advancing");
}

void test_role_mutation_and_random_walk_handlers(
    openswd3::test::Context &test) {
  {
    Fixture fixture;
    auto &target = fixture.roles[2];
    target.world_x += 4U;
    const u32 target_cell = target.map_cell_pointer_32;
    write_u32(fixture.surface, target_cell * sizeof(u32), 0x10000000U);
    auto &slot = fixture.slots[0];
    write_u16(slot.bytes, 0x00U, 2U);
    write_u16(slot.bytes, 0x02U, 0U);
    slot.bytes[0x1BU] = 1U;
    slot.bytes[0x1CU] = 7U;
    constexpr std::array<u16, 4U> script{24U, 2U, 9U, 5U};
    fixture.set_script(script);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 2U &&
                         target.world_x == (8U << 4U) &&
                         target.path_data_id == 9U &&
                         target.path_word_index == 0U &&
                         fixture.roles[1].path_word_index == 4U &&
                         slot.bytes[0U] == 0xFFU &&
                         read_surface_u32(fixture.surface, target_cell) == 0U,
                     "opcode 24 aligns, unlinks, clears, and retargets a role");
  }

  {
    Fixture fixture;
    write_u32(fixture.path_database, 0x208U, 0x40U);
    write_u16(fixture.path_database, 0x240U, 5U);
    constexpr std::array<u16, 3U> switch_self{24U, 0xFFFEU, 2U};
    fixture.set_script(switch_self);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 2U &&
                         fixture.roles[1].path_data_id == 2U &&
                         fixture.roles[1].path_word_index == 1U,
                     "opcode 24 immediately continues in a self-selected script");
  }

  {
    Fixture fixture;
    auto &slot = fixture.slots[0];
    write_u16(slot.bytes, 0x00U, 2U);
    slot.bytes[0x1BU] = 2U;
    for (std::size_t offset = 0x08U; offset <= 0x0EU;
         offset += sizeof(u16)) {
      write_u16(slot.bytes, offset, 1U);
    }
    constexpr std::array<u16, 4U> retarget{24U, 2U, 9U, 5U};
    fixture.set_script(retarget);
    static_cast<void>(fixture.run());
    test.expect_true(read_slot_u16(slot, 0x08U) == 0xFFFFU &&
                         read_slot_u16(slot, 0x0AU) == 0xFFFFU &&
                         read_slot_u16(slot, 0x0CU) == 0xFFFFU &&
                         read_slot_u16(slot, 0x0EU) == 0xFFFFU &&
                         (slot.bytes[0x1BU] & 0x0FU) == 2U,
                     "opcode 24 resets only four links in a kind-2 slot");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 4U> missing_target{24U, 99U, 9U, 5U};
    fixture.set_script(missing_target);
    const auto result = fixture.run();
    test.expect_true(
        result.status == LegacyWorldPathScriptStatus::invalid_role_index &&
            fixture.roles[1].path_word_index == 0U,
        "opcode 24 isolates the original FFFFFFFF role-array underrun");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 3U> enable{28U, 2U, 5U};
    fixture.set_script(enable);
    static_cast<void>(fixture.run());
    test.expect_equal(fixture.roles[2].flags & 0x0000C000U, u32{0xC000U},
                      "opcode 28 sets both PATH role flag bits");

    fixture.roles[1].path_word_index = 0U;
    write_u32(fixture.surface,
              fixture.roles[2].map_cell_pointer_32 * sizeof(u32),
              0x30000000U);
    constexpr std::array<u16, 3U> disable{27U, 2U, 5U};
    fixture.set_script(disable);
    static_cast<void>(fixture.run());
    test.expect_true((fixture.roles[2].flags & 0x0000C000U) == 0U &&
                         read_surface_u32(
                             fixture.surface,
                             fixture.roles[2].map_cell_pointer_32) == 0U,
                     "opcode 27 clears both flags before surface occupancy");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 3U> disable_missing{27U, 99U, 5U};
    fixture.set_script(disable_missing);
    const auto disabled = fixture.run();
    test.expect_equal(
        disabled.status, LegacyWorldPathScriptStatus::invalid_role_index,
        "opcode 27 isolates the original FFFFFFFF role-array underrun");

    fixture.roles[1].path_word_index = 0U;
    constexpr std::array<u16, 3U> enable_missing{28U, 99U, 5U};
    fixture.set_script(enable_missing);
    const auto enabled = fixture.run();
    test.expect_equal(
        enabled.status, LegacyWorldPathScriptStatus::invalid_role_index,
        "opcode 28 isolates the original FFFFFFFF role-array underrun");
  }

  {
    Fixture fixture;
    const u32 old_cell = fixture.roles[1].map_cell_pointer_32;
    write_u32(fixture.surface, old_cell * sizeof(u32), 0x10000100U);
    constexpr std::array<u16, 3U> move{29U, 9U, 6U};
    fixture.set_script(move);
    const auto result = fixture.run();
    const u32 new_cell = 6U * kMapWidth + 9U;
    test.expect_true(result.action_updates == 1U &&
                         fixture.roles[1].world_x == (9U << 4U) &&
                         fixture.roles[1].world_y == (6U << 4U) &&
                         fixture.roles[1].map_cell_pointer_32 == new_cell &&
                         fixture.roles[1].path_word_index == 3U &&
                         read_surface_u32(fixture.surface, old_cell) == 0U &&
                         read_surface_u32(fixture.surface, new_cell) ==
                             0x10000100U,
                     "opcode 29 preserves clear-move-update-mark ordering");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 1U> random_walk{30U};
    fixture.set_script(random_walk);
    const auto result = fixture.run();
    const auto &slot = fixture.slots[0];
    test.expect_true(result.opcodes_dispatched == 1U &&
                         fixture.crt_rng.state() == 0x0029E2C0U &&
                         fixture.secondary_rng.index() == 2U &&
                         read_slot_u16(slot, 0x00U) == 1U &&
                         (slot.bytes[0x1BU] & 0x0FU) == 1U &&
                         slot.bytes[0x1CU] == 1U &&
                         read_slot_u16(slot, 0x16U) == 0U &&
                         read_slot_u16(slot, 0x18U) == 4U &&
                         fixture.roles[1].action.base_variant == 8U &&
                         fixture.roles[1].path_word_index == 0U,
                     "opcode 30 consumes both RNGs and arms one random walk");
  }

  {
    Fixture fixture;
    fixture.secondary_rng.seed(2U);
    constexpr std::array<u16, 1U> random_walk{30U};
    fixture.set_script(random_walk);
    static_cast<void>(fixture.run());
    const auto &slot = fixture.slots[0];
    test.expect_true(slot.bytes[0x1CU] == 4U &&
                         read_slot_u16(slot, 0x16U) == 0xFFFCU &&
                         read_slot_u16(slot, 0x18U) == 0xFFFCU &&
                         fixture.roles[1].action.base_variant == 8U,
                     "opcode 30 adopts random directions below eight");
  }

  {
    Fixture fixture;
    fixture.secondary_rng.seed(7U);
    constexpr std::array<u16, 1U> random_walk{30U};
    fixture.set_script(random_walk);
    static_cast<void>(fixture.run());
    const auto &slot = fixture.slots[0];
    test.expect_true((slot.bytes[3U] & 0x80U) != 0U &&
                         fixture.roles[1].action.base_variant == 0U &&
                         read_slot_u16(slot, 0x16U) == 0xFFFFU &&
                         read_slot_u16(slot, 0x18U) == 0xFFFFU,
                     "opcode 30 idles without rewriting step fields when the "
                     "random value exceeds sixty");
  }
}

void test_follow_and_label_handlers(openswd3::test::Context &test) {
  {
    Fixture fixture;
    constexpr std::array<u16, 3U> follow{33U, 2U, 5U};
    fixture.set_script(follow);
    const auto result = fixture.run();
    test.expect_true(result.path_requests == 1U &&
                         result.opcodes_dispatched == 2U &&
                         fixture.path_state.camera_target_x == 8U &&
                         fixture.path_state.camera_target_y == 5U &&
                         fixture.roles[1].path_word_index == 3U,
                     "opcode 33 follows an aligned role then continues");
  }

  {
    Fixture fixture;
    fixture.roles[2].world_x += 1U;
    constexpr std::array<u16, 3U> follow{33U, 2U, 5U};
    fixture.set_script(follow);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 1U &&
                         result.path_requests == 0U &&
                         fixture.roles[1].path_word_index == 0U,
                     "opcode 33 yields while its target is not cell-aligned");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 3U> missing{33U, 99U, 5U};
    fixture.set_script(missing);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 1U &&
                         result.path_requests == 0U &&
                         fixture.roles[1].path_word_index == 2U,
                     "missing opcode-33 targets stop after two words");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 5U> missing{36U, 99U, 1U, 1U, 5U};
    fixture.set_script(missing);
    const auto result = fixture.run();
    test.expect_true(result.opcodes_dispatched == 1U &&
                         result.path_requests == 0U &&
                         fixture.roles[1].path_word_index == 2U,
                     "missing opcode-36 targets advance only two words");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 2U> truncated_missing{36U, 99U};
    fixture.set_script(truncated_missing);
    fixture.path_database.resize(0x224U);
    const auto result = fixture.run();
    test.expect_true(result.status == LegacyWorldPathScriptStatus::completed &&
                         result.path_requests == 0U &&
                         fixture.roles[1].path_word_index == 2U,
                     "a missing opcode-36 target does not read its offsets");
  }

  {
    constexpr std::array<u16, 8U> expected_x{9U, 0U, 7U, 9U,
                                             10U, 6U, 8U, 8U};
    constexpr std::array<u16, 8U> expected_y{4U, 0U, 6U, 4U,
                                             5U, 5U, 3U, 7U};
    for (u32 direction = 0U; direction < 8U; ++direction) {
      if (direction == 1U) {
        continue;
      }
      Fixture fixture;
      fixture.roles[2].action.variant_delta = direction;
      constexpr std::array<u16, 5U> follow_offset{36U, 2U, 1U, 1U, 5U};
      fixture.set_script(follow_offset);
      const auto result = fixture.run();
      test.expect_true(
          result.path_requests == 1U &&
              fixture.path_state.camera_target_x == expected_x[direction] &&
              fixture.path_state.camera_target_y == expected_y[direction] &&
              fixture.roles[1].path_word_index == 5U,
          "opcode 36 applies each initialized assembly transform");
    }
  }

  {
    Fixture fixture;
    fixture.roles[2].action.variant_delta = 0U;
    const u32 blocked_cell = 4U * kMapWidth + 9U;
    write_u32(fixture.surface, blocked_cell * sizeof(u32), 0x40000000U);
    constexpr std::array<u16, 5U> follow_offset{36U, 2U, 1U, 1U, 5U};
    fixture.set_script(follow_offset);
    static_cast<void>(fixture.run());
    test.expect_true(fixture.path_state.camera_target_x == 8U &&
                         fixture.path_state.camera_target_y == 5U,
                     "opcode 36 falls back when either probed cell is blocked");
  }

  {
    Fixture fixture;
    fixture.roles[2].action.variant_delta = 6U;
    constexpr std::array<u16, 5U> follow_offset{36U, 2U, 2U, 3U, 5U};
    fixture.set_script(follow_offset);
    const auto result = fixture.run();
    test.expect_true(result.path_requests == 1U &&
                         fixture.path_state.camera_target_x == 9U &&
                         fixture.path_state.camera_target_y == 0U &&
                         fixture.roles[1].path_word_index == 5U,
                     "opcode 36 case 6 uses the assembly rotation equations");

    fixture.roles[1].path_word_index = 0U;
    fixture.roles[2].action.variant_delta = 1U;
    const auto indeterminate = fixture.run();
    test.expect_equal(
        indeterminate.status,
        LegacyWorldPathScriptStatus::indeterminate_legacy_stack_state,
        "opcode 36 case 1 exposes its first-use uninitialized stack state");
  }

  {
    Fixture fixture;
    constexpr std::array<u16, 5U> label{
        34U, 11U, 0xA440U, 0xA441U, 0x5125U};
    fixture.set_script(label);
    const auto result = fixture.run();
    const auto payload = openswd3::world_map::resolve_legacy_world_path_label(
        fixture.path_state, fixture.roles[1].path_payload_pointer_32);
    test.expect_true(result.opcodes_dispatched == 1U &&
                         fixture.roles[1].path_payload_relation == 0U &&
                         fixture.roles[1].path_payload_pointer_32 == 2U &&
                         fixture.roles[1].path_word_index == 5U &&
                         payload.size() == 5U && payload[0] == 0x40U &&
                         payload[1] == 0xA4U && payload[2] == 0x41U &&
                         payload[3] == 0xA4U && payload[4] == 0U,
                     "opcode 34 scans percent-Q bytewise and owns a zero tail");

    fixture.roles[1].path_word_index = 0U;
    constexpr std::array<u16, 2U> clear_label{35U, 5U};
    fixture.set_script(clear_label);
    static_cast<void>(fixture.run());
    test.expect_true(fixture.roles[1].path_payload_pointer_32 == 0U &&
                         fixture.roles[1].path_payload_relation == 0U &&
                         fixture.path_state.role_label_payloads[1].empty(),
                     "opcode 35 releases and clears an existing label");
  }
}

void test_ignored_path_request_failures(openswd3::test::Context &test) {
  Fixture fixture;
  constexpr std::array<u16, 4U> request{7U, 5U, 5U, 5U};
  fixture.set_script(request);
  const auto result = run_legacy_world_path_script(
      1U, fixture.path_database, fixture.roles,
      LegacyWorldRoleSurfaceContext{
          .map_width = kMapWidth,
          .selected_guid = 1U,
          .surface_grid = fixture.surface,
      },
      kMapHeight, std::span<LegacyWorldObjectSlot>{}, fixture.node_pool,
      fixture.path_state,
      LegacyWorldPathScriptRuntime{
          .shared_script_state = &fixture.shared_state,
          .spatial_index = &fixture.spatial_index,
          .crt_rng = &fixture.crt_rng,
          .secondary_rng = &fixture.secondary_rng,
          .controlled_role_index = 1U,
      },
      fixture.ports);
  test.expect_true(
      result.status == LegacyWorldPathScriptStatus::completed &&
          result.path_request_status ==
              LegacyWorldRolePathRequestStatus::insufficient_object_slots &&
          result.opcodes_dispatched == 2U &&
          fixture.roles[1].path_word_index == 4U,
      "opcode 7 records but does not branch on the path helper return");
}

void test_shared_path_clock_cadence(openswd3::test::Context &test) {
  LegacyWorldStoryVmState state;
  for (u32 frame = 0U; frame < 20U; ++frame) {
    advance_legacy_world_script_clock(state);
  }
  test.expect_true(state.script_clock_frame_counter == 20U &&
                       state.script_clock == 0U,
                   "the PATH clock does not advance during the first 20 frames");

  advance_legacy_world_script_clock(state);
  test.expect_true(state.script_clock_frame_counter == 0U &&
                       state.script_clock == 1U,
                   "the 21st frame advances and resets the PATH clock divider");

  state.script_clock_frame_counter = 20U;
  state.script_clock = 1000U;
  advance_legacy_world_script_clock(state);
  test.expect_equal(state.script_clock, u32{0U},
                    "the PATH clock wraps after 1000");
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
      kMapHeight, fixture.slots, fixture.node_pool, fixture.path_state,
      LegacyWorldPathScriptRuntime{
          .shared_script_state = &fixture.shared_state,
          .controlled_role_index = 1U,
      },
      fixture.ports);
  test.expect_true(
      result.status == LegacyWorldPathScriptStatus::completed &&
          result.roles_scanned == 5U && result.eligible_roles == 1U &&
          result.scripts_completed == 1U && roles[1].path_word_index == 1U &&
          roles[2].path_word_index == 0U && roles[3].path_word_index == 0U &&
          roles[4].path_word_index == 0U && roles[5].path_word_index == 0U,
      "the sub_405430 ordinary-role gates precede PATH dispatch");
}

void test_sub_405430_surface_restore_branches(
    openswd3::test::Context &test) {
  Fixture fixture;
  constexpr std::array<u16, 2U> script{5U, 0U};
  fixture.set_script(script);
  std::array<LegacyWorldRoleRecord, 6U> roles{};
  for (std::size_t index = 1U; index < roles.size(); ++index) {
    roles[index] = fixture.roles[1];
    roles[index].map_cell_pointer_32 = static_cast<u32>(index);
  }
  roles[1].flags |= 0x80000000U;
  roles[1].action.action_id = 1U;
  roles[2].interaction_gate = 1U;
  roles[2].action.action_id = 1U;
  roles[3].path_data_id = 0U;
  roles[3].flags |= 0x1000U;
  roles[3].action.action_id = 1U;
  roles[4].path_data_id = 0U;
  roles[4].flags |= 0x1000U;
  roles[4].action.action_id = 0U;
  roles[5].path_data_id = 0U;
  roles[5].flags |= 0x1000U;
  roles[5].action.action_id = 1U;

  const auto result = run_legacy_world_path_scripts(
      fixture.path_database, roles,
      LegacyWorldRoleSurfaceContext{
          .map_width = kMapWidth,
          .selected_guid = 0xFFFFU,
          .surface_grid = fixture.surface,
      },
      kMapHeight, fixture.slots, fixture.node_pool, fixture.path_state,
      LegacyWorldPathScriptRuntime{
          .shared_script_state = &fixture.shared_state,
          .controlled_role_index = 5U,
      },
      fixture.ports);
  test.expect_true(
      result.status == LegacyWorldPathScriptStatus::completed &&
          read_surface_u32(fixture.surface, 1U) == 0x10000000U &&
          read_surface_u32(fixture.surface, 2U) == 0x10000000U &&
          read_surface_u32(fixture.surface, 3U) == 0x10000000U &&
          read_surface_u32(fixture.surface, 4U) == 0U &&
          read_surface_u32(fixture.surface, 5U) == 0U &&
          fixture.ports.action_updates == 2U,
      "sub_405430 restores suspended roles and updates eligible no-PATH "
      "roles while excluding the controlled role");
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
  LegacyWorldPathScriptState path_state;
  LegacyWorldStoryVmState shared_state;
  FakePathPorts ports;
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
        1U, slots, node_pool, path_state,
        LegacyWorldPathScriptRuntime{
            .shared_script_state = &shared_state,
            .controlled_role_index = 1U,
        },
        ports);
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

  constexpr std::size_t payload_offset = 0x200U;
  constexpr u32 opcode_count = 37U;
  const u32 directory_count = read_u32(path, payload_offset + sizeof(u32)) /
                              static_cast<u32>(sizeof(u32));
  test.expect_equal(directory_count, u32{802U},
                    "current PATH directory contains 802 entries");
  if (directory_count != 802U ||
      payload_offset + static_cast<std::size_t>(directory_count) * sizeof(u32) >
          path.size()) {
    return;
  }

  struct PathCursor {
    u16 path_id{};
    u32 word_index{};
  };
  std::vector<PathCursor> pending;
  std::unordered_set<std::uint64_t> visited;
  std::array<bool, opcode_count> seen_opcodes{};
  u32 invalid_commands = 0U;
  u32 unsupported_opcodes = 0U;
  u32 unaligned_transfers = 0U;
  u32 valid_directory_entries = 0U;

  const auto enqueue = [&](const u16 path_id, const u32 word_index) {
    const std::uint64_t key =
        (static_cast<std::uint64_t>(path_id) << 32U) | word_index;
    if (visited.insert(key).second) {
      pending.push_back({path_id, word_index});
    }
  };
  for (u32 path_id = 1U; path_id < directory_count; ++path_id) {
    const u32 relative =
        read_u32(path, payload_offset + path_id * sizeof(u32));
    if (payload_offset + static_cast<std::size_t>(relative) + sizeof(u16) <=
        path.size()) {
      enqueue(static_cast<u16>(path_id), 0U);
      ++valid_directory_entries;
    }
  }
  for (const auto &source : decoded.database.role_sources) {
    if (source.path_data_id != 0U) {
      enqueue(source.path_data_id, static_cast<u32>(source.path_word_index));
    }
  }

  while (!pending.empty()) {
    const PathCursor cursor = pending.back();
    pending.pop_back();
    if (cursor.path_id >= directory_count) {
      std::cerr << "PATH invalid id=" << cursor.path_id << " word="
                << cursor.word_index << '\n';
      ++invalid_commands;
      continue;
    }
    const u32 script_relative = read_u32(
        path, payload_offset + static_cast<std::size_t>(cursor.path_id) *
                                   sizeof(u32));
    const std::int64_t signed_command_offset =
        static_cast<std::int64_t>(payload_offset) + script_relative +
        static_cast<std::int64_t>(
            std::bit_cast<std::int32_t>(cursor.word_index)) *
            static_cast<std::int64_t>(sizeof(u16));
    if (signed_command_offset < 0 ||
        signed_command_offset + static_cast<std::int64_t>(sizeof(u16)) >
            static_cast<std::int64_t>(path.size())) {
      ++invalid_commands;
      continue;
    }
    const std::size_t command_offset =
        static_cast<std::size_t>(signed_command_offset);

    const u16 opcode = read_u16(path, command_offset);
    if (opcode >= opcode_count) {
      std::cerr << "PATH unsupported id=" << cursor.path_id << " word="
                << cursor.word_index << " opcode=" << opcode << '\n';
      ++unsupported_opcodes;
      continue;
    }
    seen_opcodes[opcode] = true;
    const auto words_available = [&](const u32 words) {
      return command_offset + static_cast<std::size_t>(words) * sizeof(u16) <=
             path.size();
    };
    const auto advance = [&](const u32 words) {
      enqueue(cursor.path_id, cursor.word_index + words);
    };
    const auto transfer = [&](const std::size_t operand_word) {
      if (!words_available(static_cast<u32>(operand_word + 2U))) {
        ++invalid_commands;
        return;
      }
      const u32 target_relative = read_u32(
          path, command_offset + operand_word * sizeof(u16));
      const u32 difference = target_relative - script_relative;
      if ((difference & 1U) != 0U) {
        ++unaligned_transfers;
      }
      enqueue(cursor.path_id, std::bit_cast<u32>(
                                  std::bit_cast<std::int32_t>(difference) >> 1U));
    };

    switch (opcode) {
    case 0U:
      enqueue(cursor.path_id, 0U);
      break;
    case 1U:
    case 30U:
      break;
    case 2U:
    case 3U:
    case 17U:
    case 18U:
    case 27U:
    case 28U:
    case 32U:
    case 33U:
      if (words_available(2U)) {
        advance(2U);
      } else {
        ++invalid_commands;
      }
      break;
    case 4U:
    case 9U:
      if (words_available(2U)) {
        advance(2U);
      } else {
        ++invalid_commands;
      }
      break;
    case 5U:
    case 6U:
    case 8U:
    case 26U:
    case 35U:
      advance(1U);
      break;
    case 7U:
    case 19U:
    case 20U:
    case 21U:
    case 29U:
      if (words_available(3U)) {
        advance(3U);
      } else {
        ++invalid_commands;
      }
      break;
    case 10U:
    case 13U:
    case 14U:
    case 25U:
      if (words_available(4U)) {
        advance(4U);
        transfer(2U);
      } else {
        ++invalid_commands;
      }
      break;
    case 11U:
      if (words_available(2U)) {
        advance(1U);
      } else {
        ++invalid_commands;
      }
      break;
    case 12U:
      transfer(1U);
      break;
    case 15U:
    case 16U: {
      std::size_t operand_word = 1U;
      while (words_available(static_cast<u32>(operand_word + 1U)) &&
             read_u16(path, command_offset + operand_word * sizeof(u16)) !=
                 0xFFFFU) {
        ++operand_word;
      }
      if (!words_available(static_cast<u32>(operand_word + 3U))) {
        ++invalid_commands;
        break;
      }
      advance(static_cast<u32>(operand_word + 3U));
      transfer(operand_word + 1U);
      break;
    }
    case 22U:
    case 23U:
      if (words_available(5U)) {
        advance(5U);
        transfer(3U);
      } else {
        ++invalid_commands;
      }
      break;
    case 24U:
      if (!words_available(3U)) {
        ++invalid_commands;
        break;
      }
      advance(3U);
      if (const u16 selected_path =
              read_u16(path, command_offset + 2U * sizeof(u16));
          selected_path != 0U && selected_path < directory_count) {
        enqueue(selected_path, 0U);
      }
      break;
    case 31U:
      if (words_available(4U)) {
        advance(4U);
      } else {
        ++invalid_commands;
      }
      break;
    case 34U: {
      if (!words_available(3U)) {
        ++invalid_commands;
        break;
      }
      std::size_t terminator = command_offset + 2U * sizeof(u16);
      while (terminator + sizeof(u16) <= path.size() &&
             read_u16(path, terminator) != 0x5125U) {
        ++terminator;
      }
      if (terminator + sizeof(u16) > path.size()) {
        ++invalid_commands;
        break;
      }
      advance(static_cast<u32>((terminator - command_offset + sizeof(u16)) /
                               sizeof(u16)));
      break;
    }
    case 36U:
      if (words_available(4U)) {
        advance(4U);
      } else {
        ++invalid_commands;
      }
      break;
    default:
      ++unsupported_opcodes;
      break;
    }
  }

  const u32 covered_opcodes = static_cast<u32>(
      std::count(seen_opcodes.begin(), seen_opcodes.end(), true));
  if (invalid_commands != 0U || unsupported_opcodes != 0U ||
      unaligned_transfers != 0U || covered_opcodes != 20U) {
    std::cerr << "PATH coverage: invalid=" << invalid_commands
              << " unsupported=" << unsupported_opcodes
              << " unaligned=" << unaligned_transfers
              << " opcodes=" << covered_opcodes << " seen=";
    for (u32 opcode = 0U; opcode < opcode_count; ++opcode) {
      if (seen_opcodes[opcode]) {
        std::cerr << opcode << ',';
      }
    }
    std::cerr << '\n';
  }
  test.expect_equal(invalid_commands, u32{0U},
                    "all reachable current PATH commands stay in range");
  test.expect_equal(valid_directory_entries, u32{800U},
                    "all 800 current nonzero PATH directory entries resolve");
  test.expect_equal(unsupported_opcodes, u32{0U},
                    "all reachable current PATH commands use opcodes 0..36");
  test.expect_equal(unaligned_transfers, u32{0U},
                    "all current PATH transfer targets preserve word alignment");
  test.expect_equal(covered_opcodes, u32{20U},
                    "all 20 opcodes used by current PATH assets are covered");
}

} // namespace

int main(const int argc, char **argv) {
  openswd3::test::Context test;
  test_wait_opcode_preserves_frame_boundaries(test);
  test_request_and_movement_handshake(test);
  test_movement_gates_and_checked_boundaries(test);
  test_action_wait_and_audio_handlers(test);
  test_clock_flag_and_transfer_handlers(test);
  test_variable_handlers(test);
  test_role_mutation_and_random_walk_handlers(test);
  test_follow_and_label_handlers(test);
  test_ignored_path_request_failures(test);
  test_shared_path_clock_cadence(test);
  test_sub_405430_scan_gates(test);
  test_sub_405430_surface_restore_branches(test);
  if (argc == 3) {
    test_current_path_entries(test, argv[1], argv[2]);
  }
  return test.exit_code();
}
