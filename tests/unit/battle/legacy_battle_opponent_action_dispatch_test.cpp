#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_opponent_action_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        if (request.callee_token == 0x004786B0U) {
            return {.eax = action};
        }
        if (request.callee_token == 0x00489E90U) {
            return {.eax = 0x90000000U};
        }
        if (request.callee_token == 0x004783B0U) {
            auto reply = default_reply;
            reply.publish_metric_word = true;
            reply.metric_word = 1U;
            return reply;
        }
        return default_reply;
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_group_b_actor_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::battle::LegacyBattleActorGroupBElementState& actor
    ) override {
        const auto reply = invoke(request);
        if (complete_group_b_execution &&
            request.callee_token == 0x0047C950U) {
            actor.action_execution.primary_action_record.field_8c = 1U;
        }
        return reply;
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_group_b_action_record(
        const LegacyBattleActionCallRequest& request,
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        const auto reply = invoke(request);
        if (complete_group_b_execution) {
            record.field_8c = 1U;
            if (request.callee_token == 0x004831C0U) {
                record.field_5a = 4U;
                record.field_24 = 1U;
            }
        }
        return reply;
    }

    [[nodiscard]] bool group_b_action_configuration_typed_stop(
        const u32 callee_token
    ) const noexcept override {
        return group_b_typed_stop_callee == callee_token;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleActionCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    u16 action{};
    u32 group_b_typed_stop_callee{};
    bool complete_group_b_execution{true};
    LegacyBattleActionCallReply default_reply{.eax = 1U};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
};

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        if (!ready) {
            return {};
        }

        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes,
        };
    }

    std::array<u8, 2> bytes{0x44U, 0x45U};
    bool ready{};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        if (!available) {
            return false;
        }

        piece.source = {
            .bytes = source,
            .layout = openswd3::rendering::LegacyBlitSourceLayout::direct_16,
        };
        piece.width = 1U;
        piece.height = 1U;
        return true;
    }

    std::array<u8, 2> source{0x34U, 0x12U};
    bool available{};
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
    ActionStreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    CountdownFlags countdown_flags;
    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    std::array<u8, 16> flags{};
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};
    std::array<u32, 0x32> attack_order_party_sources{};
    u32 attack_order_primary_gate{};
    u32 attack_order_secondary_gate{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};

    Fixture() {
        startup->group_b_lifecycle = std::make_unique<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        for (std::size_t index = 0U;
             index < startup->group_b_lifecycle->size(); ++index) {
            auto& actor = (*startup->group_b_lifecycle)[index];
            actor.object_token = 0x00525508U +
                static_cast<u32>(index) * 0x2B28U;
            actor.action_execution.turn_frame_token =
                0x70000000U + static_cast<u32>(index) * 0x100U;
            actor.action_execution.resource.token =
                0x70100000U + static_cast<u32>(index) * 0x100U;
            actor.action_execution.resource.value_0c = 32U;
            actor.action_execution.resource.value_0e = 24U;
            actor.action_execution.render_source_token =
                0x71000000U + static_cast<u32>(index) * 0x100U;
            actor.action_execution.render_source_value_04 =
                0x72000000U + static_cast<u32>(index) * 0x100U;
            actor.action_configuration.profile_buffer[0x0CU] = std::byte{1U};
        }
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
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = flags,
            .startup = startup.get(),
            .attack_order_records = attack_order_records,
            .attack_order_party_sources = attack_order_party_sources,
            .attack_order_primary_gate = &attack_order_primary_gate,
            .attack_order_secondary_gate = &attack_order_secondary_gate,
            .attack_order_adjacent_record = &attack_order_adjacent_record,
            .status_indicator_action_eax_snapshot = 0U,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
        };
    }
};

[[nodiscard]] bool has_call_argument(
    const DispatchPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleActionCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_opponent_action_dispatch(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 1U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 8U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop &&
                result.port_calls == 0U,
            "opponent source stops at first group B object query"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 100U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 0xFFFFFFFFU
            );
        test.expect_true(
            result.return_value == 1U && result.port_calls == 1U,
            "opponent action one hundred returns one before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 500U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 0xFFFFFFFFU
            );
        test.expect_true(
            result.return_value == 0U && result.port_calls == 1U,
            "unrecognized large opponent action returns before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 200U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 10U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                result.port_calls == 1U,
            "opponent action two hundred stops at first group A target call"
        );
    }

    {
        bool special_match = true;
        for (const u16 action : {u16{200U}, u16{300U}}) {
            LegacyBattleActionDispatchState state;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result =
                openswd3::battle::dispatch_legacy_battle_opponent_action(
                    state, port, context, 1U, 2U
                );
            special_match = special_match &&
                result.status == LegacyBattleActionDispatchStatus::completed;
            if (action == 200U) {
                special_match = special_match && result.return_value == 0U &&
                    port.count(0x00482310U) == 1U;
            } else {
                special_match = special_match && result.return_value == 1U &&
                    state.current_actor_index == 0xFFFFU &&
                    port.count(0x00482840U) == 1U;
            }
        }
        test.expect_true(
            special_match,
            "opponent actions two and three hundred use distinct group A target callees"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.selection_word = 4U;
        state.selection_high_word = 5U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 3U;
        port.push(0x004786B0U, {.eax = 1U, .edx = 0x13572468U});
        (*fixture.startup->group_b_lifecycle)[1U]
            .action_configuration.profile_buffer[0x0CU] = std::byte{9U};
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 1U, 2U
            );
        test.expect_true(
            result.return_value == 0U && state.action_pending == 1U &&
                openswd3::compat::u16(state.packed_action_state) == 2U &&
                state.selected_target_index == 2U &&
                state.current_actor_index == 0xFFFFU &&
                port.battle_pair_primary_value() == 0U &&
                state.selection_word == 0U && state.selection_high_word == 0U &&
                fixture.framebuffer.physical_pixels().front() == 0xFFFFU &&
                result.group_b_action_execution_calls == 1U &&
                result.pair_transition_calls == 1U &&
                result.pair_transition.port_calls == 1U &&
                has_call_argument(port, 0x004831C0U, 0U, 0x00508838U) &&
                std::ranges::find_if(
                    port.calls,
                    [](const auto& call) {
                        return call.callee_token == 0x0047C6B0U &&
                            call.eax == 0x0000179AU &&
                            call.edx == 0x13572468U;
                    }
                ) != port.calls.end() &&
                port.count(0x004758A0U) == 0U &&
                has_call_argument(port, 0x00478710U, 1U, 300U),
            "opponent action one side zero commits pair then clears all three visual channels"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.side_mode = 1U;
        state.selection_word = 8U;
        state.selection_high_word = 9U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 7U;
        port.push(0x004786B0U, {.eax = 1U, .edx = 0x24681357U});
        (*fixture.startup->group_b_lifecycle)[1U]
            .action_configuration.profile_buffer[0x0CU] = std::byte{9U};
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 1U, 3U
            );
        test.expect_true(
            result.return_value == 1U && state.group_a_to_actor[3] == 3U &&
                state.selected_target_index == 3U &&
                port.battle_pair_primary_value() == 0U &&
                state.selection_word == 8U && state.selection_high_word == 9U &&
                result.group_b_action_execution_calls == 1U &&
                result.pair_transition_calls == 0U &&
                has_call_argument(port, 0x004831C0U, 0U, 0x0052D680U) &&
                std::ranges::find_if(
                    port.calls,
                    [](const auto& call) {
                        return call.callee_token == 0x0047C6B0U &&
                            call.eax == 0x0000102FU &&
                            call.edx == 0x24681357U;
                    }
                ) != port.calls.end() &&
                port.count(0x004758A0U) == 0U &&
                port.count(0x00478710U) == 0U,
            "opponent action one side nonzero skips pair commit and preserves selection words"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 2U;
        auto context = fixture.context();
        const auto initialized =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        state.action_runtime_flags |= 1U;
        const auto completed =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            initialized.return_value == 0U && completed.return_value == 1U &&
                port.count(0x00489E90U) == 1U &&
                port.count(0x00489D00U) == 1U && !state.deformation &&
                !state.deformation_active &&
                state.deformation_owner_token == 0U &&
                state.frame_effect.fade_active == 1U,
            "opponent action two releases deformation owner on completed runtime bit"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 2U, 4U
            );
        test.expect_true(
            result.return_value == 1U &&
                openswd3::compat::u16(state.phase_counter) == 0U &&
                openswd3::compat::u16(state.input_mode) == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.frame_effect.fade_active == 1U &&
                port.count(0x00484020U) == 1U && port.count(0x004841B0U) == 1U,
            "opponent action six initializes and completes target phase in original order"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.active_target_code = 7U;
        state.active_effect_target = 7U;
        state.active_effect_gate = 9U;
        state.packed_actor_counter = 0xAABBCCFFU;
        Fixture fixture;
        fixture.attack_order_records[0].value_00 = 7U;
        DispatchPort port;
        port.action = 7U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 3U
            );
        test.expect_true(
            result.return_value == 1U &&
                state.packed_actor_counter == 0xAABBCC00U &&
                state.active_target_code == 0U &&
                state.active_effect_target == 0xFFFFFFFFU &&
                state.active_effect_gate == 0U &&
                result.attack_order_remove_calls == 1U &&
                result.attack_order_remove.matched &&
                fixture.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                port.count(0x0045EFB0U) == 0U,
            "opponent action seven wraps packed low byte and clears matching active targets"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.phase_counter = 0x12340000U;
        state.opponent_special_action = 0x1111U;
        state.opponent_spawn_count = 0x2222U;
        Fixture fixture;
        DispatchPort port;
        port.action = 15U;
        auto context = fixture.context();
        context.startup = nullptr;
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_opponent_wave_parameters_typed_stop &&
                result.group_b_opponent_wave_parameters_calls == 1U &&
                result.group_b_opponent_wave_parameters.status ==
                    openswd3::battle::
                        LegacyBattleGroupBOpponentWaveParametersStatus::
                            actor_state_typed_stop &&
                result.group_b_opponent_wave_parameters.return_eax ==
                    0x12340000U &&
                result.group_b_opponent_wave_parameters.return_ecx ==
                    0x00525508U &&
                result.group_b_opponent_wave_parameters.return_edx ==
                    0x0053BF28U &&
                state.phase_counter == 0x12348019U &&
                state.opponent_special_action == 0x1111U &&
                state.opponent_spawn_count == 0x2222U &&
                result.group_b_iterations == 0U &&
                port.count(0x00476900U) == 0U && port.calls.size() == 1U,
            "opponent action fifteen stops at the source actor read after committing the phase prefix"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.phase_counter = 0xCAFE0000U;
        state.group_b_count = 0;
        state.mirror_group_b_spawn = 1U;
        Fixture fixture;
        fixture.startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        auto& source_actor = (*fixture.startup->group_b_lifecycle)[0U];
        source_actor.action_record.prefix[3U] = std::byte{0xA5};
        source_actor.action_configuration.profile_buffer[0x20U] =
            std::byte{0x34};
        source_actor.action_configuration.profile_buffer[0x21U] =
            std::byte{0x12};
        source_actor.action_configuration.profile_buffer[0x24U] = std::byte{2U};
        DispatchPort port;
        port.action = 15U;
        port.push(0x00478220U, {.eax = 0xABCD0000U});
        port.push(
            0x00478220U,
            {.eax = 0x11110000U, .ecx = 0x11112222U, .edx = 0x11113333U}
        );
        port.push(0x00478220U, {.eax = 0xBEEF0000U});
        port.push(
            0x00478220U,
            {.eax = 0x22220000U, .ecx = 0x22222222U, .edx = 0x22223333U}
        );
        auto context = fixture.context();
        const auto running =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        state.phase_counter = 0x8001U;
        const auto completed =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            running.return_value == 0U && completed.return_value == 1U &&
                running.group_b_iterations == 2U && state.group_b_count == 2 &&
                running.group_b_opponent_wave_parameters_calls == 1U &&
                completed.group_b_opponent_wave_parameters_calls == 0U &&
                running.group_b_opponent_wave_parameters.special_action ==
                    0x1234U &&
                running.group_b_opponent_wave_parameters.spawn_count == 2U &&
                running.group_b_opponent_wave_parameters.return_eax ==
                    0xCAFE0002U &&
                running.group_b_opponent_wave_parameters.return_ecx ==
                    0x0053BF2AU &&
                running.group_b_opponent_wave_parameters.return_edx ==
                    0x0053BF28U &&
                fixture.startup->group_b_lifecycle != nullptr &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_record.action_id == 0x1234U &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_record.position_x == 400U &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_record.position_y == 220U &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_record.prefix[3U] == std::byte{0xA5} &&
                (*fixture.startup->group_b_lifecycle)[1U]
                        .action_record.position_x == 400U &&
                (*fixture.startup->group_b_lifecycle)[1U]
                        .action_record.position_y == 350U &&
                state.opponent_spawn_count == 0U &&
                port.requested_definition_ids ==
                    std::vector<u32>{0x1234U, 0x1234U, 0x1234U, 0x1234U} &&
                port.count(0x00476900U) == 0U &&
                port.count(0x00475720U) == 0U &&
                port.count(0x00476A80U) == 0U && port.open_calls == 1U &&
                port.read_calls == 18U && port.release_calls == 6U &&
                port.count(0x0045B0E0U) == 0U &&
                port.count(0x004783B0U) == 3U &&
                port.count(0x0045B190U) == 0U && port.count(0x0045B5A0U) == 0U,
            "opponent action fifteen builds two mirrored records with callee stale EAX high words"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 0;
        Fixture fixture;
        (*fixture.startup->group_b_lifecycle)[0U]
            .action_configuration.profile_buffer[0x24U] = std::byte{9U};
        DispatchPort port;
        port.action = 15U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop &&
                result.group_b_iterations == 8U && state.group_b_count == 8 &&
                result.group_b_opponent_wave_parameters_calls == 1U &&
                port.count(0x00476900U) == 0U && port.count(0x0047D350U) == 8U,
            "ninth opponent wave stops only after eight complete record side effects"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        auto& source_profile = (*fixture.startup->group_b_lifecycle)[0U]
                                   .action_configuration.profile_buffer;
        source_profile[0x20U] = std::byte{0x34};
        source_profile[0x21U] = std::byte{0x12};
        source_profile[0x24U] = std::byte{1U};
        DispatchPort port;
        port.action = 15U;
        port.allocation_succeeds = false;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        mon_definition_load_typed_stop &&
                result.group_b_iterations == 0U && state.group_b_count == 0 &&
                fixture.startup->group_b_lifecycle != nullptr &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_record.action_id == 0x1234U &&
                result.group_b_opponent_wave_parameters_calls == 1U &&
                port.count(0x00476900U) == 0U &&
                port.count(0x00475720U) == 0U &&
                port.count(0x00476A80U) == 0U && port.allocation_calls == 1U &&
                port.release_calls == 0U && port.count(0x00478220U) == 0U,
            "opponent action fifteen propagates the typed profile stop without completing the wave"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        Fixture fixture;
        auto& actor =
            (*fixture.startup->group_b_lifecycle)[0U].action_execution;
        actor.turn_countdown = 7;
        actor.profile_value = 0x1234U;
        actor.position_x = 100U;
        actor.position_y = 100U;
        auto& record = actor.turn_action_record;
        record.action_id = actor.profile_value;
        record.cached_action_id = actor.profile_value;
        record.base_variant = 0x24U;
        record.cached_base_variant = 0x24U;
        record.field_4a = 2U;
        record.field_4c = 2U;
        fixture.stream_provider.ready = true;
        fixture.frame_provider.available = true;
        DispatchPort port;
        port.action = 17U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U &&
                result.group_b_action_seventeen_frame_calls == 1U &&
                result.group_b_action_seventeen_frame.return_eax == 0U &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                state.overlay_gate == 0U && port.count(0x004763D0U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x0047D860U) == 0U &&
                port.count(0x004787F0U) == 0U &&
                port.count(0x00478600U) == 1U && port.count(0x004785C0U) == 1U,
            "opponent action seventeen frame zero preserves the public per-frame path"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        Fixture fixture;
        DispatchPort port;
        port.action = 17U;
        auto context = fixture.context();
        context.startup = nullptr;
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_action_seventeen_frame_typed_stop &&
                result.return_value == 17U &&
                result.group_b_action_seventeen_frame_calls == 1U &&
                result.port_calls == 1U && state.overlay_gate == 0U &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                port.count(0x004763D0U) == 0U && port.count(0x0047D870U) == 0U,
            "opponent action seventeen propagates the first actor typed stop"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        Fixture fixture;
        DispatchPort port;
        port.action = 17U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        bool workspace_matches = true;
        for (std::size_t index = 0U; index < state.opponent_workspace.size();
             ++index) {
            workspace_matches = workspace_matches &&
                state.opponent_workspace[index] ==
                    (index % 7U == 0U ? 0xFFFFFFFFU : 0U);
        }
        test.expect_true(
            result.return_value == 1U && workspace_matches &&
                state.action_pending_aux == 1U &&
                port.battle_terminal_latch() == 0U &&
                port.battle_message_state() == 0x63U,
            "opponent action seventeen zeros workspace then writes eighteen spaced all one heads"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 2;
        state.opponent_special_action = 0x55U;
        state.post_battle_counter = 7U;
        Fixture fixture;
        DispatchPort port;
        port.action = 17U;
        port.push(0x0047CE80U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 99U
            );
        test.expect_true(
            result.return_value == 1U && state.group_b_count == 1 &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                state.opponent_special_action == 0U &&
                state.post_battle_counter == 0U &&
                port.count(0x0045B0E0U) == 0U &&
                port.count(0x004783B0U) == 1U &&
                port.count(0x0045B190U) == 0U && port.count(0x0045B5A0U) == 0U,
            "opponent action seventeen collapses final special opponent and reruns three stages"
        );
    }

    {
        bool valid_cases_complete = true;
        constexpr std::array<u16, 9> actions{
            1U,
            2U,
            6U,
            7U,
            10U,
            11U,
            12U,
            15U,
            17U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.group_b_count = 1;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result =
                openswd3::battle::dispatch_legacy_battle_opponent_action(
                    state, port, context, 0U, 0U
                );
            valid_cases_complete = valid_cases_complete &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_code == action;
        }
        test.expect_true(
            valid_cases_complete,
            "all nine populated opponent switch cases execute without default fallthrough"
        );
    }

    {
        bool sparse_cases_match = true;
        constexpr std::array<u16, 9> actions{
            0U,
            3U,
            4U,
            5U,
            8U,
            9U,
            13U,
            14U,
            16U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result =
                openswd3::battle::dispatch_legacy_battle_opponent_action(
                    state, port, context, 0U, 99U
                );
            sparse_cases_match = sparse_cases_match &&
                result.return_value == 0U && result.port_calls == 1U;
        }
        test.expect_true(
            sparse_cases_match,
            "zero and eight sparse opponent switch holes return before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.side_mode = 1U;
        Fixture fixture;
        fixture.raster.surface.width = 641;
        DispatchPort port;
        port.battle_pair_primary_value() = 9U;
        port.action = 1U;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::dispatch_legacy_battle_opponent_action(
                state, port, context, 0U, 1U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::framebuffer_typed_stop &&
                result.framebuffer_clear_calls == 1U &&
                state.frame_refresh_pending == 1U &&
                fixture.framebuffer.physical_pixels().front() == 0xFFFFU &&
                fixture.framebuffer.physical_pixels().back() == 0xFFFFU,
            "opponent oversized clear publishes refresh and fills owned prefix before stop"
        );
    }
}
