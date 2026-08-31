#include "openswd3/battle/legacy_battle_group_b_script_resource_parameters.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

constexpr compat::u32 kLegacyBattleGroupBResourcePointerOffset = 0x0CU;
constexpr compat::u32 kLegacyBattleGroupBResourceParameterOffset = 0x92U;
constexpr compat::u32 kLegacyBattleGroupBResourceParameterCount = 9U;

[[nodiscard]] constexpr compat::u32
wrapping_add(const compat::u32 left, const compat::u32 right) noexcept {
    return left + right;
}

}  // namespace

LegacyBattleGroupBScriptResourceParametersResult
write_legacy_battle_group_b_script_resource_parameters(
    LegacyBattleActorGroupBElementState* actor,
    const LegacyBattleGroupBScriptResourceParametersRequest& request
) {
    LegacyBattleGroupBScriptResourceParametersResult result;
    result.return_eax = request.source_token;
    result.return_ecx = request.actor_token;
    result.return_edx = request.entry_edx;

    for (compat::u32 index = 0U;
         index < kLegacyBattleGroupBResourceParameterCount;
         ++index) {
        if (actor == nullptr) {
            result.status = LegacyBattleGroupBScriptResourceParametersStatus::
                actor_state_typed_stop;
            result.stopped_offset = kLegacyBattleGroupBResourcePointerOffset;
            return result;
        }

        const compat::u32 resource_token = actor->resource_token;
        ++result.resource_pointer_loads;
        if (index + 1U == kLegacyBattleGroupBResourceParameterCount) {
            result.return_ecx = resource_token;
        } else {
            result.return_edx = resource_token;
        }

        const compat::u32 source_offset =
            wrapping_add(request.source_offset, index * 2U);
        if (source_offset >= request.script_capacity ||
            source_offset >= request.script_bytes.size()) {
            result.status = LegacyBattleGroupBScriptResourceParametersStatus::
                script_read_typed_stop;
            result.stopped_offset = source_offset;
            return result;
        }

        const compat::u8 value = request.script_bytes[source_offset];
        ++result.source_reads;
        if (index + 1U == kLegacyBattleGroupBResourceParameterCount) {
            result.return_edx = (result.return_edx & 0xFFFFFF00U) |
                static_cast<compat::u32>(value);
        }

        const compat::u32 target_offset =
            kLegacyBattleGroupBResourceParameterOffset + index;
        if (resource_token == 0U) {
            result.status = LegacyBattleGroupBScriptResourceParametersStatus::
                resource_write_typed_stop;
            result.stopped_offset = target_offset;
            return result;
        }

        actor->resource_bytes[target_offset] = value;
        ++result.resource_writes;
    }

    return result;
}

}  // namespace openswd3::battle
