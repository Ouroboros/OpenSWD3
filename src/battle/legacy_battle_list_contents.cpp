#include "openswd3/battle/legacy_battle_list_contents.hpp"

#include "openswd3/special_modes/legacy_standard_mode.hpp"

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
using Call = LegacyBattleListContentsCall;
using Status = LegacyBattleListContentsStatus;

inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kFontToken = 0x004C9A28U;
inline constexpr u32 kTextSurfaceToken = 0x004CD76CU;
inline constexpr u32 kSelectionWorkspaceToken = 0x0053C184U;
inline constexpr u32 kPanelRowLimitToken = 0x0053BDF2U;
inline constexpr u32 kSharedTextToken = 0x004FC2A0U;

[[nodiscard]] constexpr i32 signed_dword(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr i16 signed_word(const u32 value) noexcept {
    return std::bit_cast<i16>(static_cast<u16>(value));
}

[[nodiscard]] constexpr u32 bits(const i32 value) noexcept {
    return std::bit_cast<u32>(value);
}

[[nodiscard]] constexpr i32 from_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u32 low) noexcept {
    return (value & 0xFFFF0000U) | (low & 0xFFFFU);
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

[[nodiscard]] constexpr bool accepted_rectangle_status(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out;
}

[[nodiscard]] u32 format_signed_width_three(
    std::array<u8, 10>& destination, const i16 value
) noexcept {
    destination.fill(0U);
    std::array<char, 8> digits{};
    const auto converted = std::to_chars(
        digits.data(), digits.data() + digits.size(), static_cast<i32>(value)
    );
    const std::size_t digit_count =
        static_cast<std::size_t>(converted.ptr - digits.data());
    const std::size_t padding = digit_count < 3U ? 3U - digit_count : 0U;
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
        LegacyBattleListContentsState& state,
        const LegacyBattleListContentsBindings bindings,
        LegacyBattleListContentsPort& port,
        const LegacyBattleListContentsRequest& request
    )
        : state_(state), bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleListContentsResult run() {
        ecx_ = 0U;
        eax_ = bindings_.queued_actor_code;
        state_.numeric_text.fill(0U);
        state_.numeric_text_length = 0U;
        state_.local_limit_word = request_.initial_limit_word;
        state_.local_limit_byte = request_.initial_limit_byte;
        if (eax_ == 0U) {
            return finish();
        }

        invoke({
            .call = Call::configure_font_mode,
            .arguments = {0U},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
        });
        invoke({
            .call = Call::configure_font_style,
            .arguments = {0xFFFEU},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
        });
        invoke({
            .call = Call::configure_font_width,
            .arguments = {0x10U},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
        });

        bindings_.panel_row_limit = 0U;
        const u32 actor_code = bindings_.queued_actor_code;
        const u32 actor_token = group_a_actor_token(actor_code);
        eax_ = group_a_scaled_1007(actor_code);
        ecx_ = actor_token;
        edx_ = bindings_.action_category_index;
        if (!valid_group_a_code(actor_code)) {
            result_.status = Status::group_a_actor_typed_stop;
            return finish();
        }

        const auto initialized = invoke({
            .call = Call::initialize_rows,
            .object_token = actor_token,
            .arguments =
                {bindings_.action_category_index, 0U, kPanelRowLimitToken},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_initialization_calls;
        if (initialized.publish_panel_row_limit) {
            bindings_.panel_row_limit = initialized.panel_row_limit;
        }

        eax_ = group_a_scaled_3021(actor_code);
        ecx_ = actor_token;
        edx_ = actor_code;
        invoke({
            .call = Call::refresh_actor,
            .object_token = actor_token,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.actor_refresh_calls;

        u32 iterator = request_.scroll_offset + 1U;
        u32 row_count = 0U;
        u32 esi = 0U;
        for (;;) {
            auto& trace = result_.rows[static_cast<std::size_t>(row_count)];
            trace.iterator = iterator;
            u32 row_value = 0U;

            eax_ = group_a_scaled_3021(actor_code);
            ecx_ = actor_token;
            edx_ = actor_code;
            const auto queried = invoke({
                .call = Call::query_row,
                .object_token = actor_token,
                .arguments =
                    {
                        bindings_.action_category_index,
                        0U,
                        iterator,
                        kSelectionWorkspaceToken,
                        request_.local_value_token,
                    },
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
            });
            ++result_.row_query_calls;
            if (queried.publish_row_value) {
                row_value = queried.row_value;
            }
            const u32 query_return = queried.eax;
            trace.query_return = query_return;
            trace.raw_value = row_value;
            eax_ = row_value;
            if (static_cast<u16>(row_value) == 0xFFFFU) {
                eax_ = group_a_scaled_1007(actor_code);
                edx_ = group_a_scaled_3021(actor_code);
                ecx_ = actor_token;
                invoke({
                    .call = Call::refresh_actor,
                    .object_token = actor_token,
                    .eax = eax_,
                    .ecx = ecx_,
                    .edx = edx_,
                });
                ++result_.actor_refresh_calls;
                result_.final_iterator = iterator;
                break;
            }

            trace.negative_selector = (row_value & 0x8000U) != 0U;
            if (trace.negative_selector) {
                eax_ = group_a_scaled_1007(actor_code);
                ecx_ = actor_token;
                edx_ = request_.local_limit_byte_token;
                const auto resolved = invoke({
                    .call = Call::resolve_negative_row,
                    .object_token = actor_token,
                    .arguments =
                        {
                            request_.local_limit_byte_token,
                            request_.local_limit_word_token,
                        },
                    .eax = eax_,
                    .ecx = ecx_,
                    .edx = edx_,
                });
                publish_limits(resolved);
                row_value &= 0x7FFFU;
            } else {
                eax_ = group_a_scaled_1007(actor_code);
                edx_ = group_a_scaled_3021(actor_code);
                ecx_ = actor_token;
                const auto resolved = invoke({
                    .call = Call::resolve_regular_row,
                    .object_token = actor_token,
                    .arguments =
                        {
                            request_.local_limit_byte_token,
                            request_.local_limit_word_token,
                        },
                    .eax = eax_,
                    .ecx = ecx_,
                    .edx = edx_,
                });
                publish_limits(resolved);
            }
            ++result_.row_resolver_calls;
            trace.displayed_value = row_value;
            trace.limit_word = state_.local_limit_word;
            trace.limit_byte = state_.local_limit_byte;

            if (static_cast<u16>(query_return) != 0xFFFFU) {
                const u32 frame_index = query_return - 1U;
                const i32 draw_x = from_bits(request_.origin_x - 4U);
                const u32 row_offset = (esi & 0xFFFFU) * 20U;
                const i32 draw_y =
                    from_bits(request_.origin_y + row_offset + 0x25U);
                auto& frame_result =
                    result_
                        .resource_frames[static_cast<std::size_t>(row_count)];
                frame_result = draw_legacy_battle_resource_frame(
                    state_.resource_frame,
                    bindings_.framebuffer,
                    bindings_.clip,
                    bindings_.shared_request,
                    bindings_.shared_effects,
                    bindings_.jitter,
                    bindings_.frame_provider,
                    0x241CU,
                    frame_index,
                    draw_x,
                    draw_y
                );
                ++result_.resource_frame_calls;
                trace.resource_drawn = true;
                const auto& registers =
                    request_.resource_frame_return_registers
                        [static_cast<std::size_t>(row_count)];
                eax_ = registers.eax;
                ecx_ = registers.ecx;
                edx_ = registers.edx;
                if (frame_result.status !=
                    LegacyBattleFrameDrawStatus::completed) {
                    result_.status = Status::resource_frame_typed_stop;
                    return finish();
                }
            }

            esi &= 0xFFFFU;
            const u32 row_offset = esi * 20U;
            const u32 row_y = request_.origin_y + row_offset;
            trace.limit_is_less =
                signed_word(state_.local_limit_word) < signed_word(row_value);
            u32 color{};
            if (trace.limit_is_less) {
                eax_ = replace_low_word(eax_, bindings_.secondary_text_color);
                color = eax_;
                ecx_ = kFontToken;
                edx_ = kTextSurfaceToken;
            } else {
                edx_ = replace_low_word(edx_, bindings_.primary_text_color);
                color = edx_;
                eax_ = request_.origin_x + 0x0CU;
                ecx_ = kFontToken;
            }
            invoke_text(
                request_.origin_x + 0x0CU,
                row_y + 0x25U,
                kSelectionWorkspaceToken,
                color,
                {},
                0U
            );

            state_.numeric_text_length = format_signed_width_three(
                state_.numeric_text, signed_word(row_value)
            );
            trace.numeric_text = state_.numeric_text;
            trace.numeric_text_length = state_.numeric_text_length;
            const auto& format_registers =
                request_.format_return_registers[static_cast<std::size_t>(
                    row_count
                )];
            eax_ = state_.numeric_text_length;
            ecx_ = format_registers.ecx;
            edx_ = format_registers.edx;
            if (trace.limit_is_less) {
                ecx_ = replace_low_word(ecx_, bindings_.secondary_text_color);
                color = ecx_;
                eax_ = request_.origin_x + 0x90U;
                edx_ = kTextSurfaceToken;
            } else {
                eax_ = replace_low_word(eax_, bindings_.primary_text_color);
                color = eax_;
                eax_ = kTextSurfaceToken;
                edx_ = request_.origin_x + 0x90U;
            }
            invoke_text(
                request_.origin_x + 0x90U,
                row_y + 0x25U,
                request_.numeric_text_token,
                color,
                state_.numeric_text,
                state_.numeric_text_length
            );

            eax_ = request_.selected_row;
            ++esi;
            if (eax_ == esi) {
                trace.selected = true;
                edx_ = replace_low_word(edx_, state_.local_limit_word);
                if (trace.limit_is_less) {
                    ecx_ =
                        replace_low_word(ecx_, bindings_.secondary_text_color);
                    color = ecx_;
                    eax_ = request_.origin_x + 0x0BU;
                    edx_ = row_y + 0x24U;
                } else {
                    eax_ = replace_low_word(
                        request_.selected_row, bindings_.primary_text_color
                    );
                    color = eax_;
                    eax_ = kTextSurfaceToken;
                    edx_ = request_.origin_x + 0x0BU;
                }
                invoke_text(
                    request_.origin_x + 0x0BU,
                    row_y + 0x24U,
                    kSelectionWorkspaceToken,
                    color,
                    {},
                    0U
                );

                result_.rectangle_statuses[0U] =
                    rendering::apply_legacy_rectangle_effect(
                        bindings_.framebuffer,
                        bindings_.raster,
                        bindings_.shared_effects.pixel_conversion,
                        {
                            .x = from_bits(request_.origin_x - 8U),
                            .y = from_bits(row_y + 0x21U),
                            .width = 0xC0,
                            .height = 0x18,
                            .red = 0x1F,
                            .green = 0x14,
                            .blue = 0,
                            .mode = 5U,
                        }
                    );
                ++result_.rectangle_calls;
                if (!accepted_rectangle_status(
                        result_.rectangle_statuses[0U]
                    )) {
                    result_.status = Status::first_rectangle_typed_stop;
                    return finish();
                }

                result_.rectangle_statuses[1U] =
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
                ++result_.rectangle_calls;
                if (!accepted_rectangle_status(
                        result_.rectangle_statuses[1U]
                    )) {
                    result_.status = Status::second_rectangle_typed_stop;
                    return finish();
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
                    return finish();
                }
                const std::span<
                    u8,
                    special_modes::kLegacyStandardModeSharedTextCapacity>
                    shared_text{
                        bindings_.shared_text.data(),
                        special_modes::kLegacyStandardModeSharedTextCapacity,
                    };
                const auto resolved =
                    special_modes::resolve_legacy_standard_mode_shared_text(
                        description_text_index,
                        bindings_.maps_payload,
                        shared_text
                    );
                ++result_.shared_text_resolution_calls;
                if (resolved.status !=
                    special_modes::LegacyStandardModeTextResolutionStatus::
                        completed) {
                    result_.status = Status::shared_text_typed_stop;
                    return finish();
                }
                const auto terminator =
                    std::ranges::find(shared_text, static_cast<u8>(0U));
                eax_ = static_cast<u32>(terminator - shared_text.begin());
                ++result_.shared_text_length_calls;
                const u32 first_half = bits(signed_dword(eax_) >> 1);
                u32 centered = first_half << 4U;
                centered = bits(signed_dword(centered) >> 1);
                centered = 0x140U - centered;
                eax_ = centered;
                ecx_ = kFontToken;
                edx_ = kTextSurfaceToken;
                invoke_text(
                    centered, 0x168U, kSharedTextToken, 0xFFFFU, {}, 0U
                );
                invoke({
                    .call = Call::configure_font_style,
                    .arguments = {0xFFFEU},
                    .eax = eax_,
                    .ecx = kFontToken,
                    .edx = edx_,
                });
                bindings_.selection_input_gate = 1U;
                bindings_.candidate_argument = iterator;
                result_.selected_iterator = iterator;
            }

            eax_ = row_count;
            edx_ = iterator;
            ++eax_;
            ++edx_;
            row_count = eax_;
            iterator = edx_;
            ++result_.completed_rows;
            result_.final_iterator = iterator;
            if (static_cast<u16>(row_count) >= 7U) {
                break;
            }
            esi = row_count;
        }

        invoke({
            .call = Call::configure_font_style,
            .arguments = {0xFFFEU},
            .eax = eax_,
            .ecx = kFontToken,
            .edx = edx_,
        });
        return finish();
    }

private:
    LegacyBattleListContentsCallReply
    invoke(const LegacyBattleListContentsCallRequest& request) {
        const auto reply = port_.invoke_list_contents(request);
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return reply;
    }

    void publish_limits(const LegacyBattleListContentsCallReply& reply) {
        if (reply.publish_limit_word) {
            state_.local_limit_word = reply.limit_word;
        }
        if (reply.publish_limit_byte) {
            state_.local_limit_byte = reply.limit_byte;
        }
    }

    void invoke_text(
        const u32 x,
        const u32 y,
        const u32 text_token,
        const u32 color,
        const std::array<u8, 10>& text_bytes,
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

    [[nodiscard]] LegacyBattleListContentsResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleListContentsState& state_;
    LegacyBattleListContentsBindings bindings_;
    LegacyBattleListContentsPort& port_;
    const LegacyBattleListContentsRequest& request_;
    LegacyBattleListContentsResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleListContentsResult draw_legacy_battle_list_contents(
    LegacyBattleListContentsState& state,
    const LegacyBattleListContentsBindings bindings,
    LegacyBattleListContentsPort& port,
    const LegacyBattleListContentsRequest& request
) {
    return Executor(state, bindings, port, request).run();
}

}  // namespace openswd3::battle
