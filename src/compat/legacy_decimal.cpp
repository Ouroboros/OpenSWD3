#include "openswd3/compat/legacy_decimal.hpp"

#include <cstdlib>

namespace openswd3::compat {

LegacyDecimalParseResult parse_legacy_decimal_contract(
    const std::string_view bytes, i32& output
) noexcept {
    const std::size_t nul = bytes.find('\0');
    const std::string_view text =
        nul == std::string_view::npos ? bytes : bytes.substr(0U, nul);

    std::size_t first_digit = 0U;
    i32 sign = 1;
    if (!text.empty() && (text.front() == '+' || text.front() == '-')) {
        if (text.front() == '-') {
            sign = -1;
        }
        first_digit = 1U;
    }

    const std::size_t digit_count = text.size() - first_digit;
    if (digit_count == 0U) {
        output = 0;
        return LegacyDecimalParseResult::legacy_fault_no_digits;
    }

    if (digit_count > 9U) {
        return LegacyDecimalParseResult::invalid;
    }
    for (std::size_t index = first_digit; index < text.size(); ++index) {
        const unsigned char byte = static_cast<unsigned char>(text[index]);
        if (byte < static_cast<unsigned char>('0') ||
            byte > static_cast<unsigned char>('9')) {
            return LegacyDecimalParseResult::invalid;
        }
    }

    i32 value = 0;
    i32 multiplier = sign;
    for (std::size_t index = text.size(); index > first_digit; --index) {
        const i32 digit = static_cast<i32>(
            static_cast<unsigned char>(text[index - 1U]) -
            static_cast<unsigned char>('0')
        );
        value += digit * multiplier;
        multiplier *= 10;
    }
    output = value;
    return LegacyDecimalParseResult::success;
}

bool parse_legacy_decimal_or_terminate(
    const std::string_view bytes, i32& output
) noexcept {
    switch (parse_legacy_decimal_contract(bytes, output)) {
    case LegacyDecimalParseResult::success:
        return true;
    case LegacyDecimalParseResult::invalid:
        return false;
    case LegacyDecimalParseResult::legacy_fault_no_digits:
        std::abort();
    }
    std::abort();
}

}  // namespace openswd3::compat
