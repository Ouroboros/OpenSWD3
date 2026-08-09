#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_role_particle_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::kLegacyAniRoleParticleActionId;
using openswd3::asset_runtime::kLegacyAniRoleParticleEmitterCount;
using openswd3::asset_runtime::kLegacyAniRoleParticleSpecialMapId;
using openswd3::asset_runtime::kLegacyAniRoleParticleVariant;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::asset_runtime::LegacyAniRoleParticleEffect;
using openswd3::asset_runtime::LegacyAniRoleParticleEmitter;
using openswd3::asset_runtime::LegacyAniRoleParticleNode;
using openswd3::asset_runtime::LegacyAniRoleParticlePorts;
using openswd3::asset_runtime::LegacyAniRoleParticlePositionPort;
using openswd3::asset_runtime::LegacyAniRoleParticleRuntimePorts;
using openswd3::asset_runtime::LegacyAniRoleParticleStatus;
using openswd3::asset_runtime::LegacyAniRoleParticleViewport;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::initialize_legacy_raster_geometry;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

constexpr LegacyAniRoleParticleViewport kViewport{
    .left = 0,
    .top = 0,
    .right = 640,
    .bottom = 480,
};

struct DrawCall {
  i32 x{};
  i32 y{};
  u32 flags{};
  i32 red_offset{};
  i32 green_offset{};
  i32 blue_offset{};
};

class FakePositions final : public LegacyAniRoleParticlePositionPort {
public:
  [[nodiscard]] bool resolve_role_position(const u16 role_selector,
                                           i16 &world_x,
                                           i16 &world_y) override {
    ++call_count;
    selectors.push_back(role_selector);
    if (!available) {
      return false;
    }
    world_x = x;
    world_y = y;
    return true;
  }

  bool available{true};
  i16 x{100};
  i16 y{200};
  u32 call_count{};
  std::vector<u16> selectors;
};

class FakePorts final : public LegacyAniRoleParticlePorts {
public:
  [[nodiscard]] LegacyActionUpdateStatus
  update_action_record(LegacyActionRecord &record) override {
    ++update_count;
    action_ids.push_back(record.action_id);
    variants.push_back(record.base_variant);
    if (fail_update) {
      return LegacyActionUpdateStatus::stream_load_failed;
    }
    record.draw_offset_x = 10U;
    record.draw_offset_y = 20U;
    record.mode_flags = 4U;
    record.field_4a = 123U;
    record.field_4c = 456U;
    return LegacyActionUpdateStatus::completed;
  }

  [[nodiscard]] bool load_frame_piece(const u16 resource_id,
                                      const u16 frame_index,
                                      LegacyFramePiece &piece) override {
    ++load_count;
    loads.emplace_back(resource_id, frame_index);
    if (fail_load) {
      return false;
    }
    piece.width = 8U;
    piece.height = 6U;
    return true;
  }

  [[nodiscard]] LegacyBlitExecutionStatus
  draw_frame_piece(const LegacyFramePiece &, const i32 destination_x,
                   const i32 destination_y, const u32 flags,
                   const i32 red_offset, const i32 green_offset,
                   const i32 blue_offset) noexcept override {
    draws.push_back(DrawCall{
        .x = destination_x,
        .y = destination_y,
        .flags = flags,
        .red_offset = red_offset,
        .green_offset = green_offset,
        .blue_offset = blue_offset,
    });
    if (draws.size() - 1U == failed_draw_call) {
      return LegacyBlitExecutionStatus::malformed_source;
    }
    return LegacyBlitExecutionStatus::completed;
  }

  bool fail_update{};
  bool fail_load{};
  u32 update_count{};
  u32 load_count{};
  std::size_t failed_draw_call{std::numeric_limits<std::size_t>::max()};
  std::vector<u32> action_ids;
  std::vector<u32> variants;
  std::vector<std::pair<u16, u16>> loads;
  std::vector<DrawCall> draws;
};

[[nodiscard]] LegacyActionRecord make_action_record() {
  LegacyActionRecord record{};
  initialize_legacy_action_record(record);
  return record;
}

void test_physical_layout_pool_and_reset(openswd3::test::Context &test) {
  test.expect_equal(sizeof(LegacyAniRoleParticleNode), std::size_t{0x10U},
                    "particle nodes retain the original 0x10-byte fields");
  test.expect_equal(sizeof(LegacyAniRoleParticleEmitter), std::size_t{0x10U},
                    "emitter records retain the original 0x10-byte fields");

  LegacyAniRoleParticleEffect effect;
  test.expect_equal(effect.emitters().size(),
                    kLegacyAniRoleParticleEmitterCount,
                    "the owner contains exactly four physical emitters");
  test.expect_true(
      std::ranges::all_of(effect.emitters(),
                          [](const LegacyAniRoleParticleEmitter &emitter) {
                            return emitter.head_token == 0U &&
                                   emitter.role_selector == 0;
                          }),
      "loader-zero startup leaves all four emitters empty");

  const u32 first = effect.nodes().allocate_zeroed();
  const u32 second = effect.nodes().allocate_zeroed();
  test.expect_equal(first, u32{1U}, "the token pool reserves zero for null");
  test.expect_equal(second, u32{2U}, "tokens are stable one-based indices");
  effect.emitters()[0U].head_token = second;
  effect.emitters()[0U].role_selector = 7;
  effect.nodes().node(second)->next_token = first;
  effect.nodes().node(first)->lifetime = 9;

  effect.reset();
  test.expect_equal(effect.nodes().active_count(), std::size_t{0U},
                    "reset releases every allocated node");
  test.expect_true(std::ranges::all_of(
                       effect.emitters(),
                       [](const LegacyAniRoleParticleEmitter &emitter) {
                         return emitter.head_token == 0U &&
                                emitter.world_x == 0 && emitter.world_y == 0 &&
                                emitter.field_08 == 0 && emitter.flags == 0 &&
                                emitter.role_selector == 0 &&
                                emitter.reserved == 0;
                       }),
                   "reset zeros the full four-record 0x40-byte state");
}

void test_cull_and_pre_loop_order(openswd3::test::Context &test) {
  const auto expect_culled = [&test](const i32 x, const i32 y,
                                     const char *message) {
    LegacyAniRoleParticleEffect effect;
    effect.emitters()[0U].role_selector = 7;
    LegacySecondaryRng random;
    random.seed(39U);
    FakePositions positions;
    FakePorts ports;
    LegacyActionRecord action = make_action_record();
    const auto result =
        effect.update(x, y, 7, 1, kViewport, random, positions, action, ports);
    test.expect_equal(result.status, LegacyAniRoleParticleStatus::culled,
                      message);
    test.expect_equal(random.index(), std::size_t{0U},
                      "early cull consumes no random value");
    test.expect_equal(ports.update_count, u32{0U},
                      "early cull performs no action update");
    test.expect_equal(positions.call_count, u32{0U},
                      "early cull performs no role lookup");
  };
  expect_culled(-64, 100, "the left margin is inclusive");
  expect_culled(704, 100, "the right margin is inclusive");
  expect_culled(100, -64, "the top margin is inclusive");
  expect_culled(100, 544, "the bottom margin is inclusive");

  LegacyAniRoleParticleEffect effect;
  effect.emitters()[0U].role_selector = 7;
  FakePositions positions;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();

  LegacySecondaryRng inside_random;
  inside_random.seed(39U);
  const auto no_match = effect.update(-63, 100, 8, 1, kViewport, inside_random,
                                      positions, action, ports);
  test.expect_equal(no_match.status, LegacyAniRoleParticleStatus::ready,
                    "the coordinate immediately inside the margin proceeds");
  test.expect_equal(no_match.random_call_count, u32{1U},
                    "the frame drift is consumed before scanning emitters");
  test.expect_equal(inside_random.index(), std::size_t{2U},
                    "one bounded call advances the secondary RNG twice");
  test.expect_equal(
      ports.update_count, u32{1U},
      "the shared action updates even without a matching emitter");
  test.expect_equal(ports.load_count, u32{1U},
                    "the shared TSW frame loads before emitter matching");
  test.expect_equal(positions.call_count, u32{0U},
                    "nonmatching emitters do not resolve role state");

  LegacyAniRoleParticleEffect signed_selector;
  signed_selector.emitters()[0U].role_selector = -32767;
  LegacySecondaryRng signed_random;
  signed_random.seed(39U);
  FakePositions signed_positions;
  FakePorts signed_ports;
  const auto signed_result = signed_selector.update(
      100, 100, u16{0x8001U}, 1, kViewport, signed_random, signed_positions,
      action, signed_ports);
  test.expect_equal(
      signed_result.matching_emitter_count, u32{0U},
      "signed slot selector does not equal zero-extended caller selector");
  test.expect_equal(signed_positions.call_count, u32{0U},
                    "the high-bit selector mismatch reaches no role lookup");
}

void test_four_emitters_spawn_rng_and_common_fade(
    openswd3::test::Context &test) {
  LegacyAniRoleParticleEffect effect;
  for (LegacyAniRoleParticleEmitter &emitter : effect.emitters()) {
    emitter.role_selector = 7;
    emitter.flags = 0x7FFE;
  }
  LegacySecondaryRng random;
  random.seed(39U);
  FakePositions positions;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();

  const auto result = effect.update(100, 200, 7, 1, kViewport, random,
                                    positions, action, ports);
  test.expect_equal(result.status, LegacyAniRoleParticleStatus::ready,
                    "the four-emitter update completes");
  test.expect_equal(result.matching_emitter_count, u32{4U},
                    "all four duplicate selectors are processed");
  test.expect_equal(result.role_query_count, u32{4U},
                    "the role index and coordinates are resolved per emitter");
  test.expect_equal(result.spawned_node_count, u32{2U},
                    "strict probability rolls spawn the first and third nodes");
  test.expect_equal(result.updated_node_count, u32{2U},
                    "new nodes update in their creation frame");
  test.expect_equal(result.draw_count, u32{2U},
                    "new nodes draw in their creation frame");
  test.expect_equal(
      result.random_call_count, u32{15U},
      "drift, four gates, two creations and two updates consume 15 calls");
  test.expect_equal(random.index(), std::size_t{30U},
                    "the fixed vector consumes 30 accepted raw state words");
  test.expect_equal(effect.nodes().active_count(), std::size_t{2U},
                    "only the two successful creation gates allocate nodes");

  test.expect_equal(action.action_id, kLegacyAniRoleParticleActionId,
                    "the shared record uses action 0x232b");
  test.expect_equal(action.base_variant, kLegacyAniRoleParticleVariant,
                    "the shared record uses base variant 59");
  test.expect_equal(ports.loads[0U], std::pair<u16, u16>{123U, 456U},
                    "updated +0x4a/+0x4c form the single TSW request");
  test.expect_equal(positions.selectors, std::vector<u16>{7U, 7U, 7U, 7U},
                    "each matching emitter passes the caller selector");

  test.expect_equal(effect.emitters()[0U].flags, i16{0x7FFF},
                    "the successful gate sets only flag bit zero");
  test.expect_equal(effect.emitters()[1U].flags, i16{0x7FFE},
                    "the failed gate clears only flag bit zero");
  test.expect_equal(effect.emitters()[2U].flags, i16{0x7FFF},
                    "the third gate sets only flag bit zero");
  test.expect_equal(effect.emitters()[3U].flags, i16{0x7FFE},
                    "the fourth gate preserves all upper flag bits");

  const LegacyAniRoleParticleNode *const first =
      effect.nodes().node(effect.emitters()[0U].head_token);
  const LegacyAniRoleParticleNode *const third =
      effect.nodes().node(effect.emitters()[2U].head_token);
  test.expect_equal(
      first->fixed_x_1_16, i16{1527},
      "spawn x and horizontal step use 1/16-pixel wrapping state");
  test.expect_equal(first->world_y, i16{199},
                    "the new node moves upward before its first draw");
  test.expect_equal(first->horizontal_step_1_16, i16{7},
                    "the first horizontal step retains the creation RNG value");
  test.expect_equal(first->lifetime, i16{35},
                    "the first creation-frame probability decrements lifetime");
  test.expect_equal(third->fixed_x_1_16, i16{1502},
                    "the third node applies its own creation x and step");
  test.expect_equal(third->lifetime, i16{41},
                    "the third lifetime follows its independent RNG sequence");

  test.expect_equal(
      ports.draws[0U].x, i32{85},
      "fixed x truncates toward zero before action and camera offsets");
  test.expect_equal(ports.draws[0U].y, i32{179},
                    "world y subtracts the shared action offset");
  test.expect_equal(ports.draws[0U].red_offset, i32{-13},
                    "ordinary maps use lifetime arithmetic half minus 30");
  test.expect_equal(ports.draws[0U].green_offset, i32{-13},
                    "ordinary maps publish the same green offset");
  test.expect_equal(ports.draws[0U].blue_offset, i32{-13},
                    "ordinary maps publish the same blue offset");
  test.expect_equal(ports.draws[1U].x, i32{83},
                    "the third emitter draws after its own update");
  test.expect_equal(ports.draws[1U].red_offset, i32{-10},
                    "the third lifetime produces its exact common fade");
}

void test_special_map_fade_and_ignored_blit(openswd3::test::Context &test) {
  LegacyAniRoleParticleEffect effect;
  effect.emitters()[0U].role_selector = 7;
  for (std::size_t index = 1U; index < effect.emitters().size(); ++index) {
    effect.emitters()[index].role_selector = 8;
  }
  LegacySecondaryRng random;
  random.seed(39U);
  FakePositions positions;
  FakePorts ports;
  ports.failed_draw_call = 0U;
  LegacyActionRecord action = make_action_record();

  const auto result =
      effect.update(100, 200, 7, kLegacyAniRoleParticleSpecialMapId, kViewport,
                    random, positions, action, ports);
  test.expect_equal(result.status, LegacyAniRoleParticleStatus::ready,
                    "the original ignores the blitter return value");
  test.expect_equal(result.blit_failure_count, u32{1U},
                    "an ignored blitter failure remains observable");
  test.expect_equal(result.random_call_count, u32{7U},
                    "one special-map creation consumes seven bounded calls");
  const LegacyAniRoleParticleNode *const node =
      effect.nodes().node(effect.emitters()[0U].head_token);
  test.expect_equal(node->lifetime, i16{15},
                    "special map lifetime starts in 16..31 then updates");
  test.expect_equal(ports.draws[0U].red_offset, i32{-15},
                    "special map red offset is lifetime minus 30");
  test.expect_equal(ports.draws[0U].green_offset, i32{-23},
                    "special map green offset is lifetime minus 38");
  test.expect_equal(ports.draws[0U].blue_offset, i32{-35},
                    "special map blue offset is lifetime minus 50");
}

void test_signed_word_wrap_and_draw_conversion(openswd3::test::Context &test) {
  LegacyAniRoleParticleEffect effect;
  LegacyAniRoleParticleEmitter &emitter = effect.emitters()[0U];
  emitter.role_selector = 7;
  emitter.field_08 = 24;
  for (std::size_t index = 1U; index < effect.emitters().size(); ++index) {
    effect.emitters()[index].role_selector = 8;
  }

  const u32 token = effect.nodes().allocate_zeroed();
  emitter.head_token = token;
  LegacyAniRoleParticleNode *const node = effect.nodes().node(token);
  node->fixed_x_1_16 = std::numeric_limits<i16>::max();
  node->world_y = std::numeric_limits<i16>::min();
  node->horizontal_step_1_16 = 1;
  node->vertical_step = -1;
  node->lifetime = 10;

  LegacySecondaryRng random;
  random.seed(39U);
  FakePositions positions;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();
  const auto result = effect.update(100, 200, 7, 1, kViewport, random,
                                    positions, action, ports);

  test.expect_equal(result.status, LegacyAniRoleParticleStatus::ready,
                    "signed-word boundary update completes");
  test.expect_equal(node->fixed_x_1_16, std::numeric_limits<i16>::min(),
                    "fixed x wraps from INT16_MAX to INT16_MIN");
  test.expect_equal(node->world_y, std::numeric_limits<i16>::max(),
                    "world y wraps from INT16_MIN to INT16_MAX");
  test.expect_equal(node->lifetime, i16{9},
                    "the fixed node roll decrements lifetime after wrapping");
  test.expect_equal(ports.draws.size(), std::size_t{1U},
                    "the wrapped live node still draws once");
  test.expect_equal(ports.draws[0U].x, i32{-2058},
                    "negative fixed x divides toward zero before offset");
  test.expect_equal(ports.draws[0U].y, i32{32747},
                    "wrapped y is sign-extended before the action offset");
}

void test_delete_by_copy_and_tail_release(openswd3::test::Context &test) {
  LegacyAniRoleParticleEffect effect;
  LegacyAniRoleParticleEmitter &emitter = effect.emitters()[0U];
  emitter.role_selector = 7;
  emitter.field_08 = 24;
  for (std::size_t index = 1U; index < effect.emitters().size(); ++index) {
    effect.emitters()[index].role_selector = 8;
  }

  const u32 successor_token = effect.nodes().allocate_zeroed();
  const u32 current_token = effect.nodes().allocate_zeroed();
  emitter.head_token = current_token;
  LegacyAniRoleParticleNode *const current = effect.nodes().node(current_token);
  current->next_token = successor_token;
  current->lifetime = 0;
  LegacyAniRoleParticleNode *const successor =
      effect.nodes().node(successor_token);
  successor->fixed_x_1_16 = 160;
  successor->world_y = 100;
  successor->horizontal_step_1_16 = 2;
  successor->vertical_step = -1;
  successor->lifetime = 10;
  successor->reserved = 0x1234;

  LegacySecondaryRng random;
  random.seed(39U);
  FakePositions positions;
  FakePorts ports;
  LegacyActionRecord action = make_action_record();
  const auto copied = effect.update(100, 200, 7, 1, kViewport, random,
                                    positions, action, ports);
  test.expect_equal(copied.updated_node_count, u32{2U},
                    "the expired node and copied successor are both visited");
  test.expect_equal(copied.removed_node_count, u32{1U},
                    "copy deletion releases exactly the successor allocation");
  test.expect_equal(copied.copied_successor_count, u32{1U},
                    "non-tail expiry uses the original copy-successor path");
  test.expect_equal(
      effect.nodes().active_count(), std::size_t{1U},
      "the current allocation survives after copying its successor");
  test.expect_equal(emitter.head_token, current_token,
                    "the link still names the original current allocation");
  const LegacyAniRoleParticleNode *const replacement =
      effect.nodes().node(current_token);
  test.expect_equal(replacement->fixed_x_1_16, i16{162},
                    "the copied successor is reprocessed in the same frame");
  test.expect_equal(replacement->world_y, i16{99},
                    "the copied successor receives its vertical update");
  test.expect_equal(replacement->lifetime, i16{9},
                    "the copied successor consumes the next lifetime roll");
  test.expect_equal(replacement->reserved, i16{0x1234},
                    "the full 0x10-byte successor record is copied");
  test.expect_equal(ports.draws.size(), std::size_t{1U},
                    "only the replacement node is drawn");

  effect.nodes().node(current_token)->lifetime = 0;
  LegacySecondaryRng tail_random;
  tail_random.seed(39U);
  const auto tail = effect.update(100, 200, 7, 1, kViewport, tail_random,
                                  positions, action, ports);
  test.expect_equal(tail.removed_node_count, u32{1U},
                    "an expired tail releases the current allocation");
  test.expect_equal(tail.copied_successor_count, u32{0U},
                    "tail expiry does not use successor copying");
  test.expect_equal(emitter.head_token, u32{0U},
                    "tail expiry clears the pointer-to-pointer link");
  test.expect_equal(effect.nodes().active_count(), std::size_t{0U},
                    "the list is empty after tail deletion");
}

void test_modern_failure_guards(openswd3::test::Context &test) {
  LegacyAniRoleParticleEffect effect;
  effect.emitters()[0U].role_selector = 7;
  for (std::size_t index = 1U; index < effect.emitters().size(); ++index) {
    effect.emitters()[index].role_selector = 8;
  }
  LegacyActionRecord action = make_action_record();
  FakePositions positions;

  LegacySecondaryRng update_random;
  update_random.seed(39U);
  FakePorts update_ports;
  update_ports.fail_update = true;
  const auto update_failure =
      effect.update(100, 200, 7, 1, kViewport, update_random, positions, action,
                    update_ports);
  test.expect_equal(update_failure.status,
                    LegacyAniRoleParticleStatus::action_update_failed,
                    "an unavailable ACT stream is isolated at the modern port");
  test.expect_equal(update_failure.random_call_count, u32{1U},
                    "the original frame drift precedes the action update");

  LegacySecondaryRng load_random;
  load_random.seed(39U);
  FakePorts load_ports;
  load_ports.fail_load = true;
  const auto load_failure = effect.update(
      100, 200, 7, 1, kViewport, load_random, positions, action, load_ports);
  test.expect_equal(load_failure.status,
                    LegacyAniRoleParticleStatus::frame_load_failed,
                    "an unavailable TSW frame is isolated before emitter scan");

  LegacySecondaryRng role_random;
  role_random.seed(39U);
  FakePorts role_ports;
  positions.available = false;
  const auto role_failure = effect.update(
      100, 200, 7, 1, kViewport, role_random, positions, action, role_ports);
  test.expect_equal(
      role_failure.status, LegacyAniRoleParticleStatus::role_lookup_failed,
      "an invalid world role index does not become host OOB access");
  test.expect_equal(role_failure.random_call_count, u32{1U},
                    "role lookup fails before the emitter probability roll");

  LegacyAniRoleParticleEffect corrupt;
  corrupt.emitters()[0U].role_selector = 7;
  corrupt.emitters()[0U].field_08 = 24;
  corrupt.emitters()[0U].head_token = 99U;
  for (std::size_t index = 1U; index < corrupt.emitters().size(); ++index) {
    corrupt.emitters()[index].role_selector = 8;
  }
  positions.available = true;
  LegacySecondaryRng corrupt_random;
  corrupt_random.seed(39U);
  FakePorts corrupt_ports;
  const auto corrupt_result =
      corrupt.update(100, 200, 7, 1, kViewport, corrupt_random, positions,
                     action, corrupt_ports);
  test.expect_equal(
      corrupt_result.status, LegacyAniRoleParticleStatus::corrupt_node_link,
      "an invalid 32-bit link token is isolated before dereference");
}

void test_real_act_tsw_and_blitter(openswd3::test::Context &test,
                                   const std::filesystem::path &data_root) {
  LegacyActRuntime act_runtime{data_root};
  act_runtime.set_cache_limit(0x00080000U);
  LegacyActActionStreamProvider stream_provider{act_runtime};
  LegacyActionUpdater action_updater{stream_provider};
  LegacyTswRuntime tsw_runtime{data_root};
  tsw_runtime.set_cache_limit(0x01000000U);
  LegacyFramebuffer framebuffer;
  LegacyRasterGeometryState raster;
  test.expect_true(
      initialize_legacy_raster_geometry(raster, LegacySurfaceGeometry{}),
      "real role-particle raster initializes");
  LegacyBlitEffectState effects;
  LegacyRleRowJitterState jitter;
  LegacyAniRoleParticleRuntimePorts ports{
      action_updater, tsw_runtime, framebuffer, raster, effects, jitter,
  };
  LegacyAniRoleParticleEffect effect;
  effect.emitters()[0U].role_selector = 7;
  for (std::size_t index = 1U; index < effect.emitters().size(); ++index) {
    effect.emitters()[index].role_selector = 8;
  }
  LegacyActionRecord action = make_action_record();
  LegacySecondaryRng random;
  random.seed(39U);
  FakePositions positions;
  positions.x = 320;
  positions.y = 240;

  const auto result = effect.update(320, 240, 7, 1, kViewport, random,
                                    positions, action, ports);
  test.expect_equal(result.status, LegacyAniRoleParticleStatus::ready,
                    "real variant 59 resolves through ACT and TSW");
  test.expect_equal(result.spawned_node_count, u32{1U},
                    "the fixed real vector creates one particle");
  test.expect_equal(result.draw_count, u32{1U},
                    "the fixed real vector submits one frame");
  test.expect_equal(result.blit_failure_count, u32{0U},
                    "the real frame selects a supported blitter path");
  test.expect_true(
      std::ranges::any_of(framebuffer.physical_pixels(),
                          [](const u16 pixel) { return pixel != 0U; }),
      "real action and TSW data produce nonempty framebuffer pixels");
  test.expect_equal(
      openswd3::rendering::legacy_framebuffer_logical_fnv1a64(framebuffer),
      std::uint64_t{0xFA22737232A60CF6ULL},
      "the real variant-59 framebuffer vector is stable");
}

} // namespace

int main(const int argument_count, char **arguments) {
  openswd3::test::Context test;
  test_physical_layout_pool_and_reset(test);
  test_cull_and_pre_loop_order(test);
  test_four_emitters_spawn_rng_and_common_fade(test);
  test_special_map_fade_and_ignored_blit(test);
  test_signed_word_wrap_and_draw_conversion(test);
  test_delete_by_copy_and_tail_release(test);
  test_modern_failure_guards(test);
  if (argument_count == 2) {
    test_real_act_tsw_and_blitter(test, std::filesystem::path{arguments[1]});
  }
  return test.exit_code();
}
