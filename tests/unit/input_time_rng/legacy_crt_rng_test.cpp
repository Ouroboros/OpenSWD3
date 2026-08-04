#include "test.hpp"

#include "openswd3/input_time_rng/legacy_crt_rng.hpp"

#include <array>

namespace {

using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyCrtRng;

void test_known_crt_streams(openswd3::test::Context& test) {
    LegacyCrtRng rng;
    rng.seed(1U);
    constexpr std::array<u32, 10> kSeedOne{
        41U, 18467U, 6334U, 26500U, 19169U,
        15724U, 11478U, 29358U, 26962U, 24464U,
    };
    for (const u32 expected : kSeedOne) {
        test.expect_equal(
            rng.next(),
            expected,
            "MSVC-compatible rand stream for seed one"
        );
    }
    test.expect_equal(rng.state(), 0xDF90722BU, "seed one tenth state");

    rng.seed(0x12345678U);
    constexpr std::array<u32, 10> kFixedSeed{
        13289U, 23359U, 19469U, 24737U, 23446U,
        14229U, 6193U, 18180U, 32073U, 13357U,
    };
    for (const u32 expected : kFixedSeed) {
        test.expect_equal(
            rng.next(),
            expected,
            "MSVC-compatible rand stream for fixed seed"
        );
    }
    test.expect_equal(rng.state(), 0x342D28BAU, "fixed seed tenth state");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_known_crt_streams(test);
    return test.exit_code();
}
