#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"

#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr compat::u16 kLegacyWorldSelectionSentinel = 0xCFCFU;
inline constexpr std::size_t kLegacyWorldSelectionWordCount = 64U;

struct LegacyWorldSelectionScrollState {
    compat::u32 cursor_word_index{};
    compat::i32 frames_remaining{};
    compat::i32 frame_interval{};
    compat::u32 saved_left{};
    compat::u32 saved_top{};
};

enum class LegacyWorldSelectionScrollStatus : compat::u8 {
    completed,
    selection_inactive,
    map_excluded,
    invalid_selection_window,
};

[[nodiscard]] LegacyWorldSelectionScrollStatus
advance_legacy_world_selection_scroll(
    std::span<const compat::i16> selection_words,
    compat::u32 map_id,
    LegacyWorldCameraRect& camera,
    LegacyWorldSelectionScrollState& state
) noexcept;

}  // namespace openswd3::world_map
