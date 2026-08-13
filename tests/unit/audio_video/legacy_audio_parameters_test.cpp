#include "test.hpp"

#include "openswd3/audio_video/legacy_audio_parameters.hpp"

#include <limits>

namespace {

using openswd3::audio_video::legacy_audio_pan_parameter;
using openswd3::audio_video::legacy_audio_volume_parameter;
using openswd3::compat::i32;

void test_volume_parameter(openswd3::test::Context& test) {
    test.expect_equal(
        legacy_audio_volume_parameter(std::numeric_limits<i32>::min()),
        0,
        "minimum signed volume clamps to zero"
    );
    test.expect_equal(
        legacy_audio_volume_parameter(-1), 0, "negative volume clamps to zero"
    );
    test.expect_equal(
        legacy_audio_volume_parameter(0), 0, "zero volume is preserved"
    );
    test.expect_equal(
        legacy_audio_volume_parameter(126), 126, "in-range volume is preserved"
    );
    test.expect_equal(
        legacy_audio_volume_parameter(127), 127, "maximum volume is preserved"
    );
    test.expect_equal(
        legacy_audio_volume_parameter(128), 127, "volume above maximum clamps"
    );
    test.expect_equal(
        legacy_audio_volume_parameter(std::numeric_limits<i32>::max()),
        127,
        "maximum signed volume clamps"
    );
}

void test_pan_parameter(openswd3::test::Context& test) {
    test.expect_equal(
        legacy_audio_pan_parameter(-64),
        0,
        "pan below centered range clamps to zero"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(-63), 0, "pan negative endpoint maps to zero"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(-62), 1, "pan adds the original bias"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(0), 63, "center pan maps to 63"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(64), 127, "pan positive endpoint maps to 127"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(65), 127, "pan above endpoint clamps"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(std::numeric_limits<i32>::max()),
        0,
        "positive overflow wraps before signed clamp"
    );
    test.expect_equal(
        legacy_audio_pan_parameter(std::numeric_limits<i32>::min()),
        0,
        "minimum signed pan remains negative after bias"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_volume_parameter(test);
    test_pan_parameter(test);
    return test.exit_code();
}
