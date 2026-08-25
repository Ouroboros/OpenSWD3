#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleRenderGeometry {
    compat::i32 surface_width{};
    compat::i32 surface_height{};
    compat::i32 left{};
    compat::i32 top{};
    compat::i32 right{};
    compat::i32 bottom{};
};

// sub_4342E0. The final two parameters are dimensions, not absolute edges.
compat::i32 set_legacy_battle_render_rectangle(
    LegacyBattleRenderGeometry& geometry,
    compat::i32 left,
    compat::i32 top,
    compat::i32 width,
    compat::i32 height
) noexcept;

}  // namespace openswd3::battle
