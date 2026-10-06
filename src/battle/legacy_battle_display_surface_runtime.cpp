#include "openswd3/battle/legacy_battle_display_surface_runtime.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <utility>

namespace openswd3::battle {

LegacyBattleDisplaySurfaceRuntime::LegacyBattleDisplaySurfaceRuntime(
    const rendering::LegacySurfaceGeometry& display
) noexcept
    : display_(display) {}

std::optional<compat::u32>
LegacyBattleDisplaySurfaceRuntime::release_battle_display_surface(
    const compat::u32 token
) {
    const auto found = surfaces_.find(token);
    if (found == surfaces_.end()) {
        return std::nullopt;
    }

    surfaces_.erase(found);
    return 0U;
}

compat::u32 LegacyBattleDisplaySurfaceRuntime::battle_display_height() {
    return static_cast<compat::u32>(display_.height);
}

compat::u32 LegacyBattleDisplaySurfaceRuntime::battle_display_width() {
    return static_cast<compat::u32>(display_.width);
}

compat::u32 LegacyBattleDisplaySurfaceRuntime::create_battle_display_surface(
    const compat::u32 width, const compat::u32 height
) {
    // The software backend has a signed byte pitch and signed dimensions.
    // Unrepresentable geometry is a normal failed surface creation.
    constexpr auto maximum = std::numeric_limits<compat::i32>::max();
    if (width == 0U || height == 0U ||
        width > static_cast<compat::u32>(maximum) / 2U ||
        height > static_cast<compat::u32>(maximum)) {
        return 0U;
    }

    const std::uint64_t count = std::uint64_t{width} * height;
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(compat::u16)) {
        return 0U;
    }

    try {
        LegacyBattleDisplaySurface surface;
        surface.geometry = {
            .pitch_bytes = static_cast<compat::i32>(width * 2U),
            .width = static_cast<compat::i32>(width),
            .height = static_cast<compat::i32>(height),
        };
        surface.pixels.resize(static_cast<std::size_t>(count));
        // Only an opaque object identity is exported; no host pointer or COM
        // object layout is exposed. Share the existing guest identity space.
        const auto token = asset_runtime::reserve_legacy_guest_bytes(1U);
        if (!token.has_value()) {
            return 0U;
        }

        surfaces_.emplace(*token, std::move(surface));
        // 437BD6..437BE9 accumulates allocation bytes with dword wrapping.
        // Release does not subtract from this counter.
        allocated_bytes_ += width * height * 2U;
        return *token;
    } catch (const std::bad_alloc&) {
        return 0U;
    } catch (const std::length_error&) {
        return 0U;
    }
}

LegacyBattleDisplaySurface*
LegacyBattleDisplaySurfaceRuntime::find(const compat::u32 token) noexcept {
    const auto found = surfaces_.find(token);
    return found == surfaces_.end() ? nullptr : &found->second;
}

std::size_t
LegacyBattleDisplaySurfaceRuntime::live_surface_count() const noexcept {
    return surfaces_.size();
}

compat::u32
LegacyBattleDisplaySurfaceRuntime::allocated_bytes() const noexcept {
    return allocated_bytes_;
}

}  // namespace openswd3::battle
