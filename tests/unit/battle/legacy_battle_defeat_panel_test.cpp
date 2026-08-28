#include "openswd3/battle/legacy_battle_defeat_panel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleDefeatPanelCall;
using openswd3::battle::LegacyBattleDefeatPanelCallReply;
using openswd3::battle::LegacyBattleDefeatPanelCallRequest;
using openswd3::battle::LegacyBattleDefeatPanelRequest;
using openswd3::battle::LegacyBattleDefeatPanelStatus;
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

class Port final : public openswd3::battle::LegacyBattleDefeatPanelPort {
public:
    [[nodiscard]] LegacyBattleDefeatPanelCallReply invoke_defeat_panel(
        const LegacyBattleDefeatPanelCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return LegacyBattleDefeatPanelPort::invoke_defeat_panel(request);
        }
        return found->second[index++];
    }

    void reply(
        const LegacyBattleDefeatPanelCall call,
        const LegacyBattleDefeatPanelCallReply& reply_value
    ) {
        replies[call].push_back(reply_value);
    }

    [[nodiscard]] u32 count(const LegacyBattleDefeatPanelCall call) const {
        return static_cast<u32>(std::ranges::count_if(
            calls, [call](const auto& request) { return request.call == call; }
        ));
    }

    std::vector<LegacyBattleDefeatPanelCallRequest> calls;
    std::map<
        LegacyBattleDefeatPanelCall,
        std::vector<LegacyBattleDefeatPanelCallReply>>
        replies;
    std::map<LegacyBattleDefeatPanelCall, std::size_t> indices;
};

struct Fixture {
    Fixture()
        : action_updater(action_streams), raster(framebuffer.geometry()) {}

    [[nodiscard]] openswd3::battle::LegacyBattleDefeatPanelBindings bindings() {
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

[[nodiscard]] openswd3::battle::LegacyBattleDefeatPanelResult
run(Fixture& fixture, const LegacyBattleDefeatPanelRequest& request = {}) {
    return openswd3::battle::draw_legacy_battle_defeat_panel(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleDefeatPanelCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

}  // namespace

void test_battle_defeat_panel(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleDefeatDetailToken;
    using openswd3::battle::kLegacyBattleDefeatFramebufferToken;
    using openswd3::battle::kLegacyBattleDefeatTitleToken;
    using openswd3::battle::kLegacyBattleVictoryFontToken;
    using openswd3::battle::kLegacyBattleVictoryPanelAction;

    {
        Fixture fixture;
        fixture.target.transition_stage = 12U;
        fixture.port.reply(
            LegacyBattleDefeatPanelCall::draw_title,
            {.eax = 0x41414141U, .ecx = 0xABCD1234U, .edx = 0x43434343U}
        );
        fixture.port.reply(
            LegacyBattleDefeatPanelCall::query_panel,
            {.eax = 2U, .ecx = 0x51515151U, .edx = 0x52525252U}
        );
        const LegacyBattleDefeatPanelRequest request{
            .entry_eax = 0x01020304U,
            .entry_ecx = 0x11112222U,
            .entry_edx = 0x33334444U,
            .action_return = {0x10101010U, 0x20202020U, 0x30303030U},
            .rectangle_return = {0x31313131U, 0xA1B20000U, 0x32323232U},
            .title_frame_return = {0x33333333U, 0x34343434U, 0x35353535U},
            .detail_frame_return = {0x61616161U, 0x62626262U, 0x63636363U},
        };
        const auto result = run(fixture, request);
        test.expect_true(
            result.status == LegacyBattleDefeatPanelStatus::completed &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.title_draw_calls == 1U && result.query_calls == 1U &&
                result.font_size_calls == 0U &&
                result.detail_draw_calls == 0U &&
                result.rectangle_height == 52 &&
                result.detail_frame_bottom == 224 &&
                fixture.victory.panel_action_record.action_id ==
                    kLegacyBattleVictoryPanelAction &&
                fixture.victory.panel_action_record.base_variant == 0U &&
                result.return_eax == 2U && result.return_ecx == 0x51515151U &&
                result.return_edx == 0x52525252U,
            "defeat panel draws the fixed title and both frames then returns the non-one query registers"
        );
        test.expect_true(
            result.action_entry.eax == 0x01020304U &&
                result.action_entry.ecx == 0x11112222U &&
                result.action_entry.edx == 0x33334444U &&
                result.rectangle_entry.eax == 52U &&
                result.rectangle_entry.ecx == 0x20202020U &&
                result.rectangle_entry.edx == 0x30303030U &&
                (result.first_frame_resource & 0xFFFF0000U) == 0xA1B20000U &&
                (result.first_frame_resource & 0xFFFFU) ==
                    fixture.victory.panel_action_record.field_4a &&
                result.second_frame_entry.eax == 224U &&
                (result.second_frame_resource & 0xFFFF0000U) == 0xABCD0000U &&
                (result.second_frame_resource & 0xFFFFU) ==
                    fixture.victory.panel_action_record.field_4a,
            "defeat panel preserves its EAX stage chain and both ECX resource high-word variants"
        );
        test.expect_true(
            fixture.port.calls.size() == 2U &&
                fixture.port.calls[0U].object_token ==
                    kLegacyBattleVictoryFontToken &&
                fixture.port.calls[0U].arguments[0U] ==
                    kLegacyBattleDefeatFramebufferToken &&
                fixture.port.calls[0U].arguments[1U] == 0x104U &&
                fixture.port.calls[0U].arguments[2U] == 0xB4U &&
                fixture.port.calls[0U].arguments[3U] ==
                    kLegacyBattleDefeatTitleToken &&
                text_bytes(fixture.port.calls[0U]) ==
                    std::vector<u8>({
                        0xBEU,
                        0xD4U,
                        0xB0U,
                        0xABU,
                        0xA5U,
                        0xA2U,
                        0xB1U,
                        0xD1U,
                    }) &&
                fixture.port.calls[1U].arguments[0U] == 0xD4U &&
                fixture.port.calls[1U].arguments[1U] == 0xF4U &&
                fixture.port.calls[1U].arguments[2U] == 3U,
            "defeat panel publishes the original title bytes, coordinates and fixed query"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_stage = 20U;
        fixture.port.reply(
            LegacyBattleDefeatPanelCall::query_panel,
            {.eax = 1U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );
        fixture.port.reply(
            LegacyBattleDefeatPanelCall::set_font_size,
            {.eax = 0x31313131U, .ecx = 0x32323232U, .edx = 0x33333333U}
        );
        fixture.port.reply(
            LegacyBattleDefeatPanelCall::draw_detail,
            {.eax = 0x41414141U, .ecx = 0x42424242U, .edx = 0x43434343U}
        );
        fixture.port.reply(
            LegacyBattleDefeatPanelCall::set_font_size,
            {.eax = 0x51515151U, .ecx = 0x52525252U, .edx = 0x53535353U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleDefeatPanelStatus::completed &&
                result.font_size_calls == 2U &&
                result.detail_draw_calls == 1U &&
                result.return_eax == 0x51515151U &&
                result.return_ecx == 0x52525252U &&
                result.return_edx == 0x53535353U,
            "defeat panel switches to font seventeen, draws the party defeat detail and restores font sixteen only for an exact-one query"
        );
        test.expect_true(
            fixture.port.calls.size() == 5U &&
                fixture.port.calls[2U].arguments[1U] == 0x11U &&
                fixture.port.calls[2U].eax == 1U &&
                fixture.port.calls[2U].edx == 0x22220000U &&
                fixture.port.calls[3U].arguments[0U] ==
                    kLegacyBattleDefeatFramebufferToken &&
                fixture.port.calls[3U].arguments[1U] == 0xFEU &&
                fixture.port.calls[3U].arguments[2U] == 0xD8U &&
                fixture.port.calls[3U].arguments[3U] ==
                    kLegacyBattleDefeatDetailToken &&
                fixture.port.calls[3U].eax == 0x31313131U &&
                fixture.port.calls[3U].ecx == kLegacyBattleVictoryFontToken &&
                fixture.port.calls[3U].edx ==
                    kLegacyBattleDefeatFramebufferToken &&
                text_bytes(fixture.port.calls[3U]) ==
                    std::vector<u8>({
                        0xB6U,
                        0xA4U,
                        0xA5U,
                        0xEEU,
                        0xA5U,
                        0xFEU,
                        0xB7U,
                        0xC0U,
                        0x21U,
                        0x21U,
                    }) &&
                fixture.port.calls[4U].arguments[1U] == 0x10U,
            "defeat panel preserves the query, font, detail text and restore callsite registers"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.successful_loads = 0U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleDefeatPanelStatus::title_frame_typed_stop &&
                result.title_draw_calls == 0U && result.query_calls == 0U &&
                result.font_size_calls == 0U,
            "defeat panel stops after the first frame failure without title, query or font side effects"
        );

        Fixture second_frame;
        second_frame.target.transition_stage = 12U;
        second_frame.frame_provider.successful_loads = 213U;
        const auto second_frame_result = run(second_frame);
        test.expect_true(
            second_frame_result.status ==
                    LegacyBattleDefeatPanelStatus::detail_frame_typed_stop &&
                second_frame_result.title_draw_calls == 1U &&
                second_frame_result.query_calls == 0U &&
                second_frame_result.font_size_calls == 0U,
            "defeat panel preserves the title when the second frame stops before query and font effects"
        );
    }
}
