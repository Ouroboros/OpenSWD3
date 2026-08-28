#include "openswd3/battle/legacy_battle_guard_panel_frame.hpp"

#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u16;
using compat::u32;
using Call = LegacyBattleGridFrameCall;
using Reply = LegacyBattleGridFrameCallReply;
using Request = LegacyBattleGridFrameCallRequest;
using Status = LegacyBattleGuardPanelFrameStatus;

constexpr u32 kPanelActionId = 0x233BU;
constexpr u32 kGroupABaseToken = 0x005029D0U;
constexpr u32 kGroupACount = 10U;
constexpr u32 kTextObjectToken = 0x004C9A28U;
constexpr u32 kFontToken = 0x004CD76CU;

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_scaled_1007(const u32 index) noexcept {
    u32 scaled = (index << 6U) - index;
    scaled = (scaled << 4U) - index;
    return scaled;
}

[[nodiscard]] constexpr u32 group_a_scaled_3021(const u32 index) noexcept {
    const u32 scaled = group_a_scaled_1007(index);
    return scaled + scaled * 2U;
}

[[nodiscard]] constexpr u32 group_a_actor_token(const u32 index) noexcept {
    return kGroupABaseToken + group_a_scaled_3021(index) * 4U;
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low_word) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low_word);
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
        LegacyBattleGuardPanelFrameBindings bindings,
        LegacyBattleGridFramePort& port,
        const LegacyBattleGuardPanelFrameRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGuardPanelFrameResult run() {
        if (!draw_panel()) {
            return finish();
        }
        draw_title();
        if (!draw_body_frame()) {
            return finish();
        }
        if (!draw_selection()) {
            return finish();
        }

        u32 remaining = static_cast<u16>(bindings_.target_effect_value >> 16U);
        if (remaining != 0U) {
            const u32 total = remaining;
            do {
                invoke_font_width(0x12U);
                const u32 actor_index = bindings_.group_a_count - remaining;
                eax_ = group_a_scaled_1007(actor_index);
                ecx_ = group_a_actor_token(actor_index);
                edx_ = group_a_scaled_3021(actor_index);
                if (actor_index >= kGroupACount) {
                    result_.status = Status::group_a_actor_typed_stop;
                    return finish();
                }
                const auto label = invoke({
                    .call = Call::query_guard_actor_label,
                    .object_token = ecx_,
                    .eax = eax_,
                    .ecx = ecx_,
                    .edx = edx_,
                });
                ++result_.actor_label_query_calls;
                const u32 row_index = total - remaining;
                const u32 y = request_.origin_y + row_index * 0x16U + 8U;
                auto& trace = result_.rows[row_index];
                trace.actor_index = actor_index;
                trace.actor_token = group_a_actor_token(actor_index);
                trace.label_token = label.eax;
                trace.x = request_.origin_x + 0x0CU;
                trace.y = y;
                invoke_text(
                    trace.x,
                    y,
                    label.eax,
                    0xFFFFU,
                    0x10U,
                    y,
                    kTextObjectToken,
                    row_index * 0x0BU
                );
                ++result_.displayed_rows;
                --remaining;
            } while (remaining != 0U);
        }

        if (static_cast<u16>(bindings_.target_effect_value >> 16U) == 1U) {
            const u32 x = request_.origin_x + 0x0CU;
            const u32 y = request_.origin_y + 0x1EU;
            invoke_text(
                x,
                y,
                kLegacyBattleMissingGuardTextToken,
                0xFFFFU,
                0x10U,
                eax_,
                kTextObjectToken,
                kFontToken
            );
            result_.missing_row_drawn = true;
        }

        invoke_font_width(0x10U);
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleGuardPanelFrameResult finish() noexcept {
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

    void invoke_font_width(const u32 width) {
        static_cast<void>(invoke({
            .call = Call::configure_font_width,
            .object_token = kTextObjectToken,
            .arguments = {width},
            .eax = eax_,
            .ecx = kTextObjectToken,
            .edx = edx_,
        }));
        ++result_.font_width_calls;
    }

    void invoke_text(
        const u32 x,
        const u32 y,
        const u32 text_token,
        const u32 color,
        const u32 width,
        const u32 call_eax,
        const u32 call_ecx,
        const u32 call_edx
    ) {
        static_cast<void>(invoke({
            .call = Call::draw_text,
            .object_token = kTextObjectToken,
            .arguments = {kFontToken, x, y, text_token, color, width},
            .eax = call_eax,
            .ecx = call_ecx,
            .edx = call_edx,
            .text_token = text_token,
        }));
        ++result_.text_draw_calls;
    }

    [[nodiscard]] bool draw_panel() {
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
                    .x = 0xC4,
                    .y = 0xB0,
                    .width = 0xBC,
                    .height = 0x58,
                    .red = 0,
                    .green = 4,
                    .blue = 4,
                    .mode = 0U,
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

        eax_ = replace_low_word(eax_, bindings_.panel_action_record.field_4a);
        return draw_tiled_frame(
            0U,
            eax_,
            0xC8U,
            0xB4U,
            0x178U,
            0xC4U,
            Status::first_tiled_frame_typed_stop
        );
    }

    void draw_title() {
        invoke_text(
            0xFCU,
            0xB4U,
            kLegacyBattleGuardPanelTitleToken,
            0xFFC0U,
            0x10U,
            eax_,
            kTextObjectToken,
            edx_
        );
    }

    [[nodiscard]] bool draw_body_frame() {
        edx_ = replace_low_word(edx_, bindings_.panel_action_record.field_4a);
        return draw_tiled_frame(
            1U,
            edx_,
            0xC8U,
            0xD4U,
            0x178U,
            0x104U,
            Status::second_tiled_frame_typed_stop
        );
    }

    [[nodiscard]] bool draw_tiled_frame(
        const std::size_t index,
        const u32 resource_id,
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
                .resource_id = resource_id,
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

    [[nodiscard]] bool draw_selection() {
        const u32 selection = bindings_.group_b_row_selection;
        const u32 row_offset = selection + selection * 10U;
        const u32 x = request_.origin_x - 2U;
        const u32 y = request_.origin_y + row_offset * 2U - 0x12U;
        result_.selection_rectangle_status =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(x),
                    .y = wrapping_i32(y),
                    .width = 0xBC,
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
        return true;
    }

    LegacyBattleGuardPanelFrameBindings bindings_;
    LegacyBattleGridFramePort& port_;
    const LegacyBattleGuardPanelFrameRequest& request_;
    LegacyBattleGuardPanelFrameResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleGuardPanelFrameResult draw_legacy_battle_guard_panel_frame(
    LegacyBattleGuardPanelFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleGuardPanelFrameRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
