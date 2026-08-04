#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::input_time_rng {

inline constexpr std::size_t kLegacyKeyBindingBlockSize = 0x80U;
inline constexpr std::size_t kLegacyKeyBindingWordCount =
    kLegacyKeyBindingBlockSize / sizeof(compat::u32);
inline constexpr std::size_t kLegacyKeyboardSnapshotSize = 0x100U;
inline constexpr std::size_t kLegacyInputRecordCount = 20U;

using LegacyKeyboardSnapshot =
    std::array<compat::u8, kLegacyKeyboardSnapshotSize>;

enum class LegacyKeyBinding : std::size_t {
    cancel,
    primary_action,
    configurable_2,
    left,
    up,
    right,
    down,
    page_up,
    page_down,
    configurable_9,
    configurable_10,
    alternate_action,
    configurable_16,
    configurable_17,
    configurable_18,
    configurable_19,
};

struct LegacyKeyBindingBlock {
    std::array<compat::u32, kLegacyKeyBindingWordCount> words{};
};

void initialize_default_key_bindings(
    LegacyKeyBindingBlock& block
) noexcept;

[[nodiscard]] compat::u32& key_binding(
    LegacyKeyBindingBlock& block,
    LegacyKeyBinding binding
) noexcept;

[[nodiscard]] const compat::u32& key_binding(
    const LegacyKeyBindingBlock& block,
    LegacyKeyBinding binding
) noexcept;

struct LegacyInputRecord {
    compat::u32 rapid_press_multiplicity{};
    compat::u32 release_milliseconds{};
    compat::u32 rapid_press_stage{};
    compat::u32 held_sample_count{};

    bool operator==(const LegacyInputRecord&) const = default;
};

struct LegacyMouseState {
    compat::i32 absolute_x_baseline{};
    compat::i32 absolute_y_baseline{};
    compat::i32 sensitivity_scale{};
};

struct LegacyMouseDeviceSample {
    compat::i32 absolute_x{};
    compat::i32 absolute_y{};
    compat::u8 button_0{};
    compat::u8 button_1{};
};

struct LegacyMouseFrame {
    compat::i32 logical_x{};
    compat::i32 logical_y{};
    compat::u32 button_mask{};

    bool operator==(const LegacyMouseFrame&) const = default;
};

struct LegacyInputNormalizationState {
    LegacyKeyBindingBlock key_bindings{};
    std::array<LegacyInputRecord, kLegacyInputRecordCount> records{};
    LegacyMouseFrame current_mouse{};
    compat::i32 previous_mouse_x{};
    compat::i32 previous_mouse_y{};
    compat::u32 current_input_milliseconds{};
    compat::u32 last_normalized_input_milliseconds{};
    compat::u32 left_button_suppression_count{};
    compat::u32 mouse_inactivity_sample_count{};
    bool mouse_inactive_flag_9{};
};

[[nodiscard]] compat::u32 update_input_record(
    LegacyInputRecord& record,
    compat::u32 raw_state,
    compat::u32 current_input_milliseconds
) noexcept;

[[nodiscard]] compat::u32 read_raw_key(
    const LegacyKeyboardSnapshot& snapshot,
    compat::u32 dik_code
) noexcept;

void synthesize_raw_key(
    LegacyKeyboardSnapshot& snapshot,
    compat::u32 dik_code
) noexcept;

[[nodiscard]] compat::u32 find_first_pressed_key(
    const LegacyKeyboardSnapshot& snapshot
) noexcept;

void set_mouse_sensitivity(
    LegacyMouseState& state,
    double sensitivity
) noexcept;

void rebase_mouse_coordinates(
    LegacyMouseState& state,
    const LegacyMouseDeviceSample& sample,
    compat::i32 target_x,
    compat::i32 target_y
) noexcept;

[[nodiscard]] LegacyMouseFrame normalize_mouse_sample(
    LegacyMouseState& state,
    const LegacyMouseDeviceSample& sample
) noexcept;

void begin_input_normalization(
    LegacyInputNormalizationState& state,
    compat::u32 current_milliseconds
) noexcept;

void normalize_input_frame(
    LegacyInputNormalizationState& state,
    LegacyMouseState& mouse_state,
    const LegacyKeyboardSnapshot& keyboard_snapshot,
    const LegacyMouseDeviceSample& mouse_sample
) noexcept;

static_assert(sizeof(LegacyKeyBindingBlock) == kLegacyKeyBindingBlockSize);
static_assert(sizeof(LegacyKeyboardSnapshot) == kLegacyKeyboardSnapshotSize);
static_assert(sizeof(LegacyInputRecord) == 0x10U);
static_assert(
    sizeof(std::array<LegacyInputRecord, kLegacyInputRecordCount>) == 0x140U
);
static_assert(offsetof(LegacyInputRecord, rapid_press_multiplicity) == 0x00U);
static_assert(offsetof(LegacyInputRecord, release_milliseconds) == 0x04U);
static_assert(offsetof(LegacyInputRecord, rapid_press_stage) == 0x08U);
static_assert(offsetof(LegacyInputRecord, held_sample_count) == 0x0CU);

}  // namespace openswd3::input_time_rng
