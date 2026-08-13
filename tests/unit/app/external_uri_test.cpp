#include "test.hpp"

#include "external_uri.hpp"

#include <string>

int main() {
    openswd3::test::Context test;

    test.expect_equal(
        openswd3::platform_sdl3::make_legacy_http_uri("www.softstar.com.tw"),
        std::string{"http://www.softstar.com.tw"},
        "legacy http registry class becomes an explicit http URI"
    );
    test.expect_equal(
        openswd3::platform_sdl3::make_legacy_http_uri(
            "http://www.softstar.com.tw"
        ),
        std::string{"http://www.softstar.com.tw"},
        "existing legacy scheme is not duplicated"
    );

    const auto file_uri =
        openswd3::platform_sdl3::make_absolute_file_uri("Read me.txt");
    test.expect_true(
        file_uri.has_value(), "relative document path becomes absolute"
    );
    if (file_uri.has_value()) {
        test.expect_true(
            file_uri->starts_with("file:///"),
            "local document uses an absolute file URI"
        );
        test.expect_true(
            file_uri->ends_with("/Read%20me.txt"),
            "document URI percent-encodes spaces"
        );
    }

    return test.exit_code();
}
