#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace openswd3::input_time_rng {
class LegacySecondaryRng;
}

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyAniRowCopyStateCount = 64U;
inline constexpr std::size_t kLegacyAniRowCopyActiveCount = 48U;
inline constexpr compat::u32 kLegacyAniRowCopyServiceId = 7U;

struct LegacyAniRowCopyState {
    std::array<compat::u32, kLegacyAniRowCopyStateCount> pixel_offsets{};
    std::array<compat::i16, kLegacyAniRowCopyStateCount> copy_width_bytes{};
    std::array<compat::i16, kLegacyAniRowCopyStateCount> copy_row_counts{};
    compat::u16 frame_counter{};
};

enum class LegacyAniRowCopyStatus {
    ready,
    disabled,
    framebuffer_too_small,
};

enum class LegacyAniRowCopyRefresh {
    none,
    initialized,
    widths,
    offsets,
    row_counts,
};

struct LegacyAniRowCopyResult {
    LegacyAniRowCopyStatus status{LegacyAniRowCopyStatus::ready};
    LegacyAniRowCopyRefresh refresh{LegacyAniRowCopyRefresh::none};
    compat::u32 copied_rows{};
};

class LegacyAniRowCopyEffect final {
public:
    LegacyAniRowCopyEffect() noexcept;

    void reset() noexcept;
    [[nodiscard]] LegacyAniRowCopyResult update(
        bool enabled,
        std::span<compat::u8> framebuffer,
        input_time_rng::LegacySecondaryRng& random
    ) noexcept;

    [[nodiscard]] LegacyAniRowCopyState& state() noexcept;
    [[nodiscard]] const LegacyAniRowCopyState& state() const noexcept;

private:
    LegacyAniRowCopyState state_;
};

}  // namespace openswd3::asset_runtime
