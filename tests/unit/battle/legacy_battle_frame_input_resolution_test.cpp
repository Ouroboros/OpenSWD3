#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

#include <algorithm>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleFrameInputResolutionCall;
using openswd3::battle::LegacyBattleFrameInputResolutionCallReply;
using openswd3::battle::LegacyBattleFrameInputResolutionCallRequest;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u32;

class InputPort final
    : public openswd3::battle::LegacyBattleFrameInputResolutionPort {
public:
    [[nodiscard]] LegacyBattleFrameInputResolutionCallReply
    invoke_frame_input_resolution(
        const LegacyBattleFrameInputResolutionCallRequest& request
    ) override {
        calls.push_back(request);
        const auto found = replies.find(request.call);
        auto reply = found == replies.end() ? default_reply : found->second;
        if (request.call ==
            LegacyBattleFrameInputResolutionCall::resolve_actor_surface) {
            reply.surface.command_stream = command_stream;
        }
        return reply;
    }

    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        input_calls.push_back(request);
        return {};
    }

    void delay_input_milliseconds(u32) override {}

    [[nodiscard]] LegacyBattleInputDispatchCallReply play_input_sample(
        const u32 sound_id,
        const i32 mix_level,
        const u32 eax,
        const u32 ecx,
        const u32 edx
    ) override {
        samples.push_back({sound_id, static_cast<u32>(mix_level)});
        return {.eax = eax + 1U, .ecx = ecx + 2U, .edx = edx + 3U};
    }

    [[nodiscard]] std::size_t
    count(const LegacyBattleFrameInputResolutionCall call) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls,
            [call](const LegacyBattleFrameInputResolutionCallRequest& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleFrameInputResolutionCallRequest> calls;
    std::vector<LegacyBattleInputDispatchCallRequest> input_calls;
    std::map<
        LegacyBattleFrameInputResolutionCall,
        LegacyBattleFrameInputResolutionCallReply>
        replies;
    LegacyBattleFrameInputResolutionCallReply default_reply{};
    std::vector<openswd3::compat::u8> command_stream;
    std::vector<std::array<u32, 2>> samples;
};

struct Fixture {
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::input_time_rng::LegacyInputNormalizationState input;
    u32 message{};
    std::vector<openswd3::world_map::LegacyWorldInteractionHotspot> hotspots;
    InputPort port;

    [[nodiscard]] openswd3::battle::LegacyBattleFrameInputResolutionBindings
    bindings() {
        return {
            .startup = startup,
            .final_actor = final_actor,
            .metrics = metrics,
            .input_dispatch = port.battle_input_dispatch_state(),
            .input = input,
            .message_state = message,
            .choice_hotspots = hotspots,
        };
    }

    void set_mouse(const i32 x, const i32 y) {
        input.current_mouse.logical_x = x;
        input.current_mouse.logical_y = y;
    }

    void prepare_visible_surface(const i32 origin_x, const i32 origin_y) {
        port.command_stream = {
            0xFFU,
            0xFFU,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            1U,
            0U,
        };
        port.replies
            [LegacyBattleFrameInputResolutionCall::prepare_actor_origin] = {
            .origin_x = origin_x,
            .origin_y = origin_y,
        };
        port.replies
            [LegacyBattleFrameInputResolutionCall::resolve_actor_surface] = {
            .surface = {
                .object_token = 0x12345678U,
                .command_stream_present = true,
                .width = 1U,
                .height = 1U,
            },
        };
    }
};

}  // namespace

void test_battle_frame_input_resolution(openswd3::test::Context& test) {
    {
        Fixture fixture;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(),
                fixture.port,
                {.entry_eax = 9U, .entry_ecx = 10U, .entry_edx = 11U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 10U &&
                result.return_edx == 11U && fixture.port.calls.empty(),
            "unchanged mouse with a clear frame gate returns zero before every case and callee"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(100, 400);
        fixture.metrics.group_b_count = 2U;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 2U;
        fixture.startup.party_offsets[2U] = 100;
        fixture.port.battle_input_dispatch_state().mouse_action_gate = 9U;
        fixture.port.battle_frame_input_resolution_state()
            .pointer_activity_gate = 8U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_input_dispatch_state()
                        .selected_option_word == 8U &&
                fixture.final_actor.pre_frame_gate_b == 1U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    9U &&
                fixture.port.battle_frame_input_resolution_state()
                        .pointer_activity_gate == 0U &&
                fixture.port.battle_frame_input_resolution_state()
                        .previous_mouse_x == 100 &&
                fixture.port.battle_frame_input_resolution_state()
                        .previous_mouse_y == 400,
            "case zero clears only the pointer-activity gate, preserves the mouse-action gate, and publishes the first party hover"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(20, 50);
        fixture.message = 1U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.startup.reset.value_524414 = 1U;
        fixture.port.battle_input_dispatch_state().selection_index = 2U;
        fixture.port.battle_input_dispatch_state().sample_mix_level = -5;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_input_dispatch_state().selection_index ==
                    1U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U &&
                fixture.port.samples ==
                    std::vector<std::array<u32, 2>>{{0x2EU, 0xFFFFFFFBU}},
            "case one accepts an enabled grid option and preserves the signed selection sample"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(250, 199);
        fixture.message = 30U;
        fixture.port.battle_input_dispatch_state().sample_mix_level = 7;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_frame_input_resolution_state()
                        .grid_selection == 2U &&
                fixture.port.samples ==
                    std::vector<std::array<u32, 2>>{{0x2EU, 7U}} &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U,
            "case thirty maps the second strict grid row and plays one changed-selection sample"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(0x193, 0xA0);
        fixture.message = 2U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_input_dispatch_state().interaction_mode ==
                    1U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U,
            "case two uses strict action-button rectangles after rejecting their boundaries"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(0x19F, 0x130);
        fixture.message = 4U;
        fixture.port.battle_frame_input_resolution_state().panel_row_limit_c =
            8U;
        fixture.input.records[15U].held_sample_count = 0xFFFFFFFFU;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_input_dispatch_state().interaction_mode ==
                    2U,
            "case four keeps the signed negative held count below the action-button block gate"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(15, 15);
        fixture.hotspots.push_back({
            .left = 10U,
            .top = 10U,
            .right = 20U,
            .bottom = 20U,
        });
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 0U && result.hotspot_queries == 1U &&
                fixture.port.battle_input_dispatch_state().choice_guard == 1U &&
                fixture.port.battle_input_dispatch_state()
                        .choice_selection_index == 0U,
            "nonempty choice owner directly performs the strict hotspot query and publishes its first hit"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(0xC5, 220);
        fixture.message = 5U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_frame_input_resolution_state()
                        .group_b_row_selection == 1U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U,
            "case five accepts the first strict twenty-two-pixel row"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(0xE1, 170);
        fixture.message = 8U;
        fixture.port.battle_frame_input_resolution_state().panel_row_limit_b =
            2U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_frame_input_resolution_state()
                        .narrow_list_selection == 1U,
            "case eight accepts the first strict twenty-four-pixel row under its signed byte limit"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(0x195, 0x130);
        fixture.message = 27U;
        fixture.port.battle_frame_input_resolution_state().panel_row_limit_c =
            8U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.port.battle_input_dispatch_state().interaction_mode ==
                    2U,
            "case twenty-seven preserves its wider second action-button rectangle"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.port.battle_input_dispatch_state().selection_index = 6U;
        fixture.prepare_visible_surface(10, 10);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 1U &&
                fixture.final_actor.published_actor_code == 1U &&
                fixture.port.battle_frame_input_resolution_state()
                        .selected_target_index == 0U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    1U &&
                fixture.port.calls.front().actor_token == 0x00525508U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        configure_actor_selection
                ) == 2U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::query_group_b_mode
                ) == 1U &&
                fixture.port.battle_frame_input_resolution_state()
                        .target_action_available == 0U &&
                result.image_queries == 1U,
            "case three scans group B in reverse and commits the first visible pixel candidate"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.reset.block_520e90[5U] = 1U;
        fixture.port.battle_frame_input_resolution_state().target_markers.fill(
            9U
        );
        fixture.prepare_visible_surface(10, 10);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        const auto& markers =
            fixture.port.battle_frame_input_resolution_state().target_markers;
        test.expect_true(
            result.return_eax == 1U &&
                fixture.final_actor.published_actor_code == 1U &&
                markers[0U] == 0U && markers[1U] == 0U && markers[2U] == 0U &&
                markers[3U] == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_a_candidate
                ) == 1U,
            "case three group-A direct scan clears the physical first marker dword after a visible target"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.reset.block_520e90[0U] = 1U;
        fixture.prepare_visible_surface(10, 10);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.return_eax == 0U && result.image_queries == 64U &&
                fixture.final_actor.published_actor_code == 0U,
            "same active group-A target with a nonoverride selection preserves all sixty-four visible pixel calls without publishing"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 12U;
        fixture.startup.reset.block_520e90[5U] = 1U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_order_typed_stop &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        configure_actor_selection
                ) == 13U &&
                result.image_queries == 0U,
            "large live group-A count stops at the first reverse actor-order read after every preceding reset call"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.reset.block_520e90[5U] = 1U;
        fixture.prepare_visible_surface(10, 10);
        fixture.port.command_stream.clear();
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        image_source_typed_stop &&
                fixture.final_actor.pre_frame_gate_b == 1U &&
                fixture.port.battle_input_dispatch_state().mouse_action_gate ==
                    0U &&
                result.image_queries == 1U,
            "missing actor image data stops only at the closed pixel query after preserving the mouse prefix"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 0x100U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        startup_mode_typed_stop &&
                fixture.final_actor.pre_frame_gate_b == 1U &&
                fixture.port.calls.empty(),
            "actor mode access stops at the first physical startup table read after the entry mouse writes"
        );
    }
}
