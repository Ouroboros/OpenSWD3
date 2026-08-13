#include "test.hpp"

#include "openswd3/world_map/legacy_world_path_requests.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::insert_legacy_role_spatially;
using openswd3::world_map::kLegacySpatialNoRole;
using openswd3::world_map::kLegacySpatialRowPadding;
using openswd3::world_map::kLegacyWorldActiveObjectSlotCount;
using openswd3::world_map::kLegacyWorldPartySlotCount;
using openswd3::world_map::LegacyRoleSpatialIndex;
using openswd3::world_map::LegacyWorldCameraRect;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldPartyPathPorts;
using openswd3::world_map::LegacyWorldPartyPathPreparationStatus;
using openswd3::world_map::LegacyWorldPathNodePool;
using openswd3::world_map::LegacyWorldPlayerPostFrameState;
using openswd3::world_map::LegacyWorldRolePathRequestStatus;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::prepare_legacy_world_party_path;
using openswd3::world_map::prepare_legacy_world_party_paths;
using openswd3::world_map::request_legacy_world_role_path;

constexpr u32 kMapWidth = 20U;
constexpr u32 kMapHeight = 20U;
constexpr std::size_t kPathBytesOffset = 0x1CU;

[[nodiscard]] u16
read_u16(const LegacyWorldObjectSlot& slot, const std::size_t offset) noexcept {
    return static_cast<u16>(slot.bytes[offset]) |
        static_cast<u16>(static_cast<u16>(slot.bytes[offset + 1U]) << 8U);
}

void write_u16(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_cell(
    std::vector<u8>& surface, const u32 index, const u32 value
) noexcept {
    const std::size_t offset = static_cast<std::size_t>(index) * 4U;
    surface[offset] = static_cast<u8>(value);
    surface[offset + 1U] = static_cast<u8>(value >> 8U);
    surface[offset + 2U] = static_cast<u8>(value >> 16U);
    surface[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::array<u8, 6U>
make_path_command(const u16 x, const u16 y) noexcept {
    return {
        7U,
        0U,
        static_cast<u8>(x),
        static_cast<u8>(x >> 8U),
        static_cast<u8>(y),
        static_cast<u8>(y >> 8U)
    };
}

struct RolePathFixture {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::vector<u8> surface = std::vector<u8>(kMapWidth * kMapHeight * 4U, 0U);
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    LegacyWorldPathNodePool node_pool;

    RolePathFixture() {
        roles[1].world_x = 5U << 4U;
        roles[1].world_y = 5U << 4U;
        roles[1].map_cell_pointer_32 = 5U * kMapWidth + 5U;
        roles[1].action.field_2c = 1U;
        roles[1].action.field_30 = 1U;
    }

    [[nodiscard]] auto run(const std::span<const u8> command) {
        return request_legacy_world_role_path(
            1U,
            command,
            roles,
            LegacyWorldRoleSurfaceContext{
                .map_width = kMapWidth,
                .selected_guid = 1U,
                .surface_grid = surface,
            },
            kMapHeight,
            slots,
            node_pool
        );
    }
};

void test_ordinary_role_path_slot_layout(openswd3::test::Context& test) {
    RolePathFixture fixture;
    const auto result = fixture.run(make_path_command(7U, 5U));

    test.expect_true(
        result.status == LegacyWorldRolePathRequestStatus::completed &&
            result.free_slot_found && result.target_in_legacy_bounds &&
            result.path_found && result.slot_index == 0U &&
            result.legacy_return_value == 1,
        "0x00406390 uses the first free ordinary object slot"
    );
    const auto& slot = fixture.slots[0];
    test.expect_true(
        read_u16(slot, 0x00U) == 1U && read_u16(slot, 0x02U) == 0U &&
            read_u16(slot, 0x04U) == (7U << 4U) &&
            read_u16(slot, 0x06U) == (5U << 4U) &&
            read_u16(slot, 0x10U) == 0xFFFFU &&
            read_u16(slot, 0x12U) == 0xFFFFU &&
            read_u16(slot, 0x14U) == 0xFFFFU && slot.bytes[0x1BU] == 0xF1U &&
            slot.bytes[kPathBytesOffset] == 7U &&
            slot.bytes[kPathBytesOffset + 1U] == 7U &&
            slot.bytes[kPathBytesOffset + 2U] == 0xFFU,
        "role, destination, action sentinels and east path keep their offsets"
    );
}

void test_ordinary_role_path_legacy_returns(openswd3::test::Context& test) {
    {
        RolePathFixture fixture;
        for (u32 index = 0U; index < 0x20U; ++index) {
            write_u16(fixture.slots[index], 0U, static_cast<u16>(index));
        }
        const auto result = fixture.run(make_path_command(7U, 5U));
        test.expect_true(
            result.path_found && result.slot_index == 0x20U &&
                result.legacy_return_value == 0,
            "the original returns false for successful slot 32"
        );
    }

    {
        RolePathFixture fixture;
        for (u32 index = 0U; index < fixture.slots.size(); ++index) {
            write_u16(fixture.slots[index], 0U, static_cast<u16>(index));
        }
        const auto result = fixture.run(make_path_command(7U, 5U));
        test.expect_true(
            !result.free_slot_found && result.slot_index == 72U &&
                result.legacy_return_value == 1,
            "a full 72-slot table preserves the strange true return"
        );
    }

    {
        RolePathFixture fixture;
        const auto result = fixture.run(make_path_command(21U, 5U));
        test.expect_true(
            result.free_slot_found && !result.target_in_legacy_bounds &&
                result.legacy_return_value == 0 &&
                read_u16(fixture.slots[0], 0U) == 0xFFFFU,
            "an out-of-map command returns zero without filling slot"
        );
    }
}

void test_ordinary_path_failure_writes_destination(
    openswd3::test::Context& test
) {
    RolePathFixture fixture;
    write_cell(fixture.surface, 5U * kMapWidth + 7U, 0x40800000U);
    const auto result = fixture.run(make_path_command(7U, 5U));
    test.expect_true(
        result.status == LegacyWorldRolePathRequestStatus::completed &&
            !result.path_found && result.role_relocated_after_path_failure &&
            result.legacy_return_value == 1 &&
            fixture.roles[1].world_x == (7U << 4U) &&
            fixture.roles[1].world_y == (5U << 4U) &&
            read_u16(fixture.slots[0], 0U) == 0xFFFFU,
        "failed A* directly writes role coordinates but leaves the slot free"
    );
}

struct PartyPathFixture {
    class Ports final : public LegacyWorldPartyPathPorts {
    public:
        [[nodiscard]] bool query_collision_disabled() noexcept override {
            ++query_count;
            return collision_disabled;
        }

        bool collision_disabled{};
        u32 query_count{};
    } ports;

    std::array<LegacyWorldRoleRecord, 3U> roles{};
    LegacyRoleSpatialIndex spatial;
    std::vector<u8> surface = std::vector<u8>(kMapWidth * kMapHeight * 4U, 0U);
    std::array<u32, kLegacyWorldPartySlotCount> party_indices = [] {
        std::array<u32, kLegacyWorldPartySlotCount> value{};
        value.fill(0xFFFFFFFFU);
        return value;
    }();
    std::array<LegacyWorldObjectSlot, kLegacyWorldPartySlotCount> slots;
    LegacyWorldPlayerPostFrameState history;
    LegacyWorldCameraRect camera{0U, 0U, 640U, 480U};
    LegacyWorldPathNodePool node_pool;

    PartyPathFixture() {
        roles[1].world_x = 10U << 4U;
        roles[1].world_y = 10U << 4U;
        roles[1].guid = 1U;
        roles[1].action.variant_delta = 6U;

        auto& follower = roles[2];
        follower.world_x = 5U << 4U;
        follower.world_y = 5U << 4U;
        follower.map_cell_pointer_32 = 5U * kMapWidth + 5U;
        follower.flags = 0x00008080U;
        follower.guid = 2U;
        follower.action.field_2c = 1U;
        follower.action.field_30 = 1U;

        spatial.map_height = kMapHeight;
        const std::size_t row_count = static_cast<std::size_t>(kMapHeight) +
            2U * kLegacySpatialRowPadding;
        for (auto& rows : spatial.row_heads) {
            rows.assign(row_count, kLegacySpatialNoRole);
        }
        static_cast<void>(insert_legacy_role_spatially(spatial, roles, 2U));
        write_cell(surface, follower.map_cell_pointer_32, 0x10000000U);

        party_indices[0] = 1U;
        party_indices[1] = 2U;
        history.world_x_history.fill(roles[1].world_x);
        history.world_y_history.fill(roles[1].world_y);
        history.world_x_history[2] = 7U << 4U;
        history.world_y_history[2] = 5U << 4U;
    }

    [[nodiscard]] auto run(const bool collision_disabled = false) {
        ports.collision_disabled = collision_disabled;
        return prepare_legacy_world_party_paths(
            roles,
            spatial,
            LegacyWorldRoleSurfaceContext{
                .map_width = kMapWidth,
                .selected_guid = roles[1].guid,
                .surface_grid = surface,
            },
            1U,
            2U,
            party_indices,
            slots,
            history,
            camera,
            node_pool,
            ports
        );
    }
};

void test_party_path_generation_and_reuse(openswd3::test::Context& test) {
    {
        PartyPathFixture fixture;
        const auto result = fixture.run();
        const auto& slot = fixture.slots[1];
        test.expect_true(
            result.status == LegacyWorldPartyPathPreparationStatus::completed &&
                result.roles_scanned == 2U && result.eligible_roles == 1U &&
                result.paths_generated == 1U && result.paths_reused == 0U &&
                result.movement_slots_enabled == 1U &&
                result.preadvanced_steps == 0U &&
                result.collision_service_queries == 2U &&
                fixture.ports.query_count == 2U &&
                result.spatial_removal_failures == 0U,
            "aligned party role generates one route and arms its movement slot"
        );
        test.expect_equal(read_u16(slot, 0x00U), u16{2U}, "party slot role");
        test.expect_equal(read_u16(slot, 0x02U), u16{0U}, "party slot cursor");
        test.expect_equal(
            read_u16(slot, 0x04U), u16{7U << 4U}, "party slot target x"
        );
        test.expect_equal(slot.bytes[0x1AU], u8{1U}, "party slot stall");
        test.expect_equal(slot.bytes[0x1BU], u8{0xF1U}, "party slot flags");
        test.expect_equal(read_u16(slot, 0x16U), u16{8U}, "party east step x");
        test.expect_equal(read_u16(slot, 0x18U), u16{0U}, "party east step y");
        test.expect_equal(
            slot.bytes[kPathBytesOffset], u8{7U}, "party first path direction"
        );
        test.expect_equal(
            fixture.roles[2].action.base_variant,
            u32{8U},
            "party movement action base"
        );
        test.expect_equal(
            fixture.roles[2].action.variant_delta,
            u32{3U},
            "generated east path selects the east-facing variant"
        );
    }

    {
        PartyPathFixture fixture;
        auto& slot = fixture.slots[1];
        write_u16(slot, 0x00U, 2U);
        write_u16(slot, 0x02U, 0x8000U);
        slot.bytes[0x1AU] = 0U;
        slot.bytes[kPathBytesOffset] = 7U;
        slot.bytes[kPathBytesOffset + 1U] = 0xFFU;
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyWorldPartyPathPreparationStatus::completed &&
                result.paths_reused == 1U && result.paths_generated == 0U &&
                result.preadvanced_steps == 0U &&
                result.collision_service_queries == 1U &&
                fixture.ports.query_count == 1U &&
                result.movement_slots_enabled == 1U &&
                read_u16(slot, 0x02U) == 0U && slot.bytes[0x1AU] == 1U,
            "live zero-stall path is reused and clears the frame-gate cursor bit"
        );
    }
}

void test_single_party_role_entry_point(openswd3::test::Context& test) {
    PartyPathFixture fixture;
    const auto result = prepare_legacy_world_party_path(
        2U,
        fixture.roles,
        fixture.spatial,
        LegacyWorldRoleSurfaceContext{
            .map_width = kMapWidth,
            .selected_guid = fixture.roles[1].guid,
            .surface_grid = fixture.surface,
        },
        1U,
        2U,
        fixture.party_indices,
        fixture.slots,
        fixture.history,
        fixture.camera,
        fixture.node_pool,
        fixture.ports
    );
    test.expect_true(
        result.status == LegacyWorldPartyPathPreparationStatus::completed &&
            result.roles_scanned == 1U && result.eligible_roles == 1U &&
            result.paths_generated == 1U &&
            read_u16(fixture.slots[1], 0x00U) == 2U,
        "the direct sub_406960 entry processes exactly the role selected by " "the caller's per-role dispatcher"
    );

    const auto invalid = prepare_legacy_world_party_path(
        3U,
        fixture.roles,
        fixture.spatial,
        LegacyWorldRoleSurfaceContext{
            .map_width = kMapWidth,
            .selected_guid = fixture.roles[1].guid,
            .surface_grid = fixture.surface,
        },
        1U,
        2U,
        fixture.party_indices,
        fixture.slots,
        fixture.history,
        fixture.camera,
        fixture.node_pool,
        fixture.ports
    );
    test.expect_equal(
        invalid.status,
        LegacyWorldPartyPathPreparationStatus::invalid_party_role_index,
        "the modern direct entry checks the caller-provided role boundary"
    );
}

void test_party_offscreen_preadvance_and_terminal_action(
    openswd3::test::Context& test
) {
    PartyPathFixture fixture;
    fixture.camera = {640U, 480U, 1280U, 960U};
    const auto result = fixture.run();
    const auto& slot = fixture.slots[1];
    test.expect_true(
        result.status == LegacyWorldPartyPathPreparationStatus::completed &&
            result.paths_generated == 1U && result.preadvanced_steps == 2U &&
            result.terminal_paths == 1U && result.movement_slots_enabled == 0U,
        "off-screen follower consumes its complete generated path"
    );
    test.expect_equal(
        fixture.roles[2].world_x,
        u32{(5U << 4U) + 8U},
        "off-screen follower final x"
    );
    test.expect_equal(
        fixture.roles[2].world_y, u32{5U << 4U}, "off-screen follower final y"
    );
    test.expect_equal(
        fixture.roles[2].map_cell_pointer_32,
        u32{5U * kMapWidth + 5U},
        "off-screen follower final map cell"
    );
    test.expect_equal(
        fixture.roles[2].action.base_variant,
        u32{0U},
        "terminal follower action base"
    );
    test.expect_equal(
        fixture.roles[2].action.variant_delta,
        u32{6U},
        "terminal follower copies selected role facing"
    );
    test.expect_equal(
        read_u16(slot, 0x02U),
        u16{0x8002U},
        "terminal follower cursor and frame gate"
    );
    test.expect_equal(
        read_u16(slot, 0x16U), u16{0U}, "terminal follower step x"
    );
    test.expect_equal(
        read_u16(slot, 0x18U), u16{0U}, "terminal follower step y"
    );
}

void test_party_collision_gate_and_service_override(
    openswd3::test::Context& test
) {
    for (const bool collision_disabled : std::array<bool, 2U>{false, true}) {
        PartyPathFixture fixture;
        auto& slot = fixture.slots[1];
        write_u16(slot, 0x00U, 2U);
        write_u16(slot, 0x02U, 0U);
        slot.bytes[0x1AU] = 0U;
        slot.bytes[kPathBytesOffset] = 7U;
        slot.bytes[kPathBytesOffset + 1U] = 0xFFU;
        write_cell(fixture.surface, 5U * kMapWidth + 6U, 0x40000000U);

        const auto result = fixture.run(collision_disabled);
        test.expect_true(
            result.status == LegacyWorldPartyPathPreparationStatus::completed &&
                result.paths_reused == 1U &&
                result.movement_slots_blocked ==
                    (collision_disabled ? 0U : 1U) &&
                result.movement_slots_enabled ==
                    (collision_disabled ? 1U : 0U) &&
                fixture.roles[2].action.base_variant ==
                    (collision_disabled ? 8U : 0U) &&
                read_u16(slot, 0x16U) == (collision_disabled ? 8U : 0U),
            "service 0x4F alone changes the 0x40000000 collision mask to zero"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_ordinary_role_path_slot_layout(test);
    test_ordinary_role_path_legacy_returns(test);
    test_ordinary_path_failure_writes_destination(test);
    test_party_path_generation_and_reuse(test);
    test_single_party_role_entry_point(test);
    test_party_offscreen_preadvance_and_terminal_action(test);
    test_party_collision_gate_and_service_override(test);
    return test.exit_code();
}
