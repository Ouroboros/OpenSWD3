#include "openswd3/battle/legacy_battle_group_b_opponent_mode.hpp"
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
        return value;
    }

    void push(const u32 value) {
        values.push_back(value);
    }

    std::deque<u32> values;
    std::vector<u32> bounds;
};

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_dword(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

void prepare_actor(
    openswd3::battle::LegacyBattleActorGroupBElementState& actor
) {
    actor.resource_token = 0x73000000U;
    write_dword(actor.resource_bytes, 0x4CU, 3U);
    write_word(actor.resource_bytes, 0x64U, 0U);
    write_word(actor.resource_bytes, 0x7CU, 1U);
    write_word(actor.resource_bytes, 0x80U, 1U);
    actor.resource_bytes[0x8EU] = 1U;
}

}  // namespace

void test_battle_group_b_opponent_mode(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupBOpponentModeStatus;

    {
        RandomPort random;
        random.push(0x12340004U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                nullptr, random
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOpponentModeStatus::
                        actor_state_typed_stop &&
                result.random_calls == 1U && result.random_value == 0x12340004U &&
                result.normalized_random == 0U &&
                result.return_eax == 0x12340004U &&
                result.return_edx == 0x12340004U &&
                !result.return_ecx_known && random.bounds == std::vector<u32>{10U},
            "opponent mode consumes random before the first actor-state stop"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_configuration.timing_value = 0xCAFEBABEU;
        RandomPort random;
        random.push(0xABCD0009U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBOpponentModeStatus::
                        resource_read_typed_stop &&
                result.normalized_random == 1U &&
                result.return_eax == 0xCAFEBABEU && result.return_ecx == 0U &&
                result.return_edx == 0xABCD0009U && result.return_ecx_known,
            "opponent mode reads the timing override before the resource stop"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor);
        actor.action_execution.opponent_mode = 7U;
        RandomPort random;
        random.push(0x12340004U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );
        test.expect_true(
            result.status == LegacyBattleGroupBOpponentModeStatus::completed &&
                result.return_eax == 1U &&
                result.return_ecx == actor.resource_token &&
                result.return_edx == 1U &&
                actor.action_execution.opponent_mode == 1U,
            "random low word zero through four selects opponent mode one"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor);
        RandomPort random;
        random.push(0xABCD0005U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );
        test.expect_true(
            result.return_eax == 1U && result.normalized_random == 1U &&
                actor.action_execution.opponent_mode == 2U,
            "random low word five through nine selects opponent mode two"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor);
        actor.action_execution.opponent_mode = 7U;
        RandomPort random;
        random.push(0x0001000AU);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );
        test.expect_true(
            result.return_eax == 0U &&
                result.normalized_random == 0x0001000AU &&
                actor.action_execution.opponent_mode == 7U,
            "out-of-range random low word preserves the full value and mode"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor);
        actor.action_execution.opponent_mode = 7U;
        write_dword(actor.resource_bytes, 0x4CU, 0xFFFFFFFDU);
        write_word(actor.resource_bytes, 0x64U, 0xFFFFU);
        RandomPort random;
        random.push(0U);
        const auto equal_signed =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );

        write_dword(actor.resource_bytes, 0x4CU, 6U);
        actor.action_configuration.timing_value = 2U;
        random.push(0U);
        const auto override_equal =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );
        test.expect_true(
            equal_signed.return_eax == 0U &&
                equal_signed.return_edx == 0xFFFFFFFFU &&
                override_equal.return_eax == 0U &&
                override_equal.return_edx == 2U &&
                actor.action_execution.opponent_mode == 7U,
            "signed third threshold and full timing override reject equality"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        prepare_actor(actor);
        actor.action_execution.opponent_mode = 7U;
        RandomPort random;
        random.push(0U);
        random.push(0U);
        random.push(0U);
        random.push(5U);

        write_word(actor.resource_bytes, 0x7CU, 0U);
        const auto missing_first =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );

        write_word(actor.resource_bytes, 0x7CU, 1U);
        actor.action_execution.retreat_ready_flags = 0x0100U;
        const auto blocked =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );

        actor.action_execution.retreat_ready_flags = 0U;
        actor.resource_bytes[0x8EU] = 0U;
        const auto hidden =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );

        actor.resource_bytes[0x8EU] = 1U;
        write_word(actor.resource_bytes, 0x80U, 0U);
        const auto missing_second =
            openswd3::battle::select_legacy_battle_group_b_opponent_mode(
                &actor, random
            );
        test.expect_true(
            missing_first.return_eax == 0U && blocked.return_eax == 0U &&
                hidden.return_eax == 0U && missing_second.return_eax == 0U &&
                actor.action_execution.opponent_mode == 7U &&
                random.bounds == std::vector<u32>{10U, 10U, 10U, 10U},
            "opponent mode preserves both resource gates, actor bit and visibility"
        );
    }
}
