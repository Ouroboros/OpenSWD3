#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_movement_collision.hpp"
#include "openswd3/world_map/legacy_world_map_business.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr compat::u16 kLegacyWorldTalkIdleSource = 0xFFFFU;
inline constexpr compat::u16 kLegacyWorldTalkMapEventSource = 0xFFFDU;
inline constexpr compat::u32 kLegacyWorldTalkTurningRoleFlag = 0x00000800U;

struct LegacyWorldTalkContext {
    compat::u32 field_00{};
    compat::u32 world_x{};
    compat::u32 world_y{};
    compat::u32 field_0c{};
    compat::u32 source_flags{};
    compat::u32 talk_data_offset{};
    compat::u32 field_18{};
    compat::u16 field_1c{};
    compat::u16 talk_script_id{};
    compat::u16 instruction_offset{};
    compat::u16 field_22{};
    compat::u16 source_guid{kLegacyWorldTalkIdleSource};
    compat::u16 field_26{};
    std::array<compat::u8, 0xB0> tail{};
};

static_assert(sizeof(LegacyWorldTalkContext) == 0xD8U);
static_assert(offsetof(LegacyWorldTalkContext, world_x) == 0x04U);
static_assert(offsetof(LegacyWorldTalkContext, world_y) == 0x08U);
static_assert(offsetof(LegacyWorldTalkContext, source_flags) == 0x10U);
static_assert(offsetof(LegacyWorldTalkContext, talk_data_offset) == 0x14U);
static_assert(offsetof(LegacyWorldTalkContext, talk_script_id) == 0x1EU);
static_assert(offsetof(LegacyWorldTalkContext, instruction_offset) == 0x20U);
static_assert(offsetof(LegacyWorldTalkContext, source_guid) == 0x24U);

class LegacyWorldCollisionTalkPorts {
public:
    virtual ~LegacyWorldCollisionTalkPorts() = default;

    [[nodiscard]] virtual LegacyMovementCollisionResult query_collision(
        compat::u32 role_index, compat::i32 delta_x, compat::i32 delta_y
    ) = 0;

    [[nodiscard]] virtual compat::u32
    query_internal_flag(compat::u32 bit_index) = 0;

    [[nodiscard]] virtual compat::u32
    update_action(asset_runtime::LegacyActionRecord& action) = 0;
};

enum class LegacyWorldCollisionTalkStatus {
    completed,
    invalid_player_index,
    collision_query_failed,
    invalid_hit_role_index,
    missing_map_event,
    invalid_player_direction,
};

enum class LegacyWorldTalkSource {
    none,
    map_event,
    role,
};

struct LegacyWorldCollisionTalkRequest {
    compat::u32 player_index{};
    compat::i32 adjusted_delta_x{};
    compat::i32 adjusted_delta_y{};
    compat::i32 original_delta_x{};
    compat::i32 original_delta_y{};
};

struct LegacyWorldCollisionTalkResult {
    LegacyWorldCollisionTalkStatus status{
        LegacyWorldCollisionTalkStatus::completed
    };
    LegacyWorldTalkSource talk_source{LegacyWorldTalkSource::none};
    compat::i32 delta_x{};
    compat::i32 delta_y{};
    compat::u32 event_code{};
    compat::u32 hit_role_index{kLegacyMovementCollisionNoRole};
    compat::u32 collision_query_count{};
    bool map_event_stopped_motion{};
    bool target_action_update_failed{};
    bool post_player_turn_target_update_failed{};
};

[[nodiscard]] LegacyWorldCollisionTalkResult
coordinate_legacy_world_collision_talk(
    LegacyWorldCollisionTalkRequest request,
    std::span<LegacyWorldRoleRecord> roles,
    std::span<const LegacyWorldMapEvent> map_events,
    LegacyWorldTalkContext& talk_context,
    compat::u32& one_shot_interaction_state,
    LegacyWorldCollisionTalkPorts& ports
);

}  // namespace openswd3::world_map
