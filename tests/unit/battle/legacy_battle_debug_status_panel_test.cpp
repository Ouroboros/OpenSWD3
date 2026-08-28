#include "openswd3/battle/legacy_battle_debug_status_panel.hpp"

#include <array>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 pixel = static_cast<u16>(0x4100U + index);
            storage[index] = {
                static_cast<u8>(pixel), static_cast<u8>(pixel >> 8U)
            };
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (piece_index >= storage.size()) {
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::vector<u8>, 9> storage;
    std::vector<u32> resource_ids;
};

class TextPort final : public openswd3::battle::LegacyBattleTextPanelPort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleTextPanelCallReply
    invoke_text_panel(
        const openswd3::battle::LegacyBattleTextPanelCallRequest& request
    ) override {
        calls.push_back(request);
        if (calls.size() == 1U) {
            return {.eax = 0xAAAA0001U, .ecx = 0x12340000U, .edx = 0xBBBB0003U};
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::vector<openswd3::battle::LegacyBattleTextPanelCallRequest> calls;
};

struct Fixture {
    Fixture() : raster(framebuffer.geometry()), action_updater(action_streams) {
        victory.panel_action_record.external_mode = 1U;
        victory.panel_action_record.command_cursor = 1U;
        victory.panel_action_record.field_4a = 0x66U;
        target.debug_status_profile_token = 0x71000000U;
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleDebugStatusPanelBindings bindings() {
        return {
            .debug_hotkeys = debug,
            .target_selection = target,
            .victory_rewards = victory,
            .selection_frame = selection,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = shared_request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .pixel_conversion = pixel_conversion,
        };
    }

    openswd3::battle::LegacyBattleDebugHotkeyState debug;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::battle::LegacyBattleSelectionFrameState selection;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 600,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 600};
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    openswd3::rendering::LegacyPixelConversionState pixel_conversion;
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    FrameProvider frame_provider;
    TextPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleDebugStatusPanelRequest request() {
    return {
        .entry = {.eax = 0x11111111U, .ecx = 0x22222222U, .edx = 0x33333333U},
        .local_text_token = 0x72000000U,
        .local_text_seed = 0x41U,
        .rectangle_return = {.eax = 0xABCD0001U, .ecx = 2U, .edx = 3U},
        .panel_frame_return = {.eax = 0xDEAD0004U, .ecx = 5U, .edx = 6U},
        .color_fade_return = {
            .eax = 0xCCCC0001U, .ecx = 0xCCCC0002U, .edx = 0xCCCC0003U
        },
    };
}

}  // namespace

void test_battle_debug_status_panel(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleDebugStatusPanelStatus;
    using openswd3::battle::draw_legacy_battle_debug_status_panel;

    {
        Fixture fixture;
        fixture.target.debug_status_profile_token = 0U;
        const auto result = draw_legacy_battle_debug_status_panel(
            fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status == LegacyBattleDebugStatusPanelStatus::completed &&
                !result.debug_gate_open && result.call_trace.empty() &&
                result.local_text ==
                    std::array<u8, 8>{0x41U, 0U, 0U, 0U, 0U, 0U, 0U, 0U} &&
                result.return_registers.eax == 0x11111111U &&
                result.return_registers.ecx == 0x22222222U &&
                result.return_registers.edx == 0x33333333U,
            "closed debug bit returns after only the local seed initialization"
        );
    }

    {
        Fixture fixture;
        fixture.debug.battle_mode_flags_53bc24 = 0x20U;
        fixture.target.debug_status_profile_token = 0U;
        const auto result = draw_legacy_battle_debug_status_panel(
            fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugStatusPanelStatus::profile_typed_stop &&
                result.debug_gate_open && result.call_trace.size() == 3U &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.panel_frame_calls == 1U && result.text_calls == 0U &&
                result.transition_stage_advance_calls == 0U,
            "missing profile stops at the title access after preserving the panel prefix"
        );
    }

    {
        Fixture fixture;
        fixture.debug.battle_mode_flags_53bc24 = 0x20U;
        fixture.target.transition_stage = 0U;
        const auto result = draw_legacy_battle_debug_status_panel(
            fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status == LegacyBattleDebugStatusPanelStatus::completed &&
                !result.stage_gate_open && result.call_trace.size() == 6U &&
                result.panel_frame_calls == 2U && result.text_calls == 1U &&
                result.transition_stage_advance_calls == 1U &&
                result.transition_stage_advance.numerator == 180 &&
                result.transition_stage_advance.quotient == 60 &&
                fixture.target.transition_stage == 60U &&
                result.return_registers.eax == 0U &&
                result.return_registers.ecx == 0U &&
                result.return_registers.edx == 0U,
            "nonzero stage quotient draws the shell then skips all nine rows"
        );
    }

    {
        Fixture fixture;
        fixture.debug.battle_mode_flags_53bc24 = 0x20U;
        fixture.target.transition_stage = 180U;
        fixture.target.debug_status_values = {
            0,
            5,
            -3,
            -11,
            10,
            -10,
            1,
            -128,
            127,
        };
        const auto result = draw_legacy_battle_debug_status_panel(
            fixture.bindings(), fixture.port, request()
        );
        const auto gray = openswd3::rendering::legacy_pack_color_pair(
            fixture.pixel_conversion, 16, 16, 16
        );
        const auto positive = openswd3::rendering::legacy_pack_color_pair(
            fixture.pixel_conversion, 28, 2, 2
        );
        const auto negative = openswd3::rendering::legacy_pack_color_pair(
            fixture.pixel_conversion, 2, 13, 28
        );
        const auto deep_negative = openswd3::rendering::legacy_pack_color_pair(
            fixture.pixel_conversion, 2, 28, 13
        );
        test.expect_true(
            result.status == LegacyBattleDebugStatusPanelStatus::completed &&
                result.stage_gate_open &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.panel_rectangle_height == 220 &&
                result.panel_frame_calls == 2U &&
                result.panel_frame_resources ==
                    std::array<u32, 2>{0xABCD0066U, 0x12340066U} &&
                result.panel_frame_bottoms ==
                    std::array<openswd3::compat::i32, 2>{106, 302} &&
                result.transition_stage_advance_calls == 1U &&
                result.transition_stage_advance.quotient == 0 &&
                fixture.target.transition_stage == 180U &&
                result.text_calls == 10U && result.color_fade_calls == 9U &&
                result.port_calls == 10U && result.call_trace.size() == 64U &&
                result.return_registers.eax == 0U &&
                result.return_registers.ecx == 0xCCCC0002U &&
                result.return_registers.edx ==
                    openswd3::battle::
                            kLegacyBattleDebugStatusPanelLabelTableToken +
                        36U,
            "full panel preserves shell title stage gate loop and tail registers"
        );
        test.expect_true(
            result.rows[0].fade_width == 2U &&
                result.rows[1].fade_width == 60U &&
                result.rows[2].fade_width == 36U &&
                result.rows[3].fade_width == 12U &&
                result.rows[4].fade_width == 120U &&
                result.rows[5].fade_width == 120U &&
                result.rows[6].fade_width == 12U &&
                result.rows[7].fade_width == 1416U &&
                result.rows[8].fade_width == 1524U &&
                result.rows[0].packed_color == gray &&
                result.rows[1].packed_color == positive &&
                result.rows[2].packed_color == negative &&
                result.rows[3].packed_color == deep_negative &&
                result.rows[5].packed_color == negative,
            "signed row branches preserve x87 widths and final colors"
        );
        test.expect_true(
            result.rows[0].formatted_text[0] == 0x2DU &&
                result.rows[0].formatted_text[1] == 0x2DU &&
                result.rows[1].formatted_text[0] == 0x20U &&
                result.rows[1].formatted_text[1] == 0x35U &&
                result.rows[1].formatted_text[2] == 0x30U &&
                result.rows[1].formatted_text[3] == 0x25U &&
                result.rows[3].formatted_text[0] == 0x31U &&
                result.rows[3].formatted_text[1] == 0x31U &&
                result.rows[3].formatted_text[2] == 0x30U &&
                result.rows[3].formatted_text[3] == 0x25U &&
                result.rows[7].formatted_text[0] == 0x31U &&
                result.rows[7].formatted_text[1] == 0x32U &&
                result.rows[7].formatted_text[2] == 0x38U,
            "dead local formatting preserves lstrcpy and wsprintf bytes"
        );
        bool calls_match = fixture.port.calls.size() == 10U &&
            fixture.port.calls[0].arguments[2] == 304U &&
            fixture.port.calls[0].arguments[3] == 90U &&
            fixture.port.calls[0].arguments[4] == 0x71000000U;
        for (std::size_t index = 0U; calls_match && index < result.rows.size();
             ++index) {
            const auto& call = fixture.port.calls[index + 1U];
            calls_match = call.call ==
                    openswd3::battle::LegacyBattleTextPanelCall::draw_text &&
                call.arguments[2] == 252U &&
                call.arguments[3] == static_cast<u32>(122U + index * 20U) &&
                call.arguments[4] ==
                    openswd3::battle::kLegacyBattleDebugStatusPanelLabelTokens
                        [index] &&
                call.eax ==
                    openswd3::battle::kLegacyBattleDebugStatusPanelLabelTokens
                        [index] &&
                call.ecx ==
                    openswd3::battle::kLegacyBattleDebugStatusPanelFontToken &&
                call.edx ==
                    openswd3::battle::kLegacyBattleDebugStatusPanelSurfaceToken;
        }
        test.expect_true(
            calls_match,
            "profile title and nine labels preserve tokens coordinates and entry registers"
        );
    }
}
