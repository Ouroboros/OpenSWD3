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

inline constexpr compat::u32 kLegacyAniSparkServiceId = 0x16U;
inline constexpr std::size_t kLegacyAniSparkSlotCount = 96U;
inline constexpr compat::i16 kLegacyAniSparkTargetMaximum = 1;

struct LegacyAniSparkSlot {
    compat::i16 fixed_x{};
    compat::i16 fixed_y{};
    compat::i16 horizontal_step{};
    compat::i16 vertical_step{};
    compat::i16 point_count{};
    compat::i16 remaining_height{};
    compat::i16 phase{};
    compat::i16 active_flags{};
};

static_assert(sizeof(LegacyAniSparkSlot) == 0x10U);

struct LegacyAniSparkState {
    std::array<LegacyAniSparkSlot, kLegacyAniSparkSlotCount> slots{};
    compat::i16 previous_live_count{};
    compat::i16 target_spawn_count{};
};

class LegacyAniSparkServicePort {
public:
    virtual ~LegacyAniSparkServicePort() = default;

    [[nodiscard]] virtual bool service_enabled(compat::u32 service_id) = 0;
};

enum class LegacyAniSparkStatus {
    ready,
    framebuffer_too_small,
};

struct LegacyAniSparkResult {
    LegacyAniSparkStatus status{LegacyAniSparkStatus::ready};
    bool scanned_slots{};
    compat::u32 service_query_count{};
    compat::u32 created_count{};
    compat::u32 visited_active_count{};
    compat::u32 packed_color_count{};
    compat::u32 adjusted_pixel_count{};
    compat::u32 pixel_failure_count{};
    compat::u32 invalid_phase_count{};
};

class LegacyAniSparkEffect final {
public:
    LegacyAniSparkEffect() noexcept = default;

    // The original scene initializer clears only these two counters. The
    // 96-slot pool is loader-zeroed once and is deliberately left intact.
    void reset_counters() noexcept;

    [[nodiscard]] LegacyAniSparkResult update(
        input_time_rng::LegacySecondaryRng& random,
        rendering::LegacyFramebuffer& framebuffer,
        const rendering::LegacyPixelConversionState& pixel_format,
        LegacyAniSparkServicePort& services
    ) noexcept;

    [[nodiscard]] LegacyAniSparkState& state() noexcept;
    [[nodiscard]] const LegacyAniSparkState& state() const noexcept;

private:
    LegacyAniSparkState state_{};
};

}  // namespace openswd3::asset_runtime
