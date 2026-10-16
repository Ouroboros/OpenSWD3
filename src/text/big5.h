#pragma once

#include <string>
#include <string_view>

namespace swd3
{

// The game's text is Big5 (code page 950); the source writes it as u8 literals and converts where it is used.
// Characters code page 950 lacks are dropped.
std::string Big5(std::u8string_view text);
// UTF-8 in chars (typed text from SDL) to Big5.
std::string Big5FromUtf8(std::string_view text);
// Big5 to UTF-8 in chars, for the system (window titles, message boxes); a byte with no character becomes '?'.
std::string Utf8FromBig5(std::string_view text);
// A u8 literal as chars for the system.
std::string Utf8Chars(std::u8string_view text);

// An argument of a game-text formatter: UTF-8 text becomes Big5, anything else passes unchanged.
template <typename T>
const T& GameArgument(const T& value)
{
    return value;
}

inline std::string GameArgument(std::u8string_view text)
{
    return Big5(text);
}

template <std::size_t N>
std::string GameArgument(const char8_t (&text)[N])
{
    return Big5(std::u8string_view(text, N - 1));
}

}  // namespace swd3
