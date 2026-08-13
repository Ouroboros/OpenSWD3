#include "test.hpp"

#include "legacy_command_line.hpp"
#include "single_instance.hpp"

#include <chrono>
#include <string>
#include <string_view>

namespace {

void test_windows_tail_parser(openswd3::test::Context& test) {
    using openswd3::platform_sdl3::extract_windows_command_line_tail;

    test.expect_equal(
        extract_windows_command_line_tail("program.exe"),
        std::string_view{},
        "program name alone has an empty tail"
    );
    test.expect_equal(
        extract_windows_command_line_tail("program.exe 7payload with spaces"),
        std::string_view{"7payload with spaces"},
        "unquoted executable is removed"
    );
    test.expect_equal(
        extract_windows_command_line_tail(
            "\"C:\\Program Files\\OpenSWD3.exe\"\t7payload"
        ),
        std::string_view{"7payload"},
        "quoted executable and following whitespace are removed"
    );
    test.expect_equal(
        extract_windows_command_line_tail("\"unterminated"),
        std::string_view{},
        "unterminated executable quote yields an empty tail"
    );
    test.expect_equal(
        extract_windows_command_line_tail(
            "openswd3.exe --data-dir \"E:\\Game\\swd3 data\" 7payload raw", 3U
        ),
        std::string_view{"7payload raw"},
        "modern prefix is removed without changing the legacy tail"
    );
}

void test_argument_reconstruction(openswd3::test::Context& test) {
    char program[] = "openswd3";
    char option[] = "--data-dir";
    char directory[] = "game-data";
    char first_legacy[] = "7payload";
    char second_legacy[] = "with spaces";
    char* arguments[]{
        program,
        option,
        directory,
        first_legacy,
        second_legacy,
    };
#if !defined(_WIN32)
    test.expect_equal(
        openswd3::platform_sdl3::reconstruct_legacy_command_line_tail(
            5, arguments, 3
        ),
        std::string{"7payload with spaces"},
        "non-Windows argv joins only the remaining legacy arguments"
    );
#else
    static_cast<void>(arguments);
    static_cast<void>(test);
#endif
}

void test_single_instance_guard(openswd3::test::Context& test) {
    const auto unique_value =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string identity =
        "OpenSWD3-single-instance-test-" + std::to_string(unique_value);

    {
        openswd3::platform_sdl3::SingleInstanceGuard first(identity);
        test.expect_false(
            first.matching_instance_exists(),
            "first guard acquires the identity"
        );
        test.expect_false(
            first.matching_instance_exists(),
            "repeated check preserves the acquired result"
        );

        openswd3::platform_sdl3::SingleInstanceGuard second(identity);
        test.expect_true(
            second.matching_instance_exists(),
            "second guard observes the held identity"
        );
    }

    openswd3::platform_sdl3::SingleInstanceGuard after_release(identity);
    test.expect_false(
        after_release.matching_instance_exists(),
        "identity can be acquired after the first guard is destroyed"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_windows_tail_parser(test);
    test_argument_reconstruction(test);
    test_single_instance_guard(test);
    return test.exit_code();
}
