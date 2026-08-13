#include "openswd3/world_map/legacy_world_selection_scroll.hpp"

#include <bit>

namespace openswd3::world_map {
namespace {

[[nodiscard]] compat::i32 wrapping_decrement(const compat::i32 value) noexcept {
    return std::bit_cast<compat::i32>(std::bit_cast<compat::u32>(value) - 1U);
}

[[nodiscard]] compat::u32 signed_delta_bits(const compat::i16 value) noexcept {
    return std::bit_cast<compat::u32>(static_cast<compat::i32>(value));
}

}  // namespace

LegacyWorldSelectionScrollStatus advance_legacy_world_selection_scroll(
    const std::span<const compat::i16> selection_words,
    const compat::u32 map_id,
    LegacyWorldCameraRect& camera,
    LegacyWorldSelectionScrollState& state
) noexcept {
    if (selection_words.empty()) {
        return LegacyWorldSelectionScrollStatus::invalid_selection_window;
    }
    if (std::bit_cast<compat::u16>(selection_words.front()) ==
        kLegacyWorldSelectionSentinel) {
        return LegacyWorldSelectionScrollStatus::selection_inactive;
    }
    if (map_id == 0x16U) {
        return LegacyWorldSelectionScrollStatus::map_excluded;
    }

    compat::u32 cursor = state.cursor_word_index;
    if (cursor >= selection_words.size()) {
        return LegacyWorldSelectionScrollStatus::invalid_selection_window;
    }
    if (std::bit_cast<compat::u16>(selection_words[cursor]) ==
        kLegacyWorldSelectionSentinel) {
        cursor = 0U;
        state.cursor_word_index = 0U;
    }
    if (cursor >= selection_words.size() ||
        selection_words.size() - cursor < 2U) {
        return LegacyWorldSelectionScrollStatus::invalid_selection_window;
    }

    const compat::u32 delta_x = signed_delta_bits(selection_words[cursor]);
    const compat::u32 delta_y = signed_delta_bits(selection_words[cursor + 1U]);

    state.frames_remaining = wrapping_decrement(state.frames_remaining);
    if (state.frames_remaining <= 0) {
        state.frames_remaining = state.frame_interval;
        state.cursor_word_index = cursor + 2U;
    }

    state.saved_left = camera.left;
    camera.left += delta_x;
    camera.right += delta_x;
    state.saved_top = camera.top;
    camera.top += delta_y;
    camera.bottom += delta_y;
    return LegacyWorldSelectionScrollStatus::completed;
}

}  // namespace openswd3::world_map
