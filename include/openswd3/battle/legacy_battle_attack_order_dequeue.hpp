#pragma once

#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueRecordBase =
    0x00524788U;
inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueRecordEnd =
    0x00524980U;
inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueGroupABase =
    0x005029D0U;
inline constexpr compat::u32 kLegacyBattleAttackOrderDequeueGroupAStride =
    0x2F34U;

struct LegacyBattleAttackOrderDequeueOutput {
    compat::u32* value_00{};
    std::span<compat::u32> tail_dwords;
};

struct LegacyBattleAttackOrderDequeueBindings {
    std::span<LegacyBattleStartupResetRecord> records;
    std::span<LegacyBattleIntensityEffectRecord> adjacent_intensity_records;
    LegacyBattleAttackOrderDequeueOutput output;
};

struct LegacyBattleAttackOrderDequeueRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleAttackOrderDequeueActorRequest {
    compat::u32 actor_token{};
    compat::u32 actor_code{};
    compat::u32 actor_index{};
    compat::u32 stale_eax{};
    compat::u32 stale_edx{};
};

struct LegacyBattleAttackOrderDequeueActorReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleAttackOrderDequeuePort {
public:
    virtual ~LegacyBattleAttackOrderDequeuePort() = default;

    [[nodiscard]] virtual LegacyBattleAttackOrderDequeueActorReply
    query_actor(const LegacyBattleAttackOrderDequeueActorRequest& request) = 0;
};

enum class LegacyBattleAttackOrderDequeueStatus : compat::u8 {
    completed,
    record_scan_typed_stop,
    output_source_typed_stop,
    output_destination_typed_stop,
    shift_source_typed_stop,
    shift_destination_typed_stop,
    empty_scan_typed_stop,
    cleanup_typed_stop,
};

struct LegacyBattleAttackOrderDequeueResult {
    LegacyBattleAttackOrderDequeueStatus status{
        LegacyBattleAttackOrderDequeueStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 selected_index{0xFFFFFFFFU};
    compat::u32 actor_query_calls{};
    compat::u32 output_dwords{};
    compat::u32 shifted_records{};
    compat::u32 cleared_records{};
    bool selected_from_adjacent_intensity{};
};

// Typed closure of legacy 0x0045F020. It scans from the fixed attack-order
// base, copies one complete physical record to the caller output, then removes
// the selected record and restores the all-one/zero tail convention.
[[nodiscard]] LegacyBattleAttackOrderDequeueResult
dequeue_legacy_battle_attack_order_entry(
    LegacyBattleAttackOrderDequeueBindings bindings,
    LegacyBattleAttackOrderDequeuePort& port,
    const LegacyBattleAttackOrderDequeueRequest& request = {}
);

}  // namespace openswd3::battle
