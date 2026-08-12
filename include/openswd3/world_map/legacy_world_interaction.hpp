#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/input_time_rng/legacy_input.hpp"
#include "openswd3/world_map/legacy_world_collision_talk.hpp"
#include "openswd3/world_map/legacy_world_player_motion.hpp"

#include <cstddef>
#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldInteractionNoRole = 0xFFFFFFFFU;
inline constexpr compat::u32 kLegacyWorldDefaultCursorVariant = 0x0DU;
inline constexpr compat::u32 kLegacyWorldRoleCursorVariant = 0x09U;
inline constexpr compat::u32 kLegacyWorldBlockingRoleCursorVariant = 0x0AU;
inline constexpr compat::u32 kLegacyWorldMapEventCursorVariant = 0x0BU;

struct LegacyWorldInteractionHotspot {
    compat::u16 left{};
    compat::u16 top{};
    compat::u16 right{};
    compat::u16 bottom{};
};

struct LegacyWorldInteractionState {
    compat::u32 cursor_variant{kLegacyWorldDefaultCursorVariant};
    compat::u32 global_lock{};
    compat::u32 selected_choice_index{};
};

struct LegacyWorldInteractionRequest {
    compat::u32 player_index{};
    compat::u32 mouse_x{};
    compat::u32 mouse_y{};
    compat::u32 map_width{};
    LegacyWorldCameraRect camera;
    std::span<const LegacyWorldInteractionHotspot> choice_hotspots;
    bool dialog_chain_active{};
};

class LegacyWorldInteractionPorts {
public:
    virtual ~LegacyWorldInteractionPorts() = default;

    [[nodiscard]] virtual compat::u32 query_internal_flag(
        compat::u32 bit_index
    ) = 0;

    [[nodiscard]] virtual bool load_role_frame_size(
        compat::u16 resource_id,
        compat::u16 frame_index,
        compat::u16& width,
        compat::u16& height
    ) = 0;

    [[nodiscard]] virtual compat::u32 update_action(
        asset_runtime::LegacyActionRecord& action
    ) = 0;
};

enum class LegacyWorldInteractionStatus : compat::u8 {
    completed,
    invalid_player_index,
    missing_input_records,
    invalid_surface_grid,
    missing_map_event,
};

enum class LegacyWorldInteractionSource : compat::u8 {
    none,
    choice,
    role,
    map_event,
};

struct LegacyWorldInteractionResult {
    LegacyWorldInteractionStatus status{
        LegacyWorldInteractionStatus::completed
    };
    LegacyWorldInteractionSource source{LegacyWorldInteractionSource::none};
    compat::u32 hovered_role_index{kLegacyWorldInteractionNoRole};
    compat::u32 map_event_code{};
    compat::u32 facing{};
    compat::u32 distance{};
    compat::u32 role_frames_requested{};
    compat::u32 unavailable_role_frames{};
    compat::u32 internal_flag_queries{};
    compat::u32 action_update_count{};
    compat::u32 action_update_failure_count{};
    bool choice_chain_clear_requested{};
    bool primary_input_cleared{};
    bool delayed_primary_input_copied{};
    bool map_event_suppressed{};
};

// sub_427300 (0x00427300..0x00427919): resolve world mouse hover,
// dialog-choice clicks, role/map Talk activation, mouse-directed movement and
// the left-click-to-primary-input bridge. Valid-input behavior follows the
// IA-32 branches and unsigned wrapping comparisons; invalid modern spans stop
// before the original unchecked memory access.
[[nodiscard]] LegacyWorldInteractionResult coordinate_legacy_world_interaction(
    const LegacyWorldInteractionRequest& request,
    std::span<LegacyWorldRoleRecord> roles,
    std::span<const LegacyWorldMapEvent> map_events,
    std::span<const compat::u8> surface_grid,
    std::span<input_time_rng::LegacyInputRecord> input_records,
    LegacyWorldTalkContext& talk_context,
    LegacyWorldInteractionState& state,
    LegacyWorldInteractionPorts& ports
);

}  // namespace openswd3::world_map
