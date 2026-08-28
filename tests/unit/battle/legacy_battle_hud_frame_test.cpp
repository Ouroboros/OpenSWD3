#include "openswd3/battle/legacy_battle_hud_frame.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleHudCallReply;
using openswd3::battle::LegacyBattleHudCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u32;

class HudPort final : public openswd3::battle::LegacyBattleHudCallPort {
public:
    [[nodiscard]] LegacyBattleHudCallReply
    invoke_hud(const LegacyBattleHudCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        return default_reply;
    }

    void push(const u32 callee, const LegacyBattleHudCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleHudCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    LegacyBattleHudCallReply default_reply{};
    std::unordered_map<u32, std::deque<LegacyBattleHudCallReply>> replies;
    std::vector<LegacyBattleHudCallRequest> calls;
};

[[nodiscard]] LegacyBattleHudCallReply
pair_reply(const u32 first, const u32 second) {
    LegacyBattleHudCallReply reply{};
    reply.outputs[0] = first;
    reply.outputs[1] = second;
    return reply;
}

[[nodiscard]] bool has_argument(
    const HudPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleHudCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_hud_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleHudFrameState;
    using openswd3::battle::LegacyBattleHudFrameStatus;

    {
        LegacyBattleHudFrameState state;
        HudPort port;
        port.push(0x00435660U, {.eax = 0x12345678U});
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.status == LegacyBattleHudFrameStatus::completed &&
                result.return_value == 0x12345678U && result.port_calls == 2U &&
                result.top_actor_rows == 0U && result.actor_rows == 0U &&
                has_argument(port, 0x00435670U, 0U, 0x004C9A28U) &&
                has_argument(port, 0x00435660U, 1U, 0xFFFEU),
            "empty HUD preserves font-style EAX and performs only two font calls"
        );
    }

    {
        LegacyBattleHudFrameState state;
        state.active_actor_count = 11;
        state.actor_skip_primary.fill(1U);
        HudPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.status ==
                    LegacyBattleHudFrameStatus::actor_index_typed_stop &&
                result.top_actor_rows == 0U && result.actor_rows == 0U,
            "fixed ten-actor storage stops at the first eleventh actor access"
        );
    }

    {
        LegacyBattleHudFrameState state;
        state.active_actor_count = 1;
        state.side_mode = 1U;
        state.actor_active[0] = 1U;
        state.actor_skip_primary[0] = 1U;
        state.top_pulse = -2;
        HudPort port;
        port.push(0x00480AD0U, {.eax = 0x7000U});
        port.push(0x00484500U, pair_reply(50U, 100U));
        port.push(0x00478340U, {.eax = 5U});
        port.push(0x004239D0U, {.eax = 0x99U});
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.status == LegacyBattleHudFrameStatus::completed &&
                result.top_actor_rows == 1U && state.top_pulse == 1 &&
                result.x87_conversions == 1U && result.text_panel_calls == 1U &&
                result.text_panels.size() == 1U &&
                !result.text_panels[0U].used_default_text_position &&
                result.text_panels[0U].action_entry.eax == 0x7000U &&
                result.text_panels[0U].action_entry.ecx == 12U &&
                result.text_panels[0U].action_entry.edx == 15U &&
                port.count(
                    openswd3::battle::kLegacyBattleHudReservedTextPanelSlot
                ) == 0U &&
                has_argument(port, 0x0043B110U, 0U, 10U) &&
                has_argument(port, 0x00436AD0U, 2U, 15U) &&
                has_argument(port, 0x00450490U, 2U, 110U) &&
                has_argument(port, 0x00450490U, 4U, 28U) &&
                has_argument(port, 0x00450A50U, 4U, 0x99U) &&
                has_argument(port, 0x0043B110U, 4U, 1U),
            "top actor row uses side position, x87 width, status fade and wrapped pulse"
        );
    }

    {
        LegacyBattleHudFrameState state;
        state.active_actor_count = 1;
        state.display_order[0] = 2U;
        state.status_x[2] = 100;
        state.value_x[2] = 120;
        state.bar_x[2] = 140;
        state.selected_actor_code = 8;
        state.selected_pulse = 8U;
        state.selected_pulse_counter = 2U;
        state.actor_status_mode[0] = 1U;
        state.actor_value[0] = 40;
        state.actor_value_display[0] = 20;
        state.actor_value_target[0] = 30;
        state.primary_value_snapshot[0] = 20;
        state.secondary_value_snapshot[0] = 0;
        state.tertiary_value_snapshot[0] = 20;
        state.tertiary_display[0] = 10;
        state.tertiary_display_target[0] = 0;
        HudPort port;
        port.push(0x0047D930U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 1U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x00478340U, {.eax = 7U});
        port.push(0x00484500U, pair_reply(30U, 90U));
        port.push(0x004838A0U, pair_reply(2U, 4U));
        port.push(0x00483870U, pair_reply(12U, 24U));
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.status == LegacyBattleHudFrameStatus::completed &&
                result.actor_rows == 1U && state.selected_pulse == 0x89U &&
                state.selected_pulse_counter == 0U &&
                state.status_blink_counter[0] == 1U &&
                state.actor_value_display[0] == 21 &&
                state.primary_delta[0] == -9 && state.primary_step[0] == 1 &&
                state.primary_display_target[0] == 18 &&
                state.secondary_delta[0] == -1 &&
                state.secondary_display_target[0] == 28 &&
                state.tertiary_delta[0] == 7 &&
                state.tertiary_display[0] == 9 &&
                result.x87_conversions == 2U && port.count(0x0047CE80U) == 3U &&
                port.count(0x00436AD0U) == 3U &&
                has_argument(port, 0x0043B110U, 0U, 116U) &&
                has_argument(port, 0x004502B0U, 3U, 0U) &&
                has_argument(port, 0x004506B0U, 0U, 0x2355U) &&
                has_argument(port, 0x004506B0U, 1U, 21U) &&
                has_argument(port, 0x004506B0U, 1U, 1U) &&
                has_argument(port, 0x004506B0U, 3U, 446U),
            "actor HUD preserves pulse, repeated blocked queries and asymmetric smoothers"
        );
    }

    {
        LegacyBattleHudFrameState state;
        state.active_actor_count = 1;
        state.actor_value_tokens[0] = 0U;
        state.actor_value_display[0] = 0;
        state.actor_value_target[0] = 20;
        HudPort port;
        port.push(0x0047D930U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.status ==
                    LegacyBattleHudFrameStatus::actor_value_typed_stop &&
                state.actor_value_display[0] == 3 &&
                port.count(0x004505B0U) == 1U && port.count(0x00484500U) == 0U,
            "null actor value stops after unequal-display draw and easing prefix"
        );
    }

    {
        LegacyBattleHudFrameState state;
        state.active_actor_count = 1;
        state.primary_value_snapshot[0] = 0x08000000;
        state.actor_skip_primary[0] = 0U;
        HudPort port;
        port.push(0x0047D930U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x0047CE80U, {.eax = 0U});
        port.push(0x00484500U, pair_reply(0U, 0U));
        port.push(0x004838A0U, pair_reply(0U, 0U));
        port.push(0x00483870U, pair_reply(0U, 0U));
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.status == LegacyBattleHudFrameStatus::completed &&
                state.primary_step[0] == -13421771 &&
                state.primary_delta[0] == 0 &&
                state.primary_value_snapshot[0] == 0 &&
                state.primary_display_target[0] == 0 &&
                result.x87_conversions == 3U,
            "bit twenty-seven complement bug and zero-denominator x87 low dword survive"
        );
    }

    {
        LegacyBattleHudFrameState state;
        state.footer_mode = 1U;
        HudPort port;
        port.push(0x00436AD0U, {.eax = 0xCAFEBABEU});
        const auto result =
            openswd3::battle::advance_legacy_battle_hud_frame(state, port);
        test.expect_true(
            result.return_value == 0xCAFEBABEU && state.footer_delta == 22 &&
                state.footer_position == 22 && result.text_panel_calls == 1U &&
                result.text_panels.size() == 1U &&
                result.text_panels[0U].used_default_text_position &&
                result.text_panels[0U].action_entry.eax == 0U &&
                result.text_panels[0U].action_entry.ecx == 68U &&
                result.text_panels[0U].action_entry.edx == 22U &&
                port.count(
                    openswd3::battle::kLegacyBattleHudReservedTextPanelSlot
                ) == 0U &&
                has_argument(port, 0x0043B110U, 0U, 0xFFFFFFDCU) &&
                has_argument(port, 0x0043B110U, 1U, 354U) &&
                has_argument(port, 0x00436AD0U, 2U, 0xFFFFFFDEU) &&
                has_argument(port, 0x00436AD0U, 4U, 0x004A7814U),
            "footer uses signed third-step convergence and returns text EAX"
        );
    }
}
