#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

#include "core/types.h"

// The parts of the statically linked MSVC runtime whose results the game can observe.
namespace swd3::crt
{

i64 Ftol(double value);
double Difftime(i32 end, i32 start);
// time(): seconds since 1970-01-01 UTC (the original converts the local time back through the time zone).
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

// swd32.exe 0x00489D90: vsprintf() as the game uses it: numbers, floating values and strings (std::string arguments)
// into its 256-byte buffers; the original writes past them for longer texts, here the text is cut.
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

// rand() and srand() of the single-threaded runtime: one state (0x004A833C, initially 1).
class Random
{
public:
    static constexpr u32 InitialSeed = 1;

    void Seed(u32 seed);
    int Next();

private:
    u32 holdrand = Random::InitialSeed;
};

}  // namespace swd3::crt
