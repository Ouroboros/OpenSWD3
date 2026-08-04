#include "openswd3/input_time_rng/legacy_input.hpp"

#include <bit>
#include <cstdint>
#include <exception>

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

struct KeyboardRecordBinding {
    std::size_t record_index;
    LegacyKeyBinding binding;
};

constexpr std::array<KeyboardRecordBinding, 16> kKeyboardRecordUpdateOrder{
    KeyboardRecordBinding{0U, LegacyKeyBinding::cancel},
    KeyboardRecordBinding{1U, LegacyKeyBinding::primary_action},
    KeyboardRecordBinding{12U, LegacyKeyBinding::alternate_action},
    KeyboardRecordBinding{2U, LegacyKeyBinding::configurable_2},
    KeyboardRecordBinding{3U, LegacyKeyBinding::left},
    KeyboardRecordBinding{4U, LegacyKeyBinding::up},
    KeyboardRecordBinding{5U, LegacyKeyBinding::right},
    KeyboardRecordBinding{6U, LegacyKeyBinding::down},
    KeyboardRecordBinding{9U, LegacyKeyBinding::configurable_9},
    KeyboardRecordBinding{10U, LegacyKeyBinding::configurable_10},
    KeyboardRecordBinding{8U, LegacyKeyBinding::page_down},
    KeyboardRecordBinding{7U, LegacyKeyBinding::page_up},
    KeyboardRecordBinding{16U, LegacyKeyBinding::configurable_16},
    KeyboardRecordBinding{17U, LegacyKeyBinding::configurable_17},
    KeyboardRecordBinding{18U, LegacyKeyBinding::configurable_18},
    KeyboardRecordBinding{19U, LegacyKeyBinding::configurable_19},
};

[[nodiscard]] constexpr std::size_t binding_index(
    const LegacyKeyBinding binding
) noexcept {
    return kBindingWordIndices[static_cast<std::size_t>(binding)];
}

[[nodiscard]] compat::i32 wrap_subtract(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32 wrap_multiply(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) * std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32 scaled_mouse_axis(
    const compat::i32 absolute,
    const compat::i32 baseline,
    const compat::i32 scale
) noexcept {
    return wrap_multiply(wrap_subtract(absolute, baseline), scale) / 10;
}

[[nodiscard]] compat::i32 clamp_rebase_target(
    const compat::i32 value,
    const compat::i32 upper_bound
) noexcept {
    if (value >= upper_bound) {
        return upper_bound - 1;
    }

    if (value <= 0) {
        return 0;
    }

    return value;
}

[[nodiscard]] compat::i32 rebase_axis(
    const compat::i32 absolute,
    const compat::i32 target,
    const compat::i32 scale
) noexcept {
    if (scale == 0) {
        std::terminate();
    }

    return wrap_subtract(absolute, wrap_multiply(target / scale, 10));
}

[[nodiscard]] compat::i32 normalize_mouse_axis(
    compat::i32& baseline,
    const compat::i32 absolute,
    const compat::i32 scale,
    const compat::i32 maximum
) noexcept {
    compat::i32 logical = scaled_mouse_axis(absolute, baseline, scale);
    if (logical < 0) {
        baseline = absolute;
        return 0;
    }

    if (logical <= maximum) {
        return logical;
    }

    if (scale == 0) {
        std::terminate();
    }

    logical = maximum;
    baseline = wrap_subtract(absolute, (maximum * 10) / scale);
    return logical;
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

void set_mouse_sensitivity(
    LegacyMouseState& state,
    const double sensitivity
) noexcept {
    const auto scaled = static_cast<std::int64_t>(sensitivity * 10.0);
    state.sensitivity_scale = std::bit_cast<compat::i32>(
        static_cast<compat::u32>(scaled)
    );
}

void rebase_mouse_coordinates(
    LegacyMouseState& state,
    const LegacyMouseDeviceSample& sample,
    const compat::i32 target_x,
    const compat::i32 target_y
) noexcept {
    const compat::i32 clamped_x = clamp_rebase_target(target_x, 640);
    const compat::i32 clamped_y = clamp_rebase_target(target_y, 480);
    state.absolute_x_baseline = rebase_axis(
        sample.absolute_x,
        clamped_x,
        state.sensitivity_scale
    );
    state.absolute_y_baseline = rebase_axis(
        sample.absolute_y,
        clamped_y,
        state.sensitivity_scale
    );
}

LegacyMouseFrame normalize_mouse_sample(
    LegacyMouseState& state,
    const LegacyMouseDeviceSample& sample
) noexcept {
    compat::u32 button_mask{};
    if ((sample.button_0 & 0x80U) != 0U) {
        button_mask = 1U;
    }
    if ((sample.button_1 & 0x80U) != 0U) {
        button_mask |= 2U;
    }

    const compat::i32 logical_x = normalize_mouse_axis(
        state.absolute_x_baseline,
        sample.absolute_x,
        state.sensitivity_scale,
        639
    );
    const compat::i32 logical_y = normalize_mouse_axis(
        state.absolute_y_baseline,
        sample.absolute_y,
        state.sensitivity_scale,
        479
    );
    return {logical_x, logical_y, button_mask};
}

void begin_input_normalization(
    LegacyInputNormalizationState& state,
    const compat::u32 current_milliseconds
) noexcept {
    state.current_input_milliseconds = current_milliseconds;
}

void normalize_input_frame(
    LegacyInputNormalizationState& state,
    LegacyMouseState& mouse_state,
    const LegacyKeyboardSnapshot& keyboard_snapshot,
    const LegacyMouseDeviceSample& mouse_sample
) noexcept {
    state.current_mouse = normalize_mouse_sample(mouse_state, mouse_sample);

    for (const auto& item : kKeyboardRecordUpdateOrder) {
        const compat::u32 raw_state = read_raw_key(
            keyboard_snapshot,
            key_binding(state.key_bindings, item.binding)
        );
        static_cast<void>(update_input_record(
            state.records[item.record_index],
            raw_state,
            state.current_input_milliseconds
        ));
    }

    static_cast<void>(update_input_record(
        state.records[15U],
        state.current_mouse.button_mask & 1U,
        state.current_input_milliseconds
    ));
    static_cast<void>(update_input_record(
        state.records[14U],
        state.current_mouse.button_mask & 2U,
        state.current_input_milliseconds
    ));

    state.last_normalized_input_milliseconds =
        state.current_input_milliseconds;
    if (state.left_button_suppression_count != 0U) {
        state.left_button_suppression_count -= 1U;
        state.records[15U] = {};
    }

    state.mouse_inactive_flag_9 = false;
    if (state.current_mouse.logical_x == state.previous_mouse_x &&
        state.current_mouse.logical_y == state.previous_mouse_y &&
        state.current_mouse.button_mask == 0U) {
        state.mouse_inactivity_sample_count += 1U;
        if (state.mouse_inactivity_sample_count > 450U) {
            state.mouse_inactive_flag_9 = true;
        }
    } else {
        state.mouse_inactivity_sample_count = 0U;
    }

    state.previous_mouse_x = state.current_mouse.logical_x;
    state.previous_mouse_y = state.current_mouse.logical_y;
}

}  // namespace openswd3::input_time_rng
