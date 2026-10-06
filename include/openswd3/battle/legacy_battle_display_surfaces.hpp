#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <optional>

namespace openswd3::battle {

struct LegacyBattleStartupState;

class LegacyBattleDisplaySurfacePort {
public:
    virtual ~LegacyBattleDisplaySurfacePort() = default;

    // A disengaged result means the host cannot resolve the legacy surface.
    // A normal COM reference count of zero is a completed release.
    [[nodiscard]] virtual std::optional<compat::u32>
    release_battle_display_surface(compat::u32 token) = 0;
    [[nodiscard]] virtual compat::u32 battle_display_height() = 0;
    [[nodiscard]] virtual compat::u32 battle_display_width() = 0;
    // Zero is the original CreateSurface failure result, not a typed stop.
    [[nodiscard]] virtual compat::u32
    create_battle_display_surface(compat::u32 width, compat::u32 height) = 0;
};

struct LegacyBattleDisplaySurfaceReleaseResult {
    compat::u32 release_calls{};
    compat::u32 return_value{};
    bool typed_stop{};
};

struct LegacyBattleDisplaySurfaceCreationResult {
    compat::u32 create_calls{};
    compat::u32 return_value{};
    std::array<compat::u8, 3> completion_write_order{};
};

// sub_451AE0: release each nonzero slot before clearing that slot.
[[nodiscard]] LegacyBattleDisplaySurfaceReleaseResult
release_legacy_battle_display_surfaces(
    LegacyBattleStartupState& state, LegacyBattleDisplaySurfacePort& port
);

// sub_451A90: query height, query width, create, publish; repeat twice.
[[nodiscard]] LegacyBattleDisplaySurfaceCreationResult
create_legacy_battle_display_surfaces(
    LegacyBattleStartupState& state, LegacyBattleDisplaySurfacePort& port
);

}  // namespace openswd3::battle
