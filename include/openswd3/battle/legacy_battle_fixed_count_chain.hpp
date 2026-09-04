#pragma once

#include "openswd3/battle/legacy_battle_fixed_object_reset.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFixedCountOwnerToken = 0x004B9F00U;
inline constexpr compat::u32 kLegacyBattleFixedCurveOwnerToken = 0x004ACBA8U;
inline constexpr compat::u32 kLegacyBattleFixedDefinitionCurveOwnerToken =
    0x004B8A00U;
inline constexpr compat::u32 kLegacyBattleFixedDefinitionScratchToken =
    0x0053CF50U;
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

struct LegacyBattleFixedCurveAdvanceRequest {
    compat::u32 owner_token{kLegacyBattleFixedCurveOwnerToken};
    compat::u32 key{};
    compat::u32 maximum{};
    compat::u32 multiplier{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

// Tracks only values left pending by this helper. `empty` does not assert that
// the caller's pre-existing physical x87 stack was empty on entry.
enum class LegacyBattleFixedCurveX87StackState : compat::u8 {
    empty,
    maximum,
    ratio,
};

struct LegacyBattleFixedCurveAdvanceResult {
    LegacyBattleFixedCountStatus status{
        LegacyBattleFixedCountStatus::completed
    };
    LegacyBattleFixedCountPath path{LegacyBattleFixedCountPath::none};
    LegacyBattleFixedCurveX87StackState x87_stack{
        LegacyBattleFixedCurveX87StackState::empty
    };
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
    compat::u32 scale_writes{};
    compat::u32 root_key_increments{};
    compat::u32 truncate_calls{};
    compat::u16 count{};
    compat::u16 scale{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

struct LegacyBattleFixedCurveSetRequest {
    compat::u32 owner_token{kLegacyBattleFixedCurveOwnerToken};
    compat::u32 key{};
    compat::u32 maximum{};
    compat::u32 count{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleFixedCurveSetResult {
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
    compat::u32 scale_writes{};
    compat::u32 root_key_increments{};
    compat::u32 truncate_calls{};
    compat::u16 count{};
    compat::u16 scale{};
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

// Typed closure of legacy 0x00477830. The selected record count advances by one
// and is capped by the input maximum before two x87-compatible scaled values
// are published. Missing keys use the shared twenty-byte allocator boundary.
[[nodiscard]] LegacyBattleFixedCurveAdvanceResult
advance_legacy_battle_fixed_curve(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    const LegacyBattleFixedCurveAdvanceRequest& request
);

// Typed closure of legacy 0x00477920. The input count word is written before
// the inclusive maximum clamp, then its x87-compatible percentage is stored.
// Missing keys use the same physical curve root and twenty-byte allocator.
[[nodiscard]] LegacyBattleFixedCurveSetResult set_legacy_battle_fixed_curve(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    const LegacyBattleFixedCurveSetRequest& request
);

struct LegacyBattleFixedCurveLookupRequest {
    compat::u32 owner_token{kLegacyBattleFixedCurveOwnerToken};
    compat::u32 key{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleFixedCurveLookupResult {
    LegacyBattleFixedCountStatus status{
        LegacyBattleFixedCountStatus::completed
    };
    compat::u16 value{};
    compat::u32 matched_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 key_reads{};
    compat::u32 chain_link_reads{};
    compat::u32 value_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x004779F0. The root participates in the first key
// comparison; a hit replaces only AX with the value word at plus eight, while
// a missing key returns zero after scanning the existing chain.
[[nodiscard]] LegacyBattleFixedCurveLookupResult
lookup_legacy_battle_fixed_curve(
    LegacyBattleFixedObjectState& state,
    const LegacyBattleFixedCurveLookupRequest& request
) noexcept;

struct LegacyBattleFixedDefinitionCurveSetRequest {
    std::filesystem::path definition_path{"mon.dat"};
    compat::u32 owner_token{kLegacyBattleFixedDefinitionCurveOwnerToken};
    compat::u32 definition_output_token{
        kLegacyBattleFixedDefinitionScratchToken
    };
    compat::u32 key{};
    compat::u32 count{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleFixedDefinitionCurveSetStatus : compat::u8 {
    completed,
    definition_load_typed_stop,
    record_access_typed_stop,
    allocation_record_access_typed_stop,
};

struct LegacyBattleFixedDefinitionCurveSetResult {
    LegacyBattleFixedDefinitionCurveSetStatus status{
        LegacyBattleFixedDefinitionCurveSetStatus::completed
    };
    LegacyBattleFixedCountPath path{LegacyBattleFixedCountPath::none};
    LegacyBattleFixedCurveX87StackState x87_stack{
        LegacyBattleFixedCurveX87StackState::empty
    };
    LegacyBattleMonDefinitionLoadResult definition_load{};
    compat::u32 owner_token{};
    compat::u32 matched_token{};
    compat::u32 allocation_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 definition_load_calls{};
    compat::u32 definition_cleanup_calls{};
    compat::u32 definition_text_release_calls{};
    compat::u32 root_count_reads{};
    compat::u32 chain_link_reads{};
    compat::u32 key_reads{};
    compat::u32 lock_reads{};
    compat::u32 allocation_calls{};
    compat::u32 link_writes{};
    compat::u32 dword_zero_writes{};
    compat::u32 key_writes{};
    compat::u32 count_writes{};
    compat::u32 clamp_writes{};
    compat::u32 scale_writes{};
    compat::u32 root_count_increments{};
    compat::u32 truncate_calls{};
    compat::u16 maximum{};
    compat::u16 count{};
    compat::u16 scale{};
    bool locked{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00477A20. A MON definition is loaded and its
// transient text is released before the third fixed chain is searched. An
// existing nonzero word at plus ten locks the record; otherwise the input
// count is clamped to the definition word at plus 0x44 and scaled to percent.
[[nodiscard]] LegacyBattleFixedDefinitionCurveSetResult
set_legacy_battle_fixed_definition_curve(
    LegacyBattleFixedObjectState& state,
    LegacyBattleFixedCountAllocationPort& allocation_port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattleFixedDefinitionCurveSetRequest& request
);

struct LegacyBattleFixedDefinitionCurveLookupRequest {
    std::filesystem::path definition_path{"mon.dat"};
    compat::u32 owner_token{kLegacyBattleFixedDefinitionCurveOwnerToken};
    compat::u32 definition_output_token{
        kLegacyBattleFixedDefinitionScratchToken
    };
    compat::u32 maximum_output_token{};
    compat::u32 count_output_token{};
    compat::u32 key{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleFixedDefinitionCurveLookupStatus : compat::u8 {
    completed,
    record_access_typed_stop,
    definition_load_typed_stop,
    maximum_output_typed_stop,
    count_output_typed_stop,
};

struct LegacyBattleFixedDefinitionCurveLookupResult {
    LegacyBattleFixedDefinitionCurveLookupStatus status{
        LegacyBattleFixedDefinitionCurveLookupStatus::completed
    };
    LegacyBattleFixedCountPath path{LegacyBattleFixedCountPath::none};
    LegacyBattleMonDefinitionLoadResult definition_load{};
    compat::u32 owner_token{};
    compat::u32 matched_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 definition_load_calls{};
    compat::u32 definition_cleanup_calls{};
    compat::u32 definition_text_release_calls{};
    compat::u32 key_reads{};
    compat::u32 chain_link_reads{};
    compat::u32 maximum_reads{};
    compat::u32 count_reads{};
    compat::u32 maximum_output_writes{};
    compat::u32 count_output_writes{};
    compat::u16 maximum{};
    compat::u16 count{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x00477B40. The third fixed chain is searched before
// the MON definition is loaded and cleaned. The definition maximum is written
// first; a hit then reads and writes the stored count, while a miss writes zero.
[[nodiscard]] LegacyBattleFixedDefinitionCurveLookupResult
lookup_legacy_battle_fixed_definition_curve(
    LegacyBattleFixedObjectState& state,
    LegacyBattleMonDatabasePort& mon_port,
    compat::u16* maximum_output,
    compat::u16* count_output,
    const LegacyBattleFixedDefinitionCurveLookupRequest& request
);

}  // namespace openswd3::battle
