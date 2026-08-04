#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::input_time_rng {

class LegacyCrtRng final {
public:
    void seed(compat::u32 value) noexcept;
    [[nodiscard]] compat::u32 next() noexcept;
    [[nodiscard]] compat::u32 state() const noexcept;

private:
    compat::u32 state_{1U};
};

}  // namespace openswd3::input_time_rng
