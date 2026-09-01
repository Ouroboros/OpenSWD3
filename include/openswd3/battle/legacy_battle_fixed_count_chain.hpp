#pragma once

#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFixedCountOwnerToken = 0x004B9F00U;
inline constexpr compat::u32 kLegacyBattleFixedCountAllocateCallToken =
    0x00487C10U;
inline constexpr compat::u32 kLegacyBattleFixedCountLimit = 0x14U;

struct LegacyBattleFixedCountAllocationRequest {
    compat::u32 allocation_size{kLegacyBattleFixedObjectSize};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleFixedCountAllocationReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, kLegacyBattleFixedObjectDwordCount> initial_words{};
    compat::u32 accessible_bytes{};
};

class LegacyBattleFixedCountAllocationPort {
public:
    virtual ~LegacyBattleFixedCountAllocationPort() = default;

    [[nodiscard]] virtual LegacyBattleFixedCountAllocationReply
    allocate_legacy_battle_fixed_count_node(
        const LegacyBattleFixedCountAllocationRequest& request
    ) = 0;
};

struct LegacyBattleFixedCountRequest {
    compat::u32 owner_token{kLegacyBattleFixedCountOwnerToken};
    compat::u32 key{};
    compat::u32 delta{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleFixedCountStatus : compat::u8 {
    completed,
    record_access_typed_stop,
    allocation_record_access_typed_stop,
};

enum class LegacyBattleFixedCountPath : compat::u8 {
    none,
    existing_root,
    existing_node,
    allocated_node,
};

struct LegacyBattleFixedCountResult {
    LegacyBattleFixedCountStatus status{
        LegacyBattleFixedCountStatus::completed
    };
    LegacyBattleFixedCountPath path{LegacyBattleFixedCountPath::none};
    compat::u32 owner_token{};
    compat::u32 matched_token{};
    compat::u32 allocation_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 chain_link_reads{};
    compat::u32 key_reads{};
    compat::u32 count_reads{};
    compat::u32 allocation_calls{};
    compat::u32 link_writes{};
    compat::u32 dword_zero_writes{};
    compat::u32 count_writes{};
    compat::u32 key_writes{};
    compat::u32 root_key_increments{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleFixedCountSetRequest {
    compat::u32 owner_token{kLegacyBattleFixedCountOwnerToken};
    compat::u32 key{};
    compat::u32 count{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleFixedCountSetResult {
    LegacyBattleFixedCountStatus status{
        LegacyBattleFixedCountStatus::completed
    };
    LegacyBattleFixedCountPath path{LegacyBattleFixedCountPath::none};
    compat::u32 owner_token{};
    compat::u32 matched_token{};
    compat::u32 allocation_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 chain_link_reads{};
    compat::u32 key_reads{};
    compat::u32 allocation_calls{};
    compat::u32 link_writes{};
    compat::u32 dword_zero_writes{};
    compat::u32 key_writes{};
    compat::u32 count_writes{};
    compat::u32 clamp_writes{};
    compat::u32 root_key_increments{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleFixedCountLookupRequest {
    compat::u32 owner_token{kLegacyBattleFixedCountOwnerToken};
    compat::u32 key{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleFixedCountLookupResult {
    LegacyBattleFixedCountStatus status{
        LegacyBattleFixedCountStatus::completed
    };
    LegacyBattleFixedCountPath path{LegacyBattleFixedCountPath::none};
    compat::u32 owner_token{};
    compat::u32 matched_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 chain_link_reads{};
    compat::u32 key_reads{};
    compat::u32 count_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00477710. Legacy tokens select records owned by
// LegacyBattleFixedObjectState; they are never interpreted as host pointers.
[[nodiscard]] LegacyBattleFixedCountResult accumulate_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    const LegacyBattleFixedCountRequest& request
);

// Typed closure of legacy 0x00477780. Existing records receive the input low
// word before values above twenty are overwritten with twenty. Missing keys
// use the same physical owner and allocation boundary as the accumulating path.
[[nodiscard]] LegacyBattleFixedCountSetResult set_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    const LegacyBattleFixedCountSetRequest& request
);

// Typed closure of legacy 0x00477800. The root participates in the first key
// comparison; missing keys return zero after scanning the existing chain.
[[nodiscard]] LegacyBattleFixedCountLookupResult
lookup_legacy_battle_fixed_count(
    LegacyBattleFixedObjectState& state,
    const LegacyBattleFixedCountLookupRequest& request
) noexcept;

}  // namespace openswd3::battle
