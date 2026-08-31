#include "openswd3/battle/legacy_battle_group_b_script_action_item_parameters.hpp"

namespace openswd3::battle {
namespace {

constexpr compat::u32 kLegacyBattleGroupBResourcePointerOffset = 0x0CU;
constexpr compat::u32 kLegacyBattleGroupBActionItemParameterOffset = 0x66U;
constexpr compat::u32 kLegacyBattleGroupBActionItemParameterCount = 6U;

constexpr void
set_low_word(compat::u32& destination, const compat::u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | static_cast<compat::u32>(value);
}

}  // namespace

LegacyBattleGroupBScriptActionItemParametersResult
write_legacy_battle_group_b_script_action_item_parameters(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBScriptActionItemParametersRequest& request
) {
    LegacyBattleGroupBScriptActionItemParametersResult result;
    result.return_eax = request.entry_eax;
    result.return_ecx = request.actor_token;
    result.return_edx = request.entry_edx;

    for (compat::u32 index = 0U;
         index < kLegacyBattleGroupBActionItemParameterCount;
         ++index) {
        const compat::u16 parameter = request.parameters[index];
        set_low_word(result.return_eax, parameter);
        ++result.parameter_reads;
        if (parameter == 0U) {
            continue;
        }

        if (actor == nullptr) {
            result.status = LegacyBattleGroupBScriptActionItemParametersStatus::
                actor_state_typed_stop;
            result.stopped_offset = kLegacyBattleGroupBResourcePointerOffset;
            result.stopped_parameter_index = index;
            return result;
        }

        const compat::u32 resource_token = actor->resource_token;
        ++result.resource_pointer_loads;
        if (index + 1U == kLegacyBattleGroupBActionItemParameterCount) {
            result.return_ecx = resource_token;
        } else {
            result.return_edx = resource_token;
        }

        const compat::u32 target_offset =
            kLegacyBattleGroupBActionItemParameterOffset + index * 2U;
        if (resource_token == 0U) {
            result.status = LegacyBattleGroupBScriptActionItemParametersStatus::
                resource_write_typed_stop;
            result.stopped_offset = target_offset;
            result.stopped_parameter_index = index;
            return result;
        }

        actor->resource_bytes[target_offset] =
            static_cast<compat::u8>(parameter);
        actor->resource_bytes[target_offset + 1U] =
            static_cast<compat::u8>(parameter >> 8U);
        ++result.resource_writes;
    }

    return result;
}

}  // namespace openswd3::battle
