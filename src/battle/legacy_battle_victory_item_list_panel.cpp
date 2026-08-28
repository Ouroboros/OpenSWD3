#include "openswd3/battle/legacy_battle_victory_item_list_panel.hpp"

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

constexpr std::array<u8, 6U> kVictoryItemListTitle{
    0xBEU, 0xD4U, 0xA7U, 0x51U, 0xABU, 0x7EU
};

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

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

class VictoryItemListPanelRunner final {
public:
    VictoryItemListPanelRunner(
        LegacyBattleVictoryItemListPanelBindings bindings,
        LegacyBattleVictoryItemListPanelPort& port,
        const LegacyBattleVictoryItemListPanelRequest& request
    )
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleVictoryItemListPanelResult run() {
        initialize_local_text();
        draw_panels();
        if (result_.status !=
            LegacyBattleVictoryItemListPanelStatus::completed) {
            return finish();
        }

        query_and_draw_rows();
        if (result_.status !=
            LegacyBattleVictoryItemListPanelStatus::completed) {
            return finish();
        }

        set_font_size(0x10U);
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleVictoryItemListPanelCallReply invoke(
        const LegacyBattleVictoryItemListPanelCall call,
        const u32 object_token = 0U,
        const std::array<u32, 8U> arguments = {},
        const u32 item_name_token = 0U,
        const u16 item_quantity = 0U,
        const std::span<const u8> text = {}
    ) {
        LegacyBattleVictoryItemListPanelCallRequest call_request{
            .call = call,
            .object_token = object_token,
            .arguments = arguments,
            .item_name_token = item_name_token,
            .item_quantity = item_quantity,
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

        const auto reply = port_.invoke_victory_item_list_panel(call_request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_item_count) {
            bindings_.target_selection.transition_sample_word =
                reply.item_count;
        }
        return reply;
    }

    void initialize_local_text() noexcept {
        local_text_.fill(0U);
        local_text_[0U] = request_.local_text_seed;
        result_.local_text = local_text_;
    }

    void set_font_size(const u32 size) {
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleVictoryItemListPanelCall::set_font_size,
            kLegacyBattleVictoryFontToken,
            {kLegacyBattleVictoryFontToken, size}
        ));
        ++result_.font_size_calls;
    }

    void draw_panels() {
        const u32 item_count =
            static_cast<u32>(bindings_.target_selection.transition_sample_word);
        result_.initial_item_count = item_count;
        result_.panel_bottom = 0xD4U + item_count * 0x14U;

        eax_ = item_count;
        set_font_size(0x12U);

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

        edx_ = bindings_.target_selection.transition_stage + 0x28U;
        result_.rectangle_height = std::bit_cast<i32>(edx_);
        result_.rectangle_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.shared_effects.pixel_conversion,
            {
                .x = 0xC4,
                .y = 0xB0,
                .width = 0xB8,
                .height = std::bit_cast<i32>(edx_),
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
                LegacyBattleVictoryItemListPanelStatus::rectangle_typed_stop;
            return;
        }

        eax_ = replace_low_word(
            eax_, bindings_.victory.panel_action_record.field_4a
        );
        result_.first_frame_resource = eax_;
        result_.first_frame_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.tiled_frames[0U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = eax_,
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
                LegacyBattleVictoryItemListPanelStatus::title_frame_typed_stop;
            return;
        }

        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleVictoryItemListPanelCall::draw_title,
            kLegacyBattleVictoryFontToken,
            {
                kLegacyBattleVictoryItemListFramebufferToken,
                0x108U,
                0xB4U,
                kLegacyBattleVictoryItemListTitleToken,
                0xFFC0U,
                0x10U,
            },
            0U,
            0U,
            kVictoryItemListTitle
        ));
        ++result_.title_draw_calls;

        edx_ = bindings_.target_selection.transition_stage + 0xD4U;
        result_.list_frame_bottom = std::bit_cast<i32>(edx_);
        eax_ = replace_low_word(
            eax_, bindings_.victory.panel_action_record.field_4a
        );
        result_.second_frame_resource = eax_;
        result_.second_frame_entry = {.eax = eax_, .ecx = ecx_, .edx = edx_};
        result_.tiled_frames[1U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = eax_,
                .left = 0xC8,
                .top = 0xD4,
                .right = 0x178,
                .bottom = std::bit_cast<i32>(edx_),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.list_frame_return.eax;
        ecx_ = request_.list_frame_return.ecx;
        edx_ = request_.list_frame_return.edx;
        if (result_.tiled_frames[1U].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleVictoryItemListPanelStatus::list_frame_typed_stop;
        }
    }

    void query_and_draw_rows() {
        static_cast<void>(invoke(
            LegacyBattleVictoryItemListPanelCall::query_panel,
            0U,
            {0xD4U, result_.panel_bottom, 3U}
        ));
        ++result_.query_calls;
        if (eax_ != 1U ||
            bindings_.target_selection.transition_sample_word == 0U) {
            return;
        }

        u32 item_index = 0U;
        while (true) {
            if (!draw_item_row(item_index)) {
                return;
            }

            eax_ = 0U;
            ++item_index;
            eax_ = static_cast<u32>(
                bindings_.target_selection.transition_sample_word
            );
            if (as_i32(item_index) >= as_i32(eax_)) {
                break;
            }
        }
    }

    [[nodiscard]] bool draw_item_row(const u32 item_index) {
        result_.stopped_item_index = item_index;
        if (item_index >= bindings_.victory.player_item_tokens.size() ||
            item_index >= bindings_.victory.collected_item_quantities.size()) {
            result_.status =
                LegacyBattleVictoryItemListPanelStatus::item_row_typed_stop;
            return false;
        }

        const u32 name_token = bindings_.victory.player_item_tokens[item_index];
        const u16 quantity =
            bindings_.victory.collected_item_quantities[item_index];
        edx_ = name_token;
        ecx_ = static_cast<u32>(quantity);
        eax_ = request_.local_text_token;
        const auto formatted = invoke(
            LegacyBattleVictoryItemListPanelCall::format_item_row,
            0U,
            {
                request_.local_text_token,
                kLegacyBattleVictoryItemListFormatToken,
                name_token,
                static_cast<u32>(quantity),
            },
            name_token,
            quantity,
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
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
            result_.status = LegacyBattleVictoryItemListPanelStatus::
                format_buffer_typed_stop;
            return false;
        }
        local_text_[copied] = 0U;
        local_text_length_ = length;
        result_.local_text = local_text_;
        result_.local_text_length = length;

        edx_ = kLegacyBattleVictoryItemListFramebufferToken;
        ecx_ = request_.local_text_token;
        static_cast<void>(invoke(
            LegacyBattleVictoryItemListPanelCall::draw_item_row,
            kLegacyBattleVictoryFontToken,
            {
                kLegacyBattleVictoryItemListFramebufferToken,
                0xD2U,
                0xD4U + item_index * 0x14U,
                request_.local_text_token,
                0xFFC0U,
                0x10U,
            },
            name_token,
            quantity,
            std::span<const u8>(local_text_.data(), copied)
        ));
        ++result_.item_draw_calls;
        ++result_.item_rows_drawn;
        return true;
    }

    [[nodiscard]] LegacyBattleVictoryItemListPanelResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        result_.local_text = local_text_;
        result_.local_text_length = local_text_length_;
        return result_;
    }

    LegacyBattleVictoryItemListPanelBindings bindings_;
    LegacyBattleVictoryItemListPanelPort& port_;
    const LegacyBattleVictoryItemListPanelRequest& request_;
    LegacyBattleVictoryItemListPanelResult result_{};
    std::array<u8, 64U> local_text_{};
    u32 local_text_length_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleVictoryItemListPanelResult
draw_legacy_battle_victory_item_list_panel(
    const LegacyBattleVictoryItemListPanelBindings bindings,
    LegacyBattleVictoryItemListPanelPort& port,
    const LegacyBattleVictoryItemListPanelRequest& request
) {
    return VictoryItemListPanelRunner(bindings, port, request).run();
}

}  // namespace openswd3::battle
