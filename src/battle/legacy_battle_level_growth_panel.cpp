#include "openswd3/battle/legacy_battle_level_growth_panel.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cstddef>
#include <span>
#include <string_view>

namespace openswd3::battle {
namespace {

using compat::i8;
using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

inline constexpr u32 kFramebufferToken = 0x004CD76CU;
inline constexpr u32 kFontToken = 0x004C9A28U;
inline constexpr u32 kNameColor = 0xFFC0U;
inline constexpr u32 kLabelColor = 0xFFFFU;
inline constexpr u32 kGrowthColor = 0xF000U;
inline constexpr u32 kFontSize = 0x10U;
inline constexpr std::array<i32, 7U> kTextY{
    0x78,
    0x8C,
    0xA0,
    0xB4,
    0xC8,
    0xDC,
    0xF0,
};
inline constexpr std::array<std::array<u8, 7U>, 7U> kLabelPrefixes{
    std::array<u8, 7U>{0xA5U, 0xCDU, 0xA9U, 0x52U, 0xA4U, 0x4FU, 0x3AU},
    std::array<u8, 7U>{0xC5U, 0x5DU, 0xAAU, 0x6BU, 0xA4U, 0x4FU, 0x3AU},
    std::array<u8, 7U>{0xC5U, 0xE9U, 0x20U, 0x20U, 0xA4U, 0x4FU, 0x3AU},
    std::array<u8, 7U>{0xA4U, 0x4FU, 0x20U, 0x20U, 0xB6U, 0x71U, 0x3AU},
    std::array<u8, 7U>{0xADU, 0x40U, 0x20U, 0x20U, 0xA4U, 0x4FU, 0x3AU},
    std::array<u8, 7U>{0xB4U, 0xBCU, 0x20U, 0x20U, 0xBCU, 0x7AU, 0x3AU},
    std::array<u8, 7U>{0xB1U, 0xD3U, 0x20U, 0x20U, 0xB1U, 0xB6U, 0x3AU},
};
inline constexpr std::array<u8, 5U> kArrowText{
    0x20U,
    0x3DU,
    0x3DU,
    0x3EU,
    0x20U,
};

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

[[nodiscard]] constexpr u16 baseline_value(
    const world_map::LegacyWorldStoryPartyMemberResources& profile,
    const std::size_t index
) noexcept {
    switch (index) {
    case 0U:
        return profile.limit_first;
    case 1U:
        return profile.limit_second;
    case 2U:
        return profile.limit_third;
    default:
        return profile.fields_10_to_1e[index - 3U];
    }
}

[[nodiscard]] constexpr u16& growth_delta(
    LegacyBattleLevelAdvancementState& state, const std::size_t index
) noexcept {
    if (index < 3U) {
        return state.growth_delta_primary[index];
    }
    return state.growth_delta_secondary[index - 3U];
}

[[nodiscard]] std::array<u8, 64U> format_decimal(
    const std::span<const u8> prefix, const i32 value, u32& length
) noexcept {
    std::array<u8, 64U> text{};
    std::array<char, 16U> digits{};
    const auto converted =
        std::to_chars(digits.data(), digits.data() + digits.size(), value);
    const auto digit_count =
        static_cast<std::size_t>(converted.ptr - digits.data());
    const auto padding = digit_count < 4U ? 4U - digit_count : 0U;
    std::size_t cursor = 0U;
    for (const auto byte : prefix) {
        text[cursor++] = byte;
    }
    for (std::size_t index = 0U; index < padding; ++index) {
        text[cursor++] = 0x20U;
    }
    for (std::size_t index = 0U; index < digit_count; ++index) {
        text[cursor++] = static_cast<u8>(digits[index]);
    }
    length = static_cast<u32>(cursor);
    return text;
}

class Runner {
public:
    Runner(
        LegacyBattleLevelGrowthPanelBindings bindings,
        LegacyBattleLevelGrowthPanelPort& port,
        const LegacyBattleLevelGrowthPanelRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleLevelGrowthPanelResult run() {
        local_text_.fill(0U);
        local_text_[0U] = 0xFFU;
        if (bindings_.victory.target_selection.transition_actor_index ==
            0xFFU) {
            return finish();
        }

        draw_panel_frame();
        if (result_.status != LegacyBattleLevelGrowthPanelStatus::completed) {
            return finish();
        }

        query_panel_and_draw_baseline();
        if (result_.status != LegacyBattleLevelGrowthPanelStatus::completed ||
            !panel_visible_) {
            return finish();
        }

        advance_stage();
        return finish();
    }

private:
    [[nodiscard]] LegacyBattleLevelGrowthPanelCallReply invoke(
        const LegacyBattleLevelGrowthPanelCall call,
        const std::array<u32, 6U>& arguments = {},
        const std::span<const u8> text = {}
    ) {
        LegacyBattleLevelGrowthPanelCallRequest request{
            .call = call,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text_length = static_cast<u32>(text.size()),
        };
        std::copy(text.begin(), text.end(), request.text.begin());
        const auto reply = port_.invoke_level_growth_panel(request);
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

    void draw_panel_frame() {
        auto& action = bindings_.victory.state.panel_action_record;
        action.action_id = kLegacyBattleVictoryPanelAction;
        action.base_variant = 0U;
        result_.panel_action_update =
            bindings_.victory.action_updater.update(action);

        const i32 dynamic_height = std::bit_cast<i32>(
            bindings_.victory.target_selection.transition_stage + 0x28U
        );
        result_.rectangle_status = rendering::apply_legacy_rectangle_effect(
            bindings_.victory.framebuffer,
            bindings_.victory.raster,
            bindings_.victory.shared_effects.pixel_conversion,
            {
                .x = 0xC4,
                .y = 0x4C,
                .width = 0xD4,
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
                LegacyBattleLevelGrowthPanelStatus::rectangle_typed_stop;
            return;
        }

        edx_ = replace_low_word(edx_, action.field_4a);
        result_.tiled_frames[0U] = rendering::draw_legacy_tiled_frame(
            bindings_.victory.framebuffer,
            bindings_.victory.raster,
            bindings_.victory.frame_provider,
            {
                .resource_id = edx_,
                .left = 0xC8,
                .top = 0x50,
                .right = 0x190,
                .bottom = 0x60,
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.victory.shared_effects,
            bindings_.victory.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.title_frame_return.eax;
        ecx_ = request_.title_frame_return.ecx;
        edx_ = request_.title_frame_return.edx;
        if (result_.tiled_frames[0U].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::title_frame_typed_stop;
            return;
        }

        const auto actor = static_cast<i32>(static_cast<i8>(
            bindings_.victory.target_selection.transition_actor_index
        ));
        eax_ = std::bit_cast<u32>(actor);
        edx_ = kFramebufferToken;
        if (actor < 0 ||
            static_cast<std::size_t>(actor) >=
                bindings_.victory.startup.action_mode_source.actor_label_indices
                    .size()) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::actor_index_typed_stop;
            return;
        }
        const u32 label =
            bindings_.victory.startup.action_mode_source
                .actor_label_indices[static_cast<std::size_t>(actor)];
        ecx_ = label << 4U;
        ecx_ += kLegacyBattleLevelGrowthNameBaseToken;
        ecx_ = kFontToken;
        static_cast<void>(invoke(
            LegacyBattleLevelGrowthPanelCall::draw_text,
            {
                kFramebufferToken,
                0x118U,
                0x50U,
                kLegacyBattleLevelGrowthNameBaseToken + (label << 4U),
                kNameColor,
                kFontSize,
            }
        ));
        ++result_.text_draw_calls;

        eax_ = bindings_.victory.target_selection.transition_stage;
        ecx_ = replace_low_word(ecx_, action.field_4a);
        result_.tiled_frames[1U] = rendering::draw_legacy_tiled_frame(
            bindings_.victory.framebuffer,
            bindings_.victory.raster,
            bindings_.victory.frame_provider,
            {
                .resource_id = ecx_,
                .left = 0xC8,
                .top = 0x70,
                .right = 0x190,
                .bottom = std::bit_cast<i32>(
                    bindings_.victory.target_selection.transition_stage + 0x70U
                ),
                .opacity_step = 0,
                .flags = 0x80000008U,
            },
            bindings_.victory.shared_effects,
            bindings_.victory.jitter
        );
        ++result_.tiled_frame_calls;
        eax_ = request_.summary_frame_return.eax;
        ecx_ = request_.summary_frame_return.ecx;
        edx_ = request_.summary_frame_return.edx;
        if (result_.tiled_frames[1U].status !=
            rendering::LegacyTiledFrameStatus::completed) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::summary_frame_typed_stop;
        }
    }

    void query_panel_and_draw_baseline() {
        result_.transition_stage = advance_legacy_battle_transition_stage(
            bindings_.victory.target_selection.transition_stage,
            {.base_offset = 0x70U, .target = 0x10CU, .divisor = 2U}
        );
        ++result_.transition_stage_calls;
        eax_ = result_.transition_stage.return_eax;
        ecx_ = result_.transition_stage.return_ecx;
        edx_ = result_.transition_stage.return_edx;
        if (result_.transition_stage.status !=
            LegacyBattleTransitionStageAdvanceStatus::completed) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::transition_stage_typed_stop;
            return;
        }
        if (eax_ != 1U) {
            return;
        }
        panel_visible_ = true;

        for (std::size_t index = 0U; index < 7U; ++index) {
            const u16 raw = baseline_value(
                bindings_.advancement.profile_copy_scratch, index
            );
            const i32 value = index < 3U
                ? static_cast<i32>(static_cast<i16>(raw))
                : static_cast<i32>(raw);
            prepare_baseline_format_registers(index, raw, value);
            if (!format_into_local(
                    value,
                    kLegacyBattleLevelGrowthLabelFormatTokens[index],
                    kLabelPrefixes[index]
                )) {
                return;
            }
            prepare_baseline_draw_registers(index);
            draw_local_text(0xD8, kTextY[index], kLabelColor);
        }
    }

    void prepare_baseline_format_registers(
        const std::size_t index, const u16 raw, const i32 value
    ) noexcept {
        switch (index) {
        case 0U:
            edx_ = std::bit_cast<u32>(value);
            eax_ = request_.local_text_token;
            break;
        case 1U:
            eax_ = std::bit_cast<u32>(value);
            ecx_ = request_.local_text_token;
            break;
        case 2U:
            ecx_ = std::bit_cast<u32>(value);
            edx_ = request_.local_text_token;
            break;
        case 3U:
            edx_ = raw;
            eax_ = request_.local_text_token;
            break;
        case 4U:
            eax_ = raw;
            ecx_ = request_.local_text_token;
            break;
        case 5U:
            ecx_ = raw;
            edx_ = request_.local_text_token;
            break;
        default:
            edx_ = raw;
            eax_ = request_.local_text_token;
            break;
        }
    }

    void prepare_baseline_draw_registers(const std::size_t index) noexcept {
        switch (index) {
        case 1U:
        case 4U:
            eax_ = kFramebufferToken;
            edx_ = request_.local_text_token;
            break;
        case 2U:
        case 5U:
            eax_ = request_.local_text_token;
            break;
        case 0U:
        case 3U:
        case 6U:
        default:
            edx_ = kFramebufferToken;
            break;
        }
        ecx_ = kFontToken;
    }

    [[nodiscard]] bool format_into_local(
        const i32 value,
        const u32 format_token,
        const std::span<const u8> prefix
    ) {
        u32 fallback_length = 0U;
        const auto fallback = format_decimal(prefix, value, fallback_length);
        const auto reply = invoke(
            LegacyBattleLevelGrowthPanelCall::format_integer,
            {
                request_.local_text_token,
                format_token,
                std::bit_cast<u32>(value),
            },
            std::span<const u8>(fallback.data(), fallback_length)
        );
        ++result_.format_calls;
        const auto& text =
            reply.publish_formatted_text ? reply.formatted_text : fallback;
        const u32 length = reply.publish_formatted_text
            ? reply.formatted_text_length
            : fallback_length;
        if (length >= local_text_.size()) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::format_buffer_typed_stop;
            return false;
        }
        local_text_.fill(0U);
        std::copy_n(
            text.begin(), static_cast<std::size_t>(length), local_text_.begin()
        );
        local_text_[static_cast<std::size_t>(length)] = 0U;
        local_text_length_ = length;
        return true;
    }

    void draw_local_text(const i32 x, const i32 y, const u32 color) {
        static_cast<void>(invoke(
            LegacyBattleLevelGrowthPanelCall::draw_text,
            {
                kFramebufferToken,
                static_cast<u32>(x),
                static_cast<u32>(y),
                request_.local_text_token,
                color,
                kFontSize,
            },
            std::span<const u8>(
                local_text_.data(), static_cast<std::size_t>(local_text_length_)
            )
        ));
        ++result_.text_draw_calls;
    }

    void advance_stage() {
        auto& stage = bindings_.victory.input_dispatch.target_transition_word;
        edx_ = replace_low_word(edx_, stage);
        const u16 baseline_first =
            baseline_value(bindings_.advancement.profile_copy_scratch, 0U);

        if (stage == 100U) {
            eax_ = 0U;
            ecx_ = 0U;
            bindings_.advancement.growth_delta_primary.fill(0U);
            bindings_.advancement.growth_delta_secondary.fill(0U);
            draw_growth_values(baseline_first);
            return;
        }

        if (stage == 29U) {
            if (!initialize_growth_deltas()) {
                return;
            }
        } else if (stage >= 30U) {
            draw_growth_values(baseline_first);
            return;
        }

        stage = static_cast<u16>(stage + 1U);
        edx_ = replace_low_word(edx_, stage);
        if (stage < 30U) {
            return;
        }
        draw_growth_values(baseline_first);
    }

    [[nodiscard]] bool initialize_growth_deltas() {
        const auto profile = live_profile(true);
        if (profile == nullptr) {
            return false;
        }
        for (std::size_t index = 0U; index < 9U; ++index) {
            growth_delta(bindings_.advancement, index) = static_cast<u16>(
                baseline_value(*profile, index) -
                baseline_value(
                    bindings_.advancement.profile_copy_scratch, index
                )
            );
        }
        ecx_ = replace_low_word(
            ecx_, bindings_.advancement.growth_delta_secondary[4U]
        );
        eax_ = replace_low_word(
            eax_, bindings_.advancement.growth_delta_secondary[5U]
        );
        return true;
    }

    [[nodiscard]] const world_map::LegacyWorldStoryPartyMemberResources*
    live_profile(const bool delta_initialization = false) {
        const i32 actor = static_cast<i32>(static_cast<i8>(
            bindings_.victory.target_selection.transition_actor_index
        ));
        eax_ = std::bit_cast<u32>(actor);
        if (actor < 0 ||
            static_cast<std::size_t>(actor) >=
                bindings_.victory.startup.action_mode_source.actor_label_indices
                    .size()) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::actor_index_typed_stop;
            return nullptr;
        }
        const u32 label =
            bindings_.victory.startup.action_mode_source
                .actor_label_indices[static_cast<std::size_t>(actor)];
        if (delta_initialization) {
            ecx_ = label;
            eax_ = label * 56U;
        } else {
            eax_ = label;
            edx_ = label * 7U;
        }
        if (label >= bindings_.victory.party_member_resources.size()) {
            result_.status = LegacyBattleLevelGrowthPanelStatus::
                party_member_resource_typed_stop;
            return nullptr;
        }
        return &bindings_.victory.party_member_resources[label];
    }

    void draw_growth_values(const u16 baseline_first) {
        for (std::size_t index = 0U; index < 7U; ++index) {
            const u16 baseline = index == 0U
                ? baseline_first
                : baseline_value(
                      bindings_.advancement.profile_copy_scratch, index
                  );
            const auto profile = live_growth_profile(index, baseline);
            if (profile == nullptr) {
                return;
            }
            const u16 current = baseline_value(*profile, index);
            if (index >= 2U && index <= 5U) {
                edx_ = replace_low_word(edx_, current);
            }
            const bool increased = index < 3U
                ? static_cast<i16>(current) > static_cast<i16>(baseline)
                : current > baseline;
            if (!increased) {
                continue;
            }

            prepare_arrow_registers(index);
            static_cast<void>(invoke(
                LegacyBattleLevelGrowthPanelCall::draw_text,
                {
                    kFramebufferToken,
                    0x136U,
                    static_cast<u32>(kTextY[index]),
                    kLegacyBattleLevelGrowthArrowToken,
                    kLabelColor,
                    kFontSize,
                },
                kArrowText
            ));
            ++result_.text_draw_calls;

            const u16 delta = growth_delta(bindings_.advancement, index);
            const i32 display = index < 3U
                ? static_cast<i32>(static_cast<i16>(current)) -
                    static_cast<i32>(static_cast<i16>(delta))
                : static_cast<i32>(current) - static_cast<i32>(delta);
            ecx_ = index < 3U
                ? std::bit_cast<u32>(static_cast<i32>(static_cast<i16>(delta)))
                : static_cast<u32>(delta);
            eax_ = std::bit_cast<u32>(display);
            edx_ = request_.local_text_token;
            if (!format_into_local(
                    display, kLegacyBattleLevelGrowthIntegerFormatToken, {}
                )) {
                return;
            }
            ecx_ = kFontToken;
            eax_ = request_.local_text_token;
            draw_local_text(0x158, kTextY[index], kGrowthColor);
            ++result_.displayed_growth_values;

            if (decrement_growth(index)) {
                edx_ = std::bit_cast<u32>(
                    bindings_.victory.input_dispatch.sample_mix_level
                );
                const auto sample = port_.play_level_growth_sample(
                    eax_,
                    ecx_,
                    edx_,
                    kLegacyBattleLevelGrowthSample,
                    bindings_.victory.input_dispatch.sample_mix_level
                );
                ++result_.sample_calls;
                eax_ = sample.eax;
                ecx_ = sample.ecx;
                edx_ = sample.edx;
            }
        }
    }

    [[nodiscard]] const world_map::LegacyWorldStoryPartyMemberResources*
    live_growth_profile(const std::size_t index, const u16 baseline) {
        const i32 actor = static_cast<i32>(static_cast<i8>(
            bindings_.victory.target_selection.transition_actor_index
        ));
        if (index == 0U) {
            ecx_ = std::bit_cast<u32>(actor);
        } else {
            eax_ = std::bit_cast<u32>(actor);
        }
        if (index == 1U || index == 6U) {
            edx_ = replace_low_word(edx_, baseline);
        }
        if (actor < 0 ||
            static_cast<std::size_t>(actor) >=
                bindings_.victory.startup.action_mode_source.actor_label_indices
                    .size()) {
            result_.status =
                LegacyBattleLevelGrowthPanelStatus::actor_index_typed_stop;
            return nullptr;
        }
        const u32 label =
            bindings_.victory.startup.action_mode_source
                .actor_label_indices[static_cast<std::size_t>(actor)];
        eax_ = label;
        if (index == 0U) {
            edx_ = label * 7U;
        } else {
            ecx_ = label * 7U;
        }
        if (label >= bindings_.victory.party_member_resources.size()) {
            result_.status = LegacyBattleLevelGrowthPanelStatus::
                party_member_resource_typed_stop;
            return nullptr;
        }
        return &bindings_.victory.party_member_resources[label];
    }

    void prepare_arrow_registers(const std::size_t index) noexcept {
        if (index == 0U) {
            eax_ = kFramebufferToken;
        } else if (index == 1U || index == 2U) {
            eax_ = kFramebufferToken;
        } else {
            eax_ = kFramebufferToken;
        }
        ecx_ = kFontToken;
    }

    [[nodiscard]] bool decrement_growth(const std::size_t index) {
        if (index == 1U &&
            bindings_.advancement.growth_delta_primary[0U] != 0U) {
            return false;
        }
        if (index == 2U &&
            bindings_.advancement.growth_delta_primary[1U] != 0U) {
            return false;
        }
        if (index >= 4U &&
            bindings_.advancement.growth_delta_secondary[index - 4U] != 0U) {
            return false;
        }

        auto& delta = growth_delta(bindings_.advancement, index);
        const bool positive =
            index < 3U ? static_cast<i16>(delta) > 0 : delta > 0U;
        if (!positive) {
            return false;
        }
        delta = static_cast<u16>(delta - 1U);
        ++result_.decremented_growth_values;
        if (index < 3U && static_cast<i16>(delta) < 0) {
            delta = 0U;
            return false;
        }
        return true;
    }

    [[nodiscard]] LegacyBattleLevelGrowthPanelResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleLevelGrowthPanelBindings bindings_;
    LegacyBattleLevelGrowthPanelPort& port_;
    const LegacyBattleLevelGrowthPanelRequest& request_;
    LegacyBattleLevelGrowthPanelResult result_{};
    std::array<u8, 64U> local_text_{};
    u32 local_text_length_{};
    bool panel_visible_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleLevelGrowthPanelResult advance_legacy_battle_level_growth_panel(
    LegacyBattleLevelGrowthPanelBindings bindings,
    LegacyBattleLevelGrowthPanelPort& port,
    const LegacyBattleLevelGrowthPanelRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
