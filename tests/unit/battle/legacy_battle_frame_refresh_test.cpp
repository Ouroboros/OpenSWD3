#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_frame_refresh.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
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

class FrameRefreshPort final : public LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
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

    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
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
        SharedFrameRefreshPort port;
        LegacyBattleActionDispatchPort& action_port = port;
        LegacyBattleEffectCallPort& effect_port = port;
        action_port.frame_refresh_state().snapshot_word_36 = 0x1234U;
        test.expect_true(
            &action_port.frame_refresh_state() ==
                    &effect_port.frame_refresh_state() &&
                effect_port.frame_refresh_state().snapshot_word_36 == 0x1234U,
            "action and effect ports share one physical refresh storage"
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
        const auto result =
            refresh_legacy_battle_frame(port, 0x1234U, 0x5678U, 0x9ABCU);
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
        const auto result =
            refresh_legacy_battle_frame(port, 3U, 0xFFFFU, 0xFFFDU);
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
    }
}
