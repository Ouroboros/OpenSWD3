#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

#include "core/types.h"

// The statically linked MSVC runtime calls of the game, each a thin wrapper over the current C runtime (user
// requirement 2026-10-18: the current runtime's results are used, the original runtime is not rewritten).
namespace swd3::crt
{

// swd32.exe 0x00489654 _ftol: truncation towards zero.
i64 Ftol(double value);
// swd32.exe 0x0048A1F0 floor.
double Floor(double value);
// swd32.exe 0x00489B50 difftime.
double Difftime(i32 end, i32 start);
// swd32.exe 0x00489B70 time.
i32 Time();

namespace detail
{

template <typename T>
auto PrintfArgument(const T& value)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return value.c_str();
    }
    else
    {
        return value;
    }
}

}  // namespace detail

// swd32.exe 0x00489D90 vsprintf into the game's 256-byte buffers (std::string arguments as %s); the original writes
// past them for longer texts, here the text is cut.
template <typename... Args>
std::string Sprintf(std::string_view format, const Args&... args)
{
    std::array<char, 0x100> buffer = {};
    const std::string pattern(format);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    std::snprintf(buffer.data(), buffer.size(), pattern.c_str(), detail::PrintfArgument(args)...);
#pragma clang diagnostic pop
    return std::string(buffer.data());
}

// swd32.exe 0x00489B10 srand and 0x00489B20 rand.
class Random
{
public:
    void Seed(u32 seed);
    int Next();
};

}  // namespace swd3::crt
