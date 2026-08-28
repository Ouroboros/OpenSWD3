#include "openswd3/battle/legacy_battle_talisman_result_panel.hpp"

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

constexpr std::array<u8, 8U> kSuccessTitle{
    0xB7U, 0xD2U, 0xB2U, 0xC5U, 0xA6U, 0xA8U, 0xA5U, 0x5CU
};
constexpr std::array<u8, 11U> kSuccessFormat{
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
};
constexpr std::array<u8, 8U> kFailureTitle{
    0xB7U, 0xD2U, 0xB2U, 0xC5U, 0xA5U, 0xA2U, 0xB1U, 0xD1U
};
constexpr std::array<u8, 12U> kFailureDetail{
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

class TalismanResultPanelRunner final {
public:
    TalismanResultPanelRunner(
        LegacyBattleTalismanResultPanelBindings bindings,
        LegacyBattleTalismanResultPanelPort& port,
        const LegacyBattleTalismanResultPanelRequest& request
    )
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {
        local_text_.fill(0U);
        local_text_[0U] = request.local_text_seed;
        result_.local_text = local_text_;
    }

    [[nodiscard]] LegacyBattleTalismanResultPanelResult run() {
        draw_panels();
        if (result_.status !=
            LegacyBattleTalismanResultPanelStatus::completed) {
            return finish();
        }
        query_and_draw_result();
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleTalismanResultPanelCallReply invoke(
        const LegacyBattleTalismanResultPanelCall call,
        const u32 object_token = 0U,
        const std::array<u32, 8U> arguments = {},
        const u32 item_name_token = 0U,
        const std::span<const u8> text = {}
    ) {
        LegacyBattleTalismanResultPanelCallRequest call_request{
            .call = call,
            .object_token = object_token,
            .arguments = arguments,
            .item_name_token = item_name_token,
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
        const auto reply = port_.invoke_talisman_result_panel(call_request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_stage) {
            bindings_.target_selection.transition_stage = reply.stage;
        }
        if (reply.publish_result_mode) {
            bindings_.target_selection.transition_aux_byte = reply.result_mode;
        }
        return reply;
    }

    void draw_panels() {
        eax_ = 0U;
        ecx_ = 0U;
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

        ecx_ = bindings_.target_selection.transition_stage + 0x28U;
        result_.rectangle_height = std::bit_cast<i32>(ecx_);
        result_.rectangle_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.shared_effects.pixel_conversion,
            {
                .x = 0xC4,
                .y = 0xB0,
                .width = 0xB8,
                .height = std::bit_cast<i32>(ecx_),
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
                LegacyBattleTalismanResultPanelStatus::rectangle_typed_stop;
            return;
        }

        edx_ = replace_low_word(
            edx_, bindings_.victory.panel_action_record.field_4a
        );
        result_.first_frame_resource = edx_;
        result_.first_frame_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.tiled_frames[0U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = edx_,
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
                LegacyBattleTalismanResultPanelStatus::title_frame_typed_stop;
            return;
        }

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
                LegacyBattleTalismanResultPanelStatus::detail_frame_typed_stop;
        }
    }

    void draw_text(
        const LegacyBattleTalismanResultPanelCall call,
        const u32 x,
        const u32 y,
        const u32 token,
        const std::span<const u8> text
    ) {
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            call,
            kLegacyBattleVictoryFontToken,
            {
                kLegacyBattleTalismanFramebufferToken,
                x,
                y,
                token,
                0xFFC0U,
                0x10U,
            },
            0U,
            text
        ));
        ++result_.title_draw_calls;
    }

    [[nodiscard]] bool format_success_detail() {
        const u32 item_name_token = bindings_.victory.player_item_tokens[0U];
        eax_ = item_name_token;
        ecx_ = request_.local_text_token;
        const auto formatted = invoke(
            LegacyBattleTalismanResultPanelCall::format_success_detail,
            0U,
            {
                request_.local_text_token,
                kLegacyBattleTalismanSuccessFormatToken,
                item_name_token,
            },
            item_name_token,
            kSuccessFormat
        );
        ++result_.format_calls;

        const std::array<u8, 64U> empty{};
        const auto& source =
            formatted.publish_formatted_text ? formatted.formatted_text : empty;
        const u32 length = formatted.publish_formatted_text
            ? formatted.formatted_text_length
            : 0U;
        const std::size_t copied =
            std::min<std::size_t>(length, local_text_.size());
        std::copy_n(source.begin(), copied, local_text_.begin());
        result_.stopped_text_index = static_cast<u32>(copied);
        result_.local_text_length = static_cast<u32>(copied);
        if (length >= local_text_.size()) {
            result_.local_text = local_text_;
            result_.status =
                LegacyBattleTalismanResultPanelStatus::format_buffer_typed_stop;
            return false;
        }
        local_text_[copied] = 0U;
        local_text_length_ = length;
        result_.local_text = local_text_;
        result_.local_text_length = length;
        return true;
    }

    void query_and_draw_result() {
        result_.transition_stage = advance_legacy_battle_transition_stage(
            bindings_.target_selection.transition_stage,
            {.base_offset = 0xD4U, .target = 0xFCU, .divisor = 3U}
        );
        ++result_.transition_stage_calls;
        eax_ = result_.transition_stage.return_eax;
        ecx_ = result_.transition_stage.return_ecx;
        edx_ = result_.transition_stage.return_edx;
        if (result_.transition_stage.status !=
            LegacyBattleTransitionStageAdvanceStatus::completed) {
            result_.status = LegacyBattleTalismanResultPanelStatus::
                transition_stage_typed_stop;
            return;
        }
        if (eax_ != 1U) {
            return;
        }

        eax_ = (eax_ & 0xFFFFFF00U) |
            static_cast<u32>(bindings_.target_selection.transition_aux_byte);
        edx_ = kLegacyBattleTalismanFramebufferToken;
        if (bindings_.target_selection.transition_aux_byte == 1U) {
            draw_text(
                LegacyBattleTalismanResultPanelCall::draw_success_title,
                0x100U,
                0xB4U,
                kLegacyBattleTalismanSuccessTitleToken,
                kSuccessTitle
            );
            if (!format_success_detail()) {
                return;
            }
            eax_ = kLegacyBattleTalismanFramebufferToken;
            edx_ = request_.local_text_token;
            ecx_ = kLegacyBattleVictoryFontToken;
            static_cast<void>(invoke(
                LegacyBattleTalismanResultPanelCall::draw_success_detail,
                kLegacyBattleVictoryFontToken,
                {
                    kLegacyBattleTalismanFramebufferToken,
                    0xD0U,
                    0xDEU,
                    request_.local_text_token,
                    0xFFC0U,
                    0x10U,
                },
                bindings_.victory.player_item_tokens[0U],
                std::span<const u8>(local_text_.data(), local_text_length_)
            ));
            ++result_.detail_draw_calls;
            return;
        }

        draw_text(
            LegacyBattleTalismanResultPanelCall::draw_failure_title,
            0x100U,
            0xB4U,
            kLegacyBattleTalismanFailureTitleToken,
            kFailureTitle
        );
        edx_ = kLegacyBattleTalismanFramebufferToken;
        draw_text(
            LegacyBattleTalismanResultPanelCall::draw_failure_detail,
            0xD0U,
            0xDEU,
            kLegacyBattleTalismanFailureDetailToken,
            kFailureDetail
        );
        --result_.title_draw_calls;
        ++result_.detail_draw_calls;
    }

    [[nodiscard]] LegacyBattleTalismanResultPanelResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleTalismanResultPanelBindings bindings_;
    LegacyBattleTalismanResultPanelPort& port_;
    const LegacyBattleTalismanResultPanelRequest& request_;
    LegacyBattleTalismanResultPanelResult result_{};
    std::array<u8, 64U> local_text_{};
    u32 local_text_length_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleTalismanResultPanelResult draw_legacy_battle_talisman_result_panel(
    const LegacyBattleTalismanResultPanelBindings bindings,
    LegacyBattleTalismanResultPanelPort& port,
    const LegacyBattleTalismanResultPanelRequest& request
) {
    return TalismanResultPanelRunner(bindings, port, request).run();
}

}  // namespace openswd3::battle
