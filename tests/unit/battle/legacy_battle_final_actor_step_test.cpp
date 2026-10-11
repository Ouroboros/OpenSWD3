#include "fixed_node_memory_resource.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::compat::u16;
using openswd3::compat::u32;

class FrameRandom final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        return 0U;
    }
};

class FinalStepPort final : public LegacyBattleActionDispatchPort {
public:
    FinalStepPort() = default;

    explicit FinalStepPort(std::pmr::memory_resource& memory) {
        std::destroy_at(&fixed_count_nodes);
        std::construct_at(&fixed_count_nodes, &memory);
    }

    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request);
        }

        const auto found = replies.find(request.callee_token);
        if (found == replies.end() || found->second.empty()) {
            return {};
        }

        const auto reply = found->second.front();
        found->second.pop_front();
        return reply;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(
            std::ranges::count_if(calls, [callee](const auto& request) {
                return request.callee_token == callee;
            })
        );
    }

    [[nodiscard]] openswd3::battle::LegacyBattleAttackOrderRemoveBindings
    attack_order() {
        return {
            .records = attack_order_records,
            .adjacent_intensity_records = {&attack_order_adjacent_record, 1U},
        };
    }

    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };

    std::function<void(const LegacyBattleActionCallRequest&)> on_call;
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 18>
        attack_order_records{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};
};

[[nodiscard]] openswd3::battle::LegacyBattleActorGroupBElementState&
group_b_actor(FinalStepPort& port, const u32 actor_index) {
    if (port.startup->group_b_lifecycle == nullptr) {
        port.startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }
    return (*port.startup->group_b_lifecycle)[actor_index];
}

void bind_group_b_actor_coordinates(
    FinalStepPort& port, const u32 actor_index, const u16 x, const u16 y
) {
    auto& actor = group_b_actor(port, actor_index);
    actor.action_execution.position_x = x;
    actor.action_execution.position_y = y;
}

void bind_group_b_coordinate_resource(
    FinalStepPort& port, const u32 actor_index, const u16 x, const u16 y
) {
    auto& actor = group_b_actor(port, actor_index);
    actor.object_token = openswd3::battle::kLegacyBattleActorGroupBBaseToken +
        actor_index * openswd3::battle::kLegacyBattleActorGroupBElementSize;
    actor.resource_token =
        openswd3::battle::kLegacyBattleActorGroupBResourceStateBaseToken +
        actor_index * 0xA4U;
    actor.resource_bytes[0x62U] = static_cast<openswd3::compat::u8>(x);
    actor.resource_bytes[0x63U] = static_cast<openswd3::compat::u8>(x >> 8U);
    actor.resource_bytes[0x8AU] = static_cast<openswd3::compat::u8>(y);
    actor.resource_bytes[0x8BU] = static_cast<openswd3::compat::u8>(y >> 8U);
}

void test_final_overlay_clearing(openswd3::test::Context& test) {
    struct OverlayCase {
        openswd3::compat::i32 party_count;
        openswd3::compat::i32 enemy_count;
        bool allocated;
        bool queued;
        u32 cleared_party;
        u32 cleared_enemy;
        bool failed;
    };

    constexpr OverlayCase cases[]{
        {3, 2, true, true, 3U, 2U, false},
        {10, 8, true, true, 10U, 8U, false},
        {0, 0, true, true, 0U, 0U, false},
        {-1, -1, true, true, 0U, 0U, false},
        {3, 2, true, false, 0U, 0U, false},
        {3, 2, false, true, 3U, 0U, true},
        {11, 2, true, true, 10U, 0U, true},
        {3, 9, true, true, 3U, 8U, true},
    };

    for (const u32 before : {1U, 0x80000000U, 0xFFFFFFFFU}) {
        for (const auto& scenario : cases) {
            LegacyBattleFinalActorStepState state;
            auto action_storage = std::make_unique<
                openswd3::battle::LegacyBattleActionDispatchState>();
            auto& action = *action_storage;
            action.group_a_count = scenario.party_count;
            action.group_b_count = scenario.enemy_count;
            state.queued_actor_code = scenario.queued ? 8U : 7U;
            state.action_execution_active = 77U;
            state.group_a_slot_values[0U] = 0x12345678U;
            for (auto& actor : action.group_a_action_execution) {
                actor.overlay_render_enabled = before;
            }

            FinalStepPort port;
            if (scenario.allocated) {
                static_cast<void>(group_b_actor(port, 0U));
                for (auto& enemy : *port.startup->group_b_lifecycle) {
                    enemy.action_execution.overlay_render_enabled = before;
                }
            }

            port.attack_order_records[0U].value_00 = 8U;
            port.push(0x00479850U, {.eax = 1U});
            const auto result =
                openswd3::battle::advance_legacy_battle_final_actor_step(
                    state,
                    action,
                    port,
                    port.attack_order(),
                    0U,
                    1U,
                    port.startup.get()
                );
            bool fields_match = true;
            for (std::size_t index = 0U;
                 index < action.group_a_action_execution.size();
                 ++index) {
                fields_match = fields_match &&
                    action.group_a_action_execution[index]
                            .overlay_render_enabled ==
                        (index < scenario.cleared_party ? 0U : before);
            }

            if (scenario.allocated) {
                for (std::size_t index = 0U;
                     index < port.startup->group_b_lifecycle->size();
                     ++index) {
                    fields_match = fields_match &&
                        (*port.startup->group_b_lifecycle)[index]
                                .action_execution.overlay_render_enabled ==
                            (index < scenario.cleared_enemy ? 0U : before);
                }
            }

            const bool gates_published = scenario.queued && !scenario.failed;
            test.expect_true(
                result.status ==
                        (scenario.failed
                             ? openswd3::battle::
                                   LegacyBattleActionDispatchStatus::
                                       actor_overlay_state_missing
                             : openswd3::battle::
                                   LegacyBattleActionDispatchStatus::
                                       completed) &&
                    fields_match && state.group_a_slot_values[0U] == 0U &&
                    port.attack_order_records[0U].value_00 == 0xFFFFFFFFU &&
                    state.queued_actor_code ==
                        (gates_published       ? 0U
                             : scenario.queued ? 8U
                                               : 7U) &&
                    state.action_execution_active ==
                        (gates_published ? 0U : 77U),
                "final actor clears party then enemy overlays with signed bounds, preserving earlier writes and suppressing gate publication on missing storage"
            );
        }
    }
}

void test_final_enemy_profile_reads(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::advance_legacy_battle_final_actor_step;

    constexpr std::array<u16, 6> profiles{
        0U, 1U, 0x7FFFU, 0x8000U, 0xBEEFU, 0xFFFFU
    };

    for (u32 selected = 0U; selected < 8U; ++selected) {
        for (const u16 profile : profiles) {
            LegacyBattleFinalActorStepState state;
            auto action_storage =
                std::make_unique<LegacyBattleActionDispatchState>();
            auto& action = *action_storage;
            action.group_b_count = 8;
            action.packed_actor_counter = 0xAABBCC00U;
            state.coordinate_x = 0xFFFEU;
            state.coordinate_y = 0xFFFAU;
            state.action_delay = 0xFFFAU;
            FinalStepPort port;
            for (u32 index = 0U; index < 8U; ++index) {
                group_b_actor(port, index).action_execution.profile_value =
                    static_cast<u16>(0x2200U + index);
            }

            bind_group_b_coordinate_resource(port, selected, 5U, 6U);
            group_b_actor(port, selected).action_execution.profile_value =
                profile;
            port.attack_order_records[0U].value_00 = selected;
            port.push(0x00479850U, {.eax = 1U});
            port.push(
                0x00480AD0U,
                {.eax = 0x12345678U,
                 .object_flags = (selected & 1U) != 0U ? 0x20U : 0U}
            );
            const auto result = advance_legacy_battle_final_actor_step(
                state,
                action,
                port,
                port.attack_order(),
                selected,
                0U,
                port.startup.get()
            );
            bool profiles_preserved = true;
            for (u32 index = 0U; index < 8U; ++index) {
                const u16 expected = index == selected
                    ? profile
                    : static_cast<u16>(0x2200U + index);
                profiles_preserved = profiles_preserved &&
                    group_b_actor(port, index).action_execution.profile_value ==
                        expected;
            }

            test.expect_true(
                result.status == LegacyBattleActionDispatchStatus::completed &&
                    result.return_value == 1U && profiles_preserved &&
                    state.coordinate_x == 3U && state.coordinate_y == 0U &&
                    state.action_delay ==
                        ((selected & 1U) != 0U ? 0x000EU : 0xFFFDU) &&
                    (profile == 0U ? port.fixed_count_nodes.empty() &&
                             port.object_words[0U][1U] == 0x00010000U
                                   : port.fixed_count_nodes.size() == 1U &&
                             port.fixed_count_nodes.front().words[1U] ==
                                 (0x00010000U | profile)) &&
                    port.attack_order_records[0U].value_00 == 0xFFFFFFFFU &&
                    action.packed_actor_counter == 0xAABBCC01U,
                "enemy finalization reads the selected actual unsigned profile WORD after wrapped coordinates and delay, then counts and removes that actor"
            );
        }
    }

    struct CompletionCase {
        bool profile_readable;
        bool allocation_available;
        bool attack_order_available;
    };

    constexpr CompletionCase cases[]{
        {true, true, true},
        {false, true, true},
        {true, false, true},
        {true, true, false}
    };

    for (const auto& scenario : cases) {
        LegacyBattleFinalActorStepState state;
        auto action_storage =
            std::make_unique<LegacyBattleActionDispatchState>();
        auto& action = *action_storage;
        action.group_b_count = 8;
        action.packed_actor_counter = 0x12340000U;
        action.selection_cache_gate_b = 33U;
        action.action_pending_aux = 44U;
        state.coordinate_x = 0xFFFEU;
        state.coordinate_y = 0xFFFAU;
        state.action_delay = 0xFFFAU;
        openswd3::test::FixedNodeMemoryResource memory;
        memory.allocation_enabled = scenario.allocation_available;
        FinalStepPort port(memory);
        bind_group_b_coordinate_resource(port, 7U, 5U, 6U);
        auto& enemy = group_b_actor(port, 7U).action_execution;
        enemy.profile_value = 0x1234U;
        port.attack_order_records[0U].value_00 = 7U;
        port.battle_message_state() = 55U;
        port.battle_terminal_latch() = 66U;
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x12345678U, .object_flags = 0x20U});
        bool descriptor_prefix = false;
        port.on_call = [&](const LegacyBattleActionCallRequest& request) {
            if (request.callee_token == 0x00480AD0U) {
                descriptor_prefix = state.coordinate_x == 3U &&
                    state.coordinate_y == 0U && state.action_delay == 0xFFFAU &&
                    port.fixed_count_nodes.empty() &&
                    memory.outstanding_blocks == 0U;
                enemy.profile_value = 0x8001U;
                enemy.profile_value_read_accessible = scenario.profile_readable;
            }
        };

        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            scenario.attack_order_available
                ? port.attack_order()
                : openswd3::battle::LegacyBattleAttackOrderRemoveBindings{},
            7U,
            0U,
            port.startup.get()
        );
        const bool counted =
            scenario.profile_readable && scenario.allocation_available;
        const bool completed = counted && scenario.attack_order_available;
        const auto expected_status = !scenario.profile_readable
            ? LegacyBattleActionDispatchStatus::actor_base_state_missing
            : !scenario.allocation_available
            ? LegacyBattleActionDispatchStatus::fixed_count_typed_stop
            : !scenario.attack_order_available
            ? LegacyBattleActionDispatchStatus::attack_order_remove_typed_stop
            : LegacyBattleActionDispatchStatus::completed;
        test.expect_true(
            descriptor_prefix && result.status == expected_status &&
                result.return_value == (completed ? 1U : 0U) &&
                state.coordinate_x == 3U && state.coordinate_y == 0U &&
                state.actor_descriptor_token == 0x12345678U &&
                state.action_delay == 0x000EU &&
                enemy.profile_value == 0x8001U &&
                port.fixed_count_nodes.size() == (counted ? 1U : 0U) &&
                memory.outstanding_blocks == (counted ? 1U : 0U) &&
                (!counted ||
                 port.fixed_count_nodes.front().words[1U] == 0x00018001U) &&
                port.attack_order_records[0U].value_00 ==
                    (completed ? 0xFFFFFFFFU : 7U) &&
                action.packed_actor_counter ==
                    (completed ? 0x12340001U : 0x12340000U) &&
                action.selection_cache_gate_b == 33U &&
                action.action_pending_aux == 44U &&
                port.battle_message_state() == 55U &&
                port.battle_terminal_latch() == 66U,
            "finalization consumes the actual profile after its descriptor predecessor, retaining coordinate and delay prefixes on profile, allocation or attack-order access failure"
        );
    }
}

}  // namespace

void test_battle_final_actor_step(openswd3::test::Context& test) {
    test_final_overlay_clearing(test);
    test_final_enemy_profile_reads(test);
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::advance_legacy_battle_final_actor_step;

    {
        bool all_physical_caller_stops = true;
        for (const u32 group : {1U, 0U}) {
            LegacyBattleFinalActorStepState state;
            LegacyBattleActionDispatchState action;
            FinalStepPort port;
            openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
            snapshot.entry_esp = 0x00140000U + group * 0x100U;
            snapshot.call_stack_writable = false;
            openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
            u32 parent_argument_4 = 0xFFFFFFFFU;
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord
                parent_argument_owner{
                    .token = snapshot.entry_esp + 0x18U,
                    .word = &parent_argument_4,
                };
            const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
                .caller_snapshot = &snapshot,
                .observed = &observed,
                .final_group_a_argument_4 =
                    group == 1U ? &parent_argument_owner : nullptr,
            };
            const auto result = advance_legacy_battle_final_actor_step(
                state,
                action,
                port,
                port.attack_order(),
                0U,
                group,
                port.startup.get(),
                &binding
            );
            const u32 call_site = group == 1U ? 0x0045AA33U : 0x0045ACBFU;
            all_physical_caller_stops = all_physical_caller_stops &&
                result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_frame_caller_typed_stop &&
                observed.status ==
                    openswd3::battle::LegacyBattleActorFrameCallerRunStatus::
                        caller_stack_write_typed_stop &&
                observed.eip == call_site &&
                observed.esp == snapshot.entry_esp && !observed.returned &&
                (group != 1U ||
                 parent_argument_4 ==
                     openswd3::battle::kLegacyBattleActorGroupABaseToken) &&
                port.count(0x00479850U) == 0U;
        }
        test.expect_true(
            all_physical_caller_stops,
            "both real final-actor callers honor injected parent stack write stops before opaque port and parent suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
        snapshot.entry_esp = 0x00140800U;
        openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
        const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
            .caller_snapshot = &snapshot,
            .observed = &observed,
        };
        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            port.attack_order(),
            0U,
            1U,
            port.startup.get(),
            &binding
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_frame_caller_typed_stop &&
                observed.status ==
                    openswd3::battle::LegacyBattleActorFrameCallerRunStatus::
                        parent_argument_write_typed_stop &&
                observed.eip == 0x0045AA2FU &&
                observed.esp == snapshot.entry_esp && !observed.returned &&
                result.group_a_actor_cleanup_calls == 0U &&
                port.count(0x00479850U) == 0U,
            "final Group-A missing physical parent arg_4 stops before the child CALL and every completion suffix"
        );
    }

    {
        bool both_boundary_callers_stopped = true;
        for (const u32 group : {1U, 0U}) {
            LegacyBattleFinalActorStepState state;
            LegacyBattleActionDispatchState action;
            FinalStepPort port;
            openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
            snapshot.entry_esp = 0x00140900U + group * 0x100U;
            openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
            u32 parent_argument_4 = 0xFFFFFFFFU;
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord
                parent_argument_owner{
                    .token = snapshot.entry_esp + 0x18U,
                    .word = &parent_argument_4,
                };
            const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
                .caller_snapshot = &snapshot,
                .observed = &observed,
                .final_group_a_argument_4 =
                    group == 1U ? &parent_argument_owner : nullptr,
            };
            const u32 index = group == 1U ? 10U : 8U;
            const auto result = advance_legacy_battle_final_actor_step(
                state,
                action,
                port,
                port.attack_order(),
                index,
                group,
                port.startup.get(),
                &binding
            );
            both_boundary_callers_stopped = both_boundary_callers_stopped &&
                result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_frame_caller_typed_stop &&
                !observed.returned && observed.eip == 0x0047985BU &&
                observed.esp == snapshot.entry_esp - 0x28U &&
                observed.child.status ==
                    openswd3::battle::LegacyBattleActorFrameEntryStatus::
                        actor_read_typed_stop &&
                observed.child.stopped_token ==
                    (group == 1U ? 0x00522C94U : 0x0053D904U) &&
                observed.admission.child_request.actor_token ==
                    (group == 1U ? 0x005201D8U : 0x0053AE48U) &&
                (group != 1U || parent_argument_4 == 0x005201D8U) &&
                result.group_a_actor_cleanup_calls == 0U &&
                port.count(0x00479850U) == 0U;
        }
        test.expect_true(
            both_boundary_callers_stopped,
            "final A10/B8 callers reach the child first read, retaining A's prior parent argument write without invented physical data"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_a_count = 1;
        FinalStepPort port;
        port.startup->party[0U].progress.presentation_enabled = 1U;
        port.startup->party[0U].configuration.source_runtime_value = 1U;
        port.attack_order_records[0U].value_00 = 8U;
        FrameRandom random;
        openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
        snapshot.entry_esp = 0x00141000U;
        openswd3::battle::LegacyBattleActorFrameEntryRoutePorts frame_ports{};
        frame_ports.random = &random;
        openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
        u32 parent_argument_4 = 0xFFFFFFFFU;
        openswd3::battle::LegacyBattleActorFrameParentArgumentWord
            parent_argument_owner{
                .token = snapshot.entry_esp + 0x18U,
                .word = &parent_argument_4,
            };
        const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
            .caller_snapshot = &snapshot,
            .ports = &frame_ports,
            .observed = &observed,
            .final_group_a_argument_4 = &parent_argument_owner,
        };
        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            port.attack_order(),
            0U,
            1U,
            port.startup.get(),
            &binding
        );
        test.expect_true(
            observed.returned && observed.eax == 1U &&
                observed.eip == 0x0045AA38U &&
                observed.esp == snapshot.entry_esp &&
                parent_argument_4 ==
                    openswd3::battle::kLegacyBattleActorGroupABaseToken &&
                port.startup->party[0U].progress.presentation_enabled == 0U &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U &&
                state.removed_group_a_count == 1U &&
                result.attack_order_remove.has_value() &&
                result.attack_order_remove->removed_index.has_value() &&
                port.count(0x00479850U) == 0U,
            "final Group-A caller takes typed reset EAX-one RET before actor cleanup and completed parent suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_b_count = 1;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 0U, 5U, 6U);
        auto& actor = group_b_actor(port, 0U);
        actor.action_configuration.source_runtime_value = 1U;
        port.startup->enemies[0U].progress.presentation_enabled = 1U;
        port.push(0x00480AD0U, {.eax = 0x12345678U});
        actor.action_execution.profile_value = 0x55U;
        port.attack_order_records[0U].value_00 = 0U;
        FrameRandom random;
        openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
        snapshot.entry_esp = 0x00142000U;
        openswd3::battle::LegacyBattleActorFrameEntryRoutePorts frame_ports{};
        frame_ports.random = &random;
        openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
        u32 parent_argument_0 = 0xFFFFFFFFU;
        u32 parent_argument_4 = 0xFFFFFFFFU;
        openswd3::battle::LegacyBattleActorFrameParentArgumentWord
            argument_0_owner{
                .token = snapshot.entry_esp + 0x14U,
                .word = &parent_argument_0,
            };
        openswd3::battle::LegacyBattleActorFrameParentArgumentWord
            argument_4_owner{
                .token = snapshot.entry_esp + 0x18U,
                .word = &parent_argument_4,
            };
        const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
            .caller_snapshot = &snapshot,
            .ports = &frame_ports,
            .observed = &observed,
            .final_group_b_argument_0 = &argument_0_owner,
            .final_group_b_argument_4 = &argument_4_owner,
        };
        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            port.attack_order(),
            0U,
            0U,
            port.startup.get(),
            &binding
        );
        test.expect_true(
            observed.returned && observed.eax == 1U &&
                observed.eip == 0x0045ACC4U &&
                observed.esp == snapshot.entry_esp && parent_argument_0 == 5U &&
                parent_argument_4 == 6U && state.coordinate_x == 5U &&
                state.coordinate_y == 6U &&
                port.startup->enemies[0U].progress.presentation_enabled == 0U &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U &&
                result.attack_order_remove.has_value() &&
                result.attack_order_remove->removed_index.has_value() &&
                port.battle_message_state() == 0x63U &&
                port.count(0x00479850U) == 0U,
            "final Group-B caller takes typed reset EAX-one RET before coordinates, removal and terminal message"
        );
    }

    {
        bool all_parent_stack_stops = true;
        constexpr u32 parent_esp = 0x00143000U;
        const std::array<u32, 4U> expected_eips{
            0x0045ACD7U, 0x0045ACD8U, 0x0045ACDBU, 0x0045ACDFU
        };
        const std::array<u32, 4U> expected_esps{
            parent_esp, parent_esp - 4U, parent_esp - 8U, parent_esp - 8U
        };
        const std::array<u32, 4U> expected_tokens{
            parent_esp - 4U,
            parent_esp - 8U,
            parent_esp + 0x14U,
            parent_esp + 0x18U
        };
        for (u32 step = 0U; step < expected_eips.size(); ++step) {
            LegacyBattleFinalActorStepState state;
            LegacyBattleActionDispatchState action;
            FinalStepPort port;
            group_b_actor(port, 0U).action_configuration.source_runtime_value =
                1U;
            port.startup->enemies[0U].progress.presentation_enabled = 1U;
            FrameRandom random;
            openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
            snapshot.entry_esp = parent_esp;
            openswd3::battle::LegacyBattleActorFrameEntryRoutePorts ports{};
            ports.random = &random;
            openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
            u32 argument_0 = 0xA0A0A0A0U;
            u32 argument_4 = 0xB4B4B4B4U;
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord
                first_owner{
                    .token = parent_esp + 0x14U,
                    .word = &argument_0,
                    .writable = step != 2U,
                };
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord
                second_owner{
                    .token = parent_esp + 0x18U,
                    .word = &argument_4,
                    .writable = step != 3U,
                };
            const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
                .caller_snapshot = &snapshot,
                .ports = &ports,
                .observed = &observed,
                .final_group_b_argument_0 = &first_owner,
                .final_group_b_argument_4 = &second_owner,
                .final_group_b_first_push_writable = step != 0U,
                .final_group_b_second_push_writable = step != 1U,
            };
            const auto result = advance_legacy_battle_final_actor_step(
                state,
                action,
                port,
                port.attack_order(),
                0U,
                0U,
                port.startup.get(),
                &binding
            );
            const auto& stop = result.final_group_b_physical_stop;
            all_parent_stack_stops = all_parent_stack_stops &&
                observed.returned && observed.eax == 1U &&
                result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_frame_parent_stack_typed_stop &&
                stop.eip == expected_eips[step] &&
                stop.esp == expected_esps[step] &&
                stop.token == expected_tokens[step] &&
                stop.eax == parent_esp + 0x14U &&
                observed.ecx == observed.child.ecx &&
                stop.ecx == observed.ecx && stop.edx == parent_esp + 0x18U &&
                stop.flags_known && stop.flags.zero && stop.flags.parity &&
                !stop.flags.carry && !stop.flags.auxiliary_carry_defined &&
                argument_0 == (step == 3U ? 0U : 0xA0A0A0A0U) &&
                argument_4 == 0xB4B4B4B4U && state.coordinate_x == 0U;
        }
        test.expect_true(
            all_parent_stack_stops,
            "final Group-B EAX-one suffix preserves ordered pointer PUSH and two parent stack zero writes at four independent physical stops"
        );
    }

    {
        bool all_coordinate_child_stops = true;
        constexpr u32 parent_esp = 0x00144000U;
        const std::array<u32, 3U> expected_eips{
            0x00475873U, 0x0047587BU, 0x0047588CU
        };
        const std::array<u32, 3U> expected_tokens{
            0x62U, parent_esp + 0x14U, parent_esp + 0x18U
        };
        for (u32 stop_site = 0U; stop_site < 3U; ++stop_site) {
            LegacyBattleFinalActorStepState state;
            LegacyBattleActionDispatchState action;
            FinalStepPort port;
            bind_group_b_coordinate_resource(port, 0U, 5U, 6U);
            auto& actor = group_b_actor(port, 0U);
            actor.action_configuration.source_runtime_value = 1U;
            if (stop_site == 0U) {
                actor.resource_token = 0U;
            }
            port.startup->enemies[0U].progress.presentation_enabled = 1U;
            FrameRandom random;
            openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
            snapshot.entry_esp = parent_esp;
            openswd3::battle::LegacyBattleActorFrameEntryRoutePorts ports{};
            ports.random = &random;
            openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
            u32 argument_0 = 0xA0A0A0A0U;
            u32 argument_4 = 0xB4B4B4B4U;
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord
                first_owner{
                    .token = snapshot.entry_esp + 0x14U,
                    .word = &argument_0,
                };
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord
                second_owner{
                    .token = snapshot.entry_esp + 0x18U,
                    .word = &argument_4,
                };
            const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
                .caller_snapshot = &snapshot,
                .ports = &ports,
                .observed = &observed,
                .final_group_b_argument_0 = &first_owner,
                .final_group_b_argument_4 = &second_owner,
                .final_group_b_first_output_writable = stop_site != 1U,
                .final_group_b_second_output_writable = stop_site != 2U,
            };
            const auto result = advance_legacy_battle_final_actor_step(
                state,
                action,
                port,
                port.attack_order(),
                0U,
                0U,
                port.startup.get(),
                &binding
            );
            const auto& stop = result.final_group_b_physical_stop;
            all_coordinate_child_stops = all_coordinate_child_stops &&
                observed.returned && observed.eax == 1U &&
                result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_coordinate_offset_typed_stop &&
                argument_0 == (stop_site == 2U ? 5U : 0U) && argument_4 == 0U &&
                state.coordinate_x == 0U && state.coordinate_y == 0U &&
                stop.eip == expected_eips[stop_site] &&
                stop.esp == parent_esp - 0x0CU &&
                stop.token == expected_tokens[stop_site] &&
                stop.eax ==
                    (stop_site == 0U
                         ? 0U
                         : parent_esp + (stop_site == 1U ? 0x14U : 0x18U)) &&
                stop.ecx ==
                    (stop_site == 2U
                         ? actor.resource_token
                         : observed.admission.child_request.actor_token) &&
                stop.edx ==
                    ((parent_esp + 0x18U) & 0xFFFF0000U) +
                        (stop_site == 0U ? ((parent_esp + 0x18U) & 0xFFFFU)
                             : stop_site == 1U ? 5U
                                               : 6U) &&
                stop.flags_known && stop.flags.zero && !stop.flags.carry &&
                port.count(0x00480AD0U) == 0U;
        }
        test.expect_true(
            all_coordinate_child_stops,
            "final Group-B coordinate child preserves both cleared parent words after resource fault and only the first output after the second output write fault"
        );
    }

    {
        bool both_parent_reads_stopped = true;
        constexpr u32 parent_esp = 0x00145000U;
        for (u32 read = 0U; read < 2U; ++read) {
            LegacyBattleFinalActorStepState state;
            LegacyBattleActionDispatchState action;
            FinalStepPort port;
            bind_group_b_coordinate_resource(port, 0U, 5U, 6U);
            auto& actor = group_b_actor(port, 0U);
            actor.action_configuration.source_runtime_value = 1U;
            port.startup->enemies[0U].progress.presentation_enabled = 1U;
            FrameRandom random;
            openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
            snapshot.entry_esp = parent_esp;
            openswd3::battle::LegacyBattleActorFrameEntryRoutePorts ports{};
            ports.random = &random;
            openswd3::battle::LegacyBattleActorFrameCallerRunResult observed{};
            u32 argument_0 = 0xFFFFFFFFU;
            u32 argument_4 = 0xFFFFFFFFU;
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord first{
                .token = parent_esp + 0x14U,
                .word = &argument_0,
                .readable = read != 0U,
            };
            openswd3::battle::LegacyBattleActorFrameParentArgumentWord second{
                .token = parent_esp + 0x18U,
                .word = &argument_4,
                .readable = read != 1U,
            };
            const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
                .caller_snapshot = &snapshot,
                .ports = &ports,
                .observed = &observed,
                .final_group_b_argument_0 = &first,
                .final_group_b_argument_4 = &second,
            };
            const auto result = advance_legacy_battle_final_actor_step(
                state,
                action,
                port,
                port.attack_order(),
                0U,
                0U,
                port.startup.get(),
                &binding
            );
            const auto& stop = result.final_group_b_physical_stop;
            both_parent_reads_stopped = both_parent_reads_stopped &&
                observed.returned && observed.eax == 1U &&
                result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_frame_parent_stack_typed_stop &&
                argument_0 == 5U && argument_4 == 6U &&
                state.coordinate_x == 0U && state.coordinate_y == 0U &&
                stop.eip == (read == 0U ? 0x0045ACE8U : 0x0045ACEDU) &&
                stop.esp == parent_esp &&
                stop.token == parent_esp + (read == 0U ? 0x14U : 0x18U) &&
                stop.eax == parent_esp + 0x18U &&
                stop.ecx ==
                    ((actor.resource_token & 0xFFFF0000U) |
                     (read == 0U ? (actor.resource_token & 0xFFFFU) : 5U)) &&
                stop.edx == ((parent_esp + 0x18U) & 0xFFFF0000U) + 6U &&
                stop.flags_known && stop.flags.zero && !stop.flags.carry &&
                port.count(0x00480AD0U) == 0U;
        }
        test.expect_true(
            both_parent_reads_stopped,
            "final Group-B parent reads two returned stack words independently after callee RET8 without undoing either output"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 0U, 5U, 6U);
        group_b_actor(port, 0U).action_configuration.source_runtime_value = 1U;
        port.startup->enemies[0U].progress.presentation_enabled = 1U;
        port.push(0x00480AD0U, {.eax = 0U});
        FrameRandom random;
        openswd3::battle::LegacyBattleActorFrameEntryRequest snapshot{};
        snapshot.entry_esp = 0x00146000U;
        openswd3::battle::LegacyBattleActorFrameEntryRoutePorts ports{};
        ports.random = &random;
        u32 aliased_word = 0xFFFFFFFFU;
        openswd3::battle::LegacyBattleActorFrameParentArgumentWord first{
            .token = snapshot.entry_esp + 0x14U,
            .word = &aliased_word,
        };
        openswd3::battle::LegacyBattleActorFrameParentArgumentWord second{
            .token = snapshot.entry_esp + 0x18U,
            .word = &aliased_word,
        };
        const openswd3::battle::LegacyBattleActorFrameCallerBinding binding{
            .caller_snapshot = &snapshot,
            .ports = &ports,
            .final_group_b_argument_0 = &first,
            .final_group_b_argument_4 = &second,
        };
        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            port.attack_order(),
            0U,
            0U,
            port.startup.get(),
            &binding
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        final_actor_descriptor_typed_stop &&
                aliased_word == 6U && state.coordinate_x == 6U &&
                state.coordinate_y == 6U && port.count(0x00480AD0U) == 1U,
            "final Group-B parent rereads the canonical aliased stack word after both coordinate outputs rather than using stale child-local values"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 99U, 1U
        );
        test.expect_true(
            result.return_value == 0U &&
                result.status == LegacyBattleActionDispatchStatus::completed,
            "group A validity failure returns before the first indexed flag access"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.group_a_completion_flags[1] = 1U;
        state.queued_actor_code = 9U;
        state.actor_order = {9U, 77U, 88U};
        action.group_a_count = 2;
        action.group_b_count = 1;
        action.phase_counter = 0x00010000U;
        action.opponent_workspace.fill(0xFFFFFFFFU);
        state.actor_runtime_records[1].fill(0xFFFFFFFFU);
        FinalStepPort port;
        port.actor_metric_state().priority_actor_index = 9U;
        action.group_a_action_execution[0U].position_x = 1U;
        action.group_a_action_execution[0U].position_y = 1U;
        bind_group_b_actor_coordinates(port, 0U, 2U, 2U);
        action.group_a_action_execution[0U].overlay_render_enabled = 1U;
        group_b_actor(port, 0U).action_execution.overlay_render_enabled =
            0xFFFFFFFFU;
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x0047F340U, {.eax = 1U});
        port.attack_order_records[0].value_00 = 9U;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 1U, 1U, port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && action.group_a_count == 1 &&
                ((action.packed_actor_counter >> 8U) & 0xFFU) == 1U &&
                action.opponent_workspace[16] == 0U &&
                action.opponent_workspace[23] == 0U &&
                action.opponent_workspace[15] == 0xFFFFFFFFU &&
                state.queued_actor_code == 0U &&
                port.actor_metric_state().priority_actor_index == 0xFFFFFFFFU &&
                result.actor_availability_block.has_value() &&
                result.actor_availability_block->status ==
                    openswd3::battle::LegacyBattleActorAvailabilityBlockStatus::
                        completed &&
                state.group_a_availability_blocks[1U].value == 1U &&
                port.battle_message_state() == 1U &&
                state.actor_order[0] == 77U && state.actor_order[9] == 0U &&
                action.opponent_workspace[3] == 1U &&
                state.actor_runtime_records[1][4] == 0U &&
                action.group_a_action_execution[0U].overlay_render_enabled ==
                    0U &&
                group_b_actor(port, 0U)
                        .action_execution.overlay_render_enabled == 0U &&
                port.count(0x0045B0E0U) == 0U &&
                result.attack_order_remove.has_value() &&
                result.attack_order_remove->removed_index.has_value() &&
                port.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                port.count(0x004783B0U) == 0U,
            "group A completion clears the counted workspace and runs common actor cleanup"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.removed_group_a_count = 0U;
        action.group_a_count = 1;
        action.opponent_workspace.fill(0xFFFFFFFFU);
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 1U, port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && state.removed_group_a_count == 1U &&
                action.selection_cache_gate_b == 1U &&
                action.action_pending_aux == 1U &&
                port.battle_message_state() == 0x67U &&
                std::ranges::all_of(
                    action.opponent_workspace,
                    [](const u32 value) { return value == 0U; }
                ) &&
                port.count(0x0047F340U) == 0U,
            "group A removed-count threshold takes the fixed workspace finalization return"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.group_a_slot_values[0] = 9U;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        port.attack_order_records[17].value_00 = 8U;
        auto attack_order = port.attack_order();
        attack_order.adjacent_intensity_records = {};

        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, attack_order, 0U, 1U, port.startup.get()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        attack_order_remove_typed_stop &&
                state.removed_group_a_count == 1U &&
                state.group_a_slot_values[0] == 9U &&
                result.group_a_actor_cleanup_calls == 1U &&
                port.count(0x004750C0U) == 0U &&
                result.attack_order_remove->status ==
                    openswd3::battle::LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop,
            "final actor removal stop preserves validation and actor cleanup prefix then blocks slot clearing"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.group_a_completion_flags[0] = 1U;
        action.group_a_count = 20;
        action.phase_counter = 0x00010000U;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 1U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        final_actor_workspace_typed_stop &&
                ((action.packed_actor_counter >> 8U) & 0xFFU) == 1U &&
                action.group_a_count == 20 && port.count(0x0045B0E0U) == 0U,
            "group A oversized workspace stops at the first real zero write"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_a_count = 2;
        state.removed_group_a_count = 0U;
        state.group_a_availability_blocks[0U].write_accessible = false;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x0047F340U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 1U, port.startup.get()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        actor_availability_block_typed_stop &&
                result.actor_availability_block.has_value() &&
                result.actor_availability_block->status ==
                    openswd3::battle::LegacyBattleActorAvailabilityBlockStatus::
                        actor_write_typed_stop &&
                state.action_execution_active == 0U &&
                action.opponent_workspace[2] == 0U,
            "group A typed write stop preserves the completed prefix and suppresses the caller suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0xFFFFFFFFU, 0U
        );
        test.expect_true(
            result.return_value == 0U,
            "group B all-one index returns before object validation"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        FinalStepPort port;
        port.push(0x00479850U, {.eax = 1U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_coordinate_offset_typed_stop &&
                state.coordinate_x == 0U && state.coordinate_y == 0U &&
                port.count(0x00480AD0U) == 0U,
            "group B coordinate owner stop preserves validation then blocks descriptor lookup"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_b_count = 2;
        state.coordinate_x = 0xFFFEU;
        state.coordinate_y = 3U;
        state.group_b_reset_word = 9U;
        FinalStepPort port;
        bind_group_b_actor_coordinates(port, 0U, 1U, 1U);
        bind_group_b_coordinate_resource(port, 2U, 5U, 6U);
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x12345678U, .object_flags = 0x20U});
        group_b_actor(port, 2U).action_execution.profile_value = 0x55U;
        port.push(0x0047CE80U, {.eax = 0U});
        port.attack_order_records[0].value_00 = 2U;
        const auto result = advance_legacy_battle_final_actor_step(
            state,
            action,
            port,
            port.attack_order(),
            2U,
            0xFFFFFFFFU,
            port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && state.coordinate_x == 3U &&
                state.coordinate_y == 9U && state.action_delay == 0x14U &&
                state.actor_descriptor_token == 0x12345678U &&
                action.group_b_count == 1 &&
                (action.packed_actor_counter & 0xFFU) == 0U &&
                state.group_b_reset_word == 0U &&
                port.count(0x0045B0E0U) == 0U &&
                result.attack_order_remove.has_value() &&
                result.attack_order_remove->removed_index.has_value() &&
                port.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                result.fixed_count.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::
                        allocated_node &&
                port.count(0x004783B0U) == 0U &&
                port.fixed_count_nodes.size() == 1U &&
                port.fixed_count_nodes.front().words[1U] == 0x00010055U &&
                port.count(0x00475870U) == 0U,
            "every non-one selector reads group B coordinates before descriptor, action and reset suffix"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        action.group_b_count = 1;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 0U, 0U, 0U);
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x12345678U});
        group_b_actor(port, 0U).action_execution.profile_value = 0x55U;
        port.attack_order_records[0].value_00 = 0U;
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 0U, port.startup.get()
        );
        test.expect_true(
            result.return_value == 1U && action.group_b_count == 1 &&
                action.selection_cache_gate_b == 1U &&
                action.action_pending_aux == 1U &&
                port.battle_message_state() == 0x63U &&
                port.battle_terminal_latch() == 0U,
            "final group-B actor publishes shared message 99 and clears the terminal latch"
        );
    }

    {
        LegacyBattleFinalActorStepState state;
        LegacyBattleActionDispatchState action;
        state.coordinate_x = 7U;
        FinalStepPort port;
        bind_group_b_coordinate_resource(port, 0U, 2U, 0U);
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0U});
        const auto result = advance_legacy_battle_final_actor_step(
            state, action, port, port.attack_order(), 0U, 0U, port.startup.get()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        final_actor_descriptor_typed_stop &&
                state.coordinate_x == 9U && port.count(0x00475870U) == 0U &&
                port.fixed_count_nodes.empty(),
            "group B null descriptor stops at the first object-field access after coordinates"
        );
    }
}
