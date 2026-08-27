#include "openswd3/battle/legacy_battle_outcome_resolution.hpp"
#include "test.hpp"

#include <algorithm>
#include <functional>
#include <vector>

namespace {

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

class OutcomePort final : public LegacyBattleOutcomeResolutionPort {
public:
    LegacyBattleOutcomeResolutionState* shared_state{};
    std::vector<LegacyBattleOutcomeResolutionCall> calls;
    LegacyBattleOutcomeResolutionCallReply outcome_reply{};
    std::function<void(LegacyBattleOutcomeResolutionCall)> on_call;

    [[nodiscard]] LegacyBattleOutcomeResolutionState&
    outcome_resolution_state() noexcept override {
        return *shared_state;
    }

    [[nodiscard]] LegacyBattleOutcomeResolutionCallReply
    invoke_outcome_resolution(
        const LegacyBattleOutcomeResolutionCall call
    ) override {
        calls.push_back(call);
        if (on_call) {
            on_call(call);
        }
        return call == LegacyBattleOutcomeResolutionCall::resolve_outcome
            ? outcome_reply
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
        fixture.message_state = 7U;
        std::ranges::fill(fixture.framebuffer.physical_pixels(), u16{0x7FFFU});
        OutcomePort port;
        port.shared_state = &fixture.state;
        port.outcome_reply.eax = 0x12345678U;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            result.group_a_threshold_met && !result.group_b_threshold_met &&
                result.darkening_calls == 1U &&
                result.audio_suspend_calls == 1U &&
                result.outcome_calls == 1U &&
                port.calls ==
                    std::vector{
                        LegacyBattleOutcomeResolutionCall::suspend_audio_stream,
                        LegacyBattleOutcomeResolutionCall::resolve_outcome,
                    } &&
                fixture.state.resolution_latch == 1U &&
                fixture.state.darkening.channel_delta == 0 &&
                fixture.frame_active == 2U && result.return_value == 1U,
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
        port.outcome_reply.eax = 0xAABBCCDDU;

        const auto result =
            update_legacy_battle_outcome_resolution(fixture.bindings(), port);

        test.expect_true(
            !result.group_a_threshold_met && result.group_b_threshold_met &&
                result.darkening_calls == 1U &&
                result.audio_suspend_calls == 0U &&
                result.outcome_calls == 1U &&
                port.calls ==
                    std::vector{
                        LegacyBattleOutcomeResolutionCall::resolve_outcome
                    } &&
                fixture.frame_active == 0U &&
                result.return_value == 0xAABBCCDDU,
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
        port.outcome_reply.eax = 0x11223344U;
        port.on_call = [&](const LegacyBattleOutcomeResolutionCall call) {
            if (call == LegacyBattleOutcomeResolutionCall::resolve_outcome &&
                port.calls.size() == 2U) {
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
                        LegacyBattleOutcomeResolutionCall::suspend_audio_stream,
                        LegacyBattleOutcomeResolutionCall::resolve_outcome,
                        LegacyBattleOutcomeResolutionCall::resolve_outcome,
                    } &&
                result.return_value == 0x11223344U &&
                fixture.frame_active == 0U,
            "the second side rereads progress and darkening state after the first outcome callee and can resolve again"
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
