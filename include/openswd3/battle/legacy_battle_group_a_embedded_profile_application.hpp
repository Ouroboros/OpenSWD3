#pragma once

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

struct LegacyBattleGroupAEmbeddedProfileItemQuantityRequest {
    compat::u32 item_list_token{};
    compat::u16 item_id{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGroupAEmbeddedProfileItemQuantityReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupAEmbeddedProfileApplicationPort {
public:
    virtual ~LegacyBattleGroupAEmbeddedProfileApplicationPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAEmbeddedProfileItemQuantityReply
    lookup_embedded_profile_item_quantity(
        const LegacyBattleGroupAEmbeddedProfileItemQuantityRequest& request
    ) = 0;
};

struct LegacyBattleGroupAEmbeddedProfileApplicationRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleGroupAEmbeddedProfileApplicationStatus : compat::u8 {
    completed,
    profile_typed_stop,
    actor_state_typed_stop,
    actor_record_typed_stop,
};

struct LegacyBattleGroupAEmbeddedProfileApplicationResult {
    LegacyBattleGroupAEmbeddedProfileApplicationStatus status{
        LegacyBattleGroupAEmbeddedProfileApplicationStatus::completed
    };
    compat::u16 profile_kind{};
    compat::u16 item_id{};
    compat::u32 port_calls{};
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
