#include "test.hpp"

#include "openswd3/world_map/legacy_world_interaction.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacyInputRecord;
using openswd3::world_map::coordinate_legacy_world_interaction;
using openswd3::world_map::count_legacy_world_choice_hotspots;
using openswd3::world_map::find_legacy_world_choice_hotspot;
using openswd3::world_map::kLegacyWorldDefaultCursorVariant;
using openswd3::world_map::kLegacyWorldMapEventCursorVariant;
using openswd3::world_map::kLegacyWorldTalkIdleSource;
using openswd3::world_map::kLegacyWorldTalkMapEventSource;
using openswd3::world_map::LegacyWorldInteractionHotspot;
using openswd3::world_map::LegacyWorldInteractionPorts;
using openswd3::world_map::LegacyWorldInteractionRequest;
using openswd3::world_map::LegacyWorldInteractionSource;
using openswd3::world_map::LegacyWorldInteractionState;
using openswd3::world_map::LegacyWorldInteractionStatus;
using openswd3::world_map::LegacyWorldMapEvent;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldTalkContext;

class RecordingPorts final : public LegacyWorldInteractionPorts {
public:
    u32 query_internal_flag(const u32 bit_index) override {
        flag_queries.push_back(bit_index);
        return bit_index == 9U ? flag_9 : map_flag;
    }

    bool load_role_frame_size(
        const u16 resource_id, const u16 frame_index, u16& width, u16& height
    ) override {
        frame_requests.push_back(
            (static_cast<u32>(resource_id) << 16U) | frame_index
        );
        width = frame_width;
        height = frame_height;
        return frame_available;
    }

    u32 update_action(LegacyActionRecord& action) override {
        action_updates.push_back(&action);
        return action_update_result;
    }

    std::vector<u32> flag_queries;
    std::vector<u32> frame_requests;
    std::vector<LegacyActionRecord*> action_updates;
    u32 flag_9{};
    u32 map_flag{};
    u32 action_update_result{1U};
    u16 frame_width{20U};
    u16 frame_height{20U};
    bool frame_available{true};
};

LegacyWorldRoleRecord make_player() {
    LegacyWorldRoleRecord player{};
    player.world_x = 32U;
    player.world_y = 32U;
    player.action.base_variant = 0U;
    return player;
}

LegacyWorldTalkContext idle_talk() {
    LegacyWorldTalkContext talk{};
    talk.source_guid = kLegacyWorldTalkIdleSource;
    return talk;
}

std::array<LegacyInputRecord, 20U> first_left_click() {
    std::array<LegacyInputRecord, 20U> records{};
    records[15U].rapid_press_multiplicity = 1U;
    records[15U].release_milliseconds = 2U;
    records[15U].rapid_press_stage = 3U;
    records[15U].held_sample_count = 1U;
    return records;
}

void test_role_hover_and_activation(openswd3::test::Context& test) {
    std::vector roles{make_player(), LegacyWorldRoleRecord{}};
    auto& target = roles[1];
    target.world_x = 100U;
    target.world_y = 50U;
    target.flags = 0x00008800U;
    target.talk_data_offset = 0x11223344U;
    target.talk_script_id = 0x1234U;
    target.talk_initial_offset = 0x42U;
    target.guid = 0x77U;
    target.action.field_4a = 0x12U;
    target.action.field_4c = 0x34U;
    target.action.base_variant = 9U;
    target.action.variant_delta = 6U;
    target.action.wait_remaining = 5U;

    auto inputs = first_left_click();
    auto talk = idle_talk();
    talk.field_18 = 0xCAFEBABEU;
    LegacyWorldInteractionState state;
    RecordingPorts ports;

    const auto result = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .mouse_x = 110U,
            .mouse_y = 60U,
            .map_width = 8U,
            .camera = {},
            .choice_hotspots = {},
        },
        roles,
        {},
        std::array<openswd3::compat::u8, 4U>{},
        inputs,
        talk,
        state,
        ports
    );

    test.expect_equal(
        result.status,
        LegacyWorldInteractionStatus::completed,
        "role interaction completes"
    );
    test.expect_equal(
        result.source,
        LegacyWorldInteractionSource::role,
        "first-sample left click activates hovered role"
    );
    test.expect_equal(
        result.hovered_role_index, 1U, "scan starts at role index one"
    );
    test.expect_equal(
        ports.frame_requests,
        std::vector<u32>{0x00120034U},
        "hover resolves current TSW resource and frame"
    );
    test.expect_equal(
        ports.flag_queries,
        std::vector<u32>{9U},
        "choice suppression flag remains in original slot"
    );
    test.expect_equal(
        ports.action_updates.size(),
        std::size_t{2U},
        "turning target and player each refresh once"
    );
    test.expect_true(
        ports.action_updates[0] == &roles[1].action,
        "first refresh owns the hovered target action"
    );
    test.expect_true(
        ports.action_updates[1] == &roles[0].action,
        "second refresh owns the player action, not target again"
    );
    test.expect_equal(
        roles[1].action.one_shot_base_variant,
        9U,
        "target saves prior base variant"
    );
    test.expect_equal(
        roles[1].action.one_shot_variant_delta, 6U, "target saves prior facing"
    );
    test.expect_equal(
        roles[1].action.base_variant, 0U, "target enters base action zero"
    );
    test.expect_equal(
        roles[1].action.variant_delta, 2U, "target faces the player"
    );
    test.expect_equal(
        roles[0].action.variant_delta,
        3U,
        "player uses exact opposite-direction fold"
    );
    test.expect_equal(
        talk.talk_data_offset, 0x11223344U, "role Talk copies data offset"
    );
    test.expect_equal(
        talk.instruction_offset,
        u16{0x42U},
        "role Talk copies initial instruction offset"
    );
    test.expect_equal(
        talk.talk_script_id, u16{0x1234U}, "role Talk copies script id"
    );
    test.expect_equal(talk.source_guid, u16{0x77U}, "role Talk copies GUID");
    test.expect_equal(talk.world_x, 100U, "role Talk copies world X");
    test.expect_equal(talk.world_y, 50U, "role Talk copies world Y");
    test.expect_equal(
        talk.field_18, 0xCAFEBABEU, "unwritten Talk fields retain their bytes"
    );
    test.expect_equal(
        inputs[15U],
        LegacyInputRecord{},
        "role activation clears all four left-input dwords"
    );
}

void test_choice_chain_has_absolute_priority(openswd3::test::Context& test) {
    std::vector roles{make_player(), LegacyWorldRoleRecord{}};
    roles[1].world_x = 100U;
    roles[1].world_y = 50U;
    roles[1].flags = 0x8000U;
    roles[1].talk_script_id = 5U;
    const std::array hotspots{
        LegacyWorldInteractionHotspot{10U, 10U, 30U, 30U},
        LegacyWorldInteractionHotspot{40U, 10U, 60U, 30U},
    };
    auto inputs = first_left_click();
    auto talk = idle_talk();
    LegacyWorldInteractionState state;
    RecordingPorts ports;

    const auto result = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .mouse_x = 50U,
            .mouse_y = 20U,
            .map_width = 8U,
            .camera = {},
            .choice_hotspots = hotspots,
        },
        roles,
        {},
        std::array<openswd3::compat::u8, 4U>{},
        inputs,
        talk,
        state,
        ports
    );

    test.expect_equal(
        result.source,
        LegacyWorldInteractionSource::choice,
        "choice click returns before role/map activation"
    );
    test.expect_equal(
        state.selected_choice_index, 1U, "choice index is zero based"
    );
    test.expect_equal(
        state.cursor_variant,
        kLegacyWorldDefaultCursorVariant,
        "live choice chain forces cursor variant thirteen"
    );
    test.expect_true(
        result.choice_chain_clear_requested,
        "accepted choice requests whole hotspot-chain release"
    );
    test.expect_equal(
        talk.source_guid,
        kLegacyWorldTalkIdleSource,
        "choice click does not create Talk context"
    );
    test.expect_equal(
        inputs[15U],
        LegacyInputRecord{},
        "choice click clears the left input record"
    );

    inputs = first_left_click();
    talk = idle_talk();
    state.cursor_variant = 9U;
    const auto border = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .mouse_x = 40U,
            .mouse_y = 20U,
            .map_width = 8U,
            .camera = {},
            .choice_hotspots = hotspots,
        },
        roles,
        {},
        std::array<openswd3::compat::u8, 4U>{},
        inputs,
        talk,
        state,
        ports
    );
    test.expect_equal(
        border.source,
        LegacyWorldInteractionSource::none,
        "hotspot left edge is excluded by strict comparison"
    );
    test.expect_equal(
        inputs[15U].rapid_press_multiplicity,
        1U,
        "missed choice leaves the input record intact"
    );
    test.expect_equal(
        state.cursor_variant,
        kLegacyWorldDefaultCursorVariant,
        "choice-chain miss still returns with variant thirteen"
    );
}

void test_choice_hotspot_chain_helpers(openswd3::test::Context& test) {
    const std::array hotspots{
        LegacyWorldInteractionHotspot{10U, 20U, 30U, 40U},
        LegacyWorldInteractionHotspot{50U, 60U, 70U, 80U},
    };
    test.expect_equal(
        count_legacy_world_choice_hotspots(hotspots),
        2U,
        "sub_40DB40 returns the ordered hotspot-node count"
    );
    test.expect_equal(
        count_legacy_world_choice_hotspots({}),
        0U,
        "sub_40DB40 returns zero for a null legacy chain"
    );

    const auto second = find_legacy_world_choice_hotspot(hotspots, 60U, 70U);
    test.expect_true(
        second.index == 1U && second.hotspot == &hotspots[1],
        "sub_40DB60 returns the first hit node and zero-based index"
    );

    constexpr std::array border_points{
        std::array<u32, 2U>{10U, 30U},
        std::array<u32, 2U>{30U, 30U},
        std::array<u32, 2U>{20U, 20U},
        std::array<u32, 2U>{20U, 40U},
    };
    for (const auto& point : border_points) {
        const auto border = find_legacy_world_choice_hotspot(
            std::span{hotspots}.first(1U), point[0], point[1]
        );
        test.expect_true(
            border.index == 1U && border.hotspot == nullptr,
            "sub_40DB60 excludes every rectangle border"
        );
    }

    const auto miss = find_legacy_world_choice_hotspot(hotspots, 90U, 90U);
    test.expect_true(
        miss.index == 2U && miss.hotspot == nullptr,
        "sub_40DB60 miss returns terminal count and null node"
    );
}

void test_map_event_activation(openswd3::test::Context& test) {
    std::vector roles{make_player()};
    roles[0].action.base_variant = 0x34U;
    roles[0].action.variant_delta = 6U;
    const std::vector events{
        LegacyWorldMapEvent{
            .field_04 = 7U,
            .field_08 = 0xF123U,
            .field_0c = 0xCAFE0022U,
            .name_bytes_with_terminator = {},
        },
    };
    std::array<openswd3::compat::u8, 8U * 8U * 4U> surface{};
    surface[(2U * 8U + 3U) * 4U] = 7U;
    auto inputs = first_left_click();
    auto talk = idle_talk();
    LegacyWorldInteractionState state;
    RecordingPorts ports;
    ports.map_flag = 2U;

    const auto result = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .mouse_x = 48U,
            .mouse_y = 32U,
            .map_width = 8U,
            .camera = {},
            .choice_hotspots = {},
        },
        roles,
        events,
        surface,
        inputs,
        talk,
        state,
        ports
    );

    test.expect_equal(
        result.source,
        LegacyWorldInteractionSource::map_event,
        "enabled cell event creates a map Talk source"
    );
    test.expect_equal(
        ports.flag_queries,
        std::vector<u32>({9U, 0x22U}),
        "map event queries the low word of field 0C"
    );
    test.expect_equal(
        state.cursor_variant,
        kLegacyWorldMapEventCursorVariant,
        "enabled event selects cursor variant eleven"
    );
    test.expect_equal(
        state.global_lock,
        0x8000U,
        "map event assigns, rather than ORs, global lock 8000"
    );
    test.expect_equal(
        talk.talk_data_offset, 0U, "map event clears Talk data offset"
    );
    test.expect_equal(
        talk.instruction_offset, u16{0U}, "map event clears instruction offset"
    );
    test.expect_equal(
        talk.talk_script_id, u16{0x7123U}, "map event masks script id with 7FFF"
    );
    test.expect_equal(
        talk.source_guid,
        kLegacyWorldTalkMapEventSource,
        "map event writes FFFD source sentinel"
    );
    test.expect_equal(
        talk.world_x,
        48U,
        "map Talk X aligns screen coordinate before camera add"
    );
    test.expect_equal(
        talk.world_y,
        32U,
        "map Talk Y aligns screen coordinate before camera add"
    );
    test.expect_equal(
        roles[0].action.one_shot_base_variant,
        0x34U,
        "map click saves player base variant"
    );
    test.expect_equal(
        roles[0].action.one_shot_variant_delta,
        6U,
        "map click saves player facing"
    );
    test.expect_equal(
        roles[0].action.variant_delta,
        3U,
        "player faces the clicked point through opposite fold"
    );
    test.expect_equal(
        inputs[15U].rapid_press_multiplicity,
        1U,
        "map activation intentionally does not clear left input"
    );
}

void test_direction_synthesis_and_delayed_primary_copy(
    openswd3::test::Context& test
) {
    std::vector roles{make_player()};
    std::array<openswd3::compat::u8, 8U * 8U * 4U> surface{};
    std::array<LegacyInputRecord, 20U> inputs{};
    inputs[14U] = LegacyInputRecord{2U, 3U, 2U, 1U};
    inputs[15U] = LegacyInputRecord{3U, 4U, 3U, 1U};
    auto talk = idle_talk();
    LegacyWorldInteractionState state;
    state.cursor_variant = kLegacyWorldDefaultCursorVariant;
    RecordingPorts ports;

    const auto result = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .mouse_x = 32U,
            .mouse_y = 0U,
            .map_width = 8U,
            .camera = {},
            .choice_hotspots = {},
            .dialog_chain_active = true,
        },
        roles,
        {},
        surface,
        inputs,
        talk,
        state,
        ports
    );

    test.expect_equal(
        result.distance, 32U, "mouse-to-player center distance is retained"
    );
    test.expect_equal(
        result.facing, 0U, "mouse above player quantizes to direction zero"
    );
    test.expect_equal(
        state.cursor_variant,
        0U,
        "variant thirteen is replaced by movement direction"
    );
    test.expect_equal(
        inputs[4U].rapid_press_multiplicity,
        2U,
        "right mouse multiplicity synthesizes up input"
    );
    test.expect_equal(
        inputs[3U].rapid_press_multiplicity,
        0U,
        "cardinal direction does not synthesize left input"
    );
    test.expect_equal(
        inputs[1U], inputs[15U], "live dialog copies all four left-input dwords"
    );
    test.expect_true(
        result.delayed_primary_input_copied,
        "delayed primary bridge is reported"
    );
}

void test_safe_span_boundaries(openswd3::test::Context& test) {
    std::vector roles{make_player()};
    auto talk = idle_talk();
    LegacyWorldInteractionState state;
    RecordingPorts ports;
    std::array<LegacyInputRecord, 20U> inputs{};

    const auto short_grid = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .mouse_x = 48U,
            .mouse_y = 32U,
            .map_width = 8U,
            .camera = {},
            .choice_hotspots = {},
        },
        roles,
        {},
        std::array<openswd3::compat::u8, 4U>{},
        inputs,
        talk,
        state,
        ports
    );
    test.expect_equal(
        short_grid.status,
        LegacyWorldInteractionStatus::invalid_surface_grid,
        "modern span stops before original unchecked cell read"
    );

    const auto short_inputs = coordinate_legacy_world_interaction(
        LegacyWorldInteractionRequest{
            .player_index = 0U,
            .camera = {},
            .choice_hotspots = {},
        },
        roles,
        {},
        {},
        std::span<LegacyInputRecord>{inputs}.first(15U),
        talk,
        state,
        ports
    );
    test.expect_equal(
        short_inputs.status,
        LegacyWorldInteractionStatus::missing_input_records,
        "all referenced normalized records are required"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_role_hover_and_activation(test);
    test_choice_chain_has_absolute_priority(test);
    test_choice_hotspot_chain_helpers(test);
    test_map_event_activation(test);
    test_direction_synthesis_and_delayed_primary_copy(test);
    test_safe_span_boundaries(test);
    return test.exit_code();
}
