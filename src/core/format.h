#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

#include "core/types.h"

namespace swd3
{

// An argument of WinFormat: an integer (for %d, %i, %u, %x, %X, %c) or a string (for %s).
class FormatArgument
{
public:
    FormatArgument(i32 value);
    FormatArgument(u32 value);
    FormatArgument(i64 value);
    FormatArgument(const char* text);
    FormatArgument(std::string_view text);

    bool IsText() const;
    i64 Number() const;
    std::string_view Text() const;

private:
    bool text = false;
    i64 number = 0;
    std::string_view string;
};

// USER32 wsprintfA as the game uses it: %[-][0][width][.precision] with c, d, i, u, x, X and s (an h or l size prefix
// is accepted), "%%" for a percent sign; the result is cut to 1023 characters like the 1024-byte limit of wsprintfA.
std::string WinFormat(std::string_view format, std::initializer_list<FormatArgument> arguments);

}  // namespace swd3
