#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"

#include <algorithm>
#include <cstddef>
#include <span>

namespace openswd3::battle {
namespace {

constexpr std::size_t kUpperWorkspaceBegin = 0x29U;

}  // namespace

LegacyBattleGroupAWorkspaceResetResult reset_legacy_battle_group_a_workspace(
    LegacyBattleGroupAWorkspaceState& state,
    const LegacyBattleGroupAWorkspaceResetBindings bindings
) noexcept {
    state.tail_words[7U] = 0U;
    state.tail_words[8U] = 0U;
    state.tail_words[9U] = 0U;
    state.field_2f0c = 0U;
    if (bindings.action != nullptr) {
        bindings.action->special_particle_sequence_index = 0U;
    }

    state.tail_words[0U] = 0U;
    state.tail_words[1U] = 0U;
    if (bindings.item_effect != nullptr) {
        bindings.item_effect->cached_profile_item_id = 0U;
    }

    state.tail_words[6U] = 0U;
    state.tail_words[5U] = 0U;
    if (bindings.action != nullptr) {
        bindings.action->effect_curve_index = 0U;
    }

    state.tail_words[10U] = 0U;
    if (bindings.action != nullptr) {
        bindings.action->special_particle_sequence_count = 0U;
    }

    state.tail_words[2U] = 0U;
    if (bindings.action != nullptr) {
        bindings.action->summon_action_id = 0U;
    }

    state.tail_words[4U] = 0U;
    if (bindings.final_processing != nullptr) {
        bindings.final_processing->replacement_action_kind = 0U;
    }

    state.tail_words[3U] = 0U;
    if (bindings.final_processing != nullptr) {
        bindings.final_processing->applied_output_value = 0U;
    }

    std::fill(
        state.late_workspace.begin() + kUpperWorkspaceBegin,
        state.late_workspace.end(),
        0U
    );
    if (bindings.particle != nullptr) {
        auto bytes = std::as_writable_bytes(
            std::span{bindings.particle->spawn_action_records}
        );
        static_assert(
            sizeof(bindings.particle->spawn_action_records) == 0xBEU * 4U
        );
        std::fill(bytes.begin(), bytes.end(), std::byte{});
    }

    state.early_workspace.fill(0U);
    if (bindings.action != nullptr) {
        auto first = std::as_writable_bytes(
            std::span{&bindings.action->special_action_record, 1U}
        );
        auto second = std::as_writable_bytes(
            std::span{&bindings.action->special_secondary_action_record, 1U}
        );
        static_assert(
            sizeof(bindings.action->special_action_record) * 2U == 0x4CU * 4U
        );
        std::fill(first.begin(), first.end(), std::byte{});
        std::fill(second.begin(), second.end(), std::byte{});
    }

    std::fill(
        state.late_workspace.begin(),
        state.late_workspace.begin() + kUpperWorkspaceBegin,
        0U
    );

    return {
        .explicit_words_zeroed = 11U,
        .explicit_dwords_zeroed = 1U,
        .upper_workspace_dwords_zeroed = 0xBEU,
        .early_workspace_dwords_zeroed = 0x4CU,
        .lower_workspace_dwords_zeroed = 0x29U,
        .return_edx = state.object_token,
    };
}

}  // namespace openswd3::battle
