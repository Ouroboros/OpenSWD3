#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const LegacyBattleActionCallReply reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        if (request.callee_token == 0x004786B0U) {
            return {.eax = action};
        }
        if (request.callee_token == 0x004786C0U) {
            return {.eax = fallback_action};
        }
        if (request.callee_token == 0x0047CE80U) {
            return {.eax = terminal_return};
        }
        if (request.callee_token == 0x00489E90U) {
            return {.eax = 0x90000000U};
        }
        if (request.callee_token == 0x00480AD0U) {
            return {.eax = 0xA0000000U};
        }
        if (request.callee_token == 0x00478620U) {
            return {
                .eax = 0x72000000U,
                .outputs = {0x73000000U, 0x20U, 0x50U},
            };
        }
        if (request.callee_token == 0x00478470U) {
            return {.outputs = {0x30U, 0x40U}};
        }
        if (request.callee_token == 0x004019A0U) {
            return {.eax = 0x74000000U, .resource_words = decoded_pixels};
        }
        if (request.callee_token == 0x004783B0U) {
            auto reply = default_reply;
            reply.publish_metric_word = true;
            reply.metric_word = 1U;
            return reply;
        }
        return default_reply;
    }

    [[nodiscard]] openswd3::battle::
        LegacyBattleGroupASummonMaterializationCallReply
        invoke_group_a_summon_materialization(
            const openswd3::battle::
                LegacyBattleGroupASummonMaterializationCallRequest& request
        ) override {
        summon_materialization_calls.push_back(request);
        auto reply = openswd3::battle::LegacyBattleActionDispatchPort::
            invoke_group_a_summon_materialization(request);
        if (request.call ==
            openswd3::battle::LegacyBattleGroupASummonMaterializationCall::
                load_profile) {
            reply.profile_record = summon_profile;
        }
        return reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageCallReply
    invoke_text_message(
        const openswd3::battle::LegacyBattleTextMessageCallRequest& request
    ) override {
        text_message_calls.push_back(request);
        if (request.call ==
            openswd3::battle::LegacyBattleTextMessageCall::allocate) {
            const u32 token = next_text_message_token;
            next_text_message_token += 0x24U;
            return {.eax = token, .edx = request.edx};
        }
        return {.eax = text_length};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleRetreatCommitCallReply
    invoke_retreat_commit(
        const openswd3::battle::LegacyBattleRetreatCommitCallRequest& request
    ) override {
        retreat_commit_calls.push_back(request);
        if (retreat_commit_replies.empty()) {
            return {.eax = 1U};
        }
        const auto reply = retreat_commit_replies.front();
        retreat_commit_replies.pop_front();
        return reply;
    }

    void push(const u32 callee, const LegacyBattleActionCallReply reply) {
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
    u16 fallback_action{};
    u32 terminal_return{};
    LegacyBattleActionCallReply default_reply{.eax = 1U};
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<u16> decoded_pixels = std::vector<u16>(0x20U * 0x50U, 0x1234U);
    std::vector<
        openswd3::battle::LegacyBattleGroupASummonMaterializationCallRequest>
        summon_materialization_calls;
    openswd3::battle::LegacyBattleGroupASummonProfileRecord summon_profile{};
    std::vector<openswd3::battle::LegacyBattleTextMessageCallRequest>
        text_message_calls;
    u32 next_text_message_token{0x71000000U};
    u32 text_length{4U};
    std::deque<openswd3::battle::LegacyBattleRetreatCommitCallReply>
        retreat_commit_replies;
    std::vector<openswd3::battle::LegacyBattleRetreatCommitCallRequest>
        retreat_commit_calls;
};

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    ActionStreamProvider() {
        constexpr std::array<u16, 8> words{
            0x5246U,
            0x0066U,
            0x5041U,
            0U,
            0x5859U,
            0U,
            0U,
            0x4544U,
        };
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes,
        };
    }

    std::vector<u8> bytes;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        bytes.resize(32U * 32U * sizeof(u16));
        for (std::size_t offset = 0U; offset < bytes.size(); offset += 2U) {
            bytes[offset] = 0x34U;
            bytes[offset + 1U] = 0x12U;
        }
    }

    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        piece = {
            .source =
                openswd3::rendering::LegacyBlitSource{
                    .bytes = bytes,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 32U,
            .height = 32U,
        };
        return true;
    }

    std::vector<u8> bytes;
};

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        return value;
    }
    u32 value{};
};

class SoundPort final
    : public openswd3::battle::LegacyBattleIndicatorSoundPort {
public:
    void play_indicator_sound(u16, u16) override {
        ++calls;
    }
    u32 calls{};
};

class CountdownFlags final
    : public openswd3::rendering::LegacyCountdownFlagPorts {
public:
    explicit CountdownFlags(std::array<u8, 16>& bytes) : bytes_(bytes) {}

    [[nodiscard]] bool query_internal_flag(const u32 index) noexcept override {
        return (bytes_[index >> 3U] & static_cast<u8>(1U << (index & 7U))) !=
            0U;
    }

    void set_internal_flag(const u32 index) noexcept override {
        bytes_[index >> 3U] |= static_cast<u8>(1U << (index & 7U));
    }

private:
    std::array<u8, 16>& bytes_;
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
    std::array<u8, 16> flags{};
    CountdownFlags countdown_flags{flags};
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleStartupResetBlocks& startup_reset{
        startup.reset
    };
    openswd3::battle::LegacyBattleTextMessageState& text_messages{
        startup.text_messages
    };
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};
    std::array<u32, 0x32> attack_order_party_sources{};
    u32 attack_order_primary_gate{};
    u32 attack_order_secondary_gate{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};

    Fixture() {
        startup.party[0U].role_id = 1U;
        startup.party[0U].active = 1U;
        startup.party[0U].configuration.actor_record_token = 0x005029D0U;
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
            .startup = &startup,
            .startup_reset = &startup_reset,
            .text_messages = &text_messages,
            .attack_order_records = attack_order_records,
            .attack_order_party_sources = attack_order_party_sources,
            .attack_order_primary_gate = &attack_order_primary_gate,
            .attack_order_secondary_gate = &attack_order_secondary_gate,
            .attack_order_adjacent_record = &attack_order_adjacent_record,
            .status_indicator_action_eax_snapshot = 0U,
            .group_a_skip_primary = {},
            .group_a_skip_secondary = {},
            .scripted_resource_release_test_compat = true,
        };
    }
};

void set_summon_profile_word(
    openswd3::battle::LegacyBattleGroupASummonProfileRecord& record,
    const std::size_t offset,
    const u16 value
) noexcept {
    record[offset] = static_cast<std::byte>(static_cast<u8>(value));
    record[offset + 1U] = static_cast<std::byte>(static_cast<u8>(value >> 8U));
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

void test_battle_action_dispatch(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionDispatchState;
    using openswd3::battle::LegacyBattleActionDispatchStatus;

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 10U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        group_a_index_typed_stop &&
                result.port_calls == 0U,
            "group A overflow stops at first actor object query"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 1U;
        port.terminal_return = 1U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 99U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 1U && result.port_calls == 2U,
            "terminal actor returns one before any target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 0U;
        port.fallback_action = 5U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.action_code == 5U && result.return_value == 1U &&
                port.count(0x004786C0U) == 1U,
            "zero primary action dispatches the low word returned by fallback query"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.stored_group_b_index = 2U;
        state.stored_group_a_index = 3U;
        Fixture fixture;
        DispatchPort port;
        port.action = 0x63U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U && result.terminal_resets == 1U &&
                state.stored_group_b_index == 0xFFFFU &&
                state.stored_group_a_index == 0xFFFFU &&
                state.current_actor_index == 0xFFFFU &&
                state.result_mode == 1U && state.battle_submode == 2U &&
                port.count(0x0047CC40U) == 1U && port.count(0x0047F900U) == 1U,
            "action ninety nine performs exact terminal reset and two mode calls"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.action_runtime_flags = 0x8001U;
        state.packed_actor_counter = 0xAABBCCDDU;
        Fixture fixture;
        DispatchPort port;
        port.action = 3U;
        port.actor_metric_state().group_b_count = 0x12U;
        port.retreat_commit_replies = {
            {.eax = 1U},
            {.eax = 1U, .ecx = 0x12345678U, .edx = 0x87654321U},
        };
        port.battle_debug_hotkey_state().committed_actor_code = 9U;
        port.battle_debug_overlay_gate() = 9U;
        port.battle_message_state() = 9U;
        auto& actor_list = fixture.startup.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x76000000U;
        actor_list.selected_resource_token = 0x76000010U;
        actor_list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x34U,
             .secondary_quantity = 2,
             .name = {}},
        };
        auto context = fixture.context();
        context.scripted_resource_release_test_compat = false;

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.retreat_commit_calls == 1U &&
                result.actor_resource_release_calls == 1U &&
                result.actor_resource_release.output_word == 0x34U &&
                actor_list.selected_resource_token == 0U &&
                result.retreat_commit.branch ==
                    openswd3::battle::LegacyBattleRetreatCommitBranch::
                        committed &&
                port.retreat_commit_calls.size() == 2U &&
                port.count(0x0045EA80U) == 0U &&
                state.packed_actor_counter == 0xAABBCC12U &&
                port.retreat_commit_state().completion_gate_a == 1U &&
                port.retreat_commit_state().completion_gate_b == 1U &&
                port.battle_debug_hotkey_state().committed_actor_code == 0U &&
                port.battle_debug_overlay_gate() == 0U &&
                port.battle_message_state() == 0U &&
                port.outcome_resolution_state().darkening_gate == 1U,
            "action three directly finalizes the selected actor and publishes the shared battle state without the old opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].completion_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x0047F150U, {.eax = 1U});
        port.push(0x00482E90U, {.eax = 1U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U && state.action_pending == 1U &&
                state.selected_target_index == 1U &&
                state.selected_group_b_identity[1] == 1U &&
                state.frame_refresh_pending == 1U &&
                result.framebuffer_clear_calls == 1U &&
                fixture.framebuffer.physical_pixels()[0] == 0xFFFFU &&
                port.battle_pair_primary_value() == 0U &&
                result.pair_transition_calls == 1U &&
                result.pair_transition.port_calls == 1U,
            "ordinary attack publishes target clears framebuffer pairs actors and returns zero"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.side_mode = 1U;
        state.group_a_to_actor[0] = 0U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].completion_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x0047F150U, {.eax = 1U});
        port.push(0x00482E90U, {.eax = 1U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.pair_transition_calls == 1U &&
                result.pair_transition.port_calls == 1U &&
                port.battle_pair_primary_value() == 0U,
            "alternate-side ordinary attack directly composes the first pair transition call site"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        state.blocking_effect = 1U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].completion_gate = 1U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x00482E90U, {.eax = 8U});
        port.push(0x00482F10U, {.eax = 50U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                has_call_argument(port, 0x0047F150U, 0U, 0U - 550U) &&
                port.count(0x0047CF00U) == 1U && port.count(0x0047CEC0U) == 1U,
            "class eight target attack preserves percentage remainder formula and unsigned negation presentation"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 2U;
        auto context = fixture.context();
        const auto initialized =
            openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
        state.action_runtime_flags |= 1U;
        const auto completed = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            initialized.status == LegacyBattleActionDispatchStatus::completed &&
                completed.status ==
                    LegacyBattleActionDispatchStatus::completed &&
                port.count(0x00489E90U) == 1U &&
                port.count(0x00489D00U) == 1U && !state.deformation &&
                !state.deformation_active &&
                state.deformation_owner_token == 0U &&
                state.frame_effect.fade_active == 1U,
            "action two constructs deformation after allocation and releases owner only on completed low bit"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 10U;
        port.action = 409U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.return_value == 1U && state.current_actor_index == 0xFFFFU &&
                openswd3::compat::u16(state.scan_push_state) == 0x8000U &&
                state.frame_refresh_pending == 1U &&
                port.count(0x0047CC40U) == 1U &&
                has_call_argument(port, 0x0047F150U, 0U, 10U),
            "special action four hundred nine preserves unique screen mode and scan push publication"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 2;
        state.group_b_count = 2;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 0x194U;
        port.push(0x0047D930U, {.eax = 0U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                state.special_phase == 0U &&
                state.frame_effect.split_suppression == 0U &&
                result.group_a_iterations == 4U &&
                result.group_b_iterations == 4U &&
                port.count(0x0047D810U) == 6U && port.count(0x0047D830U) == 6U,
            "special phase four hundred four runs pause action and restore loops in one fallthrough call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.phase_counter = 3U;
        state.phase_condition = 1U;
        state.group_b_count = 1;
        state.packed_actor_counter = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U &&
                openswd3::compat::u16(state.phase_counter) == 0U &&
                state.current_actor_index == 0xFFFFU &&
                state.selected_target_index == 0xFFFFU &&
                state.frame_effect.fade_active == 1U &&
                port.count(0x00485610U) == 1U &&
                result.text_message_calls == 1U &&
                fixture.startup_reset.block_5214f8[0U] == 0x71000000U &&
                fixture.text_messages.allocations[0U].record.text_token ==
                    0x004A77F0U,
            "phase six countdown reaches two emits selected message and clears visual phase state"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.phase_counter = 1U;
        state.group_b_status_words[0] = 7U;
        state.group_a_target_phases[0U].emitter.initialized = 1;
        state.group_a_target_phases[0U].emitter.remaining_batches = 0U;
        state.group_a_target_phases[0U].emitter.spawned_count = 0;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        port.push(0x00487C10U, {.eax = 0x00630000U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        const auto& item =
            port.world_item_list_state().player_inventory.front();
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.player_item_calls == 1U &&
                result.player_item.return_token == 0x0063000CU &&
                item.item_id == 7U && item.quantity_b == 1U,
            "phase six completion directly publishes the target status into the shared player inventory"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        Fixture fixture;
        DispatchPort port;
        port.action = 23U;
        port.push(0x00472430U, {.eax = 0x22U});
        port.push(0x00487C10U, {.eax = 0x00640000U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        const auto& item =
            port.world_item_list_state().player_inventory.front();
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.player_item_calls == 1U &&
                result.player_item.return_token == 0x0064000CU &&
                item.item_id == 0x22U && item.quantity_b == 1U,
            "action twenty-three message path directly publishes selector-one player quantity"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 13U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 2U, 3U
        );
        test.expect_true(
            result.return_value == 1U &&
                static_cast<u16>(fixture.startup_reset.block_52022c[10U]) ==
                    4U &&
                state.frame_effect.fade_active == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.temporary_record[0x19U] == 0x20U,
            "action thirteen initializes effect then appends target plus one to the shared first-four-row event table"
        );

        LegacyBattleActionDispatchState tail_state;
        Fixture tail_fixture;
        DispatchPort tail_port;
        tail_port.action = 13U;
        auto tail_context = tail_fixture.context();
        const auto tail_result =
            openswd3::battle::dispatch_legacy_battle_action(
                tail_state, tail_port, tail_context, 4U, 2U
            );
        test.expect_true(
            tail_result.return_value == 1U &&
                tail_state.group_a_event_slots_tail[0U] == 3U &&
                tail_fixture.startup_reset.block_52022c[0U] == 0U,
            "action thirteen keeps rows five through ten in the adjacent event-table tail without duplicating the shared prefix"
        );

        LegacyBattleActionDispatchState missing_state;
        Fixture missing_fixture;
        DispatchPort missing_port;
        missing_port.action = 13U;
        auto missing_context = missing_fixture.context();
        missing_context.startup_reset = nullptr;
        const auto missing_result =
            openswd3::battle::dispatch_legacy_battle_action(
                missing_state, missing_port, missing_context, 2U, 3U
            );
        test.expect_true(
            missing_result.status ==
                    LegacyBattleActionDispatchStatus::event_slot_typed_stop &&
                missing_state.frame_effect.fade_active == 1U &&
                missing_state.temporary_record[0x19U] == 0x20U,
            "action thirteen stops at the first shared-prefix slot access after preserving its visual and record prefix"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 1;
        state.status_indicator.tick_counter = 24U;
        state.status_indicator.intensity = 32U;
        state.status_indicator.intensity_countdown = 32U;
        Fixture fixture;
        DispatchPort port;
        port.action = 22U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.status_indicator_calls == 1U &&
                result.status_indicator.return_value == 1U &&
                (state.action_runtime_flags & 0x8000U) != 0U &&
                state.scene_value == 1U && state.available_actor_count == 1,
            "action twenty two directly completes status indicator then counts and selects first live opponent"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.action_runtime_flags = 0x8001U;
        Fixture fixture;
        DispatchPort port;
        port.action = 22U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.return_value == 1U && state.frame_effect.fade_active == 1U &&
                result.status_indicator_calls == 0U,
            "action twenty two completed runtime branch returns one without replaying indicator"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        state.action_runtime_flags = 0x8000U;
        state.group_a_action_execution[0U].completion_gate = 1U;
        Fixture fixture;
        fixture.raster.surface.width = 641;
        DispatchPort port;
        port.battle_pair_primary_value() = 9U;
        port.action = 1U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::framebuffer_typed_stop &&
                result.framebuffer_clear_calls == 1U &&
                state.frame_refresh_pending == 1U &&
                fixture.framebuffer.physical_pixels().front() == 0xFFFFU &&
                fixture.framebuffer.physical_pixels().back() == 0xFFFFU,
            "oversized clear fills owned framebuffer prefix then stops at first out of range pixel"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_b_count = 2;
        state.blocking_effect = 1U;
        Fixture fixture;
        DispatchPort port;
        port.action = 24U;
        port.push(0x004724D0U, {.eax = 0x8001U});
        port.push(0x00481010U, {.eax = 100U});
        port.push(0x00481010U, {.eax = 200U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.group_b_iterations == 2U && state.message_aux == 1U &&
                openswd3::compat::u16(state.packed_action_state) == 0U &&
                port.count(0x00481010U) == 2U && port.count(0x00478780U) == 2U,
            "action twenty four scans every live target accumulates signed values and clears nonterminal low word"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 31U;
        fixture.flags[0x4BU >> 3U] = static_cast<u8>(1U << (0x4BU & 7U));
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 99U
        );
        test.expect_true(
            result.return_value == 1U &&
                result.action_record_clear_calls == 1U &&
                state.countdown.secondary_ticks == 150U &&
                state.message_gate == 0U &&
                state.current_actor_index == 0xFFFFU &&
                (fixture.flags[0x4BU >> 3U] &
                 static_cast<u8>(1U << (0x4BU & 7U))) == 0U,
            "action thirty one initializes five second countdown then escape clears bit before target access"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        Fixture fixture;
        DispatchPort port;
        port.action = 33U;
        port.push(0x00480AD0U, {.eax = 0U});
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 1U
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        target_object_typed_stop &&
                state.current_actor_index == 0xFFFFU &&
                port.count(0x00482F10U) == 0U,
            "null resolved target stops at first flags dereference after current actor clear"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        fixture.attack_order_records[0].value_00 = 0U;
        DispatchPort port;
        port.action = 7U;
        port.push(0x00479850U, {.eax = 1U});
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.attack_order_remove_calls == 1U &&
                result.attack_order_remove.matched &&
                fixture.attack_order_records[0].value_00 == 0xFFFFFFFFU &&
                (state.packed_actor_counter & 0xFFU) == 1U &&
                port.count(0x0045EFB0U) == 0U && result.return_value == 1U,
            "action seven removes the opponent directly from the shared attack order before publishing completion"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        fixture.attack_order_records[17].value_00 = 0U;
        DispatchPort port;
        port.action = 7U;
        port.push(0x00479850U, {.eax = 1U});
        auto context = fixture.context();
        context.attack_order_adjacent_record = nullptr;

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        attack_order_remove_typed_stop &&
                result.attack_order_remove.status ==
                    openswd3::battle::LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop &&
                (state.packed_actor_counter & 0xFFU) == 0U &&
                result.return_value == 0U,
            "attack-order one-past stop preserves the ready and delay prefix then blocks action completion"
        );
    }

    {
        DispatchPort port;
        static_cast<void>(port.invoke_group_a_summon_materialization({
            .call =
                openswd3::battle::LegacyBattleGroupASummonMaterializationCall::
                    report_missing_role,
            .window_token = 0x12340000U,
            .diagnostic_text_token = 0x004A7C68U,
            .diagnostic_source_token = 0x004A7C44U,
            .diagnostic_source_line = 0x123U,
        }));
        test.expect_true(
            port.count(0x00431150U) == 1U &&
                has_call_argument(port, 0x00431150U, 0U, 0x12340000U) &&
                has_call_argument(port, 0x00431150U, 1U, 0x004A7C68U) &&
                has_call_argument(port, 0x00431150U, 2U, 0U) &&
                has_call_argument(port, 0x00431150U, 3U, 0x004A7C44U) &&
                has_call_argument(port, 0x00431150U, 4U, 0x123U),
            "summon diagnostic adapter preserves the fixed window, text, flags, source, and line arguments"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.phase_counter = 0xAABB0000U;
        state.group_a_status_words[0U] = 2U;
        Fixture fixture;
        fixture.startup.window_token = 0x12340000U;
        auto& summon = fixture.startup.party[2U];
        summon.placement_prefix = {1U, 2U, 3U, 4U, 5U};
        summon.role_id = 7U;
        summon.position_x = 0x2345U;
        summon.position_y = 0x3456U;
        summon.placement_field_1a = 0x4567U;
        summon.active = 1U;
        summon.configuration.actor_record_token = 0x005029D0U + 2U * 0x2F34U;
        DispatchPort port;
        port.action = 15U;
        port.push(0x00487C10U, {.eax = 0x71000000U});
        port.push(0x00471D60U, {.eax = 0U});
        set_summon_profile_word(port.summon_profile, 0x56U, 0x1111U);
        set_summon_profile_word(port.summon_profile, 0x60U, 0x2222U);
        set_summon_profile_word(port.summon_profile, 0x64U, 0x3333U);
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.return_value == 0U &&
                result.summon_materialization_calls == 1U &&
                result.summon_materialization.status ==
                    openswd3::battle::
                        LegacyBattleGroupASummonMaterializationStatus::
                            completed &&
                result.summon_materialization.return_eax == 0x71000000U &&
                result.summon_materialization.return_edx ==
                    summon.configuration.actor_record_token &&
                port.summon_materialization_calls.size() == 3U &&
                port.count(0x0046E890U) == 0U &&
                port.count(0x00487C10U) == 1U &&
                port.count(0x00476DB0U) == 1U &&
                port.count(0x00478220U) == 1U &&
                has_call_argument(port, 0x00476DB0U, 0U, 0x71000000U) &&
                has_call_argument(port, 0x00476DB0U, 1U, 7U) &&
                summon.configuration.profile_token == 0x71000000U &&
                summon.configuration.placement_primary[5U] == 0x23450007U &&
                summon.configuration.source_record_token ==
                    summon.configuration.actor_record_token &&
                (summon.configuration.actor_record[9U] >> 16U) == 0x1111U &&
                summon.configuration.profile_field_f2 == 0x2222U &&
                static_cast<u16>(summon.configuration.actor_record[1U]) ==
                    0x3333U &&
                state.phase_counter == 0xAABB0001U && state.summon_x == 0U &&
                state.summon_y == 0U,
            "action fifteen materializes the selected summon from the shared startup record before beginning its frame phase"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_status_words[0U] = 1U;
        Fixture fixture;
        fixture.startup.window_token = 0x76543210U;
        fixture.startup.party[1U].role_id = 0U;
        fixture.startup.party[1U].configuration.actor_record_token =
            0x005029D0U + 0x2F34U;
        DispatchPort port;
        port.action = 15U;
        port.push(0x00487C10U, {.eax = 0x72000000U});
        port.push(0x00471D60U, {.eax = 0U});
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.summon_materialization.diagnostic_calls == 1U &&
                port.count(0x00431150U) == 1U &&
                has_call_argument(port, 0x00431150U, 0U, 0x76543210U) &&
                has_call_argument(port, 0x00431150U, 1U, 0x004A7C68U) &&
                has_call_argument(port, 0x00431150U, 2U, 0U) &&
                has_call_argument(port, 0x00431150U, 3U, 0x004A7C44U) &&
                has_call_argument(port, 0x00431150U, 4U, 0x123U),
            "zero summon role forwards the fixed diagnostic payload after profile release"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_status_words[0U] = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 15U;
        port.push(0x00487C10U, {.eax = 0x73000000U});
        auto context = fixture.context();
        context.startup = nullptr;

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::
                        summon_materialization_typed_stop &&
                result.summon_materialization.status ==
                    openswd3::battle::
                        LegacyBattleGroupASummonMaterializationStatus::
                            actor_state_typed_stop &&
                result.summon_materialization.allocation_calls == 1U &&
                result.summon_materialization.load_calls == 0U &&
                port.count(0x00487C10U) == 1U &&
                port.count(0x0046E890U) == 0U &&
                static_cast<u16>(state.phase_counter) == 0U,
            "missing shared summon owner stops after allocation and clear before the first actor write"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        actor.position_x = 240U;
        actor.position_y = 220U;
        actor.target_phase_y_adjustment = 20;
        actor.source_x_offset = 40U;
        actor.source_y_offset = 10U;
        phase.mode_flags = 0x80U;
        phase.runtime_gate = 1U;
        phase.block_0df4.fill(1U);
        phase.block_0500.fill(2U);
        phase.block_2bc8.fill(3U);
        Fixture fixture;
        DispatchPort port;
        const auto result = openswd3::battle::start_legacy_battle_target_phase(
            &phase,
            &actor,
            &fixture.startup.render_geometry,
            port,
            {
                .target_token = 0x00525508U,
                .surface_width = 640,
                .surface_height = 480,
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0xBBBBBBBBU,
                .entry_edx = 0xCCCCCCCCU,
            }
        );
        const auto& emitter = phase.emitter;
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        completed &&
                result.port_calls == 4U && result.resource_query_calls == 1U &&
                result.coordinate_query_calls == 1U &&
                result.decode_calls == 1U &&
                result.property_query_calls == 1U &&
                result.presentation_dwords_zeroed == 0x16U &&
                result.tail_dwords_zeroed == 0xECU &&
                result.host_surface_calls == 1U && result.return_eax == 0U &&
                result.return_ecx == 0U &&
                phase.resource_token == 0x72000000U &&
                phase.decoded_resource_token == 0x74000000U &&
                emitter.source_pixels.size() == 0x20U * 0x50U &&
                emitter.source_width == 0x20U &&
                emitter.source_height == 0x50U &&
                emitter.source_origin_x == 0x2F &&
                emitter.source_origin_y == 0x40 &&
                emitter.target_origin_x == 220 &&
                emitter.target_origin_y == 240 && emitter.target_width == 1 &&
                emitter.target_height == 1 &&
                emitter.distance_offset_base == 0x14U &&
                emitter.lifetime_divisor == 0x1EU &&
                emitter.remaining_batches == 0x46U &&
                emitter.spawn_divisor == 0x28U && emitter.flags == 0x57U &&
                emitter.published_value_2c == 5 &&
                emitter.published_value_30 == 5 &&
                emitter.published_value_34 == 5 && phase.mode_flags == 0x88U &&
                phase.runtime_gate == 0U &&
                std::ranges::all_of(
                    phase.block_0df4,
                    [](const u32 value) { return value == 0U; }
                ) &&
                fixture.startup.render_geometry.surface_width == 640 &&
                fixture.startup.render_geometry.surface_height == 480,
            "target phase start publishes the decoded emitter, geometry and exact clear suffix"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
        phase.emitter.flags = 0xFFFFU;
        phase.block_0df4.fill(9U);
        Fixture fixture;
        DispatchPort port;
        port.push(0x00478620U, {.eax = 0U});
        const auto result = openswd3::battle::start_legacy_battle_target_phase(
            &phase,
            &actor,
            &fixture.startup.render_geometry,
            port,
            {.target_token = 0x00525508U,
             .surface_width = 640,
             .surface_height = 480}
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseStartStatus::
                        resource_object_typed_stop &&
                result.resource_query_calls == 1U &&
                result.coordinate_query_calls == 1U &&
                result.decode_calls == 0U &&
                result.presentation_dwords_zeroed == 0x16U &&
                phase.emitter.flags == 0U && phase.block_0df4[0U] == 9U,
            "target phase resource stop preserves both callee calls and the emitter clear prefix"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        phase.tick = 39U;
        phase.emitter.initialized = 1;
        phase.emitter.remaining_batches = 1U;
        phase.emitter.spawn_divisor = 1U;
        phase.emitter.source_width = 40U;
        phase.emitter.source_height = 1U;
        phase.emitter.source_origin_x = 100;
        phase.emitter.source_origin_y = 200;
        phase.emitter.spawned_count = 1;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase(
                &phase,
                &shared.target_phase_particle_nodes,
                &shared.target_phase_particle_rng,
                &shared.target_phase_particle_shared,
                &shared.target_phase_particle_diagnostics,
                {},
                &fixture.effects.pixel_conversion,
                port,
                {.target_token = 0x005029D0U, .time_seed = 0x12345678U}
            );
        const auto spawns = std::ranges::count_if(
            port.calls, [](const LegacyBattleActionCallRequest& call) {
                return call.callee_token == 0x00471FC0U;
            }
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseAdvanceStatus::
                        completed &&
                result.particle_frame_calls == 1U &&
                result.particle_frame.legacy_return_value == 0 &&
                result.spawn_calls == 5U && spawns == 5 && phase.tick == 40U &&
                phase.active_gate == 1U && result.return_eax == 0U &&
                has_call_argument(port, 0x00471FC0U, 1U, 1U) &&
                has_call_argument(port, 0x00471FC0U, 2U, 0U) &&
                has_call_argument(port, 0x00471FC0U, 3U, 110U) &&
                has_call_argument(port, 0x00471FC0U, 4U, 195U) &&
                has_call_argument(port, 0x00471FC0U, 5U, 0x0EU) &&
                has_call_argument(port, 0x00471FC0U, 2U, 4U) &&
                has_call_argument(port, 0x00471FC0U, 4U, 205U),
            "target phase tick forty repeats all five threshold particle calls with the original coordinates"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        phase.tick = 0x7FFFU;
        phase.emitter.initialized = 1;
        phase.emitter.remaining_batches = 1U;
        phase.emitter.spawn_divisor = 1U;
        phase.emitter.source_height = 1U;
        phase.emitter.spawned_count = 1;
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase(
                &phase,
                &shared.target_phase_particle_nodes,
                &shared.target_phase_particle_rng,
                &shared.target_phase_particle_shared,
                &shared.target_phase_particle_diagnostics,
                {},
                &fixture.effects.pixel_conversion,
                port,
                {.target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseAdvanceStatus::
                        completed &&
                phase.tick == 0x8000U && result.spawn_calls == 1U,
            "target phase signed tick overflow suppresses the four threshold branches while keeping the unconditional spawn"
        );
    }

    {
        openswd3::battle::LegacyBattleTargetPhaseState phase;
        phase.tick = 9U;
        phase.active_gate = 1U;
        phase.decoded_resource_token = 0x74000000U;
        phase.emitter.initialized = 1;
        phase.emitter.remaining_batches = 0U;
        phase.emitter.spawned_count = 0;
        phase.spawn_counters.fill(7U);
        phase.block_0df4.fill(8U);
        phase.block_0500.fill(9U);
        phase.block_2bc8.fill(10U);
        LegacyBattleActionDispatchState shared;
        Fixture fixture;
        DispatchPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_target_phase(
                &phase,
                &shared.target_phase_particle_nodes,
                &shared.target_phase_particle_rng,
                &shared.target_phase_particle_shared,
                &shared.target_phase_particle_diagnostics,
                {},
                &fixture.effects.pixel_conversion,
                port,
                {.target_token = 0x005029D0U}
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleTargetPhaseAdvanceStatus::
                        completed &&
                result.particle_frame.legacy_return_value == 1 &&
                result.resource_release_calls == 1U &&
                result.presentation_dwords_zeroed == 0x16U &&
                result.spawn_counter_clears == 5U &&
                result.tail_dwords_zeroed == 0x2EU && result.return_eax == 1U &&
                phase.tick == 0U && phase.active_gate == 0U &&
                phase.decoded_resource_token == 0U &&
                std::ranges::all_of(
                    phase.spawn_counters,
                    [](const u32 value) { return value == 0U; }
                ) &&
                phase.block_0df4[0U] == 0U && phase.block_0500[0U] == 0U &&
                phase.block_2bc8[0U] == 10U && port.count(0x004885A0U) == 1U,
            "target phase completion releases the decoded buffer and clears only the original presentation, counters and two tail blocks"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_action_execution[0U].position_x = 240U;
        state.group_a_action_execution[0U].position_y = 220U;
        state.group_a_action_execution[0U].source_x_offset = 40U;
        state.group_a_action_execution[0U].source_y_offset = 10U;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        auto context = fixture.context();
        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );
        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.target_phase_start_calls == 1U &&
                result.target_phase_start.port_calls == 4U &&
                port.count(0x004710D0U) == 0U &&
                port.count(0x00478620U) == 1U &&
                port.count(0x00478470U) == 1U &&
                port.count(0x004019A0U) == 1U &&
                port.count(0x0047CE70U) == 1U && state.phase_condition == 1U &&
                static_cast<u16>(state.phase_counter) == 1U &&
                result.target_phase_advance_calls == 1U &&
                port.count(0x00471270U) == 0U &&
                state.group_a_target_phases[0U].decoded_resource_token ==
                    0x74000000U,
            "action six production starts the typed target phase without the whole-function opaque call"
        );
    }

    {
        bool actions_complete = true;
        constexpr std::array<u16, 27> actions{
            1U,  2U,  3U,  4U,  5U,  6U,  7U,  11U, 12U,
            13U, 14U, 15U, 17U, 22U, 23U, 24U, 25U, 26U,
            27U, 28U, 29U, 31U, 32U, 33U, 34U, 35U, 36U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.group_a_count = 1;
            state.group_b_count = 1;
            state.group_a_to_actor[0] = 0U;
            state.battle_flags = 0x20U;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            actions_complete = actions_complete &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_code == action;
        }
        test.expect_true(
            actions_complete,
            "all twenty seven populated ordinary switch entries execute without default fallthrough"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        state.battle_flags = 0x20U;
        state.stored_group_b_index = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 25U;
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.attack_order_calls == 1U &&
                result.attack_order.written_index == 0U &&
                fixture.attack_order_records[0].value_00 == 0U &&
                fixture.attack_order_records[0].value_08 == 2U &&
                port.count(0x0045EDF0U) == 0U &&
                state.current_actor_index == 0xFFFFU &&
                result.return_value == 1U,
            "action twenty five directly appends the selected opponent to the shared attack order"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.group_a_to_actor[0] = 0U;
        state.battle_flags = 0x20U;
        state.stored_group_b_index = 0U;
        Fixture fixture;
        DispatchPort port;
        port.action = 25U;
        auto context = fixture.context();
        context.attack_order_records = {};

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status ==
                    LegacyBattleActionDispatchStatus::attack_order_typed_stop &&
                state.group_b_status_words[0] != 0U &&
                result.return_value == 0U,
            "attack-order typed stop preserves the choice status write then blocks action completion"
        );
    }

    {
        bool special_actions_complete = true;
        constexpr std::array<u16, 10> actions{
            100U,
            200U,
            300U,
            400U,
            402U,
            404U,
            405U,
            406U,
            409U,
            500U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.group_a_count = 1;
            state.group_b_count = 1;
            state.group_a_to_actor[0] = 0U;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            special_actions_complete = special_actions_complete &&
                result.status == LegacyBattleActionDispatchStatus::completed &&
                result.action_code == action;
        }
        test.expect_true(
            special_actions_complete,
            "all ten populated special action codes execute their predispatch branches"
        );
    }

    {
        bool defaults_match = true;
        constexpr std::array<u16, 10> actions{
            8U,
            9U,
            10U,
            16U,
            18U,
            19U,
            20U,
            21U,
            30U,
            37U,
        };
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 99U
            );
            defaults_match = defaults_match && result.return_value == 0U &&
                result.port_calls == 2U;
        }
        test.expect_true(
            defaults_match,
            "nine sparse switch holes and out of range action return zero before target access"
        );
    }

    {
        bool cases_match = true;
        constexpr std::array<u16, 3> actions{34U, 35U, 36U};
        for (const u16 action : actions) {
            LegacyBattleActionDispatchState state;
            state.selection_word = 7U;
            state.selection_high_word = 9U;
            Fixture fixture;
            DispatchPort port;
            port.action = action;
            port.push(0x00481010U, {.eax = 5U});
            auto context = fixture.context();
            const auto result = openswd3::battle::dispatch_legacy_battle_action(
                state, port, context, 0U, 0U
            );
            cases_match = cases_match && result.return_value == 1U &&
                state.current_actor_index == 0xFFFFU &&
                port.count(0x0047D640U) == 1U && port.count(0x0047F150U) == 1U;
        }
        test.expect_true(
            cases_match,
            "actions thirty four through thirty six preserve separate signed component presentation tails"
        );
    }
}
