#include "test.hpp"

#include "openswd3/world_map/legacy_world_player_post_frame.hpp"

#include <array>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionDrawPorts;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::rendering::LegacyBlitExecutionStatus;
using openswd3::rendering::LegacyFramePiece;
using openswd3::world_map::advance_legacy_world_player_post_frame;
using openswd3::world_map::initialize_legacy_world_player_position_history;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialGroupCount;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldMovementRuntimeState;
using openswd3::world_map::LegacyWorldPlayerPostFrameState;
using openswd3::world_map::LegacyWorldPlayerPostFrameStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;

class RecordingActionPorts final : public LegacyActionDrawPorts {
public:
    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        ++updates;
        variants.push_back(record.variant_delta);
        return fail ? LegacyActionUpdateStatus::stream_load_failed
                    : LegacyActionUpdateStatus::completed;
    }

    [[nodiscard]] bool load_frame_piece(const u16, const u16,
                                        LegacyFramePiece&) override {
        return false;
    }

    [[nodiscard]] LegacyBlitExecutionStatus
    draw_frame_piece(const LegacyFramePiece&, const i32, const i32, const u32,
                     const i32) noexcept override {
        return LegacyBlitExecutionStatus::completed;
    }

    bool fail{};
    u32 updates{};
    std::vector<u32> variants;
};

struct Fixture {
    static constexpr u32 map_width = 4U;
    static constexpr u32 map_height = 4U;

    std::array<LegacyWorldRoleRecord, 2U> roles{};
    LegacyRoleSpatialIndex spatial;
    std::vector<u8> surface = std::vector<u8>(map_width * map_height * 4U);
    LegacyWorldMovementRuntimeState movement{
        .camera_x_transition = 1,
        .player_x_transition = 0,
        .camera_y_transition = 0,
        .player_y_transition = 1,
    };
    LegacyWorldPlayerPostFrameState state;
    RecordingActionPorts ports;

    Fixture() {
        spatial.map_height = map_height;
        const std::size_t rows = map_height + 2U * kLegacySpatialRowPadding;
        for (std::size_t group = 0U; group < kLegacySpatialGroupCount;
             ++group) {
            spatial.row_heads[group].assign(rows, kLegacySpatialNoRole);
        }

        LegacyWorldRoleRecord& player = roles[1];
        player.world_x = 32U;
        player.world_y = 32U;
        player.map_cell_pointer_32 = 5U;
        player.flags = 0x00001100U;
        player.guid = 7U;
        player.action.variant_delta = 6U;
        player.action.field_2c = 1U;
        player.action.field_30 = 1U;
        static_cast<void>(insert_legacy_role_spatially(spatial, roles, 1U));
        state.world_x_history.fill(16U);
        state.world_y_history.fill(16U);
        state.action_variant_history.fill(3U);
        write_cell(5U, 0xFFFFFFFFU);
    }

    void write_cell(const std::size_t index, const u32 value) {
        const std::size_t offset = index * 4U;
        surface[offset] = static_cast<u8>(value);
        surface[offset + 1U] = static_cast<u8>(value >> 8U);
        surface[offset + 2U] = static_cast<u8>(value >> 16U);
        surface[offset + 3U] = static_cast<u8>(value >> 24U);
    }

    [[nodiscard]] u32 read_cell(const std::size_t index) const {
        const std::size_t offset = index * 4U;
        return static_cast<u32>(surface[offset]) |
               (static_cast<u32>(surface[offset + 1U]) << 8U) |
               (static_cast<u32>(surface[offset + 2U]) << 16U) |
               (static_cast<u32>(surface[offset + 3U]) << 24U);
    }

    [[nodiscard]] auto run() {
        return advance_legacy_world_player_post_frame(
            roles[1], roles, spatial, movement, state,
            LegacyWorldRoleSurfaceContext{
                .map_width = map_width,
                .selected_guid = 7U,
                .surface_grid = surface,
            },
            ports);
    }
};

void test_initial_history(openswd3::test::Context& test) {
    LegacyWorldRoleRecord player{};
    player.world_x = 0x1234U;
    player.world_y = 0x5678U;
    LegacyWorldPlayerPostFrameState state;
    state.action_variant_history.fill(9U);

    initialize_legacy_world_player_position_history(state, player);
    test.expect_true(
        state.world_x_history.front() == 0x1234U &&
            state.world_x_history.back() == 0x1234U &&
            state.world_y_history.front() == 0x5678U &&
            state.world_y_history.back() == 0x5678U &&
            state.action_variant_history.front() == 9U,
        "initial world fills only the two 32-entry position histories");
}

void test_aligned_full_order(openswd3::test::Context& test) {
    Fixture fixture;
    const auto result = fixture.run();
    const std::size_t row = 2U + kLegacySpatialRowPadding;

    test.expect_true(
        result.status == LegacyWorldPlayerPostFrameStatus::completed &&
            result.aligned && result.spatially_relocated &&
            result.old_occupancy_cleared &&
            result.new_occupancy_marked && result.transitions_cleared &&
            result.history_shifted && result.cell_flags_refreshed &&
            result.action_validation_requested &&
            !result.action_update_failed && result.map_cell_delta == 5U &&
            result.cleared_cells == 1U && result.marked_cells == 1U,
        "aligned player executes every 0041272E..0041287C state slot");
    test.expect_true(
        fixture.roles[1].map_cell_pointer_32 == 10U &&
            fixture.read_cell(5U) == 0xCF7FFEFFU &&
            fixture.read_cell(10U) == 0x10000100U &&
            fixture.spatial.row_heads[0][row] == 1U &&
            fixture.roles[1].spatial_next_link_32 == 0U,
        "spatial relink, old clear, cell delta and new mark preserve order");
    test.expect_true(
        fixture.movement.camera_x_transition == 0 &&
            fixture.movement.player_x_transition == 0 &&
            fixture.movement.camera_y_transition == 0 &&
            fixture.movement.player_y_transition == 0 &&
            fixture.state.world_x_history[0] == 32U &&
            fixture.state.world_x_history[1] == 16U &&
            fixture.state.world_y_history[0] == 32U &&
            fixture.state.action_variant_history[0] == 6U &&
            fixture.state.action_variant_history[1] == 3U &&
            fixture.ports.updates == 1U,
        "transition clear precedes one overlapping 0x7C history shift");
}

void test_unaligned_still_validates_action(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].world_x = 33U;
    fixture.ports.fail = true;

    const auto result = fixture.run();
    test.expect_true(
        result.status == LegacyWorldPlayerPostFrameStatus::completed &&
            !result.aligned && !result.spatially_relocated &&
            !result.transitions_cleared && !result.history_shifted &&
            result.action_validation_requested &&
            result.action_update_failed && result.action_update_count == 1U &&
            fixture.movement.camera_x_transition == 1 &&
            fixture.roles[1].map_cell_pointer_32 == 5U &&
            fixture.ports.updates == 1U,
        "unaligned gate skips bookkeeping but not 0041283C validation");
}

void test_bit30_suppresses_validation(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].world_x = 33U;
    fixture.roles[1].flags |= 0x40000000U;

    const auto result = fixture.run();
    test.expect_true(
        result.status == LegacyWorldPlayerPostFrameStatus::completed &&
            !result.action_validation_requested &&
            result.action_update_count == 0U && fixture.ports.updates == 0U,
        "role bit 30 wins over the bit-12 action validation gate");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_initial_history(test);
    test_aligned_full_order(test);
    test_unaligned_still_validates_action(test);
    test_bit30_suppresses_validation(test);
    return test.exit_code();
}
