#include "openswd3/battle/legacy_battle_growth_item_completion_panel.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleGrowthItemCompletionPanelCall;
using openswd3::battle::LegacyBattleGrowthItemCompletionPanelCallReply;
using openswd3::battle::LegacyBattleGrowthItemCompletionPanelCallRequest;
using openswd3::battle::LegacyBattleGrowthItemCompletionPanelRequest;
using openswd3::compat::u8;
using openswd3::compat::u32;

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (resource_ids.size() > successful_loads || piece_index >= 9U) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = pixels[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::array<u8, 2U>, 9U> pixels{{
        {1U, 0U},
        {2U, 0U},
        {3U, 0U},
        {4U, 0U},
        {5U, 0U},
        {6U, 0U},
        {7U, 0U},
        {8U, 0U},
        {9U, 0U},
    }};
    std::vector<u32> resource_ids;
    std::size_t successful_loads{1000U};
};

class Port final
    : public openswd3::battle::LegacyBattleGrowthItemCompletionPanelPort {
public:
    [[nodiscard]] LegacyBattleGrowthItemCompletionPanelCallReply
    invoke_growth_item_completion_panel(
        const LegacyBattleGrowthItemCompletionPanelCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found != replies.end() && index < found->second.size()) {
            return found->second[index++];
        }
        return LegacyBattleGrowthItemCompletionPanelPort::
            invoke_growth_item_completion_panel(request);
    }

    void reply(
        const LegacyBattleGrowthItemCompletionPanelCall call,
        const LegacyBattleGrowthItemCompletionPanelCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32
    count(const LegacyBattleGrowthItemCompletionPanelCall call) const noexcept {
        return static_cast<u32>(std::count_if(
            calls.begin(), calls.end(), [call](const auto& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleGrowthItemCompletionPanelCallRequest> calls;
    std::map<
        LegacyBattleGrowthItemCompletionPanelCall,
        std::vector<LegacyBattleGrowthItemCompletionPanelCallReply>>
        replies;
    std::map<LegacyBattleGrowthItemCompletionPanelCall, std::size_t>
        reply_indices;
};

struct Fixture {
    Fixture() : raster(framebuffer.geometry()) {
        target.transition_mode = 1U;
        target.transition_stage = 12U;
        victory.panel_action_record.field_4a = 0x4567U;
        advancement.growth_caption_text = {0x41U, 0x42U, 0U};
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleGrowthItemCompletionPanelBindings bindings() {
        return {
            .level_advancement = advancement,
            .target_selection = target,
            .victory_rewards = victory,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleLevelAdvancementState advancement;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    Port port;
};

[[nodiscard]] openswd3::battle::LegacyBattleGrowthItemCompletionPanelResult
run(Fixture& fixture,
    const LegacyBattleGrowthItemCompletionPanelRequest& request = {
        .initial_text_byte = 0x7FU,
        .entry_eax = 0x11112222U,
        .entry_ecx = 0x33334444U,
        .entry_edx = 0x55556666U,
        .rectangle_return =
            {
                .eax = 0x77778888U,
                .ecx = 0x9999AAAAU,
                .edx = 0xABCD9999U,
            },
        .frame_return =
            {
                .eax = 0xBBBBCCCCU,
                .ecx = 0xDDDDEEEEU,
                .edx = 0xFFFF0000U,
            },
        .local_text_token = 0x70003000U,
    }) {
    return openswd3::battle::advance_legacy_battle_growth_item_completion_panel(
        fixture.bindings(), fixture.port, request
    );
}

}  // namespace

void test_battle_growth_item_completion_panel(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleGrowthItemCompletionFontToken;
    using openswd3::battle::kLegacyBattleGrowthItemCompletionFramebufferToken;
    using openswd3::battle::LegacyBattleGrowthItemCompletionPanelStatus;

    {
        Fixture fixture;
        fixture.target.transition_mode = 0U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemCompletionPanelStatus::completed &&
                result.port_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0x55556666U &&
                result.formatted_text[0U] == 0x7FU &&
                result.formatted_text[1U] == 0U,
            "growth item completion panel initializes its local text then obeys the exact-one mode gate"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            LegacyBattleGrowthItemCompletionPanelCall::query_panel,
            {.eax = 1U, .ecx = 0x01020304U, .edx = 0x05060708U}
        );
        fixture.port.reply(
            LegacyBattleGrowthItemCompletionPanelCall::set_font_size,
            {.eax = 0x11111111U, .ecx = 0x22222222U, .edx = 0x33333333U}
        );
        fixture.port.reply(
            LegacyBattleGrowthItemCompletionPanelCall::draw_text,
            {.eax = 0x44444444U, .ecx = 0x55555555U, .edx = 0x66666666U}
        );
        fixture.port.reply(
            LegacyBattleGrowthItemCompletionPanelCall::set_font_size,
            {.eax = 0x77777777U, .ecx = 0x88888888U, .edx = 0x99999999U}
        );

        const auto result = run(fixture);

        constexpr std::array<u8, 18U> kExpectedText{
            0xAAU,
            0x6BU,
            0xC4U,
            0x5FU,
            0x41U,
            0x42U,
            0xA4U,
            0x77U,
            0xA7U,
            0xB9U,
            0xA5U,
            0xFEU,
            0xA6U,
            0xA8U,
            0xAAU,
            0xF8U,
            0x21U,
            0x21U,
        };
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemCompletionPanelStatus::completed &&
                result.port_calls == 7U && result.format_calls == 1U &&
                result.length_calls == 2U && result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U && result.query_calls == 1U &&
                result.font_size_calls == 2U && result.text_draw_calls == 1U &&
                result.first_measured_length == 18U &&
                result.second_measured_length == 18U &&
                result.half_text_length == 9 &&
                result.panel_base_width == 185U &&
                result.rectangle_width == 197 &&
                result.rectangle_height == 20 &&
                result.frame_resource_id == 0xABCD4567U &&
                result.frame_right == 385 && result.frame_bottom == 224 &&
                result.formatted_text_length == kExpectedText.size() &&
                std::equal(
                    kExpectedText.begin(),
                    kExpectedText.end(),
                    result.formatted_text.begin()
                ) &&
                result.formatted_text[kExpectedText.size()] == 0U &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xABCD4567U &&
                result.return_eax == 0x77777777U &&
                result.return_ecx == 0x88888888U &&
                result.return_edx == 0x99999999U,
            "growth item completion panel formats the CP950 message, draws its dynamic frame and restores font size"
        );
        test.expect_true(
            fixture.port.calls[0U].call ==
                    LegacyBattleGrowthItemCompletionPanelCall::format_text &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == 0x70003000U &&
                fixture.port.calls[0U].edx == 0x55556666U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleGrowthItemCompletionPanelCall::measure_text &&
                fixture.port.calls[1U].eax == 18U &&
                fixture.port.calls[1U].ecx == 0x70003000U &&
                fixture.port.calls[1U].edx == 0x70003000U &&
                fixture.port.calls[2U].eax == 0x70003000U &&
                fixture.port.calls[2U].ecx == 0xDDDDEEEEU &&
                fixture.port.calls[2U].edx == 0xFFFF0000U &&
                fixture.port.calls[3U].arguments[0U] == 0xD4U &&
                fixture.port.calls[3U].arguments[1U] == 0xF4U &&
                fixture.port.calls[3U].arguments[2U] == 3U &&
                fixture.port.calls[4U].eax == 1U &&
                fixture.port.calls[4U].ecx ==
                    kLegacyBattleGrowthItemCompletionFontToken &&
                fixture.port.calls[4U].edx == 0x05060708U &&
                fixture.port.calls[4U].arguments[1U] == 0x11U &&
                fixture.port.calls[5U].eax == 0x11111111U &&
                fixture.port.calls[5U].ecx ==
                    kLegacyBattleGrowthItemCompletionFontToken &&
                fixture.port.calls[5U].edx ==
                    kLegacyBattleGrowthItemCompletionFramebufferToken &&
                fixture.port.calls[5U].arguments[1U] == 0xD8U &&
                fixture.port.calls[5U].arguments[2U] == 0xDAU &&
                fixture.port.calls[5U].arguments[4U] == 0xFFC0U &&
                fixture.port.calls[6U].eax == 0x44444444U &&
                fixture.port.calls[6U].ecx ==
                    kLegacyBattleGrowthItemCompletionFontToken &&
                fixture.port.calls[6U].edx == 0x66666666U &&
                fixture.port.calls[6U].arguments[1U] == 0x10U,
            "growth item completion panel preserves all seven generic callsite register layouts"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            LegacyBattleGrowthItemCompletionPanelCall::query_panel,
            {.eax = 2U, .ecx = 0x12345678U, .edx = 0x90ABCDEFU}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemCompletionPanelStatus::completed &&
                result.port_calls == 4U && result.font_size_calls == 0U &&
                result.text_draw_calls == 0U && result.return_eax == 2U &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0x90ABCDEFU,
            "growth item completion panel draws no text and preserves the non-one panel query return"
        );
    }

    {
        Fixture fixture;
        fixture.advancement.growth_caption_text.fill(0x58U);
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemCompletionPanelStatus::
                        caption_source_typed_stop &&
                result.port_calls == 1U && result.format_calls == 1U &&
                result.rectangle_calls == 0U &&
                result.formatted_text_length == 28U &&
                result.formatted_text[0U] == 0xAAU &&
                result.formatted_text[3U] == 0x5FU &&
                result.formatted_text[27U] == 0x58U,
            "growth item completion panel stops after the 24-byte shared caption prefix when no terminator is present"
        );
    }

    {
        Fixture fixture;
        LegacyBattleGrowthItemCompletionPanelCallReply overflow{
            .eax = 64U,
            .publish_formatted_text = true,
            .formatted_text_length = 64U,
        };
        overflow.formatted_text.fill(0x59U);
        fixture.port.reply(
            LegacyBattleGrowthItemCompletionPanelCall::format_text, overflow
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemCompletionPanelStatus::
                        format_buffer_typed_stop &&
                result.port_calls == 1U &&
                result.formatted_text_length == 64U &&
                std::ranges::all_of(
                    result.formatted_text,
                    [](const u8 value) { return value == 0x59U; }
                ),
            "growth item completion panel keeps the full 64-byte formatting side effect before the following terminator stop"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.successful_loads = 0U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemCompletionPanelStatus::
                        frame_typed_stop &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U && result.query_calls == 0U &&
                result.return_eax == 0xBBBBCCCCU &&
                result.return_ecx == 0xDDDDEEEEU &&
                result.return_edx == 0xFFFF0000U,
            "growth item completion panel preserves the rectangle prefix and frame return registers on tiled-frame stop"
        );
    }
}
