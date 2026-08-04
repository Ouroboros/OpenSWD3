#include "test.hpp"

#include "openswd3/input_time_rng/legacy_input.hpp"

#include <array>
#include <limits>

namespace {

using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::input_time_rng::LegacyKeyBinding;
using openswd3::input_time_rng::LegacyKeyBindingBlock;
using openswd3::input_time_rng::LegacyKeyboardSnapshot;
using openswd3::input_time_rng::LegacyMouseDeviceSample;
using openswd3::input_time_rng::LegacyMouseFrame;
using openswd3::input_time_rng::LegacyMouseState;

void test_default_bindings(openswd3::test::Context& test) {
    constexpr u32 kUntouched = 0xA5A5A5A5U;
    LegacyKeyBindingBlock block;
    block.words.fill(kUntouched);

    openswd3::input_time_rng::initialize_default_key_bindings(block);
    constexpr std::array<LegacyKeyBinding, 16> kBindings{
        LegacyKeyBinding::cancel,
        LegacyKeyBinding::primary_action,
        LegacyKeyBinding::configurable_2,
        LegacyKeyBinding::left,
        LegacyKeyBinding::up,
        LegacyKeyBinding::right,
        LegacyKeyBinding::down,
        LegacyKeyBinding::page_up,
        LegacyKeyBinding::page_down,
        LegacyKeyBinding::configurable_9,
        LegacyKeyBinding::configurable_10,
        LegacyKeyBinding::alternate_action,
        LegacyKeyBinding::configurable_16,
        LegacyKeyBinding::configurable_17,
        LegacyKeyBinding::configurable_18,
        LegacyKeyBinding::configurable_19,
    };
    constexpr std::array<u32, 16> kExpected{
        0x01U, 0x39U, 0x36U, 0xCBU,
        0xC8U, 0xCDU, 0xD0U, 0xC9U,
        0xD1U, 0x9DU, 0xCFU, 0x1CU,
        0x13U, 0x1EU, 0x22U, 0x3BU,
    };
    constexpr std::array<std::size_t, 16> kPhysicalWordIndices{
        0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
        8U, 9U, 10U, 12U, 16U, 17U, 18U, 19U,
    };
    for (std::size_t index = 0U; index < kBindings.size(); ++index) {
        test.expect_equal(
            block.words[kPhysicalWordIndices[index]],
            kExpected[index],
            "0x00424390 physical dword offset"
        );
        test.expect_equal(
            openswd3::input_time_rng::key_binding(block, kBindings[index]),
            kExpected[index],
            "0x00424390 default dword"
        );
    }

    constexpr std::array<std::size_t, 16> kUntouchedWordIndices{
        11U, 13U, 14U, 15U,
        20U, 21U, 22U, 23U,
        24U, 25U, 26U, 27U,
        28U, 29U, 30U, 31U,
    };
    for (const std::size_t index : kUntouchedWordIndices) {
        test.expect_equal(
            block.words[index],
            kUntouched,
            "default initializer preserves compatibility-block gaps"
        );
    }
}

void test_release_transitions(openswd3::test::Context& test) {
    LegacyInputRecord record{2U, 0U, 3U, 9U};
    test.expect_equal(
        openswd3::input_time_rng::update_input_record(record, 0U, 100U),
        0U,
        "first released sample returns zero"
    );
    test.expect_equal(
        record,
        LegacyInputRecord{2U, 100U, 3U, 0U},
        "first released sample preserves multiplicity and stage"
    );

    record.held_sample_count = 7U;
    static_cast<void>(
        openswd3::input_time_rng::update_input_record(record, 0U, 250U)
    );
    test.expect_equal(
        record,
        LegacyInputRecord{0U, 100U, 3U, 0U},
        "elapsed equal to 150 preserves the rapid-press stage"
    );

    record.rapid_press_multiplicity = 2U;
    static_cast<void>(
        openswd3::input_time_rng::update_input_record(record, 0U, 251U)
    );
    test.expect_equal(
        record,
        LegacyInputRecord{0U, 100U, 0U, 0U},
        "elapsed greater than 150 clears the rapid-press stage"
    );

    LegacyInputRecord wrapped{2U, 0xFFFFFFF0U, 3U, 5U};
    static_cast<void>(
        openswd3::input_time_rng::update_input_record(wrapped, 0U, 0x86U)
    );
    test.expect_equal(
        wrapped.rapid_press_stage,
        3U,
        "wrapped elapsed equal to 150 preserves the stage"
    );
    wrapped.rapid_press_multiplicity = 2U;
    static_cast<void>(
        openswd3::input_time_rng::update_input_record(wrapped, 0U, 0x87U)
    );
    test.expect_equal(
        wrapped.rapid_press_stage,
        0U,
        "wrapped elapsed greater than 150 clears the stage"
    );
}

void test_press_transitions(openswd3::test::Context& test) {
    constexpr std::array<u32, 4> kExpectedMultiplicity{1U, 2U, 3U, 3U};
    for (std::size_t index = 0U; index < kExpectedMultiplicity.size(); ++index) {
        const u32 stage = static_cast<u32>(index);
        LegacyInputRecord record{9U, 100U, stage, 7U};
        test.expect_equal(
            openswd3::input_time_rng::update_input_record(
                record,
                0x80U,
                1000U
            ),
            1U,
            "re-press returns a fresh held sample count"
        );
        test.expect_equal(
            record,
            LegacyInputRecord{
                kExpectedMultiplicity[index],
                0U,
                kExpectedMultiplicity[index],
                1U,
            },
            "re-press maps the chain stage through {1,2,3,3}"
        );
    }

    LegacyInputRecord fresh{};
    test.expect_equal(
        openswd3::input_time_rng::update_input_record(fresh, 1U, 500U),
        1U,
        "fresh held input increments the held sample count"
    );
    test.expect_equal(
        fresh,
        LegacyInputRecord{0U, 0U, 0U, 1U},
        "fresh held input preserves zero multiplicity"
    );

    LegacyInputRecord wrapping{2U, 0U, 3U, 0xFFFFFFFFU};
    test.expect_equal(
        openswd3::input_time_rng::update_input_record(wrapping, 2U, 500U),
        0U,
        "held sample count uses 32-bit wrapping increment"
    );
    test.expect_equal(
        wrapping,
        LegacyInputRecord{2U, 0U, 3U, 0U},
        "continuous hold preserves multiplicity and stage"
    );
}

void test_zero_clock_release_quirk(openswd3::test::Context& test) {
    LegacyInputRecord record{2U, 0U, 3U, 4U};
    static_cast<void>(
        openswd3::input_time_rng::update_input_record(record, 0U, 0U)
    );
    static_cast<void>(
        openswd3::input_time_rng::update_input_record(record, 0U, 0U)
    );
    test.expect_equal(
        record,
        LegacyInputRecord{2U, 0U, 3U, 0U},
        "a zero release clock repeats the first-release branch"
    );
}

void test_raw_keyboard_snapshot(openswd3::test::Context& test) {
    LegacyKeyboardSnapshot snapshot{};
    snapshot[0x1EU] = 0x7FU;
    test.expect_equal(
        openswd3::input_time_rng::read_raw_key(snapshot, 0x1EU),
        0U,
        "raw query ignores every bit except 0x80"
    );

    openswd3::input_time_rng::synthesize_raw_key(snapshot, 0x1EU);
    test.expect_equal(
        snapshot[0x1EU],
        static_cast<openswd3::compat::u8>(0xFFU),
        "synthetic write preserves existing low bits"
    );
    test.expect_equal(
        openswd3::input_time_rng::read_raw_key(snapshot, 0x1EU),
        0x80U,
        "raw query returns exactly 0x80"
    );
    test.expect_equal(
        openswd3::input_time_rng::find_first_pressed_key(snapshot),
        0x1EU,
        "first-key query returns the lowest pressed DIK code"
    );

    snapshot.fill(0U);
    test.expect_equal(
        openswd3::input_time_rng::find_first_pressed_key(snapshot),
        0U,
        "first-key query returns zero when no key is pressed"
    );
    snapshot[0U] = 0x80U;
    test.expect_equal(
        openswd3::input_time_rng::find_first_pressed_key(snapshot),
        0U,
        "pressed DIK zero is indistinguishable from no pressed key"
    );
}

void test_mouse_sensitivity_and_rebase(openswd3::test::Context& test) {
    LegacyMouseState state{};
    openswd3::input_time_rng::set_mouse_sensitivity(state, 2.0);
    test.expect_equal(
        state.sensitivity_scale,
        20,
        "startup sensitivity 2.0 stores integer scale 20"
    );

    const LegacyMouseDeviceSample sample{1000, 2000, 0U, 0U};
    openswd3::input_time_rng::rebase_mouse_coordinates(
        state,
        sample,
        480,
        360
    );
    test.expect_equal(
        state.absolute_x_baseline,
        760,
        "x rebase divides target by scale before multiplying by ten"
    );
    test.expect_equal(
        state.absolute_y_baseline,
        1820,
        "y rebase divides target by scale before multiplying by ten"
    );
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, sample),
        LegacyMouseFrame{480, 360, 0U},
        "unchanged absolute axes reproduce the requested common coordinates"
    );

    openswd3::input_time_rng::set_mouse_sensitivity(state, 1.99);
    test.expect_equal(
        state.sensitivity_scale,
        19,
        "sensitivity conversion truncates toward zero"
    );
    openswd3::input_time_rng::set_mouse_sensitivity(state, -1.99);
    test.expect_equal(
        state.sensitivity_scale,
        -19,
        "negative sensitivity conversion also truncates toward zero"
    );
}

void test_mouse_clamp_baseline_quirks(openswd3::test::Context& test) {
    LegacyMouseState state{0, 0, 20};
    const LegacyMouseDeviceSample upper{320, 240, 0x80U, 0x80U};
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, upper),
        LegacyMouseFrame{639, 479, 3U},
        "upper overflow clamps the current frame and combines both buttons"
    );
    test.expect_equal(
        state.absolute_x_baseline,
        1,
        "x upper clamp uses truncated 6390 divided by scale"
    );
    test.expect_equal(
        state.absolute_y_baseline,
        1,
        "y upper clamp uses truncated 4790 divided by scale"
    );
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, upper),
        LegacyMouseFrame{638, 478, 3U},
        "next unchanged sample exposes the original upper-edge fallback"
    );

    state = LegacyMouseState{100, 200, 20};
    const LegacyMouseDeviceSample lower{99, 199, 0x7FU, 0xFFU};
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, lower),
        LegacyMouseFrame{0, 0, 2U},
        "negative logical coordinates clamp and only high button bits count"
    );
    test.expect_equal(
        state.absolute_x_baseline,
        99,
        "lower x clamp stores the current absolute axis"
    );
    test.expect_equal(
        state.absolute_y_baseline,
        199,
        "lower y clamp stores the current absolute axis"
    );
}

void test_mouse_rebase_clamps_and_wraps(openswd3::test::Context& test) {
    LegacyMouseState state{0, 0, 20};
    const LegacyMouseDeviceSample sample{1000, 2000, 0U, 0U};
    openswd3::input_time_rng::rebase_mouse_coordinates(
        state,
        sample,
        640,
        -1
    );
    test.expect_equal(
        state.absolute_x_baseline,
        690,
        "rebase clamps x to 639 then performs divide-before-multiply"
    );
    test.expect_equal(
        state.absolute_y_baseline,
        2000,
        "rebase clamps nonpositive y to zero"
    );
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, sample),
        LegacyMouseFrame{620, 0, 0U},
        "rebase retains its original integer truncation"
    );

    state = LegacyMouseState{0x7FFFFFFF, 0, 20};
    const LegacyMouseDeviceSample wrapped{
        std::numeric_limits<openswd3::compat::i32>::min(),
        0,
        0U,
        0U,
    };
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, wrapped),
        LegacyMouseFrame{2, 0, 0U},
        "axis subtraction and multiplication use 32-bit wrapping"
    );

    state = LegacyMouseState{123, 456, 0};
    test.expect_equal(
        openswd3::input_time_rng::normalize_mouse_sample(state, sample),
        LegacyMouseFrame{0, 0, 0U},
        "zero scale remains observable until a dividing path is reached"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_default_bindings(test);
    test_release_transitions(test);
    test_press_transitions(test);
    test_zero_clock_release_quirk(test);
    test_raw_keyboard_snapshot(test);
    test_mouse_sensitivity_and_rebase(test);
    test_mouse_clamp_baseline_quirks(test);
    test_mouse_rebase_clamps_and_wraps(test);
    return test.exit_code();
}
