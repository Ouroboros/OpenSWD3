#include "openswd3/battle/legacy_battle_group_b_frame.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort {
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
        if (request.callee_token == 0x004786E0U) {
            return {.eax = action_target};
        }
        return default_reply;
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
    u16 action_target{};
    LegacyBattleActionCallReply default_reply{};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
};

class ActionStreamProvider final
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
    ActionStreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;
    RandomPort random;
    SoundPort sound;
    CountdownFlags countdown_flags;
    std::array<u8, 16> flags{};
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};

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
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .bounded_random = random,
            .indicator_sound = sound,
            .countdown_flags = countdown_flags,
            .internal_flags = flags,
            .attack_order_records = attack_order_records,
            .status_indicator_action_eax_snapshot = 0U,
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

void test_battle_group_b_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchStatus;
    using openswd3::battle::LegacyBattleGroupBFrameState;

    {
        LegacyBattleGroupBFrameState state;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 8U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_b_index_typed_stop &&
                result.port_calls == 0U,
            "group B frame stops at first source actor access"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.shared.action.frame_effect.primary_suppression = 1U;
        state.shared.action.group_a_to_actor[0] = 3U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x004786D0U, {.eax = 1U});
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x1234U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 1U &&
                has_call_argument(port, 0x00478B60U, 1U, 1U) &&
                has_call_argument(port, 0x00479850U, 0U, 0x0052D680U) &&
                state.shared.action.group_a_to_actor[0] == 0xFFFFFFFFU &&
                state.shared.action.overlay_gate == 1U,
            "disabled group B body still runs effect and complete EAX final actor suffix"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.shared.action.frame_effect.primary_suppression = 1U;
        state.shared.action.group_a_to_actor[0] = 3U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x004786D0U, {.eax = 1U});
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        final_actor_descriptor_typed_stop &&
                state.shared.action.group_a_to_actor[0] == 3U &&
                state.shared.action.overlay_gate == 0U,
            "group B frame propagates final actor descriptor stop before cleanup"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.update_gate_argument = 0x55U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x004755E0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 2U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                port.count(0x0047DAD0U) == 1U &&
                has_call_argument(port, 0x004755E0U, 0U, 0x55U) &&
                result.attack_order_calls == 1U &&
                result.attack_order.written_index == 0U &&
                fixture.attack_order_records[0].value_00 == 2U &&
                fixture.attack_order_records[0].value_08 == 2U &&
                port.count(0x0045EDF0U) == 0U &&
                state.shared.action_block_gate == 0x5650U,
            "live opponent update directly appends its index to the shared attack order when update and message gates allow"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.update_gate_argument = 0x55U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x004755E0U, {.eax = 1U});
        auto context = fixture.context();
        context.attack_order_records = {};

        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 2U
            );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::attack_order_typed_stop &&
                result.attack_order.status ==
                    openswd3::battle::LegacyBattleAttackOrderEntryStatus::
                        record_typed_stop &&
                result.attack_order.return_eax == 0x00524788U &&
                port.count(0x004786A0U) == 0U,
            "attack-order typed stop preserves the opponent update prefix then blocks the remaining frame path"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 3U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x0047F920U, {.eax = 1U});
        port.push(0x00478850U, {.eax = 0xA5A55A5AU});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 3U
            );
        test.expect_true(
            result.return_value == 0xA5A55A5AU &&
                state.shared.final_actor_step.queued_actor_code == 4U &&
                state.shared.action.active_effect_target == 0xFFFFFFFFU &&
                port.count(0x004786D0U) == 0U,
            "completed opponent queue path returns reset actor full EAX before common suffix"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.phase_mode = 1U;
        state.shared.action_side = 1U;
        state.shared.action.group_b_count = 5;
        state.shared.action.opponent_processed_counter = 2U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x004786A0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.phase_progress == 3U &&
                state.shared.target_ready_gate == 1U &&
                port.count(0x00478A70U) == 1U && port.count(0x00483820U) == 0U,
            "phase mode side path publishes zero and jumps directly to common action stage"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.shared.action.group_b_count = 3;
        Fixture fixture;
        DispatchPort port;
        port.push(0x00483820U, {.eax = 1U});
        port.push(0x00439070U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.selection_initialized == 1U &&
                state.random_target_index == 1U &&
                state.shared.action_side == 1U && port.count(0x00439070U) == 1U,
            "selection mode chooses random nonterminal group B peer and records side"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0x7AU};
        state.stale_action_profile_edx = 0x11223300U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x004786A0U, {.eax = 0U});
        port.push(0x00476330U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 0U &&
                has_call_argument(port, 0x00476330U, 0U, 0x00525508U) &&
                has_call_argument(port, 0x00476330U, 1U, 0x1122337AU) &&
                has_call_argument(port, 0x00478710U, 1U, 0x11U) &&
                has_call_argument(port, 0x0047D860U, 1U, 2U),
            "profile byte replaces only stale EDX low byte before status action query"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0U};
        state.status_words[0] = 0xE002U;
        state.special_selection_pending = 1U;
        state.opponent_text_present[0] = 1U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x004786A0U, {.eax = 0U});
        port.push(0x004761D0U, {.eax = 0x77U});
        port.push(0x0047D880U, {.eax = 1U});
        port.push(0x0047D8D0U, {.eax = 1U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x004786A0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.return_value == 0U && state.status_action_value == 0x77U &&
                state.random_target_index == 2U &&
                state.shared.action_side == 1U &&
                state.special_selection_pending == 0U &&
                state.special_action_latch == 1U && state.phase_mode == 1U &&
                port.count(0x00476140U) == 0U &&
                port.count(0x004761D0U) == 1U &&
                port.count(0x004698E0U) == 1U &&
                has_call_argument(port, 0x00478A70U, 1U, 2U),
            "negative packed status uses signed callee, both action modes, text and side target"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0U};
        state.shared.action.group_a_count = 0;
        Fixture fixture;
        DispatchPort port;
        port.action = 100U;
        port.action_target = 0U;
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                port.count(0x00455D60U) == 0U &&
                port.count(0x004786B0U) == 1U &&
                state.selection_initialized == 0U &&
                state.shared.action_block_gate == 0U &&
                state.shared.action.active_effect_target == 0xFFFFFFFFU &&
                state.status_words[0] == 0U,
            "idle opponent directly calls closed opponent dispatcher and runs common cleanup"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0U};
        Fixture fixture;
        DispatchPort port;
        port.action = 0U;
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U &&
                state.shared.action_block_gate == 1U &&
                port.count(0x004786B0U) == 1U,
            "opponent dispatcher incomplete return publishes call-stage stale EBX one"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0U};
        state.shared.action.group_a_count = 1;
        state.group_a_completion_words[0] = 1U;
        state.group_a_completion_slots[0] = 9U;
        state.completion_value_table[16] = 0x1234U;
        Fixture fixture;
        DispatchPort port;
        port.action = 100U;
        port.action_target = 0U;
        port.push(0x0047F920U, {.eax = 0U});
        port.push(0x0047F920U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x00478B40U, {.eax = 1U});
        port.push(0x00478370U, {.eax = 0xABCD0000U});
        port.push(0x00487C10U, {.eax = 0x00620000U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                state.group_a_completion_words[0] == 0U &&
                state.group_a_completion_slots[0] == 0U &&
                result.player_item_calls == 1U &&
                result.player_item.return_token == 0x0062000CU &&
                port.world_item_list_state().player_inventory.front().item_id ==
                    0x1234U &&
                port.world_item_list_state()
                        .player_inventory.front()
                        .quantity_a == 1U &&
                port.count(0x004750C0U) == 1U,
            "all-target completion keeps query EAX high word and table low word"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0U};
        state.shared.action.group_a_count = 0;
        state.completion_rect_right = 2;
        state.completion_rect_bottom = 2;
        state.completion_surface_token = 0x1234U;
        std::array<u16, 2> pixels{};
        state.completion_surface = pixels;
        Fixture fixture;
        DispatchPort port;
        port.action = 100U;
        port.action_target = 0U;
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x0047F360U, {.eax = 1U});
        port.push(0x0047F150U, {.eax = 1U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::framebuffer_typed_stop &&
                pixels[0] == 0xFFFFU && pixels[1] == 0xFFFFU &&
                state.shared.action.group_a_to_actor[0] == 0U &&
                state.completion_selected == 0xFFFFFFFFU &&
                state.completion_gate == 1U && port.count(0x004786D0U) == 0U,
            "completion surface writes owned prefix after mapping side effects then stops"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_bytes = {0U};
        state.completion_rect_right = 1;
        state.completion_rect_bottom = 1;
        std::array<u16, 1> pixels{0x1234U};
        state.completion_surface = pixels;
        Fixture fixture;
        DispatchPort port;
        port.action = 100U;
        port.action_target = 0U;
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x004786A0U, {.eax = 1U});
        port.push(0x0047F360U, {.eax = 1U});
        port.push(0x0047F150U, {.eax = 1U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::framebuffer_typed_stop &&
                pixels[0] == 0x1234U &&
                state.shared.action.group_a_to_actor[0] == 0U &&
                state.completion_gate == 1U,
            "zero completion surface token stops before first byte but after state publication"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.pending_effect_ids[1] = 7U;
        state.pending_effect_argument = 0x66U;
        state.pending_effect_frame.primary[1].complete = 1U;
        state.pending_effect_frame.primary[1].source_value = 0x77U;
        state.shared.action.group_a_to_actor[1] = 5U;
        Fixture fixture;
        DispatchPort port;
        port.push(0x00479850U, {.eax = 1U});
        port.push(0x00480AD0U, {.eax = 0x1234U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 1U
            );
        test.expect_true(
            result.return_value == 1U &&
                state.pending_effect_ids[1] == 0xFFFFFFFFU &&
                state.pending_effect_frame.primary[1].source_value == 0U &&
                port.count(0x004599B0U) == 0U &&
                has_call_argument(port, 0x00479850U, 0U, 0x00532CD0U) &&
                state.final_actor_state[1] == 0U &&
                state.final_actor_targets[1] == 0xFFFFFFFFU &&
                state.shared.queued_selection_word == 0xFFFFU,
            "pending effect clears only on one and final actor success resets mapped slots"
        );
    }

    {
        LegacyBattleGroupBFrameState state;
        state.frame_enabled = 1U;
        state.shared.action.active_effect_target = 0U;
        state.selection_initialized = 1U;
        state.action_profile_index = 1U;
        state.action_profile_bytes = {0U};
        Fixture fixture;
        DispatchPort port;
        port.push(0x004786A0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result =
            openswd3::battle::advance_legacy_battle_group_b_frame(
                state, port, context, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::target_table_typed_stop &&
                state.phase_mode == 0U && port.count(0x00476330U) == 0U,
            "profile table stops at first indexed byte after preserving phase reset"
        );
    }
}
