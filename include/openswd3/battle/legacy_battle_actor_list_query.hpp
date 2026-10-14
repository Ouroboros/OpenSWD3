#pragma once

#include <optional>

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_final_processing_state.hpp"
#include "openswd3/battle/legacy_battle_group_a_item_effect_application.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"
#include "openswd3/battle/legacy_battle_text_message.hpp"
#include "openswd3/battle/legacy_battle_inventory.hpp"
#include "openswd3/audio_video/legacy_sample_commands.hpp"

#include <span>

namespace openswd3::battle {

struct LegacyBattleActorListQueryState {
    world_map::LegacyWorldItemListState* inventory{};
    compat::u32 resource_head_token{};       // actor + 0x2EC8
    compat::u32 next_resource_head_token{};  // actor + 0x2ECC
    compat::u32 selected_resource_token{};
    compat::u16 primary_required{};
    compat::u16 secondary_required{};
};

inline constexpr std::array<compat::u16, 24>
    kLegacyBattleActorListDisplayFrames{
        8U,      1U,      2U,  3U,   4U,   0xFFFFU, 5U,   6U,
        7U,      0xFFFFU, 9U,  10U,  20U,  25U,     10U,  0xFFF6U,
        0xFFE7U, 0xFFDDU, 62U, 124U, 186U, 248U,    310U, 372U,
    };

enum class LegacyBattleActorListQueryStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    list_owner_typed_stop,
    list_node_typed_stop,
    resource_owner_typed_stop,
    resource_node_typed_stop,
    list_text_typed_stop,
    return_table_typed_stop,
    profile_load_typed_stop,
    message_typed_stop,
};

struct LegacyBattleActorListQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };
    compat::u16 display_frame{0xFFFFU};
};

struct LegacyBattleActorListApplyResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    std::optional<compat::u16> selection_mode;
};

class LegacyBattleActorResourceSelectionPort {
public:
    virtual ~LegacyBattleActorResourceSelectionPort() = default;
    virtual void report_missing_runtime_word(compat::u16 resource_id) = 0;
};

struct LegacyBattleActorResourceSelectionResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool selected{};
};

struct LegacyBattleActorFlaggedResourceQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool found{};
};

struct LegacyBattleActorModeResourceQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool found{};
};

struct LegacyBattleActorResourceReleaseResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    compat::u16 output_word{};
};

struct LegacyBattleActorResourceListQueryResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool found{};
};

struct LegacyBattleActorListStateResult {
    LegacyBattleActorListQueryStatus status{
        LegacyBattleActorListQueryStatus::completed
    };

    bool available{};
};

// sub_470180.
[[nodiscard]] LegacyBattleActorListQueryResult query_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 category_selector,
    compat::u32 type_selector,
    compat::u32 occurrence,
    std::span<compat::u8> text,
    compat::u32& output_value,
    LegacyBattleMonProfile& profile,
    std::span<const compat::u16> display_frames =
        kLegacyBattleActorListDisplayFrames,
    std::span<compat::u8> text_continuation = {}
);

// sub_4705C0.
[[nodiscard]] LegacyBattleActorListApplyResult apply_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 category_selector,
    compat::u32 type_selector,
    compat::u32 occurrence,
    compat::u32& output_value
);

// sub_4702E0.
[[nodiscard]] LegacyBattleActorListQueryStatus count_legacy_battle_actor_list(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    compat::u32 category_selector,
    compat::u32 type_selector,
    compat::u8& count
);

// sub_470890.
[[nodiscard]] LegacyBattleActorListQueryStatus
refresh_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration
);

// sub_470820.
[[nodiscard]] LegacyBattleActorListQueryStatus
execute_legacy_battle_actor_list_action(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAWorkspaceState* workspace
);

// sub_470AC0.
[[nodiscard]] LegacyBattleActorResourceSelectionResult
select_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleGroupAFinalProcessingState* final_state,
    LegacyBattleGroupAItemEffectApplicationState* item_effect,
    LegacyBattleGroupAWorkspaceState* workspace,
    LegacyBattleActorResourceSelectionPort& port,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u32 category_selector,
    compat::u32 occurrence,
    compat::u32& output_runtime_word,
    compat::u16& output_mode
);

// sub_470F70.
[[nodiscard]] LegacyBattleActorFlaggedResourceQueryResult
query_legacy_battle_actor_flagged_resource(
    LegacyBattleActorListQueryState* list,
    compat::u32 occurrence,
    std::span<compat::u8> output_name,
    compat::u16& output_quantity
);

// sub_470FE0.
[[nodiscard]] LegacyBattleActorModeResourceQueryResult
query_legacy_battle_actor_mode_resource(
    LegacyBattleActorListQueryState* list,
    compat::u32 mode,
    compat::u32 occurrence,
    std::span<compat::u8> output_name,
    compat::u16& output_quantity
);

// sub_471080.
[[nodiscard]] LegacyBattleActorListQueryStatus
count_legacy_battle_actor_mode_resources(
    LegacyBattleActorListQueryState* list, compat::u16& output_quantity
);

// sub_470E20.
[[nodiscard]] LegacyBattleActorResourceReleaseResult
release_legacy_battle_actor_resource(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAWorkspaceState* workspace
);

// sub_470A10.
[[nodiscard]] LegacyBattleActorListQueryStatus
count_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    compat::u32 category_selector,
    compat::u16& count
);

// sub_470910.
[[nodiscard]] LegacyBattleActorResourceListQueryResult
query_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    compat::u32 category_selector,
    compat::u32 occurrence,
    std::span<compat::u8> output_name,
    compat::u16& output_flags,
    compat::u16& output_quantity
);

// sub_470900.
[[nodiscard]] LegacyBattleActorListQueryStatus
commit_legacy_battle_actor_resource_list(
    LegacyBattleActorListQueryState* list
) noexcept;

// sub_470380.
[[nodiscard]] LegacyBattleActorListStateResult
process_legacy_battle_actor_list_state(
    LegacyBattleGroupAActionExecutionState* actor,
    LegacyBattleActorListQueryState* list,
    LegacyBattleGroupAConfigurationState* configuration,
    LegacyBattleTextMessageState& messages,
    compat::u32& message_head,
    LegacyBattleTextMessagePort& message_port,
    audio_video::LegacySampleManager& samples,
    const compat::i32& sample_mix_level,
    compat::u32 category_selector,
    compat::u32 occurrence
);

}  // namespace openswd3::battle
