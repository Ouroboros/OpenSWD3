#include "test.hpp"

#include "openswd3/world_map/legacy_picture_actions.hpp"

#include <bit>
#include <functional>
#include <list>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::world_map::LegacyPictureActionAudioPorts;
using openswd3::world_map::LegacyPictureActionLists;
using openswd3::world_map::LegacyPictureActionNode;
using openswd3::world_map::LegacyPictureActionStatus;
using openswd3::world_map::release_legacy_picture_actions;
using openswd3::world_map::update_draw_legacy_picture_actions;

struct DrawCall {
    i32 x{};
    i32 y{};
    u32 flags{};
    i32 opacity{};
};

class RecordingActionPorts final : public LegacyActionDrawPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        updated_ids.push_back(record.action_id);
        if (update_callback) {
            update_callback();
        }
        if (record.action_id == 2U) {
            return LegacyActionUpdateStatus::malformed_stream;
        }
        return LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        loads.emplace_back(resource_id, frame_index);
        piece.width = 16U;
        piece.height = 20U;
        if (load_callback) {
            load_callback();
        }
        return resource_id != 33U;
    }

    [[nodiscard]] LegacyBlitExecutionStatus draw_frame_piece(
        const LegacyFramePiece&,
        const i32 destination_x,
        const i32 destination_y,
        const u32 flags,
        const i32 opacity_step
    ) noexcept override {
        draws.push_back(
            DrawCall{destination_x, destination_y, flags, opacity_step}
        );
        if (draw_callback) {
            draw_callback();
        }
        return draws.size() == 2U ? LegacyBlitExecutionStatus::malformed_source
                                  : LegacyBlitExecutionStatus::completed;
    }

    std::function<void()> update_callback;
    std::function<void()> load_callback;
    std::function<void()> draw_callback;
    std::vector<u32> updated_ids;
    std::vector<std::pair<u16, u16>> loads;
    std::vector<DrawCall> draws;
};

class RecordingAudioPorts final : public LegacyPictureActionAudioPorts {
public:
    void play_positional_sample(
        const u16 sound_id, const i32 world_x, const i32 world_y
    ) noexcept override {
        samples.emplace_back(sound_id, world_x, world_y);
        if (play_callback) {
            play_callback();
        }
    }

    std::function<void()> play_callback;
    std::vector<std::tuple<u16, i32, i32>> samples;
};

[[nodiscard]] LegacyPictureActionNode make_node(
    const u32 id,
    const u16 x,
    const u16 y,
    const u16 resource_id,
    const u16 frame_index,
    const u16 sound_id,
    const u32 completion
) {
    LegacyPictureActionNode node{};
    node.screen_x = x;
    node.screen_y = y;
    node.action.action_id = id;
    node.action.draw_offset_x = id;
    node.action.draw_offset_y = id + 1U;
    node.action.mode_flags = 0x100U + id;
    node.action.field_4a = resource_id;
    node.action.field_4c = frame_index;
    node.action.field_58 = sound_id;
    node.action.field_8a = static_cast<openswd3::compat::u8>(0x20U + id);
    node.action.field_8c = completion;
    return node;
}

void test_exact_update_draw_audio_and_removal(openswd3::test::Context& test) {
    std::list<LegacyPictureActionNode> nodes;
    nodes.push_back(make_node(1U, 10U, 20U, 11U, 12U, 5U, 0U));
    nodes.push_back(make_node(2U, 30U, 40U, 22U, 23U, 0U, 1U));
    nodes.push_back(make_node(3U, 50U, 60U, 33U, 34U, 7U, 1U));
    RecordingActionPorts action_ports;
    RecordingAudioPorts audio_ports;

    const auto result = update_draw_legacy_picture_actions(
        nodes, 100, 200, action_ports, audio_ports
    );

    test.expect_true(
        result.status == LegacyPictureActionStatus::frame_load_failed &&
            result.visited_count == 3U &&
            result.action_update_failure_count == 1U &&
            result.frame_request_count == 3U &&
            result.frame_failure_count == 1U && result.draw_count == 2U &&
            result.blit_failure_count == 1U &&
            result.positional_sample_count == 1U && result.removed_count == 1U,
        "the checked frame boundary stops at the original unsafe dereference"
    );
    test.expect_true(
        action_ports.updated_ids == std::vector<u32>{1U, 2U, 3U} &&
            action_ports.loads ==
                std::vector<std::pair<u16, u16>>{
                    {11U, 12U}, {22U, 23U}, {33U, 34U}
                } &&
            action_ports.draws.size() == 2U,
        "an action-update diagnostic does not skip the original frame request"
    );
    test.expect_true(
        action_ports.draws[0].x == 9 && action_ports.draws[0].y == 18 &&
            action_ports.draws[0].flags == 0x101U &&
            action_ports.draws[0].opacity == 0x21 &&
            action_ports.draws[1].x == 28 && action_ports.draws[1].y == 37,
        "draw coordinates and action fields use their exact post-update widths"
    );
    test.expect_true(
        audio_ports.samples ==
                std::vector<std::tuple<u16, i32, i32>>{{5U, 110, 220}} &&
            nodes.size() == 2U && nodes.front().action.action_id == 1U &&
            nodes.front().action.field_58 == 0U &&
            nodes.back().action.action_id == 3U &&
            nodes.back().action.field_58 == 7U,
        "post-dereference sound and removal do not run after frame failure"
    );
}

void test_callback_reload_order(openswd3::test::Context& test) {
    std::list<LegacyPictureActionNode> nodes;
    nodes.push_back(make_node(1U, 10U, 20U, 11U, 12U, 5U, 0U));
    auto& node = nodes.front();
    RecordingActionPorts action_ports;
    RecordingAudioPorts audio_ports;
    action_ports.update_callback = [&]() {
        node.action.field_4a = 21U;
        node.action.field_4c = 22U;
    };
    action_ports.load_callback = [&]() {
        node.screen_x = 30U;
        node.screen_y = 40U;
        node.action.draw_offset_x = 3U;
        node.action.draw_offset_y = 4U;
        node.action.mode_flags = 0xABCDEF01U;
        node.action.field_8a = 0x2AU;
        node.action.field_58 = 6U;
    };
    action_ports.draw_callback = [&]() {
        node.screen_x = 50U;
        node.screen_y = 60U;
        node.action.field_58 = 7U;
        node.action.field_8c = 0U;
    };
    audio_ports.play_callback = [&]() { node.action.field_8c = 1U; };

    const auto result = update_draw_legacy_picture_actions(
        nodes, 100, 200, action_ports, audio_ports
    );
    test.expect_true(
        result.status == LegacyPictureActionStatus::completed &&
            result.visited_count == 1U && result.draw_count == 1U &&
            result.positional_sample_count == 1U &&
            result.removed_count == 1U && nodes.empty() &&
            action_ports.loads ==
                std::vector<std::pair<u16, u16>>{{21U, 22U}} &&
            action_ports.draws[0].x == 27 && action_ports.draws[0].y == 36 &&
            action_ports.draws[0].flags == 0xABCDEF01U &&
            action_ports.draws[0].opacity == 0x2A &&
            audio_ports.samples ==
                std::vector<std::tuple<u16, i32, i32>>{{7U, 150, 260}},
        "each callback boundary reloads only the fields read after it"
    );
}

void test_completion_must_equal_one(openswd3::test::Context& test) {
    std::list<LegacyPictureActionNode> nodes;
    nodes.push_back(make_node(4U, 0xFFFFU, 0U, 44U, 45U, 0U, 2U));
    RecordingActionPorts action_ports;
    RecordingAudioPorts audio_ports;

    const auto result = update_draw_legacy_picture_actions(
        nodes, std::bit_cast<i32>(0x7FFFFFFFU), 0, action_ports, audio_ports
    );

    test.expect_true(
        result.removed_count == 0U && nodes.size() == 1U,
        "completion values other than exact one remain linked"
    );
    test.expect_equal(
        action_ports.draws.front().x,
        i32{65531},
        "u16 screen coordinates are zero extended before subtraction"
    );
}

void test_release_primary_then_secondary_lists(openswd3::test::Context& test) {
    LegacyPictureActionLists lists;
    lists.primary.push_back(make_node(1U, 10U, 20U, 11U, 12U, 0U, 0U));
    lists.primary.push_back(make_node(2U, 30U, 40U, 21U, 22U, 0U, 0U));
    lists.secondary.push_back(make_node(3U, 50U, 60U, 31U, 32U, 0U, 0U));

    const auto released = release_legacy_picture_actions(lists);
    test.expect_true(
        released.primary_release_count == 2U &&
            released.secondary_release_count == 1U && lists.primary.empty() &&
            lists.secondary.empty(),
        "sub_40F5E0 releases both +0xA0 chains in primary-secondary order"
    );

    const auto empty = release_legacy_picture_actions(lists);
    test.expect_true(
        empty.primary_release_count == 0U &&
            empty.secondary_release_count == 0U,
        "sub_40F5E0 performs no release for two empty roots"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_update_draw_audio_and_removal(test);
    test_callback_reload_order(test);
    test_completion_must_equal_one(test);
    test_release_primary_then_secondary_lists(test);
    return test.exit_code();
}
