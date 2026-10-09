#include "test.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_group_a_startup_reset.hpp"
#include "openswd3/battle/legacy_battle_group_a_storage.hpp"
#include "openswd3/battle/legacy_battle_group_b_storage.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_runtime_shutdown.hpp"

#include <algorithm>
#include <bit>
#include <functional>
#include <memory>
#include <vector>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u32;

class Heap final : public LegacyBattleActorStartupResetHeapPort {
public:
    std::optional<u32> read_linked_action_next(u32) override {
        return std::nullopt;
    }

    std::optional<LegacyBattleActorStartupResetRegisters>
    release_heap_block(const u32 token) override {
        released.push_back(token);
        if (fail_release) {
            return std::nullopt;
        }

        return LegacyBattleActorStartupResetRegisters{.edx = 0xABCDEF01U};
    }

    bool fail_release{};
    std::vector<u32> released;
};

struct Fixture {
    std::unique_ptr<LegacyBattleStartupState> startup{
        std::make_unique<LegacyBattleStartupState>()
    };
    LegacyBattleActionDispatchState action;
    LegacyBattleFinalActorStepState final_actor;
    Heap heap;

    Fixture() {
        auto& party = startup->party[0U];
        party.configuration.actor_record_token = 0x71000000U;
        party.configuration.actor_record.fill(0xA5A5A5A5U);
        party.secondary_resource_token = 0x72000000U;
        party.base_resource_definition.fill(0x44U);
        party.attribute_aggregation.embedded_profile_application.status_bits =
            0xFFFFFFFFU;
        party.final_processing.profile_buffer.fill(0xA5A5A5A5U);
        party.startup_reset.bytes_283c_2953.fill(std::byte{0xA5});
        party.startup_reset.bytes_295a_299f.fill(std::byte{0xA5});
        party.startup_reset.field_2ae8 = 12U;
        action.group_a_action_execution[0U].action_target = 0xCAFEU;
        action.group_a_action_execution[0U].start_gate = 16U;
        action.group_a_action_execution[0U].effect_resource_slots.fill(0xFFFFU);
        action.group_a_target_phases[0U].group_a_mode_flags = 0xFFU;
        action.group_a_target_phases[0U].tick = 17U;
        action.group_a_target_phases[0U].active_gate = 19U;
        final_actor.group_a_availability_blocks[0U].value = 23U;
        final_actor.group_a_availability_blocks[1U].value = 29U;
    }

    LegacyBattleActorStartupResetResult reset() {
        return reset_legacy_battle_group_a_for_startup(
            *startup, action, final_actor, heap, {.actor_index = 0U}
        );
    }
};

class RegistryShutdown final : public LegacyBattleRenderAuxiliaryBufferReleaser,
                               public LegacyBattleGroupAResourceReleasePort,
                               public LegacyBattleGroupBResourceReleasePort {
public:
    RegistryShutdown(
        LegacyBattleGroupAStorage& party, LegacyBattleGroupBStorage& enemies
    )
        : party_(party), enemies_(enemies) {}

    void release(u32) noexcept override {}

    LegacyBattleGroupAResourceReleaseCallReply release_group_a_resource(
        const LegacyBattleGroupAResourceReleaseCallRequest& request
    ) override {
        const auto result =
            party_.release_heap_block(request.resource_token).value();
        released.push_back(request.resource_token);
        return {.eax = result.eax, .ecx = result.ecx, .edx = result.edx};
    }

    LegacyBattleGroupBResourceReleaseCallReply release_group_b_resource(
        const LegacyBattleGroupBResourceReleaseCallRequest& request
    ) override {
        releases_precede_clear = releases_precede_clear &&
            (*enemies_.actors())[request.actor_index].resource_token ==
                request.resource_token &&
            !enemies_.resource_bytes(request.resource_token).empty();
        const auto result =
            enemies_.release_heap_block(request.resource_token).value();
        released.push_back(request.resource_token);
        return {.eax = result.eax, .ecx = result.ecx, .edx = result.edx};
    }

    LegacyBattleGroupAStorage& party_;
    LegacyBattleGroupBStorage& enemies_;
    std::vector<u32> released;
    bool releases_precede_clear{true};
};

class Diagnostic final : public LegacyBattleGroupAConfigurationDiagnosticPort {
public:
    LegacyBattleGroupAConfigurationDiagnosticReply report_missing_placement(
        const LegacyBattleGroupAConfigurationDiagnosticRequest&
    ) override {
        ++calls;
        if (on_report) {
            on_report();
        }

        return {};
    }

    std::function<void()> on_report;
    u32 calls{};
};

void test_initial_party_binding(openswd3::test::Context& test) {
    for (const u32 variant : {0U, 1U, 2U, 3U}) {
        const u32 mirror = variant % 3U;
        const bool counted_mode = variant != 3U;
        Fixture fixture;
        LegacyBattleGroupAStorage storage{*fixture.startup, fixture.action};
        test.expect_true(
            storage.construct(), "construct initial-party backing"
        );
        auto& party = fixture.startup->party[0U];
        const auto token = party.configuration.actor_record_token;
        std::array<std::byte, 0x38> source{};
        source[0x25U] = counted_mode ? std::byte{0x80} : std::byte{};
        fixture.startup->group_a_configuration_sources[2U] = source;
        fixture.startup->action_mode_source.actor_label_indices[0U] = 2U;
        fixture.startup->mirror_mode = mirror;
        fixture.startup->party_actor_mode_count = 255U;
        party.role_id = 7U;
        party.active = 1U;
        party.placement_position_x = 0xFFFAU;
        party.placement_position_y = 300U;
        fixture.startup->party_offsets[0U] =
            std::bit_cast<openswd3::compat::i32>(0x80000001U);
        Diagnostic diagnostic;
        const auto status =
            storage.initialize_party(0U, fixture.final_actor, diagnostic, 0U);
        const auto expected_x = mirror == 1U ? 646U : 0xFFFAU;
        const auto expected_anchor = mirror == 1U ? 0x8000026FU : 0x80000001U;
        test.expect_true(
            status == LegacyBattleGroupAStartupBindingStatus::completed &&
                party.configuration.actor_record_token == token &&
                party.configuration.source_record_token == 0x004AB800U &&
                party.configuration.auxiliary_record_token == 0x004AD010U &&
                party.position_x == expected_x &&
                party.alternate_position_x == expected_x &&
                party.position_y == 300U && party.identity_word == 7U &&
                fixture.action.group_a_action_execution[0U].position_x ==
                    expected_x &&
                party.placement_position_x == expected_x &&
                std::bit_cast<u32>(fixture.startup->party_offsets[0U]) ==
                    expected_anchor &&
                fixture.action.group_a_target_phases[0U].render_toggle_gate ==
                    (mirror == 1U ? 1U : 0U) &&
                fixture.startup->party_actor_mode_count ==
                    (counted_mode ? 0U : 255U) &&
                diagnostic.calls == 0U,
            "initial party binds the compact source after reset, exact-one mirror and byte count wrap"
        );
        fixture.action.group_a_target_phases[0U].tick = 19U;
        fixture.final_actor.group_a_availability_blocks[0U].value = 29U;
        reset_legacy_battle_dispatch_preserving_actors(
            fixture.action, fixture.final_actor
        );
        test.expect_true(
            fixture.action.group_a_target_phases[0U].tick == 19U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    29U,
            "global reset leaves party physical fields for the actual actor reset"
        );
        party.placement_position_x = 300U;
        party.final_processing.replacement_action_kind = 13U;
        fixture.action.group_a_target_phases[0U]
            .spawn_action_records[4U]
            .action_id = 17U;
        test.expect_true(
            storage.initialize_party(0U, fixture.final_actor, diagnostic, 0U) ==
                    LegacyBattleGroupAStartupBindingStatus::completed &&
                party.configuration.actor_record_token == token &&
                fixture.action.group_a_target_phases[0U].tick == 19U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    0U &&
                party.position_x == (mirror == 1U ? 340U : 300U) &&
                party.final_processing.replacement_action_kind == 0U &&
                fixture.action.group_a_target_phases[0U]
                        .spawn_action_records[4U]
                        .action_id == 0U &&
                fixture.startup->party_actor_mode_count ==
                    (counted_mode ? 1U : 255U),
            "repeated entry reuses the primary allocation and only counts actors whose mode query returns one"
        );
    }

    {
        Fixture fixture;
        LegacyBattleGroupAStorage storage{*fixture.startup, fixture.action};
        test.expect_true(
            storage.construct(), "construct diagnostic party backing"
        );
        auto& party = fixture.startup->party[0U];
        std::array<std::byte, 0x38> source{};
        fixture.startup->group_a_configuration_sources[0U] = source;
        party.placement_position_x = 527U;
        party.placement_position_y = 287U;
        party.active = 1U;
        party.final_processing.replacement_action_kind = 13U;
        Diagnostic diagnostic;
        bool observed{};
        diagnostic.on_report = [&] {
            observed = party.final_processing.replacement_action_kind == 0U &&
                party.position_x == 527U &&
                party.alternate_position_y == 287U &&
                fixture.action.group_a_action_execution[0U].position_x == 527U;
            party.position_x = 71U;
            party.progress.mode_gate = 0x2000U;
        };
        test.expect_true(
            storage.initialize_party(0U, fixture.final_actor, diagnostic, 0U) ==
                    LegacyBattleGroupAStartupBindingStatus::completed &&
                observed && party.position_x == 71U && diagnostic.calls == 1U &&
                fixture.startup->party_actor_mode_count == 1U,
            "diagnostics see already-published coordinates and subsequent mode query reads live callback changes"
        );
    }

    for (const bool invalid_source : {false, true}) {
        Fixture fixture;
        LegacyBattleGroupAStorage storage{*fixture.startup, fixture.action};
        test.expect_true(
            storage.construct(), "construct source/mode failure backing"
        );
        auto& party = fixture.startup->party[0U];
        std::array<std::byte, 0x38> source{};
        source[0x25U] = std::byte{0x80};
        fixture.startup->group_a_configuration_sources[0U] = source;
        fixture.startup->action_mode_source.actor_label_indices[0U] =
            invalid_source ? 4U : 0U;
        fixture.startup->party_actor_mode_count = 9U;
        fixture.startup->mirror_mode = 1U;
        party.role_id = 7U;
        party.placement_position_x = 500U;
        party.progress.special_ready_read_accessible = invalid_source;
        party.final_processing.replacement_action_kind = 13U;
        Diagnostic diagnostic;
        test.expect_true(
            storage.initialize_party(0U, fixture.final_actor, diagnostic, 0U) ==
                    (invalid_source ? LegacyBattleGroupAStartupBindingStatus::
                                          source_index_typed_stop
                                    : LegacyBattleGroupAStartupBindingStatus::
                                          mode_read_typed_stop) &&
                party.placement_position_x == 140U &&
                party.position_x == 140U &&
                party.final_processing.replacement_action_kind == 0U &&
                fixture.startup->party_actor_mode_count == 9U,
            "invalid compact source retains workspace and placement stores while unreadable mode stops after configuration without incrementing"
        );
    }

    for (const bool reset_failure : {false, true}) {
        Fixture fixture;
        LegacyBattleGroupAStorage storage{*fixture.startup, fixture.action};
        test.expect_true(
            storage.construct(), "construct failure-prefix party backing"
        );
        auto& party = fixture.startup->party[0U];
        party.role_id = 7U;
        party.placement_position_x = 500U;
        party.placement_position_y = 300U;
        fixture.startup->mirror_mode = 1U;
        fixture.startup->party_actor_mode_count = 9U;
        if (reset_failure) {
            party.configuration.source_runtime_value = 1U;
            party.configuration.profile_token = 0xDEADC0DEU;
        }

        Diagnostic diagnostic;
        const auto status =
            storage.initialize_party(0U, fixture.final_actor, diagnostic, 0U);
        test.expect_true(
            status ==
                    (reset_failure ? LegacyBattleGroupAStartupBindingStatus::
                                         actor_reset_typed_stop
                                   : LegacyBattleGroupAStartupBindingStatus::
                                         configuration_typed_stop) &&
                party.placement_position_x == (reset_failure ? 500U : 140U) &&
                party.position_x == (reset_failure ? 0U : 140U) &&
                fixture.startup->party_actor_mode_count == 9U &&
                diagnostic.calls == 0U,
            "reset failure stops before mirror, whereas missing primary source retains completed placement stores"
        );
    }
}

}  // namespace

void test_battle_group_a_startup_reset(openswd3::test::Context& test) {
    test_initial_party_binding(test);

    {
        Fixture fixture;
        LegacyBattleGroupAStorage storage{*fixture.startup, fixture.action};
        test.expect_true(
            storage.construct(), "construct primary records for shutdown"
        );
        fixture.startup->party[0U].secondary_resource_token = 0U;
        std::array<u32, 10> tokens{};
        for (std::size_t index = 0U; index < tokens.size(); ++index) {
            tokens[index] =
                fixture.startup->party[index].configuration.actor_record_token;
        }

        LegacyBattleGroupBStorage enemies;
        test.expect_true(
            enemies.construct(), "construct enemy records for shutdown"
        );
        fixture.startup->group_b_lifecycle = enemies.actors();
        std::array<u32, 8> enemy_tokens{};
        std::vector<u32> expected_releases(tokens.begin(), tokens.end());
        for (std::size_t index = 0U; index < enemy_tokens.size(); ++index) {
            enemy_tokens[index] = (*enemies.actors())[index].resource_token;
            expected_releases.push_back(enemy_tokens[index]);
            test.expect_true(
                !enemies.resource_bytes(enemy_tokens[index]).empty() &&
                    enemies.read_linked_action_next(enemy_tokens[index])
                        .has_value(),
                "enemy allocation is accessible before shutdown"
            );
        }

        RegistryShutdown port{storage, enemies};
        const auto result =
            shutdown_legacy_battle_runtime(*fixture.startup, port, port, port);
        test.expect_true(
            result.status == LegacyBattleRuntimeShutdownStatus::completed &&
                port.released == expected_releases &&
                port.releases_precede_clear &&
                std::ranges::all_of(
                    tokens,
                    [&](const auto token) {
                        return storage.record_bytes(token).empty();
                    }
                ) &&
                std::ranges::all_of(
                    fixture.startup->party,
                    [](const auto& party) {
                        return party.configuration.actor_record_token == 0U;
                    }
                ),
            "shutdown retires party allocations before enemy allocations and clears canonical party tokens"
        );
        test.expect_true(
            std::ranges::all_of(
                enemy_tokens,
                [&](const auto token) {
                    return enemies.resource_bytes(token).empty() &&
                        !enemies.read_linked_action_next(token).has_value() &&
                        !enemies.release_heap_block(token).has_value();
                }
            ) &&
                std::ranges::all_of(
                    *enemies.actors(),
                    [](const auto& actor) { return actor.resource_token == 0U; }
                ),
            "enemy records become unreadable and cannot be freed twice after shutdown"
        );
        const auto repeated =
            shutdown_legacy_battle_runtime(*fixture.startup, port, port, port);
        test.expect_true(
            repeated.status == LegacyBattleRuntimeShutdownStatus::completed &&
                port.released == expected_releases,
            "repeated shutdown does not release retired allocations again"
        );
    }

    {
        Fixture fixture;
        LegacyBattleGroupAStorage storage{*fixture.startup, fixture.action};
        test.expect_true(
            storage.construct(), "construct all ten persistent party actors"
        );
        u32 previous_end{};
        for (const auto& party : fixture.startup->party) {
            const auto token = party.configuration.actor_record_token;
            const auto bytes = storage.record_bytes(token);
            test.expect_true(
                token != 0U && token >= previous_end && bytes.size() == 0x38U &&
                    std::ranges::all_of(
                        bytes, [](const auto value) { return value == 0U; }
                    ),
                "party constructor reserves distinct complete primary records"
            );
            previous_end = token + 0x38U;
        }

        auto& party = fixture.startup->party[0U];
        const auto token = party.configuration.actor_record_token;
        const auto bytes = storage.record_bytes(token);
        bytes[0U] = 0x5AU;
        test.expect_true(
            party.configuration.actor_record[0U] == 0x5AU &&
                storage.construct() &&
                party.configuration.actor_record_token == token &&
                bytes[0U] == 0x5AU &&
                fixture.action.group_a_target_phases[0U].tick == 0U,
            "registered records borrow the configuration owner and repeated entry does not reconstruct actors"
        );
        test.expect_true(
            storage.read_linked_action_next(token) ==
                    std::optional<u32>{0x5AU} &&
                !storage.read_linked_action_next(token + 0x35U).has_value() &&
                storage.release_heap_block(token).has_value() &&
                storage.record_bytes(token).empty() &&
                !storage.release_heap_block(token).has_value(),
            "allocation lookup observes shared bytes and invalidates released records"
        );
    }

    {
        Fixture fixture;
        const auto result = fixture.reset();
        const auto& party = fixture.startup->party[0U];
        test.expect_true(
            result.status == LegacyBattleActorStartupResetStatus::completed &&
                fixture.heap.released.empty() &&
                party.configuration.actor_record_token == 0x71000000U &&
                party.secondary_resource_token == 0x72000000U &&
                std::ranges::all_of(
                    party.configuration.actor_record,
                    [](const u32 value) { return value == 0xA5A5A5A5U; }
                ) &&
                std::ranges::all_of(
                    party.base_resource_definition,
                    [](const auto value) { return value == 0x44U; }
                ) &&
                std::ranges::all_of(
                    party.startup_reset.bytes_283c_2953,
                    [](const auto value) { return value == std::byte{}; }
                ) &&
                std::ranges::all_of(
                    party.startup_reset.bytes_295a_299f,
                    [](const auto value) { return value == std::byte{}; }
                ) &&
                party.attribute_aggregation.embedded_profile_application
                        .status_bits == 0U &&
                fixture.action.group_a_action_execution[0U].action_target ==
                    0xFFFFU &&
                fixture.action.group_a_action_execution[0U].start_gate == 0U &&
                fixture.action.group_a_target_phases[0U].group_a_mode_flags ==
                    0U &&
                fixture.action.group_a_target_phases[0U].tick == 17U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    0U &&
                fixture.final_actor.group_a_availability_blocks[1U].value ==
                    29U,
            "party startup reset writes shared actor owners while retaining primary records and constructor-only fields"
        );
    }

    for (const bool fail : {false, true}) {
        Fixture fixture;
        auto& party = fixture.startup->party[0U];
        party.configuration.source_runtime_value = 1U;
        party.configuration.profile_token = 0x73000000U;
        fixture.heap.fail_release = fail;
        const auto result = fixture.reset();
        test.expect_true(
            fixture.heap.released == std::vector<u32>{0x73000000U} &&
                party.configuration.actor_record_token == 0x71000000U &&
                std::ranges::all_of(
                    party.final_processing.profile_buffer,
                    [](const u32 value) { return value == 0U; }
                ) &&
                result.status ==
                    (fail ? LegacyBattleActorStartupResetStatus::
                                heap_release_typed_stop
                          : LegacyBattleActorStartupResetStatus::completed) &&
                party.configuration.profile_token ==
                    (fail ? 0x73000000U : 0U) &&
                party.configuration.source_runtime_value == (fail ? 1U : 0U) &&
                fixture.action.group_a_action_execution[0U].start_gate ==
                    (fail ? 16U : 0U) &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    (fail ? 23U : 0U),
            "profile release uses the separate dynamic pointer and failure retains the reset prefix"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.group_a_availability_blocks[0U].write_accessible =
            false;
        const auto result = fixture.reset();
        test.expect_true(
            result.status ==
                    LegacyBattleActorStartupResetStatus::
                        actor_write_typed_stop &&
                result.stopped_instruction == 0x0047D5DBU &&
                result.stopped_offset_or_token == 0x2AE4U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    23U &&
                fixture.startup->party[0U].startup_reset.field_2ae8 == 12U &&
                fixture.action.group_a_target_phases[0U].active_gate == 19U &&
                fixture.action.group_a_action_execution[0U].start_gate == 0U,
            "an unavailable shared availability field stops before later actor stores"
        );
    }

    {
        Fixture fixture;
        fixture.startup->party[0U]
            .configuration.source_runtime_value_read_accessible = false;
        const auto result = fixture.reset();
        test.expect_true(
            result.status ==
                    LegacyBattleActorStartupResetStatus::
                        actor_read_typed_stop &&
                result.stopped_instruction == 0x0047D422U &&
                result.stopped_offset_or_token == 0x2AA0U &&
                fixture.heap.released.empty() &&
                fixture.action.group_a_action_execution[0U].start_gate == 16U &&
                fixture.final_actor.group_a_availability_blocks[0U].value ==
                    23U,
            "missing activity-field access preserves the completed REP prefix"
        );
    }
}
