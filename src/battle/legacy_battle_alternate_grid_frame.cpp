#include "openswd3/battle/legacy_battle_alternate_grid_frame.hpp"

#include <algorithm>
#include <bit>
#include <charconv>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using Call = LegacyBattleGridFrameCall;
using Status = LegacyBattleAlternateGridFrameStatus;

inline constexpr u32 kPanelActionId = 0x233BU;
inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kFontToken = 0x004C9A28U;
inline constexpr u32 kTextSurfaceToken = 0x004CD76CU;
inline constexpr u32 kPanelRowLimitToken = 0x0053BDF4U;

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_scaled_1007(const u32 code) noexcept {
    const u32 index = code - 8U;
    u32 scaled = (index << 6U) - index;
    scaled = (scaled << 4U) - index;
    return scaled;
}

[[nodiscard]] constexpr u32 group_a_scaled_3021(const u32 code) noexcept {
    const u32 scaled = group_a_scaled_1007(code);
    return scaled + scaled * 2U;
}

[[nodiscard]] constexpr u32 group_a_actor_token(const u32 code) noexcept {
    return kGroupABaseToken + group_a_scaled_3021(code) * 4U;
}

[[nodiscard]] constexpr bool valid_group_a_code(const u32 code) noexcept {
    return code >= 8U && code - 8U < kGroupACount;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFF0000U) | (low & 0xFFFFU);
}

[[nodiscard]] constexpr bool panel_rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

[[nodiscard]] constexpr bool selection_rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out;
}

[[nodiscard]] u32 format_signed_width_two(
    std::array<u8, 20>& destination, const i16 value
) noexcept {
    destination.fill(0U);
    std::array<char, 8> digits{};
    const auto converted = std::to_chars(
        digits.data(), digits.data() + digits.size(), static_cast<i32>(value)
    );
    const std::size_t digit_count =
        static_cast<std::size_t>(converted.ptr - digits.data());
    const std::size_t padding = digit_count < 2U ? 2U - digit_count : 0U;
    std::fill_n(destination.begin(), padding, static_cast<u8>(' '));
    std::transform(
        digits.begin(),
        digits.begin() + static_cast<std::ptrdiff_t>(digit_count),
        destination.begin() + static_cast<std::ptrdiff_t>(padding),
        [](const char character) { return static_cast<u8>(character); }
    );
    return static_cast<u32>(padding + digit_count);
}

class Executor {
public:
    Executor(
        LegacyBattleAlternateGridFrameState& state,
        const LegacyBattleAlternateGridFrameBindings bindings,
        LegacyBattleGridFramePort& port,
        const LegacyBattleAlternateGridFrameRequest& request
    )
        : state_(state), bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleAlternateGridFrameResult run() {
        state_.row_text.fill(0U);
        state_.numeric_text.fill(0U);
        state_.row_text[0U] = 0xFFU;
        state_.displayed_rows = 0U;
        state_.row_value = 0U;
        state_.numeric_text_length = 0U;
        eax_ = bindings_.queued_actor_code;
        ecx_ = 0U;
        edx_ = 0U;
        if (eax_ == 0U) {
            return finish();
        }

        if (!draw_panel()) {
            return finish();
        }
        draw_title();
        invoke_font(Call::configure_font_mode, 0U);
        invoke_font(Call::configure_font_style, 0xFFFEU);
        invoke_font(Call::configure_font_width, 0x10U);

        bindings_.panel_row_limit = 0U;
        const u32 actor_code = bindings_.queued_actor_code;
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        edx_ = actor_code;
        if (!valid_group_a_code(actor_code)) {
            result_.status = Status::group_a_actor_typed_stop;
            return finish();
        }

        const auto initialized = invoke({
            .call = Call::initialize_rows,
            .object_token = ecx_,
            .arguments = {4U, kPanelRowLimitToken},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_initialization_calls;
        if (initialized.publish_panel_row_limit) {
            bindings_.panel_row_limit = initialized.panel_row_limit;
        }
        refresh_actor(actor_code);

        u32 iterator = request_.scroll_offset;
        for (;;) {
            ++iterator;
            result_.final_iterator = iterator;
            eax_ = group_a_scaled_1007(actor_code);
            ecx_ = group_a_actor_token(actor_code);
            edx_ = group_a_scaled_3021(actor_code);
            const auto queried = invoke({
                .call = Call::query_alternate_row,
                .object_token = ecx_,
                .arguments =
                    {
                        0U,
                        iterator,
                        request_.row_text_token,
                        request_.row_value_token,
                    },
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
                .text_token = request_.row_text_token,
                .text_bytes = state_.row_text,
            });
            ++result_.row_query_calls;
            if (queried.publish_row_value) {
                state_.row_value = queried.row_value;
            }
            if (queried.publish_row_text) {
                state_.row_text = queried.row_text;
            }
            if (eax_ == 0U) {
                refresh_actor(actor_code);
                break;
            }

            ++result_.scanned_rows;
            if (static_cast<u16>(state_.displayed_rows) >= 7U) {
                break;
            }
            if (!draw_row(iterator)) {
                return finish();
            }
            invoke_font(Call::configure_font_style, 0xFFFEU);
            ++state_.displayed_rows;
            result_.displayed_rows = state_.displayed_rows;
        }

        invoke_font(Call::configure_font_style, 0xFFFEU);
        return finish();
    }

private:
    [[nodiscard]] bool draw_panel() {
        static_cast<void>(
            clear_legacy_battle_action_record(bindings_.panel_action_record)
        );
        bindings_.panel_action_record.action_id = kPanelActionId;
        bindings_.panel_action_record.base_variant = 0U;
        result_.panel_action_update =
            bindings_.action_updater.update(bindings_.panel_action_record);
        ++result_.panel_action_update_calls;

        result_.panel_rectangle_status =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(request_.origin_x),
                    .y = wrapping_i32(request_.origin_y),
                    .width = 0xBE,
                    .height = 0xBC,
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
        if (!panel_rectangle_completed(result_.panel_rectangle_status)) {
            result_.status = Status::panel_rectangle_typed_stop;
            return false;
        }

        edx_ = replace_low_word(edx_, bindings_.panel_action_record.field_4a);
        if (!draw_tiled_frame(
                0U,
                request_.origin_x + 6U,
                request_.origin_y + 8U,
                request_.origin_x + 0xBAU,
                request_.origin_y + 0x18U,
                Status::first_tiled_frame_typed_stop
            )) {
            return false;
        }
        edx_ = replace_low_word(edx_, bindings_.panel_action_record.field_4a);
        return draw_tiled_frame(
            1U,
            request_.origin_x + 6U,
            request_.origin_y + 0x28U,
            request_.origin_x + 0xBAU,
            request_.origin_y + 0xB8U,
            Status::second_tiled_frame_typed_stop
        );
    }

    [[nodiscard]] bool draw_tiled_frame(
        const std::size_t index,
        const u32 left,
        const u32 top,
        const u32 right,
        const u32 bottom,
        const Status failure_status
    ) {
        result_.tiled_frames[index] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = edx_,
                .left = wrapping_i32(left),
                .top = wrapping_i32(top),
                .right = wrapping_i32(right),
                .bottom = wrapping_i32(bottom),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        const auto& registers = request_.tiled_frame_return_registers[index];
        eax_ = registers.eax;
        ecx_ = registers.ecx;
        edx_ = registers.edx;
        if (result_.tiled_frames[index].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status = failure_status;
            return false;
        }
        return true;
    }

    void draw_title() {
        eax_ = kTextSurfaceToken;
        ecx_ = kFontToken;
        invoke_text(
            request_.origin_x + 0x50U,
            request_.origin_y + 8U,
            request_.title_text_token,
            0xFFC0U,
            {},
            0U,
            eax_,
            ecx_,
            edx_
        );
    }

    [[nodiscard]] bool draw_row(const u32 iterator) {
        const std::size_t index =
            static_cast<std::size_t>(state_.displayed_rows);
        auto& trace = result_.rows[index];
        trace.iterator = iterator;
        trace.value = state_.row_value;
        trace.displayed_row = state_.displayed_rows + 1U;
        trace.row_text = state_.row_text;

        const u32 row_y =
            request_.origin_y + state_.displayed_rows * 20U + 0x2CU;
        ecx_ = replace_low_word(ecx_, bindings_.primary_text_color);
        const u32 name_color = ecx_;
        invoke_text(
            request_.origin_x + 0x10U,
            row_y,
            request_.row_text_token,
            name_color,
            state_.row_text,
            0U,
            request_.origin_x + 0x10U,
            kFontToken,
            request_.row_text_token
        );

        state_.numeric_text_length = format_signed_width_two(
            state_.numeric_text,
            std::bit_cast<i16>(static_cast<u16>(state_.row_value))
        );
        trace.numeric_text = state_.numeric_text;
        trace.numeric_text_length = state_.numeric_text_length;
        const auto& format_registers = request_.format_return_registers[index];
        eax_ = state_.numeric_text_length;
        ecx_ = format_registers.ecx;
        edx_ = format_registers.edx;
        ecx_ = replace_low_word(ecx_, bindings_.primary_text_color);
        const u32 numeric_color = ecx_;
        invoke_text(
            request_.origin_x + 0x90U,
            row_y,
            request_.numeric_text_token,
            numeric_color,
            state_.numeric_text,
            state_.numeric_text_length,
            request_.origin_x + 0x90U,
            kFontToken,
            request_.numeric_text_token
        );

        eax_ = request_.selected_row;
        edx_ = state_.displayed_rows + 1U;
        if (eax_ != edx_) {
            return true;
        }
        trace.selected = true;
        eax_ = replace_low_word(eax_, bindings_.primary_text_color);
        const u32 selected_color = eax_;
        invoke_text(
            request_.origin_x + 0x0FU,
            request_.origin_y + state_.displayed_rows * 20U + 0x2BU,
            request_.row_text_token,
            selected_color,
            state_.row_text,
            0U,
            kTextSurfaceToken,
            kFontToken,
            request_.origin_x + 0x0FU
        );
        invoke_font(Call::configure_font_style, 0xFFFEU);

        result_.selection_rectangle_status =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(request_.origin_x),
                    .y = wrapping_i32(
                        request_.origin_y + state_.displayed_rows * 20U + 0x28U
                    ),
                    .width = 0xC2,
                    .height = 0x18,
                    .red = 0x1F,
                    .green = 0x14,
                    .blue = 0,
                    .mode = 5U,
                }
            );
        ++result_.selection_rectangle_calls;
        eax_ = request_.selection_rectangle_return_registers.eax;
        ecx_ = request_.selection_rectangle_return_registers.ecx;
        edx_ = request_.selection_rectangle_return_registers.edx;
        if (!selection_rectangle_completed(
                result_.selection_rectangle_status
            )) {
            result_.status = Status::selection_rectangle_typed_stop;
            return false;
        }

        eax_ = iterator;
        bindings_.target_argument = eax_;
        bindings_.selection_input_gate = 1U;
        result_.selected_iterator = iterator;
        return true;
    }

    void refresh_actor(const u32 actor_code) {
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        edx_ = group_a_scaled_3021(actor_code);
        invoke({
            .call = Call::refresh_actor,
            .object_token = ecx_,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_refresh_calls;
    }

    LegacyBattleGridFrameCallReply
    invoke(const LegacyBattleGridFrameCallRequest& request) {
        const auto reply = port_.invoke_grid_frame(request);
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return reply;
    }

    void invoke_font(const Call call, const u32 value) {
        invoke({
            .call = call,
            .arguments = {value},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
        });
        ++result_.font_calls;
    }

    void invoke_text(
        const u32 x,
        const u32 y,
        const u32 text_token,
        const u32 color,
        const std::array<u8, 20>& text_bytes,
        const u32 text_length,
        const u32 call_eax,
        const u32 call_ecx,
        const u32 call_edx
    ) {
        invoke({
            .call = Call::draw_text,
            .arguments = {kTextSurfaceToken, x, y, text_token, color, 0x10U},
            .eax = call_eax,
            .ecx = call_ecx,
            .edx = call_edx,
            .text_token = text_token,
            .text_bytes = text_bytes,
            .text_length = text_length,
        });
        ++result_.text_draw_calls;
    }

    [[nodiscard]] LegacyBattleAlternateGridFrameResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleAlternateGridFrameState& state_;
    LegacyBattleAlternateGridFrameBindings bindings_;
    LegacyBattleGridFramePort& port_;
    const LegacyBattleAlternateGridFrameRequest& request_;
    LegacyBattleAlternateGridFrameResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleAlternateGridFrameResult draw_legacy_battle_alternate_grid_frame(
    LegacyBattleAlternateGridFrameState& state,
    const LegacyBattleAlternateGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleAlternateGridFrameRequest& request
) {
    return Executor(state, bindings, port, request).run();
}

}  // namespace openswd3::battle
