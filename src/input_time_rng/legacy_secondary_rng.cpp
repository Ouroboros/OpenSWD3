#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"

#include <exception>

namespace openswd3::input_time_rng {
namespace {

constexpr compat::u32 kSeedMultiplier = 0x015A4E35U;
constexpr compat::u32 kSeedOutputMask = 0x00007FFFU;
constexpr compat::u32 kStateOutputRange = 0x0000FFFFU;
constexpr compat::u32 kHighHalfThreshold = 0x00004000U;
constexpr compat::u32 kHighHalfBit = 0x00008000U;
constexpr std::size_t kXorLag = 147U;
constexpr std::size_t kXorForwardOffset = 103U;

}  // namespace

void LegacySecondaryRng::seed(const compat::u32 value) noexcept {
    seed_generator_state_ = value;
    index_ = 0U;

    for (compat::u32& word : state_words_) {
        word = next_seed_word();
    }

    for (compat::u32& word : state_words_) {
        if (next_seed_word() > kHighHalfThreshold) {
            word |= kHighHalfBit;
        }
    }

    compat::u32 retained_mask = kStateOutputRange;
    compat::u32 forced_bit = kHighHalfBit;
    for (std::size_t word = 3U; word < 179U; word += 11U) {
        state_words_[word] &= retained_mask;
        state_words_[word] |= forced_bit;
        retained_mask >>= 1U;
        forced_bit >>= 1U;
    }
}

compat::u32 LegacySecondaryRng::next_raw() noexcept {
    const std::size_t other =
        index_ >= kXorLag ? index_ - kXorLag : index_ + kXorForwardOffset;
    state_words_[index_] ^= state_words_[other];
    const compat::u32 value = state_words_[index_];

    if (index_ == kLegacySecondaryRngWordCount - 1U) {
        index_ = 0U;
    } else {
        ++index_;
    }

    return value;
}

compat::u32
LegacySecondaryRng::next_bounded(const compat::u32 upper_bound) noexcept {
    if (upper_bound == 0U) {
        std::terminate();
    }

    const compat::u32 acceptance_limit =
        (kStateOutputRange / upper_bound) * upper_bound;
    for (;;) {
        static_cast<void>(next_raw());
        const compat::u32 value = next_raw();
        if (value < acceptance_limit) {
            return value % upper_bound;
        }
    }
}

const std::array<compat::u32, kLegacySecondaryRngWordCount>&
LegacySecondaryRng::state_words() const noexcept {
    return state_words_;
}

std::size_t LegacySecondaryRng::index() const noexcept {
    return index_;
}

compat::u32 LegacySecondaryRng::seed_generator_state() const noexcept {
    return seed_generator_state_;
}

compat::u32 LegacySecondaryRng::next_seed_word() noexcept {
    seed_generator_state_ = seed_generator_state_ * kSeedMultiplier + 1U;
    return (seed_generator_state_ >> 16U) & kSeedOutputMask;
}

}  // namespace openswd3::input_time_rng
