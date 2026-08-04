#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>

namespace openswd3::input_time_rng {

inline constexpr std::size_t kLegacySecondaryRngWordCount = 250U;

class LegacySecondaryRng final {
public:
    void seed(compat::u32 value) noexcept;

    [[nodiscard]] compat::u32 next_raw() noexcept;
    [[nodiscard]] compat::u32 next_bounded(
        compat::u32 upper_bound
    ) noexcept;

    [[nodiscard]] const std::array<
        compat::u32,
        kLegacySecondaryRngWordCount
    >& state_words() const noexcept;
    [[nodiscard]] std::size_t index() const noexcept;
    [[nodiscard]] compat::u32 seed_generator_state() const noexcept;

private:
    [[nodiscard]] compat::u32 next_seed_word() noexcept;

    std::array<compat::u32, kLegacySecondaryRngWordCount> state_words_{};
    std::size_t index_{};
    compat::u32 seed_generator_state_{};
};

}  // namespace openswd3::input_time_rng
