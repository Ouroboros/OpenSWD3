#pragma once

#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupASummonProfileSize = 0xA4U;
inline constexpr compat::u32 kLegacyBattleGroupASummonAllocateCallToken =
    0x00487C10U;
inline constexpr compat::u32 kLegacyBattleGroupASummonReleaseCallToken =
    0x00478220U;
inline constexpr compat::u32 kLegacyBattleGroupASummonDiagnosticCallToken =
    0x00431150U;
inline constexpr compat::u32 kLegacyBattleGroupASummonDiagnosticTextToken =
    0x004A7C68U;
inline constexpr compat::u32 kLegacyBattleGroupASummonDiagnosticSourceToken =
    0x004A7C44U;
inline constexpr compat::u32 kLegacyBattleGroupASummonDiagnosticSourceLine =
    0x123U;

using LegacyBattleGroupASummonProfileRecord =
    std::array<std::byte, kLegacyBattleGroupASummonProfileSize>;

enum class LegacyBattleGroupASummonMaterializationCall : compat::u8 {
    allocate_profile,
    reserved_load_profile,
    load_profile = reserved_load_profile,
    release_profile_text,
    report_missing_role,
};

struct LegacyBattleGroupASummonMaterializationCallRequest {
    LegacyBattleGroupASummonMaterializationCall call{
        LegacyBattleGroupASummonMaterializationCall::allocate_profile
    };
    compat::u32 profile_token{};
    compat::u16 role_id{};
    compat::u32 window_token{};
    compat::u32 diagnostic_text_token{};
    compat::u32 diagnostic_source_token{};
    compat::u32 diagnostic_source_line{};
    LegacyBattleGroupASummonProfileRecord profile_record{};
};

struct LegacyBattleGroupASummonMaterializationCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattleGroupASummonProfileRecord profile_record{};
};

class LegacyBattleGroupASummonMaterializationPort
    : public virtual LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyBattleGroupASummonMaterializationPort() = default;

    [[nodiscard]] virtual LegacyBattleGroupASummonMaterializationCallReply
    invoke_group_a_summon_materialization(
        const LegacyBattleGroupASummonMaterializationCallRequest& request
    ) = 0;
};

enum class LegacyBattleGroupASummonMaterializationStatus : compat::u8 {
    completed,
    allocation_typed_stop,
    actor_state_typed_stop,
    source_record_typed_stop,
    actor_record_typed_stop,
    profile_load_typed_stop,
};

struct LegacyBattleGroupASummonMaterializationResult {
    LegacyBattleGroupASummonMaterializationStatus status{
        LegacyBattleGroupASummonMaterializationStatus::completed
    };
    compat::u32 port_calls{};
    compat::u32 allocation_calls{};
    compat::u32 load_calls{};
    compat::u32 release_calls{};
    compat::u32 diagnostic_calls{};
    compat::u32 profile_dwords_zeroed{};
    compat::u32 placement_dwords_copied{};
    compat::u32 profile_name_bytes_copied{};
    compat::u32 allocated_profile_token{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// sub_46E890.
[[nodiscard]] LegacyBattleGroupASummonMaterializationResult
materialize_legacy_battle_group_a_summon(
    LegacyBattleGroupAConfigurationState* state,
    const LegacyBattleGroupAPlacementRecord* source,
    compat::u32 actor_token,
    compat::u32 source_token,
    compat::u32 window_token,
    LegacyBattleGroupASummonMaterializationPort& port
);

}  // namespace openswd3::battle
