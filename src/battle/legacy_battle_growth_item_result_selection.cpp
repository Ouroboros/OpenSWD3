#include "openswd3/battle/legacy_battle_growth_item_result_selection.hpp"

#include <algorithm>
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
        static_cast<void>(invoke(
            LegacyBattleGrowthItemResultSelectionCall::select_growth_item,
            actor_token,
            0U,
            0U,
            kLegacyBattleGrowthItemResultProfileToken,
            0U,
            {kLegacyBattleGrowthItemResultProfileToken, 0U, 0U, 0U}
        ));
        ++result_.item_selection_calls;
        if (static_cast<u16>(eax_) == 0U) {
            return ActorAdvance::skipped;
        }

        const u32 item_code = eax_;
        result_.selected_item_code = item_code;
        load_item_definition(item_code);
        release_item_description();
        bindings_.target_selection.transition_mode = 1U;
        if (!copy_caption()) {
            return ActorAdvance::stopped;
        }

        bindings_.target_selection.transition_actor_index =
            static_cast<u8>(actor_index);
        ++result_.selected_actor_count;
        return ActorAdvance::selected;
    }

    void load_item_definition(const u32 item_code) {
        scratch_.bytes.fill(0U);
        scratch_.description.clear();
        const auto reply = invoke(
            LegacyBattleGrowthItemResultSelectionCall::load_item_definition,
            0U,
            kLegacyBattleGrowthItemScratchToken,
            0U,
            0U,
            item_code,
            {kLegacyBattleGrowthItemScratchToken, item_code, 0U, 0U}
        );
        ++result_.item_load_calls;
        if (!reply.publish_definition) {
            return;
        }

        scratch_.bytes = reply.definition;
        const auto length = std::min<std::size_t>(
            reply.description_length, reply.description.size()
        );
        scratch_.description.assign(
            reply.description.begin(),
            reply.description.begin() + static_cast<std::ptrdiff_t>(length)
        );
    }

    void release_item_description() {
        static_cast<void>(invoke(
            LegacyBattleGrowthItemResultSelectionCall::release_item_description,
            0U,
            0U,
            kLegacyBattleGrowthItemScratchToken,
            0U,
            0U,
            {kLegacyBattleGrowthItemScratchToken, 0U, 0U, 0U}
        ));
        ++result_.item_release_calls;
        scratch_.description.clear();
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
