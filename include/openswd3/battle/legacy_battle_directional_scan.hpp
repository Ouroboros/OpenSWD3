#pragma once

#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_frame_color.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleDirectionalScanSource {
    std::span<const compat::u8> pixels{};
    compat::u16 width{};
    compat::u16 height{};
    compat::i32 start_x{};
    compat::i32 start_y{};
    compat::i32 horizontal_divisor{};
    compat::i32 vertical_divisor{};
    compat::u16 flags{};
    compat::i32 published_value_2c{};
    compat::i32 published_value_30{};
    compat::i32 published_value_34{};
    compat::i32 direction_index{};
};

struct LegacyBattleDirectionalSurface {
    compat::i32 width{};
    compat::i32 height{};
    std::span<const compat::u32> row_offsets{};
    std::span<compat::u16> pixels{};
    // Optional original pointer tokens: [surface+0xB44] and arg_0.
    compat::u32 row_offsets_token{};
    compat::u32 pixels_token{};
    bool row_offsets_token_known{};
    bool pixels_token_known{};
};

struct LegacyBattleDirectionalScanSharedState {
    compat::i32 published_value_2c{};
    compat::i32 published_value_30{};
    compat::i32 published_value_34{};
    compat::u16 first_transparent_color{0x319FU};
    compat::u16 second_transparent_color{0x026BU};
};

enum class LegacyBattleDirectionalScanStatus : compat::u8 {
    completed,
    horizontal_divisor_zero,
    vertical_divisor_zero,
    direction_index_out_of_range,
    source_out_of_range,
    row_table_out_of_range,
    destination_out_of_range,
    frame_color_failed,
};

struct LegacyBattleDirectionalScanResult {
    LegacyBattleDirectionalScanStatus status{
        LegacyBattleDirectionalScanStatus::completed
    };
    rendering::LegacyFrameColorStatus frame_color_status{
        rendering::LegacyFrameColorStatus::completed
    };
    compat::i32 legacy_return_value{};
    // Exact instruction for division by zero, source/row reads and known
    // direct/blended pixel faults; unresolved nested failures retain zero.
    compat::u32 stopped_instruction{};
    compat::u32 stopped_source_byte_offset{};
    compat::u32 stopped_surface_token{};
    bool stopped_surface_token_known{};
    compat::u32 outer_iterations{};
    compat::u32 inner_iterations{};
    compat::u32 bounds_skips{};
    compat::u32 transparent_skips{};
    compat::u32 direct_writes{};
    compat::u32 combined_writes{};
};

// sub_4344E0.
[[nodiscard]] LegacyBattleDirectionalScanResult
scan_legacy_battle_directional_surface(
    const LegacyBattleDirectionVectors& vectors,
    const LegacyBattleDirectionalScanSource& source,
    const LegacyBattleDirectionalSurface& destination,
    LegacyBattleDirectionalScanSharedState& shared,
    rendering::LegacyPixelConversionState& pixel_format
) noexcept;

}  // namespace openswd3::battle
