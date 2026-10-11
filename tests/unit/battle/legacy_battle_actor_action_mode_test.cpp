#include "openswd3/battle/legacy_battle_actor_action_mode.hpp"

#include "test.hpp"

#include <array>

namespace {

using openswd3::battle::LegacyBattleActorActionModeMemoryAccess;
using openswd3::battle::LegacyBattleActorActionModeStatus;
using openswd3::battle::set_legacy_battle_actor_action_mode;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct BranchCase {
    u32 mode;
    u16 action;
    u16 display;
    u8 mask;
};

constexpr std::array branches{
    BranchCase{0U, 0U, 0xBEEFU, 0U},
    BranchCase{1U, 1U, 0xBEEFU, 0U},
    BranchCase{2U, 0U, 2U, 0U},
    BranchCase{3U, 0U, 3U, 0U},
    BranchCase{4U, 4U, 0xBEEFU, 0U},
    BranchCase{5U, 5U, 0xBEEFU, 0U},
    BranchCase{6U, 0U, 6U, 0x08U},
    BranchCase{7U, 7U, 0xBEEFU, 0U},
    BranchCase{8U, 8U, 0xBEEFU, 0U},
    BranchCase{9U, 9U, 0xBEEFU, 0U},
    BranchCase{10U, 10U, 0xBEEFU, 0U},
    BranchCase{11U, 11U, 0xBEEFU, 0U},
    BranchCase{12U, 12U, 0xBEEFU, 0U},
    BranchCase{13U, 0U, 13U, 0U},
    BranchCase{14U, 0U, 14U, 0x40U},
    BranchCase{15U, 0U, 15U, 0x40U},
    BranchCase{16U, 16U, 0xBEEFU, 0U},
    BranchCase{300U, 300U, 0xBEEFU, 0U},
    BranchCase{0xFFFFU, 0xFFFFU, 0xBEEFU, 0U},
    BranchCase{0xFFFFFFFFU, 0xFFFFU, 0xBEEFU, 0U},
    BranchCase{0x80000000U, 0U, 0xBEEFU, 0U},
    BranchCase{0x12345678U, 0x5678U, 0xBEEFU, 0U},
    BranchCase{0x10002U, 2U, 0xBEEFU, 0U},
    BranchCase{0x10003U, 3U, 0xBEEFU, 0U},
    BranchCase{0x10006U, 6U, 0xBEEFU, 0U},
    BranchCase{0x1000DU, 13U, 0xBEEFU, 0U},
    BranchCase{0x1000EU, 14U, 0xBEEFU, 0U},
    BranchCase{0x1000FU, 15U, 0xBEEFU, 0U},
};

}  // namespace

void test_battle_actor_action_mode(openswd3::test::Context& test) {
    using Status = LegacyBattleActorActionModeStatus;
    for (const auto& expected : branches) {
        for (u32 initial_flags = 0U; initial_flags < 256U; ++initial_flags) {
            u16 action = 0xCAFEU;
            u16 display = 0xBEEFU;
            u8 flags = static_cast<u8>(initial_flags);
            const auto result = set_legacy_battle_actor_action_mode(
                {&action, &display, &flags}, expected.mode
            );
            test.expect_true(
                result.status == Status::completed &&
                    action == expected.action && display == expected.display &&
                    flags == static_cast<u8>(initial_flags | expected.mask),
                "full DWORD comparisons select the LST branch before WORD truncation and preserve every existing flag bit"
            );
        }
    }

    struct FailureCase {
        u32 mode;
        LegacyBattleActorActionModeMemoryAccess access;
        Status status;
        u16 action;
        u16 display;
        u8 flags;
    };

    const std::array failures{
        FailureCase{
            14U,
            {.argument_readable = false},
            Status::argument_read_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            0xABCD1234U,
            {.action_kind_writable = false},
            Status::action_kind_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            14U,
            {.mode_flags_readable = false},
            Status::mode_flags_read_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            14U,
            {.mode_flags_writable = false},
            Status::mode_flags_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            6U,
            {.mode_flags_readable = false},
            Status::mode_flags_read_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            6U,
            {.mode_flags_writable = false},
            Status::mode_flags_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            6U,
            {.display_kind_writable = false},
            Status::display_kind_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x19U
        },
        FailureCase{
            14U,
            {.display_kind_writable = false},
            Status::display_kind_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x51U
        },
        FailureCase{
            2U,
            {.display_kind_writable = false},
            Status::display_kind_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            15U,
            {.action_kind_writable = false},
            Status::action_kind_write_typed_stop,
            0xCAFEU,
            15U,
            0x51U
        },
        FailureCase{
            6U,
            {.action_kind_writable = false},
            Status::action_kind_write_typed_stop,
            0xCAFEU,
            6U,
            0x19U
        },
        FailureCase{
            13U,
            {.action_kind_writable = false},
            Status::action_kind_write_typed_stop,
            0xCAFEU,
            13U,
            0x11U
        },
        FailureCase{
            13U,
            {.return_address_readable = false},
            Status::return_address_read_typed_stop,
            0U,
            13U,
            0x11U
        },
        FailureCase{
            15U,
            {.return_address_readable = false},
            Status::return_address_read_typed_stop,
            0U,
            15U,
            0x51U
        },
        FailureCase{
            0xABCD1234U,
            {.return_address_readable = false},
            Status::return_address_read_typed_stop,
            0x1234U,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            0U,
            {.mode_flags_readable = false,
             .mode_flags_writable = false,
             .display_kind_writable = false},
            Status::completed,
            0U,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            3U,
            {.mode_flags_readable = false, .mode_flags_writable = false},
            Status::completed,
            0U,
            3U,
            0x11U
        },
        FailureCase{
            14U,
            {.argument_readable = false,
             .mode_flags_readable = false,
             .return_address_readable = false},
            Status::argument_read_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
        FailureCase{
            6U,
            {.mode_flags_writable = false,
             .display_kind_writable = false,
             .action_kind_writable = false},
            Status::mode_flags_write_typed_stop,
            0xCAFEU,
            0xBEEFU,
            0x11U
        },
    };

    for (const auto& expected : failures) {
        u16 action = 0xCAFEU;
        u16 display = 0xBEEFU;
        u8 flags = 0x11U;
        const auto result = set_legacy_battle_actor_action_mode(
            {&action, &display, &flags}, expected.mode, expected.access
        );
        test.expect_true(
            result.status == expected.status && action == expected.action &&
                display == expected.display && flags == expected.flags,
            "the first failed access preserves exactly the preceding writes and unused fields do not cause stops"
        );
    }

    {
        u16 action = 0xCAFEU;
        u16 display = 0xBEEFU;
        u8 flags = 0x11U;
        const auto argument = set_legacy_battle_actor_action_mode(
            {}, 6U, {.argument_readable = false}
        );
        const auto missing_flags = set_legacy_battle_actor_action_mode(
            {&action, &display, nullptr}, 6U
        );
        const auto missing_display =
            set_legacy_battle_actor_action_mode({&action, nullptr, &flags}, 6U);
        test.expect_true(
            argument.status == Status::argument_read_typed_stop &&
                missing_flags.status == Status::mode_flags_read_typed_stop &&
                missing_display.status ==
                    Status::display_kind_write_typed_stop &&
                action == 0xCAFEU && display == 0xBEEFU && flags == 0x19U,
            "missing borrowed fields fail at the original access without rolling back the flag write"
        );
        const auto missing_action = set_legacy_battle_actor_action_mode(
            {nullptr, &display, &flags}, 15U
        );
        test.expect_true(
            missing_action.status == Status::action_kind_write_typed_stop &&
                display == 15U && flags == 0x59U,
            "missing action storage preserves the preceding display and mode writes"
        );
        const auto unused_fields = set_legacy_battle_actor_action_mode(
            {&action, nullptr, nullptr}, 0x10002U
        );
        test.expect_true(
            unused_fields.status == Status::completed && action == 2U,
            "the default path does not dereference display or flag storage"
        );
    }

    {
        u16 first_action = 0xCAFEU;
        u16 first_display = 0xBEEFU;
        u8 first_flags = 0x80U;
        u16 second_action = 0x1234U;
        u16 second_display = 0x5678U;
        u8 second_flags = 0x01U;
        const auto first = set_legacy_battle_actor_action_mode(
            {&first_action, &first_display, &first_flags}, 6U
        );
        first_flags = 0x01U;
        const auto reread = set_legacy_battle_actor_action_mode(
            {&first_action, &first_display, &first_flags}, 14U
        );
        const auto second = set_legacy_battle_actor_action_mode(
            {&second_action, &second_display, &second_flags}, 300U
        );
        test.expect_true(
            first.status == Status::completed &&
                reread.status == Status::completed &&
                second.status == Status::completed && first_action == 0U &&
                first_display == 14U && first_flags == 0x41U &&
                second_action == 300U && second_display == 0x5678U &&
                second_flags == 0x01U,
            "repeated settings read current shared flag storage and do not mutate another actor"
        );
    }
}
