#pragma once

#include "openswd3/battle/legacy_battle_display_surfaces.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace openswd3::battle {

struct LegacyBattleDisplaySurface {
    rendering::LegacySurfaceGeometry geometry{};
    // Contents are not an original-game clear operation. Callers must capture
    // the source image before using a newly created display surface.
    std::vector<compat::u16> pixels;
};

// Software backing for the two display surfaces. The logical display geometry
// is borrowed from the game's framebuffer, independently of host window size.
class LegacyBattleDisplaySurfaceRuntime final
    : public LegacyBattleDisplaySurfacePort {
public:
    explicit LegacyBattleDisplaySurfaceRuntime(
        const rendering::LegacySurfaceGeometry& display
    ) noexcept;

    LegacyBattleDisplaySurfaceRuntime(
        const LegacyBattleDisplaySurfaceRuntime&
    ) = delete;
    LegacyBattleDisplaySurfaceRuntime&
    operator=(const LegacyBattleDisplaySurfaceRuntime&) = delete;

    [[nodiscard]] std::optional<compat::u32>
    release_battle_display_surface(compat::u32 token) override;
    [[nodiscard]] compat::u32 battle_display_height() override;
    [[nodiscard]] compat::u32 battle_display_width() override;
    [[nodiscard]] compat::u32 create_battle_display_surface(
        compat::u32 width, compat::u32 height
    ) override;

    // Borrowed until that token is released. Missing or released tokens have
    // no backing surface; they must never be redirected to another surface.
    [[nodiscard]] LegacyBattleDisplaySurface* find(compat::u32 token) noexcept;
    [[nodiscard]] std::size_t live_surface_count() const noexcept;
    [[nodiscard]] compat::u32 allocated_bytes() const noexcept;

private:
    const rendering::LegacySurfaceGeometry& display_;
    std::unordered_map<compat::u32, LegacyBattleDisplaySurface> surfaces_;
    compat::u32 allocated_bytes_{};
};

}  // namespace openswd3::battle
