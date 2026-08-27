#pragma once

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

enum class LegacyBattleRuntimeShutdownCall : compat::u8 {
    release_group_a_object,
    release_group_b_object,
};

struct LegacyBattleRuntimeShutdownCallRequest {
    LegacyBattleRuntimeShutdownCall call{
        LegacyBattleRuntimeShutdownCall::release_group_a_object
    };
    compat::u32 object_token{};
    compat::u32 object_index{};
};

struct LegacyBattleRuntimeShutdownCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleRuntimeShutdownPort
    : public LegacyBattleRenderAuxiliaryBufferReleaser {
public:
    ~LegacyBattleRuntimeShutdownPort() override = default;

    [[nodiscard]] virtual LegacyBattleRuntimeShutdownCallReply
    invoke_battle_runtime_shutdown(
        const LegacyBattleRuntimeShutdownCallRequest& request
    ) = 0;
};

struct LegacyBattleRuntimeShutdownResult {
    LegacyBattleRenderCleanupResult render_cleanup{};
    std::array<LegacyBattleRuntimeShutdownCallReply, 10> group_a_replies{};
    std::array<LegacyBattleRuntimeShutdownCallReply, 8> group_b_replies{};
    compat::u32 render_cleanup_calls{};
    compat::u32 group_a_calls{};
    compat::u32 group_b_calls{};
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
};

[[nodiscard]] LegacyBattleRuntimeShutdownResult shutdown_legacy_battle_runtime(
    LegacyBattleStartupState& startup, LegacyBattleRuntimeShutdownPort& port
) noexcept;

}  // namespace openswd3::battle
