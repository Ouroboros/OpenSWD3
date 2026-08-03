#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::compat {

[[nodiscard]] constexpr i32 legacy_zero_result() noexcept {
    return 0;
}

[[nodiscard]] constexpr i32 legacy_true_result() noexcept {
    return 1;
}

inline void legacy_noop() noexcept {}

}  // namespace openswd3::compat
