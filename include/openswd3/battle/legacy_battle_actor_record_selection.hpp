#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/battle/legacy_battle_group_a_configuration.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorRecordSelectionAddress =
    0x00478670U;

enum class LegacyBattleActorRecordSelectionStatus : compat::u8 {
    completed,
    argument_read_typed_stop,
    source_record_read_typed_stop,
    actor_record_read_typed_stop,
    source_return_address_read_typed_stop,
    final_return_address_read_typed_stop,
};

struct LegacyBattleActorRecordSelectionAccess {
    bool argument_readable{true};
    bool source_record_readable{true};
    bool actor_record_readable{true};
    bool source_return_address_readable{true};
    bool final_return_address_readable{true};
};

struct LegacyBattleActorRecordSelectionRequest {
    compat::u32 argument{};
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    LegacyBattleActorRecordSelectionAccess access{};
};

struct LegacyBattleActorRecordSelectionResult {
    LegacyBattleActorRecordSelectionStatus status{
        LegacyBattleActorRecordSelectionStatus::completed
    };
    std::array<compat::u32, 2> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 argument_reads{};
    compat::u32 actor_field_reads{};
    compat::u32 return_address_reads{};
    compat::u32 stack_read_count{};
    compat::u32 tests_executed{};
    bool selected_actor_record{};
    bool selected_source_record{};
    bool xor_zero_executed{};
    bool returned{};
    bool flags_known{true};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x00478670. The stack argument is read and tested
// before exactly one of actor +0x04 or actor +0x00 is accessed. Both RET 4
// exits read the real caller return address and pop the argument.
[[nodiscard]] LegacyBattleActorRecordSelectionResult
select_legacy_battle_actor_record(
    const LegacyBattleGroupAConfigurationState& actor,
    const LegacyBattleActorRecordSelectionRequest& request = {}
) noexcept;

}  // namespace openswd3::battle
