#include "openswd3/battle/legacy_battle_victory_rewards.hpp"

#include <bit>
#include <charconv>
#include <cstddef>
#include <limits>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

inline constexpr u32 kFramebufferToken = 0x004CD76CU;
inline constexpr u32 kTitleX = 0xF8U;
inline constexpr u32 kTitleY = 0xB4U;
inline constexpr u32 kSummaryX = 0xD0U;
inline constexpr u32 kSummaryFirstY = 0xDCU;
inline constexpr u32 kTextColor = 0xFFC0U;
inline constexpr u32 kFontSize = 0x10U;

inline constexpr std::array<u8, 8> kVictoryTitle{
    0xBEU, 0xD4U, 0xB0U, 0xABU, 0xB3U, 0xD3U, 0xA7U, 0x51U
};
inline constexpr std::array<u8, 14> kPartyExperiencePrefix{
    0xA8U,
    0x43U,
    0xA4U,
    0x48U,
    0xB1U,
    0x6FU,
    0xA8U,
    0xECU,
    0xB8U,
    0x67U,
    0xC5U,
    0xE7U,
    0xADU,
    0xC8U
};
inline constexpr std::array<u8, 8> kMoneyPrefix{
    0xB1U, 0x6FU, 0xA8U, 0xECU, 0xBBU, 0xC8U, 0xB9U, 0xF4U
};
inline constexpr std::array<u8, 14> kRewardExperiencePrefix{
    0xB1U,
    0x6FU,
    0xA8U,
    0xECU,
    0xAAU,
    0x6BU,
    0xC4U,
    0x5FU,
    0xB8U,
    0x67U,
    0xC5U,
    0xE7U,
    0xADU,
    0xC8U
};

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 index) noexcept {
    return kLegacyBattleVictoryGroupABaseToken +
        index * kLegacyBattleVictoryGroupAElementSize;
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kLegacyBattleVictoryGroupBBaseToken +
        index * kLegacyBattleVictoryGroupBElementSize;
}

[[nodiscard]] constexpr bool rectangle_completed(
    const rendering::LegacyRectangleEffectStatus status
) noexcept {
    return status == rendering::LegacyRectangleEffectStatus::completed ||
        status == rendering::LegacyRectangleEffectStatus::clipped_out ||
        status == rendering::LegacyRectangleEffectStatus::unsupported_mode;
}

[[nodiscard]] bool write_decimal_text(
    std::array<u8, 64>& buffer,
    const std::span<const u8> prefix,
    const u32 value,
    u32& length
) noexcept {
    std::array<char, 16> digits{};
    const auto conversion =
        std::to_chars(digits.data(), digits.data() + digits.size(), value);
    if (conversion.ec != std::errc{}) {
        return false;
    }
    const auto digit_count =
        static_cast<std::size_t>(conversion.ptr - digits.data());
    const std::size_t required = prefix.size() + 1U + digit_count + 1U;
    if (required > buffer.size()) {
        return false;
    }
    std::size_t cursor = 0U;
    for (const u8 byte : prefix) {
        buffer[cursor++] = byte;
    }
    buffer[cursor++] = static_cast<u8>(':');
    for (std::size_t index = 0U; index < digit_count; ++index) {
        buffer[cursor++] = static_cast<u8>(digits[index]);
    }
    buffer[cursor] = 0U;
    length = static_cast<u32>(cursor);
    return true;
}

class Runner {
public:
    Runner(
        LegacyBattleVictoryRewardBindings bindings,
        LegacyBattleVictoryRewardPort& port,
        const LegacyBattleVictoryRewardRequest& request
    ) noexcept
        : bindings_(bindings), port_(port), request_(request),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleVictoryRewardResult run() {
        initialize_local_text();
        draw_base_panel();
        if (result_.status != LegacyBattleVictoryRewardStatus::completed) {
            return finish();
        }

        const u32 packed_reward_words =
            static_cast<u32>(bindings_.state.committed_money_word) |
            (static_cast<u32>(bindings_.state.experience_per_party_member)
             << 16U);
        eax_ = packed_reward_words;
        if ((eax_ & 0x00008000U) == 0U) {
            if (!distribute_rewards()) {
                return finish();
            }
            commit_money();
            if (result_.status != LegacyBattleVictoryRewardStatus::completed) {
                return finish();
            }
        }

        draw_summary();
        return finish();
    }

private:
    void initialize_local_text() noexcept {
        text_.fill(0U);
        eax_ = 0U;
        ecx_ = 0U;
    }

    void draw_base_panel() {
        bindings_.state.panel_action_record.action_id =
            kLegacyBattleVictoryPanelAction;
        bindings_.state.panel_action_record.base_variant = 0U;
        result_.panel_action_update = bindings_.action_updater.update(
            bindings_.state.panel_action_record
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
                LegacyBattleVictoryRewardStatus::rectangle_typed_stop;
            return;
        }

        edx_ = (edx_ & 0xFFFF0000U) |
            static_cast<u32>(bindings_.state.panel_action_record.field_4a);
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
                LegacyBattleVictoryRewardStatus::title_frame_typed_stop;
            return;
        }

        std::array<u8, 64> title{};
        for (std::size_t index = 0U; index < kVictoryTitle.size(); ++index) {
            title[index] = kVictoryTitle[index];
        }
        eax_ = kFramebufferToken;
        ecx_ = kLegacyBattleVictoryFontToken;
        draw_text(
            title, static_cast<u32>(kVictoryTitle.size()), kTitleX, kTitleY
        );

        edx_ = (edx_ & 0xFFFF0000U) |
            static_cast<u32>(bindings_.state.panel_action_record.field_4a);
        const i32 dynamic_bottom = std::bit_cast<i32>(
            bindings_.target_selection.transition_stage + 0xD4U
        );
        result_.tiled_frames[1U] = rendering::draw_legacy_tiled_frame(
            bindings_.framebuffer,
            bindings_.raster,
            bindings_.frame_provider,
            {
                .resource_id = edx_,
                .left = 0xC8,
                .top = 0xD4,
                .right = 0x178,
                .bottom = dynamic_bottom,
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
                LegacyBattleVictoryRewardStatus::summary_frame_typed_stop;
        }
    }

    [[nodiscard]] bool distribute_rewards() {
        auto registers = port_.begin_music_fade(eax_, ecx_, edx_);
        ++result_.music_fade_calls;
        eax_ = registers.eax;
        ecx_ = registers.ecx;
        edx_ = registers.edx;

        registers = port_.stop_all_samples(eax_, ecx_, edx_);
        ++result_.stop_all_sample_calls;
        eax_ = registers.eax;
        ecx_ = registers.ecx;
        edx_ = registers.edx;

        eax_ = std::bit_cast<u32>(bindings_.input_dispatch.sample_mix_level);
        const auto sample = port_.play_input_sample(
            kLegacyBattleVictorySample,
            bindings_.input_dispatch.sample_mix_level,
            eax_,
            ecx_,
            edx_
        );
        ++result_.sample_calls;
        eax_ = sample.eax;
        ecx_ = sample.ecx;
        edx_ = sample.edx;

        u32 group_b_index = 0U;
        eax_ = bindings_.metrics.group_b_count;
        while (as_i32(group_b_index) <
               as_i32(bindings_.metrics.group_b_count)) {
            ecx_ = group_b_token(group_b_index);
            if (group_b_index >= 8U) {
                result_.status =
                    LegacyBattleVictoryRewardStatus::group_b_actor_typed_stop;
                return false;
            }
            call(LegacyBattleVictoryRewardCall::query_group_b_item, ecx_);
            ++result_.group_b_query_calls;
            const u16 item_id = static_cast<u16>(eax_);
            if (item_id != 0U && !store_item(item_id)) {
                return false;
            }
            ++group_b_index;
            eax_ = bindings_.metrics.group_b_count;
        }

        u32 group_a_index = 0U;
        eax_ = bindings_.metrics.group_a_count;
        while (as_i32(group_a_index) <
               as_i32(bindings_.metrics.group_a_count)) {
            if (group_a_index >= bindings_.state.group_a_skip_primary.size() ||
                group_a_index >=
                    bindings_.state.group_a_skip_secondary.size()) {
                result_.status =
                    LegacyBattleVictoryRewardStatus::group_a_actor_typed_stop;
                return false;
            }
            eax_ = bindings_.state.group_a_skip_primary[group_a_index];
            if (eax_ != 1U &&
                bindings_.state.group_a_skip_secondary[group_a_index] != 1U) {
                ecx_ = group_a_token(group_a_index);
                call(
                    LegacyBattleVictoryRewardCall::query_group_a_reward_block,
                    ecx_
                );
                ++result_.group_a_query_calls;
                if (eax_ == 0U && !reward_group_a_actor(group_a_index)) {
                    return false;
                }
            }
            ++group_a_index;
            eax_ = bindings_.metrics.group_a_count;
        }
        return true;
    }

    [[nodiscard]] bool store_item(const u16 item_id) {
        std::size_t slot = bindings_.state.collected_item_ids.size();
        for (std::size_t index = 0U;
             index < bindings_.state.collected_item_ids.size();
             ++index) {
            if (bindings_.state.collected_item_ids[index] == item_id) {
                slot = index;
                break;
            }
        }
        const bool existing = slot < bindings_.state.collected_item_ids.size();
        if (!existing) {
            slot = bindings_.target_selection.transition_sample_word;
        }

        result_.player_item_quantity =
            advance_legacy_battle_player_item_quantity(port_, item_id, 1U);
        ++result_.player_item_quantity_calls;
        result_.port_calls += result_.player_item_quantity.port_calls;
        eax_ = result_.player_item_quantity.return_token;
        if (result_.player_item_quantity.status !=
            LegacyBattlePlayerItemQuantityStatus::completed) {
            result_.status = LegacyBattleVictoryRewardStatus::
                player_item_quantity_typed_stop;
            return false;
        }

        if (slot >= bindings_.state.collected_item_quantities.size()) {
            result_.status = LegacyBattleVictoryRewardStatus::
                collected_item_quantity_typed_stop;
            return false;
        }
        ++bindings_.state.collected_item_quantities[slot];
        if (existing) {
            return true;
        }

        ++bindings_.target_selection.transition_sample_word;
        bindings_.state.player_item_tokens[slot] = eax_;
        bindings_.state.collected_item_ids[slot] = item_id;
        return true;
    }

    [[nodiscard]] bool reward_group_a_actor(const u32 actor_index) {
        if (actor_index >=
            bindings_.startup.action_mode_source.actor_label_indices.size()) {
            result_.status =
                LegacyBattleVictoryRewardStatus::action_label_typed_stop;
            return false;
        }
        const u32 label = bindings_.startup.action_mode_source
                              .actor_label_indices[actor_index];
        if (label >= bindings_.party_member_resources.size()) {
            result_.status = LegacyBattleVictoryRewardStatus::
                party_member_resource_typed_stop;
            return false;
        }
        auto& profile = bindings_.party_member_resources[label];
        if (static_cast<u16>(profile.field_2c) <
            bindings_.state.party_profile_threshold) {
            edx_ =
                static_cast<u32>(bindings_.state.experience_per_party_member);
            profile.field_00 += edx_;
        }

        ecx_ = group_a_token(actor_index);
        eax_ = (label * 56U & 0xFFFF0000U) |
            static_cast<u32>(bindings_.state.reward_experience);
        call(
            LegacyBattleVictoryRewardCall::apply_group_a_reward,
            ecx_,
            {kLegacyBattleVictoryProfileToken, eax_}
        );
        if (eax_ == 1U) {
            bindings_.state.actor_reward_gate = 1U;
        }

        if (label >= bindings_.state.party_reward_counters.size()) {
            result_.status = LegacyBattleVictoryRewardStatus::
                party_reward_counter_typed_stop;
            return false;
        }
        ++bindings_.state.party_reward_counters[label];
        if (as_i32(bindings_.metrics.group_b_count) >= 3) {
            ++bindings_.state.party_reward_counters[label];
        }
        eax_ = 0x004ACF54U + label * 0x60U;
        ecx_ = bindings_.state.party_reward_counters[label];
        edx_ = bindings_.metrics.group_b_count;

        ecx_ = group_a_token(actor_index);
        call(LegacyBattleVictoryRewardCall::prepare_group_a_actor, ecx_, {1U});
        ecx_ = group_a_token(actor_index);
        call(
            LegacyBattleVictoryRewardCall::configure_group_a_actor,
            ecx_,
            {0U, 0U}
        );
        return true;
    }

    void commit_money() {
        const u16 money =
            static_cast<u16>(bindings_.state.committed_money_word & 0x7FFFU);
        bindings_.state.committed_money_word =
            static_cast<u16>(bindings_.state.committed_money_word | 0x8000U);
        bindings_.target_selection.transition_timer = 0U;
        eax_ = money;
        if (bindings_.script_variables.empty()) {
            result_.status =
                LegacyBattleVictoryRewardStatus::script_variable_typed_stop;
            return;
        }
        bindings_.script_variables[0U] += eax_;
        ecx_ = bindings_.script_variables[0U];
    }

    void draw_summary() {
        call(
            LegacyBattleVictoryRewardCall::query_summary_panel,
            0U,
            {0xD4U, 0x11CU, 2U}
        );
        if (eax_ != 1U) {
            return;
        }

        if (!format_and_draw(
                kPartyExperiencePrefix,
                bindings_.state.experience_per_party_member,
                kSummaryFirstY,
                0U
            ) ||
            !format_and_draw(
                kMoneyPrefix,
                bindings_.state.committed_money_word & 0x7FFFU,
                kSummaryFirstY + 0x14U,
                1U
            ) ||
            !format_and_draw(
                kRewardExperiencePrefix,
                bindings_.state.reward_experience,
                kSummaryFirstY + 0x28U,
                2U
            )) {
            return;
        }
    }

    template <std::size_t Size>
    [[nodiscard]] bool format_and_draw(
        const std::array<u8, Size>& prefix,
        const u32 value,
        const u32 y,
        const u32 register_mode
    ) {
        u32 length = 0U;
        if (!write_decimal_text(text_, prefix, value, length)) {
            result_.status =
                LegacyBattleVictoryRewardStatus::format_buffer_typed_stop;
            return false;
        }
        if (register_mode == 0U) {
            eax_ = length;
            edx_ = kFramebufferToken;
        } else if (register_mode == 1U) {
            eax_ = kFramebufferToken;
            edx_ = request_.local_text_token;
        } else {
            eax_ = request_.local_text_token;
            edx_ = 0U;
        }
        ecx_ = kLegacyBattleVictoryFontToken;
        draw_text(text_, length, kSummaryX, y);
        return true;
    }

    void draw_text(
        const std::array<u8, 64>& text,
        const u32 length,
        const u32 x,
        const u32 y
    ) {
        LegacyBattleVictoryRewardCallRequest request{
            .call = LegacyBattleVictoryRewardCall::draw_text,
            .arguments =
                {
                    kFramebufferToken,
                    x,
                    y,
                    kTextColor,
                    kFontSize,
                    0U,
                },
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text = text,
            .text_length = length,
        };
        const auto reply = port_.invoke_victory_reward(request);
        ++result_.port_calls;
        ++result_.text_draw_calls;
        result_.call_trace.push_back(request.call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        apply(reply);
    }

    void call(
        const LegacyBattleVictoryRewardCall call_kind,
        const u32 actor_token = 0U,
        const std::array<u32, 4>& arguments = {}
    ) {
        std::array<u32, 6> expanded_arguments{};
        for (std::size_t index = 0U; index < arguments.size(); ++index) {
            expanded_arguments[index] = arguments[index];
        }
        const LegacyBattleVictoryRewardCallRequest request{
            .call = call_kind,
            .actor_token = actor_token,
            .arguments = expanded_arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        };
        const auto reply = port_.invoke_victory_reward(request);
        ++result_.port_calls;
        result_.call_trace.push_back(call_kind);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        apply(reply);
    }

    void apply(const LegacyBattleVictoryRewardCallReply& reply) noexcept {
        if (reply.publish_group_b_count) {
            bindings_.metrics.group_b_count = reply.group_b_count;
        }
        if (reply.publish_group_a_count) {
            bindings_.metrics.group_a_count = reply.group_a_count;
        }
        if (reply.publish_collected_item_count) {
            bindings_.target_selection.transition_sample_word =
                reply.collected_item_count;
        }
        if (reply.publish_reward_words) {
            bindings_.state.committed_money_word = reply.committed_money_word;
            bindings_.state.experience_per_party_member =
                reply.experience_per_party_member;
            bindings_.state.reward_experience = reply.reward_experience;
        }
    }

    [[nodiscard]] LegacyBattleVictoryRewardResult finish() noexcept {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleVictoryRewardBindings bindings_;
    LegacyBattleVictoryRewardPort& port_;
    const LegacyBattleVictoryRewardRequest& request_;
    LegacyBattleVictoryRewardResult result_{};
    std::array<u8, 64> text_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleVictoryRewardResult advance_legacy_battle_victory_rewards(
    const LegacyBattleVictoryRewardBindings bindings,
    LegacyBattleVictoryRewardPort& port,
    const LegacyBattleVictoryRewardRequest& request
) {
    return Runner(bindings, port, request).run();
}

}  // namespace openswd3::battle
