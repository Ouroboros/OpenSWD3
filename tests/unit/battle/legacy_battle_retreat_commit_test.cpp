#include "openswd3/battle/legacy_battle_retreat_commit.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleRetreatCommitCall;
using openswd3::battle::LegacyBattleRetreatCommitCallReply;
using openswd3::battle::LegacyBattleRetreatCommitCallRequest;
using openswd3::battle::LegacyBattleRetreatCommitPort;
using openswd3::compat::i32;
using openswd3::compat::u32;

class FinalizationPort final : public LegacyBattleRetreatCommitPort {
public:
    [[nodiscard]] LegacyBattleRetreatCommitCallReply invoke_retreat_commit(
        const LegacyBattleRetreatCommitCallRequest& request
    ) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request, calls.size());
        }
        if (request.call ==
            LegacyBattleRetreatCommitCall::text_message_allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token, .edx = request.edx};
        }
        if (request.call ==
            LegacyBattleRetreatCommitCall::text_message_measure) {
            return {.eax = 4U};
        }
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    [[nodiscard]] i32 battle_sample_mix_level() const noexcept override {
        return mix_level;
    }

    std::vector<LegacyBattleRetreatCommitCallRequest> calls;
    std::deque<LegacyBattleRetreatCommitCallReply> replies;
    std::function<
        void(const LegacyBattleRetreatCommitCallRequest&, std::size_t)>
        on_call;
    i32 mix_level{6};
    u32 next_text_message_token{0x77000000U};
};

}  // namespace

void test_battle_retreat_commit(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleRetreatCommitBranch;
    using openswd3::battle::LegacyBattleRetreatCommitBindings;
    using openswd3::battle::commit_legacy_battle_retreat;

    {
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        actor.retreat_ready_flags = 0x0800U;
        const auto blocked =
            openswd3::battle::query_legacy_battle_actor_retreat_ready(
                &actor,
                {
                    .actor_token = 0x005029D0U,
                    .entry_eax = 0xA5A50000U,
                    .entry_edx = 0x11223344U,
                }
            );
        actor.retreat_ready_flags = 0U;
        const auto ready =
            openswd3::battle::query_legacy_battle_actor_retreat_ready(
                &actor,
                {
                    .actor_token = 0x005029D0U,
                    .entry_eax = 0x5A5AFFFFU,
                    .entry_edx = 0x55667788U,
                }
            );
        const auto stopped =
            openswd3::battle::query_legacy_battle_actor_retreat_ready(
                nullptr,
                {
                    .actor_token = 0x005029D0U,
                    .entry_eax = 0x12345678U,
                    .entry_edx = 0x9ABCDEF0U,
                }
            );
        test.expect_true(
            blocked.return_eax == 0U && ready.return_eax == 1U &&
                ready.return_ecx == 0x005029D0U &&
                ready.return_edx == 0x55667788U &&
                stopped.status == openswd3::battle::
                    LegacyBattleActorRetreatReadyStatus::
                        actor_state_typed_stop &&
                stopped.return_eax == 0x12345678U &&
                stopped.return_ecx == 0x005029D0U &&
                stopped.return_edx == 0x9ABCDEF0U,
            "retreat ready queries inverted bit eleven while preserving ECX EDX and the original access stop"
        );
    }

    {
        FinalizationPort port;
        port.replies.push_back({
            .eax = 2U,
            .ecx = 0x11223344U,
            .edx = 0x55667788U,
        });
        u32 packed_counter = 0xAABBCCDDU;

        const auto result = commit_legacy_battle_retreat(
            LegacyBattleRetreatCommitBindings{packed_counter}, port, 0xFFFFFFFFU
        );

        test.expect_true(
            result.branch ==
                    LegacyBattleRetreatCommitBranch::selected_actor_not_ready &&
                result.selected_object_token == 0x005029D0U - 0x00002F34U &&
                result.return_value == 2U && result.final_ecx == 0x11223344U &&
                result.final_edx == 0x55667788U && result.port_calls == 1U &&
                port.calls.size() == 1U &&
                port.calls[0].object_token == result.selected_object_token &&
                packed_counter == 0xAABBCCDDU,
            "selected actor index uses wrapping stride arithmetic and any readiness return other than exact one exits unchanged"
        );
    }

    {
        FinalizationPort port;
        port.mix_level = -7;
        port.replies = {
            {.eax = 1U, .edx = 0x87654321U},
            {.eax = 0x22222222U, .ecx = 0x33333333U, .edx = 0x44444444U},
        };
        auto& state = port.retreat_commit_state();
        state = {9U, 9U, 9U, 9U};
        port.actor_metric_state().group_b_count = 5U;
        port.battle_debug_hotkey_state().committed_actor_code = 9U;
        port.battle_debug_overlay_gate() = 9U;
        port.outcome_resolution_state().resolution_latch = 9U;
        port.outcome_resolution_state().darkening_gate = 9U;
        port.battle_message_state() = 9U;
        u32 packed_counter = 0xAABBCCDDU;
        openswd3::battle::LegacyBattleTextMessageState text_messages;
        u32 text_message_head = 0U;
        std::array<openswd3::battle::LegacyBattleGroupAActionExecutionState, 10>
            actors{};
        actors[0U].retreat_ready_flags = 0x0800U;

        const auto result = commit_legacy_battle_retreat(
            {packed_counter, &text_messages, &text_message_head, actors},
            port,
            3U
        );

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::warning &&
                !result.mode_bit_blocked && result.port_calls == 4U &&
                result.primary_actor_calls == 1U &&
                result.return_value == 0x22222222U &&
                result.final_ecx == 0x33333333U &&
                result.final_edx == 0x44444444U &&
                result.warning_text.appended &&
                text_message_head == 0x77000000U &&
                text_messages.allocations[0U].record.value_04 == 0x118U &&
                text_messages.allocations[0U].record.kind == 50U &&
                port.calls[3].call ==
                    LegacyBattleRetreatCommitCall::play_warning_sample &&
                port.calls[3].arguments[0] == 0x8CU &&
                port.calls[3].arguments[1] == 0xFFFFFFF9U &&
                state.completion_gate_a == 9U &&
                state.completion_gate_b == 9U && state.auxiliary_latch == 9U &&
                state.selected_actor_token == 9U &&
                packed_counter == 0xAABBCCDDU,
            "zero primary actor query displays the fixed warning then plays sample with the live signed mix level and publishes no state"
        );
    }

    {
        FinalizationPort port;
        port.replies = {{.eax = 1U}, {}};
        port.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x200U;
        u32 packed_counter = 0U;
        openswd3::battle::LegacyBattleTextMessageState text_messages;
        u32 text_message_head = 0U;
        std::array<openswd3::battle::LegacyBattleGroupAActionExecutionState, 10>
            actors{};

        const auto result = commit_legacy_battle_retreat(
            {packed_counter, &text_messages, &text_message_head, actors},
            port,
            0U
        );

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::warning &&
                result.mode_bit_blocked && result.port_calls == 4U,
            "battle mode bit nine forces the same warning branch after a nonzero primary actor query"
        );
    }

    {
        FinalizationPort port;
        port.replies = {
            {.eax = 1U, .ecx = 0x01020304U, .edx = 0x05060708U},
        };
        auto& state = port.retreat_commit_state();
        state = {9U, 9U, 9U, 9U};
        port.actor_metric_state().group_b_count = 0x1234U;
        port.battle_debug_hotkey_state().committed_actor_code = 9U;
        port.battle_debug_overlay_gate() = 9U;
        port.outcome_resolution_state().resolution_latch = 9U;
        port.outcome_resolution_state().darkening_gate = 9U;
        port.battle_message_state() = 9U;
        u32 packed_counter = 0xA1B2C3D4U;
        std::array<openswd3::battle::LegacyBattleGroupAActionExecutionState, 10>
            actors{};

        const auto result = commit_legacy_battle_retreat(
            {packed_counter, nullptr, nullptr, actors}, port, 2U
        );

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::committed &&
                result.state_committed && result.port_calls == 1U &&
                result.primary_actor_calls == 1U &&
                result.return_value == 0U && result.final_ecx == 0x00502934U &&
                result.final_edx == 0x05060708U &&
                state.completion_gate_a == 1U &&
                state.completion_gate_b == 1U &&
                port.outcome_resolution_state().resolution_latch == 0U &&
                state.auxiliary_latch == 0U &&
                port.battle_debug_hotkey_state().committed_actor_code == 0U &&
                port.battle_debug_overlay_gate() == 0U &&
                state.selected_actor_token == 0xFFFFFFFFU &&
                port.battle_message_state() == 0U &&
                packed_counter == 0xA1B2C334U &&
                port.outcome_resolution_state().darkening_gate == 1U,
            "successful finalization publishes the nine shared states in original width semantics and returns zero with live group-B count in CL"
        );
    }

    {
        FinalizationPort port;
        port.replies = {{.eax = 1U}, {}};
        port.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x200U;
        port.actor_metric_state().group_b_count = 0x77U;
        u32 packed_counter = 0x12345678U;
        openswd3::battle::LegacyBattleTextMessageState text_messages;
        u32 text_message_head = 0U;
        std::array<openswd3::battle::LegacyBattleGroupAActionExecutionState, 10>
            actors{};

        const auto result = commit_legacy_battle_retreat(
            {packed_counter, &text_messages, &text_message_head, actors},
            port,
            1U
        );

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::warning &&
                result.mode_bit_blocked && packed_counter == 0x12345678U &&
                std::ranges::none_of(port.calls, [](const auto& call) {
                    return call.call == LegacyBattleRetreatCommitCall::
                        reserved_query_primary_actor_state_slot;
                }),
            "mode flags are read after the typed primary actor query without the reserved opaque slot"
        );
    }
}
