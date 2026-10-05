#include "openswd3/resource_io/legacy_save_slots.hpp"

#include "test.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

int main() {
    openswd3::test::Context test;
    using openswd3::resource_io::LegacySaveSlotMove;
    using openswd3::resource_io::move_legacy_save_slot;
    test.expect_equal(
        move_legacy_save_slot(0U, LegacySaveSlotMove::previous),
        0U,
        "previous clamps at first slot"
    );
    test.expect_equal(
        move_legacy_save_slot(0U, LegacySaveSlotMove::next),
        1U,
        "next advances one slot"
    );
    test.expect_equal(
        move_legacy_save_slot(2U, LegacySaveSlotMove::next_page),
        5U,
        "page down advances three slots"
    );
    test.expect_equal(
        move_legacy_save_slot(2U, LegacySaveSlotMove::previous_page),
        0U,
        "page up clamps at zero"
    );
    test.expect_equal(
        move_legacy_save_slot(98U, LegacySaveSlotMove::next),
        98U,
        "next clamps at final slot"
    );
    test.expect_equal(
        move_legacy_save_slot(97U, LegacySaveSlotMove::next_page),
        98U,
        "page down clamps at final slot"
    );
    const std::filesystem::path test_root =
        std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
        ("save-scan-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()
         ));
    const std::filesystem::path save_directory = test_root / "Save";
    const bool created = std::filesystem::create_directories(save_directory);
    test.expect_true(created, "save scan fixture is newly owned by this test");
    if (!created) {
        return test.exit_code();
    }

    test.expect_false(
        openswd3::resource_io::scan_legacy_save_slots(test_root),
        "missing save slots return false"
    );

    {
        std::ofstream file(save_directory / "99.sav", std::ios::binary);
        file.put('x');
    }
    test.expect_false(
        openswd3::resource_io::scan_legacy_save_slots(test_root),
        "slot 99 is outside the original scan"
    );

    {
        std::ofstream file(save_directory / "98.sav", std::ios::binary);
        file.put('x');
    }
    test.expect_true(
        openswd3::resource_io::scan_legacy_save_slots(test_root),
        "slot 98 is included and a nonempty file is not required"
    );

    std::filesystem::remove(save_directory / "98.sav");
    std::filesystem::remove(save_directory / "99.sav");
    std::filesystem::remove(save_directory);
    std::filesystem::remove(test_root);

#ifdef OPENSWD3_GAME_DATA_ROOT
    test.expect_true(
        openswd3::resource_io::scan_legacy_save_slots(
            std::filesystem::path{OPENSWD3_GAME_DATA_ROOT}
        ),
        "original Save/0.sav and Save/1.sav are discoverable"
    );
#endif
    return test.exit_code();
}
