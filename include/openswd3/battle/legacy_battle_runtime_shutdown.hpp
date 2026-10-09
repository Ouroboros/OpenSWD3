#pragma once

#include "openswd3/battle/legacy_battle_group_b_resource_cleanup.hpp"
#include "openswd3/battle/legacy_battle_render_geometry.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleGroupAObjectBaseToken = 0x005029D0U;
inline constexpr compat::u32 kLegacyBattleGroupAObjectStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleGroupAObjectCount = 10U;
inline constexpr compat::u32 kLegacyBattleGroupBObjectBaseToken = 0x00525508U;
inline constexpr compat::u32 kLegacyBattleGroupBObjectStride = 0x2B28U;
inline constexpr compat::u32 kLegacyBattleGroupBObjectCount = 8U;

static_assert(
    kLegacyBattleGroupAObjectBaseToken +
        kLegacyBattleGroupAObjectCount * kLegacyBattleGroupAObjectStride ==
    0x005201D8U
);
static_assert(
    kLegacyBattleGroupBObjectBaseToken +
        kLegacyBattleGroupBObjectCount * kLegacyBattleGroupBObjectStride ==
    0x0053AE48U
);

enum class LegacyBattleRuntimeShutdownStatus : compat::u8 {
    completed,
    group_b_resource_typed_stop,
};

struct LegacyBattleRuntimeShutdownResult {
    LegacyBattleRuntimeShutdownStatus status{
        LegacyBattleRuntimeShutdownStatus::completed
    };
    LegacyBattleRenderCleanupResult render_cleanup{};
    std::array<LegacyBattleGroupAResourceCleanupResult, 10>
        group_a_resource_cleanups{};
    std::array<LegacyBattleGroupBResourceCleanupResult, 8>
        group_b_resource_cleanups{};
    compat::u32 render_cleanup_calls{};
    compat::u32 group_a_calls{};
    compat::u32 group_a_resource_calls{};
    compat::u32 group_b_calls{};
    compat::u32 group_b_resource_calls{};
    compat::u32 stopped_group_b_index{};
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
};

[[nodiscard]] LegacyBattleRuntimeShutdownResult shutdown_legacy_battle_runtime(
    LegacyBattleStartupState& startup,
    LegacyBattleRenderAuxiliaryBufferReleaser& render_resources,
    LegacyBattleGroupAResourceReleasePort& party_resources,
    LegacyBattleGroupBResourceReleasePort& enemy_resources
) noexcept;

}  // namespace openswd3::battle
