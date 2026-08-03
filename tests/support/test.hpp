#pragma once

#include <iostream>
#include <source_location>
#include <string_view>

namespace openswd3::test {

class Context {
public:
    template <typename Actual, typename Expected>
    void expect_equal(
        const Actual& actual,
        const Expected& expected,
        const std::string_view expression,
        const std::source_location location = std::source_location::current()
    ) {
        if (actual == expected) {
            return;
        }
        ++failures_;
        std::cerr << location.file_name() << ':' << location.line()
                  << ": expectation failed: " << expression << '\n';
    }

    void expect_true(
        const bool value,
        const std::string_view expression,
        const std::source_location location = std::source_location::current()
    ) {
        expect_equal(value, true, expression, location);
    }

    void expect_false(
        const bool value,
        const std::string_view expression,
        const std::source_location location = std::source_location::current()
    ) {
        expect_equal(value, false, expression, location);
    }

    [[nodiscard]] int exit_code() const noexcept {
        return failures_ == 0 ? 0 : 1;
    }

private:
    int failures_{};
};

}  // namespace openswd3::test
