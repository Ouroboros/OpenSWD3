#include "openswd3/battle/legacy_battle_selection_hint_frame.hpp"

#include "openswd3/battle/legacy_battle_startup.hpp"

#include <bit>
#include <charconv>
#include <cstddef>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using Call = LegacyBattleSelectionHintFrameCall;
using Reply = LegacyBattleSelectionHintFrameCallReply;
using Request = LegacyBattleSelectionHintFrameCallRequest;
using Status = LegacyBattleSelectionHintFrameStatus;

constexpr u32 kPanelActionId = 0x233BU;
constexpr u32 kGroupBCount = 8U;

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32
arithmetic_shift_right_one(const u32 value) noexcept {
    return std::bit_cast<u32>(std::bit_cast<i32>(value) >> 1);
}

[[nodiscard]] constexpr u32 group_b_scaled_837(const u32 code) noexcept {
    u32 scaled = code + code * 2U;
    scaled <<= 3U;
    scaled -= code;
    scaled += scaled * 2U;
    return scaled + scaled * 4U;
}

[[nodiscard]] constexpr u32 group_b_scaled_3349(const u32 code) noexcept {
    return code + group_b_scaled_837(code) * 4U;
}

[[nodiscard]] constexpr u32 group_b_actor_token(const u32 code) noexcept {
    return kLegacyBattleSelectionHintGroupBBaseToken +
        group_b_scaled_3349(code) * 8U;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

class Runner final {
public:
    Runner(
        LegacyBattleSelectionHintFrameBindings bindings,
        LegacyBattleSelectionHintFramePort& port,
        const LegacyBattleSelectionHintFrameRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleSelectionHintFrameResult run() {
        if (!read_party_source()) {
            return finish();
        }
        if (eax_ == 1U || bindings_.target_selection_block == 1U) {
            return finish();
        }

        ecx_ = bindings_.published_actor_code;
        if (std::bit_cast<i32>(ecx_) <= 0 ||
            std::bit_cast<i32>(ecx_) >
                std::bit_cast<i32>(bindings_.group_b_count)) {
            return finish();
        }

        result_.actor_code = ecx_;
        if (!query_actor_label()) {
            return finish();
        }
        compute_panel_geometry();
        if (!draw_panel()) {
            return finish();
        }
        draw_label();
        query_metric();
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleSelectionHintFrameResult finish() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    [[nodiscard]] Reply invoke(const Request& request) {
        const Reply reply = port_.invoke_selection_hint_frame(request);
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return reply;
    }

    [[nodiscard]] bool read_party_source() {
        edx_ = bindings_.queued_actor_code * 5U - 0x28U;
        ecx_ = 0U;
        if (edx_ >= bindings_.party_source_words.size()) {
            result_.status = Status::party_source_typed_stop;
            return false;
        }
        eax_ = bindings_.party_source_words[edx_];
        return true;
    }

    [[nodiscard]] bool query_actor_label() {
        const u32 code = result_.actor_code;
        eax_ = group_b_scaled_837(code);
        ecx_ = group_b_actor_token(code);
        result_.actor_token = ecx_;
        if (code == 0U || code > kGroupBCount) {
            result_.status = Status::group_b_actor_typed_stop;
            return false;
        }
        const auto reply = invoke({
            .call = Call::query_actor_label,
            .object_token = ecx_,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_label_query_calls;
        result_.label_token = reply.eax;
        result_.label_length = reply.text_length;
        const u32 sign =
            (reply.text_length & 0x80000000U) == 0U ? 0U : 0xFFFFFFFFU;
        result_.label_character_count =
            arithmetic_shift_right_one(reply.text_length - sign);
        return true;
    }

    void compute_panel_geometry() noexcept {
        const u32 text_width = result_.label_character_count * 0x14U;
        if (bindings_.mirror_mode == 1U) {
            result_.panel_x = 0x276U - text_width - request_.origin_x;
        } else {
            result_.panel_x = request_.origin_x;
        }
        result_.panel_y = request_.origin_y;
    }

    [[nodiscard]] bool draw_panel() {
        bindings_.panel_action_record.action_id = kPanelActionId;
        bindings_.panel_action_record.base_variant = 0U;
        result_.panel_action_update =
            bindings_.action_updater.update(bindings_.panel_action_record);
        ++result_.panel_action_update_calls;

        const u32 text_width = result_.label_character_count * 0x14U;
        result_.panel_rectangle_status =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(result_.panel_x - 4U),
                    .y = wrapping_i32(result_.panel_y - 4U),
                    .width = wrapping_i32(text_width + 8U),
                    .height = 0x18,
                    .red = 0,
                    .green = -8,
                    .blue = -8,
                    .mode = 1U,
                }
            );
        ++result_.panel_rectangle_calls;
        eax_ = request_.panel_rectangle_return_registers.eax;
        ecx_ = request_.panel_rectangle_return_registers.ecx;
        edx_ = request_.panel_rectangle_return_registers.edx;
        if (!rectangle_completed(result_.panel_rectangle_status)) {
            result_.status = Status::panel_rectangle_typed_stop;
            return false;
        }

        ecx_ = replace_low_word(ecx_, bindings_.panel_action_record.field_4a);
        result_.tiled_frame = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = ecx_,
                .left = wrapping_i32(result_.panel_x),
                .top = wrapping_i32(result_.panel_y),
                .right = wrapping_i32(result_.panel_x + text_width),
                .bottom = wrapping_i32(result_.panel_y + 0x10U),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.tiled_frame_return_registers.eax;
        ecx_ = request_.tiled_frame_return_registers.ecx;
        edx_ = request_.tiled_frame_return_registers.edx;
        if (result_.tiled_frame.status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status = Status::tiled_frame_typed_stop;
            return false;
        }
        return true;
    }

    void configure_font_width(const u32 width) {
        static_cast<void>(invoke({
            .call = Call::configure_font_width,
            .object_token = kLegacyBattleSelectionHintTextObjectToken,
            .arguments = {width},
            .eax = eax_,
            .ecx = kLegacyBattleSelectionHintTextObjectToken,
            .edx = edx_,
        }));
        ++result_.font_width_calls;
    }

    void draw_text(
        const u32 x,
        const u32 y,
        const u32 text_token,
        const std::array<compat::u8, 20>& text_bytes,
        const u32 text_length,
        const u32 call_eax,
        const u32 call_ecx,
        const u32 call_edx
    ) {
        static_cast<void>(invoke({
            .call = Call::draw_text,
            .object_token = kLegacyBattleSelectionHintTextObjectToken,
            .arguments =
                {
                    kLegacyBattleSelectionHintFontToken,
                    x,
                    y,
                    text_token,
                    0xFFFFU,
                    0x10U,
                },
            .eax = call_eax,
            .ecx = call_ecx,
            .edx = call_edx,
            .text_token = text_token,
            .text_bytes = text_bytes,
            .text_length = text_length,
        }));
        ++result_.text_draw_calls;
    }

    void draw_label() {
        configure_font_width(0x14U);
        std::array<compat::u8, 20> empty{};
        draw_text(
            result_.panel_x + 2U,
            result_.panel_y,
            result_.label_token,
            empty,
            0U,
            kLegacyBattleSelectionHintFontToken,
            kLegacyBattleSelectionHintTextObjectToken,
            result_.panel_x + 2U
        );
        result_.label_drawn = true;
        configure_font_width(0x10U);
    }

    void query_metric() {
        const u32 code = result_.actor_code;
        eax_ = group_b_scaled_3349(code);
        edx_ = group_b_scaled_837(code);
        ecx_ = group_b_actor_token(code);
        const auto source = invoke({
            .call = Call::query_metric_source,
            .object_token = ecx_,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.metric_source_calls;
        result_.fixed_count_lookup = lookup_legacy_battle_fixed_count(
            port_.legacy_battle_fixed_object_state(),
            {
                .key = source.eax,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.metric_value_calls;
        eax_ = result_.fixed_count_lookup.return_eax;
        ecx_ = result_.fixed_count_lookup.return_ecx;
        edx_ = result_.fixed_count_lookup.return_edx;
        if (result_.fixed_count_lookup.status !=
            LegacyBattleFixedCountStatus::completed) {
            result_.status = Status::fixed_count_typed_stop;
            return;
        }
        result_.metric_value = static_cast<u16>(eax_);
        if (result_.metric_value < 0x0AU) {
            return;
        }
        if (!draw_metric_text()) {
            return;
        }
        if (result_.metric_value < 0x0FU) {
            return;
        }
        draw_metric_fade();
    }

    [[nodiscard]] bool draw_metric_text() {
        const u32 code = result_.actor_code;
        eax_ = group_b_scaled_837(code);
        ecx_ = group_b_actor_token(code);
        edx_ = request_.local_current_token;
        const auto pair = invoke({
            .call = Call::query_metric_pair,
            .object_token = ecx_,
            .arguments =
                {request_.local_current_token, request_.local_limit_token},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.metric_pair_calls;
        if (!format_metric_text(pair.metric_current, pair.metric_limit)) {
            result_.status = Status::format_buffer_typed_stop;
            return false;
        }

        eax_ = bindings_.mirror_mode;
        const u32 x = bindings_.mirror_mode == 0U
            ? result_.panel_x + result_.label_character_count * 0x14U + 0x0AU
            : result_.panel_x - 0x8CU;
        const u32 y = result_.panel_y - 2U;
        const bool nonzero_mirror = bindings_.mirror_mode != 0U;
        draw_text(
            x,
            y,
            request_.local_text_token,
            result_.formatted_text,
            result_.formatted_text_length,
            nonzero_mirror ? kLegacyBattleSelectionHintFontToken : y,
            kLegacyBattleSelectionHintTextObjectToken,
            nonzero_mirror ? x : kLegacyBattleSelectionHintFontToken
        );
        result_.metric_text_drawn = true;
        return true;
    }

    [[nodiscard]] bool append_byte(const compat::u8 value) noexcept {
        if (result_.formatted_text_length >= result_.formatted_text.size()) {
            return false;
        }
        result_.formatted_text[result_.formatted_text_length++] = value;
        return true;
    }

    [[nodiscard]] bool append_signed(const u32 value) noexcept {
        std::array<char, 16> buffer{};
        const auto conversion = std::to_chars(
            buffer.data(),
            buffer.data() + buffer.size(),
            std::bit_cast<i32>(value)
        );
        if (conversion.ec != std::errc{}) {
            return false;
        }
        for (const char* cursor = buffer.data(); cursor != conversion.ptr;
             ++cursor) {
            if (!append_byte(static_cast<compat::u8>(*cursor))) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool
    format_metric_text(const u32 current, const u32 limit) noexcept {
        result_.formatted_text.fill(0U);
        result_.formatted_text_length = 0U;
        for (const compat::u8 byte : kLegacyBattleSelectionHintVitalityPrefix) {
            if (!append_byte(byte)) {
                return false;
            }
        }
        if (!append_signed(current) || !append_byte(0x2FU) ||
            !append_signed(limit) || !append_byte(0U)) {
            return false;
        }
        --result_.formatted_text_length;
        return true;
    }

    void draw_metric_fade() {
        const u32 code = result_.actor_code;
        eax_ = group_b_scaled_3349(code);
        edx_ = group_b_scaled_837(code);
        ecx_ = group_b_actor_token(code);
        const LegacyBattleActorProgressState* actor_progress =
            code != 0U && code <= bindings_.startup.enemies.size()
            ? &bindings_.startup.enemies[code - 1U].progress
            : nullptr;
        result_.actor_progress_width = query_legacy_battle_actor_progress_width(
            actor_progress,
            &bindings_.startup.timing,
            {.actor_token = ecx_, .entry_edx = edx_}
        );
        ++result_.fade_width_calls;
        eax_ = result_.actor_progress_width.return_eax;
        ecx_ = result_.actor_progress_width.return_ecx;
        edx_ = result_.actor_progress_width.return_edx;
        if (result_.actor_progress_width.status !=
            LegacyBattleActorProgressWidthStatus::completed) {
            result_.status = Status::actor_progress_width_typed_stop;
            return;
        }
        result_.fade_width = static_cast<u16>(eax_);
        if (result_.fade_width == 0U) {
            return;
        }

        eax_ = bindings_.mirror_mode;
        const auto color = invoke({
            .call = Call::query_fade_color,
            .arguments = {0U, 0U, 0x18U},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.fade_color_calls;
        result_.fade_color = color.eax;
        const u32 x = bindings_.mirror_mode == 0U
            ? result_.panel_x + result_.label_character_count * 0x14U + 0x0AU
            : result_.panel_x - 0x8CU;
        const u32 y = result_.panel_y + 0x11U;
        const auto fade = fade_legacy_battle_rectangle(
            bindings_.state.color_fade,
            bindings_.framebuffer,
            bindings_.clip,
            bindings_.shared_request,
            bindings_.shared_effects,
            bindings_.jitter,
            wrapping_i32(x),
            wrapping_i32(y),
            wrapping_i32(result_.fade_width),
            3,
            result_.fade_color
        );
        ++result_.color_fade_calls;
        result_.color_fade_status = fade.status;
        eax_ = request_.color_fade_return_registers.eax;
        ecx_ = request_.color_fade_return_registers.ecx;
        edx_ = request_.color_fade_return_registers.edx;
        if (fade.status != rendering::LegacyBlitExecutionStatus::completed &&
            fade.status != rendering::LegacyBlitExecutionStatus::clipped_out &&
            fade.status !=
                rendering::LegacyBlitExecutionStatus::opacity_disabled) {
            result_.status = Status::color_fade_typed_stop;
            return;
        }
        result_.fade_drawn = true;
    }

    LegacyBattleSelectionHintFrameBindings bindings_;
    LegacyBattleSelectionHintFramePort& port_;
    const LegacyBattleSelectionHintFrameRequest& request_;
    LegacyBattleSelectionHintFrameResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleSelectionHintFrameResult draw_legacy_battle_selection_hint_frame(
    LegacyBattleSelectionHintFrameBindings bindings,
    LegacyBattleSelectionHintFramePort& port,
    const LegacyBattleSelectionHintFrameRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
