#include "openswd3/input_time_rng/legacy_crt_rng.hpp"

namespace openswd3::input_time_rng {
namespace {

constexpr compat::u32 kMultiplier = 0x000343FDU;
constexpr compat::u32 kIncrement = 0x00269EC3U;
constexpr compat::u32 kOutputMask = 0x00007FFFU;

}  // namespace

void LegacyCrtRng::seed(const compat::u32 value) noexcept {
    state_ = value;
}

compat::u32 LegacyCrtRng::next() noexcept {
    state_ = state_ * kMultiplier + kIncrement;
    return (state_ >> 16U) & kOutputMask;
}

compat::u32 LegacyCrtRng::state() const noexcept {
    return state_;
}

}  // namespace openswd3::input_time_rng
