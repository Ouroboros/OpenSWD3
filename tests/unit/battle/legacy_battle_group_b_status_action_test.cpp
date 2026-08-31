#include "openswd3/battle/legacy_battle_group_b_status_action.hpp"
#include "test.hpp"

#include <array>
#include <deque>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        const u32 value = values.front();
        values.pop_front();
        if (bounds.size() == mutate_on_call && actor_to_mutate != nullptr) {
            actor_to_mutate->resource_token = replacement_resource_token;
            actor_to_mutate->resource_bytes[0x91U] = replacement_chance;
        }

        return value;
    }

    void push(const u32 value) {
        values.push_back(value);
    }

    std::deque<u32> values;
    std::vector<u32> bounds;
    openswd3::battle::LegacyBattleActorGroupBElementState* actor_to_mutate{};
    std::size_t mutate_on_call{};
    u32 replacement_resource_token{};
    u8 replacement_chance{};
};

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void prepare_actor(
    openswd3::battle::LegacyBattleActorGroupBElementState& actor,
    const u16 base,
    const u8 chance = 1U
) {
    actor.resource_token = 0x73001234U;
    write_word(actor.resource_bytes, 0x54U, base);
    actor.resource_bytes[0x91U] = chance;
}

}  // namespace

void test_battle_group_b_status_action(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBStatusActionStatus;

    {
        RandomPort random;
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                nullptr,
                random,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0x11223344U,
                    .entry_edx = 0xAABBCC7AU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBStatusActionStatus::
                        actor_state_typed_stop &&
                result.random_calls == 0U && result.argument == 0x7AU &&
                result.return_eax == 0x11223344U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0xAABBCC7AU && result.return_ecx_known &&
                random.bounds.empty(),
            "status action stops at the entry actor flag read before random"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_execution.retreat_ready_flags = 0x0800U;
        RandomPort random;
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor,
                random,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0x11223344U,
                    .entry_edx = 0xAABBCC7AU,
                }
            );
        test.expect_true(
            result.status == LegacyBattleGroupBStatusActionStatus::completed &&
                result.random_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0x00525508U &&
                result.return_edx == 0xAABBCC7AU && random.bounds.empty(),
            "actor flag bit eleven returns false without consuming random"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        RandomPort random;
        random.push(0xCAFE000BU);
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBStatusActionStatus::
                        resource_read_typed_stop &&
                result.random_calls == 1U &&
                result.initial_random_value == 0xCAFE000BU &&
                result.return_eax == 0U && result.return_edx == 0xCAFE000BU &&
                !result.return_ecx_known &&
                random.bounds == std::vector<u32>{12U},
            "initial random is consumed before the first resource byte access"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor, 0x1234U, 0U);
        RandomPort random;
        random.push(0xABCD0007U);
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            result.status == LegacyBattleGroupBStatusActionStatus::completed &&
                result.random_calls == 1U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0xABCD0007U &&
                !result.return_ecx_known &&
                random.bounds == std::vector<u32>{12U},
            "zero resource chance returns false after only the discarded draw"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor, 0x0100U);
        RandomPort random;
        random.push(0xABCD0007U);
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            result.return_eax == 0U && result.random_calls == 1U &&
                result.signed_delta == -134 &&
                result.resource_base == 0x0100U && result.return_ecx == 1U &&
                result.return_edx == 0x0100U &&
                random.bounds == std::vector<u32>{12U},
            "argument subtraction is interpreted as signed before the lower band"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor, 0x0010U);
        RandomPort random;
        random.push(0xDEAD000BU);
        random.push(0xBEEF0007U);
        const auto below =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        random.push(0xAAAA0000U);
        random.push(0x12340008U);
        const auto equal =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            below.signed_delta == 106 && below.decision_threshold == 8U &&
                below.return_eax == 1U && below.return_edx == 0xBEEF0007U &&
                !below.return_ecx_known && equal.return_eax == 0U &&
                equal.return_edx == 0x12340008U &&
                random.bounds == std::vector<u32>{12U, 10U, 12U, 10U},
            "upper band uses low-word random below eight and rejects equality"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor, 0x0070U);
        RandomPort random;
        random.push(0U);
        random.push(4U);
        const auto below =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        random.push(0U);
        random.push(5U);
        const auto equal =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            below.signed_delta == 10 && below.decision_threshold == 5U &&
                below.return_eax == 1U && equal.return_eax == 0U &&
                random.bounds == std::vector<u32>{12U, 10U, 12U, 10U},
            "middle band uses low-word random below five and rejects equality"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor, 0x0075U, 1U);
        RandomPort random;
        random.push(0xAAAA0000U);
        random.push(0xBEEF0006U);
        random.actor_to_mutate = &actor;
        random.mutate_on_call = 2U;
        random.replacement_resource_token = 0x73005678U;
        random.replacement_chance = 7U;
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            result.status == LegacyBattleGroupBStatusActionStatus::completed &&
                result.signed_delta == 5 && result.random_calls == 2U &&
                result.initial_resource_chance == 1U &&
                result.decision_threshold == 7U && result.return_eax == 1U &&
                result.return_ecx == 0x73005678U &&
                result.return_edx == 0xBEEF0007U && result.return_ecx_known &&
                random.bounds == std::vector<u32>{12U, 10U},
            "equal-five band re-reads resource token and chance after random"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor, 0x0075U, 9U);
        RandomPort random;
        random.push(0U);
        random.push(0xCAFE0008U);
        random.actor_to_mutate = &actor;
        random.mutate_on_call = 2U;
        random.replacement_resource_token = 0U;
        const auto result =
            openswd3::battle::query_legacy_battle_group_b_status_action(
                &actor, random, {.actor_token = 0x00525508U, .entry_edx = 0x7AU}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBStatusActionStatus::
                        resource_reread_typed_stop &&
                result.random_calls == 2U &&
                result.decision_random_value == 0xCAFE0008U &&
                result.return_eax == 0xCAFE0008U && result.return_ecx == 0U &&
                result.return_edx == 0xCAFE0008U && result.return_ecx_known &&
                random.bounds == std::vector<u32>{12U, 10U},
            "equal-five band stops at the post-random resource re-read"
        );
    }
}
