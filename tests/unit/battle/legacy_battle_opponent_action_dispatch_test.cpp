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
using openswd3::battle::LegacyBattleMonDatabasePort;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseCallReply;
using openswd3::battle::LegacyBattleMonDefinitionTextReleaseCallRequest;
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
        if (request.callee_token == 0x00489E90U) {
            return {.eax = 0x90000000U};
        }
        return default_reply;
    }

    [[nodiscard]] LegacyBattleActionCallReply invoke_group_b_actor_update(
        const LegacyBattleActionCallRequest& request,
        openswd3::battle::LegacyBattleActorGroupBElementState& actor
    ) override {
        const auto reply = invoke(request);
        if (complete_group_b_execution && request.callee_token == 0x0047C950U) {
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

    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    release_legacy_battle_mon_definition_text(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& request
    ) override {
        definition_text_release_requests.push_back(request);
        if (!definition_text_release_replies.empty()) {
            const auto reply = definition_text_release_replies.front();
            definition_text_release_replies.pop_front();
            if (reply.typed_stop) {
                return reply;
            }
            static_cast<void>(
                LegacyBattleMonDatabasePort::
                    release_legacy_battle_mon_definition_text(request)
            );
            return reply;
        }
        return LegacyBattleMonDatabasePort::
            release_legacy_battle_mon_definition_text(request);
    }

    void
    push_release(const LegacyBattleMonDefinitionTextReleaseCallReply& reply) {
        definition_text_release_replies.push_back(reply);
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
    std::deque<LegacyBattleMonDefinitionTextReleaseCallReply>
        definition_text_release_replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<LegacyBattleMonDefinitionTextReleaseCallRequest>
        definition_text_release_requests;
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
        piece.legacy_source_token = legacy_source_token;
        piece.width = width;
        piece.height = height;
        return true;
    }

    std::array<u8, 2> source{0x34U, 0x12U};
    u32 legacy_source_token{0x73000000U};
    u16 width{1U};
    u16 height{1U};
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
            actor.object_token =
                0x00525508U + static_cast<u32>(index) * 0x2B28U;
            actor.action_execution.position_x = 1U;
            actor.action_execution.position_y = 1U;
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

    void prepare_opponent_target_phase(
        openswd3::battle::LegacyBattleActionDispatchState& state,
        const std::size_t group_b_index,
        const std::size_t target_index
    ) {
        auto& target = state.group_a_action_execution[target_index];
        target.frame_source_action_record.action_id = 1U;
        target.position_x = 0x40U;
        target.source_y_offset = 0x10U;
        target.position_y = 0x60U;
        target.target_phase_y_adjustment = 0x20;
        startup->party[target_index].position_x = 0x40U;
        startup->party[target_index].source_y_offset = 0x10U;
        startup->party[target_index].position_y = 0x60U;
        startup->party[target_index].target_phase_y_adjustment = 0x20;
        auto& source = (*startup->group_b_lifecycle)[group_b_index];
        source.action_execution.position_x = 240U;
        source.action_execution.position_y = 220U;
        source.action_execution.target_phase_y_adjustment = 20;
        source.action_execution.source_x_offset = 40U;
        source.action_execution.source_y_offset = 10U;
        source.action_composition.mode_flags = 0x80U;
        frame_provider.available = true;
        frame_provider.height = 0x50U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTargetPhaseStartRequest
    opponent_target_phase_request() const {
        return {
            .surface_width = 640,
            .surface_height = 480,
            .entry_esp = 0x70002000U,
            .actor_frame_resource =
                {
                    .action_updater_return_eax = 1U,
                    .override_action_updater_return_eax = true,
                    .frame_provider_return_eax = 0x72000000U,
                    .frame_provider_return_edx = 0x89ABCDEFU,
                },
            .coordinate_output_x_initial = 0xAAAA1111U,
            .coordinate_output_y_initial = 0xBBBB2222U,
        };
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

[[nodiscard]] openswd3::battle::LegacyBattleActionDispatchResult dispatch(
    openswd3::battle::LegacyBattleActionDispatchState& state,
    DispatchPort& port,
    openswd3::battle::LegacyBattleActionDispatchContext& context,
    const u32 group_b_index,
    const u32 caller_group_a_index
) {
    if (context.startup != nullptr &&
        context.startup->group_b_lifecycle != nullptr &&
        group_b_index < context.startup->group_b_lifecycle->size()) {
        (*context.startup->group_b_lifecycle)[group_b_index]
            .action_composition.action_kind = port.action;
    }
    return openswd3::battle::dispatch_legacy_battle_opponent_action(
        state, port, context, group_b_index, caller_group_a_index
    );
}

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
        port.action = 300U;
        auto context = fixture.context();
        context.actor_action_kind_request.entry_edx = 0xAABBCCDDU;
        context.actor_action_kind_request.entry_esp = 0x82003000U;
        const auto result = dispatch(state, port, context, 2U, 0U);
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_code == 300U &&
                result.actor_action_kind_calls == 1U &&
                result.actor_action_kind.return_eax == 0x0000012CU &&
                result.actor_action_kind.return_ecx == 0x0052AB58U &&
                result.actor_action_kind.return_edx == 0xAABBCCDDU &&
                result.actor_action_kind.return_esp == 0x82003004U &&
                result.actor_action_kind.return_eip == 0x00455D9CU &&
                result.actor_action_kind.flags.parity &&
                result.actor_action_kind.flags.auxiliary_carry &&
                !result.actor_action_kind.flags.carry &&
                !result.actor_action_kind.flags.zero &&
                port.count(0x004786B0U) == 0U && port.count(0x00482840U) == 1U,
            "opponent dispatcher composes the action-kind getter with stride arithmetic residues and real return address"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 0x1234U;
        auto field_context = fixture.context();
        field_context.actor_action_kind_request.entry_edx = 0x11223344U;
        field_context.actor_action_kind_request.access.action_kind_readable =
            false;
        const auto field_stop = dispatch(state, port, field_context, 1U, 0U);

        auto return_context = fixture.context();
        return_context.actor_action_kind_request.entry_edx = 0x55667788U;
        return_context.actor_action_kind_request.access
            .return_address_readable = false;
        const auto return_stop = dispatch(state, port, return_context, 1U, 0U);
        test.expect_true(
            field_stop.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_action_kind_typed_stop &&
                field_stop.actor_action_kind.return_eax == 0x00000565U &&
                field_stop.actor_action_kind.return_eip == 0x004786B0U &&
                field_stop.actor_action_kind.action_kind_reads == 0U &&
                return_stop.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_action_kind_typed_stop &&
                return_stop.actor_action_kind.return_eax == 0x00001234U &&
                return_stop.actor_action_kind.return_eip == 0x004786B7U &&
                return_stop.actor_action_kind.action_kind_reads == 1U &&
                return_stop.actor_action_kind.return_address_reads == 0U &&
                port.calls.empty(),
            "opponent dispatcher propagates action-kind field and RET stops before every switch side effect"
        );
    }

    {
        using Call =
            openswd3::battle::LegacyBattleGroupBActionSeventeenFrameCall;
        static_assert(
            static_cast<u8>(Call::reserved_actor_current_coordinate_query) == 2U
        );
        const std::array reserved_calls{
            Call::reserved_actor_current_coordinate_query,
            Call::reserved_actor_coordinate_publication,
        };
        DispatchPort port;
        bool empty_replies = true;
        for (const auto call : reserved_calls) {
            const auto reply = openswd3::battle::
                invoke_legacy_battle_opponent_action_seventeen_frame_call(
                    port,
                    {.call = call,
                     .arguments = {0x11111111U, 0x22222222U},
                     .eax = 0x33333333U,
                     .ecx = 0x44444444U,
                     .edx = 0x55555555U}
                );
            empty_replies = empty_replies && reply.eax == 0U &&
                reply.ecx == 0U && reply.edx == 0U &&
                reply.outputs == std::array<u32, 2>{};
        }
        test.expect_true(
            empty_replies && port.calls.empty(),
            "opponent action seventeen keeps reserved coordinate ordinals as empty adapter slots"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 1U;
        auto context = fixture.context();
        const auto result = dispatch(state, port, context, 8U, 0U);
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
        const auto result = dispatch(state, port, context, 0U, 0xFFFFFFFFU);
        test.expect_true(
            result.return_value == 1U && result.port_calls == 0U &&
                result.actor_action_kind_calls == 1U,
            "opponent action one hundred returns one before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 500U;
        auto context = fixture.context();
        const auto result = dispatch(state, port, context, 0U, 0xFFFFFFFFU);
        test.expect_true(
            result.return_value == 0U && result.port_calls == 0U &&
                result.actor_action_kind_calls == 1U,
            "unrecognized large opponent action returns before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 200U;
        auto context = fixture.context();
        const auto result = dispatch(state, port, context, 0U, 10U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                result.port_calls == 0U && result.actor_action_kind_calls == 1U,
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
            const auto result = dispatch(state, port, context, 1U, 2U);
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
        port.action = 1U;
        port.battle_pair_primary_value() = 3U;
        (*fixture.startup->group_b_lifecycle)[1U]
            .action_configuration.profile_buffer[0x0CU] = std::byte{9U};
        auto context = fixture.context();
        context.actor_action_kind_request.entry_edx = 0x13572468U;
        const auto result = dispatch(state, port, context, 1U, 2U);
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
                            call.eax == 0x0000179AU && call.edx == 0x13572468U;
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
        port.action = 1U;
        port.battle_pair_primary_value() = 7U;
        (*fixture.startup->group_b_lifecycle)[1U]
            .action_configuration.profile_buffer[0x0CU] = std::byte{9U};
        auto context = fixture.context();
        context.actor_action_kind_request.entry_edx = 0x24681357U;
        const auto result = dispatch(state, port, context, 1U, 3U);
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
                            call.eax == 0x0000102FU && call.edx == 0x24681357U;
                    }
                ) != port.calls.end() &&
                port.count(0x004758A0U) == 0U && port.count(0x00478710U) == 0U,
            "opponent action one side nonzero skips pair commit and preserves selection words"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 2U;
        auto context = fixture.context();
        const auto initialized = dispatch(state, port, context, 0U, 99U);
        state.action_runtime_flags |= 1U;
        const auto completed = dispatch(state, port, context, 0U, 99U);
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
        auto& target = state.group_a_action_execution[4U];
        target.frame_source_action_record.action_id = 1U;
        target.position_x = 0x40U;
        target.source_y_offset = 0x10U;
        target.position_y = 0x60U;
        target.target_phase_y_adjustment = 0x20;
        state.group_a_target_phases[4U].emitter.flags = 0xCAFEU;
        state.group_b_target_phases[2U][3U] =
            std::make_unique<openswd3::battle::LegacyBattleTargetPhaseState>();
        state.group_b_target_phases[2U][3U]->emitter.flags = 0x1111U;
        state.group_b_target_phases[2U][5U] =
            std::make_unique<openswd3::battle::LegacyBattleTargetPhaseState>();
        state.group_b_target_phases[2U][5U]->emitter.flags = 0x2222U;
        Fixture fixture;
        fixture.startup->party[4U].position_x = 0x40U;
        fixture.startup->party[4U].source_y_offset = 0x10U;
        fixture.startup->party[4U].position_y = 0x60U;
        fixture.startup->party[4U].target_phase_y_adjustment = 0x20;
        fixture.frame_provider.available = true;
        fixture.frame_provider.width = 32U;
        fixture.frame_provider.height = 0x50U;
        auto& source = (*fixture.startup->group_b_lifecycle)[2U];
        source.action_execution.position_x = 240U;
        source.action_execution.position_y = 220U;
        source.action_execution.target_phase_y_adjustment = 20;
        source.action_execution.source_x_offset = 40U;
        source.action_execution.source_y_offset = 10U;
        source.action_composition.mode_flags = 0x80U;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        context.opponent_target_phase_start_request =
            fixture.opponent_target_phase_request();
        const auto result = dispatch(state, port, context, 2U, 4U);
        const auto decode_call = std::ranges::find_if(
            port.calls, [](const LegacyBattleActionCallRequest& call) {
                return call.callee_token == 0x004019A0U;
            }
        );
        const auto property_call = std::ranges::find_if(
            port.calls, [](const LegacyBattleActionCallRequest& call) {
                return call.callee_token == 0x0047CE70U;
            }
        );
        const auto target_mode_call = std::ranges::find_if(
            port.calls, [](const LegacyBattleActionCallRequest& call) {
                return call.callee_token == 0x004787F0U;
            }
        );
        const auto clear_call = std::ranges::find_if(
            port.calls, [](const LegacyBattleActionCallRequest& call) {
                return call.callee_token == 0x0047D870U;
            }
        );
        const auto& phase = *state.group_b_target_phases[2U][4U];
        const auto& emitter = phase.emitter;
        test.expect_true(
            result.return_value == 1U &&
                result.target_phase_start_calls == 1U &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        completed &&
                result.target_phase_start.parent_stack_writes ==
                    std::array<u32, 5>{
                        1U,
                        0x0050E6A0U,
                        0x0052AB58U,
                        4U,
                        0x00484034U,
                    },
            "opponent action six normal typed prefix status and stack"
        );
        test.expect_true(
            result.target_phase_start.coordinate_output_x == 0xAAAA0030U &&
                result.target_phase_start.coordinate_output_y == 0xBBBB0040U &&
                result.target_phase_start.return_eax == 480U &&
                result.target_phase_start.return_ecx == 0x0051DF80U &&
                result.target_phase_start.return_edx == 640U &&
                result.target_phase_start.return_esp == 0x7000200CU &&
                result.target_phase_start.return_eip == 0x0045645DU &&
                result.target_phase_start.flags_known &&
                !result.target_phase_start.flags.carry &&
                result.target_phase_start.flags.parity &&
                !result.target_phase_start.flags.auxiliary_carry &&
                result.target_phase_start.flags.auxiliary_carry_defined &&
                !result.target_phase_start.flags.zero &&
                !result.target_phase_start.flags.sign &&
                !result.target_phase_start.flags.overflow &&
                result.target_phase_start.return_ebx == 1U &&
                result.target_phase_start.return_ebp == 0x0050E6A0U &&
                result.target_phase_start.return_esi == 0x0052AB58U &&
                result.target_phase_start.return_edi == 4U,
            "opponent action six normal typed prefix coordinate and register residue"
        );
        test.expect_true(
            decode_call != port.calls.end() &&
                decode_call->eax == 0x70001FFCU &&
                decode_call->ecx == 0x72000000U &&
                decode_call->edx == 0x73000000U &&
                property_call != port.calls.end() && property_call->eax == 5U &&
                property_call->ecx == 0x0050E6A0U &&
                property_call->edx == 0x28U &&
                target_mode_call != port.calls.end() &&
                target_mode_call->eax == 480U &&
                target_mode_call->ecx == 0x0050E6A0U &&
                target_mode_call->edx == 640U &&
                clear_call != port.calls.end() && clear_call->eax == 1U &&
                clear_call->ecx == 0x0050E6A0U && clear_call->edx == 0U,
            "opponent action six preserves nested and immediate suffix call register residue"
        );
        test.expect_true(
            source.action_execution.target_phase_resource_token ==
                    0x72000000U &&
                target.turn_frame_token == 0x72000000U &&
                phase.borrowed_resource_token ==
                    &source.action_execution.target_phase_resource_token &&
                phase.borrowed_mode_flags ==
                    &source.action_composition.mode_flags &&
                emitter.source_width == 32U && emitter.source_height == 0x50U &&
                emitter.source_origin_x == 0x2F &&
                emitter.source_origin_y == 0x40 &&
                emitter.target_origin_x == 220 &&
                emitter.target_origin_y == 240 &&
                emitter.distance_offset_base == 0x14U &&
                emitter.lifetime_divisor == 0x28U &&
                emitter.remaining_batches == 0x28U &&
                emitter.spawn_divisor == 0x3CU && emitter.flags == 0x57U &&
                source.action_composition.mode_flags == 0x88U,
            "opponent action six normal typed prefix owner and emitter"
        );
        test.expect_true(
            state.group_a_target_phases[4U].emitter.flags == 0xCAFEU &&
                state.group_b_target_phases[2U][3U]->emitter.flags == 0x1111U &&
                state.group_b_target_phases[2U][5U]->emitter.flags == 0x2222U &&
                openswd3::compat::u16(state.phase_counter) == 0U &&
                openswd3::compat::u16(state.input_mode) == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.frame_effect.fade_active == 1U &&
                port.count(0x00484020U) == 0U &&
                port.count(0x00478620U) == 0U &&
                port.count(0x00478470U) == 0U &&
                port.count(0x004019A0U) == 1U &&
                port.count(0x0047CE70U) == 1U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x004787F0U) == 1U &&
                port.count(0x0047D870U) == 1U && port.count(0x004841B0U) == 1U,
            "opponent action six normal typed prefix isolation and suffix"
        );
        test.expect_true(
            result.return_value == 1U &&
                result.target_phase_start_calls == 1U &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        completed &&
                result.target_phase_start.parent_stack_writes ==
                    std::array<u32, 5>{
                        1U,
                        0x0050E6A0U,
                        0x0052AB58U,
                        4U,
                        0x00484034U,
                    } &&
                result.target_phase_start.coordinate_output_x == 0xAAAA0030U &&
                result.target_phase_start.coordinate_output_y == 0xBBBB0040U &&
                result.target_phase_start.presentation_dwords_zeroed == 0x16U &&
                result.target_phase_start.tail_dwords_zeroed == 0U &&
                result.target_phase_start.return_eax == 480U &&
                result.target_phase_start.return_ecx == 0x0051DF80U &&
                result.target_phase_start.return_edx == 640U &&
                result.target_phase_start.return_esp == 0x7000200CU &&
                result.target_phase_start.return_eip == 0x0045645DU &&
                result.target_phase_start.return_ebx == 1U &&
                result.target_phase_start.return_ebp == 0x0050E6A0U &&
                result.target_phase_start.return_esi == 0x0052AB58U &&
                result.target_phase_start.return_edi == 4U &&
                source.action_execution.target_phase_resource_token ==
                    0x72000000U &&
                target.turn_frame_token == 0x72000000U &&
                phase.borrowed_resource_token ==
                    &source.action_execution.target_phase_resource_token &&
                phase.borrowed_mode_flags ==
                    &source.action_composition.mode_flags &&
                emitter.source_width == 32U && emitter.source_height == 0x50U &&
                emitter.source_origin_x == 0x2F &&
                emitter.source_origin_y == 0x40 &&
                emitter.target_origin_x == 220 &&
                emitter.target_origin_y == 240 &&
                emitter.distance_offset_base == 0x14U &&
                emitter.lifetime_divisor == 0x28U &&
                emitter.remaining_batches == 0x28U &&
                emitter.spawn_divisor == 0x3CU && emitter.flags == 0x57U &&
                source.action_composition.mode_flags == 0x88U &&
                state.group_a_target_phases[4U].emitter.flags == 0xCAFEU &&
                state.group_b_target_phases[2U][3U]->emitter.flags == 0x1111U &&
                state.group_b_target_phases[2U][5U]->emitter.flags == 0x2222U &&
                openswd3::compat::u16(state.phase_counter) == 0U &&
                openswd3::compat::u16(state.input_mode) == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.frame_effect.fade_active == 1U &&
                port.count(0x00484020U) == 0U &&
                port.count(0x00478620U) == 0U &&
                port.count(0x00478470U) == 0U &&
                port.count(0x004019A0U) == 1U &&
                port.count(0x0047CE70U) == 1U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x004787F0U) == 1U &&
                port.count(0x0047D870U) == 1U && port.count(0x004841B0U) == 1U,
            "opponent action six initializes one Group-B source-by-target phase through the typed 484020 prefix before the unchanged suffix"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        auto& target = state.group_a_action_execution[2U];
        target.frame_source_action_record.action_id = 0x11111111U;
        target.frame_source_action_record.cached_action_id = 0x22222222U;
        target.frame_source_action_record.base_variant = 0x33333333U;
        target.frame_source_action_record.cached_base_variant = 0x44444444U;
        target.frame_prepared_action_record.cached_base_variant = 0xDEADBEEFU;
        state.group_b_target_phases[1U][2U] =
            std::make_unique<openswd3::battle::LegacyBattleTargetPhaseState>();
        state.group_b_target_phases[1U][2U]->emitter.flags = 0xCAFEU;
        Fixture fixture;
        fixture.prepare_opponent_target_phase(state, 1U, 2U);
        auto& source =
            (*fixture.startup->group_b_lifecycle)[1U].action_execution;
        source.target_phase_resource_token = 0xAABBCCDDU;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        context.opponent_target_phase_start_request =
            fixture.opponent_target_phase_request();
        context.opponent_target_phase_start_request.actor_frame_resource
            .source_dword_readable[3U] = false;
        const auto result = dispatch(state, port, context, 1U, 2U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_phase_start_typed_stop &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        actor_frame_resource_typed_stop &&
                result.target_phase_start.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        source_dword_read_typed_stop &&
                result.target_phase_start.actor_frame_resource.copied_dwords ==
                    3U &&
                result.target_phase_start.parent_stack_writes ==
                    std::array<u32, 5>{
                        1U,
                        0x00508838U,
                        0x00528030U,
                        2U,
                        0x00484034U,
                    } &&
                result.target_phase_start.return_eax == 0x005094F0U &&
                result.target_phase_start.return_ecx == 0x23U &&
                result.target_phase_start.return_edx == 0U &&
                result.target_phase_start.return_ebx == 0x00508838U &&
                result.target_phase_start.return_ebp == 0x00508838U &&
                result.target_phase_start.return_esi == 0x00508AE4U &&
                result.target_phase_start.return_edi == 0x005094FCU &&
                result.target_phase_start.return_esp == 0x70001FD0U &&
                result.target_phase_start.return_eip == 0x00478638U &&
                target.frame_prepared_action_record.action_id ==
                    target.frame_source_action_record.action_id &&
                target.frame_prepared_action_record.cached_action_id ==
                    target.frame_source_action_record.cached_action_id &&
                target.frame_prepared_action_record.base_variant ==
                    target.frame_source_action_record.base_variant &&
                target.frame_prepared_action_record.cached_base_variant ==
                    0xDEADBEEFU &&
                source.target_phase_resource_token == 0xAABBCCDDU &&
                state.group_b_target_phases[1U][2U]->emitter.flags == 0xCAFEU &&
                result.target_phase_start.coordinate_query_calls == 0U &&
                port.count(0x00484020U) == 0U &&
                port.count(0x00478620U) == 0U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x004841B0U) == 0U && port.calls.empty(),
            "opponent action six preserves the typed REP partial commit and suppresses the complete outer suffix"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_target_phases[0U][3U] =
            std::make_unique<openswd3::battle::LegacyBattleTargetPhaseState>();
        state.group_b_target_phases[0U][3U]->emitter.flags = 0xCAFEU;
        Fixture fixture;
        fixture.prepare_opponent_target_phase(state, 0U, 3U);
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        context.opponent_target_phase_start_request =
            fixture.opponent_target_phase_request();
        context.opponent_target_phase_start_request.actor_frame_resource
            .frame_provider_return_eax = 0U;
        const auto result = dispatch(state, port, context, 0U, 3U);
        const auto& source =
            (*fixture.startup->group_b_lifecycle)[0U].action_execution;
        const auto& phase = *state.group_b_target_phases[0U][3U];
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_phase_start_typed_stop &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        resource_object_typed_stop &&
                result.target_phase_start.actor_frame_resource.status ==
                    openswd3::battle::LegacyBattleActorFrameResourceStatus::
                        completed &&
                result.target_phase_start.actor_frame_resource
                    .frame_token_committed &&
                result.target_phase_start.actor_frame_resource.return_eax ==
                    0U &&
                source.target_phase_resource_token == 0U &&
                state.group_a_action_execution[3U].turn_frame_token == 0U &&
                result.target_phase_start.coordinate_query_calls == 1U &&
                result.target_phase_start.presentation_dwords_zeroed == 0x16U &&
                result.target_phase_start.decode_calls == 0U &&
                result.target_phase_start.return_eax == 0x70001FFCU &&
                result.target_phase_start.return_ecx == 0U &&
                result.target_phase_start.return_edx == 0x70001FF8U &&
                result.target_phase_start.return_ebx == 0x00525508U &&
                result.target_phase_start.return_ebp == 3U &&
                result.target_phase_start.return_esi == 0x00525610U &&
                result.target_phase_start.return_edi == 0x005264D4U &&
                result.target_phase_start.return_esp == 0x70001FD4U &&
                result.target_phase_start.return_eip == 0x0048407EU &&
                result.target_phase_start.flags_known &&
                !result.target_phase_start.flags.carry &&
                result.target_phase_start.flags.parity &&
                result.target_phase_start.flags.zero &&
                phase.emitter.flags == 0U &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_composition.mode_flags == 0x80U &&
                port.count(0x004019A0U) == 0U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x004841B0U) == 0U && port.calls.empty(),
            "opponent action six zero frame token reaches the exact 48407E stop after three pushes and no outer suffix"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_target_phases[0U][3U] =
            std::make_unique<openswd3::battle::LegacyBattleTargetPhaseState>();
        state.group_b_target_phases[0U][3U]->emitter.flags = 0xCAFEU;
        Fixture fixture;
        fixture.prepare_opponent_target_phase(state, 0U, 3U);
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        context.opponent_target_phase_start_request =
            fixture.opponent_target_phase_request();
        context.opponent_target_phase_start_request.resource_object_readable =
            false;
        const auto result = dispatch(state, port, context, 0U, 3U);
        const auto& source =
            (*fixture.startup->group_b_lifecycle)[0U].action_execution;
        const auto& phase = *state.group_b_target_phases[0U][3U];
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_phase_start_typed_stop &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        resource_object_typed_stop &&
                result.target_phase_start.actor_frame_resource.return_eax ==
                    0x72000000U &&
                source.target_phase_resource_token == 0x72000000U &&
                state.group_a_action_execution[3U].turn_frame_token ==
                    0x72000000U &&
                result.target_phase_start.coordinate_query_calls == 1U &&
                result.target_phase_start.presentation_dwords_zeroed == 0x16U &&
                result.target_phase_start.return_eax == 0x70001FFCU &&
                result.target_phase_start.return_ecx == 0x72000000U &&
                result.target_phase_start.return_edx == 0x70001FF8U &&
                result.target_phase_start.return_esp == 0x70001FD4U &&
                result.target_phase_start.return_eip == 0x0048407EU &&
                phase.emitter.flags == 0U && port.count(0x004019A0U) == 0U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x004841B0U) == 0U && port.calls.empty(),
            "opponent action six commits a nonzero source resource before the unreadable object stop and suppresses all later calls"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_target_phases[0U][1U] =
            std::make_unique<openswd3::battle::LegacyBattleTargetPhaseState>();
        state.group_b_target_phases[0U][1U]->emitter.flags = 0xCAFEU;
        Fixture fixture;
        fixture.prepare_opponent_target_phase(state, 0U, 1U);
        fixture.startup->party[1U].position_x_read_accessible = false;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        context.opponent_target_phase_start_request =
            fixture.opponent_target_phase_request();
        const auto result = dispatch(state, port, context, 0U, 1U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_phase_start_typed_stop &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        actor_base_coordinate_typed_stop &&
                result.target_phase_start.base_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorBaseCoordinateQueryStatus::
                            position_x_read_typed_stop,
            "opponent action six coordinate fault status"
        );
        test.expect_true(
            result.target_phase_start.return_eax == 0x70001FF2U &&
                result.target_phase_start.return_ecx == 0x00505904U &&
                result.target_phase_start.return_edx == 0x89ABCDEFU &&
                result.target_phase_start.return_esp == 0x70001FE0U,
            "opponent action six coordinate fault register residue"
        );
        test.expect_true(
            (*fixture.startup->group_b_lifecycle)[0U]
                        .action_execution.target_phase_resource_token ==
                    0x72000000U &&
                result.target_phase_start.presentation_dwords_zeroed == 0U &&
                state.group_b_target_phases[0U][1U]->emitter.flags == 0xCAFEU &&
                port.count(0x004019A0U) == 0U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x004841B0U) == 0U && port.calls.empty(),
            "opponent action six coordinate fault committed prefix and suffix suppression"
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_phase_start_typed_stop &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        actor_base_coordinate_typed_stop &&
                result.target_phase_start.base_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorBaseCoordinateQueryStatus::
                            position_x_read_typed_stop &&
                result.target_phase_start.return_eax == 0x70001FF2U &&
                result.target_phase_start.return_ecx == 0x00505904U &&
                result.target_phase_start.return_edx == 0x89ABCDEFU &&
                result.target_phase_start.return_esp == 0x70001FE0U &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_execution.target_phase_resource_token ==
                    0x72000000U &&
                result.target_phase_start.presentation_dwords_zeroed == 0U &&
                state.group_b_target_phases[0U][1U]->emitter.flags == 0xCAFEU &&
                port.count(0x004019A0U) == 0U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x004841B0U) == 0U && port.calls.empty(),
            "opponent action six keeps the source token commit and stops before the selected phase clear when target coordinates fault"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        fixture.prepare_opponent_target_phase(state, 0U, 1U);
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        context.opponent_target_phase_start_request =
            fixture.opponent_target_phase_request();
        context.opponent_target_phase_start_request.surface_height = 0x40000001;
        const auto result = dispatch(state, port, context, 0U, 1U);
        const auto& phase = *state.group_b_target_phases[0U][1U];
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_phase_start_typed_stop &&
                result.target_phase_start.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        host_surface_typed_stop &&
                result.target_phase_start.host_surface_calls == 1U &&
                result.target_phase_start.host_surface.row_offsets.status ==
                    openswd3::battle::LegacyBattleRowOffsetStatus::
                        write_out_of_range &&
                result.target_phase_start.return_eip == 0x00433EEFU &&
                result.target_phase_start.decode_calls == 1U &&
                result.target_phase_start.property_query_calls == 1U &&
                result.target_phase_start.presentation_dwords_zeroed == 0x16U &&
                result.target_phase_start.tail_dwords_zeroed == 0U &&
                phase.emitter.flags == 0x57U &&
                (*fixture.startup->group_b_lifecycle)[0U]
                        .action_composition.mode_flags == 0x88U &&
                fixture.startup->render_geometry.surface_width == 640 &&
                fixture.startup->render_geometry.surface_height == 0x40000001 &&
                port.count(0x004019A0U) == 1U &&
                port.count(0x0047CE70U) == 1U &&
                port.count(0x00478710U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x004841B0U) == 0U && port.calls.size() == 2U,
            "opponent action six preserves the selected phase prefix and suppresses the outer suffix at the host-surface write stop"
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
        const auto result = dispatch(state, port, context, 0U, 3U);
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
        const auto result = dispatch(state, port, context, 0U, 99U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_action_kind_typed_stop &&
                result.actor_action_kind_calls == 1U &&
                result.actor_action_kind.status ==
                    openswd3::battle::LegacyBattleActorActionKindStatus::
                        action_kind_read_typed_stop &&
                result.actor_action_kind.return_eip == 0x004786B0U &&
                state.phase_counter == 0x12340000U &&
                state.opponent_special_action == 0x1111U &&
                state.opponent_spawn_count == 0x2222U &&
                result.group_b_opponent_wave_parameters_calls == 0U &&
                result.group_b_iterations == 0U &&
                port.count(0x00476900U) == 0U && port.calls.empty(),
            "opponent action query stops before the action fifteen phase prefix when the source owner is missing"
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
        port.definition_description = {0x41U};
        port.push_release({.eax = 0xABCD0000U});
        port.push_release(
            {.eax = 0x11110000U, .ecx = 0x11112222U, .edx = 0x11113333U}
        );
        port.push_release({.eax = 0xBEEF0000U});
        port.push_release(
            {.eax = 0x22220000U, .ecx = 0x22222222U, .edx = 0x22223333U}
        );
        auto context = fixture.context();
        const auto running = dispatch(state, port, context, 0U, 99U);
        state.phase_counter = 0x8001U;
        const auto completed = dispatch(state, port, context, 0U, 99U);
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
                port.definition_text_release_calls == 4U &&
                port.definition_text_release_requests.size() == 4U &&
                port.count(0x00478220U) == 0U &&
                port.count(0x0045B0E0U) == 0U &&
                port.count(0x004783B0U) == 0U &&
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
        const auto result = dispatch(state, port, context, 0U, 99U);
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
        const auto result = dispatch(state, port, context, 0U, 99U);
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
        state.group_b_action_seventeen_coordinate_x_stack_initial = 0xCAFE0000U;
        state.group_b_action_seventeen_coordinate_y_stack_initial = 0xBABE0000U;
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
        const auto result = dispatch(state, port, context, 0U, 99U);
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U &&
                result.group_b_action_seventeen_frame_calls == 1U &&
                result.group_b_action_seventeen_frame.return_eax == 0U &&
                result.group_b_action_seventeen_frame
                        .coordinate_publish_calls == 1U &&
                result.group_b_action_seventeen_frame.coordinate_x ==
                    0xCAFE0064U &&
                result.group_b_action_seventeen_frame.coordinate_y ==
                    0xBABE0064U &&
                result.group_b_action_seventeen_frame.adjusted_coordinate_x ==
                    0xCAFE004BU &&
                result.group_b_action_seventeen_frame.coordinate_publication
                        .status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            completed &&
                result.group_b_action_seventeen_frame.coordinate_publication
                        .argument_x == 0x004BU &&
                result.group_b_action_seventeen_frame.coordinate_publication
                        .argument_y == 0x0064U &&
                !result.group_b_action_seventeen_frame.coordinate_publication
                     .flags.carry &&
                result.group_b_action_seventeen_frame.coordinate_publication
                    .flags.parity &&
                result.group_b_action_seventeen_frame.coordinate_publication
                    .flags.auxiliary_carry &&
                !result.group_b_action_seventeen_frame.coordinate_publication
                     .flags.zero &&
                result.group_b_action_seventeen_frame.coordinate_publication
                    .flags.sign &&
                !result.group_b_action_seventeen_frame.coordinate_publication
                     .flags.overflow &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                state.overlay_gate == 0U && port.count(0x004763D0U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x0047D860U) == 0U &&
                port.count(0x004787F0U) == 0U &&
                port.count(0x00478600U) == 0U && port.count(0x004785C0U) == 0U,
            "opponent action seventeen preserves both stack-local residues through full-width adjustment and publication flags"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        state.group_b_action_seventeen_coordinate_x_stack_initial = 0xD00D0000U;
        state.group_b_action_seventeen_coordinate_y_stack_initial = 0xBEEF0000U;
        Fixture fixture;
        auto& actor =
            (*fixture.startup->group_b_lifecycle)[0U].action_execution;
        actor.turn_countdown = 7;
        actor.profile_value = 0x1234U;
        actor.position_x = 100U;
        actor.position_y = 200U;
        actor.position_y_read_accessible = false;
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
        const auto result = dispatch(state, port, context, 0U, 99U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_action_seventeen_frame_typed_stop &&
                result.group_b_action_seventeen_frame.status ==
                    openswd3::battle::
                        LegacyBattleGroupBActionSeventeenFrameStatus::
                            actor_current_coordinate_typed_stop &&
                result.group_b_action_seventeen_frame.current_coordinate_query
                        .status ==
                    openswd3::battle::
                        LegacyBattleActorCurrentCoordinateQueryStatus::
                            position_y_read_typed_stop &&
                result.group_b_action_seventeen_frame.coordinate_query_calls ==
                    1U &&
                result.group_b_action_seventeen_frame.coordinate_x ==
                    0xD00D0064U &&
                result.group_b_action_seventeen_frame.coordinate_y ==
                    0xBEEF0000U &&
                result.group_b_action_seventeen_frame.adjusted_coordinate_x ==
                    0U &&
                result.group_b_action_seventeen_frame
                        .coordinate_publish_calls == 0U &&
                result.group_b_action_seventeen_frame.render_calls == 0U &&
                actor.turn_countdown == 7 &&
                state.group_a_action_shared.turn_frame_source_token == 0U &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                state.overlay_gate == 0U && port.count(0x00478600U) == 0U &&
                port.count(0x004785C0U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x0047D860U) == 0U && port.count(0x004787F0U) == 0U,
            "opponent action seventeen preserves the X stack-local prefix and both high words before its suffix"
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
        actor.position_x = 0U;
        actor.position_y = 0U;
        actor.alternate_position_y = 0x2222U;
        actor.publication_destination_dword_write_accessible[6U] = false;
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
        const auto result = dispatch(state, port, context, 0U, 99U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_action_seventeen_frame_typed_stop &&
                result.return_value == 0xFFFFFFE7U &&
                result.group_b_action_seventeen_frame.status ==
                    openswd3::battle::
                        LegacyBattleGroupBActionSeventeenFrameStatus::
                            actor_coordinate_publication_typed_stop &&
                result.group_b_action_seventeen_frame.coordinate_publication
                        .status ==
                    openswd3::battle::
                        LegacyBattleActorCoordinatePublicationStatus::
                            destination_dword_write_typed_stop &&
                result.group_b_action_seventeen_frame.render_calls == 0U &&
                state.group_a_action_shared.turn_frame_source_token == 0U &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                state.overlay_gate == 0U && port.count(0x00478600U) == 0U &&
                port.count(0x004785C0U) == 0U &&
                port.count(0x0047D870U) == 0U &&
                port.count(0x0047D860U) == 0U && port.count(0x004787F0U) == 0U,
            "opponent action seventeen propagates publication faults before its suffix"
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
        const auto result = dispatch(state, port, context, 0U, 99U);
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_action_kind_typed_stop &&
                result.actor_action_kind_calls == 1U &&
                result.actor_action_kind.status ==
                    openswd3::battle::LegacyBattleActorActionKindStatus::
                        action_kind_read_typed_stop &&
                result.actor_action_kind.return_eip == 0x004786B0U &&
                result.group_b_action_seventeen_frame_calls == 0U &&
                result.port_calls == 0U && state.overlay_gate == 0U &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                port.count(0x004763D0U) == 0U && port.count(0x0047D870U) == 0U,
            "opponent action query stops before action seventeen when the source owner is missing"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        Fixture fixture;
        DispatchPort port;
        port.action = 17U;
        auto context = fixture.context();
        const auto result = dispatch(state, port, context, 0U, 99U);
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
        const auto result = dispatch(state, port, context, 0U, 99U);
        test.expect_true(
            result.return_value == 1U && state.group_b_count == 1 &&
                openswd3::compat::u8(state.opponent_processed_counter) == 0U &&
                state.opponent_special_action == 0U &&
                state.post_battle_counter == 0U &&
                port.count(0x0045B0E0U) == 0U &&
                port.count(0x004783B0U) == 0U &&
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
            const auto result = dispatch(state, port, context, 0U, 0U);
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
            const auto result = dispatch(state, port, context, 0U, 99U);
            sparse_cases_match = sparse_cases_match &&
                result.return_value == 0U && result.port_calls == 0U &&
                result.actor_action_kind_calls == 1U;
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
        const auto result = dispatch(state, port, context, 0U, 1U);
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
