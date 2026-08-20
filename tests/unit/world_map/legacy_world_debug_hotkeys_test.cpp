#include "test.hpp"

#include "openswd3/world_map/legacy_world_debug_hotkeys.hpp"

#include <array>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::coordinate_legacy_world_debug_hotkeys;
using openswd3::world_map::kLegacyWorldDebugCollisionFlag;
using openswd3::world_map::kLegacyWorldDebugEnableFlag;
using openswd3::world_map::LegacyWorldDebugHotkeyOutcome;
using openswd3::world_map::LegacyWorldDebugHotkeyPorts;
using openswd3::world_map::LegacyWorldDebugHotkeyRequest;
using openswd3::world_map::LegacyWorldDebugHotkeyState;
using openswd3::world_map::LegacyWorldDebugHotkeyStatus;
using openswd3::world_map::LegacyWorldDebugModeRequest;

class RecordingPorts final : public LegacyWorldDebugHotkeyPorts {
public:
    u32 query_internal_flag(const u32 bit_index) override {
        queried_flags.push_back(bit_index);
        return flags[bit_index] ? 1U : 0U;
    }
    void set_internal_flag(const u32 bit_index, const bool value) override {
        set_flags.emplace_back(bit_index, value);
        flags[bit_index] = value;
    }
    void delay_milliseconds(const u32 milliseconds) override {
        delays.push_back(milliseconds);
    }
    void
    request_debug_mode(const LegacyWorldDebugModeRequest& request) override {
        modes.push_back(request);
    }
    void run_debug_action(const u32 argument) override {
        debug_actions.push_back(argument);
    }
    void show_cursor() override {
        ++show_cursor_calls;
    }
    i32 show_resource_dialog() override {
        ++resource_dialog_calls;
        return resource_dialog_result;
    }
    bool load_item_category(const u32 item_id, u16& category) override {
        loaded_items.push_back(item_id);
        if (fail_odd_item_loads && (item_id & 1U) != 0U) {
            return false;
        }
        category = (item_id & 1U) == 0U ? 0U : 0x99U;
        return true;
    }
    void add_item(const u32 item_id, const u32 count) override {
        added_items.emplace_back(item_id, count);
    }
    void release_item_definition(const u32 item_id) override {
        released_items.push_back(item_id);
    }

    std::array<bool, 0x100U> flags{};
    std::vector<u32> queried_flags;
    std::vector<std::pair<u32, bool>> set_flags;
    std::vector<u32> delays;
    std::vector<LegacyWorldDebugModeRequest> modes;
    std::vector<u32> debug_actions;
    std::vector<u32> loaded_items;
    std::vector<std::pair<u32, u32>> added_items;
    std::vector<u32> released_items;
    u32 show_cursor_calls{};
    u32 resource_dialog_calls{};
    i32 resource_dialog_result{};
    bool fail_odd_item_loads{};
};

std::array<u8, 0x100U> keys(std::initializer_list<std::size_t> pressed) {
    std::array<u8, 0x100U> result{};
    for (const std::size_t scan_code : pressed) {
        result[scan_code] = 0x80U;
    }
    return result;
}

void test_enable_chord_and_mode_priority(openswd3::test::Context& test) {
    auto raw = keys({0x9DU, 0x36U, 0xC7U, 0xCFU, 0x58U, 0x43U});
    LegacyWorldDebugHotkeyState state;
    RecordingPorts ports;
    const auto result = coordinate_legacy_world_debug_hotkeys(
        LegacyWorldDebugHotkeyRequest{.developer_tools_enabled = 1U},
        raw,
        state,
        ports
    );
    test.expect_true(
        result.status == LegacyWorldDebugHotkeyStatus::completed &&
            result.outcome ==
                LegacyWorldDebugHotkeyOutcome::return_from_player_control &&
            result.enable_chord_consumed &&
            ports.flags[kLegacyWorldDebugEnableFlag] &&
            ports.delays == std::vector<u32>{500U} &&
            ports.modes.size() == 1U && ports.modes[0].modal_state == 3U &&
            ports.modes[0].source == 1U && state.fixed_debug_speed == 0U,
        "enable chord publishes flag 52 and X-mode returns before Ctrl-C"
    );
}

void test_fixed_speed_and_debug_action(openswd3::test::Context& test) {
    LegacyWorldDebugHotkeyState state;
    RecordingPorts ports;
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    auto raw = keys({0x1DU, 0x43U, 0x10U});
    const auto speed =
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports);
    test.expect_true(
        speed.outcome ==
                LegacyWorldDebugHotkeyOutcome::return_from_player_control &&
            state.fixed_debug_speed == 1U &&
            ports.delays == std::vector<u32>{350U} &&
            ports.debug_actions.empty(),
        "Ctrl-C signed remainder toggle returns before the Q branch"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    raw = keys({0x10U});
    const auto action = coordinate_legacy_world_debug_hotkeys(
        LegacyWorldDebugHotkeyRequest{.debug_action_argument = 0x1234U},
        raw,
        state,
        ports
    );
    test.expect_true(
        action.outcome ==
                LegacyWorldDebugHotkeyOutcome::return_from_player_control &&
            state.debug_action_gate == 1U &&
            (state.debug_runtime_flags & 4U) != 0U &&
            ports.debug_actions == std::vector<u32>{0x1234U} &&
            !ports.flags[1U],
        "Q action writes both globals and clears internal bit one"
    );
}

void test_modal_and_overlay_branches(openswd3::test::Context& test) {
    LegacyWorldDebugHotkeyState state;
    RecordingPorts ports;
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    auto raw = keys({0x1FU});
    const auto first_mode =
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports);
    test.expect_true(
        first_mode.outcome ==
                LegacyWorldDebugHotkeyOutcome::return_from_player_control &&
            state.modal_option_a == 1U && state.modal_option_b == 0U &&
            state.modal_source == 1U && ports.delays == std::vector<u32>{500U},
        "first modal key retains option/source tuple"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    raw = keys({0x26U});
    static_cast<void>(
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports)
    );
    test.expect_true(
        state.modal_option_a == 1U && state.modal_option_b == 1U &&
            state.modal_source == 2U,
        "second modal key retains its distinct option/source tuple"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    raw = keys({0x3BU});
    static_cast<void>(
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports)
    );
    test.expect_true(
        state.diagnostic_text_visible == 1U && state.world_frame_count == 0U &&
            ports.delays == std::vector<u32>{300U},
        "F1 toggles diagnostic text and clears the frame counter"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    raw = keys({0x3CU});
    static_cast<void>(
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports)
    );
    test.expect_equal(
        state.collision_grid_visible,
        u32{1U},
        "F2 independently toggles the collision grid"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    raw = keys({0x3DU});
    static_cast<void>(
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports)
    );
    test.expect_true(
        ports.flags[kLegacyWorldDebugCollisionFlag] &&
            ports.queried_flags ==
                std::vector<u32>(
                    {kLegacyWorldDebugEnableFlag,
                     kLegacyWorldDebugCollisionFlag}
                ),
        "F3 queries then toggles exact internal bit 14h"
    );
}

void test_interval_and_resource_dialog(openswd3::test::Context& test) {
    LegacyWorldDebugHotkeyState state;
    state.tile_animation_interval = 64U;
    RecordingPorts ports;
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    auto raw = keys({0x0FU, 0x4EU});
    const auto upper =
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports);
    test.expect_true(
        upper.outcome ==
                LegacyWorldDebugHotkeyOutcome::continue_normal_control &&
            state.tile_animation_interval == 64U &&
            state.world_frame_count == 0U,
        "Tab-plus increments and clamps the interval at 64 without returning"
    );

    state.tile_animation_interval = 1U;
    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    raw = keys({0x0FU, 0x4AU});
    static_cast<void>(
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports)
    );
    test.expect_equal(
        state.tile_animation_interval,
        u32{1U},
        "Tab-minus decrements and clamps the interval at one"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    ports.resource_dialog_result = 0;
    raw = keys({0x3EU});
    const auto dialog =
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports);
    test.expect_true(
        dialog.outcome ==
                LegacyWorldDebugHotkeyOutcome::return_from_player_control &&
            ports.show_cursor_calls == 1U &&
            ports.resource_dialog_calls == 1U &&
            state.resource_dialog_cursor_state == 1U,
        "resource dialog zero result restores the original cursor-state dword"
    );
}

void test_money_and_item_grant_loop(openswd3::test::Context& test) {
    LegacyWorldDebugHotkeyState state;
    state.money = 0xFFFFFC18U;
    RecordingPorts ports;
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    auto raw = keys({0x41U, 0x40U});
    const auto result =
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports);
    test.expect_true(
        result.outcome ==
                LegacyWorldDebugHotkeyOutcome::continue_normal_control &&
            state.money == 0U &&
            ports.delays == std::vector<u32>({250U, 500U}) &&
            result.item_definitions_loaded == 1099U &&
            result.items_added == 549U &&
            result.item_definitions_released == 1099U &&
            ports.loaded_items.front() == 0x65U &&
            ports.loaded_items.back() == 0x4AFU &&
            ports.added_items.front() == std::pair<u32, u32>{0x66U, 10U},
        "money wraps and the complete 65h..4AFh item loop filters categories"
    );

    state.money = 0U;
    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    ports.fail_odd_item_loads = true;
    raw = keys({0x40U});
    const auto partial =
        coordinate_legacy_world_debug_hotkeys({}, raw, state, ports);
    test.expect_true(
        partial.item_definitions_loaded == 549U &&
            partial.items_added == 549U &&
            partial.item_definitions_released == 549U,
        "failed item definitions skip both add and cleanup at the original gate"
    );
}

void test_short_snapshot(openswd3::test::Context& test) {
    LegacyWorldDebugHotkeyState state;
    RecordingPorts ports;
    std::array<u8, 1U> disabled_state{};
    const auto disabled =
        coordinate_legacy_world_debug_hotkeys({}, disabled_state, state, ports);
    test.expect_true(
        disabled.status == LegacyWorldDebugHotkeyStatus::completed &&
            ports.queried_flags ==
                std::vector<u32>{kLegacyWorldDebugEnableFlag},
        "disabled developer flag returns without reading the raw snapshot"
    );

    ports = {};
    ports.flags[kLegacyWorldDebugEnableFlag] = true;
    std::array<u8, 0x58U> short_state{};
    const auto enabled =
        coordinate_legacy_world_debug_hotkeys({}, short_state, state, ports);
    test.expect_true(
        enabled.status == LegacyWorldDebugHotkeyStatus::missing_raw_key_state &&
            ports.queried_flags ==
                std::vector<u32>{kLegacyWorldDebugEnableFlag},
        "enabled path queries flag 52 before the first unavailable raw key"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_enable_chord_and_mode_priority(test);
    test_fixed_speed_and_debug_action(test);
    test_modal_and_overlay_branches(test);
    test_interval_and_resource_dialog(test);
    test_money_and_item_grant_loop(test);
    test_short_snapshot(test);
    return test.exit_code();
}
