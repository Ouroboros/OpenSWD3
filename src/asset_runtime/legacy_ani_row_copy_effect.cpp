#include "openswd3/asset_runtime/legacy_ani_row_copy_effect.hpp"

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <cstring>

namespace openswd3::asset_runtime {
namespace {

constexpr std::size_t kColumnCount = 8U;
constexpr std::size_t kBandCount = 6U;
constexpr compat::u32 kColumnStridePixels = 0x50U;
constexpr compat::u32 kBandStridePixels = 0xC800U;
constexpr compat::u32 kInitialPixelOffset = 0x27FU;
constexpr compat::u32 kPixelSizeBytes = 2U;
constexpr compat::u32 kWidthRandomBound = 0x26U;
constexpr compat::u32 kMinimumWidthBytes = 0x4CU;
constexpr compat::u32 kOffsetRandomBound = 3U;
constexpr compat::u32 kRowCountRandomBound = 0x4EU;

void refresh_widths(
    LegacyAniRowCopyState& state,
    input_time_rng::LegacySecondaryRng& random,
    const std::size_t count
) noexcept {
    for (std::size_t index = 0U; index < count; ++index) {
        state.copy_width_bytes[index] = static_cast<compat::i16>(
            random.next_bounded(kWidthRandomBound) * 2U +
            kMinimumWidthBytes
        );
    }
}

void refresh_offsets(
    LegacyAniRowCopyState& state,
    input_time_rng::LegacySecondaryRng& random
) noexcept {
    std::size_t index = 0U;
    for (std::size_t column = 0U; column < kColumnCount; ++column) {
        for (std::size_t band = 0U; band < kBandCount; ++band) {
            state.pixel_offsets[index] =
                random.next_bounded(kOffsetRandomBound) +
                static_cast<compat::u32>(band) * kBandStridePixels +
                static_cast<compat::u32>(column) * kColumnStridePixels +
                kInitialPixelOffset;
            ++index;
        }
    }
}

void refresh_row_counts(
    LegacyAniRowCopyState& state,
    input_time_rng::LegacySecondaryRng& random,
    const std::size_t count
) noexcept {
    for (std::size_t index = 0U; index < count; ++index) {
        state.copy_row_counts[index] = static_cast<compat::i16>(
            random.next_bounded(kRowCountRandomBound) + 1U
        );
    }
}

void initialize_active_state(
    LegacyAniRowCopyState& state,
    input_time_rng::LegacySecondaryRng& random
) noexcept {
    std::size_t index = 0U;
    for (std::size_t column = 0U; column < kColumnCount; ++column) {
        for (std::size_t band = 0U; band < kBandCount; ++band) {
            state.copy_width_bytes[index] = static_cast<compat::i16>(
                random.next_bounded(kWidthRandomBound) * 2U +
                kMinimumWidthBytes
            );
            state.pixel_offsets[index] =
                random.next_bounded(kOffsetRandomBound) +
                static_cast<compat::u32>(band) * kBandStridePixels +
                static_cast<compat::u32>(column) * kColumnStridePixels +
                kInitialPixelOffset;
            state.copy_row_counts[index] = static_cast<compat::i16>(
                random.next_bounded(kRowCountRandomBound) + 1U
            );
            ++index;
        }
    }
}

}  // namespace

LegacyAniRowCopyEffect::LegacyAniRowCopyEffect() noexcept {
    reset();
}

void LegacyAniRowCopyEffect::reset() noexcept {
    state_.pixel_offsets.fill(0U);
    state_.copy_width_bytes.fill(4);
    state_.copy_row_counts.fill(1);
    state_.frame_counter = 0U;
}

LegacyAniRowCopyResult LegacyAniRowCopyEffect::update(
    const bool enabled,
    const std::span<compat::u8> framebuffer,
    input_time_rng::LegacySecondaryRng& random
) noexcept {
    LegacyAniRowCopyResult result;
    if (!enabled) {
        result.status = LegacyAniRowCopyStatus::disabled;
        return result;
    }
    if (framebuffer.size() < rendering::kLegacyFixedCanvasBytes) {
        result.status = LegacyAniRowCopyStatus::framebuffer_too_small;
        return result;
    }

    if (state_.frame_counter == 0U) {
        initialize_active_state(state_, random);
        result.refresh = LegacyAniRowCopyRefresh::initialized;
    } else if ((state_.frame_counter & 3U) == 0U) {
        switch ((state_.frame_counter >> 2U) & 3U) {
        case 0U:
            refresh_widths(state_, random, kLegacyAniRowCopyStateCount);
            result.refresh = LegacyAniRowCopyRefresh::widths;
            break;
        case 1U:
            refresh_offsets(state_, random);
            result.refresh = LegacyAniRowCopyRefresh::offsets;
            break;
        case 2U:
            refresh_row_counts(
                state_, random, kLegacyAniRowCopyStateCount
            );
            result.refresh = LegacyAniRowCopyRefresh::row_counts;
            break;
        default:
            break;
        }
    }

    state_.frame_counter = static_cast<compat::u16>(
        static_cast<compat::u32>(state_.frame_counter) + 1U
    );

    for (std::size_t index = 0U;
         index < kLegacyAniRowCopyActiveCount; ++index) {
        const compat::i32 row_count = state_.copy_row_counts[index];
        if (row_count <= 0) {
            continue;
        }

        std::size_t destination =
            static_cast<std::size_t>(state_.pixel_offsets[index]) *
            kPixelSizeBytes;
        const std::size_t width = static_cast<std::size_t>(
            state_.copy_width_bytes[index]
        );
        for (compat::i32 row = 0; row < row_count; ++row) {
            std::memcpy(
                framebuffer.data() + destination,
                framebuffer.data() + destination +
                    rendering::kLegacyFramebufferPitchBytes,
                width
            );
            destination += rendering::kLegacyFramebufferPitchBytes;
            ++result.copied_rows;
        }
    }

    return result;
}

LegacyAniRowCopyState& LegacyAniRowCopyEffect::state() noexcept {
    return state_;
}

const LegacyAniRowCopyState& LegacyAniRowCopyEffect::state() const noexcept {
    return state_;
}

}  // namespace openswd3::asset_runtime
