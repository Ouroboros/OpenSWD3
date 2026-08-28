#include "openswd3/battle/legacy_battle_defeat_panel.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <span>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr std::array<u8, 8U> kDefeatTitle{
    0xBEU, 0xD4U, 0xB0U, 0xABU, 0xA5U, 0xA2U, 0xB1U, 0xD1U
};
constexpr std::array<u8, 10U> kDefeatDetail{
    0xB6U, 0xA4U, 0xA5U, 0xEEU, 0xA5U, 0xFEU, 0xB7U, 0xC0U, 0x21U, 0x21U
};

[[nodiscard]] constexpr u32
replace_low_word(const u32 destination, const u16 value) noexcept {
    return (destination & 0xFFFF0000U) | static_cast<u32>(value);
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

class DefeatPanelRunner final {
public:
    DefeatPanelRunner(
        LegacyBattleDefeatPanelBindings bindings,
        LegacyBattleDefeatPanelPort& port,
        const LegacyBattleDefeatPanelRequest& request
    )
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleDefeatPanelResult run() {
        draw_panels();
        if (result_.status != LegacyBattleDefeatPanelStatus::completed) {
            return finish();
        }
        query_and_draw_detail();
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleDefeatPanelCallReply invoke(
        const LegacyBattleDefeatPanelCall call,
        const u32 object_token = 0U,
        const std::array<u32, 8U> arguments = {},
        const std::span<const u8> text = {}
    ) {
        LegacyBattleDefeatPanelCallRequest call_request{
            .call = call,
            .object_token = object_token,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text_length = static_cast<u32>(text.size()),
        };
        std::copy_n(
            text.begin(),
            std::min<std::size_t>(text.size(), call_request.text.size()),
            call_request.text.begin()
        );
        const auto reply = port_.invoke_defeat_panel(call_request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_stage) {
            bindings_.target_selection.transition_stage = reply.stage;
        }
        return reply;
    }

    void draw_panels() {
        bindings_.victory.panel_action_record.action_id =
            kLegacyBattleVictoryPanelAction;
        bindings_.victory.panel_action_record.base_variant = 0U;
        result_.action_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.panel_action_update = bindings_.action_updater.update(
            bindings_.victory.panel_action_record
        );
        eax_ = request_.action_return.eax;
        ecx_ = request_.action_return.ecx;
        edx_ = request_.action_return.edx;

        eax_ = bindings_.target_selection.transition_stage + 0x28U;
        result_.rectangle_height = std::bit_cast<i32>(eax_);
        result_.rectangle_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.shared_effects.pixel_conversion,
            {
                .x = 0xC4,
                .y = 0xB0,
                .width = 0xB8,
                .height = std::bit_cast<i32>(eax_),
                .red = 0,
                .green = 4,
                .blue = 4,
                .mode = 0U,
            }
        );
        ++result_.rectangle_calls;
        eax_ = request_.rectangle_return.eax;
        ecx_ = request_.rectangle_return.ecx;
        edx_ = request_.rectangle_return.edx;
        if (!rectangle_completed(result_.rectangle_status)) {
            result_.status =
                LegacyBattleDefeatPanelStatus::rectangle_typed_stop;
            return;
        }

        ecx_ = replace_low_word(
            ecx_, bindings_.victory.panel_action_record.field_4a
        );
        result_.first_frame_resource = ecx_;
        result_.first_frame_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.tiled_frames[0U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = ecx_,
                .left = 0xC8,
                .top = 0xB4,
                .right = 0x178,
                .bottom = 0xC4,
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.title_frame_return.eax;
        ecx_ = request_.title_frame_return.ecx;
        edx_ = request_.title_frame_return.edx;
        if (result_.tiled_frames[0U].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleDefeatPanelStatus::title_frame_typed_stop;
            return;
        }

        edx_ = kLegacyBattleDefeatFramebufferToken;
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleDefeatPanelCall::draw_title,
            kLegacyBattleVictoryFontToken,
            {
                kLegacyBattleDefeatFramebufferToken,
                0x104U,
                0xB4U,
                kLegacyBattleDefeatTitleToken,
                0xFFC0U,
                0x10U,
            },
            kDefeatTitle
        ));
        ++result_.title_draw_calls;

        eax_ = bindings_.target_selection.transition_stage + 0xD4U;
        result_.detail_frame_bottom = std::bit_cast<i32>(eax_);
        ecx_ = replace_low_word(
            ecx_, bindings_.victory.panel_action_record.field_4a
        );
        result_.second_frame_resource = ecx_;
        result_.second_frame_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.tiled_frames[1U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = ecx_,
                .left = 0xC8,
                .top = 0xD4,
                .right = 0x178,
                .bottom = std::bit_cast<i32>(eax_),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.detail_frame_return.eax;
        ecx_ = request_.detail_frame_return.ecx;
        edx_ = request_.detail_frame_return.edx;
        if (result_.tiled_frames[1U].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleDefeatPanelStatus::detail_frame_typed_stop;
        }
    }

    void set_font_size(const u32 size) {
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleDefeatPanelCall::set_font_size,
            kLegacyBattleVictoryFontToken,
            {kLegacyBattleVictoryFontToken, size}
        ));
        ++result_.font_size_calls;
    }

    void query_and_draw_detail() {
        result_.transition_stage = advance_legacy_battle_transition_stage(
            bindings_.target_selection.transition_stage,
            {.base_offset = 0xD4U, .target = 0xF4U, .divisor = 3U}
        );
        ++result_.transition_stage_calls;
        eax_ = result_.transition_stage.return_eax;
        ecx_ = result_.transition_stage.return_ecx;
        edx_ = result_.transition_stage.return_edx;
        if (result_.transition_stage.status !=
            LegacyBattleTransitionStageAdvanceStatus::completed) {
            result_.status =
                LegacyBattleDefeatPanelStatus::transition_stage_typed_stop;
            return;
        }
        if (eax_ != 1U) {
            return;
        }

        set_font_size(0x11U);
        edx_ = kLegacyBattleDefeatFramebufferToken;
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleDefeatPanelCall::draw_detail,
            kLegacyBattleVictoryFontToken,
            {
                kLegacyBattleDefeatFramebufferToken,
                0xFEU,
                0xD8U,
                kLegacyBattleDefeatDetailToken,
                0xFFC0U,
                0x10U,
            },
            kDefeatDetail
        ));
        ++result_.detail_draw_calls;
        set_font_size(0x10U);
    }

    [[nodiscard]] LegacyBattleDefeatPanelResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleDefeatPanelBindings bindings_;
    LegacyBattleDefeatPanelPort& port_;
    const LegacyBattleDefeatPanelRequest& request_;
    LegacyBattleDefeatPanelResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleDefeatPanelResult draw_legacy_battle_defeat_panel(
    const LegacyBattleDefeatPanelBindings bindings,
    LegacyBattleDefeatPanelPort& port,
    const LegacyBattleDefeatPanelRequest& request
) {
    return DefeatPanelRunner(bindings, port, request).run();
}

}  // namespace openswd3::battle
