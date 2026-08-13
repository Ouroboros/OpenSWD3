#include "test.hpp"

#include "openswd3/world_map/legacy_world_collision_talk.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace {

using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::coordinate_legacy_world_collision_talk;
using openswd3::world_map::kLegacyMovementCollisionNoRole;
using openswd3::world_map::kLegacyWorldTalkIdleSource;
using openswd3::world_map::kLegacyWorldTalkMapEventSource;
using openswd3::world_map::kLegacyWorldTalkTurningRoleFlag;
using openswd3::world_map::LegacyMovementCollisionResult;
using openswd3::world_map::LegacyMovementCollisionStatus;
using openswd3::world_map::LegacyWorldCollisionTalkPorts;
using openswd3::world_map::LegacyWorldCollisionTalkRequest;
using openswd3::world_map::LegacyWorldCollisionTalkStatus;
using openswd3::world_map::LegacyWorldMapEvent;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldTalkContext;
using openswd3::world_map::LegacyWorldTalkSource;

struct CollisionCall {
    u32 role_index{};
    i32 delta_x{};
    i32 delta_y{};

    bool operator==(const CollisionCall&) const = default;
};

struct ActionSnapshot {
    u32 base_variant{};
    u32 variant_delta{};
    u32 one_shot_base_variant{};
    u32 one_shot_variant_delta{};
    u16 wait_remaining{};

    bool operator==(const ActionSnapshot&) const = default;
};

class RecordingPorts final : public LegacyWorldCollisionTalkPorts {
public:
    LegacyMovementCollisionResult query_collision(
        const u32 role_index, const i32 delta_x, const i32 delta_y
    ) override {
        collision_calls.push_back({role_index, delta_x, delta_y});
        if (next_collision < collision_results.size()) {
            return collision_results[next_collision++];
        }
        return {};
    }

    u32 query_internal_flag(const u32 bit_index) override {
        flag_queries.push_back(bit_index);
        return flag_result;
    }

    u32 update_action(LegacyActionRecord& action) override {
        action_updates.push_back({
            action.base_variant,
            action.variant_delta,
            action.one_shot_base_variant,
            action.one_shot_variant_delta,
            action.wait_remaining,
        });
        const std::size_t index = action_updates.size() - 1U;
        return index < update_results.size() ? update_results[index] : 1U;
    }

    std::vector<LegacyMovementCollisionResult> collision_results;
    std::vector<u32> update_results;
    std::vector<CollisionCall> collision_calls;
    std::vector<u32> flag_queries;
    std::vector<ActionSnapshot> action_updates;
    std::size_t next_collision{};
    u32 flag_result{};
};

LegacyMovementCollisionResult collision(
    const u32 event_code, const u32 role_index = kLegacyMovementCollisionNoRole
) {
    return {
        .status = LegacyMovementCollisionStatus::completed,
        .event_code = event_code,
        .hit_role_index = role_index,
    };
}

LegacyWorldRoleRecord make_player() {
    LegacyWorldRoleRecord player{};
    player.world_x = 1000U;
    player.world_y = 2000U;
    player.action.field_2c = 0U;
    player.action.field_30 = 0U;
    player.action.variant_delta = 0U;
    return player;
}

void test_collision_fallback_order(openswd3::test::Context& test) {
    std::vector roles{make_player(), LegacyWorldRoleRecord{}};
    const std::vector events{
        LegacyWorldMapEvent{
            .field_04 = 9U,
            .field_08 = 3U,
            .field_0c = 0U,
            .field_10 = 0U,
            .name_bytes_with_terminator = {},
        },
    };
    LegacyWorldTalkContext talk{};
    talk.source_guid = 7U;
    u32 latch = 4U;
    RecordingPorts ports;
    ports.collision_results = {
        collision(0U, 1U),
        collision(9U),
    };

    const auto result = coordinate_legacy_world_collision_talk(
        {0U, 1, -1, 1, 0}, roles, events, talk, latch, ports
    );
    test.expect_equal(
        ports.collision_calls,
        std::vector<CollisionCall>{{0U, 1, -1}, {0U, 1, 0}},
        "zero return retries the original direction even after a role hit"
    );
    test.expect_equal(result.collision_query_count, 2U, "two queries recorded");
    test.expect_equal(
        result.event_code, 9U, "second query overwrites the event"
    );
    test.expect_equal(
        result.hit_role_index,
        kLegacyMovementCollisionNoRole,
        "second query overwrites the first role index"
    );

    RecordingPorts nonzero_ports;
    nonzero_ports.collision_results = {collision(9U)};
    const auto nonzero = coordinate_legacy_world_collision_talk(
        {0U, -1, 1, 1, 0}, roles, events, talk, latch, nonzero_ports
    );
    test.expect_equal(
        nonzero_ports.collision_calls,
        std::vector<CollisionCall>{{0U, -1, 1}},
        "nonzero adjusted event skips the original direction"
    );
    test.expect_equal(nonzero.collision_query_count, 1U, "one query recorded");

    RecordingPorts still_ports;
    const auto still = coordinate_legacy_world_collision_talk(
        {0U, 0, 0, 0, 0}, roles, events, talk, latch, still_ports
    );
    test.expect_true(
        still_ports.collision_calls.empty(), "zero motion queries nothing"
    );
    test.expect_equal(
        still.event_code, 0U, "zero motion retains zero event state"
    );
}

void test_map_event_context_and_gate_order(openswd3::test::Context& test) {
    std::vector roles{make_player()};
    roles[0].action.variant_delta = 0U;
    const std::vector events{
        LegacyWorldMapEvent{
            .field_04 = 0x12U,
            .field_08 = 0x1234ABCDU,
            .field_0c = 0x002A0000U,
            .field_10 = 0U,
            .name_bytes_with_terminator = {},
        },
    };
    LegacyWorldTalkContext talk{};
    talk.source_guid = kLegacyWorldTalkIdleSource;
    talk.field_18 = 0xCAFEBABEU;
    u32 latch = 0x11223344U;
    RecordingPorts ports;
    ports.collision_results = {collision(0x12U)};
    ports.flag_result = 1U;

    const auto result = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 1, 0}, roles, events, talk, latch, ports
    );
    test.expect_equal(
        result.status,
        LegacyWorldCollisionTalkStatus::completed,
        "map event completes"
    );
    test.expect_equal(
        result.talk_source,
        LegacyWorldTalkSource::map_event,
        "map event creates its Talk source"
    );
    test.expect_true(
        result.map_event_stopped_motion,
        "flag result exactly one stops movement"
    );
    test.expect_equal(result.delta_x, 0, "map event clears adjusted X");
    test.expect_equal(result.delta_y, 0, "map event clears adjusted Y");
    test.expect_equal(
        ports.flag_queries,
        std::vector<u32>{42U},
        "event high word selects the internal flag"
    );
    test.expect_equal(
        talk.talk_data_offset, 0U, "map event clears data offset"
    );
    test.expect_equal(
        talk.instruction_offset, u16{0U}, "map event clears instruction offset"
    );
    test.expect_equal(
        talk.talk_script_id,
        u16{0xABCDU},
        "map event copies low event Talk word"
    );
    test.expect_equal(
        talk.source_guid,
        kLegacyWorldTalkMapEventSource,
        "map event writes FFFD source sentinel"
    );
    test.expect_equal(talk.source_flags, 0U, "map event clears source flags");
    test.expect_equal(
        talk.world_x, 936U, "direction table subtracts X times 16"
    );
    test.expect_equal(
        talk.world_y, 1936U, "direction table subtracts Y times 16"
    );
    test.expect_equal(
        talk.field_18,
        0xCAFEBABEU,
        "unwritten Talk fields retain their prior bytes"
    );
    test.expect_equal(
        latch,
        0x11223344U,
        "map event does not clear the role interaction latch"
    );

    LegacyWorldTalkContext busy{};
    busy.source_guid = 99U;
    RecordingPorts busy_ports;
    busy_ports.collision_results = {collision(0x12U)};
    busy_ports.flag_result = 1U;
    const auto busy_result = coordinate_legacy_world_collision_talk(
        {0U, -1, 1, -1, 1}, roles, events, busy, latch, busy_ports
    );
    test.expect_true(
        busy_result.map_event_stopped_motion,
        "map flag side effect precedes the occupied Talk gate"
    );
    test.expect_equal(
        busy_result.talk_source,
        LegacyWorldTalkSource::none,
        "occupied Talk prevents context replacement"
    );
    test.expect_equal(
        busy.source_guid, u16{99U}, "occupied context is unchanged"
    );

    LegacyWorldTalkContext not_exact{};
    not_exact.source_guid = 4U;
    RecordingPorts not_exact_ports;
    not_exact_ports.collision_results = {collision(0x12U)};
    not_exact_ports.flag_result = 2U;
    const auto not_exact_result = coordinate_legacy_world_collision_talk(
        {0U, 1, 1, 1, 1}, roles, events, not_exact, latch, not_exact_ports
    );
    test.expect_false(
        not_exact_result.map_event_stopped_motion,
        "flag result two does not satisfy cmp eax,1"
    );
}

void test_all_map_event_direction_offsets(openswd3::test::Context& test) {
    struct ExpectedPosition {
        u32 x;
        u32 y;
    };
    constexpr std::array expected{
        ExpectedPosition{936U, 1936U},
        ExpectedPosition{1000U, 1936U},
        ExpectedPosition{1064U, 1936U},
        ExpectedPosition{1064U, 2000U},
        ExpectedPosition{1064U, 2064U},
        ExpectedPosition{1000U, 2064U},
        ExpectedPosition{936U, 2064U},
        ExpectedPosition{936U, 2000U},
    };
    const std::vector events{
        LegacyWorldMapEvent{
            .field_04 = 1U,
            .field_08 = 2U,
            .field_0c = 0U,
            .field_10 = 0U,
            .name_bytes_with_terminator = {},
        },
    };

    for (u32 direction = 0U; direction < expected.size(); ++direction) {
        std::vector roles{make_player()};
        roles[0].action.variant_delta = direction;
        LegacyWorldTalkContext talk{};
        talk.source_guid = kLegacyWorldTalkIdleSource;
        u32 latch{};
        RecordingPorts ports;
        ports.collision_results = {collision(1U)};
        const auto result = coordinate_legacy_world_collision_talk(
            {0U, 1, 0, 0, 0}, roles, events, talk, latch, ports
        );
        test.expect_equal(
            result.talk_source,
            LegacyWorldTalkSource::map_event,
            "each valid direction builds map Talk"
        );
        test.expect_equal(
            talk.world_x,
            expected[direction].x,
            "map Talk X table matches .rdata 0x00499380"
        );
        test.expect_equal(
            talk.world_y,
            expected[direction].y,
            "map Talk Y table matches .rdata 0x004993A0"
        );
    }
}

void test_role_talk_and_action_updates(openswd3::test::Context& test) {
    std::vector roles{make_player(), LegacyWorldRoleRecord{}};
    LegacyWorldRoleRecord& player = roles[0];
    player.action.base_variant = 5U;
    player.action.variant_delta = 7U;
    player.action.wait_remaining = 9U;

    LegacyWorldRoleRecord& target = roles[1];
    target.world_x = 1000U;
    target.world_y = 1900U;
    target.flags = kLegacyWorldTalkTurningRoleFlag | 0x40000000U;
    target.talk_data_offset = 0x10203040U;
    target.talk_script_id = 0x3456U;
    target.talk_initial_offset = 0x789AU;
    target.guid = 0x2468U;
    target.action.base_variant = 9U;
    target.action.variant_delta = 6U;
    target.action.one_shot_base_variant = 2U;
    target.action.one_shot_variant_delta = 3U;
    target.action.wait_remaining = 8U;

    LegacyWorldTalkContext talk{};
    talk.source_guid = kLegacyWorldTalkIdleSource;
    talk.world_x = 0xAAAAAAAAU;
    talk.world_y = 0xBBBBBBBBU;
    u32 latch = 0xDEADBEEFU;
    RecordingPorts ports;
    ports.collision_results = {collision(0U), collision(0U, 1U)};
    ports.update_results = {0U, 0U};

    const auto result = coordinate_legacy_world_collision_talk(
        {0U, 1, -1, 1, 0}, roles, {}, talk, latch, ports
    );
    test.expect_equal(
        result.talk_source,
        LegacyWorldTalkSource::role,
        "role collision creates role Talk context"
    );
    test.expect_equal(
        result.event_code, 0x3456U, "role Talk word replaces collision return"
    );
    test.expect_equal(
        ports.action_updates,
        std::vector<ActionSnapshot>{
            {0U, 1U, 9U, 6U, 0U},
            {0U, 1U, 9U, 6U, 0U},
        },
        "both refresh calls receive the target action pointer"
    );
    test.expect_true(
        result.target_action_update_failed,
        "zero target update return is retained for diagnostics"
    );
    test.expect_true(
        result.post_player_turn_target_update_failed,
        "second target update failure is retained for diagnostics"
    );
    test.expect_equal(
        player.action.base_variant,
        0U,
        "player base variant is still cleared before second refresh"
    );
    test.expect_equal(
        player.action.variant_delta,
        0U,
        "player receives the exact opposite direction"
    );
    test.expect_equal(
        player.action.wait_remaining,
        u16{0U},
        "player wait is cleared without refreshing its own action"
    );
    test.expect_equal(
        talk.talk_data_offset, 0x10203040U, "role copies Talk data offset"
    );
    test.expect_equal(
        talk.instruction_offset,
        u16{0x789AU},
        "role copies initial Talk instruction offset"
    );
    test.expect_equal(talk.source_guid, u16{0x2468U}, "role copies GUID");
    test.expect_equal(talk.source_flags, target.flags, "role copies flags");
    test.expect_equal(
        talk.talk_script_id, u16{0x3456U}, "role copies Talk script id"
    );
    test.expect_equal(
        talk.world_x, 0xAAAAAAAAU, "collision role path leaves Talk X stale"
    );
    test.expect_equal(
        talk.world_y, 0xBBBBBBBBU, "collision role path leaves Talk Y stale"
    );
    test.expect_equal(latch, 0U, "role Talk clears one-shot interaction state");
}

void test_role_gates_and_checked_failures(openswd3::test::Context& test) {
    std::vector roles{make_player(), LegacyWorldRoleRecord{}};
    roles[1].talk_script_id = 9U;
    roles[1].action.base_variant = 6U;
    LegacyWorldTalkContext busy{};
    busy.source_guid = 3U;
    u32 latch = 8U;
    RecordingPorts busy_ports;
    busy_ports.collision_results = {collision(0U, 1U)};
    const auto busy_result = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 0, 0}, roles, {}, busy, latch, busy_ports
    );
    test.expect_equal(
        busy_result.talk_source,
        LegacyWorldTalkSource::none,
        "occupied Talk blocks role turning and copy"
    );
    test.expect_true(
        busy_ports.action_updates.empty(),
        "occupied Talk performs no action update"
    );
    test.expect_equal(latch, 8U, "occupied Talk does not clear role latch");

    LegacyWorldTalkContext idle{};
    idle.source_guid = kLegacyWorldTalkIdleSource;
    RecordingPorts missing_event_ports;
    missing_event_ports.collision_results = {collision(77U)};
    const auto missing = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 0, 0}, roles, {}, idle, latch, missing_event_ports
    );
    test.expect_equal(
        missing.status,
        LegacyWorldCollisionTalkStatus::missing_map_event,
        "modern boundary exposes the original null-dereference path"
    );

    RecordingPorts invalid_role_ports;
    invalid_role_ports.collision_results = {collision(0U, 99U)};
    const auto invalid_role = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 0, 0}, roles, {}, idle, latch, invalid_role_ports
    );
    test.expect_equal(
        invalid_role.status,
        LegacyWorldCollisionTalkStatus::invalid_hit_role_index,
        "modern boundary exposes an invalid hit role index"
    );

    RecordingPorts invalid_player_ports;
    const auto invalid_player = coordinate_legacy_world_collision_talk(
        {9U, 1, 0, 0, 0}, roles, {}, idle, latch, invalid_player_ports
    );
    test.expect_equal(
        invalid_player.status,
        LegacyWorldCollisionTalkStatus::invalid_player_index,
        "modern boundary exposes an invalid player index"
    );
    test.expect_true(
        invalid_player_ports.collision_calls.empty(),
        "invalid player is rejected before collision ports"
    );

    RecordingPorts failed_query_ports;
    failed_query_ports.collision_results = {{
        .status = LegacyMovementCollisionStatus::invalid_map_cell,
        .event_code = 0x55U,
    }};
    const auto failed_query = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 0, 0}, roles, {}, idle, latch, failed_query_ports
    );
    test.expect_equal(
        failed_query.status,
        LegacyWorldCollisionTalkStatus::collision_query_failed,
        "checked collision failure does not masquerade as Talk"
    );

    std::vector unflagged_roles{make_player(), LegacyWorldRoleRecord{}};
    unflagged_roles[1].world_x = 1000U;
    unflagged_roles[1].world_y = 1900U;
    unflagged_roles[1].talk_script_id = 6U;
    unflagged_roles[1].action.base_variant = 12U;
    unflagged_roles[1].action.variant_delta = 5U;
    LegacyWorldTalkContext unflagged_talk{};
    unflagged_talk.source_guid = kLegacyWorldTalkIdleSource;
    RecordingPorts unflagged_ports;
    unflagged_ports.collision_results = {collision(0U, 1U)};
    const auto unflagged = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 0, 0},
        unflagged_roles,
        {},
        unflagged_talk,
        latch,
        unflagged_ports
    );
    test.expect_equal(
        unflagged.talk_source,
        LegacyWorldTalkSource::role,
        "unflagged role still creates Talk"
    );
    test.expect_equal(
        unflagged_ports.action_updates,
        std::vector<ActionSnapshot>{{12U, 5U, 0U, 0U, 0U}},
        "without bit 0x800 only the post-player-turn target refresh remains"
    );
    test.expect_equal(
        unflagged_roles[0].action.variant_delta,
        0U,
        "unflagged target still turns the player oppositely"
    );
}

void test_map_direction_boundary(openswd3::test::Context& test) {
    std::vector roles{make_player()};
    roles[0].action.variant_delta = 8U;
    const std::vector events{
        LegacyWorldMapEvent{
            .field_04 = 3U,
            .field_08 = 4U,
            .field_0c = 0U,
            .field_10 = 0U,
            .name_bytes_with_terminator = {},
        },
    };
    LegacyWorldTalkContext talk{};
    talk.source_guid = kLegacyWorldTalkIdleSource;
    u32 latch{};
    RecordingPorts ports;
    ports.collision_results = {collision(3U)};
    const auto result = coordinate_legacy_world_collision_talk(
        {0U, 1, 0, 0, 0}, roles, events, talk, latch, ports
    );
    test.expect_equal(
        result.status,
        LegacyWorldCollisionTalkStatus::invalid_player_direction,
        "checked boundary rejects original direction-table overread"
    );
    test.expect_equal(
        talk.source_guid,
        kLegacyWorldTalkIdleSource,
        "invalid direction does not partially build Talk context"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_collision_fallback_order(test);
    test_map_event_context_and_gate_order(test);
    test_all_map_event_direction_offsets(test);
    test_role_talk_and_action_updates(test);
    test_role_gates_and_checked_failures(test);
    test_map_direction_boundary(test);
    return test.exit_code();
}
