#include "openswd3/battle/legacy_battle_pair_transition.hpp"

#include "test.hpp"

#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattlePairTransitionCall;
using openswd3::battle::LegacyBattlePairTransitionCallReply;
using openswd3::battle::LegacyBattlePairTransitionCallRequest;
using openswd3::battle::LegacyBattlePairTransitionPort;
using openswd3::compat::u32;

class PairPort final : public LegacyBattlePairTransitionPort {
public:
    std::vector<LegacyBattlePairTransitionCallRequest> calls;
    std::deque<LegacyBattlePairTransitionCallReply> replies;

    [[nodiscard]] LegacyBattlePairTransitionCallReply invoke_pair_transition(
        const LegacyBattlePairTransitionCallRequest& request
    ) override {
        calls.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }
};

[[nodiscard]] LegacyBattlePairTransitionCallReply reply(const u32 eax = 0U) {
    return {.eax = eax};
}

}  // namespace

void test_battle_pair_transition(openswd3::test::Context& test) {
    {
        PairPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                    .eax = 0xAAAAAAAAU,
                    .ecx = 0xBBBBBBBBU,
                    .edx = 0xCCCCCCCCU,
                }
            );
        test.expect_true(
            result.primary_value_was_zero && result.port_calls == 0U &&
                result.return_eax == 0xAAAAAAAAU &&
                result.return_ecx == 0xBBBBBBBBU &&
                result.return_edx == 0xCCCCCCCCU && port.calls.empty(),
            "zero primary value preserves entry registers and calls no object"
        );
    }

    {
        PairPort port;
        port.battle_pair_primary_value() = 1U;
        port.effect_shift_state().packed_reward = 0xBBBB1234U;
        port.replies.push_back({
            .eax = 0xABCD0003U,
            .ecx = 0x12345678U,
            .edx = 0x87654321U,
            .publish_primary_value = true,
            .primary_value = 9U,
            .publish_secondary_value = true,
            .secondary_value = 0xFF80U,
            .publish_packed_reward_high = true,
            .packed_reward_high = 0xAAAAU,
        });
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                }
            );
        test.expect_true(
            result.transition_kind == 3U && result.port_calls == 1U &&
                result.return_eax == 0xABCD0003U &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0x87654321U &&
                port.battle_pair_primary_value() == 9U &&
                port.battle_pair_secondary_value() == 0xFF80U &&
                port.effect_shift_state().packed_reward == 0xAAAA1234U,
            "unrecognized low word returns query registers while preserving callee shared-state side effects"
        );
    }

    {
        PairPort port;
        port.battle_pair_primary_value() = 5U;
        port.replies.push_back({
            .eax = 0xABCD0001U,
            .publish_primary_value = true,
            .primary_value = 123U,
        });
        port.replies.push_back(reply());
        port.replies.push_back(reply());
        port.replies.push_back(reply());
        port.replies.push_back({.eax = 7U, .ecx = 8U, .edx = 9U});
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                }
            );
        test.expect_true(
            result.transition_kind == 1U && result.port_calls == 5U &&
                port.calls[0].ecx == 0x11111111U &&
                port.calls[1].call ==
                    LegacyBattlePairTransitionCall::publish_action_id &&
                port.calls[1].ecx == 0x11111111U &&
                port.calls[1].arguments[0] == 0x246FU &&
                port.calls[2].arguments[0] == 0xFFFFFFFBU &&
                port.calls[3].arguments[0] == 1U &&
                port.calls[4].arguments[0] == 0xFFFFFFFBU &&
                port.calls[4].arguments[1] == 0U &&
                port.calls[4].arguments[2] == 0U &&
                port.battle_pair_primary_value() == 123U &&
                result.return_eax == 7U && result.return_ecx == 8U &&
                result.return_edx == 9U,
            "kind one negates the entry snapshot despite query side effects and returns commit registers"
        );
    }

    {
        PairPort port;
        port.battle_pair_primary_value() = 10U;
        port.battle_pair_secondary_value() = 0x7777U;
        port.replies.push_back(reply(0xABCD0002U));
        port.replies.push_back({
            .outputs = {0xAAAAU, 0xFFFDU},
            .output_write_mask = 3U,
        });
        for (u32 index = 0U; index < 6U; ++index) {
            port.replies.push_back(reply());
        }
        port.replies.push_back(
            {.eax = 0x11111111U, .ecx = 0x22222222U, .edx = 0x33333333U}
        );
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                }
            );
        test.expect_true(
            result.mode_two_path && result.port_calls == 9U &&
                port.calls[2].call ==
                    LegacyBattlePairTransitionCall::publish_value &&
                port.calls[2].object_token == 0x22222222U &&
                port.calls[2].arguments[0] == 0xFFFFFFFDU &&
                port.calls[3].arguments[0] == 0x235EU &&
                port.calls[5].arguments[0] == 0x2367U &&
                port.calls[8].arguments[0] == 0U &&
                port.calls[8].arguments[1] == 0xFFFFFFFDU &&
                port.calls[8].arguments[2] == 0U &&
                port.battle_pair_secondary_value() == 3U &&
                port.battle_pair_primary_value() == 0U &&
                result.secondary_value_published &&
                result.primary_value_cleared &&
                result.return_eax == 0x11111111U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U,
            "kind two sign extends candidate publishes nonpositive replacement and stores negated auxiliary"
        );
    }

    {
        PairPort port;
        port.battle_pair_primary_value() = 10U;
        port.replies.push_back(reply(2U));
        port.replies.push_back({
            .outputs = {0xBBBBU, 20U},
            .output_write_mask = 3U,
        });
        for (u32 index = 0U; index < 6U; ++index) {
            port.replies.push_back(reply());
        }
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                }
            );
        test.expect_true(
            result.port_calls == 8U &&
                port.calls[2].call ==
                    LegacyBattlePairTransitionCall::publish_action_id &&
                port.calls[5].call ==
                    LegacyBattlePairTransitionCall::publish_value &&
                port.calls[5].arguments[0] == 10U &&
                port.calls[7].arguments[1] == 10U &&
                port.battle_pair_secondary_value() == 0xFFF6U &&
                port.battle_pair_primary_value() == 0U,
            "positive signed delta keeps entry value and skips secondary publish"
        );
    }

    {
        PairPort port;
        port.battle_pair_primary_value() = 7U;
        port.effect_shift_state().packed_reward = 0xAAAA1234U;
        port.replies.push_back(reply(0xABCD0004U));
        port.replies.push_back({
            .outputs = {0xCCCCU, 2U},
            .output_write_mask = 3U,
        });
        for (u32 index = 0U; index < 6U; ++index) {
            port.replies.push_back(reply());
        }
        port.replies.push_back({.eax = 0x44U, .ecx = 0x55U, .edx = 0x66U});
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                }
            );
        test.expect_true(
            result.mode_four_path && result.port_calls == 9U &&
                port.calls[3].arguments[0] == 0x235EU &&
                port.calls[5].arguments[0] == 0x2366U &&
                port.calls[8].arguments[0] == 0U &&
                port.calls[8].arguments[1] == 0U &&
                port.calls[8].arguments[2] == 2U &&
                port.battle_pair_primary_value() == 0U &&
                port.effect_shift_state().packed_reward == 0xFFFE1234U &&
                result.packed_reward_high_published &&
                result.return_eax == 0x44U && result.return_ecx == 0x55U &&
                result.return_edx == 0x66U,
            "kind four publishes third commit argument clears primary and replaces only packed high word"
        );
    }

    {
        PairPort port;
        port.battle_pair_primary_value() = 5U;
        port.replies.push_back(reply(4U));
        port.replies.push_back({});
        for (u32 index = 0U; index < 6U; ++index) {
            port.replies.push_back(reply());
        }
        port.replies.push_back(reply());
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = 0x11111111U,
                    .secondary_object_token = 0x22222222U,
                }
            );
        test.expect_true(
            result.port_calls == 9U && port.calls[2].arguments[0] == 0U &&
                port.calls[8].arguments[2] == 0U &&
                port.effect_shift_state().packed_reward == 0U,
            "unwritten query outputs preserve zero initialized locals"
        );
    }
}
