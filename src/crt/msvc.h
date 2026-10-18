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
double Floor(double value);
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

// One %f conversion as the original's _cftof writes it: the value to 17 significant digits (_fltout2), then rounded
// half up at the precision (_fptostr), so an exact tie such as 2.25 at one place gives 2.3.
std::string FormatFixed(std::string_view spec, double value);

// One conversion `spec` ("%...c") of `value`.
template <typename T>
std::string FormatOne(std::string_view spec, const T& value)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        if (!spec.empty() && spec.back() == 'f')
        {
            return FormatFixed(spec, static_cast<double>(value));
        }
    }

    std::array<char, 0x100> buffer = {};
    const std::string pattern(spec);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    std::snprintf(buffer.data(), buffer.size(), pattern.c_str(), PrintfArgument(value));
#pragma clang diagnostic pop
    return std::string(buffer.data());
}

// The literal text of `format` from `at` up to its next conversion ("%%" written as one '%'); `at` is left on the '%'.
std::string TakeLiteral(std::string_view format, std::size_t& at);

// The conversion at `at` (flags, width, precision, size and type), `at` moved past it.
std::string_view TakeSpec(std::string_view format, std::size_t& at);

}  // namespace detail

// swd32.exe 0x00489D90: vsprintf() as the game uses it, one conversion per argument: numbers and strings (std::string
// arguments) as printf writes them, %f as the original rounds it (detail::FormatFixed),
// into its 256-byte buffers; the original writes past them for longer texts, here the text is cut.
template <typename... Args>
std::string Sprintf(std::string_view format, const Args&... args)
{
    constexpr std::size_t kBufferChars = 0xFF;
    std::string text;
    std::size_t at = 0;

    auto convert = [&](const auto& value)
    {
        text += detail::TakeLiteral(format, at);
        const std::string_view spec = detail::TakeSpec(format, at);

        if (!spec.empty())
        {
            text += detail::FormatOne(spec, value);
        }
    };

    (convert(args), ...);
    text += detail::TakeLiteral(format, at);

    if (text.size() > kBufferChars)
    {
        text.resize(kBufferChars);
    }

    return text;
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
