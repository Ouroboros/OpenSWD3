#include "openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp"
#include "test.hpp"

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
        if (actor_to_clear != nullptr) {
            actor_to_clear->resource_token = 0U;
        }

        return value;
    }

    void push(const u32 value) {
        values.push_back(value);
    }

    std::deque<u32> values;
    std::vector<u32> bounds;
    openswd3::battle::LegacyBattleActorGroupBElementState* actor_to_clear{};
};

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

}  // namespace

void test_battle_group_b_reward_item_selection(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBRewardItemSelectionStatus;

    {
        RandomPort random;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                nullptr,
                random,
                {
                    .actor_token = 0x05255508U,
                    .entry_eax = 0x11112222U,
                    .entry_edx = 0x33334444U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBRewardItemSelectionStatus::
                        actor_state_typed_stop &&
                result.random_calls == 0U && result.return_eax == 0x11112222U &&
                result.return_ecx == 0x05255508U &&
                result.return_edx == 0x33334444U && random.bounds.empty(),
            "reward item selection stops on the first actor resource-pointer read without consuming random"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        RandomPort random;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                &actor,
                random,
                {
                    .actor_token = 0x05255508U,
                    .entry_eax = 0x11112222U,
                    .entry_edx = 0x33334444U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBRewardItemSelectionStatus::
                        resource_read_typed_stop &&
                result.random_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0x05255508U &&
                result.return_edx == 0x33334444U && random.bounds.empty(),
            "a zero resource token stops at the original first resource word access"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x73AB1234U;
        RandomPort random;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                &actor,
                random,
                {
                    .actor_token = 0x05255508U,
                    .entry_eax = 0x11112222U,
                    .entry_edx = 0x33334444U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBRewardItemSelectionStatus::completed &&
                result.random_calls == 0U && result.item_id == 0U &&
                result.return_eax == 0x73AB0000U &&
                result.return_ecx == 0x05255508U &&
                result.return_edx == 0x33334444U && random.bounds.empty(),
            "a zero item clears only AX and skips the secondary random stream"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x73001234U;
        write_word(actor.resource_bytes, 0x82U, 0xBEEFU);
        write_word(actor.resource_bytes, 0x84U, 8U);
        RandomPort random;
        random.push(0xCAFE0007U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                &actor,
                random,
                {
                    .actor_token = 0x05255508U,
                    .entry_eax = 0x11112222U,
                    .entry_edx = 0x33334444U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBRewardItemSelectionStatus::completed &&
                result.random_calls == 1U &&
                result.random_value == 0xCAFE0007U &&
                result.item_id == 0xBEEFU && result.threshold == 8U &&
                result.return_eax == 0xCAFEBEEFU &&
                result.return_ecx == actor.resource_token &&
                result.return_edx == 0xCAFE0007U &&
                random.bounds == std::vector<u32>{20U},
            "an unsigned random low word below the threshold publishes the resource item"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x73005678U;
        write_word(actor.resource_bytes, 0x82U, 0x4444U);
        write_word(actor.resource_bytes, 0x84U, 8U);
        RandomPort random;
        random.push(0x12340007U);
        random.actor_to_clear = &actor;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                &actor,
                random,
                {
                    .actor_token = 0x05255508U,
                    .entry_eax = 0x11112222U,
                    .entry_edx = 0x33334444U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBRewardItemSelectionStatus::
                        resource_read_typed_stop &&
                result.random_calls == 1U &&
                result.random_value == 0x12340007U &&
                result.item_id == 0x4444U && result.threshold == 0U &&
                result.return_eax == 0x12340007U && result.return_ecx == 0U &&
                result.return_edx == 0x12340007U &&
                random.bounds == std::vector<u32>{20U},
            "reward item selection re-reads the actor resource token after random and stops before the second resource access"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.resource_token = 0x73005678U;
        write_word(actor.resource_bytes, 0x82U, 0x1234U);
        write_word(actor.resource_bytes, 0x84U, 0x8000U);
        RandomPort random;
        random.push(0xDEAD8000U);
        const auto equal =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                &actor, random
            );

        write_word(actor.resource_bytes, 0x84U, 0x8001U);
        random.push(0xBEEF8000U);
        const auto below =
            openswd3::battle::select_legacy_battle_group_b_reward_item(
                &actor, random
            );
        test.expect_true(
            equal.return_eax == 0xDEAD0000U &&
                equal.return_edx == 0xDEAD8000U &&
                below.return_eax == 0xBEEF1234U &&
                below.return_edx == 0xBEEF8000U &&
                random.bounds == std::vector<u32>{20U, 20U},
            "reward threshold uses unsigned 16-bit comparison, rejects equality and clears only AX"
        );
    }
}
