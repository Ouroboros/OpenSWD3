#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::input_time_rng {
class LegacySecondaryRng;
}

namespace openswd3::rendering {
class LegacyFramebuffer;
struct LegacyPixelConversionState;
}

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyAniStreakServiceId = 8U;
inline constexpr std::size_t kLegacyAniStreakSlotCount = 64U;
inline constexpr std::size_t kLegacyAniStreakResetSlotCount = 48U;
inline constexpr compat::i16 kLegacyAniStreakTargetMaximum = 8;

struct LegacyAniStreakSlot {
    compat::i16 fixed_x{};
    compat::i16 fixed_y{};
    compat::i16 horizontal_step{};
    compat::i16 vertical_step{};
    compat::i16 trail_limit{};
    compat::i16 remaining_frames{};
    compat::i16 field_c{};
    compat::i16 active_flags{};
};

static_assert(sizeof(LegacyAniStreakSlot) == 0x10U);

struct LegacyAniStreakState {
    std::array<LegacyAniStreakSlot, kLegacyAniStreakSlotCount> slots{};
    compat::i16 previous_live_count{};
    compat::i16 target_spawn_count{};
};

class LegacyAniStreakServicePort {
public:
    virtual ~LegacyAniStreakServicePort() = default;

    [[nodiscard]] virtual bool service_enabled(compat::u32 service_id) = 0;
};

enum class LegacyAniStreakStatus {
    ready,
    framebuffer_too_small,
};

struct LegacyAniStreakResult {
    LegacyAniStreakStatus status{LegacyAniStreakStatus::ready};
    bool scanned_slots{};
    compat::u32 service_query_count{};
    compat::u32 created_count{};
    compat::u32 visited_active_count{};
    compat::u32 packed_color_count{};
    compat::u32 adjusted_pixel_count{};
    compat::u32 pixel_failure_count{};
};

class LegacyAniStreakEffect final {
public:
    LegacyAniStreakEffect() noexcept;

    void reset() noexcept;

    [[nodiscard]] LegacyAniStreakResult update(
        input_time_rng::LegacySecondaryRng& random,
        rendering::LegacyFramebuffer& framebuffer,
        const rendering::LegacyPixelConversionState& pixel_format,
        LegacyAniStreakServicePort& services
    ) noexcept;

    [[nodiscard]] LegacyAniStreakState& state() noexcept;
    [[nodiscard]] const LegacyAniStreakState& state() const noexcept;

private:
    LegacyAniStreakState state_;
};

}  // namespace openswd3::asset_runtime
