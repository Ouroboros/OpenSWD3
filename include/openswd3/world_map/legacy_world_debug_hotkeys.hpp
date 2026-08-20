#pragma once

#include "openswd3/compat/types.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldDebugEnableFlag = 0x52U;
inline constexpr compat::u32 kLegacyWorldDebugCollisionFlag = 0x14U;

struct LegacyWorldDebugHotkeyState {
    compat::u32 fixed_debug_speed{};
    compat::u32 diagnostic_text_visible{};
    compat::u32 collision_grid_visible{};
    compat::u32 tile_animation_interval{1U};
    compat::u32 world_frame_count{};
    compat::u32 money{};
    compat::u32 modal_state{};
    compat::u32 modal_option_a{};
    compat::u32 modal_option_b{};
    compat::u32 modal_source{};
    compat::u32 resource_dialog_cursor_state{1U};
    compat::u32 debug_action_gate{};
    compat::u32 debug_runtime_flags{};
};

struct LegacyWorldDebugHotkeyRequest {
    compat::u32 developer_tools_enabled{};
    compat::u16 talk_source_guid{0xFFFFU};
    bool dialog_messages_active{};
    compat::u32 debug_action_argument{};
};

struct LegacyWorldDebugModeRequest {
    compat::u32 modal_state{};
    compat::u32 option_a{};
    compat::u32 option_b{};
    compat::u32 source{};
};

class LegacyWorldDebugHotkeyPorts {
public:
    virtual ~LegacyWorldDebugHotkeyPorts() = default;

    [[nodiscard]] virtual compat::u32
    query_internal_flag(compat::u32 bit_index) = 0;
    virtual void set_internal_flag(compat::u32 bit_index, bool value) = 0;
    virtual void delay_milliseconds(compat::u32 milliseconds) = 0;
    virtual void
    request_debug_mode(const LegacyWorldDebugModeRequest& request) = 0;
    virtual void run_debug_action(compat::u32 argument) = 0;
    virtual void show_cursor() = 0;
    [[nodiscard]] virtual compat::i32 show_resource_dialog() = 0;
    [[nodiscard]] virtual bool
    load_item_category(compat::u32 item_id, compat::u16& category) = 0;
    virtual void add_item(compat::u32 item_id, compat::u32 count) = 0;
    virtual void release_item_definition(compat::u32 item_id) = 0;
};

enum class LegacyWorldDebugHotkeyStatus : compat::u8 {
    completed,
    missing_raw_key_state,
};

enum class LegacyWorldDebugHotkeyOutcome : compat::u8 {
    continue_normal_control,
    return_from_player_control,
};

struct LegacyWorldDebugHotkeyResult {
    LegacyWorldDebugHotkeyStatus status{
        LegacyWorldDebugHotkeyStatus::completed
    };
    LegacyWorldDebugHotkeyOutcome outcome{
        LegacyWorldDebugHotkeyOutcome::continue_normal_control
    };
    compat::u32 raw_key_queries{};
    compat::u32 delay_calls{};
    compat::u32 item_definitions_loaded{};
    compat::u32 items_added{};
    compat::u32 item_definitions_released{};
    bool enable_chord_consumed{};
};

// sub_402F80 0x00402FAF..0x004034BD: hidden developer-key block. The
// normalized player controls are not touched here; all key reads use the live
// 256-byte raw snapshot and every returning branch retains its blocking delay.
[[nodiscard]] LegacyWorldDebugHotkeyResult
coordinate_legacy_world_debug_hotkeys(
    const LegacyWorldDebugHotkeyRequest& request,
    std::span<const compat::u8> raw_key_state,
    LegacyWorldDebugHotkeyState& state,
    LegacyWorldDebugHotkeyPorts& ports
);

}  // namespace openswd3::world_map
