#include "openswd3/battle/legacy_battle_retreat_commit.hpp"
#include "test.hpp"

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
};

}  // namespace

void test_battle_retreat_commit(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleRetreatCommitBranch;
    using openswd3::battle::LegacyBattleRetreatCommitBindings;
    using openswd3::battle::commit_legacy_battle_retreat;

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
            {.eax = 1U},
            {.eax = 0U, .ecx = 0x12345678U, .edx = 0x87654321U},
            {.eax = 0x11111111U},
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

        const auto result =
            commit_legacy_battle_retreat({packed_counter}, port, 3U);

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::warning &&
                !result.mode_bit_blocked && result.port_calls == 4U &&
                result.return_value == 0x22222222U &&
                result.final_ecx == 0x33333333U &&
                result.final_edx == 0x44444444U &&
                port.calls[2].call ==
                    LegacyBattleRetreatCommitCall::display_warning &&
                port.calls[2].arguments ==
                    std::array<u32, 5>{
                        0x118U, 10U, 50U, 0x004A7954U, 0x40000002U
                    } &&
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
        port.replies = {{.eax = 1U}, {.eax = 7U}, {}, {}};
        port.battle_debug_hotkey_state().battle_mode_flags_53bc24 = 0x200U;
        u32 packed_counter = 0U;

        const auto result =
            commit_legacy_battle_retreat({packed_counter}, port, 0U);

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
            {.eax = 7U, .ecx = 0xAABBCCDDU, .edx = 0x11223344U},
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

        const auto result =
            commit_legacy_battle_retreat({packed_counter}, port, 2U);

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::committed &&
                result.state_committed && result.port_calls == 2U &&
                result.return_value == 0U && result.final_ecx == 0xAABBCC34U &&
                result.final_edx == 0x11223344U &&
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
        port.replies = {{.eax = 1U}, {.eax = 1U}, {}, {}};
        port.on_call = [&](const auto& request, const std::size_t call_count) {
            if (request.call ==
                    LegacyBattleRetreatCommitCall::query_primary_actor_state &&
                call_count == 2U) {
                port.battle_debug_hotkey_state().battle_mode_flags_53bc24 =
                    0x200U;
                port.actor_metric_state().group_b_count = 0x77U;
            }
        };
        u32 packed_counter = 0x12345678U;

        const auto result =
            commit_legacy_battle_retreat({packed_counter}, port, 1U);

        test.expect_true(
            result.branch == LegacyBattleRetreatCommitBranch::warning &&
                result.mode_bit_blocked && packed_counter == 0x12345678U,
            "mode flags are read after the primary actor callee rather than cached at entry"
        );
    }
}
