#include "openswd3/battle/legacy_battle_growth_item_result_selection.hpp"

#include "openswd3/battle/legacy_battle_mon_definition_text_release.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

[[nodiscard]] constexpr i32 as_i32(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_a_token(const u32 actor_index) noexcept {
    return kLegacyBattleVictoryGroupABaseToken +
        actor_index * kLegacyBattleVictoryGroupAElementSize;
}

enum class ActorAdvance : u8 {
    skipped,
    selected,
    stopped,
};

class GrowthItemResultSelectionRunner final {
public:
    GrowthItemResultSelectionRunner(
        LegacyBattleGrowthItemResultSelectionBindings bindings,
        LegacyBattleGrowthItemResultSelectionPort& port,
        const LegacyBattleGrowthItemResultSelectionRequest& request
    )
        : bindings_(bindings), port_(port),
          scratch_(port.battle_growth_actor_selection_state().scratch),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGrowthItemResultSelectionResult run() {
        eax_ = bindings_.metrics.group_a_count;
        u32 actor_index = 0U;
        if (as_i32(eax_) <= 0) {
            return finish();
        }

        while (true) {
            const auto advance = advance_actor(actor_index);
            if (advance == ActorAdvance::stopped ||
                advance == ActorAdvance::selected) {
                return finish();
            }

            eax_ = bindings_.metrics.group_a_count;
            ++actor_index;
            if (as_i32(actor_index) >=
                as_i32(bindings_.metrics.group_a_count)) {
                break;
            }
        }

        return finish();
    }

private:
    [[nodiscard]] LegacyBattleGrowthItemResultSelectionCallReply invoke(
        const LegacyBattleGrowthItemResultSelectionCall call,
        const u32 actor_token = 0U,
        const u32 destination_token = 0U,
        const u32 source_token = 0U,
        const u32 profile_token = 0U,
        const u32 item_code = 0U,
        const std::array<u32, 4U> arguments = {},
        const std::array<u8, world_map::kLegacyItemDefinitionSnapshotBytes>*
            text = nullptr,
        const u32 text_length = 0U
    ) {
        LegacyBattleGrowthItemResultSelectionCallRequest request{
            .call = call,
            .actor_token = actor_token,
            .destination_token = destination_token,
            .source_token = source_token,
            .profile_token = profile_token,
            .item_code = item_code,
            .arguments = arguments,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .text_length = text_length,
        };
        if (text != nullptr) {
            request.text = *text;
        }

        const auto reply = port_.invoke_growth_item_result_selection(request);
        ++result_.port_calls;
        result_.call_trace.push_back(call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        if (reply.publish_group_a_count) {
            bindings_.metrics.group_a_count = reply.group_a_count;
        }
        return reply;
    }

    [[nodiscard]] ActorAdvance advance_actor(const u32 actor_index) {
        result_.stopped_actor_index = actor_index;
        if (actor_index >= bindings_.victory.group_a_skip_primary.size() ||
            actor_index >= bindings_.victory.group_a_skip_secondary.size()) {
            result_.status = LegacyBattleGrowthItemResultSelectionStatus::
                group_a_actor_typed_stop;
            return ActorAdvance::stopped;
        }
        if (bindings_.victory.group_a_skip_primary[actor_index] == 1U ||
            bindings_.victory.group_a_skip_secondary[actor_index] == 1U) {
            return ActorAdvance::skipped;
        }

        const u32 actor_token = group_a_token(actor_index);
        ecx_ = actor_token;
        static_cast<void>(invoke(
            LegacyBattleGrowthItemResultSelectionCall::query_actor_completion,
            actor_token
        ));
        ++result_.completion_query_calls;
        if (eax_ == 1U) {
            return ActorAdvance::skipped;
        }

        ecx_ = actor_token;
        result_.growth_reward = select_legacy_battle_group_a_growth_reward(
            &port_.group_a_reward_profile_state(),
            &bindings_.startup.party[actor_index]
                 .attribute_aggregation.embedded_profiles,
            actor_token,
            kLegacyBattleGrowthItemResultProfileToken
        );
        ++result_.item_selection_calls;
        eax_ = result_.growth_reward.return_eax;
        ecx_ = result_.growth_reward.return_ecx;
        edx_ = result_.growth_reward.return_edx;
        if (result_.growth_reward.status !=
            LegacyBattleGroupAGrowthRewardSelectionStatus::completed) {
            result_.status = LegacyBattleGrowthItemResultSelectionStatus::
                growth_reward_typed_stop;
            return ActorAdvance::stopped;
        }
        if (static_cast<u16>(eax_) == 0U) {
            return ActorAdvance::skipped;
        }

        const u32 item_code = eax_;
        result_.selected_item_code = item_code;
        if (!load_item_definition(item_code)) {
            return ActorAdvance::stopped;
        }
        if (!release_item_description()) {
            return ActorAdvance::stopped;
        }
        bindings_.target_selection.transition_mode = 1U;
        if (!copy_caption()) {
            return ActorAdvance::stopped;
        }

        bindings_.target_selection.transition_actor_index =
            static_cast<u8>(actor_index);
        ++result_.selected_actor_count;
        return ActorAdvance::selected;
    }

    [[nodiscard]] bool load_item_definition(const u32 item_code) {
        std::array<u8, kLegacyBattleMonDefinitionBytes> definition{};
        std::copy(
            scratch_.bytes.cbegin(), scratch_.bytes.cend(), definition.begin()
        );
        definition[0xA0U] = static_cast<u8>(scratch_.description_token);
        definition[0xA1U] = static_cast<u8>(scratch_.description_token >> 8U);
        definition[0xA2U] = static_cast<u8>(scratch_.description_token >> 16U);
        definition[0xA3U] = static_cast<u8>(scratch_.description_token >> 24U);
        const auto definition_result = load_legacy_battle_mon_definition(
            definition,
            scratch_.description,
            port_,
            {
                .path = "mon.dat",
                .output_token = kLegacyBattleGrowthItemScratchToken,
                .definition_id = item_code,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.port_calls;
        ++result_.item_load_calls;
        eax_ = definition_result.return_eax;
        ecx_ = definition_result.return_ecx;
        edx_ = definition_result.return_edx;
        std::copy_n(
            definition.cbegin(), scratch_.bytes.size(), scratch_.bytes.begin()
        );
        scratch_.description_token = static_cast<u32>(definition[0xA0U]) |
            (static_cast<u32>(definition[0xA1U]) << 8U) |
            (static_cast<u32>(definition[0xA2U]) << 16U) |
            (static_cast<u32>(definition[0xA3U]) << 24U);
        if (legacy_battle_mon_definition_load_stopped(
                definition_result.status
            )) {
            result_.status = LegacyBattleGrowthItemResultSelectionStatus::
                definition_load_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool release_item_description() {
        std::array<u8, kLegacyBattleMonDefinitionBytes> definition{};
        std::copy(
            scratch_.bytes.cbegin(), scratch_.bytes.cend(), definition.begin()
        );
        definition[0xA0U] = static_cast<u8>(scratch_.description_token);
        definition[0xA1U] = static_cast<u8>(scratch_.description_token >> 8U);
        definition[0xA2U] = static_cast<u8>(scratch_.description_token >> 16U);
        definition[0xA3U] = static_cast<u8>(scratch_.description_token >> 24U);
        const auto release_result = release_legacy_battle_mon_definition_text(
            definition,
            scratch_.description,
            port_,
            {
                .object_token = kLegacyBattleGrowthItemScratchToken,
                .entry_eax = eax_,
                .entry_ecx = ecx_,
                .entry_edx = edx_,
            }
        );
        ++result_.port_calls;
        ++result_.item_release_calls;
        eax_ = release_result.return_eax;
        ecx_ = release_result.return_ecx;
        edx_ = release_result.return_edx;
        scratch_.description_token = static_cast<u32>(definition[0xA0U]) |
            (static_cast<u32>(definition[0xA1U]) << 8U) |
            (static_cast<u32>(definition[0xA2U]) << 16U) |
            (static_cast<u32>(definition[0xA3U]) << 24U);
        if (legacy_battle_mon_definition_text_release_stopped(
                release_result.status
            )) {
            result_.status = LegacyBattleGrowthItemResultSelectionStatus::
                definition_release_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool copy_caption() {
        auto& destination = bindings_.level_advancement.growth_caption_text;
        u32 text_length = 0U;
        for (std::size_t index = 0U;; ++index) {
            result_.stopped_caption_index = static_cast<u32>(index);
            const u8 value = scratch_.bytes[index];
            if (index >= destination.size()) {
                ++result_.caption_copy_calls;
                result_.call_trace.push_back(
                    LegacyBattleGrowthItemResultSelectionCall::copy_caption
                );
                result_.status = LegacyBattleGrowthItemResultSelectionStatus::
                    caption_destination_typed_stop;
                return false;
            }

            destination[index] = value;
            if (value == 0U) {
                text_length = static_cast<u32>(index);
                break;
            }
        }

        static_cast<void>(invoke(
            LegacyBattleGrowthItemResultSelectionCall::copy_caption,
            0U,
            kLegacyBattleGrowthItemResultCaptionToken,
            kLegacyBattleGrowthItemScratchToken,
            0U,
            0U,
            {
                kLegacyBattleGrowthItemResultCaptionToken,
                kLegacyBattleGrowthItemScratchToken,
                0U,
                0U,
            },
            &scratch_.bytes,
            text_length
        ));
        ++result_.caption_copy_calls;
        return true;
    }

    [[nodiscard]] LegacyBattleGrowthItemResultSelectionResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleGrowthItemResultSelectionBindings bindings_;
    LegacyBattleGrowthItemResultSelectionPort& port_;
    LegacyBattleGrowthItemDefinitionState& scratch_;
    LegacyBattleGrowthItemResultSelectionResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
};

}  // namespace

LegacyBattleGrowthItemResultSelectionResult
advance_legacy_battle_growth_item_result_selection(
    const LegacyBattleGrowthItemResultSelectionBindings bindings,
    LegacyBattleGrowthItemResultSelectionPort& port,
    const LegacyBattleGrowthItemResultSelectionRequest& request
) {
    return GrowthItemResultSelectionRunner(bindings, port, request).run();
}

}  // namespace openswd3::battle
