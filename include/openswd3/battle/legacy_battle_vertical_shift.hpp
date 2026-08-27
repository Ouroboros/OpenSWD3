#pragma once

#include <array>

#include "openswd3/battle/legacy_battle_surface_blend.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleVerticalShiftOwnerToken = 0x004AB870U;
inline constexpr compat::u32 kLegacyBattleVerticalShiftSourceToken =
    0x004ACBA0U;
inline constexpr compat::u32 kLegacyBattleVerticalShiftSurfaceSelector =
    0x2711U;
inline constexpr compat::u32 kLegacyBattleVerticalShiftFlags = 0x01000000U;
inline constexpr compat::i32 kLegacyBattleVerticalShiftWidth = 640;
inline constexpr compat::i32 kLegacyBattleVerticalShiftHeight = 480;
inline constexpr compat::u32 kLegacyBattleVerticalShiftRowBytes = 1280U;

struct LegacyBattleVerticalShiftState {
    compat::u32 phase_index{};
    compat::u32 tick_counter{};
    compat::u32 tick_limit{};
};

class LegacyBattleVerticalShiftStatePort {
public:
    [[nodiscard]] virtual LegacyBattleVerticalShiftState&
    battle_vertical_shift_state() noexcept {
        return vertical_shift_state_;
    }

protected:
    ~LegacyBattleVerticalShiftStatePort() = default;

private:
    LegacyBattleVerticalShiftState vertical_shift_state_{};
};

class LegacyBattleVerticalShiftPort
    : public virtual LegacyBattleVerticalShiftStatePort {
public:
    virtual ~LegacyBattleVerticalShiftPort() = default;

    [[nodiscard]] virtual compat::u32 resolve_vertical_shift_surface(
        compat::u32 owner_token, compat::u32 selector
    ) = 0;
    [[nodiscard]] virtual compat::u32
    blit_vertical_shift(const LegacyBattleSurfaceBlendOperation& operation) = 0;
};

enum class LegacyBattleVerticalShiftStatus : compat::u8 {
    completed,
    phase_table_typed_stop,
    primary_surface_typed_stop,
    framebuffer_typed_stop,
};

struct LegacyBattleVerticalShiftResult {
    LegacyBattleVerticalShiftStatus status{
        LegacyBattleVerticalShiftStatus::completed
    };
    compat::u32 return_value{};
    std::array<compat::i32, 3> signed_offsets{};
    compat::u32 table_reads{};
    std::array<LegacyBattleSurfaceBlendOperation, 2> operations{};
    compat::u32 surface_resolve_calls{};
    compat::u32 surface_blit_calls{};
    compat::u32 cleared_bytes{};
    compat::u32 cleared_pixels{};
};

[[nodiscard]] LegacyBattleVerticalShiftResult run_legacy_battle_vertical_shift(
    LegacyBattleVerticalShiftPort& port,
    compat::u32& completion_gate,
    const compat::u32& battle_mode_flags,
    rendering::LegacyFramebuffer& framebuffer
);

}  // namespace openswd3::battle
