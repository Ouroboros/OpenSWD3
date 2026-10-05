#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>

namespace openswd3::resource_io {

// sub_409C10: visit Save/0.sav through Save/98.sav in ascending order and
// return whether at least one file can be opened for reading and writing.
[[nodiscard]] bool
scan_legacy_save_slots(const std::filesystem::path& data_directory);

// sub_4070A0: save slot navigation clamps to the inclusive 0..98 range.
enum class LegacySaveSlotMove { previous, next, previous_page, next_page };

[[nodiscard]] constexpr std::uint32_t move_legacy_save_slot(
    const std::uint32_t current, const LegacySaveSlotMove move
) noexcept {
    const auto delta = move == LegacySaveSlotMove::previous ? -1
        : move == LegacySaveSlotMove::next                  ? 1
        : move == LegacySaveSlotMove::previous_page         ? -3
                                                            : 3;
    return static_cast<std::uint32_t>(
        std::clamp(static_cast<int>(current) + delta, 0, 98)
    );
}

}  // namespace openswd3::resource_io
