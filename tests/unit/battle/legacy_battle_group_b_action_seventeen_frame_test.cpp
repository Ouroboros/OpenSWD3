#include "openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <span>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupBActionSeventeenFrameCall;
using openswd3::battle::LegacyBattleGroupBActionSeventeenFrameCallReply;
using openswd3::battle::LegacyBattleGroupBActionSeventeenFrameCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        ++calls;
        if (!ready) {
            return {};
        }

        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes,
        };
    }

    std::array<u8, 2> bytes{0x44U, 0x45U};
    u32 calls{};
    bool ready{};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        source.resize(static_cast<std::size_t>(width) * height * 2U);
        for (std::size_t offset = 0U; offset < source.size(); offset += 2U) {
            source[offset] = 0x34U;
            source[offset + 1U] = 0x12U;
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        ++calls;
        last_resource_id = resource_id;
        last_piece_index = piece_index;
        if (!available) {
            return false;
        }

        piece.source = indexed
            ? openswd3::rendering::LegacyBlitSource{
                  .bytes = std::span<const u8>{source}.first(
                      static_cast<std::size_t>(width) * height
                  ),
                  .layout = openswd3::rendering::LegacyBlitSourceLayout::
                      indexed_8,
                  .palette = palette,
              }
            : openswd3::rendering::LegacyBlitSource{
                  .bytes = source,
                  .layout = openswd3::rendering::LegacyBlitSourceLayout::
                      direct_16,
              };
        piece.width = width;
        piece.height = height;
        return true;
    }

    std::vector<u8> source;
    std::array<u16, 256> palette{};
    u32 calls{};
    u32 last_resource_id{};
    u32 last_piece_index{};
    u16 width{10U};
    u16 height{1U};
    bool available{};
    bool indexed{};
};

class FramePort final
    : public openswd3::battle::LegacyBattleGroupBActionSeventeenFramePort {
public:
    [[nodiscard]] LegacyBattleGroupBActionSeventeenFrameCallReply invoke(
        const LegacyBattleGroupBActionSeventeenFrameCallRequest& request
    ) override {
        calls.push_back(request);
        if (before_reply) {
            before_reply(request);
        }

        if (request.call ==
            LegacyBattleGroupBActionSeventeenFrameCall::play_sample) {
            if (!sample_replies.empty()) {
                const auto reply = sample_replies.front();
                sample_replies.pop_front();
                return reply;
            }
        }
        if (request.call ==
            LegacyBattleGroupBActionSeventeenFrameCall::query_coordinates) {
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
                .outputs = {coordinate_x, coordinate_y},
            };
        }

        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleGroupBActionSeventeenFrameCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls,
            [call](
                const LegacyBattleGroupBActionSeventeenFrameCallRequest& request
            ) { return request.call == call; }
        ));
    }

    [[nodiscard]] const LegacyBattleGroupBActionSeventeenFrameCallRequest* find(
        const LegacyBattleGroupBActionSeventeenFrameCall call,
        const std::size_t occurrence = 0U
    ) const {
        std::size_t seen{};
        for (const auto& request : calls) {
            if (request.call != call) {
                continue;
            }
            if (seen == occurrence) {
                return &request;
            }
            ++seen;
        }

        return nullptr;
    }

    std::function<
        void(const LegacyBattleGroupBActionSeventeenFrameCallRequest&)>
        before_reply;
    std::deque<LegacyBattleGroupBActionSeventeenFrameCallReply> sample_replies;
    std::vector<LegacyBattleGroupBActionSeventeenFrameCallRequest> calls;
    u32 coordinate_x{100U};
    u32 coordinate_y{200U};
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    ActionStreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater updater{stream_provider};
    FrameProvider frame_provider;
    FramePort port;
    openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
    openswd3::battle::LegacyBattleGroupAActionExecutionSharedState shared;

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
    }

    void prepare_unchanged_record() {
        actor.profile_value = 0x1234U;
        actor.turn_action_record.action_id = actor.profile_value;
        actor.turn_action_record.cached_action_id = actor.profile_value;
        actor.turn_action_record.base_variant = 0x24U;
        actor.turn_action_record.cached_base_variant = 0x24U;
        actor.turn_action_record.variant_delta = 0U;
        actor.turn_action_record.cached_variant_delta = 0U;
        actor.turn_action_record.field_4a = 2U;
        actor.turn_action_record.field_4c = 2U;
        actor.turn_action_record.draw_offset_x = 3U;
        actor.turn_action_record.draw_offset_y = 4U;
        actor.turn_action_record.mode_flags = 1U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGroupBActionSeventeenFrameResult
    run(openswd3::battle::LegacyBattleGroupAActionExecutionState* actor_state,
        openswd3::battle::LegacyBattleGroupAActionExecutionSharedState*
            shared_state) {
        return openswd3::battle::
            advance_legacy_battle_group_b_action_seventeen_frame(
                actor_state,
                shared_state,
                port,
                updater,
                frame_provider,
                framebuffer,
                raster,
                request,
                effects,
                jitter,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0x11112222U,
                    .entry_ecx = 0x00525508U,
                    .entry_edx = 0x33334444U,
                }
            );
    }
};

[[nodiscard]] bool action_record_is_zero(
    const openswd3::asset_runtime::LegacyActionRecord& record
) {
    const auto bytes = std::as_bytes(std::span{&record, 1U});
    return std::ranges::all_of(bytes, [](const std::byte value) {
        return value == std::byte{};
    });
}

}  // namespace

void test_battle_group_b_action_seventeen_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBActionSeventeenFrameStatus;

    {
        Fixture fixture;
        const auto result = fixture.run(nullptr, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0x33334444U &&
                fixture.port.calls.empty() &&
                fixture.stream_provider.calls == 0U &&
                fixture.frame_provider.calls == 0U,
            "action seventeen stops at the first actor access"
        );
    }

    {
        Fixture fixture;
        fixture.actor.turn_countdown = 6;
        fixture.actor.turn_action_record.action_id = 0xFFFFFFFFU;
        fixture.actor.turn_action_record.field_94 = 0xFFFFFFFFU;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::completed &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 0x33334444U &&
                result.cleared_action_record_dwords == 0x26U &&
                action_record_is_zero(fixture.actor.turn_action_record) &&
                fixture.port.calls.empty() &&
                fixture.stream_provider.calls == 0U,
            "countdown six clears exactly the action record and returns one"
        );
    }

    {
        Fixture fixture;
        fixture.actor.turn_countdown = -1;
        fixture.actor.turn_action_record.action_id = 0xFFFFFFFFU;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::completed &&
                result.return_eax == 1U && result.return_ecx == 0U &&
                result.return_edx == 0x33334444U &&
                action_record_is_zero(fixture.actor.turn_action_record),
            "negative countdown follows the signed gate and preserves stale EDX"
        );
    }

    {
        Fixture fixture;
        fixture.actor.turn_countdown = 7;
        fixture.actor.profile_value = 0x3456U;
        fixture.actor.special_mode = 1U;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::completed &&
                result.return_eax == 1U && result.action_update_calls == 1U &&
                result.frame_lookup_calls == 0U &&
                fixture.actor.turn_completion_latch == 1U &&
                fixture.actor.turn_action_record.action_id == 0x3456U &&
                fixture.actor.turn_action_record.base_variant == 0x24U &&
                fixture.actor.turn_action_record.external_mode == 0U &&
                !result.return_ecx_known && !result.return_edx_known,
            "failed action stream completes after the updater resets its external mode"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.actor.turn_countdown = 7;
        fixture.actor.special_draw_mirror_mode = 1U;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        frame_owner_typed_stop &&
                result.frame_lookup_calls == 1U &&
                result.coordinate_query_calls == 0U &&
                result.coordinate_publish_calls == 0U &&
                fixture.actor.turn_render_flags == 1U &&
                fixture.actor.turn_target_x_offset == 3U &&
                fixture.actor.turn_countdown == 7 && result.return_eax == 0U &&
                result.return_ecx == 1U && result.return_ecx_known &&
                !result.return_edx_known,
            "mirrored width adjustment stops before coordinate calls when the frame is absent"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.actor.turn_countdown = 7;
        fixture.actor.special_draw_mirror_mode = 1U;
        fixture.actor.turn_action_record.draw_offset_x = 0x00010000U;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        frame_owner_typed_stop &&
                result.coordinate_query_calls == 0U &&
                fixture.actor.turn_target_x_offset == 0U &&
                result.return_eax == 0U,
            "mirrored width access uses the full draw-offset gate before its low-word subtraction"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.actor.turn_countdown = 7;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        const auto* query = fixture.port.find(
            LegacyBattleGroupBActionSeventeenFrameCall::query_coordinates
        );
        const auto* publish = fixture.port.find(
            LegacyBattleGroupBActionSeventeenFrameCall::publish_coordinates
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        frame_owner_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_publish_calls == 1U && query != nullptr &&
                query->eax == 0U && query->ecx == 0x00525508U &&
                query->edx == 1U && query->arguments[0U] == 0U &&
                query->arguments[1U] == 1U &&
                result.adjusted_coordinate_x == 75U && publish != nullptr &&
                publish->arguments[0U] == 75U &&
                publish->arguments[1U] == 200U &&
                fixture.actor.turn_countdown == 7 && result.render_calls == 0U,
            "nonmirrored missing frame preserves both coordinate side effects before stopping"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.actor.turn_action_record.field_4c = 1U;
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        fixture.frame_provider.indexed = true;
        std::fill(
            fixture.frame_provider.source.begin(),
            fixture.frame_provider.source.end(),
            1U
        );
        fixture.frame_provider.palette[1U] = 0x5678U;
        fixture.actor.turn_action_record.mode_flags = 0U;
        fixture.actor.turn_countdown = 0x0F;
        fixture.actor.special_draw_mirror_mode = 1U;
        fixture.actor.position_x = 20U;
        fixture.actor.position_y = 10U;
        fixture.port.coordinate_x = 50U;
        fixture.port.coordinate_y = 60U;
        fixture.actor.turn_action_record.field_58 = 0xA55AU;
        fixture.actor.turn_action_record.field_5a = 0xC33CU;
        std::vector<u16> sample_observations;
        fixture.port.before_reply =
            [&](const LegacyBattleGroupBActionSeventeenFrameCallRequest& call) {
                if (call.call ==
                        LegacyBattleGroupBActionSeventeenFrameCall::
                            play_sample &&
                    call.arguments[0U] == 0x2FU) {
                    sample_observations.push_back(
                        fixture.actor.turn_action_record.field_58
                    );
                    fixture.actor.turn_action_record.field_58 = 0x8001U;
                } else if (
                    call.call ==
                    LegacyBattleGroupBActionSeventeenFrameCall::set_sample_pan
                ) {
                    sample_observations.push_back(
                        fixture.actor.turn_action_record.field_58
                    );
                    fixture.actor.turn_action_record.field_58 = 0xFFFFU;
                } else if (
                    call.call ==
                    LegacyBattleGroupBActionSeventeenFrameCall::
                        query_coordinates
                ) {
                    sample_observations.push_back(
                        fixture.actor.turn_action_record.field_58
                    );
                }
            };
        fixture.port.sample_replies.push_back({
            .eax = 0x11110000U,
            .ecx = 0xAAAA1111U,
            .edx = 0xBBBB2222U,
        });
        fixture.port.sample_replies.push_back({
            .eax = 0x22220000U,
            .ecx = 0xABCD1234U,
            .edx = 0xDCBA5678U,
        });
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        const auto* first_sample = fixture.port.find(
            LegacyBattleGroupBActionSeventeenFrameCall::play_sample, 0U
        );
        const auto* second_sample = fixture.port.find(
            LegacyBattleGroupBActionSeventeenFrameCall::play_sample, 1U
        );
        const auto* pan = fixture.port.find(
            LegacyBattleGroupBActionSeventeenFrameCall::set_sample_pan
        );
        const auto pixels = fixture.framebuffer.physical_pixels();
        const std::size_t rendered_index = 6U * 640U + 13U;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::completed &&
                result.return_eax == 0U && result.sample_play_calls == 2U &&
                result.sample_pan_calls == 1U && first_sample != nullptr &&
                first_sample->arguments[0U] == 0x10FU &&
                second_sample != nullptr &&
                second_sample->arguments[0U] == 0x2FU && pan != nullptr &&
                pan->eax == 1U && pan->arguments[0U] == 0xABCD8001U &&
                pan->ecx == 0xABCD8001U && pan->edx == 0xDCBA5678U &&
                pan->arguments[1U] == 0x10U &&
                fixture.actor.turn_action_record.field_58 == 0U &&
                fixture.actor.turn_action_record.field_5a == 0xC33CU &&
                sample_observations == std::vector<u16>{0x2FU, 0x8001U, 0U},
            "countdown fifteen preserves both sample calls and stale pan registers"
        );
        test.expect_true(
            fixture.actor.turn_render_flags == 0U &&
                fixture.actor.turn_target_x_offset == 7U &&
                result.adjusted_coordinate_x == 75U &&
                fixture.shared.turn_frame_source_token == 0x00527A54U &&
                fixture.frame_provider.last_resource_id == 2U &&
                fixture.frame_provider.last_piece_index == 1U,
            "countdown fifteen preserves mirror, coordinate, and frame lookup state"
        );
        test.expect_true(
            result.render_calls == 1U &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::completed &&
                rendered_index < pixels.size() &&
                pixels[rendered_index] == 0x5678U &&
                fixture.actor.turn_countdown == 14,
            "countdown fifteen draws the indexed frame before decrementing"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        fixture.actor.turn_countdown = 0x0F;
        fixture.actor.position_x = 20U;
        fixture.actor.position_y = 10U;
        fixture.port.sample_replies.push_back({
            .eax = 0x22220000U,
            .ecx = 0xABCD1234U,
            .edx = 0xDCBA5678U,
        });
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        const auto* pan = fixture.port.find(
            LegacyBattleGroupBActionSeventeenFrameCall::set_sample_pan
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::completed &&
                result.sample_play_calls == 1U &&
                result.sample_pan_calls == 1U && pan != nullptr &&
                pan->eax == 0U && pan->arguments[0U] == 0xDCBA002FU &&
                pan->arguments[1U] == 0xFFFFFFF0U &&
                fixture.actor.turn_render_flags == 0U &&
                fixture.actor.turn_countdown == 14,
            "nonmirrored completion sample preserves the stale EDX high word"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        fixture.frame_provider.source.clear();
        fixture.actor.turn_countdown = 7;
        fixture.request.target_height = 5;
        fixture.effects.red_offset = 7;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        blit_typed_stop &&
                result.render_calls == 1U &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        malformed_source &&
                fixture.request.target_height == 5 &&
                fixture.effects.red_offset == 7 &&
                fixture.actor.turn_countdown == 7,
            "blitter failure preserves shared scratch and blocks the decrement"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        fixture.actor.turn_countdown = 7;
        const auto result = fixture.run(&fixture.actor, nullptr);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        shared_state_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_publish_calls == 1U &&
                result.render_calls == 0U && fixture.actor.turn_countdown == 7,
            "missing shared frame source stops after coordinate publication"
        );
    }
}
