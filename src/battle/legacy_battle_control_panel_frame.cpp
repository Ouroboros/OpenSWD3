#include "openswd3/battle/legacy_battle_control_panel_frame.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u32;
using Call = LegacyBattleControlPanelFrameCall;
using Reply = LegacyBattleControlPanelFrameCallReply;
using Request = LegacyBattleControlPanelFrameCallRequest;
using Status = LegacyBattleControlPanelFrameStatus;

constexpr u32 kGroupBBaseToken = 0x00525508U;
constexpr u32 kGroupBStride = 0x2B28U;
constexpr u32 kGroupBCount = 8U;
constexpr u32 kNormalStyle = 0xFFFEU;
constexpr u32 kSelectedStyle = 0xF000U;
constexpr u32 kBorderColor = 0x48U;

[[nodiscard]] constexpr i32 wrapping_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_b_scaled_345(const u32 index) noexcept {
    u32 scaled = index + index * 2U;
    scaled <<= 3U;
    scaled -= index;
    scaled += scaled * 2U;
    return scaled + scaled * 4U;
}

[[nodiscard]] constexpr u32 group_b_actor_token(const u32 index) noexcept {
    return kGroupBBaseToken + index * kGroupBStride;
}

class Runner final {
public:
    Runner(
        LegacyBattleControlPanelFrameBindings bindings,
        LegacyBattleControlPanelFramePort& port,
        const LegacyBattleControlPanelFrameRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleControlPanelFrameResult run() {
        if (!draw_title_border()) {
            return finish();
        }
        draw_control_title();
        if (!draw_body_border()) {
            return finish();
        }
        draw_attack_row();
        if (!draw_primary_rows()) {
            return finish();
        }
        if (!draw_special_rows()) {
            return finish();
        }
        draw_release_row();
        configure_style(kNormalStyle);
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleControlPanelFrameResult finish() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        result_.final_text = load_text();
        return result_;
    }

    [[nodiscard]] Reply invoke(const Request& request) {
        const Reply reply = port_.invoke_control_panel_frame(request);
        ++result_.port_calls;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return reply;
    }

    [[nodiscard]] std::array<compat::u8, 24> load_text() const noexcept {
        std::array<compat::u8, 24> bytes{};
        for (std::size_t index = 0U;
             index < bindings_.selection_text_workspace.size();
             ++index) {
            const u32 word = bindings_.selection_text_workspace[index];
            bytes[index * 4U] = static_cast<compat::u8>(word);
            bytes[index * 4U + 1U] = static_cast<compat::u8>(word >> 8U);
            bytes[index * 4U + 2U] = static_cast<compat::u8>(word >> 16U);
            bytes[index * 4U + 3U] = static_cast<compat::u8>(word >> 24U);
        }
        return bytes;
    }

    void store_text(const std::array<compat::u8, 24>& bytes) noexcept {
        for (std::size_t index = 0U;
             index < bindings_.selection_text_workspace.size();
             ++index) {
            bindings_.selection_text_workspace[index] =
                static_cast<u32>(bytes[index * 4U]) |
                (static_cast<u32>(bytes[index * 4U + 1U]) << 8U) |
                (static_cast<u32>(bytes[index * 4U + 2U]) << 16U) |
                (static_cast<u32>(bytes[index * 4U + 3U]) << 24U);
        }
    }

    [[nodiscard]] std::span<compat::u8> text_bytes() const noexcept {
        return {
            reinterpret_cast<compat::u8*>(
                bindings_.selection_text_workspace.data()
            ),
            sizeof(compat::u32) * bindings_.selection_text_workspace.size()
        };
    }

    void clear_text() noexcept {
        std::ranges::fill(bindings_.selection_text_workspace, 0U);
    }

    [[nodiscard]] bool draw_title_border() {
        result_.borders[0U] = draw_legacy_battle_border_panel(
            bindings_.state.border_panel,
            bindings_.shared_color_fade,
            bindings_.framebuffer,
            bindings_.clip,
            bindings_.shared_request,
            bindings_.shared_effects,
            bindings_.jitter,
            bindings_.frame_provider,
            kLegacyBattleControlPanelResourceId,
            wrapping_i32(request_.origin_x - 8U),
            wrapping_i32(request_.origin_y - 0x28U),
            4,
            2,
            kBorderColor
        );
        ++result_.border_calls;
        publish_border_registers(0U);
        if (result_.borders[0U].status !=
            LegacyBattleBorderPanelStatus::completed) {
            result_.status = Status::title_border_typed_stop;
            return false;
        }
        return true;
    }

    void draw_control_title() {
        draw_text(
            request_.origin_x,
            request_.origin_y - 0x20U,
            kLegacyBattleControlPanelControlTextToken,
            {},
            eax_,
            kLegacyBattleControlPanelTextObjectToken,
            kLegacyBattleControlPanelFontToken
        );
    }

    [[nodiscard]] bool draw_body_border() {
        result_.borders[1U] = draw_legacy_battle_border_panel(
            bindings_.state.border_panel,
            bindings_.shared_color_fade,
            bindings_.framebuffer,
            bindings_.clip,
            bindings_.shared_request,
            bindings_.shared_effects,
            bindings_.jitter,
            bindings_.frame_provider,
            kLegacyBattleControlPanelResourceId,
            wrapping_i32(request_.origin_x - 8U),
            wrapping_i32(request_.origin_y - 8U),
            4,
            wrapping_i32(bindings_.alternate_selection_limit),
            kBorderColor
        );
        ++result_.border_calls;
        publish_border_registers(1U);
        if (result_.borders[1U].status !=
            LegacyBattleBorderPanelStatus::completed) {
            result_.status = Status::body_border_typed_stop;
            return false;
        }
        return true;
    }

    void publish_border_registers(const std::size_t index) noexcept {
        eax_ = request_.border_return_registers[index].eax;
        ecx_ = request_.border_return_registers[index].ecx;
        edx_ = request_.border_return_registers[index].edx;
    }

    void configure_reset(const u32 value) {
        static_cast<void>(invoke({
            .call = Call::configure_font_reset,
            .object_token = kLegacyBattleControlPanelTextObjectToken,
            .arguments = {value},
            .eax = eax_,
            .ecx = kLegacyBattleControlPanelTextObjectToken,
            .edx = edx_,
        }));
        ++result_.font_reset_calls;
    }

    void configure_style(const u32 value) {
        static_cast<void>(invoke({
            .call = Call::configure_font_style,
            .object_token = kLegacyBattleControlPanelTextObjectToken,
            .arguments = {value},
            .eax = eax_,
            .ecx = kLegacyBattleControlPanelTextObjectToken,
            .edx = edx_,
        }));
        ++result_.font_style_calls;
    }

    void draw_text(
        const u32 x,
        const u32 y,
        const u32 text_token,
        const std::array<compat::u8, 24>& bytes,
        const u32 call_eax,
        const u32 call_ecx,
        const u32 call_edx
    ) {
        static_cast<void>(invoke({
            .call = Call::draw_text,
            .object_token = kLegacyBattleControlPanelTextObjectToken,
            .arguments =
                {
                    kLegacyBattleControlPanelFontToken,
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
            .text_bytes = bytes,
        }));
        ++result_.text_draw_calls;
    }

    void trace_row(
        const u32 selected_index,
        const u32 source_index,
        const u32 text_token,
        const u32 y,
        const bool primary,
        const bool selected
    ) noexcept {
        if (result_.row_trace_count >= result_.rows.size()) {
            return;
        }
        result_.rows[result_.row_trace_count++] = {
            .selected_index = selected_index,
            .source_index = source_index,
            .text_token = text_token,
            .x = request_.origin_x,
            .y = y,
            .primary = primary,
            .selected = selected,
        };
    }

    void draw_attack_row() {
        configure_reset(0U);
        configure_style(kNormalStyle);
        draw_text(
            request_.origin_x,
            request_.origin_y,
            kLegacyBattleControlPanelAttackTextToken,
            {},
            eax_,
            kLegacyBattleControlPanelTextObjectToken,
            kLegacyBattleControlPanelFontToken
        );
        const bool selected = request_.selected_index == 1U;
        if (selected) {
            configure_style(kSelectedStyle);
            draw_text(
                request_.origin_x,
                request_.origin_y,
                kLegacyBattleControlPanelAttackTextToken,
                {},
                kLegacyBattleControlPanelFontToken,
                kLegacyBattleControlPanelTextObjectToken,
                edx_
            );
        }
        trace_row(
            1U,
            0U,
            kLegacyBattleControlPanelAttackTextToken,
            request_.origin_y,
            false,
            selected
        );
        visible_index_ = 1U;
        current_y_ = request_.origin_y;
    }

    void prepare_group_b_registers(const u32 source_index) noexcept {
        const i32 signed_index = static_cast<i32>(
            std::bit_cast<i16>(bindings_.selected_group_b_index)
        );
        const u32 index_bits = std::bit_cast<u32>(signed_index);
        eax_ = group_b_scaled_345(index_bits);
        ecx_ = group_b_actor_token(index_bits);
        edx_ = source_index;
    }

    [[nodiscard]] LegacyBattleActorGroupBElementState*
    primary_actor() const noexcept {
        const i32 signed_index = static_cast<i32>(
            std::bit_cast<i16>(bindings_.selected_group_b_index)
        );
        if (signed_index < 0 ||
            static_cast<std::size_t>(signed_index) >=
                bindings_.group_b_actors.size()) {
            return nullptr;
        }
        return &bindings_
                    .group_b_actors[static_cast<std::size_t>(signed_index)];
    }

    [[nodiscard]] bool prepare_group_b_query(const u32 source_index) {
        prepare_group_b_registers(source_index);
        const i32 signed_index = static_cast<i32>(
            std::bit_cast<i16>(bindings_.selected_group_b_index)
        );
        if (signed_index < 0 ||
            static_cast<u32>(signed_index) >= kGroupBCount) {
            result_.status = Status::group_b_actor_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool draw_primary_rows() {
        for (u32 source_index = 0U; source_index < 3U; ++source_index) {
            clear_text();
            prepare_group_b_registers(source_index);
            u32 primary_value{};
            result_.primary_options[source_index] =
                load_legacy_battle_group_b_action_item_option(
                    primary_actor(),
                    text_bytes(),
                    &primary_value,
                    port_,
                    {
                        .selector = source_index,
                        .actor_token = ecx_,
                        .text_destination_token =
                            kLegacyBattleControlPanelSharedTextToken,
                        .output_token = request_.local_primary_value_token,
                        .entry_eax = eax_,
                        .entry_ecx = ecx_,
                        .entry_edx = edx_,
                    }
                );
            ++result_.primary_query_calls;
            result_.port_calls +=
                result_.primary_options[source_index].definition_load_calls +
                result_.primary_options[source_index].name_copy_calls;
            eax_ = result_.primary_options[source_index].return_eax;
            ecx_ = result_.primary_options[source_index].return_ecx;
            edx_ = result_.primary_options[source_index].return_edx;
            if (result_.primary_options[source_index].status !=
                LegacyBattleGroupBActionItemOptionStatus::completed) {
                result_.status = result_.primary_options[source_index].status ==
                            LegacyBattleGroupBActionItemOptionStatus::
                                actor_state_typed_stop ||
                        result_.primary_options[source_index].status ==
                            LegacyBattleGroupBActionItemOptionStatus::
                                resource_read_typed_stop
                    ? Status::group_b_actor_typed_stop
                    : Status::primary_option_typed_stop;
                return false;
            }
            if (eax_ != 1U) {
                continue;
            }

            ++result_.primary_rows;
            ++result_.visible_option_rows;
            ++visible_index_;
            current_y_ += 0x14U;
            configure_style(kNormalStyle);
            const auto bytes = load_text();
            draw_text(
                request_.origin_x,
                current_y_,
                kLegacyBattleControlPanelSharedTextToken,
                bytes,
                eax_,
                kLegacyBattleControlPanelTextObjectToken,
                kLegacyBattleControlPanelFontToken
            );
            const bool selected = request_.selected_index == visible_index_;
            if (selected) {
                bindings_.transition_value_b = 0U;
                bindings_.transition_value_a = primary_value;
                configure_style(kSelectedStyle);
                draw_text(
                    request_.origin_x,
                    current_y_,
                    kLegacyBattleControlPanelSharedTextToken,
                    bytes,
                    eax_,
                    kLegacyBattleControlPanelTextObjectToken,
                    edx_
                );
            }
            trace_row(
                visible_index_,
                source_index,
                kLegacyBattleControlPanelSharedTextToken,
                current_y_,
                true,
                selected
            );
        }
        return true;
    }

    [[nodiscard]] bool draw_special_rows() {
        for (u32 source_index = 0U; source_index < 2U; ++source_index) {
            clear_text();
            if (!prepare_group_b_query(source_index)) {
                return false;
            }
            const auto reply = invoke({
                .call = Call::query_special_option,
                .object_token = ecx_,
                .arguments =
                    {source_index, kLegacyBattleControlPanelSharedTextToken},
                .eax = eax_,
                .ecx = ecx_,
                .edx = edx_,
            });
            ++result_.special_query_calls;
            if (reply.eax != 1U) {
                continue;
            }

            ++result_.special_rows;
            ++result_.visible_option_rows;
            ++visible_index_;
            clear_text();
            format_special(source_index + 1U);
            current_y_ += 0x14U;
            configure_style(kNormalStyle);
            const auto bytes = load_text();
            draw_text(
                request_.origin_x,
                current_y_,
                kLegacyBattleControlPanelSharedTextToken,
                bytes,
                kLegacyBattleControlPanelFontToken,
                kLegacyBattleControlPanelTextObjectToken,
                edx_
            );
            const bool selected = request_.selected_index == visible_index_;
            if (selected) {
                bindings_.transition_value_a = 0U;
                bindings_.transition_value_b = source_index + 1U;
                configure_style(kSelectedStyle);
                draw_text(
                    request_.origin_x,
                    current_y_,
                    kLegacyBattleControlPanelSharedTextToken,
                    bytes,
                    eax_,
                    kLegacyBattleControlPanelTextObjectToken,
                    kLegacyBattleControlPanelFontToken
                );
            }
            trace_row(
                visible_index_,
                source_index,
                kLegacyBattleControlPanelSharedTextToken,
                current_y_,
                false,
                selected
            );
        }
        return true;
    }

    void format_special(const u32 one_based_index) noexcept {
        std::array<compat::u8, 24> bytes{};
        std::copy(
            kLegacyBattleControlPanelSpecialPrefix.begin(),
            kLegacyBattleControlPanelSpecialPrefix.end(),
            bytes.begin()
        );
        bytes[4U] = static_cast<compat::u8>('0' + one_based_index);
        store_text(bytes);
        eax_ = 5U;
    }

    void draw_release_row() {
        configure_style(kNormalStyle);
        ++visible_index_;
        current_y_ += 0x14U;
        result_.release_selected_index = visible_index_;
        draw_text(
            request_.origin_x,
            current_y_,
            kLegacyBattleControlPanelReleaseTextToken,
            {},
            eax_,
            kLegacyBattleControlPanelTextObjectToken,
            kLegacyBattleControlPanelFontToken
        );
        const bool selected = request_.selected_index == visible_index_;
        if (selected) {
            configure_style(kSelectedStyle);
            draw_text(
                request_.origin_x,
                current_y_,
                kLegacyBattleControlPanelReleaseTextToken,
                {},
                kLegacyBattleControlPanelFontToken,
                kLegacyBattleControlPanelTextObjectToken,
                edx_
            );
        }
        trace_row(
            visible_index_,
            0U,
            kLegacyBattleControlPanelReleaseTextToken,
            current_y_,
            false,
            selected
        );
    }

    LegacyBattleControlPanelFrameBindings bindings_;
    LegacyBattleControlPanelFramePort& port_;
    const LegacyBattleControlPanelFrameRequest& request_;
    LegacyBattleControlPanelFrameResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u32 visible_index_{};
    u32 current_y_{};
};

}  // namespace

LegacyBattleControlPanelFrameResult draw_legacy_battle_control_panel_frame(
    LegacyBattleControlPanelFrameBindings bindings,
    LegacyBattleControlPanelFramePort& port,
    const LegacyBattleControlPanelFrameRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
