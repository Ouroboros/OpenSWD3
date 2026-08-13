#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <list>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyRoleHeadActionNodeSize = 0xB4U;

struct LegacyRoleHeadActionNode {
    asset_runtime::LegacyActionRecord action{};
    compat::i16 current_x{};
    compat::i16 horizontal_motion{};
    compat::i16 target_x{};
    compat::i16 y{};
    std::array<compat::u32, 4U> reserved_a0_af{};
    compat::u32 next_pointer_32{};
};

static_assert(
    sizeof(LegacyRoleHeadActionNode) == kLegacyRoleHeadActionNodeSize
);
static_assert(offsetof(LegacyRoleHeadActionNode, action) == 0x00U);
static_assert(offsetof(LegacyRoleHeadActionNode, current_x) == 0x98U);
static_assert(offsetof(LegacyRoleHeadActionNode, horizontal_motion) == 0x9AU);
static_assert(offsetof(LegacyRoleHeadActionNode, target_x) == 0x9CU);
static_assert(offsetof(LegacyRoleHeadActionNode, y) == 0x9EU);
static_assert(offsetof(LegacyRoleHeadActionNode, reserved_a0_af) == 0xA0U);
static_assert(offsetof(LegacyRoleHeadActionNode, next_pointer_32) == 0xB0U);

using LegacyRoleHeadActionList = std::list<LegacyRoleHeadActionNode>;

struct LegacyRoleHeadActionResult {
    compat::u32 visited_count{};
    compat::u32 action_update_failure_count{};
    compat::u32 frame_request_count{};
    compat::u32 frame_failure_count{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
    compat::u32 removed_count{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

// sub_414CE0: update, draw, move and retire the dword_4BA6E0 list. Modern
// list links replace host pointers, while each element retains the exact 0xB4
// payload including the legacy 32-bit next slot at +0xB0.
[[nodiscard]] LegacyRoleHeadActionResult update_draw_legacy_role_head_actions(
    LegacyRoleHeadActionList& nodes,
    asset_runtime::LegacyActionDrawPorts& action_ports
);

}  // namespace openswd3::world_map
