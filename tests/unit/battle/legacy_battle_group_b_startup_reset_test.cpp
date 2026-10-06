#include "test.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_group_b_startup_reset.hpp"
#include "openswd3/battle/legacy_battle_reward_scale.hpp"

#include <algorithm>
#include <array>
#include <memory>

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

}  // namespace

void test_battle_group_b_startup_reset(openswd3::test::Context& test) {
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
            actor, progress, reward, particle, heap, 0U
        );
        test.expect_equal(
            result.returned,
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
        actor, progress, reward, particle, heap, 0U
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
