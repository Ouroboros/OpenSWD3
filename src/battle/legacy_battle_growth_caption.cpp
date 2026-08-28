#include "openswd3/battle/legacy_battle_growth_caption.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <span>

namespace openswd3::battle {
namespace {

using compat::i8;
using compat::i32;
using compat::u8;
using compat::u32;

inline constexpr u32 kFramebufferToken = 0x004CD76CU;
inline constexpr u32 kFontToken = 0x004C9A28U;
inline constexpr u32 kNameColor = 0xFFFFU;
inline constexpr u32 kDetailColor = 0xF000U;
inline constexpr u32 kFontSize = 0x10U;
inline constexpr u32 kCompletionSample = 0x160U;
inline constexpr std::array<std::array<u8, 4U>, 4U> kPartyNames{
    std::array<u8, 4U>{0xC1U, 0xC9U, 0xAFU, 0x53U},
    std::array<u8, 4U>{0xA9U, 0x67U, 0xA5U, 0x69U},
    std::array<u8, 4U>{0xA5U, 0x64U, 0xBAU, 0xBFU},
    std::array<u8, 4U>{0xA7U, 0xF5U, 0xB9U, 0x74U},
};

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const compat::u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

[[nodiscard]] std::span<const u8> default_name(const u32 label) noexcept {
    if (label >= 8U) {
        return {};
    }
    return kPartyNames[label % kPartyNames.size()];
}

class Runner {
public:
    Runner(
        LegacyBattleGrowthCaptionBindings bindings,
        LegacyBattleGrowthCaptionPort& port,
        const LegacyBattleGrowthCaptionRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGrowthCaptionResult run() {
        local_text_.fill(0U);
        local_text_[0U] = is_completion() ? request_.initial_text_byte : 0xFFU;
        eax_ = 0U;
        ecx_ = 0U;
        if (bindings_.victory.target_selection.transition_mode != 1U) {
            return finish();
        }
        if (is_completion() &&
            bindings_.victory.target_selection.transition_stage == 0U) {
            ecx_ = std::bit_cast<u32>(
                bindings_.victory.input_dispatch.sample_mix_level
            );
            const auto reply = port_.play_growth_completion_sample(
                eax_,
                ecx_,
                edx_,
                kCompletionSample,
                bindings_.victory.input_dispatch.sample_mix_level
            );
            ++result_.sample_calls;
            eax_ = reply.eax;
            ecx_ = reply.ecx;
            edx_ = reply.edx;
        }

        if (!format_name()) {
            return finish();
        }
        draw_frame();
        if (result_.status != LegacyBattleGrowthCaptionStatus::completed) {
            return finish();
        }
        query_and_draw();
        return finish();
    }

private:
    [[nodiscard]] bool is_completion() const noexcept {
        return request_.variant == LegacyBattleGrowthCaptionVariant::completion;
    }

    [[nodiscard]] LegacyBattleGrowthCaptionCallReply invoke(
        const LegacyBattleGrowthCaptionCall call,
        const std::array<u32, 6U>& arguments = {},
        const std::span<const u8> text = {}
    ) {
        LegacyBattleGrowthCaptionCallRequest request{
            .call = call,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text_length = static_cast<u32>(text.size()),
        };
        std::copy(text.begin(), text.end(), request.text.begin());
        const auto reply = port_.invoke_growth_caption(request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_transition_actor_index) {
            bindings_.victory.target_selection.transition_actor_index =
                reply.transition_actor_index;
        }
        if (reply.publish_transition_stage) {
            bindings_.victory.target_selection.transition_stage =
                reply.transition_stage;
        }
        return reply;
    }

    [[nodiscard]] bool format_name() {
        const i32 actor = static_cast<i32>(static_cast<i8>(
            bindings_.victory.target_selection.transition_actor_index
        ));
        const u32 actor_bits = std::bit_cast<u32>(actor);
        if (is_completion()) {
            edx_ = actor_bits;
        } else {
            ecx_ = actor_bits;
        }
        if (actor < 0 ||
            static_cast<std::size_t>(actor) >=
                bindings_.victory.startup.action_mode_source.actor_label_indices
                    .size()) {
            result_.status =
                LegacyBattleGrowthCaptionStatus::actor_index_typed_stop;
            return false;
        }
        const u32 label =
            bindings_.victory.startup.action_mode_source
                .actor_label_indices[static_cast<std::size_t>(actor)];
        const u32 name_token =
            kLegacyBattleGrowthCaptionNameBaseToken + (label << 4U);
        if (is_completion()) {
            eax_ = name_token;
            ecx_ = request_.local_text_token;
            edx_ = actor_bits;
        } else {
            eax_ = request_.local_text_token;
            ecx_ = actor_bits;
            edx_ = name_token;
        }
        const auto fallback = default_name(label);
        const auto reply = invoke(
            LegacyBattleGrowthCaptionCall::format_name,
            {
                request_.local_text_token,
                kLegacyBattleGrowthCaptionNameFormatToken,
                name_token,
            },
            fallback
        );
        ++result_.format_calls;
        if (!copy_formatted(
                reply,
                fallback,
                LegacyBattleGrowthCaptionStatus::name_format_buffer_typed_stop
            )) {
            return false;
        }

        if (is_completion()) {
            edx_ = request_.local_text_token;
        } else {
            ecx_ = request_.local_text_token;
        }
        eax_ = local_text_length_;
        edx_ = 0U;
        ++result_.length_calls;
        result_.name_length = eax_;
        name_half_length_ = static_cast<i32>(eax_ >> 1U);
        return true;
    }

    [[nodiscard]] bool copy_formatted(
        const LegacyBattleGrowthCaptionCallReply& reply,
        const std::span<const u8> fallback,
        const LegacyBattleGrowthCaptionStatus overflow_status
    ) {
        const auto& source = reply.publish_formatted_text
            ? reply.formatted_text
            : fallback_buffer(fallback);
        const u32 length = reply.publish_formatted_text
            ? reply.formatted_text_length
            : static_cast<u32>(fallback.size());
        if (length >= local_text_.size()) {
            result_.status = overflow_status;
            return false;
        }
        local_text_.fill(0U);
        std::copy_n(
            source.begin(),
            static_cast<std::size_t>(length),
            local_text_.begin()
        );
        local_text_[static_cast<std::size_t>(length)] = 0U;
        local_text_length_ = length;
        return true;
    }

    [[nodiscard]] const std::array<u8, 64U>&
    fallback_buffer(const std::span<const u8> bytes) noexcept {
        fallback_.fill(0U);
        std::copy(bytes.begin(), bytes.end(), fallback_.begin());
        return fallback_;
    }

    void draw_frame() {
        auto& action = bindings_.victory.state.panel_action_record;
        action.action_id = kLegacyBattleVictoryPanelAction;
        action.base_variant = 0U;
        result_.panel_action_update =
            bindings_.victory.action_updater.update(action);

        const i32 scaled_half = name_half_length_ * 20;
        result_.rectangle_width = scaled_half + 8;
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.victory.framebuffer,
            bindings_.victory.raster,
            bindings_.victory.shared_effects.pixel_conversion,
            {
                .x = 0xF6,
                .y = 0xB0,
                .width = result_.rectangle_width,
                .height = std::bit_cast<i32>(
                    bindings_.victory.target_selection.transition_stage + 8U
                ),
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
                LegacyBattleGrowthCaptionStatus::rectangle_typed_stop;
            return;
        }

        ecx_ = bindings_.victory.target_selection.transition_stage + 0xB4U;
        edx_ = replace_low_word(edx_, action.field_4a);
        result_.tiled_frame = rendering::draw_legacy_tiled_frame(
            bindings_.victory.framebuffer,
            bindings_.victory.raster,
            bindings_.victory.frame_provider,
            {
                .resource_id = edx_,
                .left = 0xFA,
                .top = 0xB4,
                .right = scaled_half + 0xFA,
                .bottom = std::bit_cast<i32>(ecx_),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.victory.shared_effects,
            bindings_.victory.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.frame_return.eax;
        ecx_ = request_.frame_return.ecx;
        edx_ = request_.frame_return.edx;
        if (result_.tiled_frame.status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status = LegacyBattleGrowthCaptionStatus::frame_typed_stop;
        }
    }

    void query_and_draw() {
        static_cast<void>(invoke(
            LegacyBattleGrowthCaptionCall::query_panel, {0xB4U, 0xECU, 3U}
        ));
        if (eax_ != 1U) {
            return;
        }

        ecx_ = kFontToken;
        if (is_completion()) {
            edx_ = kFramebufferToken;
        } else {
            eax_ = request_.local_text_token;
        }
        static_cast<void>(invoke(
            LegacyBattleGrowthCaptionCall::draw_text,
            {
                kFramebufferToken,
                0x104U,
                0xBCU,
                request_.local_text_token,
                kNameColor,
                kFontSize,
            },
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
        ));
        ++result_.text_draw_calls;

        local_text_.fill(0U);
        eax_ = 0U;
        ecx_ = 0U;
        if (is_completion()) {
            eax_ = request_.local_text_token;
        } else {
            edx_ = request_.local_text_token;
        }
        std::size_t caption_length = 0U;
        while (
            caption_length < bindings_.advancement.growth_caption_text.size() &&
            bindings_.advancement.growth_caption_text[caption_length] != 0U
        ) {
            ++caption_length;
        }
        std::array<u8, 26U> fallback{};
        fallback[0U] = 0x5BU;
        std::copy_n(
            bindings_.advancement.growth_caption_text.begin(),
            caption_length,
            fallback.begin() + 1
        );
        const bool source_terminated =
            caption_length < bindings_.advancement.growth_caption_text.size();
        std::size_t fallback_length = caption_length + 1U;
        if (source_terminated) {
            fallback[fallback_length++] = 0x5DU;
        }
        const auto reply = invoke(
            LegacyBattleGrowthCaptionCall::format_detail,
            {
                request_.local_text_token,
                kLegacyBattleGrowthCaptionDetailFormatToken,
                0x0053C154U,
            },
            std::span<const u8>(fallback.data(), fallback_length)
        );
        ++result_.format_calls;
        if (!source_terminated) {
            result_.status =
                LegacyBattleGrowthCaptionStatus::caption_source_typed_stop;
            return;
        }
        if (!copy_formatted(
                reply,
                std::span<const u8>(fallback.data(), fallback_length),
                LegacyBattleGrowthCaptionStatus::detail_format_buffer_typed_stop
            )) {
            return;
        }

        if (is_completion()) {
            ecx_ = request_.local_text_token;
        } else {
            eax_ = request_.local_text_token;
        }
        eax_ = local_text_length_;
        ++result_.length_calls;
        result_.detail_length = eax_;
        const i32 detail_quarter = static_cast<i32>(eax_ >> 2U);
        const i32 name_pixels = name_half_length_ * 8;
        result_.detail_x = name_pixels - detail_quarter * 16 + 0xFA;
        if (is_completion()) {
            eax_ = kFramebufferToken;
            edx_ = request_.local_text_token;
        } else {
            edx_ = kFramebufferToken;
            eax_ = static_cast<u32>(detail_quarter * 16);
        }
        ecx_ = kFontToken;
        static_cast<void>(invoke(
            LegacyBattleGrowthCaptionCall::draw_text,
            {
                kFramebufferToken,
                std::bit_cast<u32>(result_.detail_x),
                0xD2U,
                request_.local_text_token,
                kDetailColor,
                kFontSize,
            },
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
        ));
        ++result_.text_draw_calls;
    }

    [[nodiscard]] LegacyBattleGrowthCaptionResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleGrowthCaptionBindings bindings_;
    LegacyBattleGrowthCaptionPort& port_;
    const LegacyBattleGrowthCaptionRequest& request_;
    LegacyBattleGrowthCaptionResult result_{};
    std::array<u8, 64U> local_text_{};
    std::array<u8, 64U> fallback_{};
    u32 local_text_length_{};
    i32 name_half_length_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleGrowthCaptionResult advance_legacy_battle_growth_caption(
    LegacyBattleGrowthCaptionBindings bindings,
    LegacyBattleGrowthCaptionPort& port,
    const LegacyBattleGrowthCaptionRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
