#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <list>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyMovingActionNodeSize = 0xB4U;

struct LegacyMovingActionNode {
    asset_runtime::LegacyActionRecord action{};
    compat::i16 start_x{};
    compat::i16 start_y{};
    compat::i16 target_x{};
    compat::i16 target_y{};
    float velocity_x{};
    float velocity_y{};
    float position_x{};
    float position_y{};
    compat::u32 next_pointer_32{};
};

static_assert(sizeof(LegacyMovingActionNode) == kLegacyMovingActionNodeSize);
static_assert(offsetof(LegacyMovingActionNode, action) == 0x00U);
static_assert(offsetof(LegacyMovingActionNode, start_x) == 0x98U);
static_assert(offsetof(LegacyMovingActionNode, start_y) == 0x9AU);
static_assert(offsetof(LegacyMovingActionNode, target_x) == 0x9CU);
static_assert(offsetof(LegacyMovingActionNode, target_y) == 0x9EU);
static_assert(offsetof(LegacyMovingActionNode, velocity_x) == 0xA0U);
static_assert(offsetof(LegacyMovingActionNode, velocity_y) == 0xA4U);
static_assert(offsetof(LegacyMovingActionNode, position_x) == 0xA8U);
static_assert(offsetof(LegacyMovingActionNode, position_y) == 0xACU);
static_assert(offsetof(LegacyMovingActionNode, next_pointer_32) == 0xB0U);

using LegacyMovingActionList = std::list<LegacyMovingActionNode>;

struct LegacyMovingActionReleaseResult {
    compat::u32 node_release_count{};
};

// sub_40F540 (0x0040F540..0x0040F567): detach and release every 0xB4
// dword_4AD3E8 node from the list head.
[[nodiscard]] LegacyMovingActionReleaseResult
release_legacy_moving_actions(LegacyMovingActionList& nodes) noexcept;

struct LegacyMovingActionResult {
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

// sub_414B60: update, draw, move and retire the dword_4AD3E8 list. Modern
// list links replace host pointers, while each element retains the exact 0xB4
// payload including the legacy 32-bit next slot at +0xB0.
[[nodiscard]] LegacyMovingActionResult update_draw_legacy_moving_actions(
    LegacyMovingActionList& nodes,
    compat::i32 camera_left,
    compat::i32 camera_top,
    asset_runtime::LegacyActionDrawPorts& action_ports
);

}  // namespace openswd3::world_map
