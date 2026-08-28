#include "openswd3/battle/legacy_battle_level_up_panel.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i8;
using compat::i32;
using compat::u8;
using compat::u32;

inline constexpr u32 kFramebufferToken = 0x004CD76CU;
inline constexpr u32 kTextColor = 0xFFC0U;
inline constexpr u32 kFontSize = 0x10U;

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

class Runner {
public:
    Runner(
        LegacyBattleLevelUpPanelBindings bindings,
        LegacyBattleVictoryRewardPort& port,
        const LegacyBattleLevelUpPanelRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleLevelUpPanelResult run() {
        initialize_local_text();
        eax_ = bindings_.target_selection.transition_actor_index;
        if (static_cast<u8>(eax_) != 0xFFU ||
            bindings_.target_selection.transition_mode == 1U) {
            draw_base_panel();
            if (result_.status != LegacyBattleLevelUpPanelStatus::completed) {
                return finish();
            }
        }

        query_and_draw_level_text();
        return finish();
    }

private:
    void initialize_local_text() noexcept {
        text_.fill(0U);
        eax_ = 0U;
        ecx_ = 0U;
    }

    [[nodiscard]] LegacyBattleVictoryRewardCallReply invoke(
        const LegacyBattleVictoryRewardCall call,
        const u32 actor_token = 0U,
        const std::array<u32, 6>& arguments = {},
        const std::span<const u8> text = {}
    ) {
        LegacyBattleVictoryRewardCallRequest request{
            .call = call,
            .actor_token = actor_token,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text_length = static_cast<u32>(text.size()),
        };
        std::copy(text.begin(), text.end(), request.text.begin());
        const auto reply = port_.invoke_victory_reward(request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_transition_actor_index) {
            bindings_.target_selection.transition_actor_index =
                reply.transition_actor_index;
        }
        return reply;
    }

    void draw_base_panel() {
        bindings_.victory.panel_action_record.action_id =
            kLegacyBattleVictoryPanelAction;
        bindings_.victory.panel_action_record.base_variant = 0U;
        result_.panel_action_update = bindings_.action_updater.update(
            bindings_.victory.panel_action_record
        );

        const i32 dynamic_height = std::bit_cast<i32>(
            bindings_.target_selection.transition_stage + 0x28U
        );
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.shared_effects.pixel_conversion,
            {
                .x = 0xC4,
                .y = 0xB0,
                .width = 0xBC,
                .height = dynamic_height,
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
                LegacyBattleLevelUpPanelStatus::rectangle_typed_stop;
            return;
        }

        edx_ = (edx_ & 0xFFFF0000U) |
            static_cast<u32>(bindings_.victory.panel_action_record.field_4a);
        result_.tiled_frames[0U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = edx_,
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
                LegacyBattleLevelUpPanelStatus::title_frame_typed_stop;
            return;
        }

        eax_ = kFramebufferToken;
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleVictoryRewardCall::draw_text,
            0U,
            {
                kFramebufferToken,
                0x108U,
                0xB4U,
                kLegacyBattleLevelUpTitleToken,
                kTextColor,
                kFontSize,
            },
            kLegacyBattleLevelUpTitle
        ));
        ++result_.text_draw_calls;

        ecx_ = bindings_.target_selection.transition_stage + 0xD4U;
        edx_ = (edx_ & 0xFFFF0000U) |
            static_cast<u32>(bindings_.victory.panel_action_record.field_4a);
        result_.tiled_frames[1U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = edx_,
                .left = 0xC8,
                .top = 0xD4,
                .right = 0x178,
                .bottom = std::bit_cast<i32>(ecx_),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.shared_effects,
            bindings_.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.summary_frame_return.eax;
        ecx_ = request_.summary_frame_return.ecx;
        edx_ = request_.summary_frame_return.edx;
        if (result_.tiled_frames[1U].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleLevelUpPanelStatus::summary_frame_typed_stop;
        }
    }

    void query_and_draw_level_text() {
        static_cast<void>(invoke(
            LegacyBattleVictoryRewardCall::query_summary_panel,
            0U,
            {3U, 0xF4U, 0xD4U}
        ));
        if (eax_ != 1U) {
            return;
        }

        eax_ = (eax_ & 0xFFFFFF00U) |
            bindings_.target_selection.transition_actor_index;
        if (bindings_.target_selection.transition_actor_index == 0xFFU) {
            return;
        }

        const i32 actor_index = static_cast<i32>(
            static_cast<i8>(bindings_.target_selection.transition_actor_index)
        );
        eax_ = std::bit_cast<u32>(actor_index);
        edx_ = 0U;
        if (actor_index < 0 ||
            static_cast<std::size_t>(actor_index) >=
                bindings_.startup.action_mode_source.actor_label_indices
                    .size()) {
            result_.status =
                LegacyBattleLevelUpPanelStatus::actor_index_typed_stop;
            return;
        }

        const u32 label =
            bindings_.startup.action_mode_source
                .actor_label_indices[static_cast<std::size_t>(actor_index)];
        eax_ = label;
        ecx_ = label * 7U;
        eax_ <<= 4U;
        if (label >= bindings_.party_member_resources.size()) {
            result_.status = LegacyBattleLevelUpPanelStatus::
                party_member_resource_typed_stop;
            return;
        }
        const u32 level = static_cast<u8>(
            bindings_.party_member_resources[label].fields_10_to_1e[6U]
        );
        edx_ = level;
        const u32 name_token = kLegacyBattleLevelUpNameBaseToken + eax_;
        eax_ = request_.local_text_token;
        const auto formatted = invoke(
            LegacyBattleVictoryRewardCall::format_level_up_text,
            0U,
            {
                request_.local_text_token,
                kLegacyBattleLevelUpFormatToken,
                name_token,
                level,
            }
        );
        if (formatted.formatted_text_length >= text_.size()) {
            result_.status =
                LegacyBattleLevelUpPanelStatus::format_buffer_typed_stop;
            return;
        }
        const auto length =
            static_cast<std::size_t>(formatted.formatted_text_length);
        std::copy_n(formatted.formatted_text.begin(), length, text_.begin());
        text_[length] = 0U;
        result_.formatted_text = text_;
        result_.formatted_text_length = formatted.formatted_text_length;

        edx_ = kFramebufferToken;
        ecx_ = kLegacyBattleVictoryFontToken;
        static_cast<void>(invoke(
            LegacyBattleVictoryRewardCall::draw_text,
            0U,
            {
                kFramebufferToken,
                0xD0U,
                0xDCU,
                request_.local_text_token,
                kTextColor,
                kFontSize,
            },
            std::span<const u8>(text_.data(), length)
        ));
        ++result_.text_draw_calls;
    }

    [[nodiscard]] LegacyBattleLevelUpPanelResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleLevelUpPanelBindings bindings_;
    LegacyBattleVictoryRewardPort& port_;
    const LegacyBattleLevelUpPanelRequest& request_;
    LegacyBattleLevelUpPanelResult result_{};
    std::array<u8, 64> text_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleLevelUpPanelResult draw_legacy_battle_level_up_panel(
    LegacyBattleLevelUpPanelBindings bindings,
    LegacyBattleVictoryRewardPort& port,
    const LegacyBattleLevelUpPanelRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
