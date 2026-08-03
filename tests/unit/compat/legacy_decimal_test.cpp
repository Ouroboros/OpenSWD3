#include "test.hpp"

#include "openswd3/compat/legacy_decimal.hpp"

#include <string>

namespace {

using openswd3::compat::LegacyDecimalParseResult;
using openswd3::compat::i32;

void expect_success(
    openswd3::test::Context& test,
    const std::string_view input,
    const i32 expected
) {
    i32 output = 0x12345678;
    test.expect_equal(
        openswd3::compat::parse_legacy_decimal_contract(input, output),
        LegacyDecimalParseResult::success,
        "valid legacy decimal returns success"
    );
    test.expect_equal(output, expected, "valid legacy decimal value");

    output = 0;
    test.expect_true(
        openswd3::compat::parse_legacy_decimal_or_terminate(input, output),
        "valid legacy wrapper returns true"
    );
    test.expect_equal(output, expected, "valid legacy wrapper value");
}

void test_valid(openswd3::test::Context& test) {
    expect_success(test, "0", 0);
    expect_success(test, "+17", 17);
    expect_success(test, "-17", -17);
    expect_success(test, "000000000", 0);
    expect_success(test, "999999999", 999999999);
    expect_success(test, "-999999999", -999999999);

    const std::string terminated{"12\0x", 4U};
    expect_success(test, terminated, 12);
}

void expect_invalid_untouched(
    openswd3::test::Context& test,
    const std::string_view input
) {
    i32 output = 0x12345678;
    test.expect_equal(
        openswd3::compat::parse_legacy_decimal_contract(input, output),
        LegacyDecimalParseResult::invalid,
        "invalid legacy decimal returns invalid"
    );
    test.expect_equal(
        output,
        0x12345678,
        "validation failure leaves output untouched"
    );

    test.expect_false(
        openswd3::compat::parse_legacy_decimal_or_terminate(input, output),
        "invalid legacy wrapper returns false"
    );
    test.expect_equal(
        output,
        0x12345678,
        "invalid wrapper also leaves output untouched"
    );
}

void test_invalid(openswd3::test::Context& test) {
    expect_invalid_untouched(test, "1a");
    expect_invalid_untouched(test, " 1");
    expect_invalid_untouched(test, "1 ");
    expect_invalid_untouched(test, "1234567890");
    expect_invalid_untouched(test, "+1234567890");

    std::string high_byte;
    high_byte.push_back(static_cast<char>(0xFF));
    expect_invalid_untouched(test, high_byte);
}

void test_no_digit_fault_contract(openswd3::test::Context& test) {
    for (const std::string_view input : {"", "+", "-"}) {
        i32 output = 0x12345678;
        test.expect_equal(
            openswd3::compat::parse_legacy_decimal_contract(input, output),
            LegacyDecimalParseResult::legacy_fault_no_digits,
            "empty digit sequence enters the original non-returning fault path"
        );
        test.expect_equal(
            output,
            0,
            "fault path writes output zero before reverse underflow"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_valid(test);
    test_invalid(test);
    test_no_digit_fault_contract(test);
    return test.exit_code();
}
