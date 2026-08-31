#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_outcome_resolution.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleOutcomeResolutionBindings;
using openswd3::battle::LegacyBattleOutcomeResolutionCall;
using openswd3::battle::LegacyBattleOutcomeResolutionCallReply;
using openswd3::battle::LegacyBattleOutcomeResolutionPort;
using openswd3::battle::LegacyBattleOutcomeResolutionState;
using openswd3::battle::LegacyBattleOutcomeResolutionStatus;
using openswd3::battle::LegacyBattleActionDispatchState;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::battle::update_legacy_battle_outcome_resolution;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacySurfaceGeometry;

class OutcomePort final
    : public LegacyBattleOutcomeResolutionPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    LegacyBattleOutcomeResolutionState* shared_state{};
    std::vector<LegacyBattleOutcomeResolutionCall> calls;
    LegacyBattleOutcomeResolutionCallReply audio_reply{};
    std::function<void(LegacyBattleOutcomeResolutionCall)> on_call;
    std::function<void(const LegacyBattleActionCallRequest&)> on_action_call;
    std::function<void(u32)> on_definition_load;
    std::vector<LegacyBattleActionCallRequest> action_calls;
    u32 next_allocation_token{0x73000000U};
    bool fail_allocation{};

    [[nodiscard]] LegacyBattleOutcomeResolutionState&
    outcome_resolution_state() noexcept override {
        return *shared_state;
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        action_calls.push_back(request);
        if (on_action_call) {
            on_action_call(request);
        }
        if (request.callee_token == 0x00487C10U) {
            if (fail_allocation) {
                return {};
            }
            const u32 token = next_allocation_token;
            next_allocation_token += 0xB0U;
            return {.eax = token};
        }
        return {};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<openswd3::compat::u8> destination
    ) override {
        auto reply = openswd3::test::LegacyBattleMonDatabaseFixture::
            invoke_legacy_battle_mon_database(request, destination);
        if (request.call ==
                openswd3::battle::LegacyBattleMonDatabaseCall::
                    allocate_stream &&
            on_definition_load && !requested_definition_ids.empty()) {
            on_definition_load(requested_definition_ids.back());
        }
        return reply;
    }

    [[nodiscard]] LegacyBattleOutcomeResolutionCallReply
    invoke_outcome_resolution(
        const LegacyBattleOutcomeResolutionCall call
    ) override {
        calls.push_back(call);
        if (on_call) {
            on_call(call);
        }
        return call == LegacyBattleOutcomeResolutionCall::suspend_audio_stream
            ? audio_reply
            : LegacyBattleOutcomeResolutionCallReply{};
    }
};

struct Fixture {
    LegacyBattleOutcomeResolutionState state;
    u32 frame_active{1U};
    LegacyBattleFinalActorStepState final_actor;
    LegacyBattleActionDispatchState action;
    u32 group_a_count{};
    u32 group_b_count{};
    u32 message_state{};
    u32 battle_mode_flags{};
    LegacyFramebuffer framebuffer;
    LegacyBlitEffectState effects;

    explicit Fixture(
        const LegacySurfaceGeometry& surface = LegacySurfaceGeometry{}
    )
        : framebuffer(surface) {}

    [[nodiscard]] LegacyBattleOutcomeResolutionBindings bindings() {
        return {
            .frame_active = frame_active,
            .group_a_count = group_a_count,
            .group_b_count = group_b_count,
            .final_actor = final_actor,
            .action = action,
            .message_state = message_state,
            .battle_mode_flags = battle_mode_flags,
            .framebuffer = framebuffer,
            .shared_effects = effects,
        };
    }
};

}  // namespace

void test_battle_outcome_resolution(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.group_a_count = 10U;
        fixture.final_actor.excluded_group_a_count = 2U;
        fixture.action.phase_counter = 1U << 16U;
        fixture.final_actor.removed_group_a_count = 6U;
        fixture.group_b_count = 4U;
        fixture.action.packed_actor_counter = 5U | (3U << 16U);
        OutcomePort port;
        port.shared_state = &fixture.state;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.status == LegacyBattleOutcomeResolutionStatus::completed &&
                !result.group_a_threshold_met &&
                result.group_a_remaining == 7U &&
                !result.group_b_threshold_met &&
                result.group_b_difference == 2U && result.return_value == 4U &&
                fixture.state.resolution_latch == 0U &&
                result.darkening_calls == 0U && port.calls.empty(),
            "neither side reaching its threshold returns the live group-B count without publishing the resolution latch"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 0U;
        fixture.final_actor.excluded_group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 0xFFU;
        fixture.group_b_count = 0U;
        fixture.action.packed_actor_counter = 1U << 16U;
        fixture.state.force_group_b_resolution = 2U;
        OutcomePort port;
        port.shared_state = &fixture.state;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.group_a_remaining == 0xFFFFFFFFU &&
                !result.group_a_threshold_met &&
                result.group_b_difference == 0xFFFFFFFFU &&
                !result.group_b_threshold_met &&
                fixture.state.resolution_latch == 0U,
            "group-A subtraction wraps unsigned while group-B difference compares signed and override accepts only exact one"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 10U;
        fixture.final_actor.excluded_group_a_count = 2U;
        fixture.action.phase_counter = 1U << 16U;
        fixture.final_actor.removed_group_a_count = 7U;
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -30;
        fixture.group_b_count = 1U;
        fixture.action.packed_actor_counter = 1U << 16U;
        fixture.message_state = 7U;
        std::ranges::fill(fixture.framebuffer.physical_pixels(), u16{0x7FFFU});
        OutcomePort port;
        port.shared_state = &fixture.state;
        port.audio_reply.eax = 0x12345678U;
        port.outcome_finalization_state().player_reward_item_ids[0] = 0x42U;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.group_a_threshold_met && !result.group_b_threshold_met &&
                result.darkening_calls == 1U &&
                result.audio_suspend_calls == 1U &&
                result.outcome_calls == 1U &&
                port.calls ==
                    std::vector{
                        LegacyBattleOutcomeResolutionCall::suspend_audio_stream
                    } &&
                result.first_finalization.cleanup_applied &&
                result.first_finalization.player_reward_calls == 1U &&
                result.first_finalization.return_value == 1U &&
                port.requested_definition_ids ==
                    std::vector<u32>{0x0042U, 0x0300U} &&
                fixture.group_b_count == 0U &&
                fixture.state.resolution_latch == 1U &&
                fixture.state.darkening.channel_delta == 0 &&
                fixture.frame_active == 2U && result.return_value == 0U,
            "group-A completion darkens to the terminal step then suspends audio resolves the outcome and publishes mode two"
        );
        test.expect_true(
            fixture.effects.red_offset == -30 &&
                fixture.effects.green_offset == -30 &&
                fixture.effects.blue_offset == -30 &&
                fixture.framebuffer.physical_pixels().front() == 0x0421U,
            "group-A resolution directly composes the closed full-frame darkening state and shared framebuffer"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 1U;
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -30;
        fixture.group_b_count = 1U;
        fixture.message_state = 0x68U;
        fixture.battle_mode_flags = 8U;
        OutcomePort port;
        port.shared_state = &fixture.state;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.outcome_calls == 1U && fixture.frame_active == 0U,
            "message value one-hundred-four and battle mode bit three each force the group-A outcome mode back to zero"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 2U;
        fixture.group_b_count = 2U;
        fixture.action.packed_actor_counter = 3U | (1U << 16U);
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -30;
        OutcomePort port;
        port.shared_state = &fixture.state;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            !result.group_a_threshold_met && result.group_b_threshold_met &&
                result.darkening_calls == 1U &&
                result.audio_suspend_calls == 0U &&
                result.outcome_calls == 1U && port.calls.empty() &&
                result.second_finalization.cleanup_applied &&
                result.second_finalization.group_reward_calls == 2U &&
                fixture.group_b_count == 0U && fixture.frame_active == 0U &&
                result.return_value == 2U,
            "group-B completion resolves without audio suspension and returns the complete outcome EAX"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 1U;
        fixture.group_b_count = 0U;
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -2;
        OutcomePort port;
        port.shared_state = &fixture.state;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.group_a_threshold_met && result.group_b_threshold_met &&
                result.darkening_calls == 2U &&
                result.first_darkening.return_value == 0U &&
                result.second_darkening.return_value == 0U &&
                fixture.state.darkening.channel_delta == -6 &&
                result.audio_suspend_calls == 0U &&
                result.outcome_calls == 0U && result.return_value == 0U,
            "both threshold paths can darken in one invocation and the second nonterminal result becomes the function return"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 1U;
        fixture.group_b_count = 2U;
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -30;
        OutcomePort port;
        port.shared_state = &fixture.state;
        port.on_definition_load = [&](const u32 definition_id) {
            if (definition_id == 0x0300U) {
                fixture.action.packed_actor_counter = 2U;
                fixture.state.darkening.channel_delta = -30;
            }
        };

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.group_a_threshold_met && result.group_b_threshold_met &&
                result.darkening_calls == 2U &&
                result.audio_suspend_calls == 1U &&
                result.outcome_calls == 2U &&
                port.calls ==
                    std::vector{
                        LegacyBattleOutcomeResolutionCall::suspend_audio_stream
                    } &&
                result.first_finalization.group_reward_calls == 2U &&
                result.second_finalization.cleanup_applied &&
                result.return_value == 0U && fixture.group_b_count == 0U &&
                fixture.frame_active == 0U,
            "the second side rereads progress and darkening state after the first outcome callee and can resolve again"
        );
    }

    {
        Fixture fixture;
        fixture.group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 1U;
        fixture.group_b_count = 1U;
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -30;
        OutcomePort port;
        port.shared_state = &fixture.state;
        port.outcome_finalization_state().player_reward_item_ids = {7U, 8U};
        port.outcome_finalization_state().completion_words = {9U, 10U};
        port.fail_allocation = true;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleOutcomeResolutionStatus::
                        outcome_finalization_typed_stop &&
                result.group_a_threshold_met && result.darkening_calls == 1U &&
                result.audio_suspend_calls == 1U &&
                result.outcome_calls == 1U &&
                result.first_finalization.status ==
                    openswd3::battle::LegacyBattleOutcomeFinalizationStatus::
                        player_item_quantity_typed_stop &&
                fixture.frame_active == 1U && fixture.group_b_count == 1U &&
                port.outcome_finalization_state().player_reward_item_ids ==
                    std::array<u16, 2>{7U, 8U} &&
                port.outcome_finalization_state().completion_words ==
                    std::array<u16, 2>{9U, 10U},
            "typed-stop in direct outcome finalization preserves the audio and darkening prefix but blocks frame mode and the second side"
        );
    }

    {
        Fixture fixture({.pitch_bytes = 4, .width = 2, .height = 1});
        fixture.group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 1U;
        fixture.state.darkening_gate = 1U;
        fixture.state.darkening.channel_delta = -2;
        OutcomePort port;
        port.shared_state = &fixture.state;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleOutcomeResolutionStatus::
                        full_frame_darkening_typed_stop &&
                result.group_a_threshold_met &&
                fixture.state.resolution_latch == 1U &&
                result.darkening_calls == 1U &&
                result.first_darkening.status ==
                    openswd3::battle::LegacyBattleFullFrameDarkeningStatus::
                        red_typed_stop &&
                fixture.effects.red_offset == -2 &&
                fixture.effects.green_offset == -2 &&
                fixture.effects.blue_offset == -2 &&
                result.audio_suspend_calls == 0U &&
                result.outcome_calls == 0U && port.calls.empty(),
            "darkening framebuffer failure preserves the threshold latch and three published offsets then blocks every later side effect"
        );
    }
}
