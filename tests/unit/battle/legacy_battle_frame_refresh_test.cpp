#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_frame_refresh.hpp"
#include "test.hpp"

#include <algorithm>
#include <bit>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActionDispatchPort;
using openswd3::battle::LegacyBattleEffectCallPort;
using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::compat::u32;

class SharedFrameRefreshPort final : public LegacyBattleActionDispatchPort,
                                     public LegacyBattleEffectCallPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest&) override {
        return {};
    }

    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest&) override {
        return {};
    }
};

class FrameRefreshPort final
    : public LegacyBattleActionDispatchPort,
      public openswd3::battle::LegacyBattleFrameEffectPort {
public:
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

    [[nodiscard]] openswd3::battle::LegacyBattleActionRotationUpdateSnapshot
    update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        return {.domain_token = 1U};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleFrameEffectSurfaceReply
    surface_operation(
        const openswd3::battle::LegacyBattleFrameEffectSurfaceRequest& request
    ) override {
        surface_calls.push_back(request);
        return {.callee_returned = true};
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

    std::function<void(const LegacyBattleActionCallRequest&)> on_call;
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
    std::vector<openswd3::battle::LegacyBattleFrameEffectSurfaceRequest>
        surface_calls;
};

[[nodiscard]] const LegacyBattleActionCallRequest* find_call(
    const FrameRefreshPort& port, const u32 callee, const std::size_t ordinal
) {
    std::size_t found = 0U;
    for (const auto& request : port.calls) {
        if (request.callee_token == callee && found++ == ordinal) {
            return &request;
        }
    }
    return nullptr;
}

}  // namespace

void test_battle_frame_refresh(openswd3::test::Context& test) {
    using openswd3::battle::refresh_legacy_battle_frame;

    {
        FrameRefreshPort port;
        auto& state = port.frame_refresh_state();
        auto& control = port.frame_effect_control_state();
        state.surface_tokens = {0x1111U, 0x2222U};
        state.viewport_token = 0x4444U;
        state.final_surface_token = 0x5555U;
        control.red_factor = 3;
        control.green_factor = 4;
        control.blue_factor = 5;
        bool final_lock_saw_snapshots = false;
        port.on_call = [&](const LegacyBattleActionCallRequest& request) {
            if (request.callee_token == 0x00485330U) {
                control.red_factor = port.count(0x00485330U) == 1U ? 7 : -32768;
            } else if (request.callee_token == 0x00420560U) {
                control.green_factor =
                    port.count(0x00420560U) == 1U ? -9 : 32767;
            } else if (request.callee_token == 0x00420600U) {
                control.blue_factor = port.count(0x00420600U) == 1U ? -11 : -1;
            } else if (
                request.callee_token == 0x004206F0U &&
                port.count(0x004206F0U) == 2U
            ) {
                control.red_factor = -3;
                control.green_factor = 5;
                control.blue_factor = -7;
            } else if (
                request.callee_token == 0x00416F10U &&
                port.count(0x00416F10U) == 3U
            ) {
                final_lock_saw_snapshots = state.refresh_pending == 1U &&
                    state.snapshot_word_36 == 0xFFFDU &&
                    state.snapshot_word_38 == 5U &&
                    state.snapshot_word_3a == 0xFFF9U;
                control.red_factor = 0;
                control.green_factor = 0;
                control.blue_factor = 0;
            }
        };
        const auto result = refresh_legacy_battle_frame(port);
        constexpr std::array<u32, 3> channels{
            0x00420560U, 0x00420600U, 0x004206F0U
        };
        constexpr std::array<u32, 3> first{3U, 0xFFFFFFFBU, 0xFFFFFFFAU};
        constexpr std::array<u32, 3> second{0xFFFF8000U, 0x7FFEU, 0xFFFFFFFEU};
        bool factors_match = true;
        for (std::size_t index = 0; index < channels.size(); ++index) {
            const auto* first_call = find_call(port, channels[index], 0U);
            const auto* second_call = find_call(port, channels[index], 1U);
            factors_match = factors_match && first_call != nullptr &&
                second_call != nullptr &&
                first_call->arguments[2] == first[index] &&
                second_call->arguments[2] == second[index];
        }

        test.expect_true(
            result.refreshed && result.port_calls == 16U && factors_match &&
                final_lock_saw_snapshots && control.red_factor == 0 &&
                control.green_factor == 0 && control.blue_factor == 0 &&
                state.snapshot_word_36 == 0xFFFDU &&
                state.snapshot_word_38 == 5U &&
                state.snapshot_word_3a == 0xFFF9U &&
                state.active_surface_token == 0x5555U,
            "port mutations are read at each original WORD site while the final lock keeps current colors distinct from published snapshots"
        );
    }

    {
        SharedFrameRefreshPort port;
        LegacyBattleActionDispatchPort& action_port = port;
        LegacyBattleEffectCallPort& effect_port = port;
        action_port.frame_refresh_state().snapshot_word_36 = 0x1234U;
        action_port.frame_effect_control_state().red_factor = -32768;
        action_port.frame_effect_control_state().primary_suppression =
            0x12345678U;
        test.expect_true(
            &action_port.frame_refresh_state() ==
                    &effect_port.frame_refresh_state() &&
                effect_port.frame_refresh_state().snapshot_word_36 == 0x1234U &&
                effect_port.frame_refresh_state().refresh_pending == 0U &&
                effect_port.frame_refresh_state().active_surface_token ==
                    0xFFFFFFFFU &&
                &action_port.frame_effect_control_state() ==
                    &effect_port.frame_effect_control_state() &&
                effect_port.frame_effect_control_state().red_factor == -32768 &&
                effect_port.frame_effect_control_state().primary_suppression ==
                    0x12345678U &&
                effect_port.frame_effect_control_state()
                        .secondary_suppression == 0U,
            "action and effect ports share the physical refresh and control storage"
        );
    }

    {
        FrameRefreshPort port;
        auto& state = port.frame_refresh_state();
        state.snapshot_word_36 = 0x1234U;
        state.snapshot_word_38 = 0x5678U;
        state.snapshot_word_3a = 0x9ABCU;
        state.entry_eax = 0xABCD0000U;
        state.entry_ecx = 0x13570000U;
        state.entry_edx = 0x24680000U;
        auto& control = port.frame_effect_control_state();
        control.red_factor = 0x1234;
        control.green_factor = 0x5678;
        control.blue_factor = std::bit_cast<openswd3::compat::i16>(
            openswd3::compat::u16{0x9ABCU}
        );
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            !result.refreshed && result.port_calls == 0U &&
                result.return_value == 0xABCD1234U &&
                result.final_ecx == 0x13575678U &&
                result.final_edx == 0x24689ABCU,
            "unchanged words return after three low-word register comparisons"
        );
    }

    {
        FrameRefreshPort port;
        auto& state = port.frame_refresh_state();
        state.surface_tokens = {0x1111U, 0x2222U};
        state.source_pitch = 0x3333U;
        state.viewport_token = 0x4444U;
        state.final_surface_token = 0x5555U;
        port.push(0x00416F10U, {.eax = 0xAAAAU});
        port.push(0x00416F10U, {.eax = 0xBBBBU});
        port.push(0x00416F10U, {.eax = 0xCCCCU});
        port.push(0x00416F60U, {});
        port.push(0x00416F60U, {});
        port.push(0x00416F60U, {.eax = 0xDEADBEEFU});
        auto& control = port.frame_effect_control_state();
        control.red_factor = 3;
        control.green_factor = -1;
        control.blue_factor = -3;
        const auto result = refresh_legacy_battle_frame(port);
        const auto* first_red = find_call(port, 0x00420560U, 0U);
        const auto* second_red = find_call(port, 0x00420560U, 1U);
        const auto* first_green = find_call(port, 0x00420600U, 0U);
        const auto* second_green = find_call(port, 0x00420600U, 1U);
        const auto* first_blue = find_call(port, 0x004206F0U, 0U);
        const auto* second_blue = find_call(port, 0x004206F0U, 1U);
        const auto* final_lock = find_call(port, 0x00416F10U, 2U);
        const auto* final_unlock = find_call(port, 0x00416F60U, 2U);
        test.expect_true(
            result.refreshed && result.port_calls == 16U &&
                result.surface_iterations == 2U &&
                result.return_value == 0xDEADBEEFU &&
                state.snapshot_word_36 == 3U &&
                state.snapshot_word_38 == 0xFFFFU &&
                state.snapshot_word_3a == 0xFFFDU &&
                state.captured_pitch == 0x3333U &&
                state.refresh_pending == 1U &&
                state.active_surface_token == 0x5555U &&
                state.last_lock_token == 0xCCCCU && first_red != nullptr &&
                second_red != nullptr && first_green != nullptr &&
                second_green != nullptr && first_blue != nullptr &&
                second_blue != nullptr && first_red->arguments[2] == 1U &&
                second_red->arguments[2] == 2U &&
                first_green->arguments[2] == 0xFFFFFFFFU &&
                second_green->arguments[2] == 0xFFFFFFFEU &&
                first_blue->arguments[2] == 0xFFFFFFFEU &&
                second_blue->arguments[2] == 0xFFFFFFFCU &&
                final_lock != nullptr && final_lock->eax == 0x5555U &&
                final_lock->arguments[0] == 0x4444U &&
                final_unlock != nullptr &&
                final_unlock->arguments[0] == 0x4444U &&
                final_unlock->arguments[1] == 0xCCCCU,
            "changed words refresh two surfaces with signed SAR halves then publish final lock"
        );
        test.expect_true(
            port.count(0x00485330U) == 2U && port.count(0x004170E0U) == 2U &&
                port.count(0x00420560U) == 2U &&
                port.count(0x00420600U) == 2U && port.count(0x004206F0U) == 2U,
            "refresh keeps the exact two-iteration static call schedule"
        );

        openswd3::battle::LegacyBattleFrameEffectState effect;
        control.primary_suppression = 1U;
        auto action = std::make_unique<
            openswd3::battle::LegacyBattleActionDispatchState>();
        action->current_actor_index = 0U;
        port.actor_metric_state().priority_actor_index = 0U;
        effect.cadence = 2;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        openswd3::rendering::LegacyRasterGeometryState raster{};
        static_cast<void>(openswd3::rendering::initialize_legacy_raster_geometry(
            raster, framebuffer.geometry().surface
        ));
        openswd3::rendering::LegacyBlitRequest blit{};
        openswd3::rendering::LegacyBlitEffectState effects{};
        openswd3::rendering::LegacyRleRowJitterState jitter{};
        openswd3::battle::LegacyBattleFrameEffectContext context{
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = blit,
            .shared_effects = effects,
            .jitter = jitter,
            .pending_rotation = port.effect_shift_state().actor_delta,
            .flash = port.screen_flash_state(),
            .refresh = state,
            .control = control,
            .current_actor_index = action->current_actor_index,
            .priority_actor_index =
                port.actor_metric_state().priority_actor_index,
        };
        const std::array<u32, 3> surfaces{0xA000U, 0xA100U, 0xA200U};
        const auto growth = openswd3::battle::update_legacy_battle_frame_effect(
            effect, port, context, {}, surfaces, 0
        );
        test.expect_true(
            growth.status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::completed &&
                growth.surface_operation_calls == 1U &&
                port.surface_calls.back().source_token == 0xA100U &&
                state.refresh_pending == 2U &&
                state.active_surface_token == 0x5555U,
            "actual refresh publishes the stage consumed by the following effect without synchronizing a copy"
        );
        action->current_actor_index = 1U;
        effect.fade_active = 1U;
        const auto fade = openswd3::battle::update_legacy_battle_frame_effect(
            effect, port, context, {}, surfaces, 0
        );
        test.expect_true(
            fade.status ==
                    openswd3::battle::LegacyBattleFrameEffectStatus::completed &&
                fade.surface_operation_calls == 1U &&
                port.surface_calls.back().source_token == 0xA100U &&
                port.surface_calls.back().effect_flags == 0U &&
                state.refresh_pending == 1U &&
                state.active_surface_token == 0x5555U,
            "fade tests the actual active token as a sentinel and indexes surfaces by stage"
        );
    }
}
