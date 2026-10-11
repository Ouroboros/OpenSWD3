#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp"
#include "test.hpp"

#include <array>

namespace {

using openswd3::battle::clear_legacy_battle_actor_field_26b8_high_bit;
using Access =
    openswd3::battle::LegacyBattleActorField26b8HighBitClearMemoryAccess;
using Status = openswd3::battle::LegacyBattleActorField26b8HighBitClearStatus;
using openswd3::compat::u32;

}  // namespace

void test_battle_actor_field_26b8_high_bit_clear(
    openswd3::test::Context& test
) {
    struct Case {
        u32 initial;
        u32 expected;
    };

    constexpr std::array cases{
        Case{0U, 0U},
        Case{1U, 1U},
        Case{0x7FFFFFFFU, 0x7FFFFFFFU},
        Case{0x80000000U, 0U},
        Case{0x80000001U, 1U},
        Case{0xFFFFFFFFU, 0x7FFFFFFFU},
        Case{0xA5A55A5AU, 0x25A55A5AU},
        Case{0x12345678U, 0x12345678U},
    };

    for (const auto& expected : cases) {
        u32 field = expected.initial;
        const auto result =
            clear_legacy_battle_actor_field_26b8_high_bit(&field);
        test.expect_true(
            result.status == Status::completed && field == expected.expected,
            "clearing the high bit preserves all lower DWORD bits"
        );
        const auto repeated =
            clear_legacy_battle_actor_field_26b8_high_bit(&field);
        test.expect_true(
            repeated.status == Status::completed && field == expected.expected,
            "repeated clearing reads the current shared field"
        );
    }

    struct FailureCase {
        Access access;
        Status status;
        bool committed;
    };

    const std::array failures{
        FailureCase{
            {false, false, false}, Status::field_read_typed_stop, false
        },
        FailureCase{{false, false, true}, Status::field_read_typed_stop, false},
        FailureCase{{false, true, false}, Status::field_read_typed_stop, false},
        FailureCase{{false, true, true}, Status::field_read_typed_stop, false},
        FailureCase{
            {true, false, false}, Status::field_write_typed_stop, false
        },
        FailureCase{{true, false, true}, Status::field_write_typed_stop, false},
        FailureCase{
            {true, true, false}, Status::return_address_read_typed_stop, true
        },
        FailureCase{{true, true, true}, Status::completed, true},
    };

    for (const auto& expected : cases) {
        for (const auto& failure : failures) {
            u32 field = expected.initial;
            const auto result = clear_legacy_battle_actor_field_26b8_high_bit(
                &field, failure.access
            );
            test.expect_true(
                result.status == failure.status &&
                    field ==
                        (failure.committed ? expected.expected
                                           : expected.initial),
                "the first failed access wins and an already-clear field still requires a write"
            );
        }
    }

    const auto missing = clear_legacy_battle_actor_field_26b8_high_bit(nullptr);
    test.expect_true(
        missing.status == Status::field_read_typed_stop,
        "missing shared storage fails at the field read"
    );

    u32 first = 0x80000001U;
    u32 second = 0xFEDCBA98U;
    const auto cleared = clear_legacy_battle_actor_field_26b8_high_bit(&first);
    first = 0x81234567U;
    const auto reread = clear_legacy_battle_actor_field_26b8_high_bit(&first);
    test.expect_true(
        cleared.status == Status::completed &&
            reread.status == Status::completed && first == 0x01234567U &&
            second == 0xFEDCBA98U,
        "a later call reads live storage without modifying another actor"
    );
}
