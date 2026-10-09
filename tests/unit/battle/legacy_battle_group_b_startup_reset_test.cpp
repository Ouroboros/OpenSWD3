#include "test.hpp"
#include "legacy_battle_mon_database_fixture.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_group_b_startup_reset.hpp"
#include "openswd3/battle/legacy_battle_group_b_storage.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_mon_file_runtime.hpp"
#include "openswd3/battle/legacy_battle_mon_stream_runtime.hpp"
#include "openswd3/battle/legacy_battle_mon_text_runtime.hpp"
#include "openswd3/battle/legacy_battle_setup.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_reward_scale.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <stdexcept>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using namespace openswd3::battle;

class ObservingHeap final : public LegacyBattleActorStartupResetHeapPort {
public:
    ObservingHeap(
        LegacyBattleActorGroupBElementState& actor,
        LegacyBattleActorProgressState& progress
    )
        : actor_(actor), progress_(progress) {}

    std::optional<u32> read_linked_action_next(u32) override {
        return std::nullopt;
    }

    std::optional<LegacyBattleActorStartupResetRegisters>
    release_heap_block(const u32 token) override {
        ++calls;
        released_token = token;
        target_at_release = actor_.action_execution.action_target;
        mode_at_release = static_cast<u16>(progress_.mode_gate);
        start_gate_at_release = actor_.action_execution.start_gate;
        if (fail) {
            return std::nullopt;
        }

        return LegacyBattleActorStartupResetRegisters{.edx = 0xABCD1234U};
    }

    LegacyBattleActorGroupBElementState& actor_;
    LegacyBattleActorProgressState& progress_;
    bool fail{};
    u32 calls{};
    u32 released_token{};
    u16 target_at_release{};
    u16 mode_at_release{};
    u16 start_gate_at_release{};
};

void seed_actor(LegacyBattleActorGroupBElementState& actor) {
    actor.object_token = kLegacyBattleActorGroupBBaseToken;
    actor.resource_token = 0x12340000U;
    actor.action_record.action_id = 109U;
    actor.action_record.position_x = 75U;
    actor.action_execution.action_target = 0xCAFEU;
    actor.action_execution.start_gate = 0x1234U;
    actor.action_execution.effect_resource_slots.fill(0xFFFFU);
    actor.action_execution.effect_resource_cursor = 9U;
    actor.action_execution.special_mode = 7U;
    actor.action_execution.execution_complete = 8U;
    actor.action_execution.special_four_hundred_workspace =
        std::make_unique<std::array<openswd3::compat::u8, 0x4C0U>>();
    actor.action_execution.special_four_hundred_workspace->fill(0xA5U);
    actor.action_configuration.resource_mode = 9U;
    actor.action_composition.mode_flags = 0xFFU;
    actor.startup_reset.bytes_283c_2953.fill(std::byte{0xA5});
    actor.startup_reset.bytes_295a_299f.fill(std::byte{0xA5});
    actor.startup_reset.bytes_26c9_26cb.fill(std::byte{0xA5});
    actor.startup_reset.bytes_2a97_2a9a.fill(std::byte{0xA5});
    actor.startup_reset.field_2ae4 = 19U;
    actor.runtime_reset.field_2b10 = 0xDEADBEEFU;
    actor.action_execution.early_latch = 0xDEADBEEFU;
}

class StorageMonPort final : public LegacyBattleMonDatabasePort {
public:
    u32 open_mon_file(const std::filesystem::path& path) override {
        return files.open_file(path, root);
    }

    u32 seek_mon_file(
        const u32 handle,
        const openswd3::compat::i32 distance,
        const LegacyBattleMonSeekOrigin origin
    ) override {
        return files.seek_file(handle, distance, origin);
    }

    LegacyBattleMonReadResult read_mon_file(
        const u32 handle,
        const std::span<openswd3::compat::u8> destination,
        const u32 requested_bytes
    ) override {
        return files.read_file(handle, destination, requested_bytes);
    }

    LegacyBattleMonStreamAllocation
    allocate_mon_stream(const u32 size) override {
        return streams.allocate(size);
    }

    void release_mon_stream(const u32 block_token) override {
        streams.release(block_token);
    }

    u32 mon_text_size(const u32 block_token) override {
        return texts.allocation_size(block_token);
    }

    LegacyBattleMonTextAllocation allocate_mon_text(const u32 size) override {
        return texts.allocate(size);
    }

    void release_mon_text(const u32 block_token) override {
        texts.free(block_token);
    }

    LegacyBattleMonFileRuntime files;
    LegacyBattleMonStreamRuntime streams;
    LegacyBattleMonTextRuntime texts;
    std::filesystem::path root;
};

void test_persistent_enemy_storage(openswd3::test::Context& test) {
    LegacyBattleGroupBStorage storage;
    test.expect_true(storage.construct(), "construct all eight static enemies");
    const auto actors = storage.actors();
    const auto first_token = (*actors)[0].resource_token;
    for (std::size_t index = 0U; index < actors->size(); ++index) {
        auto& actor = (*actors)[index];
        test.expect_equal(
            actor.object_token,
            kLegacyBattleActorGroupBBaseToken +
                static_cast<u32>(index) * kLegacyBattleActorGroupBElementSize,
            "static enemy identity follows original vector stride"
        );
        const auto bytes = storage.resource_bytes(actor.resource_token);
        test.expect_equal(
            bytes.size(),
            std::size_t{0xA4U},
            "each constructor reserves its complete record"
        );
        test.expect_true(
            bytes.data() == actor.resource_bytes.data(),
            "guest resource borrows the sole actor record"
        );
        for (std::size_t earlier = 0U; earlier < index; ++earlier) {
            test.expect_true(
                actor.resource_token != (*actors)[earlier].resource_token,
                "enemy allocations have distinct guest identities"
            );
        }
    }

    (*actors)[0].resource_bytes[5] = 0x78U;
    (*actors)[0].resource_bytes[6] = 0x56U;
    (*actors)[0].resource_bytes[7] = 0x34U;
    (*actors)[0].resource_bytes[8] = 0x12U;
    test.expect_equal(
        storage.read_linked_action_next(first_token + 5U),
        std::optional<u32>{0x12345678U},
        "linked read uses live bytes even at an interior address"
    );
    test.expect_false(
        storage.read_linked_action_next(first_token + 0xA1U).has_value(),
        "linked dword read cannot cross the allocated extent"
    );

    LegacyBattleGroupBStorage interrupted;
    (*interrupted.actors())[3].object_writable_bytes = 0U;
    test.expect_false(
        interrupted.construct(), "constructor stops on the failing actor"
    );
    test.expect_true(
        (*interrupted.actors())[2].resource_token != 0U,
        "completed constructor prefix retains its allocations"
    );
    test.expect_equal(
        (*interrupted.actors())[4].object_token,
        0U,
        "construction failure does not enter the next actor"
    );
    test.expect_false(
        interrupted.construct(), "construction stop is not retried as success"
    );

    (*actors)[7].runtime_reset.field_2b10 = 0xAABBCCDDU;
    (*actors)[7].resource_bytes[0] = 0x5AU;
    test.expect_true(
        storage.construct(), "repeated construction is not a new vector"
    );
    test.expect_equal(
        (*actors)[0].resource_token,
        first_token,
        "repeated construction retains the original allocation"
    );
    test.expect_equal(
        (*actors)[7].resource_bytes[0],
        openswd3::compat::u8{0x5AU},
        "repeated construction does not clear stored bytes"
    );

    auto startup = std::make_unique<LegacyBattleStartupState>();
    startup->group_b_lifecycle = actors;
    auto action = std::make_unique<LegacyBattleActionDispatchState>();
    (*action->group_b_fixed_particle_phases)[7].render_toggle_gate = 123U;
    action->group_b_reward_scale[7].status_bits = 0xA5U;
    const auto* phases = action->group_b_fixed_particle_phases.get();
    const auto node = action->target_phase_particle_nodes.allocate_zeroed();
    LegacyBattleFinalActorStepState final_actor;
    reset_legacy_battle_dispatch_preserving_actors(*action, final_actor);
    test.expect_true(
        action->group_b_fixed_particle_phases.get() == phases,
        "dispatch reset preserves enemy phase ownership"
    );
    test.expect_true(
        action->target_phase_particle_nodes.node(node) != nullptr,
        "dispatch reset retains referenced particle allocations"
    );

    LegacyBattleEnemySlot source{};
    source.active = true;
    source.resource_id = 1U;
    source.screen_x = 77U;
    source.screen_y = 88U;
    source.record_flag = true;
    openswd3::test::LegacyBattleMonDatabaseFixture mon;
    for (unsigned entry = 0U; entry < 2U; ++entry) {
        const auto result =
            storage.initialize_enemy(0U, source, true, *startup, *action, mon);
        test.expect_equal(
            result,
            LegacyBattleGroupBStartupBindingStatus::completed,
            "first and repeated entry configure the same enemy"
        );
        test.expect_equal(
            (*actors)[0].resource_token,
            first_token,
            "ordinary entry retains the static MON allocation"
        );
        test.expect_equal(
            (*actors)[0].action_record.runtime_value,
            0U,
            "enemy source record is not an active-slot boolean"
        );
        test.expect_equal(
            (*actors)[0].runtime_reset.field_2af0,
            1U,
            "record flag setter runs after MON configuration"
        );
        test.expect_equal(
            (*action->group_b_fixed_particle_phases)[0].render_toggle_gate,
            1U,
            "mirror setter uses shared actor field 2B08"
        );
    }

    test.expect_equal(
        (*actors)[7].runtime_reset.field_2b10,
        0xAABBCCDDU,
        "inactive actor fields survive entry"
    );
    test.expect_equal(
        (*action->group_b_fixed_particle_phases)[7].render_toggle_gate,
        123U,
        "inactive enemy phase survives entry"
    );
    test.expect_equal(
        action->group_b_reward_scale[7].status_bits,
        openswd3::compat::u8{0xA5U},
        "inactive reward fields survive entry"
    );

    mon.allocation_succeeds = false;
    test.expect_equal(
        storage.initialize_enemy(0U, source, false, *startup, *action, mon),
        LegacyBattleGroupBStartupBindingStatus::action_configuration_typed_stop,
        "MON allocation fault stops enemy initialization"
    );
    test.expect_equal(
        (*actors)[0].runtime_reset.field_2af0,
        0U,
        "MON failure does not execute the later flag setter"
    );

    mon.reset_mon_calls();
    startup->enemy_scratch.fill(0xABCDU);
    (*actors)[0].base_initialization.linked_action_head_token = 0x12345678U;
    test.expect_equal(
        storage.initialize_enemy(0U, source, false, *startup, *action, mon),
        LegacyBattleGroupBStartupBindingStatus::actor_reset_typed_stop,
        "unmapped linked node stops at the actual reset read"
    );
    test.expect_equal(mon.open_calls, 0U, "reset failure does not open MON");
    test.expect_equal(
        startup->enemy_scratch[0],
        0xABCDU,
        "reset failure precedes scratch clear"
    );

    test.expect_true(
        storage.release_heap_block(first_token).has_value(),
        "release removes the allocated resource mapping"
    );
    test.expect_true(
        storage.resource_bytes(first_token).empty(),
        "released guest record is no longer accessible"
    );
    test.expect_true(
        !storage.release_heap_block(first_token).has_value(),
        "repeated free is rejected"
    );

    (*actors)[0].action_configuration.source_runtime_value = 1U;
    test.expect_equal(
        storage.initialize_enemy(0U, source, false, *startup, *action, mon),
        LegacyBattleGroupBStartupBindingStatus::actor_reset_typed_stop,
        "release failure stops before configuration and scratch clear"
    );
    test.expect_equal(mon.open_calls, 0U, "failed release does not read MON");
    test.expect_equal(
        startup->enemy_scratch[0],
        0xABCDU,
        "failed release preserves scratch suffix"
    );
    test.expect_equal(
        storage.initialize_enemy(8U, source, false, *startup, *action, mon),
        LegacyBattleGroupBStartupBindingStatus::actor_index_typed_stop,
        "enemy index cannot cross the eight static objects"
    );

#ifdef OPENSWD3_GAME_DATA_ROOT
    LegacyBattleGroupBStorage real_storage;
    test.expect_true(
        real_storage.construct(), "construct real MON test enemies"
    );
    startup->group_b_lifecycle = real_storage.actors();
    StorageMonPort real_mon;
    real_mon.root = OPENSWD3_GAME_DATA_ROOT;
    LegacyBattleAssets assets;
    const auto loaded =
        load_legacy_battle_assets(real_mon.root, 98U, 0, assets);
    test.expect_equal(
        loaded.status,
        LegacyBattleAssetStatus::ready,
        "load original battle 98 placement data"
    );
    LegacyBattleSetupState setup;
    const std::array<openswd3::compat::u8, 4> party{1U, 0U, 0U, 0U};
    const auto prepared =
        prepare_legacy_battle_setup(assets, party, false, setup);
    test.expect_equal(
        prepared.status,
        LegacyBattleSetupStatus::ready,
        "prepare original enemy placement"
    );
    for (unsigned entry = 0U; entry < 2U; ++entry) {
        for (std::size_t index = 0U; index < setup.enemy_count; ++index) {
            test.expect_equal(
                real_storage.initialize_enemy(
                    index,
                    setup.enemies[index],
                    setup.mirrored,
                    *startup,
                    *action,
                    real_mon
                ),
                LegacyBattleGroupBStartupBindingStatus::completed,
                "original MON file configures persistent enemies across entries"
            );
        }
    }
#endif
}

}  // namespace

void test_battle_group_b_startup_reset(openswd3::test::Context& test) {
    test_persistent_enemy_storage(test);

    for (const bool fail_release : {false, true}) {
        LegacyBattleActorGroupBElementState actor;
        seed_actor(actor);
        actor.action_configuration.source_runtime_value = 1U;
        LegacyBattleActorProgressState progress;
        progress.mode_gate = 0x1234FFFFU;
        progress.frame_started = 88U;
        LegacyBattleRewardScaleActorState reward;
        reward.status_bits = 0xFFU;
        LegacyBattleTargetPhaseState particle;
        particle.active_gate = 33U;
        particle.render_toggle_gate = 44U;
        ObservingHeap heap{actor, progress};
        heap.fail = fail_release;
        const auto* workspace =
            actor.action_execution.special_four_hundred_workspace.get();
        const auto result = reset_legacy_battle_group_b_for_startup(
            actor, progress, reward, particle, heap
        );
        test.expect_equal(
            result.status == LegacyBattleActorStartupResetStatus::completed,
            !fail_release,
            "borrowed reset propagates real heap callback stop"
        );
        test.expect_equal(heap.calls, 1U, "resource released once");
        test.expect_equal(
            heap.released_token, 0x12340000U, "real resource token"
        );
        test.expect_equal(
            heap.mode_at_release,
            0xFEBDU,
            "callback observes earlier shared mode mask"
        );
        test.expect_equal(
            heap.target_at_release,
            0xCAFEU,
            "callback observes target before default suffix"
        );
        test.expect_equal(
            heap.start_gate_at_release,
            0x1234U,
            "callback observes unmodified start gate"
        );
        test.expect_true(
            actor.action_execution.special_four_hundred_workspace.get() ==
                workspace,
            "reset retains the existing workspace owner"
        );
        test.expect_true(
            std::ranges::all_of(
                *workspace, [](auto value) { return value == 0U; }
            ),
            "real workspace is cleared before release"
        );
        test.expect_equal(
            actor.action_record.action_id,
            109U,
            "external action source is not replaced by reset"
        );
        test.expect_equal(
            actor.action_record.position_x,
            75U,
            "external action coordinates survive reset"
        );
        test.expect_equal(
            actor.runtime_reset.field_2b10,
            0xDEADBEEFU,
            "unwritten residual field remains"
        );
        test.expect_equal(
            actor.action_execution.early_latch,
            0xDEADBEEFU,
            "unwritten action field remains"
        );
        test.expect_true(
            std::ranges::all_of(
                actor.startup_reset.bytes_283c_2953,
                [](auto value) { return value == std::byte{}; }
            ) &&
                std::ranges::all_of(
                    actor.startup_reset.bytes_295a_299f,
                    [](auto value) { return value == std::byte{}; }
                ),
            "remaining REP regions have actual owned storage"
        );
        if (fail_release) {
            test.expect_equal(
                actor.resource_token,
                0x12340000U,
                "failed free retains resource slot"
            );
            test.expect_equal(
                reward.status_bits,
                0xFFU,
                "failed free blocks shared reward clear"
            );
            test.expect_equal(
                particle.active_gate, 33U, "failed free blocks particle clear"
            );
            continue;
        }

        test.expect_equal(
            actor.resource_token, 0U, "successful free clears slot"
        );
        test.expect_equal(
            actor.action_execution.start_gate, 0U, "actual start gate cleared"
        );
        test.expect_equal(
            actor.action_execution.action_target,
            0xFFFFU,
            "actual target sentinel published"
        );
        test.expect_equal(
            actor.action_execution.effect_resource_cursor,
            0U,
            "actual effect cursor cleared"
        );
        test.expect_equal(
            actor.action_configuration.resource_mode,
            0U,
            "actual resource mode cleared"
        );
        test.expect_equal(
            actor.action_composition.mode_flags,
            0U,
            "composition observes the same mode-word clear"
        );
        test.expect_equal(
            progress.mode_gate,
            0x12340000U,
            "word store preserves host upper-word residue"
        );
        test.expect_equal(
            reward.status_bits, 0U, "shared reward owner cleared"
        );
        test.expect_equal(
            particle.active_gate, 0U, "shared particle owner cleared"
        );
        test.expect_equal(
            particle.render_toggle_gate,
            0U,
            "particle observes the same mirror reset"
        );
        test.expect_equal(
            progress.frame_started, 0U, "final shared field cleared"
        );
        test.expect_equal(
            actor.startup_reset.field_2ae4,
            0U,
            "Group-B availability storage cleared"
        );
        test.expect_true(
            std::ranges::all_of(
                actor.action_execution.effect_resource_slots,
                [](auto value) { return value == 0U; }
            ),
            "real effect slots cleared"
        );
    }

    LegacyBattleActorGroupBElementState actor;
    seed_actor(actor);
    actor.object_writable_bytes = 0x2B20U;
    LegacyBattleActorProgressState progress;
    progress.frame_started = 99U;
    LegacyBattleRewardScaleActorState reward;
    LegacyBattleTargetPhaseState particle;
    ObservingHeap heap{actor, progress};
    const auto result = reset_legacy_battle_group_b_for_startup(
        actor, progress, reward, particle, heap
    );
    test.expect_equal(
        result.stopped_instruction,
        0x0047D629U,
        "physical actor extent stops the last store"
    );
    test.expect_equal(
        progress.frame_started, 99U, "failed final store preserved"
    );
    test.expect_equal(
        actor.action_execution.start_gate,
        0U,
        "earlier shared writes survive final-store stop"
    );
}
