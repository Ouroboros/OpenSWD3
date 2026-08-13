#include "test.hpp"

#include "openswd3/world_map/legacy_moving_actions.hpp"

#include <cstddef>
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
using openswd3::world_map::LegacyMovingActionList;
using openswd3::world_map::LegacyMovingActionNode;
using openswd3::world_map::update_draw_legacy_moving_actions;

struct DrawCall {
    i32 x{};
    i32 y{};
    u32 flags{};
    i32 opacity{};
};

class RecordingPorts final : public LegacyActionDrawPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        updated_ids.push_back(record.action_id);
        record.field_4a = static_cast<u16>(0x1200U + record.action_id);
        record.field_4c = static_cast<u16>(0x20U + record.action_id);
        return record.action_id == 2U
            ? LegacyActionUpdateStatus::malformed_stream
            : LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(
        const u16 resource_id, const u16 frame_index, LegacyFramePiece& piece
    ) override {
        loads.emplace_back(resource_id, frame_index);
        piece.width = 16U;
        piece.height = 16U;
        return resource_id != 0x1203U;
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
        return draws.size() == 2U ? LegacyBlitExecutionStatus::malformed_source
                                  : LegacyBlitExecutionStatus::completed;
    }

    std::vector<u32> updated_ids;
    std::vector<std::pair<u16, u16>> loads;
    std::vector<DrawCall> draws;
};

[[nodiscard]] LegacyMovingActionNode
make_node(const u32 id, const float x, const float y) {
    LegacyMovingActionNode node{};
    node.action.action_id = id;
    node.action.draw_offset_x = id;
    node.action.draw_offset_y = id + 1U;
    node.action.mode_flags = 0x100U + id;
    node.action.field_8a = static_cast<openswd3::compat::u8>(0x30U + id);
    node.position_x = x;
    node.position_y = y;
    return node;
}

void test_exact_layout_and_frame_order(openswd3::test::Context& test) {
    test.expect_equal(
        sizeof(LegacyMovingActionNode),
        std::size_t{0xB4U},
        "moving PicPaint node retains its full physical size"
    );
    test.expect_equal(
        offsetof(LegacyMovingActionNode, action),
        std::size_t{0x00U},
        "the action record starts at node zero"
    );
    test.expect_equal(
        offsetof(LegacyMovingActionNode, position_x),
        std::size_t{0xA8U},
        "float position keeps the producer offset"
    );
    test.expect_equal(
        offsetof(LegacyMovingActionNode, next_pointer_32),
        std::size_t{0xB0U},
        "legacy next remains the final dword"
    );

    LegacyMovingActionList nodes;
    auto moving = make_node(1U, 100.75F, 200.75F);
    moving.target_x = 102;
    moving.target_y = 202;
    moving.velocity_x = 1.0F;
    moving.velocity_y = 1.0F;
    nodes.push_back(moving);

    auto held_boundary = make_node(2U, -62.0F, 20.0F);
    held_boundary.action.wait_remaining = 1U;
    nodes.push_back(held_boundary);

    auto missing_frame = make_node(3U, 60.0F, 80.0F);
    missing_frame.action.wait_remaining = 1U;
    nodes.push_back(missing_frame);

    auto blit_failure = make_node(4U, 80.0F, 90.0F);
    blit_failure.action.wait_remaining = 1U;
    nodes.push_back(blit_failure);

    RecordingPorts ports;
    const auto result = update_draw_legacy_moving_actions(nodes, 10, 20, ports);

    test.expect_true(
        result.visited_count == 4U &&
            result.action_update_failure_count == 1U &&
            result.frame_request_count == 3U &&
            result.frame_failure_count == 1U && result.draw_count == 2U &&
            result.blit_failure_count == 1U && result.removed_count == 1U,
        "0x00414B60 preserves update, strict visibility, draw and retirement " "order"
    );
    test.expect_true(
        ports.updated_ids == std::vector<u32>{1U, 2U, 3U, 4U} &&
            ports.loads ==
                std::vector<std::pair<u16, u16>>{
                    {0x1201U, 0x21U}, {0x1203U, 0x23U}, {0x1204U, 0x24U}
                } &&
            ports.draws.size() == 2U && ports.draws[0].x == 89 &&
            ports.draws[0].y == 178 && ports.draws[0].flags == 0x101U &&
            ports.draws[0].opacity == 0x31 && ports.draws[1].x == 66 &&
            ports.draws[1].y == 65,
        "drawing uses pre-movement coordinates and post-update action fields"
    );
    test.expect_true(
        nodes.size() == 3U && nodes.front().action.action_id == 2U &&
            nodes.front().position_x == -62.0F,
        "wait_remaining suppresses both movement and retirement"
    );
}

void test_strict_target_window_edges(openswd3::test::Context& test) {
    LegacyMovingActionList nodes;
    auto edge = make_node(4U, 0.0F, 0.0F);
    edge.target_x = 32;
    edge.target_y = 0;
    nodes.push_back(edge);
    auto inside = make_node(5U, 1.0F, 0.0F);
    inside.target_x = 32;
    inside.target_y = 0;
    nodes.push_back(inside);
    RecordingPorts ports;

    const auto result =
        update_draw_legacy_moving_actions(nodes, 1000, 1000, ports);

    test.expect_true(
        result.frame_request_count == 0U && result.removed_count == 1U &&
            nodes.size() == 1U && nodes.front().action.action_id == 4U,
        "target plus/minus 32 edges are excluded exactly"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_exact_layout_and_frame_order(test);
    test_strict_target_window_edges(test);
    return test.exit_code();
}
