#include "openswd3/battle/legacy_battle_render_geometry.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

[[nodiscard]] compat::i32
wrapping_add(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) + std::bit_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return std::bit_cast<compat::i32>(
        std::bit_cast<compat::u32>(left) - std::bit_cast<compat::u32>(right)
    );
}

}  // namespace

compat::i32 set_legacy_battle_render_rectangle(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 left,
    compat::i32 top,
    compat::i32 width,
    compat::i32 height
) noexcept {
    if (left < 0) {
        width = wrapping_add(width, left);
        left = 0;
    }
    if (top < 0) {
        height = wrapping_add(height, top);
        top = 0;
    }

    if (wrapping_add(left, width) >= geometry.surface_width) {
        left = wrapping_subtract(geometry.surface_width, width);
    }
    if (wrapping_add(top, height) >= geometry.surface_height) {
        top = wrapping_subtract(geometry.surface_height, height);
    }

    geometry.top = top;
    geometry.left = left;
    geometry.right = wrapping_add(left, width);
    geometry.bottom = wrapping_add(top, height);
    return geometry.bottom;
}

}  // namespace openswd3::battle
