#pragma once

#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"
#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/battle/legacy_battle_group_a_summon_materialization.hpp"
#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleEmbeddedProfileItemListToken =
    0x004B8A00U;

struct LegacyBattleGroupAEmbeddedProfileApplicationState {
    // actor + 0x26C8.
    compat::u32 status_bits{};
};

class LegacyBattleGroupAEmbeddedProfileApplicationPort
    : public virtual LegacyBattleFixedObjectStatePort {
public:
    ~LegacyBattleGroupAEmbeddedProfileApplicationPort() override = default;
};

struct LegacyBattleGroupAEmbeddedProfileApplicationRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAEmbeddedProfileApplicationStatus : compat::u8 {
    completed,
    profile_typed_stop,
    fixed_curve_typed_stop,
    actor_state_typed_stop,
    actor_record_typed_stop,
};

struct LegacyBattleGroupAEmbeddedProfileApplicationResult {
    LegacyBattleGroupAEmbeddedProfileApplicationStatus status{
        LegacyBattleGroupAEmbeddedProfileApplicationStatus::completed
    };
    compat::u16 profile_kind{};
    compat::u16 item_id{};
    LegacyBattleFixedCurveLookupResult fixed_curve{};
    compat::u32 fixed_curve_query_count{};
    compat::u32 status_writes{};
    compat::u32 actor_word_writes{};
    compat::u32 actor_byte_writes{};
    compat::u32 bytes_scanned{};
    compat::u32 modified_byte_index{9U};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46F030.
[[nodiscard]] LegacyBattleGroupAEmbeddedProfileApplicationResult
apply_legacy_battle_group_a_embedded_profile(
    LegacyBattleGroupAEmbeddedProfileApplicationState* state,
    LegacyBattleGroupAConfigurationState& configuration,
    const LegacyBattleGroupASummonProfileRecord* profile,
    compat::u32 actor_token,
    compat::u32 profile_token,
    LegacyBattleGroupAEmbeddedProfileApplicationPort& port,
    const LegacyBattleGroupAEmbeddedProfileApplicationRequest& request = {}
);

}  // namespace openswd3::battle
