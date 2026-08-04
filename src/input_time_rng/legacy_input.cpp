#include "openswd3/input_time_rng/legacy_input.hpp"

namespace openswd3::input_time_rng {

namespace {

constexpr std::array<std::size_t, 16> kBindingWordIndices{
    0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U,
    8U, 9U, 10U, 12U, 16U, 17U, 18U, 19U,
};

constexpr std::array<compat::u32, 16> kDefaultBindings{
    0x01U, 0x39U, 0x36U, 0xCBU,
    0xC8U, 0xCDU, 0xD0U, 0xC9U,
    0xD1U, 0x9DU, 0xCFU, 0x1CU,
    0x13U, 0x1EU, 0x22U, 0x3BU,
};

constexpr std::array<compat::u32, 4> kRapidPressMultiplicity{
    1U,
    2U,
    3U,
    3U,
};

[[nodiscard]] constexpr std::size_t binding_index(
    const LegacyKeyBinding binding
) noexcept {
    return kBindingWordIndices[static_cast<std::size_t>(binding)];
}

}  // namespace

void initialize_default_key_bindings(LegacyKeyBindingBlock& block) noexcept {
    for (std::size_t index = 0U; index < kDefaultBindings.size(); ++index) {
        block.words[kBindingWordIndices[index]] = kDefaultBindings[index];
    }
}

compat::u32& key_binding(
    LegacyKeyBindingBlock& block,
    const LegacyKeyBinding binding
) noexcept {
    return block.words[binding_index(binding)];
}

const compat::u32& key_binding(
    const LegacyKeyBindingBlock& block,
    const LegacyKeyBinding binding
) noexcept {
    return block.words[binding_index(binding)];
}

compat::u32 update_input_record(
    LegacyInputRecord& record,
    const compat::u32 raw_state,
    const compat::u32 current_input_milliseconds
) noexcept {
    if (raw_state == 0U) {
        record.held_sample_count = 0U;
        if (record.release_milliseconds == 0U) {
            record.release_milliseconds = current_input_milliseconds;
            return 0U;
        }

        record.rapid_press_multiplicity = 0U;
        const compat::u32 elapsed =
            current_input_milliseconds - record.release_milliseconds;
        if (elapsed > 150U) {
            record.rapid_press_stage = 0U;
        }

        return record.held_sample_count;
    }

    if (record.release_milliseconds != 0U) {
        const compat::u32 multiplicity =
            kRapidPressMultiplicity[record.rapid_press_stage];
        record.held_sample_count = 0U;
        record.rapid_press_multiplicity = multiplicity;
        record.rapid_press_stage = multiplicity;
    }

    record.release_milliseconds = 0U;
    record.held_sample_count += 1U;
    return record.held_sample_count;
}

compat::u32 read_raw_key(
    const LegacyKeyboardSnapshot& snapshot,
    const compat::u32 dik_code
) noexcept {
    return static_cast<compat::u32>(snapshot[dik_code] & 0x80U);
}

void synthesize_raw_key(
    LegacyKeyboardSnapshot& snapshot,
    const compat::u32 dik_code
) noexcept {
    snapshot[dik_code] |= 0x80U;
}

compat::u32 find_first_pressed_key(
    const LegacyKeyboardSnapshot& snapshot
) noexcept {
    for (std::size_t index = 0U; index < snapshot.size(); ++index) {
        if ((snapshot[index] & 0x80U) != 0U) {
            return static_cast<compat::u32>(index);
        }
    }

    return 0U;
}

}  // namespace openswd3::input_time_rng
