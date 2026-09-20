#include "openswd3/battle/legacy_battle_pair_transition.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
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

struct ActorResources {
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleStartupState startup;

    ActorResources() {
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }

    [[nodiscard]] openswd3::battle::
        LegacyBattleActorEffectResourceSlotWriteOwners
        owners() noexcept {
        return {.action = &action, .startup = &startup};
    }
};

inline constexpr u32 kPrimaryActorToken =
    openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken;
inline constexpr u32 kSecondaryActorToken =
    openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken;

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
        ActorResources resources;
        port.battle_pair_primary_value() = 5U;
        port.replies.push_back({
            .eax = 0xABCD0001U,
            .publish_primary_value = true,
            .primary_value = 123U,
        });
        port.replies.push_back(reply());
        port.replies.push_back(reply());
        port.replies.push_back({.eax = 7U, .ecx = 8U, .edx = 9U});
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = kPrimaryActorToken,
                    .secondary_object_token = kSecondaryActorToken,
                    .effect_resource_slot_write_owners = resources.owners(),
                }
            );
        test.expect_true(
            result.transition_kind == 1U && result.port_calls == 4U &&
                port.calls[0].ecx == kPrimaryActorToken &&
                result.effect_resource_slot_write.calls == 1U &&
                result.effect_resource_slot_write.call_addresses[0U] ==
                    0x0045D6C7U &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_slots[0U] == 0x246FU &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_cursor == 1U &&
                port.calls[1].arguments[0] == 0xFFFFFFFBU &&
                port.calls[2].arguments[0] == 1U &&
                port.calls[3].arguments[0] == 0xFFFFFFFBU &&
                port.calls[3].arguments[1] == 0U &&
                port.calls[3].arguments[2] == 0U &&
                port.battle_pair_primary_value() == 123U &&
                result.return_eax == 7U && result.return_ecx == 8U &&
                result.return_edx == 9U,
            "kind one writes the typed action resource, negates the entry snapshot, and returns commit registers"
        );
    }

    {
        PairPort port;
        ActorResources resources;
        port.battle_pair_primary_value() = 10U;
        port.battle_pair_secondary_value() = 0x7777U;
        port.replies.push_back(reply(0xABCD0002U));
        port.replies.push_back({
            .outputs = {0xAAAAU, 0xFFFDU},
            .output_write_mask = 3U,
        });
        for (u32 index = 0U; index < 4U; ++index) {
            port.replies.push_back(reply());
        }
        port.replies.push_back(
            {.eax = 0x11111111U, .ecx = 0x22222222U, .edx = 0x33333333U}
        );
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = kPrimaryActorToken,
                    .secondary_object_token = kSecondaryActorToken,
                    .effect_resource_slot_write_owners = resources.owners(),
                }
            );
        test.expect_true(
            result.mode_two_path && result.port_calls == 7U &&
                port.calls[2].call ==
                    LegacyBattlePairTransitionCall::publish_value &&
                port.calls[2].object_token == kSecondaryActorToken &&
                port.calls[2].arguments[0] == 0xFFFFFFFDU &&
                result.effect_resource_slot_write.calls == 2U &&
                result.effect_resource_slot_write.call_addresses[0U] ==
                    0x0045D72FU &&
                result.effect_resource_slot_write.call_addresses[1U] ==
                    0x0045D744U &&
                (*resources.startup.group_b_lifecycle)[0U]
                        .action_execution.effect_resource_slots[0U] ==
                    0x235EU &&
                (*resources.startup.group_b_lifecycle)[0U]
                        .action_execution.effect_resource_cursor == 1U &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_slots[0U] == 0x2367U &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_cursor == 1U &&
                port.calls[6].arguments[0] == 0U &&
                port.calls[6].arguments[1] == 0xFFFFFFFDU &&
                port.calls[6].arguments[2] == 0U &&
                port.battle_pair_secondary_value() == 3U &&
                port.battle_pair_primary_value() == 0U &&
                result.secondary_value_published &&
                result.primary_value_cleared &&
                result.return_eax == 0x11111111U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U,
            "kind two sign extends candidate, writes both typed resources, and stores the negated auxiliary"
        );
    }

    {
        PairPort port;
        ActorResources resources;
        port.battle_pair_primary_value() = 10U;
        port.replies.push_back(reply(2U));
        port.replies.push_back({
            .outputs = {0xBBBBU, 20U},
            .output_write_mask = 3U,
        });
        for (u32 index = 0U; index < 4U; ++index) {
            port.replies.push_back(reply());
        }
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = kPrimaryActorToken,
                    .secondary_object_token = kSecondaryActorToken,
                    .effect_resource_slot_write_owners = resources.owners(),
                }
            );
        test.expect_true(
            result.port_calls == 6U &&
                result.effect_resource_slot_write.calls == 2U &&
                result.effect_resource_slot_write.call_addresses[0U] ==
                    0x0045D72FU &&
                result.effect_resource_slot_write.call_addresses[1U] ==
                    0x0045D744U &&
                (*resources.startup.group_b_lifecycle)[0U]
                        .action_execution.effect_resource_slots[0U] ==
                    0x235EU &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_slots[0U] == 0x2367U &&
                port.calls[3].call ==
                    LegacyBattlePairTransitionCall::publish_value &&
                port.calls[3].arguments[0] == 10U &&
                port.calls[5].arguments[1] == 10U &&
                port.battle_pair_secondary_value() == 0xFFF6U &&
                port.battle_pair_primary_value() == 0U,
            "positive signed delta keeps the entry value, writes both typed resources, and skips secondary publish"
        );
    }

    {
        PairPort port;
        ActorResources resources;
        port.battle_pair_primary_value() = 7U;
        port.effect_shift_state().packed_reward = 0xAAAA1234U;
        port.replies.push_back(reply(0xABCD0004U));
        port.replies.push_back({
            .outputs = {0xCCCCU, 2U},
            .output_write_mask = 3U,
        });
        for (u32 index = 0U; index < 4U; ++index) {
            port.replies.push_back(reply());
        }
        port.replies.push_back({.eax = 0x44U, .ecx = 0x55U, .edx = 0x66U});
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = kPrimaryActorToken,
                    .secondary_object_token = kSecondaryActorToken,
                    .effect_resource_slot_write_owners = resources.owners(),
                }
            );
        test.expect_true(
            result.mode_four_path && result.port_calls == 7U &&
                result.effect_resource_slot_write.calls == 2U &&
                result.effect_resource_slot_write.call_addresses[0U] ==
                    0x0045D7B5U &&
                result.effect_resource_slot_write.call_addresses[1U] ==
                    0x0045D7CAU &&
                (*resources.startup.group_b_lifecycle)[0U]
                        .action_execution.effect_resource_slots[0U] ==
                    0x235EU &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_slots[0U] == 0x2366U &&
                port.calls[6].arguments[0] == 0U &&
                port.calls[6].arguments[1] == 0U &&
                port.calls[6].arguments[2] == 2U &&
                port.battle_pair_primary_value() == 0U &&
                port.effect_shift_state().packed_reward == 0xFFFE1234U &&
                result.packed_reward_high_published &&
                result.return_eax == 0x44U && result.return_ecx == 0x55U &&
                result.return_edx == 0x66U,
            "kind four writes both typed resources, clears primary, and replaces only the packed high word"
        );
    }

    {
        PairPort port;
        ActorResources resources;
        port.battle_pair_primary_value() = 5U;
        port.replies.push_back(reply(4U));
        port.replies.push_back({});
        for (u32 index = 0U; index < 4U; ++index) {
            port.replies.push_back(reply());
        }
        port.replies.push_back(reply());
        const auto result =
            openswd3::battle::advance_legacy_battle_pair_transition(
                port,
                {
                    .primary_object_token = kPrimaryActorToken,
                    .secondary_object_token = kSecondaryActorToken,
                    .effect_resource_slot_write_owners = resources.owners(),
                }
            );
        test.expect_true(
            result.port_calls == 7U &&
                result.effect_resource_slot_write.calls == 2U &&
                (*resources.startup.group_b_lifecycle)[0U]
                        .action_execution.effect_resource_slots[0U] ==
                    0x235EU &&
                resources.action.group_a_action_execution[0U]
                        .effect_resource_slots[0U] == 0x2366U &&
                port.calls[2].arguments[0] == 0U &&
                port.calls[6].arguments[2] == 0U &&
                port.effect_shift_state().packed_reward == 0U,
            "unwritten query outputs preserve zero initialized locals while typed resources still commit"
        );
    }
}
