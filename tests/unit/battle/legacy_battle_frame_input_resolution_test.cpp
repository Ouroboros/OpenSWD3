#include "openswd3/battle/legacy_battle_frame_input_resolution.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
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

class StreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        requests.push_back({resource_id, piece_index});
        piece.width = width;
        piece.height = height;
        return available;
    }

    bool available{true};
    openswd3::compat::u16 width{1U};
    openswd3::compat::u16 height{1U};
    std::vector<std::array<u32, 2>> requests;
};

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
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::input_time_rng::LegacyInputNormalizationState input;
    u32 message{};
    std::vector<openswd3::world_map::LegacyWorldInteractionHotspot> hotspots;
    InputPort port;
    StreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;

    [[nodiscard]] openswd3::battle::LegacyBattleFrameInputResolutionBindings
    bindings() {
        return {
            .startup = startup,
            .final_actor = final_actor,
            .action = action,
            .metrics = metrics,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
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
        if (startup.group_b_lifecycle == nullptr) {
            startup.group_b_lifecycle = std::make_shared<std::array<
                openswd3::battle::LegacyBattleActorGroupBElementState,
                8>>();
        }
        for (std::size_t index = 0U; index < startup.party.size(); ++index) {
            startup.party[index].position_x =
                static_cast<openswd3::compat::u16>(origin_x + 0x24);
            startup.party[index].position_y =
                static_cast<openswd3::compat::u16>(origin_y);
            action.group_a_action_execution[index].profile_value = 1U;
        }
        for (auto& actor : *startup.group_b_lifecycle) {
            actor.action_execution.position_x =
                static_cast<openswd3::compat::u16>(origin_x + 0x24);
            actor.action_execution.position_y =
                static_cast<openswd3::compat::u16>(origin_y);
            actor.action_execution.profile_value = 1U;
        }
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
        fixture.port.battle_input_dispatch_state().action_kind = 6U;
        fixture.port.battle_frame_input_resolution_state()
            .target_action_available = 9U;
        fixture.startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        (*fixture.startup.group_b_lifecycle)[0U].resource_token = 1U;
        fixture.prepare_visible_surface(10, 10);
        auto& surface_reply =
            fixture.port.replies
                [LegacyBattleFrameInputResolutionCall::resolve_actor_surface];
        surface_reply.eax = 0xDEADBEEFU;
        surface_reply.ecx = 0xCAFEBABEU;
        surface_reply.edx = 0x0BADF00DU;
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        auto& frame_resource_request =
            request.actor_frame_resource_requests[0U];
        frame_resource_request.frame_provider_return_eax = 0x1234ABCDU;
        frame_resource_request.frame_provider_return_edx = 0x89ABCDEFU;
        frame_resource_request.entry_esp = 0x71001000U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        const auto surface_call = std::ranges::find_if(
            fixture.port.calls,
            [](const LegacyBattleFrameInputResolutionCallRequest& call) {
                return call.call ==
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface;
            }
        );
        const auto mirror_call = std::ranges::find_if(
            fixture.port.calls,
            [](const LegacyBattleFrameInputResolutionCallRequest& call) {
                return call.call ==
                    LegacyBattleFrameInputResolutionCall::query_actor_mirror;
            }
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
                    LegacyBattleFrameInputResolutionCall::
                        reserved_prepare_actor_origin_slot
                ) == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        reserved_query_group_b_action_six_target_availability_slot
                ) == 0U &&
                result.actor_frame_snapshot_queries == 1U &&
                result.actor_frame_snapshot.output[0U] == 10U &&
                result.actor_frame_snapshot.output[1U] == 10U &&
                result.actor_frame_resource_queries == 1U &&
                result.actor_frame_resource_caller == 0U &&
                result.actor_frame_resource.return_eip == 0x004605DEU &&
                result.actor_frame_resource.return_ebx == 0U &&
                result.actor_frame_resource.return_esi == 0x00525508U &&
                result.actor_frame_resource.return_edi == 0U &&
                result.actor_frame_resource.return_esp == 0x71001004U &&
                result.actor_frame_resource.copied_dwords == 0x26U &&
                result.actor_frame_snapshot.action_update_calls == 1U &&
                result.actor_frame_snapshot.frame_lookup_calls == 1U &&
                result.actor_frame_resource.action_update_calls == 1U &&
                result.actor_frame_resource.frame_lookup_calls == 1U &&
                fixture.frame_provider.requests.size() == 2U &&
                (*fixture.startup.group_b_lifecycle)[0U]
                        .action_execution.turn_frame_token == 0x1234ABCDU &&
                surface_call != fixture.port.calls.end() &&
                surface_call->actor_token == 0x1234ABCDU &&
                mirror_call != fixture.port.calls.end() &&
                mirror_call->eax == 0x1234ABCDU &&
                mirror_call->edx == 0x89ABCDEFU &&
                result.action_six_availability_queries == 1U &&
                result.action_six_availability.resource_flags == 0U &&
                fixture.port.battle_frame_input_resolution_state()
                        .target_action_available == 0U &&
                result.image_queries == 1U,
            "case three keeps surface resolution transparent to the Group-B leaf registers before the first mirror call"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[0U].frame_provider_return_eax =
            0U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_object_typed_stop &&
                result.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        completed &&
                result.actor_frame_resource.frame_token_committed &&
                (*fixture.startup.group_b_lifecycle)[0U]
                        .action_execution.turn_frame_token == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                fixture.final_actor.published_actor_code == 0U &&
                result.image_queries == 0U,
            "null Group-B frame token stops at the caller object read after committing the actor prefix"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.port.battle_input_dispatch_state().action_kind = 6U;
        fixture.prepare_visible_surface(10, 10);
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[0U].frame_provider_return_eax =
            0x1234ABCDU;
        request.actor_frame_resource_object_readable[0U] = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_object_typed_stop &&
                result.actor_frame_resource_caller == 0U &&
                result.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        completed &&
                result.actor_frame_resource.frame_token_committed &&
                (*fixture.startup.group_b_lifecycle)[0U]
                        .action_execution.turn_frame_token == 0x1234ABCDU &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::query_actor_mirror
                ) == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        configure_actor_selection
                ) == 1U &&
                result.action_six_availability_queries == 0U &&
                result.image_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "unreadable nonnull Group-B frame object stops before mirror, surface, image, action-six, and publication suffixes"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        fixture.port
            .replies
                [LegacyBattleFrameInputResolutionCall::resolve_actor_surface]
            .surface.command_stream_present = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        completed &&
                result.return_eax == 0U &&
                result.actor_frame_resource.return_eax == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 1U &&
                result.image_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "nonzero frame token with a null first object dword follows the ordinary Group-B miss path"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 2U;
        fixture.prepare_visible_surface(10, 10);
        auto& actor = (*fixture.startup.group_b_lifecycle)[1U].action_execution;
        actor.frame_source_action_record.action_id = 0x11111111U;
        actor.frame_source_action_record.cached_action_id = 0x22222222U;
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[0U].source_dword_readable[1U] =
            false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_typed_stop &&
                result.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        source_dword_read_typed_stop &&
                result.actor_frame_resource.copied_dwords == 1U &&
                actor.frame_prepared_action_record.action_id == 0x11111111U &&
                actor.frame_prepared_action_record.cached_action_id == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        configure_actor_selection
                ) == 2U &&
                fixture.final_actor.published_actor_code == 0U,
            "Group-B REP source fault preserves the first copied dword and suppresses the current and remaining actor suffixes"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        (*fixture.startup.group_b_lifecycle)[0U]
            .action_configuration.special_ready = 1U;
        (*fixture.startup.group_b_lifecycle)[0U]
            .action_configuration.source_runtime_value = 0U;
        actor.turn_frame_token_write_accessible = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_typed_stop &&
                result.actor_frame_snapshot.returned_early &&
                result.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        frame_token_write_typed_stop &&
                result.actor_frame_resource.copied_dwords == 0x26U &&
                result.actor_frame_resource.action_update_calls == 1U &&
                result.actor_frame_resource.frame_lookup_calls == 1U &&
                !result.actor_frame_resource.frame_token_committed &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                result.image_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "frame-token write fault preserves the completed resource lookup and suppresses every caller suffix"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        (*fixture.startup.group_b_lifecycle)[0U]
            .action_configuration.special_ready = 1U;
        (*fixture.startup.group_b_lifecycle)[0U]
            .action_configuration.source_runtime_value = 0U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(),
                fixture.port,
                {
                    .actor_frame_initial_output = {10U, 10U, 1U, 1U},
                }
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        completed &&
                result.return_eax == 1U &&
                result.actor_frame_snapshot.returned_early &&
                result.actor_frame_snapshot.return_eax == 0U &&
                result.actor_frame_snapshot.return_ecx == 1U &&
                result.actor_frame_snapshot.action_update_calls == 0U &&
                result.actor_frame_snapshot.frame_lookup_calls == 0U &&
                result.actor_frame_snapshot.output ==
                    std::array<u32, 4>{10U, 10U, 1U, 1U} &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 1U &&
                result.image_queries == 1U,
            "leaf early return leaves the shared local snapshot intact and caller hit-testing continues"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.port.battle_input_dispatch_state().selection_index = 6U;
        fixture.port.battle_frame_input_resolution_state()
            .target_action_available = 9U;
        fixture.prepare_visible_surface(10, 10);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        completed &&
                result.action_six_availability_queries == 0U &&
                fixture.port.battle_frame_input_resolution_state()
                        .target_action_available == 1U,
            "case three gates the target query with action kind rather than the unrelated selection index"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.port.battle_input_dispatch_state().action_kind = 6U;
        fixture.port.battle_frame_input_resolution_state()
            .target_action_available = 9U;
        fixture.prepare_visible_surface(10, 10);
        fixture.startup.group_b_lifecycle.reset();
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_snapshot_typed_stop &&
                result.actor_frame_snapshot.status ==
                    openswd3::battle::LegacyBattleActorFrameSnapshotStatus::
                        special_ready_read_typed_stop &&
                result.actor_frame_snapshot_queries == 1U &&
                result.action_six_availability_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                result.return_ecx == 0x26U,
            "case three missing actor stops in the typed frame snapshot before surface resolution and target publication"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        fixture.frame_provider.available = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_snapshot_typed_stop &&
                result.actor_frame_snapshot.status ==
                    openswd3::battle::LegacyBattleActorFrameSnapshotStatus::
                        frame_width_read_typed_stop &&
                result.actor_frame_snapshot.output[0U] == 10U &&
                result.actor_frame_snapshot.output[1U] == 10U &&
                result.actor_frame_snapshot.output_writes == 2U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                result.image_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "frame provider stop preserves the candidate query and X/Y snapshot prefix while suppressing the surface suffix"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_output_writable[0U] = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_snapshot_typed_stop &&
                result.actor_frame_snapshot.status ==
                    openswd3::battle::LegacyBattleActorFrameSnapshotStatus::
                        output_x_write_typed_stop &&
                result.actor_frame_snapshot.output_writes == 0U &&
                result.actor_frame_snapshot.return_edx == 10U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                fixture.frame_provider.requests.size() == 1U &&
                fixture.final_actor.published_actor_code == 0U,
            "caller X-store stop preserves candidate and frame-query side effects while suppressing surface resolution"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_b_count = 1U;
        fixture.prepare_visible_surface(10, 10);
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(),
                fixture.port,
                {
                    .actor_frame_overlapping_resource_dword_readable = false,
                }
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_snapshot_typed_stop &&
                result.actor_frame_snapshot.status ==
                    openswd3::battle::LegacyBattleActorFrameSnapshotStatus::
                        overlapping_resource_dword_read_typed_stop &&
                result.actor_frame_snapshot.action_update_calls == 1U &&
                result.actor_frame_snapshot.local_reads == 1U &&
                result.actor_frame_snapshot.frame_lookup_calls == 0U &&
                fixture.frame_provider.requests.empty() &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        query_group_b_candidate
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "caller local-parameter stop preserves action update and candidate query while suppressing provider and hit-test suffixes"
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
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[2U].entry_esp = 0x72001000U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
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
                ) == 1U &&
                result.actor_frame_snapshot_queries == 1U &&
                result.actor_frame_resource_queries == 1U &&
                result.actor_frame_resource_caller == 2U &&
                result.actor_frame_resource.return_eip == 0x00460A0FU &&
                result.actor_frame_resource.return_ebx == 0U &&
                result.actor_frame_resource.return_esi == 0x005029D0U &&
                result.actor_frame_resource.return_edi == 0U &&
                result.actor_frame_resource.return_esp == 0x72001004U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        reserved_prepare_actor_origin_slot
                ) == 0U &&
                fixture.action.group_a_action_execution[0U].turn_frame_token ==
                    1U,
            "case three group-A direct scan clears the physical first marker dword after a visible target"
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
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[2U].frame_provider_return_eax =
            0U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_object_typed_stop &&
                result.actor_frame_resource_caller == 2U &&
                result.actor_frame_resource.frame_token_committed &&
                fixture.action.group_a_action_execution[0U].turn_frame_token ==
                    0U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::query_actor_mirror
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U &&
                result.image_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "null Group-A frame token stops at the first pixel resource access after preserving the preceding mirror query"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 2U;
        fixture.startup.reset.block_520e90[5U] = 1U;
        fixture.prepare_visible_surface(10, 10);
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[2U].frame_provider_return_eax =
            0x89ABCDEFU;
        request.actor_frame_resource_object_readable[2U] = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_object_typed_stop &&
                result.actor_frame_resource_caller == 2U,
            "unreadable nonnull Group-A frame object reports the caller object-read typed stop"
        );
        test.expect_true(
            result.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        completed &&
                result.actor_frame_resource.frame_token_committed &&
                fixture.action.group_a_action_execution[1U].turn_frame_token ==
                    0x89ABCDEFU,
            "unreadable nonnull Group-A frame object preserves the completed leaf token commit"
        );
        test.expect_true(
            fixture.port.count(
                LegacyBattleFrameInputResolutionCall::query_group_a_candidate
            ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::query_actor_mirror
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U,
            "unreadable nonnull Group-A frame object preserves one mirror before suppressing surface resolution"
        );
        test.expect_true(
            fixture.port.count(
                LegacyBattleFrameInputResolutionCall::configure_actor_selection
            ) == 3U &&
                result.actor_iterations == 2U && result.image_queries == 0U &&
                fixture.final_actor.published_actor_code == 0U,
            "unreadable nonnull Group-A frame object preserves the two-actor reset and selected-actor configuration prefix while suppressing image, remaining-candidate, and publication suffixes"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 4U;
        fixture.startup.reset.block_520e90[5U] = 1U;
        fixture.final_actor.actor_order[3U] = 0U;
        fixture.prepare_visible_surface(10, 10);
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[1U].entry_esp = 0x73001000U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        completed &&
                result.return_eax == 1U &&
                result.actor_frame_snapshot_queries == 1U &&
                result.actor_frame_resource_queries == 1U &&
                result.actor_frame_resource_caller == 1U &&
                result.actor_frame_resource.return_eip == 0x004607FBU &&
                result.actor_frame_resource.return_ebx == 0x004A797CU &&
                result.actor_frame_resource.return_esi == 1U &&
                result.actor_frame_resource.return_edi == 0U &&
                result.actor_frame_resource.return_esp == 0x73001004U &&
                result.actor_frame_snapshot.output[0U] == 10U &&
                fixture.action.group_a_action_execution[0U].turn_frame_token ==
                    1U &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::
                        reserved_prepare_actor_origin_slot
                ) == 0U &&
                fixture.final_actor.published_actor_code == 1U &&
                result.image_queries == 1U,
            "case three large Group-A order path composes the typed snapshot before its eight-by-eight hit scan"
        );
    }

    {
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 4U;
        fixture.startup.reset.block_520e90[0U] = 1U;
        fixture.final_actor.actor_order[3U] = 2U;
        fixture.final_actor.actor_order[2U] = 1U;
        fixture.prepare_visible_surface(10, 10);
        fixture.startup.party[2U].position_x = 1000U;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        completed &&
                result.return_eax == 1U,
            "actor-order continues from the full-scan miss to a later visible candidate"
        );
        test.expect_true(
            result.actor_frame_snapshot_queries == 2U &&
                result.actor_frame_resource_queries == 2U &&
                result.actor_frame_resource_caller == 1U,
            "actor-order invokes both typed frame leaves for the missed and visible candidates"
        );
        test.expect_true(
            result.actor_frame_resource.return_ebx == 0x004A7978U &&
                result.actor_frame_resource.return_esi == 1U &&
                result.actor_frame_resource.return_edi == 8U,
            "actor-order full-scan miss carries EDI eight with the next slot and ESI residues"
        );
        test.expect_true(
            result.image_queries == 65U,
            "actor-order performs sixty-four miss probes before the next candidate hit"
        );
    }

    {
        constexpr u32 actor_index = 2U;
        constexpr u32 actor_token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
            actor_index *
                openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
        Fixture fixture;
        fixture.set_mouse(10, 10);
        fixture.message = 3U;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.metrics.group_a_count = 4U;
        fixture.startup.reset.block_520e90[5U] = 1U;
        fixture.final_actor.actor_order[3U] = actor_index;
        fixture.prepare_visible_surface(10, 10);
        auto request =
            openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        auto& leaf_request = request.actor_frame_resource_requests[1U];
        leaf_request.entry_esp = 0x74001000U;
        leaf_request.stack_access.push_ebx_writable = false;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port, request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_typed_stop &&
                result.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        push_ebx_write_typed_stop &&
                result.actor_frame_resource.return_eax ==
                    actor_index * 0xBCDU &&
                result.actor_frame_resource.return_ebx == 0x004A797CU &&
                result.actor_frame_resource.return_esi == 1U &&
                result.actor_frame_resource.return_edi == 0U &&
                result.actor_frame_resource.return_esp == 0x74001000U &&
                result.actor_frame_resource.flags_known &&
                !result.actor_frame_resource.flags.carry &&
                result.actor_frame_resource.flags.parity &&
                result.actor_frame_resource.flags.auxiliary_carry_defined &&
                result.actor_frame_resource.flags.auxiliary_carry &&
                !result.actor_frame_resource.flags.zero &&
                !result.actor_frame_resource.flags.sign &&
                !result.actor_frame_resource.flags.overflow &&
                fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U,
            "actor-order caller reconstructs its post-SUB EAX, flags, slot token, and callee-saved values before the leaf push"
        );

        Fixture rep_fixture;
        rep_fixture.set_mouse(10, 10);
        rep_fixture.message = 3U;
        rep_fixture.final_actor.queued_actor_code = 9U;
        rep_fixture.metrics.group_a_count = 4U;
        rep_fixture.startup.reset.block_520e90[5U] = 1U;
        rep_fixture.final_actor.actor_order[3U] = actor_index;
        rep_fixture.prepare_visible_surface(10, 10);
        request = openswd3::battle::LegacyBattleFrameInputResolutionRequest{};
        request.actor_frame_resource_requests[1U].source_dword_readable[5U] =
            false;
        const auto rep =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                rep_fixture.bindings(), rep_fixture.port, request
            );
        test.expect_true(
            rep.status ==
                    openswd3::battle::LegacyBattleFrameInputResolutionStatus::
                        actor_frame_resource_typed_stop &&
                rep.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        source_dword_read_typed_stop &&
                rep.actor_frame_resource.stack_write_count == 3U &&
                rep.actor_frame_resource.stack_writes[0U] == 0x004A797CU &&
                rep.actor_frame_resource.return_eax == actor_token + 0x0CB8U &&
                rep.actor_frame_resource.return_ebx == actor_token &&
                rep.actor_frame_resource.return_esi ==
                    actor_token + 0x02A0U + 5U * sizeof(u32) &&
                rep.actor_frame_resource.return_edi ==
                    actor_token + 0x0CB8U + 5U * sizeof(u32) &&
                rep.actor_frame_resource.return_ecx == 0x21U &&
                rep.actor_frame_resource.flags.parity &&
                rep.actor_frame_resource.flags.auxiliary_carry &&
                rep_fixture.port.count(
                    LegacyBattleFrameInputResolutionCall::resolve_actor_surface
                ) == 0U,
            "actor-order REP fault preserves the physical caller stack and post-SUB flags with the copied prefix"
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
        fixture.port
            .replies[LegacyBattleFrameInputResolutionCall::query_actor_mirror]
            .edx = 0x13579BDFU;
        fixture.port
            .replies
                [LegacyBattleFrameInputResolutionCall::resolve_actor_surface]
            .edx = 0x0BADF00DU;
        const auto result =
            openswd3::battle::coordinate_legacy_battle_frame_input_resolution(
                fixture.bindings(), fixture.port
            );
        std::size_t mirror_calls{};
        u32 second_mirror_edx{};
        for (const auto& call : fixture.port.calls) {
            if (call.call !=
                LegacyBattleFrameInputResolutionCall::query_actor_mirror) {
                continue;
            }
            ++mirror_calls;
            if (mirror_calls == 2U) {
                second_mirror_edx = call.edx;
            }
        }
        test.expect_true(
            result.return_eax == 0U && result.image_queries == 64U &&
                mirror_calls == 64U && second_mirror_edx == 0x13579BDFU &&
                fixture.final_actor.published_actor_code == 0U,
            "same active group-A target keeps surface resolution transparent to mirror residue across all sixty-four pixel calls"
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
