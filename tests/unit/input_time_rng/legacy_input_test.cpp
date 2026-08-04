#include "test.hpp"

#include "openswd3/input_time_rng/legacy_input.hpp"

#include <array>

namespace {

using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::input_time_rng::LegacyKeyBinding;
using openswd3::input_time_rng::LegacyKeyBindingBlock;

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

}  // namespace

int main() {
    openswd3::test::Context test;
    test_default_bindings(test);
    test_release_transitions(test);
    test_press_transitions(test);
    test_zero_clock_release_quirk(test);
    return test.exit_code();
}
