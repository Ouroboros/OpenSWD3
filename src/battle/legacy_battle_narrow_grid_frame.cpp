#include "openswd3/battle/legacy_battle_narrow_grid_frame.hpp"

#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using Call = LegacyBattleGridFrameCall;
using Reply = LegacyBattleGridFrameCallReply;
using Request = LegacyBattleGridFrameCallRequest;
using Status = LegacyBattleNarrowGridFrameStatus;

constexpr u32 kPanelActionId = 0x233BU;
constexpr u32 kGroupABaseToken = 0x005029D0U;
constexpr u32 kGroupACount = 10U;
constexpr u32 kPanelRowLimitToken = 0x0053BDF3U;
constexpr u32 kTextSurfaceToken = 0x004C9A28U;
constexpr u32 kFontToken = 0x004CD76CU;

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

[[nodiscard]] constexpr bool valid_group_a_actor_code(const u32 code) noexcept {
    return code >= 8U && code - 8U < kGroupACount;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
}

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
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

class Runner final {
public:
    Runner(
        LegacyBattleNarrowGridFrameState& state,
        LegacyBattleNarrowGridFrameBindings bindings,
        LegacyBattleGridFramePort& port,
        const LegacyBattleNarrowGridFrameRequest& request
    ) noexcept
        : state_(state), bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleNarrowGridFrameResult run() {
        state_.display_count = 0U;
        state_.row_value = 0U;

        eax_ = bindings_.queued_actor_code;
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
        eax_ = group_a_scaled_3021(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        edx_ = actor_code;
        if (!valid_group_a_actor_code(actor_code)) {
            result_.status = Status::group_a_actor_typed_stop;
            return finish();
        }

        const auto initialized = invoke({
            .call = Call::initialize_narrow_rows,
            .object_token = ecx_,
            .arguments = {0U, 1U, kPanelRowLimitToken},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_initialization_calls;
        if (initialized.publish_panel_row_limit) {
            bindings_.panel_row_limit =
                static_cast<u8>(initialized.panel_row_limit);
        }
        refresh_actor(actor_code);

        u32 iterator = 1U;
        for (;;) {
            result_.final_iterator = iterator;
            eax_ = group_a_scaled_3021(actor_code);
            ecx_ = group_a_actor_token(actor_code);
            edx_ = actor_code;
            const auto queried = invoke({
                .call = Call::query_narrow_row,
                .object_token = ecx_,
                .arguments =
                    {
                        0U,
                        1U,
                        iterator,
                        request_.row_text_token,
                        request_.row_value_token,
                    },
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
                .text_token = request_.row_text_token,
                .text_bytes = load_workspace_text(),
            });
            ++result_.row_query_calls;
            if (queried.publish_row_value) {
                state_.row_value = queried.row_value;
            }
            if (queried.publish_row_text) {
                store_workspace_text(queried.row_text);
            }

            eax_ = state_.row_value;
            const u16 row_value = static_cast<u16>(state_.row_value);
            if (row_value == 0xFFFFU) {
                refresh_actor(actor_code);
                state_.row_value = 0U;
                break;
            }
            if (row_value != 0U && !draw_row(iterator)) {
                return finish();
            }

            ecx_ = iterator;
            state_.row_value = 0U;
            ++ecx_;
            iterator = ecx_;
            result_.final_iterator = iterator;
            if (static_cast<u16>(state_.display_count) >= 7U) {
                break;
            }
        }

        invoke_font(Call::configure_font_style, 0xFFFEU);
        result_.displayed_rows = state_.display_count;
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleNarrowGridFrameResult finish() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    [[nodiscard]] Reply invoke(const Request& request) {
        const Reply reply = port_.invoke_grid_frame(request);
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return reply;
    }

    void invoke_font(const Call call, const u32 argument) {
        static_cast<void>(invoke({
            .call = call,
            .object_token = kTextSurfaceToken,
            .arguments = {argument},
            .eax = eax_,
            .ecx = kTextSurfaceToken,
            .edx = edx_,
        }));
        ++result_.font_calls;
    }

    void invoke_text(
        const u32 x,
        const u32 y,
        const u32 color,
        const u32 text_token,
        const std::array<u8, 20>& text,
        const u32 call_eax,
        const u32 call_ecx,
        const u32 call_edx
    ) {
        static_cast<void>(invoke({
            .call = Call::draw_text,
            .object_token = kTextSurfaceToken,
            .arguments = {kFontToken, x, y, text_token, color, 0x10U},
            .eax = call_eax,
            .ecx = call_ecx,
            .edx = call_edx,
            .text_token = text_token,
            .text_bytes = text,
        }));
        ++result_.text_draw_calls;
    }

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
                    .height = 0xAC,
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
            request_.origin_y + 0xA8U,
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
        const u32 title_x = request_.origin_x + 0x50U;
        invoke_text(
            title_x,
            request_.origin_y + 8U,
            0xFFC0U,
            kLegacyBattleNarrowGridTitleToken,
            {},
            title_x,
            kTextSurfaceToken,
            edx_
        );
    }

    void refresh_actor(const u32 actor_code) {
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        edx_ = group_a_scaled_3021(actor_code);
        static_cast<void>(invoke({
            .call = Call::refresh_actor,
            .object_token = ecx_,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        }));
        ++result_.actor_refresh_calls;
    }

    [[nodiscard]] bool draw_row(const u32 iterator) {
        const u32 display_index = static_cast<u16>(state_.display_count);
        const u32 base_y = request_.origin_y + display_index * 0x16U;
        auto& trace = result_.rows[display_index];
        trace.iterator = iterator;
        trace.row_value = state_.row_value;
        trace.x = request_.origin_x + 0x10U;
        trace.y = base_y + 0x28U;
        trace.row_text = load_workspace_text();

        eax_ = replace_low_word(eax_, bindings_.primary_text_color);
        invoke_text(
            trace.x,
            trace.y,
            eax_,
            request_.row_text_token,
            trace.row_text,
            kFontToken,
            kTextSurfaceToken,
            trace.x
        );

        eax_ = request_.selected_row;
        const u32 selected_index = display_index + 1U;
        if (eax_ == selected_index) {
            trace.selected = true;
            ecx_ = replace_low_word(ecx_, bindings_.primary_text_color);
            const u32 selected_x = request_.origin_x + 0x0FU;
            const u32 selected_y = base_y + 0x27U;
            invoke_text(
                selected_x,
                selected_y,
                ecx_,
                request_.row_text_token,
                trace.row_text,
                selected_x,
                kTextSurfaceToken,
                selected_y
            );

            result_.selection_rectangle_status =
                rendering::apply_legacy_rectangle_effect(
                    bindings_.framebuffer,
                    bindings_.raster,
                    bindings_.shared_effects.pixel_conversion,
                    {
                        .x = wrapping_i32(request_.origin_x),
                        .y = wrapping_i32(base_y + 0x25U),
                        .width = 0xC0,
                        .height = 0x18,
                        .red = 31,
                        .green = 20,
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

            invoke_font(Call::configure_font_style, 0xFFFEU);
            bindings_.selection_input_gate = 1U;
            bindings_.candidate_argument = iterator;
        }

        ++state_.display_count;
        result_.displayed_rows = state_.display_count;
        return true;
    }

    [[nodiscard]] std::array<u8, 20> load_workspace_text() const noexcept {
        std::array<u8, 20> text{};
        for (std::size_t word_index = 0U;
             word_index < bindings_.selection_workspace.size();
             ++word_index) {
            const u32 word = bindings_.selection_workspace[word_index];
            for (std::size_t byte_index = 0U; byte_index < 4U; ++byte_index) {
                text[word_index * 4U + byte_index] =
                    static_cast<u8>(word >> static_cast<u32>(byte_index * 8U));
            }
        }
        return text;
    }

    void store_workspace_text(const std::array<u8, 20>& text) noexcept {
        for (std::size_t word_index = 0U;
             word_index < bindings_.selection_workspace.size();
             ++word_index) {
            u32 word = 0U;
            for (std::size_t byte_index = 0U; byte_index < 4U; ++byte_index) {
                word |= static_cast<u32>(text[word_index * 4U + byte_index])
                    << static_cast<u32>(byte_index * 8U);
            }
            bindings_.selection_workspace[word_index] = word;
        }
    }

    LegacyBattleNarrowGridFrameState& state_;
    LegacyBattleNarrowGridFrameBindings bindings_;
    LegacyBattleGridFramePort& port_;
    const LegacyBattleNarrowGridFrameRequest& request_;
    LegacyBattleNarrowGridFrameResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleNarrowGridFrameResult draw_legacy_battle_narrow_grid_frame(
    LegacyBattleNarrowGridFrameState& state,
    LegacyBattleNarrowGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleNarrowGridFrameRequest& request
) {
    return Runner(state, bindings, port, request).run();
}

}  // namespace openswd3::battle
