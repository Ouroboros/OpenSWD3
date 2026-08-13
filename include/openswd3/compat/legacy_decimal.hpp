#pragma once

#include "openswd3/compat/types.hpp"

#include <string_view>

namespace openswd3::compat {

enum class LegacyDecimalParseResult {
    success,
    invalid,
    legacy_fault_no_digits,
};

[[nodiscard]] LegacyDecimalParseResult
parse_legacy_decimal_contract(std::string_view bytes, i32& output) noexcept;

[[nodiscard]] bool
parse_legacy_decimal_or_terminate(std::string_view bytes, i32& output) noexcept;

}  // namespace openswd3::compat
