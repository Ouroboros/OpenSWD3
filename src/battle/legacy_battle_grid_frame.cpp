#include "openswd3/battle/legacy_battle_grid_frame.hpp"

#include "openswd3/special_modes/legacy_standard_mode.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <ranges>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;
using Call = LegacyBattleGridFrameCall;
using Status = LegacyBattleGridFrameStatus;

inline constexpr u32 kActionId = 0x2394U;
inline constexpr u32 kPanelActionId = 0x233BU;
inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kFontToken = 0x004C9A28U;
inline constexpr u32 kTextSurfaceToken = 0x004CD76CU;
inline constexpr u32 kPanelRowLimitToken = 0x0053BDF4U;
inline constexpr u32 kSharedTextToken = 0x004FC2A0U;

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
        LegacyBattleGridFrameState& state,
        const LegacyBattleGridFrameBindings bindings,
        LegacyBattleGridFramePort& port,
        const LegacyBattleGridFrameRequest& request
    )
        : state_(state), bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGridFrameResult run() {
        state_.row_text.fill(0U);
        state_.numeric_text.fill(0U);
        state_.row_text[0U] = 0xFFU;
        state_.row_flags = 0U;
        state_.row_value = 0U;
        state_.numeric_text_length = 0U;
        edx_ = 0U;
        eax_ = bindings_.queued_actor_code;
        ecx_ = 0U;
        if (eax_ == 0U) {
            return finish();
        }

        if (!draw_header()) {
            return finish();
        }
        if (!draw_panel()) {
            return finish();
        }

        invoke_font(Call::configure_font_mode, 0U);
        invoke_font(Call::configure_font_style, 0xFFFEU);
        invoke_font(Call::configure_font_width, 0x10U);

        bindings_.panel_row_limit = 0U;
        const u32 actor_code = bindings_.queued_actor_code;
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        if (!valid_group_a_code(actor_code)) {
            result_.status = Status::group_a_actor_typed_stop;
            return finish();
        }

        const auto initialized = invoke({
            .call = Call::initialize_rows,
            .object_token = ecx_,
            .arguments = {bindings_.action_category_index, kPanelRowLimitToken},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_initialization_calls;
        if (initialized.publish_panel_row_limit) {
            bindings_.panel_row_limit = initialized.panel_row_limit;
        }

        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_actor_token(actor_code);
        invoke({
            .call = Call::refresh_actor,
            .object_token = ecx_,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_refresh_calls;

        u32 iterator = request_.scroll_offset + 1U;
        u32 displayed_rows = 0U;
        for (;;) {
            eax_ = group_a_scaled_1007(actor_code);
            ecx_ = group_a_actor_token(actor_code);
            edx_ = actor_code;
            const auto queried = invoke({
                .call = Call::query_row,
                .object_token = ecx_,
                .arguments =
                    {
                        bindings_.action_category_index,
                        iterator,
                        request_.row_text_token,
                        request_.row_flags_token,
                        request_.row_value_token,
                    },
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
                .text_token = request_.row_text_token,
                .text_bytes = state_.row_text,
            });
            ++result_.row_query_calls;
            if (queried.publish_row_flags) {
                state_.row_flags = queried.row_flags;
            }
            if (queried.publish_row_value) {
                state_.row_value = queried.row_value;
            }
            if (queried.publish_row_text) {
                state_.row_text = queried.row_text;
            }
            if (eax_ == 0U) {
                eax_ = group_a_scaled_1007(actor_code);
                ecx_ = group_a_actor_token(actor_code);
                invoke({
                    .call = Call::refresh_actor,
                    .object_token = ecx_,
                    .eax = eax_,
                    .ecx = ecx_,
                    .edx = edx_,
                });
                ++result_.actor_refresh_calls;
                break;
            }

            ++result_.scanned_rows;
            if ((state_.row_flags & 0x00008000U) == 0U) {
                ++result_.hidden_rows;
                ++iterator;
                result_.final_iterator = iterator;
                continue;
            }

            auto& trace =
                result_.rows[static_cast<std::size_t>(displayed_rows)];
            trace.iterator = iterator;
            trace.flags = state_.row_flags;
            trace.value = state_.row_value;
            trace.displayed_row = displayed_rows + 1U;
            trace.row_text = state_.row_text;

            state_.numeric_text_length = format_signed_width_two(
                state_.numeric_text,
                std::bit_cast<i16>(static_cast<u16>(state_.row_value))
            );
            trace.numeric_text = state_.numeric_text;
            trace.numeric_text_length = state_.numeric_text_length;
            eax_ = state_.numeric_text_length;
            const auto& format_registers =
                request_.format_return_registers[static_cast<std::size_t>(
                    displayed_rows
                )];
            ecx_ = format_registers.ecx;
            edx_ = format_registers.edx;

            const u32 row_y = request_.origin_y + displayed_rows * 20U;
            u32 color{};
            trace.secondary_color = (state_.row_flags & 0x00004000U) != 0U;
            if (trace.secondary_color) {
                ecx_ = replace_low_word(ecx_, bindings_.secondary_text_color);
                color = ecx_;
            } else {
                ecx_ = replace_low_word(ecx_, bindings_.primary_text_color);
                color = ecx_;
            }
            eax_ = request_.origin_x + 0x90U;
            ecx_ = kFontToken;
            edx_ = request_.numeric_text_token;
            invoke_text(
                request_.origin_x + 0x90U,
                row_y + 0x2CU,
                request_.numeric_text_token,
                color,
                state_.numeric_text,
                state_.numeric_text_length
            );

            edx_ = replace_low_word(
                edx_,
                trace.secondary_color ? bindings_.secondary_text_color
                                      : bindings_.primary_text_color
            );
            color = edx_;
            eax_ = request_.origin_x + 0x10U;
            ecx_ = kFontToken;
            edx_ = kTextSurfaceToken;
            invoke_text(
                request_.origin_x + 0x10U,
                row_y + 0x2CU,
                request_.row_text_token,
                color,
                state_.row_text,
                0U
            );

            eax_ = displayed_rows + 1U;
            ecx_ = request_.selected_row;
            if (ecx_ == eax_) {
                trace.selected = true;
                if (!draw_selection(
                        actor_code, iterator, displayed_rows, row_y, trace
                    )) {
                    return finish();
                }
            }

            invoke_font(Call::configure_font_style, 0xFFFEU);
            ++displayed_rows;
            result_.displayed_rows = displayed_rows;
            ++iterator;
            result_.final_iterator = iterator;
            if (static_cast<u16>(displayed_rows) >= 7U) {
                break;
            }
        }

        invoke_font(Call::configure_font_style, 0xFFFEU);
        return finish();
    }

private:
    [[nodiscard]] bool draw_header() {
        const u32 row_y = request_.origin_y + 0x20U;
        u32 row_x = request_.origin_x + 0x7EU;
        auto& action_state = port_.battle_offset_action_frame_draw_state();
        for (u32 index = 0U; index < 4U; ++index) {
            const u32 variant = 3U - index;
            result_.action_frames[index] =
                draw_legacy_battle_offset_action_frame(
                    action_state,
                    bindings_.framebuffer,
                    bindings_.clip,
                    bindings_.shared_request,
                    bindings_.shared_effects,
                    bindings_.jitter,
                    bindings_.action_updater,
                    bindings_.frame_provider,
                    kActionId,
                    variant,
                    wrapping_i32(row_x),
                    wrapping_i32(row_y),
                    0U,
                    request_.action_update_edx_snapshots[index]
                );
            ++result_.action_frame_calls;
            publish_action_frame_registers(index);
            if (result_.action_frames[index].status !=
                LegacyBattleOffsetActionFrameDrawStatus::completed) {
                result_.status = Status::action_frame_typed_stop;
                return false;
            }
            row_x -= 0x2AU;
        }

        const u32 category = bindings_.action_category_index;
        result_.action_frames[4U] = draw_legacy_battle_offset_action_frame(
            action_state,
            bindings_.framebuffer,
            bindings_.clip,
            bindings_.shared_request,
            bindings_.shared_effects,
            bindings_.jitter,
            bindings_.action_updater,
            bindings_.frame_provider,
            kActionId,
            category + 4U,
            wrapping_i32(request_.origin_x + category * 0x2AU),
            wrapping_i32(row_y),
            0U,
            request_.action_update_edx_snapshots[4U]
        );
        ++result_.action_frame_calls;
        publish_action_frame_registers(4U);
        if (result_.action_frames[4U].status !=
            LegacyBattleOffsetActionFrameDrawStatus::completed) {
            result_.status = Status::action_frame_typed_stop;
            return false;
        }

        invoke_font(Call::configure_font_style, 0xF000U);
        invoke_font(Call::configure_font_style, 0xFFFEU);
        return true;
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
                    .y = wrapping_i32(request_.origin_y + 0x24U),
                    .width = 0xCC,
                    .height = 0x98,
                    .red = 0,
                    .green = 4,
                    .blue = 4,
                    .mode = 2U,
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

        ecx_ = replace_low_word(ecx_, bindings_.panel_action_record.field_4a);
        result_.tiled_frame = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = ecx_,
                .left = wrapping_i32(request_.origin_x + 6U),
                .top = wrapping_i32(request_.origin_y + 0x28U),
                .right = wrapping_i32(request_.origin_x + 0xC8U),
                .bottom = wrapping_i32(request_.origin_y + 0xB8U),
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

    [[nodiscard]] bool draw_selection(
        const u32 actor_code,
        const u32 iterator,
        const u32 displayed_rows,
        const u32 row_y,
        LegacyBattleGridFrameRowTrace& trace
    ) {
        u32 color{};
        if (trace.secondary_color) {
            ecx_ = replace_low_word(
                request_.selected_row, bindings_.secondary_text_color
            );
            color = ecx_;
        } else {
            edx_ = replace_low_word(edx_, bindings_.primary_text_color);
            color = edx_;
        }
        eax_ = request_.origin_x + 0x0FU;
        ecx_ = kFontToken;
        edx_ = kTextSurfaceToken;
        invoke_text(
            request_.origin_x + 0x0FU,
            row_y + 0x2BU,
            request_.row_text_token,
            color,
            state_.row_text,
            0U
        );
        invoke_font(Call::configure_font_style, 0xFFFEU);

        result_.selection_rectangle_statuses[0U] =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = wrapping_i32(request_.origin_x),
                    .y = wrapping_i32(
                        request_.origin_y + displayed_rows * 20U + 0x28U
                    ),
                    .width = 0xCE,
                    .height = 0x18,
                    .red = 0x1F,
                    .green = 0x14,
                    .blue = 0,
                    .mode = 5U,
                }
            );
        ++result_.selection_rectangle_calls;
        if (!selection_rectangle_completed(
                result_.selection_rectangle_statuses[0U]
            )) {
            result_.status = Status::first_selection_rectangle_typed_stop;
            return false;
        }

        bindings_.target_argument = iterator;
        bindings_.selection_input_gate = 1U;
        result_.selected_iterator = iterator;
        result_.selection_rectangle_statuses[1U] =
            rendering::apply_legacy_rectangle_effect(
                bindings_.framebuffer,
                bindings_.raster,
                bindings_.shared_effects.pixel_conversion,
                {
                    .x = 0,
                    .y = 0x15E,
                    .width = 0x280,
                    .height = 0x28,
                    .red = 0,
                    .green = 5,
                    .blue = 0x1F,
                    .mode = 5U,
                }
            );
        ++result_.selection_rectangle_calls;
        if (!selection_rectangle_completed(
                result_.selection_rectangle_statuses[1U]
            )) {
            result_.status = Status::second_selection_rectangle_typed_stop;
            return false;
        }

        const std::size_t actor_index =
            static_cast<std::size_t>(actor_code - 8U);
        const u32 description_record_token =
            bindings_.actor_description_record_tokens[actor_index];
        const u16 description_text_index =
            bindings_.actor_description_text_indices[actor_index];
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = group_a_scaled_3021(actor_code);
        edx_ = description_record_token;
        if (bindings_.shared_text.size() <
            special_modes::kLegacyStandardModeSharedTextCapacity) {
            result_.status = Status::shared_text_typed_stop;
            return false;
        }
        const std::
            span<u8, special_modes::kLegacyStandardModeSharedTextCapacity>
                shared_text{
                    bindings_.shared_text.data(),
                    special_modes::kLegacyStandardModeSharedTextCapacity,
                };
        const auto resolved =
            special_modes::resolve_legacy_standard_mode_shared_text(
                description_text_index, bindings_.maps_payload, shared_text
            );
        ++result_.shared_text_resolution_calls;
        if (resolved.status !=
            special_modes::LegacyStandardModeTextResolutionStatus::completed) {
            result_.status = Status::shared_text_typed_stop;
            return false;
        }
        const auto terminator =
            std::ranges::find(shared_text, static_cast<u8>(0U));
        eax_ = static_cast<u32>(terminator - shared_text.begin());
        ++result_.shared_text_length_calls;
        const u32 centered =
            0x140U - (static_cast<u32>(wrapping_i32(eax_) >> 1) << 3U);
        eax_ = centered;
        ecx_ = kFontToken;
        edx_ = kTextSurfaceToken;
        invoke_text(centered, 0x168U, kSharedTextToken, 0xFFFFU, {}, 0U);
        return true;
    }

    void publish_action_frame_registers(const u32 index) {
        const auto& registers = request_.action_frame_return_registers[index];
        eax_ = registers.eax;
        ecx_ = registers.ecx;
        edx_ = registers.edx;
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
        const u32 text_length
    ) {
        invoke({
            .call = Call::draw_text,
            .arguments = {kTextSurfaceToken, x, y, text_token, color, 0x10U},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
            .text_token = text_token,
            .text_bytes = text_bytes,
            .text_length = text_length,
        });
        ++result_.text_draw_calls;
    }

    [[nodiscard]] LegacyBattleGridFrameResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleGridFrameState& state_;
    LegacyBattleGridFrameBindings bindings_;
    LegacyBattleGridFramePort& port_;
    const LegacyBattleGridFrameRequest& request_;
    LegacyBattleGridFrameResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleGridFrameResult draw_legacy_battle_grid_frame(
    LegacyBattleGridFrameState& state,
    const LegacyBattleGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleGridFrameRequest& request
) {
    return Executor(state, bindings, port, request).run();
}

}  // namespace openswd3::battle
