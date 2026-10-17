#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

#include "core/types.h"

namespace swd3
{

// An argument of WinFormat: an integer (for %d, %i, %u, %x, %X, %c) or a string (for %s). A char string is game text
// (Big5 or ASCII); a u8 string is UTF-8. Each is converted to the encoding of the output.
class FormatArgument
{
public:
    FormatArgument(i32 value);
    FormatArgument(u32 value);
    FormatArgument(i64 value);
    FormatArgument(std::string_view text);
    FormatArgument(const std::string& text);
    FormatArgument(std::u8string_view text);
    FormatArgument(const std::u8string& text);

    template <std::size_t N>
    FormatArgument(const char (&text)[N])
        : FormatArgument(std::string_view(text, N - 1))
    {
    }

    template <std::size_t N>
    FormatArgument(const char8_t (&text)[N])
        : FormatArgument(std::u8string_view(text, N - 1))
    {
    }

    bool IsText() const;
    i64 Number() const;
    std::string Big5Text() const;
    std::string Utf8Text() const;

private:
    bool text = false;
    bool utf8 = false;
    i64 number = 0;
    std::string string;
};

// USER32 wsprintfA as the game uses it: %[-][0][width][.precision] with c, d, i, u, x, X and s (an h or l size prefix
// is accepted), "%%" for a percent sign; the result is cut to 1023 characters like the 1024-byte limit of wsprintfA.
std::string WinFormat(std::string_view format, std::initializer_list<FormatArgument> arguments);
// A UTF-8 format, the result in Big5 (game text).
std::string WinFormat(std::u8string_view format, std::initializer_list<FormatArgument> arguments);
// The same with the result in UTF-8 (messages, logs).
std::u8string Utf8Format(std::u8string_view format, std::initializer_list<FormatArgument> arguments);
// A path from an ASCII format of the game ("%sdata\\%d.cm"), in UTF-8: game text arguments (Big5) are converted.
std::u8string PathFormat(std::string_view format, std::initializer_list<FormatArgument> arguments);

// ASCII text or a system message taken as UTF-8.
std::u8string Utf8Bytes(std::string_view text);

// The strings one after the other.
std::string Concat(std::initializer_list<std::string_view> parts);
// UTF-8 strings (paths) one after the other.
std::u8string Concat(std::initializer_list<std::u8string_view> parts);

// The bytes of UTF-8 text for char interfaces that take UTF-8 (POSIX paths, SDL).
inline std::string_view AsBytes(std::u8string_view text)
{
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

}  // namespace swd3
