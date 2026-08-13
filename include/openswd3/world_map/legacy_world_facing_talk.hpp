#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_world_facing_role_query.hpp"

#include <span>

namespace openswd3::world_map {

class LegacyWorldFacingTalkPorts {
public:
    virtual ~LegacyWorldFacingTalkPorts() = default;

    [[nodiscard]] virtual compat::u32
    update_action(asset_runtime::LegacyActionRecord& action) = 0;
};

enum class LegacyWorldFacingTalkStatus {
    completed,
    invalid_player_index,
};

struct LegacyWorldFacingTalkRequest {
    compat::u32 player_index{};
    compat::i32 delta_x{};
    compat::i32 delta_y{};
    compat::u32 map_width{};
    compat::u32 map_height{};
};

struct LegacyWorldFacingTalkResult {
    LegacyWorldFacingTalkStatus status{LegacyWorldFacingTalkStatus::completed};
    compat::u32 role_index{kLegacyWorldFacingRoleNotFound};
    compat::u32 tile_query_count{};
    compat::u32 facing{};
    compat::u32 action_update_count{};
    compat::u32 action_update_failure_count{};
    bool talk_created{};
};

// sub_402F80 (0x004036D2..0x00403889): use the current facing direction to
// find a role, turn both actors, and populate the shared Talk context.
[[nodiscard]] LegacyWorldFacingTalkResult coordinate_legacy_world_facing_talk(
    const LegacyWorldFacingTalkRequest& request,
    std::span<LegacyWorldRoleRecord> roles,
    LegacyWorldTalkContext& talk_context,
    compat::u32& one_shot_interaction_state,
    LegacyWorldFacingTalkPorts& ports
) noexcept;

}  // namespace openswd3::world_map
