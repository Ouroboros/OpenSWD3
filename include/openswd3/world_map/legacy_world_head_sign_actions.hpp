#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::world_map {

inline constexpr std::size_t kLegacyWorldHeadSignActionCount = 8U;
inline constexpr compat::u32 kLegacyWorldHeadSignActionId = 0x232EU;

struct LegacyWorldHeadSignActionsState {
    LegacyWorldHeadSignActionsState() noexcept;

    std::array<asset_runtime::LegacyActionRecord,
               kLegacyWorldHeadSignActionCount>
        records{};
};

enum class LegacyWorldHeadSignActionsStatus : compat::u8 {
    completed,
    completed_with_update_failures,
};

struct LegacyWorldHeadSignActionsResult {
    LegacyWorldHeadSignActionsStatus status{
        LegacyWorldHeadSignActionsStatus::completed};
    compat::u32 visited_count{};
    compat::u32 active_count{};
    compat::u32 update_count{};
    compat::u32 update_failure_count{};
};

// 0x004120B7..0x004120F7: visit the eight shared head-sign action records in
// descending address order, skip zero action ids, and keep going after an
// updater failure (the original failure path only calls a diagnostic nullsub).
[[nodiscard]] LegacyWorldHeadSignActionsResult
advance_legacy_world_head_sign_actions(
    LegacyWorldHeadSignActionsState& state,
    asset_runtime::LegacyActionDrawPorts& action_ports);

}  // namespace openswd3::world_map
