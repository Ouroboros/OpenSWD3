#include "openswd3/battle/legacy_battle_talisman_result_panel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleTalismanResultPanelCall;
using openswd3::battle::LegacyBattleTalismanResultPanelCallReply;
using openswd3::battle::LegacyBattleTalismanResultPanelCallRequest;
using openswd3::battle::LegacyBattleTalismanResultPanelRequest;
using openswd3::battle::LegacyBattleTalismanResultPanelStatus;
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
    std::size_t successful_loads{1000U};
};

class Port final
    : public openswd3::battle::LegacyBattleTalismanResultPanelPort {
public:
    [[nodiscard]] LegacyBattleTalismanResultPanelCallReply
    invoke_talisman_result_panel(
        const LegacyBattleTalismanResultPanelCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return LegacyBattleTalismanResultPanelPort::
                invoke_talisman_result_panel(request);
        }
        return found->second[index++];
    }

    void reply(
        const LegacyBattleTalismanResultPanelCall call,
        const LegacyBattleTalismanResultPanelCallReply& reply_value
    ) {
        replies[call].push_back(reply_value);
    }

    [[nodiscard]] u32
    count(const LegacyBattleTalismanResultPanelCall call) const {
        return static_cast<u32>(std::ranges::count_if(
            calls, [call](const auto& request) { return request.call == call; }
        ));
    }

    std::vector<LegacyBattleTalismanResultPanelCallRequest> calls;
    std::map<
        LegacyBattleTalismanResultPanelCall,
        std::vector<LegacyBattleTalismanResultPanelCallReply>>
        replies;
    std::map<LegacyBattleTalismanResultPanelCall, std::size_t> indices;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        target.transition_stage = 40U;
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleTalismanResultPanelBindings bindings() {
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

[[nodiscard]] openswd3::battle::LegacyBattleTalismanResultPanelResult
run(Fixture& fixture,
    const LegacyBattleTalismanResultPanelRequest& request = {}) {
    return openswd3::battle::draw_legacy_battle_talisman_result_panel(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleTalismanResultPanelCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

}  // namespace

void test_battle_talisman_result_panel(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleTalismanFailureDetailToken;
    using openswd3::battle::kLegacyBattleTalismanFailureTitleToken;
    using openswd3::battle::kLegacyBattleTalismanFramebufferToken;
    using openswd3::battle::kLegacyBattleTalismanSuccessFormatToken;
    using openswd3::battle::kLegacyBattleTalismanSuccessTitleToken;
    using openswd3::battle::kLegacyBattleVictoryFontToken;
    using openswd3::battle::kLegacyBattleVictoryPanelAction;

    {
        Fixture fixture;
        fixture.target.transition_stage = 12U;
        fixture.port.reply(
            LegacyBattleTalismanResultPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 2U, .ecx = 0x51515151U, .edx = 0x52525252U}
        );
        const LegacyBattleTalismanResultPanelRequest request{
            .entry_eax = 0x01020304U,
            .entry_ecx = 0x11112222U,
            .entry_edx = 0x33334444U,
            .action_return = {0x10101010U, 0x20202020U, 0x30303030U},
            .rectangle_return = {0x31313131U, 0x32323232U, 0xA1B20000U},
            .title_frame_return = {0x33333333U, 0xC3D40000U, 0x35353535U},
            .detail_frame_return = {0x61616161U, 0x62626262U, 0x63636363U},
        };
        const auto result = run(fixture, request);
        test.expect_true(
            result.status == LegacyBattleTalismanResultPanelStatus::completed &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.query_calls == 0U &&
                result.transition_stage_calls == 1U &&
                result.title_draw_calls == 0U && result.format_calls == 0U &&
                result.detail_draw_calls == 0U &&
                result.rectangle_height == 52 &&
                result.detail_frame_bottom == 224 &&
                fixture.victory.panel_action_record.action_id ==
                    kLegacyBattleVictoryPanelAction &&
                fixture.victory.panel_action_record.base_variant == 0U &&
                fixture.target.transition_stage == 21U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 1U,
            "talisman result panel draws both frames and returns the nonzero-quotient stage registers without result text"
        );
        test.expect_true(
            result.action_entry.eax == 0U && result.action_entry.ecx == 0U &&
                result.action_entry.edx == 0x33334444U &&
                result.rectangle_entry.ecx == 52U &&
                result.rectangle_entry.eax == 0x10101010U &&
                (result.first_frame_resource & 0xFFFF0000U) == 0xA1B20000U &&
                (result.first_frame_resource & 0xFFFFU) ==
                    fixture.victory.panel_action_record.field_4a &&
                result.second_frame_entry.eax == 224U &&
                (result.second_frame_resource & 0xFFFF0000U) == 0xC3D40000U &&
                (result.second_frame_resource & 0xFFFFU) ==
                    fixture.victory.panel_action_record.field_4a &&
                result.transition_stage.numerator == 28 &&
                result.transition_stage.quotient == 9 &&
                result.transition_stage.remainder == 1 &&
                fixture.port.count(
                    LegacyBattleTalismanResultPanelCall::
                        reserved_transition_stage_advance_slot
                ) == 0U,
            "talisman result panel preserves the ECX height chain, EDX then ECX resource high words and typed stage arithmetic"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_aux_byte = 1U;
        fixture.victory.player_item_tokens[0U] = 0x89ABCDEFU;
        fixture.port.reply(
            LegacyBattleTalismanResultPanelCall::
                reserved_transition_stage_advance_slot,
            {
                .eax = 1U,
                .ecx = 0x11110000U,
                .edx = 0x22220000U,
                .publish_result_mode = true,
                .result_mode = 1U,
            }
        );
        LegacyBattleTalismanResultPanelCallReply formatted{
            .eax = 0x41414141U,
            .ecx = 0x42424242U,
            .edx = 0x43434343U,
            .publish_formatted_text = true,
            .formatted_text_length = 6U,
        };
        formatted.formatted_text[0U] = 'R';
        formatted.formatted_text[1U] = 'E';
        formatted.formatted_text[2U] = 'S';
        formatted.formatted_text[3U] = 'U';
        formatted.formatted_text[4U] = 'L';
        formatted.formatted_text[5U] = 'T';
        fixture.port.reply(
            LegacyBattleTalismanResultPanelCall::format_success_detail,
            formatted
        );
        fixture.port.reply(
            LegacyBattleTalismanResultPanelCall::draw_success_detail,
            {.eax = 0x51515151U, .ecx = 0x52525252U, .edx = 0x53535353U}
        );
        const auto result =
            run(fixture,
                {.local_text_seed = 0x7FU, .local_text_token = 0x70001000U});
        test.expect_true(
            result.status == LegacyBattleTalismanResultPanelStatus::completed &&
                fixture.target.transition_aux_byte == 1U &&
                result.title_draw_calls == 1U && result.format_calls == 1U &&
                result.detail_draw_calls == 1U &&
                result.local_text_length == 6U &&
                result.local_text[0U] == 'R' && result.local_text[5U] == 'T' &&
                result.local_text[6U] == 0U &&
                result.return_eax == 0x51515151U &&
                result.return_ecx == 0x52525252U &&
                result.return_edx == 0x53535353U,
            "talisman result panel rereads the live result byte after query and draws the formatted success result"
        );
        test.expect_true(
            fixture.port.calls.size() == 3U &&
                fixture.port.calls[0U].arguments[0U] ==
                    kLegacyBattleTalismanFramebufferToken &&
                fixture.port.calls[0U].arguments[1U] == 0x100U &&
                fixture.port.calls[0U].arguments[2U] == 0xB4U &&
                fixture.port.calls[0U].arguments[3U] ==
                    kLegacyBattleTalismanSuccessTitleToken &&
                fixture.port.calls[0U].eax == 1U &&
                text_bytes(fixture.port.calls[0U]) ==
                    std::vector<u8>({
                        0xB7U,
                        0xD2U,
                        0xB2U,
                        0xC5U,
                        0xA6U,
                        0xA8U,
                        0xA5U,
                        0x5CU,
                    }) &&
                fixture.port.calls[1U].arguments[0U] == 0x70001000U &&
                fixture.port.calls[1U].arguments[1U] ==
                    kLegacyBattleTalismanSuccessFormatToken &&
                fixture.port.calls[1U].arguments[2U] == 0x89ABCDEFU &&
                text_bytes(fixture.port.calls[1U]) ==
                    std::vector<u8>({
                        0xB1U,
                        0x6FU,
                        0xA8U,
                        0xECU,
                        0xB2U,
                        0xC5U,
                        0xA9U,
                        0x47U,
                        0x3AU,
                        0x25U,
                        0x73U,
                    }) &&
                fixture.port.calls[1U].item_name_token == 0x89ABCDEFU &&
                fixture.port.calls[1U].eax == 0x89ABCDEFU &&
                fixture.port.calls[1U].ecx == 0x70001000U &&
                fixture.port.calls[1U].edx ==
                    kLegacyBattleTalismanFramebufferToken &&
                fixture.port.calls[2U].eax ==
                    kLegacyBattleTalismanFramebufferToken &&
                fixture.port.calls[2U].ecx == kLegacyBattleVictoryFontToken &&
                fixture.port.calls[2U].edx == 0x70001000U &&
                fixture.port.calls[2U].arguments[1U] == 0xD0U &&
                fixture.port.calls[2U].arguments[2U] == 0xDEU &&
                fixture.port.calls[2U].arguments[3U] == 0x70001000U &&
                fixture.port.calls[2U].object_token ==
                    kLegacyBattleVictoryFontToken,
            "talisman success path publishes the original title, format, item token and detail coordinates"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_aux_byte = 2U;
        fixture.port.reply(
            LegacyBattleTalismanResultPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleTalismanResultPanelStatus::completed &&
                result.title_draw_calls == 1U && result.format_calls == 0U &&
                result.detail_draw_calls == 1U &&
                fixture.port.count(
                    LegacyBattleTalismanResultPanelCall::draw_failure_title
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleTalismanResultPanelCall::draw_failure_detail
                ) == 1U,
            "talisman result panel uses the failure pair for every live result byte other than exact one"
        );
        test.expect_true(
            fixture.port.calls[0U].arguments[3U] ==
                    kLegacyBattleTalismanFailureTitleToken &&
                fixture.port.calls[0U].arguments[1U] == 0x100U &&
                fixture.port.calls[0U].eax == 2U &&
                text_bytes(fixture.port.calls[0U]) ==
                    std::vector<u8>({
                        0xB7U,
                        0xD2U,
                        0xB2U,
                        0xC5U,
                        0xA5U,
                        0xA2U,
                        0xB1U,
                        0xD1U,
                    }) &&
                fixture.port.calls[1U].arguments[3U] ==
                    kLegacyBattleTalismanFailureDetailToken &&
                fixture.port.calls[1U].arguments[1U] == 0xD0U &&
                text_bytes(fixture.port.calls[1U]) ==
                    std::vector<u8>({
                        0xA8U,
                        0x53U,
                        0xA6U,
                        0xB3U,
                        0xB1U,
                        0x6FU,
                        0xA8U,
                        0xECU,
                        0xAAU,
                        0x46U,
                        0xA6U,
                        0xE8U,
                    }),
            "talisman failure path publishes the original fixed title and no-item detail bytes"
        );
    }

    {
        Fixture first_frame;
        first_frame.frame_provider.successful_loads = 0U;
        const auto first_result = run(first_frame);
        test.expect_true(
            first_result.status ==
                    LegacyBattleTalismanResultPanelStatus::
                        title_frame_typed_stop &&
                first_result.query_calls == 0U,
            "talisman result panel stops at the first failed frame before query"
        );

        Fixture second_frame;
        second_frame.frame_provider.successful_loads = 213U;
        const auto second_result = run(second_frame);
        test.expect_true(
            second_result.status ==
                    LegacyBattleTalismanResultPanelStatus::
                        detail_frame_typed_stop &&
                second_result.query_calls == 0U,
            "talisman result panel stops at the second failed frame before query"
        );

        Fixture overflow;
        overflow.target.transition_aux_byte = 1U;
        overflow.port.reply(
            LegacyBattleTalismanResultPanelCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        LegacyBattleTalismanResultPanelCallReply formatted{
            .publish_formatted_text = true,
            .formatted_text_length = 64U,
        };
        formatted.formatted_text.fill(0x5AU);
        overflow.port.reply(
            LegacyBattleTalismanResultPanelCall::format_success_detail,
            formatted
        );
        const auto overflow_result = run(overflow);
        test.expect_true(
            overflow_result.status ==
                    LegacyBattleTalismanResultPanelStatus::
                        format_buffer_typed_stop &&
                overflow_result.title_draw_calls == 1U &&
                overflow_result.format_calls == 1U &&
                overflow_result.detail_draw_calls == 0U &&
                overflow_result.stopped_text_index == 64U &&
                overflow_result.local_text[63U] == 0x5AU,
            "talisman result panel preserves the success title and full prefix then stops on the first byte after its 64-byte format buffer"
        );
    }
}
