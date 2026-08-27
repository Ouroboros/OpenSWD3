#include "openswd3/battle/legacy_battle_group_a_target_cycle.hpp"

#include "test.hpp"

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;

struct Fixture {
    [[nodiscard]] openswd3::battle::LegacyBattleGroupATargetCycleBindings
    bindings() {
        return {
            .frame_input = frame,
            .final_actor = final_actor,
            .metrics = metrics,
            .target_runtime = runtime,
            .supplemental_count_word = supplemental,
        };
    }

    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState runtime;
    u16 supplemental{};
};

}  // namespace

void test_battle_group_a_target_cycle(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupATargetCycleStatus;
    using openswd3::battle::cycle_legacy_battle_group_a_target;

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 10U;
        fixture.metrics.group_a_count = 4U;
        fixture.frame.target_cursor = 0U;
        fixture.frame.target_actor_index = 9U;
        const auto result = cycle_legacy_battle_group_a_target(
            fixture.bindings(),
            {.entry_eax = 0x11U, .entry_ecx = 0x22U, .entry_edx = 0x33U}
        );
        test.expect_true(
            result.status == LegacyBattleGroupATargetCycleStatus::completed &&
                result.loop_iterations == 1U &&
                result.target_order_reads == 1U &&
                fixture.frame.target_cursor == 1U &&
                fixture.final_actor.published_actor_code == 3U &&
                fixture.frame.target_actor_index == 0U &&
                fixture.runtime.selection_input_gate == 1U &&
                result.return_eax == 1U && result.return_ecx == 3U &&
                result.return_edx == 4U,
            "queued actor ten matches the first one-based order entry and publishes code three"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 4U;
        fixture.frame.target_cursor = 0U;
        const auto result =
            cycle_legacy_battle_group_a_target(fixture.bindings(), {});
        test.expect_true(
            result.status == LegacyBattleGroupATargetCycleStatus::completed &&
                result.loop_iterations == 3U &&
                result.target_order_reads == 3U &&
                fixture.frame.target_cursor == 3U &&
                fixture.final_actor.published_actor_code == 1U &&
                result.return_eax == 3U && result.return_ecx == 1U,
            "target order scans two one and zero until it matches queued actor eight"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 10U;
        fixture.metrics.group_a_count = 7U;
        fixture.runtime.target_effect_value = 0x00020055U;
        fixture.supplemental = 1U;
        fixture.frame.target_cursor = 4U;
        const auto result =
            cycle_legacy_battle_group_a_target(fixture.bindings(), {});
        test.expect_true(
            result.status == LegacyBattleGroupATargetCycleStatus::completed &&
                result.loop_iterations == 1U &&
                fixture.frame.target_cursor == 1U &&
                fixture.final_actor.published_actor_code == 3U &&
                result.return_edx == 4U,
            "signed cursor wraps to one against group count minus effect high word and supplemental count"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 18U;
        fixture.metrics.group_a_count = 4U;
        fixture.frame.target_cursor = 0xFFFFFFFCU;
        const auto result =
            cycle_legacy_battle_group_a_target(fixture.bindings(), {});
        test.expect_true(
            result.status == LegacyBattleGroupATargetCycleStatus::completed &&
                result.loop_iterations == 1U &&
                result.target_order_reads == 1U &&
                fixture.frame.target_cursor == 0xFFFFFFFDU &&
                fixture.final_actor.published_actor_code == 11U &&
                result.return_eax == 0xFFFFFFFDU && result.return_ecx == 11U,
            "negative logical cursor preserves the physical read into the preceding shared candidate table"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 12U;
        fixture.metrics.group_a_count = 5U;
        fixture.frame.target_cursor = 0U;
        fixture.frame.target_actor_index = 7U;
        fixture.final_actor.published_actor_code = 9U;
        fixture.runtime.selection_input_gate = 8U;
        const auto result =
            cycle_legacy_battle_group_a_target(fixture.bindings(), {});
        test.expect_true(
            result.status ==
                    LegacyBattleGroupATargetCycleStatus::
                        target_order_typed_stop &&
                result.loop_iterations == 5U &&
                result.target_order_reads == 4U &&
                fixture.frame.target_cursor == 0U &&
                fixture.frame.target_actor_index == 7U &&
                fixture.final_actor.published_actor_code == 9U &&
                fixture.runtime.selection_input_gate == 8U &&
                result.return_eax == 5U && result.return_ecx == 3U &&
                result.return_edx == 5U,
            "typed-stop after four misses preserves the last physical candidate in ECX and all unpublished state"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.metrics.group_a_count = 5U;
        fixture.frame.target_cursor = 4U;
        fixture.frame.target_actor_index = 7U;
        fixture.final_actor.published_actor_code = 9U;
        fixture.runtime.selection_input_gate = 8U;
        const auto result =
            cycle_legacy_battle_group_a_target(fixture.bindings(), {});
        test.expect_true(
            result.status ==
                    LegacyBattleGroupATargetCycleStatus::
                        target_order_typed_stop &&
                result.loop_iterations == 1U &&
                result.target_order_reads == 0U &&
                fixture.frame.target_cursor == 4U &&
                fixture.frame.target_actor_index == 7U &&
                fixture.final_actor.published_actor_code == 9U &&
                fixture.runtime.selection_input_gate == 8U &&
                result.return_eax == 5U && result.return_ecx == 0U &&
                result.return_edx == 5U,
            "one-past target order stops at the first real read without publishing any suffix state"
        );
    }
}
