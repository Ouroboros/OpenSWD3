#include "test.hpp"

#include "openswd3/compat/legacy_stubs.hpp"

int main() {
    openswd3::test::Context test;
    test.expect_equal(
        openswd3::compat::legacy_zero_result(),
        0,
        "0x00411F90 returns zero"
    );
    test.expect_equal(
        openswd3::compat::legacy_true_result(),
        1,
        "0x00425B40 returns one"
    );
    openswd3::compat::legacy_noop();
    return test.exit_code();
}
