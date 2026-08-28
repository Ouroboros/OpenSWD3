#include "openswd3/battle/legacy_battle_victory_item_list_panel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <span>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleVictoryItemListPanelCall;
using openswd3::battle::LegacyBattleVictoryItemListPanelCallReply;
using openswd3::battle::LegacyBattleVictoryItemListPanelCallRequest;
using openswd3::battle::LegacyBattleVictoryItemListPanelRequest;
using openswd3::battle::LegacyBattleVictoryItemListPanelStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant, const bool
    ) override {
        action_ids.push_back(action_id);
        variants.push_back(variant);
        constexpr std::array<u16, 8U> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    std::vector<u32> action_ids;
    std::vector<u32> variants;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        piece_indices.push_back(piece_index);
        if (resource_ids.size() > successful_loads ||
            piece_index >= pixels.size()) {
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
    std::vector<u32> piece_indices;
    std::size_t successful_loads{1000U};
};

class Port final
    : public openswd3::battle::LegacyBattleVictoryItemListPanelPort {
public:
    [[nodiscard]] LegacyBattleVictoryItemListPanelCallReply
    invoke_victory_item_list_panel(
        const LegacyBattleVictoryItemListPanelCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found != replies.end() && index < found->second.size()) {
            return found->second[index++];
        }
        return LegacyBattleVictoryItemListPanelPort::
            invoke_victory_item_list_panel(request);
    }

    void reply(
        const LegacyBattleVictoryItemListPanelCall call,
        const LegacyBattleVictoryItemListPanelCallReply& reply_value
    ) {
        replies[call].push_back(reply_value);
    }

    [[nodiscard]] u32
    count(const LegacyBattleVictoryItemListPanelCall call) const {
        return static_cast<u32>(std::ranges::count_if(
            calls, [call](const auto& request) { return request.call == call; }
        ));
    }

    std::vector<LegacyBattleVictoryItemListPanelCallRequest> calls;
    std::map<
        LegacyBattleVictoryItemListPanelCall,
        std::vector<LegacyBattleVictoryItemListPanelCallReply>>
        replies;
    std::map<LegacyBattleVictoryItemListPanelCall, std::size_t> reply_indices;
};

struct Fixture {
    Fixture()
        : action_updater(action_streams), raster(framebuffer.geometry()) {}

    [[nodiscard]] openswd3::battle::LegacyBattleVictoryItemListPanelBindings
    bindings() {
        return {
            .victory = victory,
            .target_selection = target,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    Port port;
};

[[nodiscard]] openswd3::battle::LegacyBattleVictoryItemListPanelResult
run(Fixture& fixture, const LegacyBattleVictoryItemListPanelRequest& request) {
    return openswd3::battle::draw_legacy_battle_victory_item_list_panel(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] LegacyBattleVictoryItemListPanelCallReply formatted_reply(
    const std::span<const u8> text, const u32 eax, const u32 ecx, const u32 edx
) {
    LegacyBattleVictoryItemListPanelCallReply reply{
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
        .publish_formatted_text = true,
        .formatted_text_length = static_cast<u32>(text.size()),
    };
    std::copy(text.begin(), text.end(), reply.formatted_text.begin());
    return reply;
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleVictoryItemListPanelCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

}  // namespace

void test_battle_victory_item_list_panel(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleVictoryFontToken;
    using openswd3::battle::kLegacyBattleVictoryItemListFormatToken;
    using openswd3::battle::kLegacyBattleVictoryItemListFramebufferToken;
    using openswd3::battle::kLegacyBattleVictoryItemListTitleToken;
    using openswd3::battle::kLegacyBattleVictoryPanelAction;

    {
        Fixture fixture;
        fixture.target.transition_sample_word = 0U;
        fixture.target.transition_stage = 0U;
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::set_font_size,
            {.eax = 0x11111111U, .ecx = 0x22222222U, .edx = 0x33333333U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::draw_title,
            {.eax = 0xABCD1234U, .ecx = 0x45454545U, .edx = 0x56565656U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U, .ecx = 0x67676767U, .edx = 0x78787878U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::set_font_size,
            {.eax = 0x81818181U, .ecx = 0x82828282U, .edx = 0x83838383U}
        );
        const LegacyBattleVictoryItemListPanelRequest request{
            .entry_eax = 0x01020304U,
            .entry_ecx = 0x11112222U,
            .entry_edx = 0x33334444U,
            .local_text_seed = 0xA5U,
            .local_text_token = 0x70004000U,
            .action_return = {0x10101010U, 0x20202020U, 0x30303030U},
            .rectangle_return = {0xA1B20000U, 0x41414141U, 0x42424242U},
            .title_frame_return = {0x51515151U, 0x52525252U, 0x53535353U},
            .list_frame_return = {0x61616161U, 0x62626262U, 0x63636363U},
        };
        const auto result = run(fixture, request);
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryItemListPanelStatus::completed &&
                result.initial_item_count == 0U &&
                result.panel_bottom == 0xD4U && result.rectangle_height == 40 &&
                result.list_frame_bottom == 212 &&
                result.font_size_calls == 2U && result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.title_draw_calls == 1U && result.query_calls == 0U &&
                result.transition_stage_calls == 1U &&
                result.format_calls == 0U && result.item_draw_calls == 0U &&
                result.local_text[0U] == 0xA5U &&
                fixture.victory.panel_action_record.action_id ==
                    kLegacyBattleVictoryPanelAction &&
                fixture.victory.panel_action_record.base_variant == 0U &&
                result.return_eax == 0x81818181U &&
                result.return_ecx == 0x82828282U &&
                result.return_edx == 0x83838383U,
            "victory item list draws both fixed panels, preserves the seeded local buffer and restores font size when no row is visible"
        );
        test.expect_true(
            result.action_entry.eax == 0x11111111U &&
                result.action_entry.ecx == 0x22222222U &&
                result.action_entry.edx == 0x33333333U &&
                result.rectangle_entry.eax == 0x10101010U &&
                result.rectangle_entry.ecx == 0x20202020U &&
                result.rectangle_entry.edx == 40U &&
                (result.first_frame_resource & 0xFFFF0000U) == 0xA1B20000U &&
                (result.first_frame_resource & 0xFFFFU) ==
                    fixture.victory.panel_action_record.field_4a &&
                (result.second_frame_resource & 0xFFFF0000U) == 0xABCD0000U &&
                (result.second_frame_resource & 0xFFFFU) ==
                    fixture.victory.panel_action_record.field_4a,
            "victory item list preserves the font, action, rectangle and two resource register chains"
        );
        test.expect_true(
            fixture.port.calls.size() == 3U &&
                fixture.port.calls[0U].arguments[1U] == 0x12U &&
                fixture.port.calls[0U].eax == 0U &&
                fixture.port.calls[0U].ecx == kLegacyBattleVictoryFontToken &&
                fixture.port.calls[0U].edx == 0x33334444U &&
                fixture.port.calls[1U].arguments[0U] ==
                    kLegacyBattleVictoryItemListFramebufferToken &&
                fixture.port.calls[1U].arguments[1U] == 0x108U &&
                fixture.port.calls[1U].arguments[2U] == 0xB4U &&
                fixture.port.calls[1U].arguments[3U] ==
                    kLegacyBattleVictoryItemListTitleToken &&
                text_bytes(fixture.port.calls[1U]) ==
                    std::vector<u8>(
                        {0xBEU, 0xD4U, 0xA7U, 0x51U, 0xABU, 0x7EU}
                    ) &&
                fixture.port.calls[2U].arguments[1U] == 0x10U &&
                result.transition_stage.return_eax == 1U &&
                fixture.port.count(
                    LegacyBattleVictoryItemListPanelCall::
                        reserved_transition_stage_advance_slot
                ) == 0U,
            "victory item list publishes the original title bytes, fixed coordinates, typed stage and font sizes"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_sample_word = 2U;
        fixture.target.transition_stage = 40U;
        fixture.victory.player_item_tokens[0U] = 0x71000000U;
        fixture.victory.player_item_tokens[1U] = 0x72000000U;
        fixture.victory.collected_item_quantities[0U] = 7U;
        fixture.victory.collected_item_quantities[1U] = 0xFFFFU;
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::set_font_size,
            {.eax = 2U, .ecx = 0x10101010U, .edx = 0x20202020U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );
        constexpr std::array<u8, 8U> kFirst{
            'I', 'T', 'E', 'M', ' ', 'X', ' ', '7'
        };
        constexpr std::array<u8, 9U> kSecond{
            'I', 'T', 'E', 'M', ' ', 'X', ' ', '9', '9'
        };
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::format_item_row,
            formatted_reply(kFirst, 8U, 0x31313131U, 0x32323232U)
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::draw_item_row,
            {.eax = 0x41414141U, .ecx = 0x42424242U, .edx = 0x43434343U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::format_item_row,
            formatted_reply(kSecond, 9U, 0x51515151U, 0x52525252U)
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::draw_item_row,
            {.eax = 0x61616161U, .ecx = 0x62626262U, .edx = 0x63636363U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::set_font_size,
            {.eax = 0x71717171U, .ecx = 0x72727272U, .edx = 0x73737373U}
        );
        const LegacyBattleVictoryItemListPanelRequest request{
            .entry_edx = 0x90909090U,
            .local_text_token = 0x70005000U,
        };
        const auto result = run(fixture, request);
        const auto formats =
            std::vector<LegacyBattleVictoryItemListPanelCallRequest>{
                fixture.port.calls[2U], fixture.port.calls[4U]
            };
        const auto draws =
            std::vector<LegacyBattleVictoryItemListPanelCallRequest>{
                fixture.port.calls[3U], fixture.port.calls[5U]
            };
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryItemListPanelStatus::completed &&
                result.panel_bottom == 0xFCU && result.item_rows_drawn == 2U &&
                result.format_calls == 2U && result.item_draw_calls == 2U &&
                result.local_text_length == kSecond.size() &&
                result.return_eax == 0x71717171U &&
                result.return_ecx == 0x72727272U &&
                result.return_edx == 0x73737373U,
            "victory item list formats and draws every live reward row before restoring the font"
        );
        test.expect_true(
            formats[0U].arguments[0U] == 0x70005000U &&
                formats[0U].arguments[1U] ==
                    kLegacyBattleVictoryItemListFormatToken &&
                formats[0U].arguments[2U] == 0x71000000U &&
                formats[0U].arguments[3U] == 7U &&
                formats[0U].eax == 0x70005000U && formats[0U].ecx == 7U &&
                formats[0U].edx == 0x71000000U &&
                formats[1U].item_quantity == 0xFFFFU &&
                draws[0U].arguments[1U] == 0xD2U &&
                draws[0U].arguments[2U] == 0xD4U &&
                draws[1U].arguments[2U] == 0xE8U &&
                text_bytes(draws[0U]) ==
                    std::vector<u8>(kFirst.begin(), kFirst.end()) &&
                text_bytes(draws[1U]) ==
                    std::vector<u8>(kSecond.begin(), kSecond.end()),
            "victory item list preserves the row format arguments, zero-extended quantity, register layout and twenty-pixel spacing"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_sample_word = 3U;
        fixture.target.transition_stage = 60U;
        fixture.victory.player_item_tokens = {1U, 2U, 3U};
        fixture.victory.collected_item_quantities = {1U, 1U, 1U};
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::draw_item_row,
            {
                .publish_item_count = true,
                .item_count = 1U,
            }
        );
        const LegacyBattleVictoryItemListPanelRequest request{
            .local_text_token = 0x70006000U,
        };
        const auto result = run(fixture, request);
        test.expect_true(
            result.item_rows_drawn == 1U &&
                fixture.target.transition_sample_word == 1U &&
                result.panel_bottom == 0x110U,
            "victory item list keeps entry-count geometry but reloads the live count after each row"
        );
    }

    {
        Fixture non_one;
        non_one.target.transition_sample_word = 1U;
        non_one.port.reply(
            LegacyBattleVictoryItemListPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 2U}
        );
        const auto non_one_result = run(non_one, {});
        test.expect_true(
            non_one_result.item_rows_drawn == 0U &&
                non_one_result.font_size_calls == 2U,
            "victory item list requires an exact query result of one before drawing rows"
        );

        Fixture overflow;
        overflow.target.transition_sample_word = 1U;
        overflow.target.transition_stage = 20U;
        overflow.victory.player_item_tokens[0U] = 0x71000000U;
        overflow.victory.collected_item_quantities[0U] = 1U;
        overflow.port.reply(
            LegacyBattleVictoryItemListPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        LegacyBattleVictoryItemListPanelCallReply overflow_reply{
            .publish_formatted_text = true,
            .formatted_text_length = 64U,
        };
        overflow_reply.formatted_text.fill(0x58U);
        overflow.port.reply(
            LegacyBattleVictoryItemListPanelCall::format_item_row,
            overflow_reply
        );
        const auto overflow_result =
            run(overflow,
                {.local_text_seed = 0xA5U, .local_text_token = 0x70007000U});
        test.expect_true(
            overflow_result.status ==
                    LegacyBattleVictoryItemListPanelStatus::
                        format_buffer_typed_stop &&
                overflow_result.stopped_text_index == 64U &&
                overflow_result.item_draw_calls == 0U &&
                overflow_result.font_size_calls == 1U &&
                std::ranges::all_of(
                    overflow_result.local_text,
                    [](const u8 value) { return value == 0x58U; }
                ),
            "victory item list preserves the full sixty-four-byte format prefix then stops before row drawing and font restoration"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_sample_word = 11U;
        fixture.target.transition_stage = 220U;
        fixture.victory.player_item_tokens.fill(0x71000000U);
        fixture.victory.collected_item_quantities.fill(1U);
        fixture.port.reply(
            LegacyBattleVictoryItemListPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        const auto result = run(fixture, {.local_text_token = 0x70008000U});
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryItemListPanelStatus::
                        item_row_typed_stop &&
                result.stopped_item_index == 10U &&
                result.item_rows_drawn == 10U && result.font_size_calls == 1U,
            "victory item list stops on the first real reward table access beyond ten without adding a modern row cap"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_sample_word = 1U;
        fixture.frame_provider.successful_loads = 0U;
        const auto result = run(fixture, {});
        test.expect_true(
            result.status ==
                    LegacyBattleVictoryItemListPanelStatus::
                        title_frame_typed_stop &&
                result.title_draw_calls == 0U && result.query_calls == 0U &&
                result.font_size_calls == 1U,
            "victory item list stops after the first frame failure without drawing text or restoring the font"
        );
    }
}
