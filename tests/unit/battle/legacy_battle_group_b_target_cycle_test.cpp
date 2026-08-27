#include "openswd3/battle/legacy_battle_group_b_target_cycle.hpp"

#include "test.hpp"

#include <functional>
#include <optional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleTargetSelectionRuntimeCall;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeCallReply;
using openswd3::battle::LegacyBattleTargetSelectionRuntimeCallRequest;
using openswd3::compat::u32;

constexpr u32 kGroupBBaseToken = 0x00525508U;
constexpr u32 kGroupBStride = 0x2B28U;

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kGroupBBaseToken + index * kGroupBStride;
}

class TargetCyclePort final
    : public openswd3::battle::LegacyBattleTargetSelectionRuntimePort {
public:
    [[nodiscard]] LegacyBattleTargetSelectionRuntimeCallReply
    invoke_target_selection_runtime(
        const LegacyBattleTargetSelectionRuntimeCallRequest& request
    ) override {
        calls.push_back(request);
        const std::size_t index = calls.size() - 1U;
        auto reply = LegacyBattleTargetSelectionRuntimeCallReply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (index < replies.size() && replies[index].has_value()) {
            reply = *replies[index];
        }
        if (after_call) {
            after_call(index, request.call);
        }
        return reply;
    }

    std::vector<LegacyBattleTargetSelectionRuntimeCallRequest> calls;
    std::vector<std::optional<LegacyBattleTargetSelectionRuntimeCallReply>>
        replies;
    std::function<void(std::size_t, LegacyBattleTargetSelectionRuntimeCall)>
        after_call;
};

struct Fixture {
    [[nodiscard]] openswd3::battle::LegacyBattleGroupBTargetCycleBindings
    bindings() {
        return {
            .frame_input = frame,
            .metrics = metrics,
            .final_actor = final_actor,
            .target_runtime = runtime,
            .message_state = message,
        };
    }

    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState runtime;
    u32 message{};
    TargetCyclePort port;
};

}  // namespace

void test_battle_group_b_target_cycle(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBTargetCycleStatus;
    using openswd3::battle::cycle_legacy_battle_group_b_target;

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 1U;
        fixture.metrics.group_b_count = 3U;
        fixture.port.replies = {
            LegacyBattleTargetSelectionRuntimeCallReply{
                .eax = 0U, .ecx = 0x11U, .edx = 0x22U
            },
            LegacyBattleTargetSelectionRuntimeCallReply{
                .eax = 0xAAU, .ecx = 0xBBU, .edx = 0xCCU
            },
        };
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.status == LegacyBattleGroupBTargetCycleStatus::completed &&
                result.port_calls == 2U && result.completion_queries == 1U &&
                result.reset_calls == 1U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        query_group_b_completion &&
                fixture.port.calls[0U].actor_token == group_b_token(1U) &&
                fixture.port.calls[0U].eax == 0x159U &&
                fixture.port.calls[0U].ecx == group_b_token(1U) &&
                fixture.port.calls[0U].edx == 0x77U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleTargetSelectionRuntimeCall::
                        reset_actor_selection &&
                fixture.port.calls[1U].arguments[0U] == 1U &&
                fixture.port.calls[1U].eax == 1U &&
                fixture.port.calls[1U].ecx == group_b_token(1U) &&
                fixture.port.calls[1U].edx == 0x565U &&
                fixture.runtime.selection_input_gate == 1U &&
                fixture.final_actor.published_actor_code == 2U &&
                result.return_eax == 2U && result.return_ecx == 0xBBU &&
                result.return_edx == 0xCCU,
            "available current target is reset then published one-based with reset-callee ECX and EDX"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 3U;
        fixture.metrics.group_b_count = 3U;
        fixture.port.replies.resize(2U);
        fixture.port.replies[0U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 0U};
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleGroupBTargetCycleStatus::completed &&
                fixture.frame.target_actor_index == 0U &&
                fixture.port.calls[0U].actor_token == group_b_token(0U) &&
                fixture.final_actor.published_actor_code == 1U,
            "signed current index at the live count normalizes to zero before the first query"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 1U;
        fixture.metrics.group_b_count = 3U;
        fixture.runtime.target_actor_indices[2U] = 2U;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 0U};
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleGroupBTargetCycleStatus::completed &&
                result.target_order_reads == 1U &&
                result.skipped_targets == 1U &&
                result.completion_queries == 2U &&
                fixture.frame.target_cursor == 2U &&
                fixture.frame.target_actor_index == 2U &&
                fixture.port.calls[1U].eax == 0xACAU &&
                fixture.port.calls[1U].ecx == group_b_token(2U) &&
                fixture.port.calls[1U].edx == 0x2B2U &&
                fixture.port.calls[2U].eax == 2U &&
                fixture.port.calls[2U].edx == 0xACAU &&
                fixture.final_actor.published_actor_code == 3U,
            "completed current target advances through the one-based order and preserves the loop query register shape"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 0U;
        fixture.metrics.group_b_count = 3U;
        fixture.runtime.target_actor_indices[1U] = 1U;
        fixture.runtime.target_actor_indices[2U] = 2U;
        fixture.runtime.target_actor_indices[3U] = 0U;
        fixture.port.replies.resize(4U);
        fixture.port.replies[0U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U};
        fixture.port.replies[2U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U};
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleGroupBTargetCycleStatus::completed &&
                result.completion_queries == 3U && result.reset_calls == 1U &&
                result.target_order_reads == 3U && fixture.message == 1U &&
                fixture.frame.target_cursor == 3U &&
                fixture.frame.target_actor_index == 0U &&
                fixture.final_actor.published_actor_code == 1U,
            "all completed targets set exhaustion after the final order read but still reset and publish that candidate"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 0U;
        fixture.metrics.group_b_count = 4U;
        fixture.runtime.target_actor_indices[2U] = 3U;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 0U};
        fixture.port.after_call =
            [&](const std::size_t index,
                const LegacyBattleTargetSelectionRuntimeCall call) {
                if (index == 0U &&
                    call ==
                        LegacyBattleTargetSelectionRuntimeCall::
                            query_group_b_completion) {
                    fixture.metrics.group_b_count = 2U;
                    fixture.frame.target_cursor = 1U;
                }
            };
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleGroupBTargetCycleStatus::completed &&
                fixture.frame.target_cursor == 2U &&
                fixture.frame.target_actor_index == 3U &&
                result.target_order_reads == 1U &&
                fixture.final_actor.published_actor_code == 4U,
            "the scan reloads live count and cursor after the current-target query"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 8U;
        fixture.metrics.group_b_count = 9U;
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {.entry_edx = 0x77U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBTargetCycleStatus::
                        group_b_actor_typed_stop &&
                result.port_calls == 0U && result.return_eax == 0xAC8U &&
                result.return_ecx == group_b_token(8U) &&
                result.return_edx == 0x77U,
            "initial index eight stops at the first real group-B query after preserving its distinct EAX shape"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 8U;
        fixture.metrics.group_b_count = 9U;
        fixture.port.replies = {
            LegacyBattleTargetSelectionRuntimeCallReply{
                .eax = 1U, .ecx = 0x33U, .edx = 0x44U
            },
        };
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBTargetCycleStatus::
                        target_order_typed_stop &&
                result.port_calls == 1U && result.target_order_reads == 0U &&
                fixture.frame.target_cursor == 9U &&
                fixture.frame.target_actor_index == 0U &&
                result.return_eax == 9U && result.return_ecx == 9U &&
                result.return_edx == 0x44U,
            "one-based order index nine stops only after cursor publication and live-count reload"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 0U;
        fixture.metrics.group_b_count = 2U;
        fixture.runtime.target_actor_indices[1U] = 8U;
        fixture.port.replies = {
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U},
        };
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBTargetCycleStatus::
                        group_b_actor_typed_stop &&
                result.port_calls == 1U && result.target_order_reads == 1U &&
                fixture.frame.target_actor_index == 8U &&
                result.return_eax == 0x2B28U &&
                result.return_ecx == group_b_token(8U) &&
                result.return_edx == 0xAC8U,
            "invalid loop candidate stops at its first query after order and current-index publication"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 0U;
        fixture.frame.target_cursor = 0U;
        fixture.metrics.group_b_count = 1U;
        fixture.runtime.target_actor_indices[1U] = 8U;
        fixture.port.replies = {
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 1U},
        };
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBTargetCycleStatus::
                        group_b_actor_typed_stop &&
                result.port_calls == 1U && fixture.message == 1U &&
                fixture.frame.target_actor_index == 8U &&
                result.return_eax == 8U &&
                result.return_ecx == group_b_token(8U) &&
                result.return_edx == 0x2B28U,
            "exhausted invalid final candidate sets the exhaustion gate then stops at the reset call"
        );
    }

    {
        Fixture fixture;
        fixture.frame.target_actor_index = 1U;
        fixture.metrics.group_b_count = 2U;
        fixture.port.replies.resize(2U);
        fixture.port.replies[0U] =
            LegacyBattleTargetSelectionRuntimeCallReply{.eax = 0U};
        fixture.port.after_call =
            [&](const std::size_t index,
                const LegacyBattleTargetSelectionRuntimeCall call) {
                if (index == 1U &&
                    call ==
                        LegacyBattleTargetSelectionRuntimeCall::
                            reset_actor_selection) {
                    fixture.frame.target_actor_index = 4U;
                }
            };
        const auto result = cycle_legacy_battle_group_b_target(
            fixture.bindings(), fixture.port, {}
        );
        test.expect_true(
            result.status == LegacyBattleGroupBTargetCycleStatus::completed &&
                fixture.port.calls[1U].actor_token == group_b_token(1U) &&
                fixture.final_actor.published_actor_code == 5U &&
                result.return_eax == 5U,
            "publication reloads the live current index after the reset callee"
        );
    }
}
