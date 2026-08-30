#include "openswd3/battle/legacy_battle_group_b_action_execution.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public openswd3::battle::LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        if (request.callee_token == 0x00439070U) {
            return {.eax = random_value};
        }
        if (request.callee_token == 0x0047CD60U) {
            return {.eax = effect_gate};
        }
        if (request.callee_token == 0x00481A40U) {
            return {.eax = calculated_effect};
        }
        if (request.callee_token == 0x0047F360U) {
            return {.eax = commit_effect};
        }
        if (request.callee_token == 0x00482E90U) {
            return {.eax = effect_status};
        }
        return {.eax = 1U, .ecx = request.ecx, .edx = request.edx};
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_group_b_actor_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::battle::LegacyBattleActorGroupBElementState& actor
    ) override {
        const auto reply = invoke(request);
        if (fail_early_finalize &&
            request.callee_token == 0x0047C950U) {
            return {.eax = 0U};
        }
        if (complete_actor && request.callee_token == 0x0047C950U) {
            actor.action_execution.primary_action_record.field_8c = 1U;
        }
        return reply;
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_group_b_action_record(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        const auto reply = invoke(request);
        if (populate_primary && request.callee_token == 0x004831C0U &&
            request.arguments[1U] == actor_token + 0x0338U) {
            record.field_5a = primary_flags;
            record.field_64 = 1U;
            record.field_66 = 2U;
            record.field_68 = 3U;
            record.field_76 = 1U;
            record.field_78 = 4U;
            record.field_7a = 5U;
            record.field_7c = 6U;
            record.field_7e = 7U;
            record.field_80 = 8U;
            record.field_82 = 9U;
            record.field_84 = 10U;
            record.field_86 = 11U;
            record.field_24 = primary_value;
            record.field_28 = primary_variant;
        }
        if (complete_records &&
            (request.callee_token == 0x00483B30U ||
             request.callee_token == 0x004838D0U ||
             request.callee_token == 0x00482E90U)) {
            record.field_8c = 1U;
        }
        return reply;
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_group_b_secondary_record(
        const LegacyBattleActionCallRequest& request,
        openswd3::battle::LegacyBattleGroupAActionExecutionRecord& record
    ) override {
        const auto reply = invoke(request);
        if (complete_secondary) {
            record.dwords[0x8CU / 4U] = 1U;
        }
        if (secondary_flags_value != 0U) {
            auto& packed = record.dwords[0x58U / 4U];
            packed = (packed & 0x0000FFFFU) |
                (static_cast<u32>(secondary_flags_value) << 16U);
        }
        return reply;
    }

    [[nodiscard]] std::size_t count(const u32 token) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [token](const auto& call) {
                return call.callee_token == token;
            }
        ));
    }

    u32 actor_token{0x00525508U};
    u32 random_value{7U};
    u32 effect_gate{1U};
    u32 calculated_effect{};
    u32 commit_effect{1U};
    u32 effect_status{2U};
    u16 primary_flags{0x052CU};
    u16 secondary_flags_value{};
    u32 primary_value{0x321U};
    u32 primary_variant{0x654U};
    bool populate_primary{};
    bool complete_records{};
    bool complete_actor{};
    bool complete_secondary{};
    bool fail_early_finalize{};
    std::vector<LegacyBattleActionCallRequest> calls;
};

class StreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece&
    ) noexcept override {
        return false;
    }
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        return 0U;
    }
};

class SoundPort final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(u16, u16) override {}
};

class CountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagPorts {
public:
    [[nodiscard]] bool query_internal_flag(u32) noexcept override {
        return false;
    }
    void set_internal_flag(u32) noexcept override {}
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    StreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater updater{stream_provider};
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    CountdownFlags countdown_flags;
    std::array<openswd3::compat::u8, 16> flags{};

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionDispatchContext
    context() {
        return {
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = flags,
            .startup = nullptr,
            .startup_reset = nullptr,
            .text_messages = nullptr,
            .attack_order_records = {},
            .attack_order_party_sources = {},
            .attack_order_primary_gate = nullptr,
            .attack_order_secondary_gate = nullptr,
            .attack_order_adjacent_record = nullptr,
            .status_indicator_action_eax_snapshot = 0U,
            .shared_action_dispatch = nullptr,
            .shared_final_actor = nullptr,
            .target_selection_runtime = nullptr,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
            .target_phase_time_seed = 0U,
            .target_phase_spawn_stack_snapshot = {},
            .scripted_resource_release_test_compat = false,
        };
    }
};

void bind_resource(
    openswd3::battle::LegacyBattleActorGroupBElementState& actor
) {
    actor.action_execution.turn_frame_token = 0x70000000U;
    actor.action_execution.resource.token = 0x70001000U;
    actor.action_execution.resource.value_0c = 90U;
    actor.action_execution.resource.value_0e = 60U;
    actor.action_execution.render_source_token = 0x71000000U;
    actor.action_execution.render_source_value_04 = 0x72000000U;
}

}  // namespace

void test_battle_group_b_action_execution(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActorGroupBElementState;
    using openswd3::battle::LegacyBattleGroupAActionExecutionSharedState;
    using openswd3::battle::LegacyBattleGroupBActionExecutionStatus;

    Fixture fixture;
    auto context = fixture.context();

    {
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                nullptr,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionExecutionStatus::
                        actor_state_typed_stop &&
                result.port_calls == 0U,
            "group B execution stops at the original first actor field read"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_execution.start_gate = 1U;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status == LegacyBattleGroupBActionExecutionStatus::completed &&
                result.return_eax == 0U && result.port_calls == 0U,
            "group B execution preserves the busy gate before every callee and resource access"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_configuration.profile_buffer[0x0CU] = std::byte{8U};
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.fail_early_finalize = true;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {
                    .actor_token = port.actor_token,
                    .target_token = 0x005029D0U,
                    .entry_eax = 0x12345678U,
                    .entry_edx = 0x89ABCDEFU,
                }
            );
        test.expect_true(
            result.return_eax == 0U && actor.action_execution.early_latch == 1U &&
                port.count(0x0047C6B0U) == 1U &&
                port.calls.front().eax == 0x12345678U &&
                port.calls.front().edx == 0x89ABCDEFU &&
                port.count(0x0047C950U) == 1U &&
                port.count(0x004831C0U) == 0U,
            "group B execution preserves the profile-bit-eight preflight failure and early latch before record preparation"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_execution.profile_value = 0x123U;
        actor.action_execution.action_runtime_gate = 0xABCD0000U;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        port.primary_flags = 2U;
        port.primary_value = 0x345U;
        port.primary_variant = 0x678U;
        port.complete_records = true;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.return_eax == 0U &&
                actor.action_execution.turn_action_record.action_id == 0x345U &&
                actor.action_execution.turn_action_record.base_variant == 0x678U &&
                actor.action_execution.primary_action_record.field_24 == 0U &&
                actor.action_execution.primary_action_record.field_28 == 0U &&
                actor.action_execution.action_runtime_gate == 0xABCD0000U &&
                port.count(0x00483B30U) == 1U &&
                std::ranges::find_if(
                    port.calls,
                    [](const auto& call) {
                        return call.callee_token == 0x00483B30U &&
                            call.arguments[1U] == 0xABCD0004U &&
                            call.eax == 0xABCD0004U && call.edx == 0x678U;
                    }
                ) != port.calls.end(),
            "group B execution transfers the bit-two record through the 0x4000 runtime gate and clears the source pair"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        actor.action_execution.profile_value = 0x123U;
        actor.action_configuration.profile_buffer[0x0CU] = std::byte{1U};
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionExecutionStatus::
                        action_resource_typed_stop &&
                result.action_record_calls == 1U &&
                actor.action_execution.primary_action_record.action_id ==
                    0x123U &&
                actor.action_execution.primary_action_record.base_variant ==
                    0x28U &&
                shared.turn_frame_source_token == 0U,
            "group B execution publishes the prepared record and null frame token before the original resource dereference stop"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        bind_resource(actor);
        actor.action_execution.render_source_token = 0U;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        port.primary_flags = 4U;
        port.primary_value = 0x222U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status == LegacyBattleGroupBActionExecutionStatus::completed &&
                result.return_eax == 0U &&
                shared.turn_frame_source_token == 0x70001000U &&
                port.count(0x004170E0U) == 3U,
            "group B incomplete secondary state draws from the action resource without touching the later render-source pointer"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        bind_resource(actor);
        actor.action_execution.render_source_token = 0U;
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        port.complete_secondary = true;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionExecutionStatus::
                        render_source_typed_stop &&
                shared.turn_frame_source_token == 0x70001000U &&
                actor.action_execution.action_runtime_gate == 0x8000U &&
                result.return_eax == 0x70000000U &&
                result.return_ecx == 0U && result.return_edx == 0U,
            "group B execution preserves resource-derived shared geometry before the second pointer fault"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        bind_resource(actor);
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.battle_pair_primary_value() = 10U;
        port.populate_primary = true;
        port.primary_flags = 0x0011U;
        port.primary_value = 0U;
        port.effect_gate = 0U;
        port.calculated_effect = 0x0000FFFEU;
        port.commit_effect = 0U;
        port.effect_status = 2U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.return_eax == 0U &&
                shared.last_effect_value == -2 &&
                port.battle_pair_primary_value() == 8U &&
                actor.action_execution.effect_application_latch == 1U &&
                actor.action_execution.primary_action_record.field_5a == 0U &&
                port.count(0x00481A40U) == 1U &&
                port.count(0x0047F360U) == 1U &&
                port.count(0x00482E90U) == 1U &&
                std::ranges::find_if(
                    port.calls,
                    [](const auto& call) {
                        return call.callee_token == 0x0047F360U &&
                            call.eax == 0xFFFFFFFEU && call.edx == 8U;
                    }
                ) != port.calls.end(),
            "group B primary effect preserves signed AX, wrapping accumulator publication, and the nonterminal status branch"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        bind_resource(actor);
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        port.primary_flags = 4U;
        port.primary_value = 0x123U;
        port.complete_secondary = true;
        port.secondary_flags_value = 0x0011U;
        port.effect_gate = 0U;
        port.calculated_effect = 5U;
        port.commit_effect = 1U;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.return_eax == 0U && shared.last_effect_value == 5,
            "group B secondary effect preserves the signed calculated value"
        );
        test.expect_true(
            port.battle_pair_primary_value() == 5U &&
                actor.action_execution.effect_application_latch == 1U &&
                std::ranges::find_if(
                    port.calls,
                    [](const auto& call) {
                        return call.callee_token == 0x0047F360U &&
                            call.eax == 5U && call.edx == 5U;
                    }
                ) != port.calls.end(),
            "group B secondary effect publishes the accumulator and completion latch"
        );
        test.expect_true(
            actor.action_execution.motion_word == 0xFFFCU,
            "group B secondary effect advances the non-direct motion word by negative four"
        );
        test.expect_true(
            port.count(0x004838D0U) == 1U,
            "group B secondary effect prepares one secondary record"
        );
        test.expect_true(
            port.count(0x004170E0U) == 3U,
            "group B secondary effect adds one non-direct draw after the two refresh viewport calls"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        bind_resource(actor);
        actor.action_execution.profile_value = 0x456U;
        actor.action_execution.motion_word = 0xFFFEU;
        actor.action_execution.completion_delay_word = 3U;
        actor.action_configuration.profile_buffer[0x0CU] = std::byte{1U};
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        port.complete_records = true;
        port.complete_actor = true;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status == LegacyBattleGroupBActionExecutionStatus::completed &&
                result.return_eax == 1U &&
                port.count(0x004758A0U) == 0U,
            "group B execution completes without the reclaimed opaque address"
        );
        test.expect_true(
            result.color_initialization_calls == 1U,
            "group B execution initializes the seven-word color transition"
        );
        test.expect_true(
            result.frame_refresh_calls == 1U,
            "group B execution always invokes the closed frame refresh"
        );
        test.expect_true(
            result.secondary_record_calls == 0U,
            "group B execution selects the direct effect path for profile bit zero"
        );
        test.expect_true(
            dispatch.active_effect_gate == 1U,
            "group B execution publishes the nonzero refreshed color gate"
        );
        test.expect_true(
            result.action_record_clears == 5U &&
                actor.action_execution.completion_delay_word == 20U &&
                actor.action_execution.turn_completion_latch == 1U &&
                actor.action_execution.primary_action_record.field_8c == 0U,
            "group B execution clears five records while preserving the original completion latch and random delay"
        );
        test.expect_true(
            actor.action_execution.special_four_hundred_workspace != nullptr &&
                std::ranges::all_of(
                    *actor.action_execution.special_four_hundred_workspace,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    actor.action_execution.target_indices,
                    [](const auto value) { return value == 0xFFFFFFFFU; }
                ),
            "group B execution materializes and clears the full workspace then publishes four all-one target slots"
        );
    }

    {
        LegacyBattleActorGroupBElementState actor;
        bind_resource(actor);
        actor.action_execution.profile_value = 0x789U;
        actor.action_execution.special_mode = 1U;
        actor.action_configuration.profile_buffer[0x0CU] = std::byte{1U};
        LegacyBattleActionDispatchState dispatch;
        LegacyBattleGroupAActionExecutionSharedState shared;
        Port port;
        port.populate_primary = true;
        port.complete_records = true;
        port.complete_actor = true;
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_action_execution(
                &actor,
                shared,
                dispatch,
                port,
                context,
                {.actor_token = port.actor_token, .target_token = 0x005029D0U}
            );
        test.expect_true(
            result.return_eax == 1U &&
                actor.action_execution.special_mode == 1U &&
                actor.action_execution.completion_delay_word == 17U &&
                port.count(0x00439070U) == 1U,
            "group B special mode consumes the shared completion bit and preserves the bounded random delay"
        );
    }
}
