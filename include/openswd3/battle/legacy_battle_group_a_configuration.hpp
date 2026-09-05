#pragma once

#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_group_a_workspace_reset.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupAMissingPlacementTextToken =
    0x004A7C2CU;
inline constexpr compat::u32 kLegacyBattleGroupAMissingPlacementSourceToken =
    0x004A7C44U;
inline constexpr compat::u32 kLegacyBattleGroupAMissingPlacementSourceLine =
    0xDEU;

struct LegacyBattleGroupAConfigurationSourceRecord {
    std::array<compat::u32, 14> dwords{};
};

struct LegacyBattleGroupAPlacementRecord {
    std::array<compat::u32, 5> prefix{};
    compat::u16 role_id{};
    compat::u16 position_x{};
    compat::u16 position_y{};
    compat::u16 field_1a{};
    compat::u32 active{};
};

struct LegacyBattleGroupAConfigurationState {
    compat::u32 actor_record_token{};
    std::array<compat::u32, 14> actor_record{};
    std::array<compat::u32, 8> placement_primary{};
    std::array<compat::u32, 8> placement_secondary{};
    compat::u32 source_record_token{};
    compat::u32 auxiliary_record_token{};
    compat::u32 placement_tail{};
    compat::u8 field_2a93{};
    compat::u16 placement_word{};
    compat::u32 profile_token{};
    std::array<std::byte, 0xA4> profile_record{};
    std::vector<compat::u8> profile_description{};
    compat::u16 profile_field_f2{};
};

struct LegacyBattleGroupAConfigurationDiagnosticRequest {
    compat::u32 window_token{};
    compat::u32 text_token{};
    compat::u32 flags{};
    compat::u32 source_token{};
    compat::u32 source_line{};
};

struct LegacyBattleGroupAConfigurationDiagnosticReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleGroupAConfigurationDiagnosticPort {
public:
    virtual ~LegacyBattleGroupAConfigurationDiagnosticPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupAConfigurationDiagnosticReply
    report_missing_placement(
        const LegacyBattleGroupAConfigurationDiagnosticRequest& request
    ) = 0;
};

enum class LegacyBattleGroupAConfigurationStatus : compat::u8 {
    completed,
    placement_typed_stop,
    source_record_typed_stop,
    actor_record_typed_stop,
};

struct LegacyBattleGroupAConfigurationResult {
    LegacyBattleGroupAConfigurationStatus status{
        LegacyBattleGroupAConfigurationStatus::completed
    };
    LegacyBattleGroupAWorkspaceResetResult workspace_reset{};
    compat::u32 placement_dwords_copied{};
    compat::u32 actor_record_dwords_copied{};
    compat::u32 source_clamp_writes{};
    compat::u32 diagnostic_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46E730.
[[nodiscard]] LegacyBattleGroupAConfigurationResult
configure_legacy_battle_group_a_actor(
    LegacyBattleGroupAWorkspaceState& workspace,
    LegacyBattleGroupAConfigurationState& state,
    LegacyBattleActorProgressState& progress,
    LegacyBattleGroupAConfigurationSourceRecord& source,
    const LegacyBattleGroupAPlacementRecord& placement,
    compat::u32 source_record_token,
    compat::u32 auxiliary_record_token,
    compat::u32 placement_token,
    compat::u32 window_token,
    LegacyBattleGroupAConfigurationDiagnosticPort& diagnostic_port
);

}  // namespace openswd3::battle
