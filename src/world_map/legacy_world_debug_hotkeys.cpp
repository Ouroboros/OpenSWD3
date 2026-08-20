#include "openswd3/world_map/legacy_world_debug_hotkeys.hpp"

#include <array>
#include <bit>

namespace openswd3::world_map {
namespace {

using compat::u16;
using compat::u32;
using compat::u8;

constexpr std::size_t kRequiredRawKeyCount = 0xD0U;
constexpr u32 kDebugReturnDelay = 500U;
constexpr u32 kDebugToggleDelay = 300U;

[[nodiscard]] u32 signed_remainder_two(const u32 value) noexcept {
    u32 result = value & 0x80000001U;
    if (std::bit_cast<compat::i32>(result) < 0) {
        --result;
        result |= 0xFFFFFFFEU;
        ++result;
    }
    return result;
}

[[nodiscard]] bool granted_item_category(const u16 category) noexcept {
    switch (category) {
    case 0U:
    case 1U:
    case 2U:
    case 3U:
    case 4U:
    case 5U:
    case 6U:
    case 7U:
    case 8U:
    case 9U:
    case 0x0FU:
    case 0x10U:
    case 0x11U:
    case 0x12U:
    case 0x13U:
    case 0x14U:
    case 0x15U:
    case 0x18U:
        return true;
    default:
        return false;
    }
}

}  // namespace

LegacyWorldDebugHotkeyResult coordinate_legacy_world_debug_hotkeys(
    const LegacyWorldDebugHotkeyRequest& request,
    const std::span<const u8> raw_key_state,
    LegacyWorldDebugHotkeyState& state,
    LegacyWorldDebugHotkeyPorts& ports
) {
    LegacyWorldDebugHotkeyResult result;
    const auto pressed = [&](const std::size_t scan_code) {
        ++result.raw_key_queries;
        return raw_key_state[scan_code] != 0U;
    };
    const auto delay = [&](const u32 milliseconds) {
        ports.delay_milliseconds(milliseconds);
        ++result.delay_calls;
    };
    const auto request_mode =
        [&](const u32 option_a, const u32 option_b, const u32 source) {
            state.modal_state = 3U;
            state.modal_option_a = option_a;
            state.modal_option_b = option_b;
            state.modal_source = source;
            ports.request_debug_mode(
                {state.modal_state, option_a, option_b, source}
            );
        };
    const auto return_now = [&]() {
        result.outcome =
            LegacyWorldDebugHotkeyOutcome::return_from_player_control;
        return result;
    };

    if (request.developer_tools_enabled == 1U) {
        if (raw_key_state.size() < kRequiredRawKeyCount) {
            result.status = LegacyWorldDebugHotkeyStatus::missing_raw_key_state;
            return result;
        }
        if (pressed(0x9DU) && pressed(0x36U) && pressed(0xC7U) &&
            pressed(0xCFU)) {
            ports.set_internal_flag(kLegacyWorldDebugEnableFlag, true);
            result.enable_chord_consumed = true;
        }
    }
    if (ports.query_internal_flag(kLegacyWorldDebugEnableFlag) == 0U) {
        return result;
    }
    if (raw_key_state.size() <= 0x9DU) {
        result.status = LegacyWorldDebugHotkeyStatus::missing_raw_key_state;
        return result;
    }

    if (pressed(0x58U) && request.talk_source_guid == 0xFFFFU &&
        !request.dialog_messages_active) {
        delay(kDebugReturnDelay);
        request_mode(0U, 0U, 1U);
        return return_now();
    }

    if (pressed(0x1DU) || pressed(0x9DU)) {
        if (pressed(0x43U)) {
            state.fixed_debug_speed =
                signed_remainder_two(state.fixed_debug_speed + 1U);
            delay(350U);
            return return_now();
        }
    }

    if (pressed(0x10U)) {
        state.debug_action_gate = 1U;
        ports.run_debug_action(request.debug_action_argument);
        state.debug_runtime_flags |= 4U;
        ports.set_internal_flag(1U, false);
        return return_now();
    }

    if (pressed(0x1FU) && request.talk_source_guid == 0xFFFFU &&
        !request.dialog_messages_active) {
        state.modal_option_a = 1U;
        state.modal_option_b = 0U;
        delay(kDebugReturnDelay);
        request_mode(1U, 0U, 1U);
        return return_now();
    }

    if (pressed(0x26U) && request.talk_source_guid == 0xFFFFU &&
        !request.dialog_messages_active) {
        state.modal_option_a = 1U;
        state.modal_option_b = 1U;
        delay(kDebugReturnDelay);
        request_mode(1U, 1U, 2U);
        return return_now();
    }

    if (pressed(0x3BU)) {
        state.diagnostic_text_visible =
            state.diagnostic_text_visible == 0U ? 1U : 0U;
        state.world_frame_count = 0U;
        delay(kDebugToggleDelay);
        return return_now();
    }
    if (pressed(0x3CU)) {
        state.collision_grid_visible =
            state.collision_grid_visible == 0U ? 1U : 0U;
        state.world_frame_count = 0U;
        delay(kDebugToggleDelay);
        return return_now();
    }
    if (pressed(0x3DU)) {
        const bool enable =
            ports.query_internal_flag(kLegacyWorldDebugCollisionFlag) == 0U;
        ports.set_internal_flag(kLegacyWorldDebugCollisionFlag, enable);
        state.world_frame_count = 0U;
        delay(kDebugToggleDelay);
        return return_now();
    }

    if (pressed(0x0FU) && pressed(0x4EU)) {
        ++state.tile_animation_interval;
        if (std::bit_cast<compat::i32>(state.tile_animation_interval) > 64) {
            state.tile_animation_interval = 64U;
        }
        state.world_frame_count = 0U;
    }
    if (pressed(0x0FU) && pressed(0x4AU)) {
        --state.tile_animation_interval;
        if (std::bit_cast<compat::i32>(state.tile_animation_interval) < 1) {
            state.tile_animation_interval = 1U;
        }
        state.world_frame_count = 0U;
    }

    if (pressed(0x3EU)) {
        state.resource_dialog_cursor_state = 0U;
        ports.show_cursor();
        if (ports.show_resource_dialog() == 0) {
            state.resource_dialog_cursor_state = 1U;
        }
        return return_now();
    }

    if (pressed(0x41U)) {
        state.money += 1000U;
        delay(250U);
    }

    if (pressed(0x40U)) {
        for (u32 item_id = 0x65U; item_id < 0x4B0U; ++item_id) {
            u16 category{};
            if (!ports.load_item_category(item_id, category)) {
                continue;
            }
            ++result.item_definitions_loaded;
            if (granted_item_category(category)) {
                ports.add_item(item_id, 10U);
                ++result.items_added;
            }
            ports.release_item_definition(item_id);
            ++result.item_definitions_released;
        }
        delay(kDebugReturnDelay);
    }

    return result;
}

}  // namespace openswd3::world_map
