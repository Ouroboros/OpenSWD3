#include "openswd3/battle/legacy_battle_group_b_action_seventeen_frame.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
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
        if (request.call ==
            LegacyBattleGroupBActionSeventeenFrameCall::play_sample) {
            if (!sample_replies.empty()) {
                const auto reply = sample_replies.front();
                sample_replies.pop_front();
                return reply;
            }
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

    std::deque<LegacyBattleGroupBActionSeventeenFrameCallReply> sample_replies;
    std::vector<LegacyBattleGroupBActionSeventeenFrameCallRequest> calls;
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
    openswd3::battle::LegacyBattleGroupBActionSeventeenFrameRequest
        frame_request{
            .actor_token = 0x00525508U,
            .coordinate_output_x_token = 0x11110002U,
            .coordinate_output_y_token = 0x33330004U,
            .entry_eax = 0x11112222U,
            .entry_ecx = 0x00525508U,
            .entry_edx = 0x33334444U,
        };

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
        actor.position_x = 100U;
        actor.position_y = 200U;
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
                frame_request
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
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        frame_owner_typed_stop &&
                result.port_calls == 0U &&
                result.coordinate_query_calls == 1U &&
                result.current_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorCurrentCoordinateQueryStatus::
                            completed &&
                result.current_coordinate_query.return_eax == 0x111100C8U &&
                result.current_coordinate_query.return_ecx == 0x33330004U &&
                result.current_coordinate_query.return_edx == 0x11110002U &&
                result.current_coordinate_query.flags.carry &&
                result.current_coordinate_query.flags.parity &&
                result.current_coordinate_query.flags.auxiliary_carry &&
                !result.current_coordinate_query.flags.zero &&
                result.current_coordinate_query.flags.sign &&
                !result.current_coordinate_query.flags.overflow &&
                result.coordinate_publish_calls == 1U &&
                result.adjusted_coordinate_x == 75U &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            completed &&
                result.coordinate_publication.argument_x == 75U &&
                result.coordinate_publication.argument_y == 200U &&
                !result.coordinate_publication.flags.carry &&
                result.coordinate_publication.flags.parity &&
                result.coordinate_publication.flags.auxiliary_carry &&
                !result.coordinate_publication.flags.zero &&
                !result.coordinate_publication.flags.sign &&
                !result.coordinate_publication.flags.overflow &&
                fixture.actor.position_x == 75U &&
                fixture.actor.position_y == 200U &&
                fixture.actor.alternate_position_x == 75U &&
                fixture.actor.alternate_position_y == 200U &&
                fixture.port.count(
                    LegacyBattleGroupBActionSeventeenFrameCall::
                        reserved_actor_current_coordinate_query
                ) == 0U &&
                fixture.port.count(
                    LegacyBattleGroupBActionSeventeenFrameCall::
                        reserved_actor_coordinate_publication
                ) == 0U &&
                fixture.actor.turn_countdown == 7 && result.render_calls == 0U,
            "nonmirrored missing frame preserves typed publication before stopping"
        );
    }

    {
        using QueryStatus =
            openswd3::battle::LegacyBattleActorCurrentCoordinateQueryStatus;
        const std::array expected_statuses{
            QueryStatus::first_output_pointer_read_typed_stop,
            QueryStatus::position_x_read_typed_stop,
            QueryStatus::first_output_write_typed_stop,
            QueryStatus::position_y_read_typed_stop,
            QueryStatus::second_output_pointer_read_typed_stop,
            QueryStatus::second_output_write_typed_stop,
        };
        for (std::size_t stage = 0U; stage < expected_statuses.size();
             ++stage) {
            Fixture fixture;
            fixture.prepare_unchanged_record();
            fixture.stream_provider.ready = true;
            fixture.frame_provider.available = true;
            fixture.actor.turn_countdown = 7;
            fixture.frame_request.coordinate_x_initial = 0xAAAA0000U;
            fixture.frame_request.coordinate_y_initial = 0xBBBB0000U;
            if (stage == 0U) {
                fixture.frame_request.current_coordinate_access
                    .first_output_pointer_readable = false;
            } else if (stage == 1U) {
                fixture.actor.position_x_read_accessible = false;
            } else if (stage == 2U) {
                fixture.frame_request.current_coordinate_access
                    .first_output_writable = false;
            } else if (stage == 3U) {
                fixture.actor.position_y_read_accessible = false;
            } else if (stage == 4U) {
                fixture.frame_request.current_coordinate_access
                    .second_output_pointer_readable = false;
            } else if (stage == 5U) {
                fixture.frame_request.current_coordinate_access
                    .second_output_writable = false;
            }
            const auto result = fixture.run(&fixture.actor, &fixture.shared);
            const u32 expected_eax = stage < 2U
                ? 0x11110002U
                : (stage < 4U ? 0x11110064U : 0x111100C8U);
            const u32 expected_ecx = stage < 5U ? 0x00525508U : 0x33330004U;
            const u32 expected_edx = stage == 0U ? 0x33330004U : 0x11110002U;
            test.expect_true(
                result.status ==
                        LegacyBattleGroupBActionSeventeenFrameStatus::
                            actor_current_coordinate_typed_stop &&
                    result.current_coordinate_query.status ==
                        expected_statuses[stage] &&
                    result.current_coordinate_query.output_writes ==
                        (stage >= 3U ? 1U : 0U) &&
                    result.current_coordinate_query.return_eax ==
                        expected_eax &&
                    result.current_coordinate_query.return_ecx ==
                        expected_ecx &&
                    result.current_coordinate_query.return_edx ==
                        expected_edx &&
                    result.current_coordinate_query.flags.carry &&
                    result.current_coordinate_query.flags.parity &&
                    result.current_coordinate_query.flags.auxiliary_carry &&
                    !result.current_coordinate_query.flags.zero &&
                    result.current_coordinate_query.flags.sign &&
                    !result.current_coordinate_query.flags.overflow &&
                    result.coordinate_x ==
                        (stage >= 3U ? 0xAAAA0064U : 0xAAAA0000U) &&
                    result.coordinate_y == 0xBBBB0000U &&
                    result.coordinate_publish_calls == 0U &&
                    result.render_calls == 0U &&
                    fixture.shared.turn_frame_source_token == 0U &&
                    fixture.actor.turn_countdown == 7 &&
                    fixture.port.count(
                        LegacyBattleGroupBActionSeventeenFrameCall::
                            reserved_actor_current_coordinate_query
                    ) == 0U &&
                    fixture.port.count(
                        LegacyBattleGroupBActionSeventeenFrameCall::
                            reserved_actor_coordinate_publication
                    ) == 0U &&
                    result.port_calls == 0U,
                "action seventeen current-coordinate stop preserves exact registers, flags and stack-local prefix"
            );
        }
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        fixture.actor.turn_countdown = 7;
        fixture.actor.alternate_position_x = 0x1111U;
        fixture.actor.alternate_position_y = 0x2222U;
        fixture.actor.publication_destination_dword_write_accessible[6U] =
            false;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        actor_coordinate_publication_typed_stop &&
                result.port_calls == 0U &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_publish_calls == 1U &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            destination_dword_write_typed_stop &&
                result.coordinate_publication.stopped_dword_index == 6U &&
                result.coordinate_publication.source_dword_reads == 7U &&
                result.coordinate_publication.destination_dword_writes == 6U &&
                result.coordinate_publication.return_eax == 75U &&
                result.coordinate_publication.return_ecx == 2U &&
                result.coordinate_publication.return_edx == 200U &&
                fixture.actor.position_x == 75U &&
                fixture.actor.position_y == 200U &&
                fixture.actor.alternate_position_x == 75U &&
                fixture.actor.alternate_position_y == 0x2222U &&
                fixture.shared.turn_frame_source_token == 0U &&
                result.render_calls == 0U &&
                fixture.actor.turn_countdown == 7 &&
                fixture.port.count(
                    LegacyBattleGroupBActionSeventeenFrameCall::
                        reserved_actor_coordinate_publication
                ) == 0U,
            "action seventeen publication fault preserves the copied prefix and suppresses rendering"
        );
    }

    {
        Fixture fixture;
        fixture.prepare_unchanged_record();
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        fixture.actor.turn_countdown = 7;
        fixture.actor.position_x = 0x1111U;
        fixture.actor.position_y = 0x2222U;
        fixture.actor.alternate_position_x = 0x3333U;
        fixture.actor.alternate_position_y = 0x4444U;
        fixture.actor.position_y_write_accessible = false;
        const auto result = fixture.run(&fixture.actor, &fixture.shared);
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::
                        actor_coordinate_publication_typed_stop &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            position_y_write_typed_stop &&
                result.coordinate_publication.coordinate_writes == 1U &&
                result.coordinate_publication.source_dword_reads == 0U &&
                result.coordinate_publication.destination_dword_writes == 0U &&
                fixture.actor.position_x == 0x10F8U &&
                fixture.actor.position_y == 0x2222U &&
                fixture.actor.alternate_position_x == 0x3333U &&
                fixture.actor.alternate_position_y == 0x4444U &&
                fixture.shared.turn_frame_source_token == 0U &&
                result.render_calls == 0U && fixture.actor.turn_countdown == 7,
            "action seventeen Y publication stop preserves the X prefix and suppresses rendering"
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
        fixture.actor.position_x = 0x32U;
        fixture.actor.position_y = 0x3CU;
        fixture.frame_request.coordinate_x_initial = 0x12340000U;
        fixture.frame_request.coordinate_y_initial = 0xAABB0000U;
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
        const std::size_t rendered_index = 56U * 640U + 68U;
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionSeventeenFrameStatus::completed &&
                result.return_eax == 0U && result.port_calls == 3U &&
                result.sample_play_calls == 2U &&
                result.sample_pan_calls == 1U && first_sample != nullptr &&
                first_sample->arguments[0U] == 0x10FU &&
                second_sample != nullptr &&
                second_sample->arguments[0U] == 0x2FU && pan != nullptr &&
                pan->eax == 1U && pan->arguments[0U] == 0xABCD002FU &&
                pan->arguments[1U] == 0x10U &&
                fixture.actor.turn_sample_word == 0U,
            "countdown fifteen preserves both sample calls and stale pan registers"
        );
        test.expect_true(
            fixture.actor.turn_render_flags == 0U &&
                fixture.actor.turn_target_x_offset == 7U &&
                result.coordinate_x == 0x12340032U &&
                result.coordinate_y == 0xAABB003CU &&
                result.current_coordinate_query.return_eax == 0x1111003CU &&
                result.current_coordinate_query.return_ecx == 0x33330004U &&
                result.current_coordinate_query.return_edx == 0x11110002U &&
                !result.current_coordinate_query.flags.carry &&
                !result.current_coordinate_query.flags.parity &&
                !result.current_coordinate_query.flags.auxiliary_carry &&
                !result.current_coordinate_query.flags.zero &&
                !result.current_coordinate_query.flags.sign &&
                !result.current_coordinate_query.flags.overflow &&
                result.adjusted_coordinate_x == 0x1234004BU &&
                result.coordinate_publication.status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            completed &&
                result.coordinate_publication.argument_x == 75U &&
                result.coordinate_publication.argument_y == 60U &&
                result.coordinate_publication.return_eax == 0x1234004BU &&
                result.coordinate_publication.return_ecx == 0U &&
                result.coordinate_publication.return_edx == 0x1234003CU &&
                result.coordinate_publication.return_esi == 0x00525508U &&
                result.coordinate_publication.return_edi == 0U &&
                !result.coordinate_publication.flags.carry &&
                result.coordinate_publication.flags.parity &&
                !result.coordinate_publication.flags.auxiliary_carry &&
                !result.coordinate_publication.flags.zero &&
                !result.coordinate_publication.flags.sign &&
                !result.coordinate_publication.flags.overflow &&
                fixture.actor.position_x == 75U &&
                fixture.actor.position_y == 60U &&
                fixture.actor.alternate_position_x == 75U &&
                fixture.actor.alternate_position_y == 60U &&
                fixture.port.count(
                    LegacyBattleGroupBActionSeventeenFrameCall::
                        reserved_actor_coordinate_publication
                ) == 0U &&
                fixture.shared.turn_frame_source_token == 0x00527A54U &&
                fixture.frame_provider.last_resource_id == 2U &&
                fixture.frame_provider.last_piece_index == 1U,
            "countdown fifteen preserves typed publication and frame lookup state"
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
