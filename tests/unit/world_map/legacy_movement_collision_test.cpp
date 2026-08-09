#include "test.hpp"

#include "openswd3/world_map/legacy_movement_collision.hpp"
#include "openswd3/world_map/legacy_role_spatial_query.hpp"

#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u32;
using openswd3::world_map::check_legacy_movement_collision;
using openswd3::world_map::kLegacyMovementCollisionNoRole;
using openswd3::world_map::kLegacyRoleCollisionFlag;
using openswd3::world_map::kLegacyRoleSpatiallyActiveFlag;
using openswd3::world_map::LegacyMovementCollisionPorts;
using openswd3::world_map::LegacyMovementCollisionStatus;
using openswd3::world_map::LegacyWorldRoleRecord;

struct Tile {
    u32 x{};
    u32 y{};

    bool operator==(const Tile&) const = default;
};

class RecordingPorts final : public LegacyMovementCollisionPorts {
public:
    std::optional<u32> read_map_cell(const u32 cell_index) override {
        read_cells.push_back(cell_index);
        if (invalid_cell.has_value() && *invalid_cell == cell_index) {
            return std::nullopt;
        }
        const auto value = cell_values.find(cell_index);
        return value == cell_values.end() ? std::optional<u32>{0U}
                                          : std::optional<u32>{value->second};
    }

    u32 find_collision_role_at_tile(
        const u32 tile_x,
        const u32 tile_y
    ) override {
        queried_tiles.push_back(Tile{tile_x, tile_y});
        if (hit_query.has_value() &&
            queried_tiles.size() == *hit_query) {
            return hit_role_index;
        }
        return kLegacyMovementCollisionNoRole;
    }

    std::vector<u32> read_cells;
    std::vector<Tile> queried_tiles;
    std::unordered_map<u32, u32> cell_values;
    std::optional<u32> invalid_cell;
    std::optional<std::size_t> hit_query;
    u32 hit_role_index{7U};
};

struct DirectionCase {
    i32 delta_x{};
    i32 delta_y{};
    std::vector<u32> cells;
    std::vector<Tile> tiles;
};

LegacyWorldRoleRecord make_role() {
    LegacyWorldRoleRecord role{};
    role.world_x = 10U * 16U + 15U;
    role.world_y = 20U * 16U + 15U;
    role.map_cell_pointer_32 = 500U;
    role.action.field_2c = 3U;
    role.action.field_30 = 2U;
    return role;
}

void test_all_nine_selector_paths(openswd3::test::Context& test) {
    const LegacyWorldRoleRecord role = make_role();
    const std::vector<DirectionCase> cases{
        {-1, -1,
         {479U, 480U, 481U, 479U, 482U},
         {{9U, 19U}, {10U, 19U}, {11U, 19U},
          {9U, 19U}, {9U, 20U}}},
        {0, -1,
         {480U, 481U, 482U},
         {{10U, 19U}, {11U, 19U}, {12U, 19U}}},
        {1, -1,
         {481U, 482U, 483U, 483U, 486U},
         {{11U, 19U}, {12U, 19U}, {13U, 19U},
          {14U, 19U}, {14U, 20U}}},
        {-1, 0,
         {499U, 502U},
         {{9U, 20U}, {9U, 21U}}},
        {0, 0, {}, {}},
        {1, 0,
         {503U, 506U},
         {{13U, 20U}, {13U, 21U}}},
        {-1, 1,
         {539U, 540U, 541U, 519U, 522U},
         {{9U, 22U}, {10U, 22U}, {11U, 22U},
          {9U, 21U}, {9U, 22U}}},
        {0, 1,
         {540U, 541U, 542U},
         {{10U, 22U}, {11U, 22U}, {12U, 22U}}},
        {1, 1,
         {541U, 542U, 543U, 503U, 506U},
         {{11U, 22U}, {12U, 22U}, {13U, 22U},
          {13U, 21U}, {13U, 22U}}},
    };

    for (const auto& direction : cases) {
        RecordingPorts ports;
        const auto result = check_legacy_movement_collision(
            role,
            direction.delta_x,
            direction.delta_y,
            20U,
            ports
        );

        test.expect_equal(
            result.status,
            LegacyMovementCollisionStatus::completed,
            "direction scan completes"
        );
        test.expect_equal(
            ports.read_cells,
            direction.cells,
            "direction preserves map-cell scan order"
        );
        test.expect_equal(
            ports.queried_tiles,
            direction.tiles,
            "direction preserves role-query coordinate order"
        );
        test.expect_equal(
            result.event_code,
            direction.cells.empty() ? u32{0xFFFFFFFFU} : u32{0U},
            "direction returns the last low-byte map value"
        );
        test.expect_equal(
            result.hit_role_index,
            kLegacyMovementCollisionNoRole,
            "direction without role collision retains the sentinel"
        );
    }
}

void test_map_event_stops_before_role_query(
    openswd3::test::Context& test
) {
    const LegacyWorldRoleRecord role = make_role();
    RecordingPorts ports;
    ports.cell_values[481U] = 0xA1B2C35AU;

    const auto result = check_legacy_movement_collision(
        role,
        0,
        -1,
        20U,
        ports
    );

    test.expect_equal(
        ports.read_cells,
        std::vector<u32>{480U, 481U},
        "the first nonzero map byte stops the edge scan"
    );
    test.expect_equal(
        ports.queried_tiles,
        std::vector<Tile>{{10U, 19U}},
        "a nonzero map byte is tested before role occupancy"
    );
    test.expect_equal(
        result.event_code,
        u32{0x5AU},
        "only the low map-cell byte is returned"
    );
    test.expect_equal(
        result.hit_role_index,
        kLegacyMovementCollisionNoRole,
        "map event leaves the role sentinel intact"
    );
}

void test_role_hit_returns_zero_and_stops(openswd3::test::Context& test) {
    const LegacyWorldRoleRecord role = make_role();
    RecordingPorts ports;
    ports.hit_query = 2U;
    ports.hit_role_index = 19U;

    const auto result = check_legacy_movement_collision(
        role,
        1,
        0,
        20U,
        ports
    );

    test.expect_equal(
        ports.read_cells,
        std::vector<u32>{503U, 506U},
        "role hit stops after the matching map cell"
    );
    test.expect_equal(
        result.event_code,
        u32{0U},
        "role collision follows the shared zero return path"
    );
    test.expect_equal(
        result.hit_role_index,
        u32{19U},
        "role collision publishes the matching role index"
    );
}

void test_selector_alias_and_default(openswd3::test::Context& test) {
    const LegacyWorldRoleRecord role = make_role();
    RecordingPorts alias_ports;
    const auto alias = check_legacy_movement_collision(
        role,
        2,
        0,
        20U,
        alias_ports
    );
    test.expect_equal(
        alias_ports.read_cells,
        std::vector<u32>{539U, 540U, 541U, 519U, 522U},
        "selector uses dx plus three times dy without range checks"
    );
    test.expect_equal(
        alias.event_code,
        u32{0U},
        "selector alias completes the selected branch"
    );

    RecordingPorts default_ports;
    const auto outside = check_legacy_movement_collision(
        role,
        5,
        0,
        20U,
        default_ports
    );
    test.expect_true(
        default_ports.read_cells.empty(),
        "selector above eight takes the default path"
    );
    test.expect_equal(
        outside.event_code,
        u32{0xFFFFFFFFU},
        "default path returns the initialized all-bits-set value"
    );
}

void test_zero_extent_and_wrapping(openswd3::test::Context& test) {
    LegacyWorldRoleRecord role = make_role();
    role.map_cell_pointer_32 = 0U;
    role.action.field_2c = 0U;
    role.action.field_30 = 2U;
    RecordingPorts ports;

    const auto result = check_legacy_movement_collision(
        role,
        -1,
        -1,
        1U,
        ports
    );
    test.expect_equal(
        ports.read_cells,
        std::vector<u32>{0xFFFFFFFEU, 0xFFFFFFFEU},
        "zero width repeats the wrapped corner address"
    );
    test.expect_equal(
        result.event_code,
        u32{0U},
        "wrapped reads retain x86 dword arithmetic"
    );
}

void test_invalid_map_cell_isolated(openswd3::test::Context& test) {
    const LegacyWorldRoleRecord role = make_role();
    RecordingPorts ports;
    ports.invalid_cell = 481U;

    const auto result = check_legacy_movement_collision(
        role,
        0,
        -1,
        20U,
        ports
    );
    test.expect_equal(
        result.status,
        LegacyMovementCollisionStatus::invalid_map_cell,
        "modern grid boundary reports an invalid cell"
    );
    test.expect_equal(
        ports.read_cells,
        std::vector<u32>{480U, 481U},
        "invalid cell stops further legacy pointer emulation"
    );
}

void write_cell(
    std::vector<openswd3::compat::u8>& cells,
    const u32 cell_index,
    const u32 value
) {
    const std::size_t offset = static_cast<std::size_t>(cell_index) * 4U;
    cells[offset] = static_cast<openswd3::compat::u8>(value);
    cells[offset + 1U] = static_cast<openswd3::compat::u8>(value >> 8U);
    cells[offset + 2U] = static_cast<openswd3::compat::u8>(value >> 16U);
    cells[offset + 3U] = static_cast<openswd3::compat::u8>(value >> 24U);
}

void test_session_grid_and_role_adapter(openswd3::test::Context& test) {
    std::vector<LegacyWorldRoleRecord> roles(2U);
    roles[0] = make_role();
    roles[1].world_x = 13U << 4U;
    roles[1].world_y = 20U << 4U;
    roles[1].talk_script_id = 1U;
    roles[1].flags =
        kLegacyRoleSpatiallyActiveFlag | kLegacyRoleCollisionFlag;
    roles[1].action.field_2c = 1U;
    roles[1].action.field_30 = 1U;
    std::vector<openswd3::compat::u8> cells(800U * 4U);

    const auto role_hit = check_legacy_movement_collision(
        roles,
        2U,
        0U,
        1,
        0,
        cells,
        20U
    );
    test.expect_equal(
        role_hit.status,
        LegacyMovementCollisionStatus::completed,
        "session adapter reads a valid four-byte grid cell"
    );
    test.expect_equal(
        role_hit.hit_role_index,
        u32{1U},
        "session adapter delegates to the collision-role query"
    );

    write_cell(cells, 480U, 0xAABBCC39U);
    const auto map_event = check_legacy_movement_collision(
        roles,
        2U,
        0U,
        0,
        -1,
        cells,
        20U
    );
    test.expect_equal(
        map_event.event_code,
        u32{0x39U},
        "session adapter decodes the little-endian cell low byte"
    );
    test.expect_equal(
        map_event.hit_role_index,
        kLegacyMovementCollisionNoRole,
        "session map event wins before role lookup"
    );
}

void test_session_adapter_boundaries(openswd3::test::Context& test) {
    const std::vector<LegacyWorldRoleRecord> roles{make_role()};
    const std::vector<openswd3::compat::u8> short_cells(4U);

    const auto invalid_role = check_legacy_movement_collision(
        roles,
        1U,
        1U,
        0,
        -1,
        short_cells,
        20U
    );
    test.expect_equal(
        invalid_role.status,
        LegacyMovementCollisionStatus::invalid_role,
        "modern role span rejects an out-of-range role index"
    );

    const auto invalid_cell = check_legacy_movement_collision(
        roles,
        1U,
        0U,
        0,
        -1,
        short_cells,
        20U
    );
    test.expect_equal(
        invalid_cell.status,
        LegacyMovementCollisionStatus::invalid_map_cell,
        "modern map span isolates an out-of-range legacy pointer"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_all_nine_selector_paths(test);
    test_map_event_stops_before_role_query(test);
    test_role_hit_returns_zero_and_stops(test);
    test_selector_alias_and_default(test);
    test_zero_extent_and_wrapping(test);
    test_invalid_map_cell_isolated(test);
    test_session_grid_and_role_adapter(test);
    test_session_adapter_boundaries(test);
    return test.exit_code();
}
