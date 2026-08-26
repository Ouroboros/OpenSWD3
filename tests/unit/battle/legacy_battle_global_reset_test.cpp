#include "openswd3/battle/legacy_battle_global_reset.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::LegacySampleBackend;
using openswd3::audio_video::LegacySampleHandle;
using openswd3::audio_video::LegacySampleManager;
using openswd3::audio_video::LegacySndArchive;
using openswd3::battle::LegacyBattleGlobalResetCall;
using openswd3::battle::LegacyBattleGlobalResetCallReply;
using openswd3::battle::LegacyBattleGlobalResetCallStage;
using openswd3::battle::LegacyBattleGlobalResetRuntimePort;
using openswd3::battle::LegacyBattleGlobalResetState;
using openswd3::battle::LegacyBattleActionDispatchState;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::battle::LegacyBattleGroupBFrameState;
using openswd3::battle::LegacyBattleDebugOverlayState;
using openswd3::battle::LegacyBattleStartupCall;
using openswd3::battle::LegacyBattleStartupCallReply;
using openswd3::battle::LegacyBattleStartupCallRequest;
using openswd3::battle::LegacyBattleStartupState;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class SilentSampleBackend final : public LegacySampleBackend {
public:
    [[nodiscard]] u32 driver_token() const override {
        return 1U;
    }
    [[nodiscard]] LegacySampleHandle allocate_sample_handle() override {
        return 1U;
    }
    void initialize_sample(LegacySampleHandle) override {}
    void release_sample_handle(LegacySampleHandle) override {}
    [[nodiscard]] bool
    set_sample_file(LegacySampleHandle, std::span<const u8>) override {
        return true;
    }
    [[nodiscard]] bool set_named_sample_file(
        LegacySampleHandle, std::string_view, std::span<const u8>, u32
    ) override {
        return true;
    }
    void set_sample_user_data(LegacySampleHandle, u32, u32) override {}
    [[nodiscard]] u32 sample_user_data(LegacySampleHandle, u32) override {
        return 0U;
    }
    void set_sample_volume(LegacySampleHandle, i32) override {}
    void set_sample_pan(LegacySampleHandle, i32) override {}
    void set_sample_loop_count(LegacySampleHandle, i32) override {}
    void start_sample(LegacySampleHandle) override {}
    void end_sample(LegacySampleHandle) override {
        ++end_calls;
    }
    [[nodiscard]] u32 sample_status(LegacySampleHandle) override {
        return 0U;
    }
    void close_output() override {}

    u32 end_calls{};
};

class ResetPort final : public LegacyBattleGlobalResetRuntimePort {
public:
    ResetPort() : samples(sample_backend, archive) {}

    [[nodiscard]] LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest& request) override {
        startup_calls.push_back(request);
        return {.return_value = startup_return};
    }

    void release_image(const u32 token) noexcept override {
        released_images.push_back(token);
    }
    void release_owner(const u32 token) noexcept override {
        released_owners.push_back(token);
    }
    void release(const u32 token) noexcept override {
        released_render_tokens.push_back(token);
    }

    [[nodiscard]] LegacyBattleGlobalResetCallReply invoke_reset(
        const LegacyBattleGlobalResetCall call, const u32 argument
    ) override {
        reset_calls.push_back(call);
        reset_arguments.push_back(argument);
        return {.eax = reset_return};
    }

    [[nodiscard]] LegacySampleManager& sample_manager() noexcept override {
        return samples;
    }

    [[nodiscard]] u32
    startup_call_count(const LegacyBattleStartupCall call) const {
        return static_cast<u32>(
            std::ranges::count_if(startup_calls, [call](const auto& request) {
                return request.call == call;
            })
        );
    }

    u32 startup_return{0xAABBCCDDU};
    u32 reset_return{0x11223344U};
    std::vector<LegacyBattleStartupCallRequest> startup_calls;
    std::vector<LegacyBattleGlobalResetCall> reset_calls;
    std::vector<u32> reset_arguments;
    std::vector<u32> released_images;
    std::vector<u32> released_owners;
    std::vector<u32> released_render_tokens;
    SilentSampleBackend sample_backend;
    LegacySndArchive archive;
    LegacySampleManager samples;
};

[[nodiscard]] std::uint64_t
write_trace_hash(const LegacyBattleGlobalResetState& state) {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    const auto append = [&hash](const u32 value) {
        for (u32 byte = 0U; byte < 4U; ++byte) {
            hash ^= static_cast<u8>(value >> (byte * 8U));
            hash *= 0x100000001B3ULL;
        }
    };
    for (const auto& write : state.write_trace) {
        append(write.address);
        append(write.size);
        append(write.count);
        append(write.value);
    }
    return hash;
}

[[nodiscard]] u8
byte_at(const LegacyBattleGlobalResetState& state, const u32 address) {
    const auto found = state.unmapped_bytes.find(address);
    return found == state.unmapped_bytes.end() ? 0xEEU : found->second;
}

void seed_state(
    LegacyBattleGlobalResetState& state,
    LegacyBattleStartupState& startup,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleGroupBFrameState& actor_frames,
    LegacyBattleDebugOverlayState& debug_overlay,
    ResetPort& port
) {
    state.unmapped_bytes[0x00ABCDEFU] = 0x5AU;
    state.unmapped_bytes[0x00520E40U] = 0x5AU;
    state.unmapped_bytes[0x005202A8U] = 0x5AU;
    state.unmapped_bytes[0x0053AE7AU] = 0x5AU;
    state.unmapped_bytes[0x0053BD40U] = 0x5AU;
    state.unmapped_bytes[0x0053BD5CU] = 0x5AU;
    state.unmapped_bytes[0x0053C000U] = 0x5AU;
    state.unmapped_bytes[0x0053C4A0U] = 0x5AU;
    startup.display_surfaces = {11U, 22U};
    startup.background_rotation_cache.frame_owner_tokens[0] = 101U;
    startup.background_rotation_cache.cached_image_tokens[0] = 202U;
    startup.render_geometry.primary_row_offsets = std::make_unique<u32[]>(2U);
    startup.render_geometry.surface_row_offsets = std::make_unique<u32[]>(2U);
    startup.render_geometry.auxiliary_buffer_token = 303U;
    startup.reset.values_502940.fill(9U);
    startup.reset.values_502940[0] = 404U;
    startup.reset.block_525470.fill(9U);
    startup.reset.block_5244e8.fill(9U);
    startup.reset.records_524788[0] = {
        .value_00 = 9U,
        .value_04 = 9U,
        .value_08 = 9U,
        .value_0a = 9U,
        .value_0c = 9U,
        .value_10 = 9U,
        .value_14 = 9U,
        .value_18 = 9U,
    };
    startup.enemies[0].role_id = 9U;
    startup.party[0].role_id = 9U;
    startup.enemy_count = 8U;
    startup.party_count = 10U;

    final_actor.active_actor_code = 9U;
    final_actor.secondary_actor_code = 9U;
    final_actor.published_actor_code = 9U;
    final_actor.source_actor_code = 9U;
    final_actor.action_execution_active = 9U;
    final_actor.auxiliary_gate = 9U;
    port.battle_terminal_latch() = 9U;
    final_actor.pre_frame_gate_a = 9U;
    final_actor.pre_frame_gate_b = 9U;
    final_actor.frame_gate_a = 9U;
    final_actor.frame_gate_b = 9U;
    final_actor.selection_gate = 9U;
    final_actor.queued_actor_code = 9U;
    final_actor.actor_runtime_records[0][0] = 9U;
    port.battle_message_state() = 9U;
    action.opponent_workspace.fill(9U);
    actor_frames.shared.selection_aux_gate = 9U;
    actor_frames.shared.target_ready_gate = 9U;
    actor_frames.shared.action_block_gate = 9U;
    actor_frames.shared.action.action_pending_aux = 9U;
    actor_frames.shared.action_pending_secondary = 9U;
    debug_overlay.gate = 9U;
    debug_overlay.resolved_actor_token = 0x11223344U;
    debug_overlay.selection_order.fill(9U);
    debug_overlay.battle_selector = 9;
    debug_overlay.battle_mode = 0x22334455U;
    debug_overlay.message_status = 0xAABBCCDDU;
    debug_overlay.selection_status = 0x33445566U;
    debug_overlay.lock_count = 0x44556677U;
    debug_overlay.tsw_cache_bytes = 0x55667788U;
    debug_overlay.initial_mode = 9;
    debug_overlay.world_level = -7;
    debug_overlay.battle_frame = 9U;
    debug_overlay.frame_divisor = -11;
    debug_overlay.marker_x = -13;
    debug_overlay.marker_row = -15;
    debug_overlay.text_buffer[0] = 'x';

    auto& color = port.battle_color_accumulation_state();
    color.countdown = 9;
    color.current_red = 9.0F;
    color.current_green = 9.0F;
    color.current_blue = 9.0F;
    color.target_red = 9.0F;
    color.target_green = 9.0F;
    color.target_blue = 9.0F;
    color.step_red = 9.0F;
    color.step_green = 9.0F;
    color.step_blue = 9.0F;
    port.battle_color_initialization_gate() = 9U;

    auto& metrics = port.actor_metric_state();
    metrics.values.fill(9);
    metrics.actor_order.fill(9U);
    metrics.selected_mask.fill(9U);
    metrics.group_b_order.fill(9U);
    metrics.group_b_count = 8U;
    metrics.group_a_count = 10U;
    metrics.priority_update_gate = 9U;
    metrics.group_a_mode = 9U;
    metrics.group_b_mode = 9U;
    metrics.priority_actor_index = 9U;
    metrics.priority_order_ready = 9U;

    port.battle_pair_secondary_value() = 0x7788U;
    auto& shift = port.effect_shift_state();
    shift.packed_reward = 0xAABBCCDDU;
    shift.actor_delta = 9;
    shift.direction_mode = 9U;
    shift.threshold_word = 9U;
    shift.completion_latch = 9U;

    auto& coordinator = port.effect_coordinator_state();
    coordinator.primary[0].complete = 9U;
    coordinator.required_completion_count = 9U;
    coordinator.group_a_global_gate = 9U;
    coordinator.group_a_effect_mode = 9U;
    coordinator.group_b_global_gate = 9U;
    coordinator.group_b_effect_mode = 9U;
    coordinator.group_b_argument = 9U;
    coordinator.completed_count = 9U;
    coordinator.group_a_render_count = 9U;
    coordinator.actor_activity_latch = 9U;
    coordinator.selected_actor_pair = 0xAABB1234U;
    coordinator.group_a_feedback_actor = 9U;
    coordinator.group_b_feedback_actor = 9U;
    port.battle_pair_primary_value() = 9U;
    coordinator.group_a_arguments.fill(9U);
    coordinator.feedback_primary.fill(9U);

    auto& debug = port.battle_debug_hotkey_state();
    debug.toggle_5244e0 = 7U;
    debug.selection_status_word_53c050 = 0xABCD0009U;
    debug.actor_retarget_gate_53bf64 = 9U;
    debug.battle_mode_flags_53bc24 = 9U;
    debug.block_53af30.fill(9U);
    debug.reset_gate_53bd50 = 9U;
    debug.screenshot_request = 7U;
}

}  // namespace

void test_battle_global_reset(openswd3::test::Context& test) {
    {
        LegacyBattleGlobalResetState state;
        LegacyBattleStartupState startup;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleGroupBFrameState actor_frames;
        LegacyBattleDebugOverlayState debug_overlay;
        ResetPort port;
        seed_state(
            state,
            startup,
            final_actor,
            action,
            actor_frames,
            debug_overlay,
            port
        );

        const auto result = openswd3::battle::reset_legacy_battle_globals(
            state,
            startup,
            final_actor,
            action,
            actor_frames,
            debug_overlay,
            port
        );

        const std::array expected_order{
            LegacyBattleGlobalResetCallStage::display_surfaces,
            LegacyBattleGlobalResetCallStage::rotation_cache,
            LegacyBattleGlobalResetCallStage::render_resources,
            LegacyBattleGlobalResetCallStage::conditional_allocation,
            LegacyBattleGlobalResetCallStage::pre_battle_resource_431960,
            LegacyBattleGlobalResetCallStage::pre_battle_resource_433010,
            LegacyBattleGlobalResetCallStage::all_samples,
            LegacyBattleGlobalResetCallStage::audio_stream,
            LegacyBattleGlobalResetCallStage::post_reset_initialization,
        };
        test.expect_true(
            result.call_count == expected_order.size() &&
                result.call_order == expected_order &&
                result.conditional_allocation_token == 404U &&
                result.conditional_allocation_released &&
                result.write_operations == 234U &&
                result.physical_writes == 3300U &&
                result.bytes_written == 13106U && result.return_value == 0U,
            "global reset preserves all nine call stages and the complete fixed write program"
        );
        test.expect_true(
            port.startup_call_count(
                LegacyBattleStartupCall::release_display_surface
            ) == 2U &&
                startup.display_surfaces == std::array<u32, 2>{0U, 0U} &&
                result.display_surfaces.release_calls == 2U &&
                result.display_surfaces.return_value == port.startup_return,
            "display surface helper releases nonzero slots in order and clears each after its call"
        );
        test.expect_true(
            port.released_images == std::vector<u32>{202U} &&
                port.released_owners == std::vector<u32>{101U} &&
                startup.background_rotation_cache.frame_owner_tokens[0] == 0U &&
                startup.background_rotation_cache.cached_image_tokens[0] == 0U,
            "closed rotation cache release runs before global stores"
        );
        test.expect_true(
            port.released_render_tokens == std::vector<u32>{303U} &&
                startup.render_geometry.primary_row_offsets == nullptr &&
                startup.render_geometry.surface_row_offsets == nullptr &&
                startup.render_geometry.auxiliary_buffer_token == 0U,
            "closed render cleanup runs before the fixed render owner zero range"
        );
        test.expect_true(
            port.reset_calls ==
                    std::vector<LegacyBattleGlobalResetCall>{
                        LegacyBattleGlobalResetCall::
                            release_conditional_allocation,
                        LegacyBattleGlobalResetCall::
                            release_pre_battle_resource_431960,
                        LegacyBattleGlobalResetCall::
                            release_pre_battle_resource_433010,
                        LegacyBattleGlobalResetCall::
                            suspend_audio_stream_485710,
                        LegacyBattleGlobalResetCall::
                            initialize_post_reset_4776a0,
                    } &&
                port.reset_arguments.front() == 404U,
            "only pending callees remain behind the narrow reset port in original order"
        );

        const auto& metrics = port.actor_metric_state();
        test.expect_true(
            std::ranges::all_of(
                metrics.values, [](const auto value) { return value == 0; }
            ) &&
                std::ranges::all_of(
                    metrics.actor_order,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    metrics.selected_mask,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    metrics.group_b_order,
                    [](const auto value) { return value == 9U; }
                ) &&
                metrics.group_b_count == 0U && metrics.group_a_count == 0U &&
                metrics.priority_actor_index == 0U,
            "metric order mask counts and priority aliases clear while the untouched group-B order remains"
        );
        const auto& color = port.battle_color_accumulation_state();
        test.expect_true(
            color.countdown == 0 &&
                port.battle_color_initialization_gate() == 9U &&
                color.current_red == 0.0F && color.current_green == 0.0F &&
                color.current_blue == 0.0F && color.target_red == 0.0F &&
                color.target_green == 0.0F && color.target_blue == 0.0F &&
                color.step_red == 0.0F && color.step_green == 0.0F &&
                color.step_blue == 0.0F &&
                state.unmapped_bytes.contains(0x004FDF8CU) == false &&
                state.unmapped_bytes.contains(0x004FDFA4U) == false &&
                state.unmapped_bytes.contains(0x00520D58U) == false &&
                state.unmapped_bytes.contains(0x00520FB8U) == false &&
                state.unmapped_bytes.contains(0x00521388U) == false &&
                state.unmapped_bytes.contains(0x00521394U) == false &&
                state.unmapped_bytes.contains(0x0052151CU) == false &&
                state.unmapped_bytes.contains(0x00525430U) == false &&
                state.unmapped_bytes.contains(0x00525448U) == false &&
                state.unmapped_bytes.contains(0x00525468U) == false,
            "global reset clears the unique battle color transition values without touching the separate initialization gate"
        );
        const std::span<const u32> workspace{action.opponent_workspace};
        test.expect_true(
            final_actor.active_actor_code == 0U &&
                final_actor.secondary_actor_code == 0U &&
                final_actor.published_actor_code == 1U &&
                final_actor.source_actor_code == 0xFFFFFFFFU &&
                final_actor.action_execution_active == 0U &&
                final_actor.auxiliary_gate == 0U &&
                port.battle_terminal_latch() == 0U &&
                final_actor.pre_frame_gate_a == 0U &&
                final_actor.pre_frame_gate_b == 0U &&
                final_actor.frame_gate_a == 0U &&
                final_actor.frame_gate_b == 0U &&
                final_actor.selection_gate == 0U &&
                final_actor.queued_actor_code == 0U &&
                final_actor.actor_runtime_records[0][0] == 9U &&
                actor_frames.shared.selection_aux_gate == 0U &&
                actor_frames.shared.target_ready_gate == 0U &&
                actor_frames.shared.action_block_gate == 0U &&
                actor_frames.shared.action.action_pending_aux == 0U &&
                actor_frames.shared.action_pending_secondary == 0U &&
                port.battle_message_state() == 0U &&
                std::ranges::all_of(
                    workspace.first(10U),
                    [](const u32 value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    workspace.subspan(10U, 6U),
                    [](const u32 value) { return value == 9U; }
                ) &&
                std::ranges::all_of(
                    workspace.subspan(16U, 80U),
                    [](const u32 value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    workspace.subspan(96U),
                    [](const u32 value) { return value == 9U; }
                ) &&
                state.unmapped_bytes.contains(0x004A754CU) == false &&
                state.unmapped_bytes.contains(0x004A7564U) == false &&
                state.unmapped_bytes.contains(0x0053BCE8U) == false &&
                state.unmapped_bytes.contains(0x0053BD50U) == false &&
                state.unmapped_bytes.contains(0x0053BF60U) == false &&
                state.unmapped_bytes.contains(0x0053BFB8U) == false &&
                state.unmapped_bytes.contains(0x0053C018U) == false,
            "global reset synchronizes pre-frame scalar aliases and only the two physical workspace ranges it actually writes"
        );
        const auto& shift = port.effect_shift_state();
        test.expect_true(
            shift.packed_reward == 0xAABBCCDDU && shift.actor_delta == 0 &&
                shift.direction_mode == 0U && shift.threshold_word == 0U &&
                shift.completion_latch == 0U &&
                port.battle_pair_secondary_value() == 0x7788U &&
                state.unmapped_bytes.contains(0x0053AE7AU) == false &&
                state.unmapped_bytes.contains(0x0053BD5CU) == false &&
                state.unmapped_bytes.contains(0x0053C000U) == false &&
                state.unmapped_bytes.contains(0x0053C4A0U) == false,
            "global reset updates effect-shift aliases without retaining unmapped duplicate bytes"
        );
        const auto& coordinator = port.effect_coordinator_state();
        test.expect_true(
            coordinator.primary[0].complete == 0U &&
                coordinator.required_completion_count == 0U &&
                coordinator.group_a_global_gate == 0U &&
                coordinator.group_a_effect_mode == 0U &&
                coordinator.group_b_global_gate == 9U &&
                coordinator.group_b_effect_mode == 0U &&
                coordinator.group_b_argument == 0U &&
                coordinator.completed_count == 0U &&
                coordinator.group_a_render_count == 0U &&
                coordinator.actor_activity_latch == 0U &&
                coordinator.selected_actor_pair == 0xAABBFFFFU &&
                coordinator.group_a_feedback_actor == 0xFFFFU &&
                coordinator.group_b_feedback_actor == 0xFFFFU &&
                port.battle_pair_primary_value() == 0U &&
                std::ranges::all_of(
                    coordinator.group_a_arguments,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    coordinator.feedback_primary,
                    [](const auto value) { return value == 9U; }
                ) &&
                state.unmapped_bytes.contains(0x005202A8U) == false &&
                state.unmapped_bytes.contains(0x0053BD40U) == false,
            "global reset writes the shared effect coordinator aliases and preserves untouched scan feedback state"
        );
        const auto& debug = port.battle_debug_hotkey_state();
        test.expect_true(
            debug.toggle_5244e0 == 7U &&
                debug.selection_status_word_53c050 == 0xABCD0000U &&
                debug.actor_retarget_gate_53bf64 == 0U &&
                debug.battle_mode_flags_53bc24 == 0U &&
                std::ranges::all_of(
                    debug.block_53af30,
                    [](const auto value) { return value == 0U; }
                ) &&
                debug.reset_gate_53bd50 == 0U && debug.screenshot_request == 7U,
            "global reset synchronizes debug hotkey aliases and preserves globals outside its write set"
        );
        test.expect_true(
            std::ranges::all_of(
                debug_overlay.selection_order,
                [](const auto value) { return value == 0U; }
            ) && debug_overlay.gate == 0U &&
                debug_overlay.resolved_actor_token == 0x11223344U &&
                debug_overlay.battle_selector == -1 &&
                debug_overlay.battle_mode == 0x22334455U &&
                debug_overlay.message_status == 0xAABBCC00U &&
                debug_overlay.selection_status == 0x33445566U &&
                debug_overlay.lock_count == 0x44556677U &&
                debug_overlay.tsw_cache_bytes == 0x55667788U &&
                debug_overlay.initial_mode == -1 &&
                debug_overlay.world_level == -7 &&
                debug_overlay.battle_frame == 0U &&
                debug_overlay.frame_divisor == -11 &&
                debug_overlay.marker_x == -13 &&
                debug_overlay.marker_row == -15 &&
                debug_overlay.text_buffer[0] == 'x' &&
                state.unmapped_bytes.contains(0x005214ACU) == false &&
                state.unmapped_bytes.contains(0x0053BD54U) == false &&
                state.unmapped_bytes.contains(0x004A754CU) == false &&
                state.unmapped_bytes.contains(0x004A7630U) == false &&
                state.unmapped_bytes.contains(0x004A7644U) == false &&
                state.unmapped_bytes.contains(0x0053BCECU) == false &&
                state.unmapped_bytes.contains(0x0053BF00U) == false &&
                state.unmapped_bytes.contains(0x0053BF74U) == false &&
                state.unmapped_bytes.contains(0x0053BFBCU) == false &&
                state.unmapped_bytes.contains(0x0053BDA0U) == false,
            "global reset synchronizes the debug overlay write set and preserves the high bytes of its byte store"
        );
        test.expect_true(
            std::ranges::all_of(
                startup.reset.block_525470,
                [](const auto value) { return value == 0U; }
            ) &&
                std::ranges::all_of(
                    startup.reset.block_5244e8,
                    [](const auto value) { return value == 0xFFFFFFFFU; }
                ) &&
                startup.reset.values_502940 ==
                    std::array<u32, 5>{0U, 0U, 0U, 0U, 0U} &&
                startup.reset.records_524788[0].value_00 == 0U &&
                startup.reset.records_524788[0].value_04 == 0U &&
                startup.reset.records_524788[0].value_08 == 0U &&
                startup.reset.records_524788[0].value_0a == 0U &&
                startup.reset.records_524788[0].value_0c == 0U &&
                startup.reset.records_524788[0].value_10 == 0U &&
                startup.reset.records_524788[0].value_14 == 0U &&
                startup.reset.records_524788[0].value_18 == 0U &&
                startup.enemies[0].role_id == 0U &&
                startup.party[0].role_id == 0U && startup.enemy_count == 0U &&
                startup.party_count == 0U,
            "existing startup typed aliases share the fixed global reset stores"
        );

        test.expect_true(
            state.write_trace.size() == 234U &&
                state.write_trace.front().address == 0x00502940U &&
                state.write_trace.front().value == 0U &&
                state.write_trace[232].address == 0x0053BFFCU &&
                state.write_trace.back().address == 0x0053C154U &&
                state.write_trace.back().count == 6U &&
                write_trace_hash(state) == 0x970D7E940E1225B2ULL,
            "write trace preserves all authoritative store tuples, the post-callee repeat, and final clear"
        );
        test.expect_true(
            byte_at(state, 0x004A7568U) == 2U &&
                byte_at(state, 0x004A7569U) == 0U &&
                byte_at(state, 0x004A7574U) == 0xFFU &&
                byte_at(state, 0x004A75FEU) == 0x10U &&
                byte_at(state, 0x0053C154U) == 0U &&
                state.unmapped_bytes.contains(0x00520E40U) == false &&
                byte_at(state, 0x00ABCDEFU) == 0x5AU,
            "unmapped byte image is little-endian, keeps mapped aliases unique, and preserves untouched bytes"
        );
    }

    {
        LegacyBattleGlobalResetState state;
        LegacyBattleStartupState startup;
        LegacyBattleFinalActorStepState final_actor;
        LegacyBattleActionDispatchState action;
        LegacyBattleGroupBFrameState actor_frames;
        LegacyBattleDebugOverlayState debug_overlay;
        ResetPort port;
        startup.reset.values_502940[0] = 0U;

        const auto result = openswd3::battle::reset_legacy_battle_globals(
            state,
            startup,
            final_actor,
            action,
            actor_frames,
            debug_overlay,
            port
        );
        test.expect_true(
            !result.conditional_allocation_released &&
                result.call_count == 8U &&
                std::ranges::find(
                    port.reset_calls,
                    LegacyBattleGlobalResetCall::release_conditional_allocation
                ) == port.reset_calls.end() &&
                result.call_order[3] ==
                    LegacyBattleGlobalResetCallStage::
                        pre_battle_resource_431960,
            "zero conditional token skips only the allocator call and keeps later call ordering"
        );
    }
}
