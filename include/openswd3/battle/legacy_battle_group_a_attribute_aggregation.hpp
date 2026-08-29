#pragma once

#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr std::size_t kLegacyBattleGroupAAttributeSourceCount = 16U;
inline constexpr compat::u32 kLegacyBattleGroupAAttributeDiagnosticTextToken =
    0x004A7C94U;
inline constexpr compat::u32 kLegacyBattleGroupAAttributeDiagnosticSourceToken =
    0x004A7C44U;
inline constexpr compat::u32 kLegacyBattleGroupAAttributeDiagnosticSourceLine =
    0x182U;

struct LegacyBattleGroupAAttributeSource {
    const world_map::LegacyWorldItemNode* record{};
    compat::u32 record_token{};
};

using LegacyBattleGroupAAttributeSourceTable = std::array<
    LegacyBattleGroupAAttributeSource,
    kLegacyBattleGroupAAttributeSourceCount>;

struct LegacyBattleGroupAAttributeAggregationState {
    LegacyBattleGroupASummonProfileRecord primary_profile{};
    std::array<LegacyBattleGroupASummonProfileRecord, 2> embedded_profiles{};
};

enum class LegacyBattleGroupAAttributeAggregationCall : compat::u8 {
    report_missing_primary_attribute,
    apply_embedded_profile,
};

struct LegacyBattleGroupAAttributeAggregationCallRequest {
    LegacyBattleGroupAAttributeAggregationCall call{
        LegacyBattleGroupAAttributeAggregationCall::
            report_missing_primary_attribute
    };
    compat::u32 actor_token{};
    compat::u32 source_record_token{};
    compat::u32 embedded_profile_token{};
    compat::u32 embedded_profile_index{};
    compat::u16 item_id{};
    compat::u32 window_token{};
    compat::u32 diagnostic_text_token{};
    compat::u32 diagnostic_source_token{};
    compat::u32 diagnostic_source_line{};
    LegacyBattleGroupASummonProfileRecord embedded_profile{};
};

struct LegacyBattleGroupAAttributeAggregationCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupAAttributeAggregationPort {
public:
    virtual ~LegacyBattleGroupAAttributeAggregationPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAAttributeAggregationCallReply
    invoke_group_a_attribute_aggregation(
        const LegacyBattleGroupAAttributeAggregationCallRequest& request
    ) = 0;
};

enum class LegacyBattleGroupAAttributeAggregationStatus : compat::u8 {
    completed,
    actor_state_typed_stop,
    source_record_typed_stop,
    actor_record_typed_stop,
};

struct LegacyBattleGroupAAttributeAggregationResult {
    LegacyBattleGroupAAttributeAggregationStatus status{
        LegacyBattleGroupAAttributeAggregationStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 diagnostic_calls{};
    compat::u32 embedded_profile_apply_calls{};
    compat::u32 embedded_profile_dwords_zeroed{};
    compat::u32 primary_profile_dwords_copied{};
    compat::u32 embedded_profile_dwords_copied{};
    compat::u32 source_records_visited{};
    compat::u32 actor_word_additions{};
    compat::u32 actor_byte_additions{};
    compat::u32 early_bonus_additions{};
    compat::u32 special_item_latch_writes{};
    compat::u32 fault_source_index{
        static_cast<compat::u32>(kLegacyBattleGroupAAttributeSourceCount)
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46EBB0.
[[nodiscard]] LegacyBattleGroupAAttributeAggregationResult
aggregate_legacy_battle_group_a_attributes(
    LegacyBattleGroupAAttributeAggregationState* state,
    LegacyBattleGroupAWorkspaceState& workspace,
    LegacyBattleGroupAConfigurationState& configuration,
    const LegacyBattleGroupAAttributeSourceTable* sources,
    compat::u32 actor_token,
    compat::u32 source_table_token,
    compat::u32 window_token,
    LegacyBattleGroupAAttributeAggregationPort& port
);

}  // namespace openswd3::battle
