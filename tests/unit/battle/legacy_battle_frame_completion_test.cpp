#include "openswd3/battle/legacy_battle_frame_completion.hpp"
#include "test.hpp"

#include <functional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleFrameCompletionCallReply;
using openswd3::battle::LegacyBattleFrameCompletionCallRequest;
using openswd3::battle::LegacyBattleFrameCompletionPort;
using openswd3::compat::u32;

class CompletionPort final : public LegacyBattleFrameCompletionPort {
public:
    [[nodiscard]] LegacyBattleFrameCompletionCallReply invoke_frame_completion(
        const LegacyBattleFrameCompletionCallRequest& request
    ) override {
        calls.push_back(request);
        return on_call ? on_call(request, calls.size()) : default_reply;
    }

    std::vector<LegacyBattleFrameCompletionCallRequest> calls;
    std::function<LegacyBattleFrameCompletionCallReply(
        const LegacyBattleFrameCompletionCallRequest&, std::size_t
    )>
        on_call;
    LegacyBattleFrameCompletionCallReply default_reply{};
};

struct Fixture {
    openswd3::battle::LegacyBattleActorMetricState actors;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleOutcomeResolutionState outcome;
    openswd3::battle::LegacyBattleStartupResetBlocks startup_reset;
    u32 message_state{};
    CompletionPort port;

    [[nodiscard]] openswd3::battle::LegacyBattleFrameCompletionBindings
    bindings() {
        return {
            .actors = actors,
            .final_actor = final_actor,
            .action = action,
            .outcome = outcome,
            .startup_reset = startup_reset,
            .message_state = message_state,
        };
    }
};

}  // namespace

void test_battle_frame_completion(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleFrameCompletionStatus;
    using openswd3::battle::update_legacy_battle_frame_completion;

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 7U;

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(),
            fixture.port,
            0x11112222U,
            0x33334444U,
            0x55556666U
        );

        test.expect_true(
            result.status == LegacyBattleFrameCompletionStatus::completed &&
                result.return_eax == 0U && result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U &&
                result.mask_query_calls == 0U && fixture.port.calls.empty(),
            "a selected current actor returns zero while preserving entry ECX and EDX before both group scans"
        );
    }

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 0xFFFFFFFFU;
        fixture.actors.group_a_count = 3U;
        fixture.port.frame_completion_state()
            .group_a_fields[0]
            .skip_mask_query_a = 1U;
        fixture.final_actor.excluded_group_a_count = 1U;
        fixture.final_actor.removed_group_a_count = 0xFFU;
        fixture.port.on_call = [&](const auto&, const std::size_t) {
            fixture.actors.group_a_count = 2U;
            return LegacyBattleFrameCompletionCallReply{
                .eax = 1U,
                .ecx = 0xAAAABBBBU,
                .edx = 0xCCCCDDDDU,
            };
        };

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(), fixture.port, 0U, 0x12345678U, 0x89ABCDEFU
        );

        test.expect_true(
            result.group_a_committed && !result.group_b_committed &&
                result.return_eax == 1U && result.return_ecx == 0x100U &&
                result.return_edx == 1U && result.group_a_scanned == 2U &&
                result.group_a_ready_count == 1U &&
                result.mask_query_calls == 1U &&
                fixture.port.calls[0].actor_token == 0x00505904U &&
                fixture.port.calls[0].actor_index == 1U &&
                fixture.port.calls[0].actor_group == 1U &&
                fixture.port.calls[0].mask == 4U &&
                fixture.port.calls[0].eax == 3U &&
                fixture.port.calls[0].ecx == 0x00505904U &&
                fixture.port.calls[0].edx == 0x89ABCDEFU &&
                fixture.final_actor.removed_group_a_count == 0U &&
                fixture.message_state == 0x67U,
            "group A skips either exact-one object flag, reloads a shrunken live count after the query, wraps the ready byte, and publishes message 103"
        );
    }

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 0xFFFFFFFFU;
        fixture.actors.group_a_count = 11U;
        for (auto& fields :
             fixture.port.frame_completion_state().group_a_fields) {
            fields.skip_mask_query_b = 1U;
        }

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(), fixture.port, 0U, 0x12345678U, 0x89ABCDEFU
        );

        test.expect_true(
            result.status ==
                    LegacyBattleFrameCompletionStatus::
                        group_a_fields_typed_stop &&
                result.stopped_index == 10U && result.group_a_scanned == 10U &&
                result.mask_query_calls == 0U && result.return_eax == 11U &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0x89ABCDEFU && fixture.message_state == 0U,
            "the eleventh direct group-A field read stops after ten complete skip checks without entering the group-B path"
        );
    }

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 0xFFFFFFFFU;
        fixture.actors.group_b_count = 2U;
        fixture.action.packed_actor_counter = 0xAABBCCFFU;
        fixture.final_actor.terminal_mode = 7U;
        fixture.port.on_call = [](const auto&, const std::size_t call) {
            return call == 1U
                ? LegacyBattleFrameCompletionCallReply{
                      .eax = 1U,
                      .ecx = 0xAAAA0011U,
                      .edx = 0x11112222U,
                  }
                : LegacyBattleFrameCompletionCallReply{
                      .eax = 1U,
                      .ecx = 0x12345678U,
                      .edx = 0x33334444U,
                  };
        };

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(), fixture.port, 0U, 0x01020304U, 0x05060708U
        );

        test.expect_true(
            result.group_b_committed && !result.group_a_committed &&
                result.return_eax == 1U && result.return_ecx == 0x12345602U &&
                result.return_edx == 0x101U && result.group_b_scanned == 2U &&
                result.group_b_ready_count == 2U &&
                result.mask_query_calls == 2U &&
                fixture.port.calls[0].actor_token == 0x00525508U &&
                fixture.port.calls[1].actor_token == 0x00528030U &&
                fixture.action.packed_actor_counter == 0xAABBCC01U &&
                fixture.startup_reset.value_53c048 == 1U &&
                fixture.final_actor.terminal_mode == 0U &&
                fixture.message_state == 0x63U,
            "group B preserves the final query ECX high bytes, compares the full signed count, wraps only the packed low byte, and publishes message 99"
        );
    }

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 0xFFFFFFFFU;
        fixture.actors.group_b_count = 1U;
        fixture.port.default_reply = {
            .eax = 2U,
            .ecx = 0xAABBCCDDU,
            .edx = 0x11223344U,
        };

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(), fixture.port, 0U, 0U, 0U
        );

        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 0xAABBCC00U &&
                result.return_edx == 0x11223344U &&
                result.group_b_ready_count == 0U &&
                fixture.startup_reset.value_53c048 == 0U &&
                fixture.message_state == 0U,
            "only an exact-one mask query counts ready while the zero-ready tail keeps the last callee EDX and replaces only CL"
        );
    }

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 0xFFFFFFFFU;
        fixture.actors.group_b_count = 1U;
        fixture.port.on_call = [&](const auto&, const std::size_t call) {
            if (call == 1U) {
                fixture.actors.group_b_count = 3U;
            }
            return LegacyBattleFrameCompletionCallReply{
                .eax = 0U,
                .ecx = static_cast<u32>(0x44000000U + call),
                .edx = static_cast<u32>(0x55000000U + call),
            };
        };

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(), fixture.port, 0U, 0U, 0U
        );

        test.expect_true(
            result.group_b_scanned == 3U && result.mask_query_calls == 3U &&
                fixture.port.calls[2].actor_token == 0x0052AB58U &&
                result.return_ecx == 0x44000000U &&
                result.return_edx == 0x55000003U,
            "the group-B loop rereads a count grown by the first query and adds no modern iteration cap"
        );
    }

    {
        Fixture fixture;
        fixture.actors.priority_actor_index = 0xFFFFFFFFU;
        fixture.actors.group_a_count = 1U;
        fixture.outcome.darkening_gate = 1U;
        fixture.startup_reset.value_53c048 = 1U;
        fixture.port.default_reply = {.eax = 1U, .ecx = 9U, .edx = 10U};

        const auto result = update_legacy_battle_frame_completion(
            fixture.bindings(), fixture.port, 0U, 0U, 0U
        );

        test.expect_true(
            result.return_eax == 0U && result.return_ecx == 1U &&
                result.return_edx == 1U && !result.group_a_committed &&
                !result.group_b_committed && fixture.message_state == 0U,
            "the shared darkening gate blocks a satisfied group-A threshold before the live group-B completion gate returns zero"
        );
    }
}
