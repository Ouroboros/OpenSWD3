#include "openswd3/battle/legacy_battle_text_panel.hpp"
#include "test.hpp"

#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleTextPanelCall;
using openswd3::battle::LegacyBattleTextPanelCallReply;
using openswd3::battle::LegacyBattleTextPanelCallRequest;
using openswd3::compat::u32;

class TextPanelPort final : public openswd3::battle::LegacyBattleTextPanelPort {
public:
    [[nodiscard]] LegacyBattleTextPanelCallReply invoke_text_panel(
        const LegacyBattleTextPanelCallRequest& request
    ) override {
        calls.push_back(request);
        if (replies.empty()) {
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    void push(const LegacyBattleTextPanelCallReply& reply) {
        replies.push_back(reply);
    }

    std::deque<LegacyBattleTextPanelCallReply> replies;
    std::vector<LegacyBattleTextPanelCallRequest> calls;
};

}  // namespace

void test_battle_text_panel(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleTextPanelRequest;
    using openswd3::battle::LegacyBattleVictoryRewardState;
    using openswd3::battle::draw_legacy_battle_text_panel;

    {
        LegacyBattleVictoryRewardState state;
        TextPanelPort port;
        port.push({
            .eax = 0x10101010U,
            .ecx = 0x20202020U,
            .edx = 0x30303030U,
            .publish_action_field_4a = true,
            .action_field_4a = 0x4567U,
        });
        port.push({.eax = 1U, .ecx = 2U, .edx = 3U});
        port.push({.eax = 4U, .ecx = 5U, .edx = 0xA1B2C3D4U});
        port.push({.eax = 0x51515151U, .ecx = 0x52525252U, .edx = 0x53535353U});
        const auto result = draw_legacy_battle_text_panel(
            state,
            port,
            {
                .left = 10,
                .top = 20,
                .width = 170,
                .height = 20,
                .text_x = 15,
                .text_y = 22,
                .text_token = 0x12345678U,
                .entry = {.eax = 7U, .ecx = 8U, .edx = 9U},
            }
        );
        test.expect_true(
            result.port_calls == 4U && result.action_calls == 1U &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U && result.text_calls == 1U &&
                result.call_trace ==
                    std::vector<LegacyBattleTextPanelCall>{
                        LegacyBattleTextPanelCall::update_action,
                        LegacyBattleTextPanelCall::draw_rectangle,
                        LegacyBattleTextPanelCall::draw_tiled_frame,
                        LegacyBattleTextPanelCall::draw_text,
                    } &&
                state.panel_action_record.action_id == 0x233BU &&
                state.panel_action_record.base_variant == 0U &&
                state.panel_action_record.field_4a == 0x4567U &&
                port.calls[0U].arguments[0U] == 0x004FC5B0U,
            "text panel updates the shared panel action and preserves call order"
        );
        test.expect_true(
            result.action_entry.eax == 7U && result.action_entry.ecx == 8U &&
                result.action_entry.edx == 9U &&
                result.rectangle_entry.eax == 170U &&
                result.rectangle_entry.ecx == 0x20202020U &&
                result.rectangle_entry.edx == 0x30303030U &&
                result.tiled_frame_entry.eax == 176U &&
                result.tiled_frame_entry.ecx == 14U &&
                result.tiled_frame_entry.edx == 0x00004567U &&
                result.text_entry.eax == 0x004CD76CU &&
                result.text_entry.ecx == 0x004C9A28U &&
                result.text_entry.edx == 0x12345678U &&
                result.return_registers.eax == 0x51515151U &&
                result.return_registers.ecx == 0x52525252U &&
                result.return_registers.edx == 0x53535353U,
            "explicit text coordinates preserve each call-site register chain"
        );
        test.expect_true(
            result.frame_left == 14 && result.frame_top == 24 &&
                result.frame_right == 176 && result.frame_bottom == 36 &&
                result.resolved_text_x == 15 && result.resolved_text_y == 22 &&
                !result.used_default_text_position &&
                port.calls[1U].arguments ==
                    std::array<u32, 8U>{10U, 20U, 170U, 20U, 0U, 4U, 4U, 0U} &&
                port.calls[2U].arguments ==
                    std::array<u32, 8U>{
                        0x00004567U, 14U, 24U, 176U, 36U, 0U, 0x80000008U, 0U
                    } &&
                port.calls[3U].arguments ==
                    std::array<u32, 8U>{
                        0x004C9A28U,
                        0x004CD76CU,
                        15U,
                        22U,
                        0x12345678U,
                        0xFFC0U,
                        16U,
                        0U
                    },
            "explicit text panel forwards rectangle frame and text arguments exactly"
        );
    }

    {
        LegacyBattleVictoryRewardState state;
        TextPanelPort port;
        port.push({
            .eax = 1U,
            .ecx = 2U,
            .edx = 3U,
            .publish_action_field_4a = true,
            .action_field_4a = 0x00AAU,
        });
        port.push({.eax = 4U, .ecx = 5U, .edx = 6U});
        port.push({.eax = 7U, .ecx = 8U, .edx = 0xAABBCCDDU});
        port.push({.eax = 9U, .ecx = 10U, .edx = 11U});
        const auto result = draw_legacy_battle_text_panel(
            state,
            port,
            {
                .left = -60,
                .top = 354,
                .width = 0x12340046,
                .height = 24,
                .text_x = 0,
                .text_y = 0,
                .text_token = 0x004A7814U,
                .entry = {.eax = 1U, .ecx = 0xFFFFFFF8U, .edx = 0xFFFFFFFEU},
            }
        );
        test.expect_true(
            result.used_default_text_position &&
                result.resolved_text_x == -58 &&
                result.resolved_text_y == 358 && result.frame_left == -56 &&
                result.frame_top == 358 &&
                result.frame_resource == 0x123400AAU &&
                result.text_entry.eax == 0x004A7814U &&
                result.text_entry.ecx == 0x004C9A28U &&
                result.text_entry.edx == 0xAABBCCDDU &&
                port.calls[3U].arguments[2U] == 0xFFFFFFC6U &&
                port.calls[3U].arguments[3U] == 358U,
            "zero coordinate pair uses wrapped panel-relative text geometry and stale tiled EDX"
        );
    }

    {
        LegacyBattleVictoryRewardState state;
        TextPanelPort port;
        const auto result = draw_legacy_battle_text_panel(
            state,
            port,
            {
                .left = 1,
                .top = 2,
                .width = 3,
                .height = 4,
                .text_x = 0,
                .text_y = 9,
                .text_token = 10U,
            }
        );
        test.expect_true(
            !result.used_default_text_position && result.resolved_text_x == 0 &&
                result.resolved_text_y == 9 &&
                result.text_entry.eax == 0x004CD76CU &&
                result.text_entry.edx == 10U,
            "one nonzero text coordinate selects the explicit branch"
        );
    }
}
