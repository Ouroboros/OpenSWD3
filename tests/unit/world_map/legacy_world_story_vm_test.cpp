#include "test.hpp"

#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"
#include "openswd3/world_map/legacy_world_map_role_paths.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"
#include "openswd3/world_map/legacy_world_spatial_audio.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <span>
#include <tuple>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyTalkWindowLoadResult;
using openswd3::resource_io::LegacyTalkWindowStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldStoryVmPorts;
using openswd3::world_map::LegacyWorldStoryVmState;
using openswd3::world_map::LegacyWorldStoryVmStatus;
using openswd3::world_map::LegacyWorldTalkContext;

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

class RecordingPorts final : public LegacyWorldStoryVmPorts {
public:
  LegacyTalkWindowLoadResult load_story_window(
      const i32 story_id,
      const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
          destination,
      const bool clear_before_read) override {
    ++story_load_count;
    last_story_id = story_id;
    copy_window(story_id == 2042 ? transferred_window : initial_window,
                destination, clear_before_read);
    return LegacyTalkWindowLoadResult{
        .status = LegacyTalkWindowStatus::ready,
        .file_number = story_id == 2042 ? 2U : 1U,
        .entry_index = story_id == 2042 ? 42U : 248U,
        .data_offset = story_id == 2042 ? 0x2222U : 0x1111U,
        .actual_size = static_cast<u32>(story_id == 2042
                                           ? transferred_window.size()
                                           : initial_window.size()),
    };
  }

  LegacyTalkWindowLoadResult load_data_window(
      const u32 file_number, const u32 data_offset,
      const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
          destination,
      const bool clear_before_read) override {
    ++data_load_count;
    copy_window(transferred_window, destination, clear_before_read);
    return LegacyTalkWindowLoadResult{
        .status = LegacyTalkWindowStatus::ready,
        .file_number = file_number,
        .data_offset = data_offset,
        .actual_size = static_cast<u32>(transferred_window.size()),
    };
  }

  u32 update_action(
      openswd3::asset_runtime::LegacyActionRecord &action) override {
    ++action_update_count;
    action.field_4a = static_cast<u16>(action.action_id);
    return 1U;
  }

  void patch_role_source(
      const openswd3::world_map::LegacyMapsRolePatchRequest &request
  ) noexcept override {
    role_patch_requests.push_back(request);
  }

  void play_sound_effect(const u16 sound_id) noexcept override {
    sound_effect_requests.push_back(sound_id);
  }

  void clear_story_framebuffer() noexcept override {
    ++framebuffer_clear_count;
  }

  void present_story_framebuffer() noexcept override {
    ++framebuffer_present_count;
  }

  void begin_story_video(const std::span<const u8> filename) override {
    ++video_begin_count;
    last_video_filename.assign(filename.begin(), filename.end());
  }

  i32 query_story_video_progress() override {
    ++video_progress_query_count;
    return video_progress;
  }

  std::array<u8, 256U> initial_window{};
  std::array<u8, 256U> transferred_window{};
  u32 story_load_count{};
  u32 data_load_count{};
  u32 action_update_count{};
  u32 framebuffer_clear_count{};
  u32 framebuffer_present_count{};
  u32 video_begin_count{};
  u32 video_progress_query_count{};
  i32 last_story_id{};
  i32 video_progress{-1};
  std::vector<u8> last_video_filename;
  std::vector<openswd3::world_map::LegacyMapsRolePatchRequest>
      role_patch_requests;
  std::vector<u16> sound_effect_requests;

private:
  template <std::size_t Size>
  static void copy_window(
      const std::array<u8, Size> &source,
      const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
          destination,
      const bool clear_before_read) noexcept {
    if (clear_before_read) {
      std::ranges::fill(destination, u8{0x0CU});
    }
    std::ranges::copy(source, destination.begin());
  }
};

class RealPorts final : public LegacyWorldStoryVmPorts {
public:
  explicit RealPorts(
      openswd3::resource_io::LegacyResourceDatabases &databases) noexcept
      : databases_(databases) {}

  LegacyTalkWindowLoadResult load_story_window(
      const i32 story_id,
      const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
          destination,
      const bool clear_before_read) override {
    return databases_.load_talk_story_window(
        story_id, destination, clear_before_read);
  }

  LegacyTalkWindowLoadResult load_data_window(
      const u32 file_number, const u32 data_offset,
      const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
          destination,
      const bool clear_before_read) override {
    return databases_.load_talk_data_window(
        file_number, data_offset, destination, clear_before_read);
  }

  u32 update_action(
      openswd3::asset_runtime::LegacyActionRecord &) override {
    ++action_update_count;
    return 1U;
  }

  void patch_role_source(
      const openswd3::world_map::LegacyMapsRolePatchRequest &
  ) noexcept override {}

  void play_sound_effect(const u16 sound_id) noexcept override {
    sound_effect_requests.push_back(sound_id);
  }

  void clear_story_framebuffer() noexcept override {
    ++framebuffer_clear_count;
  }

  void present_story_framebuffer() noexcept override {
    ++framebuffer_present_count;
  }

  void begin_story_video(const std::span<const u8> filename) override {
    ++video_begin_count;
    last_video_filename.assign(filename.begin(), filename.end());
  }

  i32 query_story_video_progress() override {
    ++video_progress_query_count;
    return -1;
  }

  u32 action_update_count{};
  u32 framebuffer_clear_count{};
  u32 framebuffer_present_count{};
  u32 video_begin_count{};
  u32 video_progress_query_count{};
  std::vector<u8> last_video_filename;
  std::vector<u16> sound_effect_requests;

private:
  openswd3::resource_io::LegacyResourceDatabases &databases_;
};

class StoryFrameActionPorts final
    : public openswd3::asset_runtime::LegacyActionDrawPorts {
public:
  openswd3::asset_runtime::LegacyActionUpdateStatus
  update_action_record(
      openswd3::asset_runtime::LegacyActionRecord &) override {
    return openswd3::asset_runtime::LegacyActionUpdateStatus::completed;
  }

  bool load_frame_piece(
      u16, u16, openswd3::rendering::LegacyFramePiece &) override {
    return false;
  }

  openswd3::rendering::LegacyBlitExecutionStatus draw_frame_piece(
      const openswd3::rendering::LegacyFramePiece &, i32, i32, u32,
      i32) noexcept override {
    return openswd3::rendering::LegacyBlitExecutionStatus::completed;
  }
};

class StoryPathCompletionPorts final
    : public openswd3::world_map::LegacyWorldMapRolePathPorts {
public:
  explicit StoryPathCompletionPorts(
      openswd3::world_map::LegacyWorldStoryPathRuntime &runtime) noexcept
      : runtime_(runtime) {}

  bool complete_role_path(const u32 role_index) noexcept override {
    const auto result =
        openswd3::world_map::complete_legacy_world_story_path(
            runtime_, role_index);
    return result.status == openswd3::world_map::
                                LegacyWorldStoryPathStatus::completed;
  }

private:
  openswd3::world_map::LegacyWorldStoryPathRuntime &runtime_;
};

struct Fixture {
  LegacyWorldTalkContext context{};
  LegacyWorldStoryVmState state{};
  std::array<LegacyWorldRoleRecord, 3U> roles{};
  std::array<LegacyWorldObjectSlot,
             openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
      active_object_slots{};
  std::array<u8, 0x100U> maps_payload{};
  openswd3::story_scene::LegacyDialogRuntimeState dialogs{};
  openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources{};
  std::array<u8, 16U> first_name{};
  std::array<u8, 16U> second_name{};
  openswd3::world_map::LegacyWorldCameraRect camera{};
  openswd3::world_map::LegacyWorldStoryVmRuntime runtime{};
  RecordingPorts ports{};

  Fixture() {
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    context.source_guid = 0x00F8U;
    context.talk_script_id = 248U;
    roles[0].world_x = 16U;
    roles[0].world_y = 16U;
    roles[1].guid = 0x00F8U;
    roles[1].flags = openswd3::world_map::kLegacyWorldGuidLookupRoleBit;
    roles[1].world_x = 320U;
    roles[1].world_y = 240U;
    roles[1].action.variant_delta = 0U;
    runtime.camera = &camera;
    dialog_resources.frame_actions[0].action_id = 0x232DU;
    dialog_resources.caption_actions[0].action_id = 0x2337U;
  }

  [[nodiscard]] auto step(const i32 camera_left = 0,
                          const i32 camera_top = 0) {
    camera.left = std::bit_cast<u32>(camera_left);
    camera.top = std::bit_cast<u32>(camera_top);
    return openswd3::world_map::step_legacy_world_story_vm(
        context, state, roles, 0U, active_object_slots, maps_payload, dialogs,
        dialog_resources, first_name, second_name, runtime, ports);
  }
};

void test_initial_flags_and_alignment_gate(openswd3::test::Context &test) {
  Fixture fixture;
  const auto initialized = fixture.state;
  fixture.roles[0].world_x = 17U;
  const auto blocked = fixture.step();
  test.expect_true(
      openswd3::world_map::query_legacy_world_story_flag(initialized, 1U) &&
          openswd3::world_map::query_legacy_world_story_flag(initialized, 3U) &&
          openswd3::world_map::query_legacy_world_story_flag(initialized, 4U) &&
          openswd3::world_map::query_legacy_world_story_flag(initialized, 10U) &&
          openswd3::world_map::query_legacy_world_story_flag(initialized, 30U) &&
          openswd3::world_map::query_legacy_world_story_flag(initialized, 70U) &&
          !openswd3::world_map::query_legacy_world_story_flag(initialized, 2U) &&
          blocked.status == LegacyWorldStoryVmStatus::yielded &&
          fixture.ports.story_load_count == 0U &&
          fixture.context.talk_data_offset == 0U,
      "sub_40E0B0 flags and the first-load tile-alignment gate are exact");
}

void test_dialog_enqueue_and_wait_protocol(openswd3::test::Context &test) {
  Fixture fixture;
  auto script = std::span<u8>{fixture.ports.initial_window};
  write_u16(script, 0U, 89U);
  write_u16(script, 2U, 0x00F8U);
  write_u16(script, 4U, 0x232DU);
  write_u16(script, 6U, 14U);
  write_u16(script, 8U, 8U);
  script[10U] = 'A';
  script[11U] = '%';
  script[12U] = 'Q';
  write_u16(script, 13U, 14U);
  write_u16(script, 15U, 0x00F8U);
  write_u16(script, 17U, 0xFFFFU);
  fixture.state.speaker_name[0] = 'N';
  fixture.state.speaker_name[1] = 0U;

  const auto enqueued = fixture.step(16, 32);
  const auto &message = fixture.dialogs.messages.front();
  test.expect_equal(enqueued.status, LegacyWorldStoryVmStatus::yielded,
                    "opcode 89 yields");
  test.expect_equal(enqueued.executed_instruction_count, 1U,
                    "opcode 89 counts one instruction");
  test.expect_equal(enqueued.dialog_enqueue_count, 1U,
                    "opcode 89 enqueues one dialog");
  test.expect_equal(enqueued.action_update_count, 3U,
                    "initial load, frame and caption update three actions");
  test.expect_equal(fixture.context.instruction_offset, u16{13U},
                    "opcode 89 advances behind %Q");
  test.expect_equal(fixture.roles[1].interaction_gate, u16{1U},
                    "opcode 89 leaves the owner gate at one");
  test.expect_true((fixture.roles[1].flags & 0x00080000U) != 0U,
                   "initial load marks the source role");
  test.expect_equal(message.record.width, u16{154U},
                    "dialog width is column count times eleven");
  test.expect_equal(message.record.height, u16{88U},
                    "dialog height is row count times eleven");
  test.expect_equal(message.record.left, u16{227U},
                    "dialog left uses role, camera and facing offset");
  test.expect_equal(message.record.top, u16{260U},
                    "dialog top uses role, camera and facing offset");
  test.expect_equal(message.record.flags, u32{0x10U},
                    "opcode 89 adds only its odd-variant flag");
  test.expect_equal(message.record.character_delay, u16{4U},
                    "dialog delay is twice the initialized base delay");
  test.expect_true(
      message.record.saved_foreground_index == 0U &&
          message.record.saved_secondary_index == 0U &&
          message.record.text_style == 4U &&
          message.record.saved_text_style == 0U,
      "calloc leaves saved text attributes zero while sub_40AFF0 sets style four");
  test.expect_equal(message.caption.size(), std::size_t{1U},
                    "speaker name becomes the caption");
  test.expect_equal(message.text.size(), std::size_t{3U},
                    "dialog text includes its %Q terminator");
  test.expect_equal(fixture.state.speaker_name[0], u8{0U},
                    "speaker buffer is cleared after enqueue");
  test.expect_equal(fixture.dialogs.close.flagged_dialog_counter,
                    u32{0x8001U},
                    "dialog count increments without losing story lock");

  const auto waiting = fixture.step();
  const u16 waiting_instruction_offset = fixture.context.instruction_offset;
  fixture.roles[1].interaction_gate = 0U;
  const auto released = fixture.step();
  test.expect_true(
      waiting.status == LegacyWorldStoryVmStatus::yielded &&
          waiting.executed_instruction_count == 1U &&
          waiting_instruction_offset == 13U &&
          released.status == LegacyWorldStoryVmStatus::yielded &&
          released.executed_instruction_count == 1U &&
          fixture.context.instruction_offset == 17U,
      "opcode 14 stalls on gate one and advances then yields at zero");
}

void test_dialog_role_overlap_avoidance(openswd3::test::Context &test) {
  Fixture fixture;
  auto script = std::span<u8>{fixture.ports.initial_window};
  write_u16(script, 0U, 89U);
  write_u16(script, 2U, 0x00F8U);
  write_u16(script, 4U, 0x232DU);
  write_u16(script, 6U, 8U);
  write_u16(script, 8U, 4U);
  script[10U] = '%';
  script[11U] = 'Q';
  fixture.roles[1].world_x = 320U;
  fixture.roles[1].world_y = 30U;
  fixture.roles[1].action.variant_delta = 1U;

  const auto result = fixture.step();
  const auto &record = fixture.dialogs.messages.front().record;
  test.expect_true(
      result.status == LegacyWorldStoryVmStatus::yielded &&
          record.left == 276U && record.top == 32U,
      "sub_40AFF0 repeats the facing offset when the first panel still overlaps its role");
}

void test_transfer_flags_and_terminal_cleanup(openswd3::test::Context &test) {
  Fixture fixture;
  auto first = std::span<u8>{fixture.ports.initial_window};
  write_u16(first, 0U, 161U);
  write_u16(first, 2U, 2042U);
  auto second = std::span<u8>{fixture.ports.transferred_window};
  write_u16(second, 0U, 25U);
  write_u16(second, 2U, 123U);
  write_u16(second, 4U, 26U);
  write_u16(second, 6U, 3U);
  write_u16(second, 8U, 0xFFFFU);
  fixture.roles[1].flags |= 0x00000800U;
  fixture.roles[1].action.base_variant = 1U;
  fixture.roles[1].action.variant_delta = 2U;
  fixture.roles[1].action.one_shot_base_variant = 7U;
  fixture.roles[1].action.one_shot_variant_delta = 6U;
  fixture.roles[2].path_data_id = 9U;
  fixture.roles[2].path_word_index = 17U;
  fixture.roles[2].action.one_shot_base_variant = 8U;
  fixture.roles[2].action.one_shot_variant_delta = 5U;
  fixture.active_object_slots[0].bytes[0] = 2U;
  fixture.active_object_slots[0].bytes[1] = 0U;
  fixture.active_object_slots[0].bytes[0x1BU] = 2U;

  const auto result = fixture.step();
  test.expect_true(
      result.status == LegacyWorldStoryVmStatus::terminated &&
          result.executed_instruction_count == 4U &&
          fixture.ports.story_load_count == 2U &&
          fixture.ports.last_story_id == 2042 &&
          openswd3::world_map::query_legacy_world_story_flag(
              fixture.state, 123U) &&
          !openswd3::world_map::query_legacy_world_story_flag(
              fixture.state, 3U) &&
          fixture.context.source_guid == 0xFFFFU &&
          fixture.context.talk_data_offset == 0xFFFFFFFFU &&
          !fixture.state.window_loaded &&
          (fixture.dialogs.close.flagged_dialog_counter & 0x8000U) == 0U &&
          (fixture.roles[1].flags & 0x00080000U) == 0U &&
          fixture.roles[1].action.base_variant == 7U &&
          fixture.roles[1].action.variant_delta == 6U &&
          fixture.roles[1].action.one_shot_base_variant == 0xFFFFFFFFU &&
          fixture.roles[1].action.one_shot_variant_delta == 0xFFFFFFFFU &&
          fixture.roles[2].action.one_shot_base_variant == 0xFFFFFFFFU &&
          fixture.roles[2].action.one_shot_variant_delta == 0xFFFFFFFFU &&
          fixture.roles[2].path_data_id == 0U &&
          fixture.roles[2].path_word_index == 0U &&
          std::ranges::all_of(fixture.active_object_slots[0].bytes,
                              [](const u8 value) { return value == 0xFFU; }) &&
          result.role_one_shot_clear_count == fixture.roles.size() &&
          result.active_object_reset_count == 1U &&
          result.action_update_count == 2U,
      "opcode 161 replaces the window, 25/26 mutate flags, and FFFF restores and releases the source role");
}

void test_same_file_branch(openswd3::test::Context &test) {
  Fixture fixture;
  auto first = std::span<u8>{fixture.ports.initial_window};
  write_u16(first, 0U, 21U);
  write_u16(first, 2U, 1U);
  write_u32(first, 4U, 0x3333U);
  auto branch = std::span<u8>{fixture.ports.transferred_window};
  write_u16(branch, 0U, 0xFFFFU);

  const auto result = fixture.step();
  test.expect_true(
      result.status == LegacyWorldStoryVmStatus::terminated &&
          result.executed_instruction_count == 2U &&
          fixture.ports.data_load_count == 1U &&
          result.action_update_count == 2U,
      "opcode 21 branches within the current TALK file before TalkEnd");
}

void test_role_action_operand_extension(openswd3::test::Context &test) {
  Fixture fixture;
  auto script = std::span<u8>{fixture.ports.initial_window};
  write_u16(script, 0U, 120U);
  write_u16(script, 2U, 0x00F8U);
  write_u16(script, 4U, 0x8000U);
  write_u16(script, 6U, 0xFFFEU);
  write_u16(script, 8U, 0x8000U);
  write_u16(script, 10U, 14U);
  write_u16(script, 12U, 0x00F8U);

  const auto result = fixture.step();
  test.expect_true(
      result.status == LegacyWorldStoryVmStatus::yielded &&
          result.opcode == 14U &&
          fixture.roles[1].action.action_id == 0xFFFF8000U &&
          fixture.roles[1].action.base_variant == 0xFFFFFFFEU &&
          fixture.roles[1].action.variant_delta == 0x00008000U,
      "opcode 120 sign-extends action and base while zero-extending variant");
}

void test_role_action_chain_update_gate(openswd3::test::Context &test) {
  const auto run_chain = [](const u16 second_opcode) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 10U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 2U);
    write_u16(script, 6U, second_opcode);
    write_u16(script, 8U, 0x00F8U);
    write_u16(script, 10U, 3U);
    write_u16(script, 12U, 14U);
    write_u16(script, 14U, 0x00F8U);
    const auto result = fixture.step();
    return std::tuple{result, fixture.roles[1]};
  };

  const auto [plain_result, plain_role] = run_chain(11U);
  const auto [flagged_result, flagged_role] = run_chain(0x400BU);
  test.expect_true(
      plain_result.status == LegacyWorldStoryVmStatus::yielded &&
          plain_result.opcode == 14U &&
          plain_result.action_update_count == 2U &&
          plain_role.action.base_variant == 2U &&
          plain_role.action.variant_delta == 3U &&
          (plain_role.flags & 0x00001000U) != 0U,
      "opcodes 10 and 11 coalesce a same-role raw action chain");
  test.expect_true(
      flagged_result.status == LegacyWorldStoryVmStatus::yielded &&
          flagged_result.opcode == 14U &&
          flagged_result.action_update_count == 3U &&
          flagged_role.action.base_variant == 2U &&
          flagged_role.action.variant_delta == 3U,
      "sub_42E740 compares the next raw opcode without masking flag bits");
}

void test_change_requested_action_id(openswd3::test::Context &test) {
  Fixture chained;
  auto chained_script = std::span<u8>{chained.ports.initial_window};
  write_u16(chained_script, 0U, 45U);
  write_u16(chained_script, 2U, 0xFFF0U);
  write_u16(chained_script, 4U, 0x222U);
  write_u16(chained_script, 6U, 45U);
  write_u16(chained_script, 8U, 0x00F8U);
  write_u16(chained_script, 10U, 0U);
  write_u16(chained_script, 12U, 14U);
  write_u16(chained_script, 14U, 0x00F8U);
  const auto chained_result = chained.step();

  Fixture missing;
  auto missing_script = std::span<u8>{missing.ports.initial_window};
  write_u16(missing_script, 0U, 45U);
  write_u16(missing_script, 2U, 0x7777U);
  write_u16(missing_script, 4U, 0x333U);
  write_u16(missing_script, 6U, 14U);
  write_u16(missing_script, 8U, 0x00F8U);
  const auto missing_result = missing.step();
  const auto patch = missing.ports.role_patch_requests.empty()
                         ? openswd3::world_map::LegacyMapsRolePatchRequest{}
                         : missing.ports.role_patch_requests.front();

  test.expect_true(
      chained_result.status == LegacyWorldStoryVmStatus::yielded &&
          chained_result.opcode == 14U &&
          chained_result.executed_instruction_count == 3U &&
          chained_result.action_update_count == 2U &&
          chained.roles[1].action.action_id == 0U &&
          (chained.roles[1].flags & 0x00001000U) != 0U &&
          chained.context.instruction_offset == 16U,
      "opcode 45 coalesces a same-role chain and still writes action id zero");
  test.expect_true(
      missing_result.status == LegacyWorldStoryVmStatus::yielded &&
          missing_result.opcode == 14U &&
          missing_result.executed_instruction_count == 2U &&
          missing_result.action_update_count == 1U &&
          missing.ports.role_patch_requests.size() == 1U &&
          patch.guid == 0x7777U && patch.action_id == 0x333U &&
          patch.base_variant == 0xFFFFU &&
          patch.variant_delta == 0xFFFFU && patch.tile_x == 0xFFFFU &&
          patch.tile_y == 0xFFFFU && patch.talk_script_id == 0xFFFFU &&
          patch.path_data_id == 0xFFFFU &&
          patch.flags_or_mask == 0x1000U &&
          patch.flags_and_mask == 0xFFFFU &&
          patch.logical_map_id == 0xFFFFU,
      "opcode 45 routes a missing role through the exact MAPS patch request");
}

void test_wait_for_role_action_position(openswd3::test::Context &test) {
  Fixture waiting;
  auto waiting_script = std::span<u8>{waiting.ports.initial_window};
  write_u16(waiting_script, 0U, 107U);
  write_u16(waiting_script, 2U, 0xFFF0U);
  write_u16(waiting_script, 4U, 5U);
  write_u16(waiting_script, 6U, 14U);
  write_u16(waiting_script, 8U, 0x00F8U);
  waiting.roles[1].action.packed_ap_state = 0x0405U;
  const auto stalled = waiting.step();
  const u16 stalled_offset = waiting.context.instruction_offset;
  waiting.roles[1].action.packed_ap_state = 0x0505U;
  const auto completed = waiting.step();

  Fixture invalid_threshold;
  auto invalid_script =
      std::span<u8>{invalid_threshold.ports.initial_window};
  write_u16(invalid_script, 0U, 107U);
  write_u16(invalid_script, 2U, 0x00F8U);
  write_u16(invalid_script, 4U, 5U);
  write_u16(invalid_script, 6U, 14U);
  write_u16(invalid_script, 8U, 0x00F8U);
  invalid_threshold.roles[1].action.packed_ap_state = 0x0104U;
  const auto invalid = invalid_threshold.step();

  Fixture missing;
  auto missing_script = std::span<u8>{missing.ports.initial_window};
  write_u16(missing_script, 0U, 107U);
  write_u16(missing_script, 2U, 0x7777U);
  write_u16(missing_script, 4U, 5U);
  write_u16(missing_script, 6U, 14U);
  write_u16(missing_script, 8U, 0x00F8U);
  const auto absent = missing.step();

  test.expect_true(
      stalled.status == LegacyWorldStoryVmStatus::yielded &&
          stalled.opcode == 107U && stalled.executed_instruction_count == 1U &&
          stalled_offset == 0U &&
          completed.status == LegacyWorldStoryVmStatus::yielded &&
          completed.opcode == 14U &&
          completed.executed_instruction_count == 2U &&
          waiting.context.instruction_offset == 10U,
      "opcode 107 waits until the packed AP one-based index reaches its threshold");
  test.expect_true(
      invalid.status == LegacyWorldStoryVmStatus::yielded &&
          invalid.opcode == 14U &&
          invalid.executed_instruction_count == 2U &&
          invalid_threshold.context.instruction_offset == 10U &&
          absent.status == LegacyWorldStoryVmStatus::yielded &&
          absent.opcode == 14U && absent.executed_instruction_count == 2U &&
          missing.context.instruction_offset == 10U,
      "opcode 107 consumes invalid thresholds and missing roles without waiting");
}

void test_play_sound_effect_request(openswd3::test::Context &test) {
  Fixture fixture;
  auto script = std::span<u8>{fixture.ports.initial_window};
  write_u16(script, 0U, 59U);
  write_u16(script, 2U, 0x1234U);
  write_u16(script, 4U, 14U);
  write_u16(script, 6U, 0x00F8U);

  const auto requested = fixture.step();
  const auto continued = fixture.step();

  test.expect_true(
      requested.status == LegacyWorldStoryVmStatus::yielded &&
          requested.opcode == 59U &&
          requested.executed_instruction_count == 1U &&
          fixture.ports.sound_effect_requests.size() == 1U &&
          fixture.ports.sound_effect_requests.front() == 0x1234U &&
          continued.status == LegacyWorldStoryVmStatus::yielded &&
          continued.opcode == 14U &&
          fixture.context.instruction_offset == 8U,
      "opcode 59 submits the u16 sound id, advances four bytes and yields");
}

void test_turn_role_toward_role(openswd3::test::Context &test) {
  Fixture fixture;
  fixture.roles[1].action.base_variant = 7U;
  fixture.roles[1].action.variant_delta = 6U;
  fixture.roles[1].action.wait_remaining = 9U;
  fixture.roles[1].action.field_2c = 0U;
  fixture.roles[1].action.field_30 = 0U;
  fixture.roles[1].world_x = 100U;
  fixture.roles[1].world_y = 100U;
  fixture.roles[2].guid = 0x00F9U;
  fixture.roles[2].flags =
      openswd3::world_map::kLegacyWorldGuidLookupRoleBit;
  fixture.roles[2].world_x = 200U;
  fixture.roles[2].world_y = 100U;

  openswd3::world_map::LegacyRoleSpatialIndex spatial_index;
  std::vector<u8> surface_grid(16U * 16U * sizeof(u32), 0U);
  openswd3::world_map::LegacyWorldPathNodePool node_pool;
  openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
  u8 scene_render_flags{};
  std::array<u8, openswd3::world_map::kLegacyWorldGuidOneArrivalByteCount>
      selected_arrival_bytes{};
  openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
      .roles = fixture.roles,
      .active_object_slots = fixture.active_object_slots,
      .spatial_index = &spatial_index,
      .role_surface = {
          .map_width = 16U,
          .selected_guid = 0U,
          .surface_grid = surface_grid,
      },
      .node_pool = &node_pool,
      .movement = &movement,
      .camera = &fixture.camera,
      .selected_arrival_bytes = selected_arrival_bytes,
      .selected_role_index = 0U,
      .map_height = 16U,
      .scene_render_flags = &scene_render_flags,
  };
  fixture.runtime.story_paths = &story_paths;

  auto script = std::span<u8>{fixture.ports.initial_window};
  write_u16(script, 0U, 76U);
  write_u16(script, 2U, 0x00F8U);
  write_u16(script, 4U, 0x00F9U);
  write_u16(script, 6U, 14U);
  write_u16(script, 8U, 0x00F8U);
  const auto result = fixture.step();

  test.expect_true(
      result.status == LegacyWorldStoryVmStatus::yielded &&
          result.opcode == 14U && result.executed_instruction_count == 2U &&
          result.action_update_count == 2U &&
          fixture.roles[1].action.base_variant == 0U &&
          fixture.roles[1].action.variant_delta == 3U &&
          fixture.roles[1].action.wait_remaining == 0U &&
          (fixture.roles[1].flags & 0x80000000U) != 0U &&
          fixture.context.instruction_offset == 10U,
      "opcode 76 turns the first role toward the second and suspends it");
}

void test_set_role_head_sign_action(openswd3::test::Context &test) {
  Fixture fixture;
  auto script = std::span<u8>{fixture.ports.initial_window};
  write_u16(script, 0U, 71U);
  write_u16(script, 2U, 0x00F8U);
  write_u16(script, 4U, 3U);
  write_u16(script, 6U, 14U);
  write_u16(script, 8U, 0x00F8U);
  const auto assigned = fixture.step();

  Fixture missing;
  missing.roles[1].field_3c = 0x12345678U;
  auto missing_script = std::span<u8>{missing.ports.initial_window};
  write_u16(missing_script, 0U, 71U);
  write_u16(missing_script, 2U, 0xFFF0U);
  write_u16(missing_script, 4U, 0U);
  write_u16(missing_script, 6U, 14U);
  write_u16(missing_script, 8U, 0x00F8U);
  const auto ignored = missing.step();

  Fixture cleared;
  cleared.roles[1].field_3c =
      openswd3::world_map::legacy_world_head_sign_action_token(2U);
  auto clear_script = std::span<u8>{cleared.ports.initial_window};
  write_u16(clear_script, 0U, 72U);
  write_u16(clear_script, 2U, 0x00F8U);
  write_u16(clear_script, 4U, 14U);
  write_u16(clear_script, 6U, 0x00F8U);
  const auto removed = cleared.step();

  test.expect_true(
      assigned.status == LegacyWorldStoryVmStatus::yielded &&
          assigned.opcode == 14U && assigned.executed_instruction_count == 2U &&
          fixture.roles[1].field_3c ==
              openswd3::world_map::legacy_world_head_sign_action_token(3U) &&
          ignored.status == LegacyWorldStoryVmStatus::yielded &&
          ignored.opcode == 14U && ignored.executed_instruction_count == 2U &&
          missing.roles[1].field_3c == 0x12345678U &&
          removed.status == LegacyWorldStoryVmStatus::yielded &&
          removed.opcode == 14U && removed.executed_instruction_count == 2U &&
          cleared.roles[1].field_3c == 0U,
      "opcodes 71 and 72 assign and clear the head sign while unresolved "
      "selectors are consumed without substituting FFF0");
}

void test_set_and_clear_role_wait_override(openswd3::test::Context &test) {
  Fixture assigned;
  assigned.roles[1].action.wait_remaining = 9U;
  auto assigned_script = std::span<u8>{assigned.ports.initial_window};
  write_u16(assigned_script, 0U, 77U);
  write_u16(assigned_script, 2U, 0xFFF0U);
  write_u16(assigned_script, 4U, 3U);
  write_u16(assigned_script, 6U, 14U);
  write_u16(assigned_script, 8U, 0x00F8U);
  const auto assigned_result = assigned.step();

  Fixture cleared;
  cleared.roles[1].action.wait_override = 0x8123U;
  cleared.roles[1].action.wait_remaining = 9U;
  auto cleared_script = std::span<u8>{cleared.ports.initial_window};
  write_u16(cleared_script, 0U, 78U);
  write_u16(cleared_script, 2U, 0x00F8U);
  write_u16(cleared_script, 4U, 14U);
  write_u16(cleared_script, 6U, 0x00F8U);
  const auto cleared_result = cleared.step();

  Fixture missing;
  auto missing_script = std::span<u8>{missing.ports.initial_window};
  write_u16(missing_script, 0U, 77U);
  write_u16(missing_script, 2U, 0x7777U);
  write_u16(missing_script, 4U, 5U);
  const auto missing_result = missing.step();

  test.expect_true(
      assigned_result.status == LegacyWorldStoryVmStatus::yielded &&
          assigned_result.opcode == 14U &&
          assigned_result.executed_instruction_count == 2U &&
          assigned_result.action_update_count == 2U &&
          assigned.roles[1].action.wait_override == 0x8003U &&
          assigned.roles[1].action.wait_remaining == 0U &&
          assigned.context.instruction_offset == 10U &&
          cleared_result.status == LegacyWorldStoryVmStatus::yielded &&
          cleared_result.opcode == 14U &&
          cleared_result.executed_instruction_count == 2U &&
          cleared_result.action_update_count == 2U &&
          cleared.roles[1].action.wait_override == 0U &&
          cleared.roles[1].action.wait_remaining == 0U &&
          cleared.context.instruction_offset == 8U &&
          missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
          missing.context.instruction_offset == 0U,
      "opcodes 77 and 78 refresh the role wait override while an unresolved "
      "selector preserves the undefined-width instruction boundary");
}

void test_real_story_248_dialog(openswd3::test::Context &test,
                                const std::filesystem::path &root) {
  openswd3::resource_io::LegacyResourceDatabases databases;
  const auto initialized = databases.initialize(root);
  const auto maps = databases.reload_maps_payload();
  LegacyWorldStoryVmState state{};
  openswd3::world_map::initialize_legacy_world_story_vm(state);
  LegacyWorldTalkContext context{};
  context.source_guid = 248U;
  context.talk_script_id = 248U;
  std::array<LegacyWorldRoleRecord, 2U> roles{};
  std::array<LegacyWorldObjectSlot,
             openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
      active_object_slots{};
  roles[0].world_x = 16U;
  roles[0].world_y = 16U;
  roles[1].guid = 248U;
  roles[1].flags = openswd3::world_map::kLegacyWorldGuidLookupRoleBit;
  roles[1].world_x = 320U;
  roles[1].world_y = 240U;
  roles[1].action.variant_delta = 4U;
  openswd3::story_scene::LegacyDialogRuntimeState dialogs;
  openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
  constexpr std::array<u16, 4U> kFrames{0x232DU, 0x232FU, 0x2330U,
                                        0x2331U};
  constexpr std::array<u16, 4U> kCaptions{0x2337U, 0x2339U, 0x233AU,
                                          0x233BU};
  for (std::size_t index = 0U; index < kFrames.size(); ++index) {
    dialog_resources.frame_actions[index].action_id = kFrames[index];
    dialog_resources.caption_actions[index].action_id = kCaptions[index];
  }
  std::array<u8, 16U> first_name{};
  std::array<u8, 16U> second_name{};
  RealPorts ports{databases};
  openswd3::world_map::LegacyWorldCameraRect camera{};
  const openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
      .camera = &camera,
  };

  const auto result = openswd3::world_map::step_legacy_world_story_vm(
      context, state, roles, 0U, active_object_slots,
      databases.maps_payload_bytes(), dialogs,
      dialog_resources, first_name, second_name, runtime, ports);
  test.expect_true(
      initialized.status ==
              openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
          maps.status ==
              openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
          result.status == LegacyWorldStoryVmStatus::yielded &&
          result.opcode == 89U && result.executed_instruction_count == 3U &&
          result.dialog_enqueue_count == 1U && dialogs.messages.size() == 1U &&
          !dialogs.messages.front().caption.empty() &&
          dialogs.messages.front().record.width == 154U &&
          dialogs.messages.front().record.height == 88U &&
          roles[1].interaction_gate == 1U,
      "real story 248 executes 0x402, 91 and 89 into its first dialog");
}

void test_real_new_game_story_reaches_first_dialog(
    openswd3::test::Context &test, const std::filesystem::path &root) {
  openswd3::resource_io::LegacyResourceDatabases databases;
  const auto initialized = databases.initialize(root);
  const auto maps = databases.reload_maps_payload();
  const auto maps_world = openswd3::world_map::decode_legacy_maps_world_database(
      databases.maps_payload_bytes());
  LegacyWorldStoryVmState state{};
  openswd3::world_map::initialize_legacy_world_story_vm(state);
  LegacyWorldTalkContext context{};
  context.source_guid = 1U;
  context.talk_script_id = 100U;

  std::vector<LegacyWorldRoleRecord> roles(11U);
  const auto initialize_role = [&](const std::size_t index, const u16 guid,
                                   const u32 tile_x, const u32 tile_y) {
    auto &role = roles[index];
    role.guid = guid;
    role.flags = openswd3::world_map::kLegacyWorldGuidLookupRoleBit;
    const auto source = std::ranges::find(
        maps_world.database.role_sources, guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid);
    if (source != maps_world.database.role_sources.end()) {
      role.flags |= source->flags;
    }
    role.world_x = tile_x << 4U;
    role.world_y = tile_y << 4U;
    role.map_cell_pointer_32 = tile_y * 80U + tile_x;
    role.action.field_2c = 1U;
    role.action.field_30 = 1U;
  };
  initialize_role(1U, 1U, 2U, 2U);
  initialize_role(2U, 1U, 3U, 3U);
  initialize_role(3U, 123U, 4U, 4U);
  initialize_role(4U, 240U, 5U, 5U);
  initialize_role(5U, 195U, 6U, 6U);
  initialize_role(6U, 248U, 7U, 7U);
  initialize_role(7U, 249U, 8U, 8U);
  initialize_role(8U, 191U, 9U, 9U);
  initialize_role(9U, 250U, 10U, 10U);
  initialize_role(10U, 251U, 11U, 11U);

  openswd3::world_map::LegacyRoleSpatialIndex spatial_index;
  spatial_index.map_height = 80U;
  for (auto &row_heads : spatial_index.row_heads) {
    row_heads.assign(120U, openswd3::world_map::kLegacySpatialNoRole);
  }
  const bool inserted_role_one =
      openswd3::world_map::insert_legacy_role_spatially(
          spatial_index, roles, 1U);
  const bool inserted_role_195 =
      openswd3::world_map::insert_legacy_role_spatially(
          spatial_index, roles, 5U);
  const bool inserted_role_248 =
      openswd3::world_map::insert_legacy_role_spatially(
          spatial_index, roles, 6U);
  const bool inserted_role_249 =
      openswd3::world_map::insert_legacy_role_spatially(
          spatial_index, roles, 7U);
  const bool inserted_role_250 =
      openswd3::world_map::insert_legacy_role_spatially(
          spatial_index, roles, 9U);
  const bool inserted_role_251 =
      openswd3::world_map::insert_legacy_role_spatially(
          spatial_index, roles, 10U);

  std::vector<u8> surface_grid(80U * 80U * sizeof(u32), 0U);
  openswd3::world_map::LegacyWorldMapRolePathState map_role_paths{};
  openswd3::story_scene::LegacyDialogRuntimeState dialogs;
  openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
  constexpr std::array<u16, 4U> kFrames{0x232DU, 0x232FU, 0x2330U,
                                        0x2331U};
  constexpr std::array<u16, 4U> kCaptions{0x2337U, 0x2339U, 0x233AU,
                                          0x233BU};
  for (std::size_t index = 0U; index < kFrames.size(); ++index) {
    dialog_resources.frame_actions[index].action_id = kFrames[index];
    dialog_resources.caption_actions[index].action_id = kCaptions[index];
  }

  openswd3::world_map::LegacyWorldCameraRect camera{};
  camera.right = 640U;
  camera.bottom = 480U;
  openswd3::world_map::LegacyWorldCameraPanState camera_pan{};
  openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
  openswd3::world_map::LegacyPictureActionLists picture_actions;
  openswd3::rendering::LegacyFrameColorTransitionState frame_color{};
  u8 scene_render_flags{};
  openswd3::world_map::LegacyWorldPathNodePool path_node_pool;
  openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
      .roles = roles,
      .active_object_slots = map_role_paths.active_object_slots,
      .spatial_index = &spatial_index,
      .role_surface = {
          .map_width = 80U,
          .selected_guid = 1U,
          .surface_grid = surface_grid,
      },
      .node_pool = &path_node_pool,
      .movement = &movement,
      .camera = &camera,
      .selected_arrival_bytes = map_role_paths.guid_one_arrival_bytes,
      .selected_role_index = 1U,
      .map_height = 80U,
      .scene_render_flags = &scene_render_flags,
  };
  openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
      .spatial_index = &spatial_index,
      .role_surface = {
          .map_width = 80U,
          .selected_guid = 1U,
          .surface_grid = surface_grid,
      },
      .camera = &camera,
      .camera_pan = &camera_pan,
      .movement = &movement,
      .picture_actions = &picture_actions,
      .frame_color = &frame_color,
      .story_paths = &story_paths,
      .scene_render_flags = &scene_render_flags,
      .map_height = 80U,
  };
  std::array<u8, 16U> first_name{};
  std::array<u8, 16U> second_name{};
  RealPorts ports{databases};
  const auto step = [&] {
    return openswd3::world_map::step_legacy_world_story_vm(
        context, state, roles, 1U, map_role_paths.active_object_slots,
        databases.maps_payload_bytes(), dialogs, dialog_resources, first_name,
        second_name, runtime, ports);
  };
  StoryFrameActionPorts frame_actions;
  StoryPathCompletionPorts path_completion{story_paths};
  const auto advance_path_frame = [&] {
    return openswd3::world_map::advance_legacy_world_map_role_paths(
        roles, spatial_index, runtime.role_surface, 1U, scene_render_flags,
        movement, camera, map_role_paths, frame_actions, path_completion);
  };

  const auto first_clear = step();
  const auto opening_wait = step();
  runtime.current_tick = 2001U;
  const auto opening_video = step();
  const auto branch_clear = step();
  const auto first_scene_wait = step();
  runtime.current_tick += 4501U;
  const auto first_picture = step();
  const auto second_scene_wait = step();
  runtime.current_tick += 7501U;
  const auto second_picture = step();
  const auto third_scene_wait = step();
  runtime.current_tick += 7501U;
  const auto transition_clear = step();
  const auto camera_wait = step();
  while (openswd3::world_map::advance_legacy_world_camera_pan(
      camera, camera_pan)) {
  }
  const auto title = step();
  const auto title_record = dialogs.messages.back().record;
  roles[2].interaction_gate = 0U;
  const auto first_dialog = step();
  const u16 first_dialog_gate = roles[6].interaction_gate;
  const bool first_dialog_has_caption = !dialogs.messages.back().caption.empty();
  auto last_dialog = first_dialog;
  for (std::size_t dialog_index = 1U; dialog_index < 10U;
       ++dialog_index) {
    for (auto &role : roles) {
      role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_dialog = step();
    test.expect_equal(released_dialog.opcode, u16{14U},
                      "story 100 dialog release boundary");
    last_dialog = step();
  }
  for (auto &role : roles) {
    role.interaction_gate = 0U;
  }
  context.field_26 = 0U;
  const auto released_last_dialog = step();
  const auto first_path_scene = step();
  const auto first_path_schedule = step();
  auto first_path_wait = first_path_schedule;
  bool first_path_frames_completed = true;
  std::size_t first_path_frame_count{};
  for (; first_path_frame_count < 512U; ++first_path_frame_count) {
    first_path_wait = step();
    if (first_path_wait.opcode == 67U) {
      break;
    }
    const auto advanced = advance_path_frame();
    if (first_path_wait.opcode != 20U ||
        advanced.status != openswd3::world_map::
                               LegacyWorldMapRolePathStatus::completed) {
      first_path_frames_completed = false;
      break;
    }
  }
  runtime.current_tick =
      state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
  const auto second_path_schedule = step();
  auto second_path_wait = second_path_schedule;
  bool second_path_frames_completed = true;
  std::size_t second_path_frame_count{};
  for (; second_path_frame_count < 512U; ++second_path_frame_count) {
    second_path_wait = step();
    if (second_path_wait.opcode == 95U) {
      break;
    }
    const auto advanced = advance_path_frame();
    if (second_path_wait.opcode != 20U ||
        advanced.status != openswd3::world_map::
                               LegacyWorldMapRolePathStatus::completed) {
      second_path_frames_completed = false;
      break;
    }
  }
  const auto hidden_scene_wait = step();
  runtime.current_tick =
      state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
  const auto first_facing_wait = step();
  runtime.current_tick =
      state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
  const auto next_dialog = step();
  const u32 path_final_facing = roles[2].action.variant_delta;
  auto later_dialog = next_dialog;
  bool later_dialog_chain_completed = true;
  for (std::size_t dialog_index = 0U; dialog_index < 5U; ++dialog_index) {
    for (auto &role : roles) {
      role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_dialog = step();
    if (released_dialog.opcode != 14U ||
        released_dialog.status != LegacyWorldStoryVmStatus::yielded) {
      later_dialog_chain_completed = false;
      break;
    }
    later_dialog = step();
    if (later_dialog.opcode != 89U ||
        later_dialog.status != LegacyWorldStoryVmStatus::yielded) {
      later_dialog_chain_completed = false;
      break;
    }
  }
  for (auto &role : roles) {
    role.interaction_gate = 0U;
  }
  context.field_26 = 0U;
  const auto released_later_dialog = step();
  const auto head_sign_wait = step();
  const u32 head_sign_token = roles[8].field_3c;
  runtime.current_tick =
      state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
  const auto post_head_sign_dialog = step();
  auto next_unsupported = post_head_sign_dialog;
  std::size_t post_head_sign_dialog_count{};
  bool post_head_sign_dialog_releases_completed = true;
  while (post_head_sign_dialog_count < 32U &&
         next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
         next_unsupported.opcode == 89U) {
    for (auto &role : roles) {
      role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_dialog = step();
    if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
        released_dialog.opcode != 14U) {
      post_head_sign_dialog_releases_completed = false;
      break;
    }
    next_unsupported = step();
    ++post_head_sign_dialog_count;
  }
  std::size_t third_path_frame_count{};
  bool third_path_frames_completed = true;
  if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
      next_unsupported.opcode == 20U) {
    for (; third_path_frame_count < 512U; ++third_path_frame_count) {
      const auto advanced = advance_path_frame();
      if (advanced.status != openswd3::world_map::
                                 LegacyWorldMapRolePathStatus::completed) {
        third_path_frames_completed = false;
        break;
      }
      next_unsupported = step();
      if (next_unsupported.opcode != 20U) {
        break;
      }
    }
  }
  bool third_path_wait_completed = false;
  if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
      next_unsupported.opcode == 67U) {
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    next_unsupported = step();
    third_path_wait_completed = true;
  }
  std::size_t third_path_dialog_count{};
  bool third_path_dialog_releases_completed = true;
  while (third_path_dialog_count < 32U &&
         next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
         next_unsupported.opcode == 89U) {
    for (auto &role : roles) {
      role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_dialog = step();
    if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
        released_dialog.opcode != 14U) {
      third_path_dialog_releases_completed = false;
      break;
    }
    next_unsupported = step();
    ++third_path_dialog_count;
  }
  std::size_t final_dialog_count{};
  if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
      next_unsupported.opcode == 67U) {
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    next_unsupported = step();
  }
  while (final_dialog_count < 32U &&
         next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
         next_unsupported.opcode == 89U) {
    for (auto &role : roles) {
      role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_dialog = step();
    if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
        released_dialog.opcode != 14U) {
      break;
    }
    next_unsupported = step();
    ++final_dialog_count;
  }
  std::size_t fourth_path_frame_count{};
  bool fourth_path_frames_completed = true;
  if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
      next_unsupported.opcode == 20U) {
    for (; fourth_path_frame_count < 512U; ++fourth_path_frame_count) {
      const auto advanced = advance_path_frame();
      if (advanced.status != openswd3::world_map::
                                 LegacyWorldMapRolePathStatus::completed) {
        fourth_path_frames_completed = false;
        break;
      }
      next_unsupported = step();
      if (next_unsupported.opcode != 20U) {
        break;
      }
    }
  }
  std::size_t post_opcode_45_wait_count{};
  std::size_t post_opcode_45_dialog_count{};
  std::size_t post_opcode_45_path_frame_count{};
  bool post_opcode_45_progression_completed = true;
  for (std::size_t boundary_count = 0U;
       boundary_count < 2048U &&
       next_unsupported.status == LegacyWorldStoryVmStatus::yielded;
       ++boundary_count) {
    switch (next_unsupported.opcode) {
    case 20U: {
      const auto advanced = advance_path_frame();
      if (advanced.status != openswd3::world_map::
                                 LegacyWorldMapRolePathStatus::completed) {
        post_opcode_45_progression_completed = false;
        break;
      }
      ++post_opcode_45_path_frame_count;
      next_unsupported = step();
      break;
    }
    case 51U:
      while (openswd3::world_map::advance_legacy_world_camera_pan(
          camera, camera_pan)) {
      }
      next_unsupported = step();
      break;
    case 67U:
      runtime.current_tick =
          state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
      ++post_opcode_45_wait_count;
      next_unsupported = step();
      break;
    case 89U:
      for (auto &role : roles) {
        role.interaction_gate = 0U;
      }
      context.field_26 = 0U;
      if (const auto released_dialog = step();
          released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
          released_dialog.opcode != 14U) {
        post_opcode_45_progression_completed = false;
        break;
      }
      ++post_opcode_45_dialog_count;
      next_unsupported = step();
      break;
    default:
      next_unsupported = step();
      break;
    }
    if (!post_opcode_45_progression_completed) {
      break;
    }
  }
  const std::string video_filename{
      ports.last_video_filename.begin(), ports.last_video_filename.end()};
  test.expect_equal(first_clear.opcode, u16{61U},
                    "story 100 first clear boundary");
  test.expect_equal(opening_wait.opcode, u16{67U},
                    "story 100 opening wait boundary");
  test.expect_equal(opening_video.opcode, u16{85U},
                    "story 100 video boundary");
  test.expect_equal(branch_clear.opcode, u16{61U},
                    "story 100 branch clear boundary");
  test.expect_equal(first_scene_wait.opcode, u16{67U},
                    "story 100 first scene wait boundary");
  test.expect_equal(first_picture.opcode, u16{153U},
                    "story 100 first picture boundary");
  test.expect_equal(second_scene_wait.opcode, u16{67U},
                    "story 100 second scene wait boundary");
  test.expect_equal(second_picture.opcode, u16{153U},
                    "story 100 second picture boundary");
  test.expect_equal(third_scene_wait.opcode, u16{67U},
                    "story 100 third scene wait boundary");
  test.expect_equal(transition_clear.opcode, u16{60U},
                    "story 100 transition clear boundary");
  test.expect_equal(camera_wait.opcode, u16{51U},
                    "story 100 camera wait boundary");
  test.expect_equal(title.opcode, u16{6U},
                    "story 100 title boundary");
  test.expect_equal(first_dialog.opcode, u16{89U},
                    "story 100 first spoken dialog boundary");
  test.expect_equal(last_dialog.opcode, u16{89U},
                    "story 100 tenth spoken dialog boundary");
  test.expect_equal(released_last_dialog.opcode, u16{14U},
                    "story 100 last dialog release boundary");
  test.expect_equal(first_path_scene.opcode, u16{94U},
                    "story 100 first path scene boundary");
  test.expect_equal(first_path_schedule.opcode, u16{20U},
                    "story 100 first path schedule boundary");
  test.expect_equal(first_path_wait.opcode, u16{67U},
                    "story 100 first path completion boundary");
  test.expect_equal(second_path_schedule.opcode, u16{20U},
                    "story 100 second path schedule boundary");
  test.expect_equal(second_path_wait.opcode, u16{95U},
                    "story 100 second path completion boundary");
  test.expect_equal(hidden_scene_wait.opcode, u16{67U},
                    "story 100 hidden-scene wait boundary");
  test.expect_equal(first_facing_wait.opcode, u16{67U},
                    "story 100 first facing wait boundary");
  test.expect_equal(next_dialog.opcode, u16{89U},
                    "story 100 next dialog boundary");
  test.expect_equal(later_dialog.opcode, u16{89U},
                    "story 100 fifth post-path dialog boundary");
  test.expect_equal(released_later_dialog.opcode, u16{14U},
                    "story 100 fifth post-path dialog release boundary");
  test.expect_equal(head_sign_wait.opcode, u16{67U},
                    "story 100 head-sign wait boundary");
  test.expect_equal(post_head_sign_dialog.opcode, u16{89U},
                    "story 100 post-head-sign dialog boundary");
  test.expect_equal(post_head_sign_dialog_count, std::size_t{11U},
                    "story 100 post-head-sign dialog count");
  test.expect_equal(third_path_frame_count, std::size_t{46U},
                    "story 100 third path frame count");
  test.expect_equal(third_path_dialog_count, std::size_t{5U},
                    "story 100 third path dialog count");
  test.expect_equal(final_dialog_count, std::size_t{7U},
                    "story 100 final dialog count");
  test.expect_equal(fourth_path_frame_count, std::size_t{40U},
                    "story 100 fourth path frame count");
  test.expect_equal(post_opcode_45_wait_count, std::size_t{6U},
                    "story 100 post-opcode-45 wait count");
  test.expect_equal(post_opcode_45_dialog_count, std::size_t{6U},
                    "story 100 post-opcode-45 dialog count");
  test.expect_equal(post_opcode_45_path_frame_count, std::size_t{29U},
                    "story 100 post-opcode-45 path frame count");
  test.expect_equal(next_unsupported.opcode, u16{53U},
                    "story 100 next unsupported opcode boundary");
  test.expect_equal(next_unsupported.status,
                    LegacyWorldStoryVmStatus::unsupported_opcode,
                    "story 100 stops without consuming opcode 53");
  test.expect_equal(state.loaded_data_offset, u32{17476U},
                    "story 100 branched window base through both paths");
  test.expect_equal(context.instruction_offset, u16{3611U},
                    "story 100 instruction boundary at opcode 53");
  test.expect_true(
      initialized.status ==
              openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
          maps.status ==
              openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
          maps_world.status == openswd3::world_map::
                                   LegacyMapsWorldDatabaseStatus::ready &&
          inserted_role_one && inserted_role_195 && inserted_role_248 &&
          inserted_role_249 && inserted_role_250 && inserted_role_251,
      "real story 100 fixture uses the real databases and valid spatial roles");
  test.expect_true(
          first_clear.opcode == 61U && opening_wait.opcode == 67U &&
          opening_video.opcode == 85U && branch_clear.opcode == 61U &&
          first_scene_wait.opcode == 67U && first_picture.opcode == 153U &&
          second_scene_wait.opcode == 67U && second_picture.opcode == 153U &&
          third_scene_wait.opcode == 67U && transition_clear.opcode == 60U &&
          camera_wait.opcode == 51U && title.opcode == 6U &&
          title.status == LegacyWorldStoryVmStatus::yielded &&
          title_record.flags == 0x468U && title_record.lifetime_limit == 20U &&
          title_record.left == 20U && title_record.top == 20U &&
          title_record.width == 132U && title_record.height == 22U &&
          first_dialog.opcode == 89U &&
          first_dialog.status == LegacyWorldStoryVmStatus::yielded &&
          first_dialog_has_caption &&
          first_dialog_gate == 1U && first_path_frames_completed &&
          second_path_frames_completed && first_path_frame_count < 512U &&
          second_path_frame_count < 512U &&
          next_dialog.status == LegacyWorldStoryVmStatus::yielded &&
          later_dialog_chain_completed &&
          head_sign_wait.status == LegacyWorldStoryVmStatus::yielded &&
          head_sign_token == openswd3::world_map::
                                 legacy_world_head_sign_action_token(0U) &&
          post_head_sign_dialog.status ==
              LegacyWorldStoryVmStatus::yielded &&
          roles[8].field_3c == 0U &&
          post_head_sign_dialog_releases_completed &&
          third_path_frames_completed && third_path_wait_completed &&
          third_path_dialog_releases_completed &&
          fourth_path_frames_completed &&
          post_opcode_45_progression_completed &&
          dialogs.messages.size() == 46U &&
          (roles[9].flags & 0x00008000U) != 0U &&
          (roles[10].flags & 0x00008000U) != 0U &&
          roles[9].action.base_variant == 0U &&
          roles[10].action.base_variant == 0U &&
          roles[9].action.wait_override == 0x8002U &&
          roles[10].action.wait_override == 0x8003U &&
          roles[9].action.wait_remaining == 0U &&
          roles[10].action.wait_remaining == 0U &&
          roles[10].field_3c == 0U,
      "real story 100 crosses opcode 45 and all subsequent restored waits, "
      "dialogs and paths through opcode 59 before opcode 53");
  test.expect_true(
      ports.sound_effect_requests.size() == 1U &&
          ports.sound_effect_requests.front() == 0x73U,
      "real story 100 submits the scripted opcode 59 sound id");
  test.expect_equal(roles[6].world_x, u32{37U * 16U},
                    "real story 100 relocates role 248 x");
  test.expect_equal(roles[6].world_y, u32{33U * 16U},
                    "real story 100 relocates role 248 y");
  test.expect_equal(roles[7].world_x, u32{39U * 16U},
                    "real story 100 relocates role 249 x");
  test.expect_equal(roles[7].world_y, u32{33U * 16U},
                    "real story 100 relocates role 249 y");
  test.expect_equal(roles[2].world_x, u32{13U * 16U},
                    "real story 100 completes visible GUID 1 second path x");
  test.expect_equal(roles[2].world_y, u32{28U * 16U},
                    "real story 100 completes visible GUID 1 second path y");
  test.expect_equal(roles[2].action.base_variant, u32{68U},
                    "real story 100 applies the later opcode 10 base variant");
  test.expect_equal(path_final_facing, u32{7U},
                    "real story 100 applies the final opcode 11 facing");
  test.expect_equal(roles[5].world_x, u32{16U * 16U},
                    "real story 100 relocates role 195 x");
  test.expect_equal(roles[5].world_y, u32{36U * 16U},
                    "real story 100 relocates role 195 y");
  test.expect_true(
          picture_actions.secondary.size() == 2U &&
          frame_color.current_red == 0.0F &&
          frame_color.current_green == 0.0F &&
          frame_color.current_blue == 0.0F &&
          frame_color.target_red == 10.0F &&
          frame_color.target_green == 10.0F &&
          frame_color.target_blue == 10.0F &&
          frame_color.step_red == 10.0F &&
          frame_color.step_green == 10.0F &&
          frame_color.step_blue == 10.0F &&
          frame_color.countdown == 1 && scene_render_flags == 0U &&
          (roles[2].flags & 0x00001000U) != 0U,
      "real story 100 creates two pictures and starts the later color transition");
  test.expect_true(
          ports.framebuffer_clear_count == 3U &&
          ports.framebuffer_present_count == 1U &&
          ports.video_begin_count == 1U &&
          ports.video_progress_query_count == 1U &&
          video_filename == "OPENING.bik",
      "real story 100 preserves its framebuffer and video side effects");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_initial_flags_and_alignment_gate(test);
  test_dialog_enqueue_and_wait_protocol(test);
  test_dialog_role_overlap_avoidance(test);
  test_transfer_flags_and_terminal_cleanup(test);
  test_same_file_branch(test);
  test_role_action_operand_extension(test);
  test_role_action_chain_update_gate(test);
  test_change_requested_action_id(test);
  test_wait_for_role_action_position(test);
  test_play_sound_effect_request(test);
  test_turn_role_toward_role(test);
  test_set_role_head_sign_action(test);
  test_set_and_clear_role_wait_override(test);
  if (argument_count == 2) {
    const std::filesystem::path root{arguments[1]};
    test_real_story_248_dialog(test, root);
    test_real_new_game_story_reaches_first_dialog(test, root);
  }
  return test.exit_code();
}
