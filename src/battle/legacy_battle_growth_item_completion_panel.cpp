#include "openswd3/battle/legacy_battle_growth_item_completion_panel.hpp"

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

inline constexpr std::array<u8, 4U> kFormatPrefix{
    0xAAU, 0x6BU, 0xC4U, 0x5FU
};  // CP950 "法寶"
inline constexpr std::array<u8, 12U> kFormatSuffix{
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
};  // CP950 "已完全成長!!"

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

class GrowthItemCompletionPanelRunner final {
public:
    GrowthItemCompletionPanelRunner(
        LegacyBattleGrowthItemCompletionPanelBindings bindings,
        LegacyBattleGrowthItemCompletionPanelPort& port,
        const LegacyBattleGrowthItemCompletionPanelRequest& request
    )
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGrowthItemCompletionPanelResult run() {
        local_text_[0U] = request_.initial_text_byte;
        std::fill(local_text_.begin() + 1, local_text_.end(), 0U);
        ecx_ = 0U;
        eax_ = 0U;
        if (bindings_.target_selection.transition_mode != 1U) {
            return finish();
        }

        if (!format_text()) {
            return finish();
        }
        measure_first();
        draw_panel();
        if (result_.status !=
            LegacyBattleGrowthItemCompletionPanelStatus::completed) {
            return finish();
        }
        measure_second();
        query_and_draw();
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleGrowthItemCompletionPanelCallReply invoke(
        const LegacyBattleGrowthItemCompletionPanelCall call,
        const std::array<u32, 8U>& arguments = {},
        const std::span<const u8> text = {}
    ) {
        LegacyBattleGrowthItemCompletionPanelCallRequest request{
            .call = call,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text_length = static_cast<u32>(text.size()),
        };
        std::copy(text.begin(), text.end(), request.text.begin());
        const auto reply = port_.invoke_growth_item_completion_panel(request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_transition_stage) {
            bindings_.target_selection.transition_stage =
                reply.transition_stage;
        }
        return reply;
    }

    [[nodiscard]] bool format_text() {
        fallback_.fill(0U);
        std::size_t fallback_length = 0U;
        for (const u8 value : kFormatPrefix) {
            fallback_[fallback_length++] = value;
        }

        bool source_terminated = false;
        for (const u8 value : bindings_.level_advancement.growth_caption_text) {
            if (value == 0U) {
                source_terminated = true;
                break;
            }
            fallback_[fallback_length++] = value;
        }
        if (source_terminated) {
            for (const u8 value : kFormatSuffix) {
                fallback_[fallback_length++] = value;
            }
        }

        ecx_ = request_.local_text_token;
        const auto reply = invoke(
            LegacyBattleGrowthItemCompletionPanelCall::format_text,
            {
                request_.local_text_token,
                kLegacyBattleGrowthItemCompletionFormatToken,
                kLegacyBattleGrowthItemCompletionCaptionToken,
            },
            std::span<const u8>(fallback_.data(), fallback_length)
        );
        ++result_.format_calls;

        const auto& source =
            reply.publish_formatted_text ? reply.formatted_text : fallback_;
        const u32 length = reply.publish_formatted_text
            ? reply.formatted_text_length
            : static_cast<u32>(fallback_length);
        const std::size_t copied_length =
            std::min<std::size_t>(length, local_text_.size());
        local_text_.fill(0U);
        std::copy_n(source.begin(), copied_length, local_text_.begin());
        local_text_length_ = static_cast<u32>(copied_length);

        if (!source_terminated) {
            result_.status = LegacyBattleGrowthItemCompletionPanelStatus::
                caption_source_typed_stop;
            return false;
        }
        if (length >= local_text_.size()) {
            result_.status = LegacyBattleGrowthItemCompletionPanelStatus::
                format_buffer_typed_stop;
            return false;
        }
        local_text_[static_cast<std::size_t>(length)] = 0U;
        local_text_length_ = length;
        return true;
    }

    void measure_first() {
        edx_ = request_.local_text_token;
        static_cast<void>(invoke(
            LegacyBattleGrowthItemCompletionPanelCall::measure_text,
            {request_.local_text_token},
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
        ));
        ++result_.length_calls;
        result_.first_measured_length = eax_;
        result_.half_text_length = std::bit_cast<i32>(eax_) / 2;
        const u32 half_bits = std::bit_cast<u32>(result_.half_text_length);
        panel_base_width_ = half_bits * 17U + 0x20U;
        result_.panel_base_width = panel_base_width_;
    }

    void draw_panel() {
        const u32 stage = bindings_.target_selection.transition_stage;
        result_.rectangle_width = std::bit_cast<i32>(panel_base_width_ + 0x0CU);
        result_.rectangle_height = std::bit_cast<i32>(stage + 8U);
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.shared_effects.pixel_conversion,
            {
                .x = 0xC4,
                .y = 0xD0,
                .width = result_.rectangle_width,
                .height = result_.rectangle_height,
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
            result_.status = LegacyBattleGrowthItemCompletionPanelStatus::
                rectangle_typed_stop;
            return;
        }

        ecx_ = stage;
        edx_ = replace_low_word(
            edx_, bindings_.victory_rewards.panel_action_record.field_4a
        );
        result_.frame_resource_id = edx_;
        result_.frame_right = std::bit_cast<i32>(panel_base_width_ + 0xC8U);
        result_.frame_bottom = std::bit_cast<i32>(stage + 0xD4U);
        result_.tiled_frame = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = edx_,
                .left = 0xC8,
                .top = 0xD4,
                .right = result_.frame_right,
                .bottom = result_.frame_bottom,
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.frame_return.eax;
        ecx_ = request_.frame_return.ecx;
        edx_ = request_.frame_return.edx;
        if (result_.tiled_frame.status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleGrowthItemCompletionPanelStatus::frame_typed_stop;
        }
    }

    void measure_second() {
        eax_ = request_.local_text_token;
        static_cast<void>(invoke(
            LegacyBattleGrowthItemCompletionPanelCall::measure_text,
            {request_.local_text_token},
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
        ));
        ++result_.length_calls;
        result_.second_measured_length = eax_;
    }

    void query_and_draw() {
        static_cast<void>(invoke(
            LegacyBattleGrowthItemCompletionPanelCall::query_panel,
            {0xD4U, 0xF4U, 3U}
        ));
        ++result_.query_calls;
        if (eax_ != 1U) {
            return;
        }

        ecx_ = kLegacyBattleGrowthItemCompletionFontToken;
        static_cast<void>(invoke(
            LegacyBattleGrowthItemCompletionPanelCall::set_font_size,
            {kLegacyBattleGrowthItemCompletionFontToken, 0x11U}
        ));
        ++result_.font_size_calls;

        edx_ = kLegacyBattleGrowthItemCompletionFramebufferToken;
        ecx_ = kLegacyBattleGrowthItemCompletionFontToken;
        static_cast<void>(invoke(
            LegacyBattleGrowthItemCompletionPanelCall::draw_text,
            {
                kLegacyBattleGrowthItemCompletionFramebufferToken,
                0xD8U,
                0xDAU,
                request_.local_text_token,
                0xFFC0U,
                0x10U,
            },
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
        ));
        ++result_.text_draw_calls;

        ecx_ = kLegacyBattleGrowthItemCompletionFontToken;
        static_cast<void>(invoke(
            LegacyBattleGrowthItemCompletionPanelCall::set_font_size,
            {kLegacyBattleGrowthItemCompletionFontToken, 0x10U}
        ));
        ++result_.font_size_calls;
    }

    [[nodiscard]] LegacyBattleGrowthItemCompletionPanelResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        result_.formatted_text = local_text_;
        result_.formatted_text_length = local_text_length_;
        return result_;
    }

    LegacyBattleGrowthItemCompletionPanelBindings bindings_;
    LegacyBattleGrowthItemCompletionPanelPort& port_;
    const LegacyBattleGrowthItemCompletionPanelRequest& request_;
    LegacyBattleGrowthItemCompletionPanelResult result_{};
    std::array<u8, 64U> local_text_{};
    std::array<u8, 64U> fallback_{};
    u32 local_text_length_{};
    u32 panel_base_width_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleGrowthItemCompletionPanelResult
advance_legacy_battle_growth_item_completion_panel(
    const LegacyBattleGrowthItemCompletionPanelBindings bindings,
    LegacyBattleGrowthItemCompletionPanelPort& port,
    const LegacyBattleGrowthItemCompletionPanelRequest& request
) {
    return GrowthItemCompletionPanelRunner(bindings, port, request).run();
}

}  // namespace openswd3::battle
