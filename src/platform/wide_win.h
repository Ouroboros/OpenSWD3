#pragma once

#include <string>
#include <string_view>

namespace swd3
{

// UTF-8 text (paths, messages) as the UTF-16 the W functions of Win32 take; invalid bytes become U+FFFD.
std::wstring Utf16(std::u8string_view text);
// UTF-16 a W function of Win32 returned, as UTF-8.
std::u8string Utf8(std::wstring_view text);

// The bytes of UTF-8 text for the char interfaces that take UTF-8 (SDL).
inline const char* Chars(const std::u8string& text)
{
    return reinterpret_cast<const char*>(text.c_str());
}

}  // namespace swd3
