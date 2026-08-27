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
        if (request.callee_token == 0x004783B0U) {
            auto reply = default_reply;
            reply.publish_metric_word = true;
            reply.metric_word = 1U;
            return reply;
        }
        return default_reply;
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
    std::array<openswd3::battle::LegacyBattleStartupResetRecord, 0x12>
        attack_order_records{};
    std::array<u32, 0x32> attack_order_party_sources{};
    u32 attack_order_primary_gate{};
    u32 attack_order_secondary_gate{};
    openswd3::battle::LegacyBattleIntensityEffectRecord
        attack_order_adjacent_record{};

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
            .attack_order_party_sources = attack_order_party_sources,
            .attack_order_primary_gate = &attack_order_primary_gate,
            .attack_order_secondary_gate = &attack_order_secondary_gate,
            .attack_order_adjacent_record = &attack_order_adjacent_record,
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
        port.battle_debug_hotkey_state().reset_gate_53bd50 = 9U;
        port.battle_debug_overlay_gate() = 9U;
        port.battle_message_state() = 9U;
        auto context = fixture.context();

        const auto result = openswd3::battle::dispatch_legacy_battle_action(
            state, port, context, 0U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleActionDispatchStatus::completed &&
                result.retreat_commit_calls == 1U &&
                result.retreat_commit.branch ==
                    openswd3::battle::LegacyBattleRetreatCommitBranch::
                        committed &&
                port.retreat_commit_calls.size() == 2U &&
                port.count(0x0045EA80U) == 0U &&
                state.packed_actor_counter == 0xAABBCC12U &&
                port.retreat_commit_state().completion_gate_a == 1U &&
                port.retreat_commit_state().completion_gate_b == 1U &&
                port.battle_debug_hotkey_state().reset_gate_53bd50 == 0U &&
                port.battle_debug_overlay_gate() == 0U &&
                port.battle_message_state() == 0U &&
                port.outcome_resolution_state().darkening_gate == 1U,
            "action three directly finalizes the selected actor and publishes the shared battle state without the old opaque call"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_to_actor[0] = 0U;
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x0046F8C0U, {.eax = 1U});
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
        Fixture fixture;
        DispatchPort port;
        port.battle_pair_primary_value() = 1000U;
        port.action = 1U;
        port.push(0x0046F8C0U, {.eax = 1U});
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
                port.count(0x00485610U) == 1U && port.count(0x004698E0U) == 1U,
            "phase six countdown reaches two emits selected message and clears visual phase state"
        );
    }

    {
        LegacyBattleActionDispatchState state;
        state.group_a_count = 1;
        state.group_b_count = 1;
        state.phase_counter = 1U;
        state.group_b_status_words[0] = 7U;
        Fixture fixture;
        DispatchPort port;
        port.action = 6U;
        port.push(0x00471270U, {.eax = 1U});
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
            result.return_value == 1U && state.group_a_event_slots[20U] == 4U &&
                state.frame_effect.fade_active == 1U &&
                state.frame_effect.primary_suppression == 0U &&
                state.temporary_record[0x19U] == 0x20U,
            "action thirteen initializes effect then appends target plus one to first empty actor event slot"
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
