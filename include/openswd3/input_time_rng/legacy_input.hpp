#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::input_time_rng {

inline constexpr std::size_t kLegacyKeyBindingBlockSize = 0x80U;
inline constexpr std::size_t kLegacyKeyBindingWordCount =
    kLegacyKeyBindingBlockSize / sizeof(compat::u32);

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

[[nodiscard]] compat::u32 update_input_record(
    LegacyInputRecord& record,
    compat::u32 raw_state,
    compat::u32 current_input_milliseconds
) noexcept;

static_assert(sizeof(LegacyKeyBindingBlock) == kLegacyKeyBindingBlockSize);
static_assert(sizeof(LegacyInputRecord) == 0x10U);
static_assert(offsetof(LegacyInputRecord, rapid_press_multiplicity) == 0x00U);
static_assert(offsetof(LegacyInputRecord, release_milliseconds) == 0x04U);
static_assert(offsetof(LegacyInputRecord, rapid_press_stage) == 0x08U);
static_assert(offsetof(LegacyInputRecord, held_sample_count) == 0x0CU);

}  // namespace openswd3::input_time_rng
