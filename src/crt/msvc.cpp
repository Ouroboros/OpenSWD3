#include "crt/msvc.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>

namespace swd3::crt
{

namespace
{

constexpr double kInt64Limit = 9223372036854775808.0;  // 2 to the 63rd
constexpr u32 kRandMultiplier = 0x343FD;               // the rand() LCG: 15-bit values from bit kRandShift
constexpr u32 kRandIncrement = 0x269EC3;
constexpr int kRandShift = 16;
constexpr u32 kRandMask = 0x7FFF;
constexpr int kSignificantDigits = 17;  // _fltout2: the mantissa digits of a double
constexpr int kDefaultPrecision = 6;
constexpr int kDecimalBase = 10;

}  // namespace

namespace detail
{

// 0x0048D630 _cftof with 0x00493540 _fltout2 and 0x00493440 _fptostr
std::string FormatFixed(std::string_view spec, double value)
{
    bool left = false;
    bool plus = false;
    bool space = false;
    bool zeros = false;
    std::size_t at = 1;

    while (at < spec.size() && std::strchr("-+ #0", spec[at]) != nullptr)
    {
        left = left || spec[at] == '-';
        plus = plus || spec[at] == '+';
        space = space || spec[at] == ' ';
        zeros = zeros || spec[at] == '0';
        ++at;
    }

    int width = 0;

    while (at < spec.size() && spec[at] >= '0' && spec[at] <= '9')
    {
        width = width * kDecimalBase + (spec[at] - '0');
        ++at;
    }

    int precision = kDefaultPrecision;

    if (at < spec.size() && spec[at] == '.')
    {
        precision = 0;
        ++at;

        while (at < spec.size() && spec[at] >= '0' && spec[at] <= '9')
        {
            precision = precision * kDecimalBase + (spec[at] - '0');
            ++at;
        }
    }

    const double magnitude = std::fabs(value);
    std::string mantissa;
    int point = 0;

    if (magnitude != 0.0)
    {
        std::array<char, 0x40> scientific = {};
        std::snprintf(scientific.data(), scientific.size(), "%.*e", kSignificantDigits - 1, magnitude);
        mantissa.push_back(scientific[0]);
        mantissa.append(scientific.data() + 2, static_cast<std::size_t>(kSignificantDigits - 1));
        point = std::atoi(std::strchr(scientific.data(), 'e') + 1) + 1;
    }

    // _fptostr: a leading carry digit, the kept digits padded with zeros, then half up unless nothing is kept
    const int keep = point + precision;
    std::string digits = "0";

    for (int i = 0; i < keep; ++i)
    {
        digits.push_back(static_cast<std::size_t>(i) < mantissa.size() ? mantissa[static_cast<std::size_t>(i)] : '0');
    }

    const char next =
        keep >= 0 && static_cast<std::size_t>(keep) < mantissa.size() ? mantissa[static_cast<std::size_t>(keep)] : '\0';

    if (keep >= 0 && next >= '5')
    {
        std::size_t carry = digits.size() - 1;

        while (digits[carry] == '9')
        {
            digits[carry--] = '0';
        }

        ++digits[carry];
    }

    if (digits[0] == '1')
    {
        ++point;
    }
    else
    {
        digits.erase(0, 1);
    }

    std::string body = point <= 0 ? std::string("0") : digits.substr(0, static_cast<std::size_t>(point));

    if (precision > 0)
    {
        body.push_back('.');

        if (point < 0)
        {
            body.append(static_cast<std::size_t>(std::min(-point, precision)), '0');
        }

        body += digits.substr(static_cast<std::size_t>(std::max(point, 0)));
    }

    std::string sign;

    if (std::signbit(value))
    {
        sign = "-";
    }
    else if (plus)
    {
        sign = "+";
    }
    else if (space)
    {
        sign = " ";
    }

    const auto length = static_cast<int>(sign.size() + body.size());
    const auto pad = static_cast<std::size_t>(std::max(width - length, 0));

    if (left)
    {
        return sign + body + std::string(pad, ' ');
    }

    if (zeros)
    {
        return sign + std::string(pad, '0') + body;
    }

    return std::string(pad, ' ') + sign + body;
}

std::string TakeLiteral(std::string_view format, std::size_t& at)
{
    std::string text;

    while (at < format.size())
    {
        if (format[at] != '%')
        {
            text.push_back(format[at++]);
            continue;
        }

        if (at + 1 < format.size() && format[at + 1] == '%')
        {
            text.push_back('%');
            at += 2;
            continue;
        }

        break;
    }

    return text;
}

std::string_view TakeSpec(std::string_view format, std::size_t& at)
{
    if (at >= format.size() || format[at] != '%')
    {
        return {};
    }

    const std::size_t start = at++;

    while (at < format.size() && std::strchr("-+ #0123456789.hlL", format[at]) != nullptr)
    {
        ++at;
    }

    if (at < format.size())
    {
        ++at;
    }

    return format.substr(start, at - start);
}

}  // namespace detail

// swd32.exe 0x00489654: _ftol, truncation towards zero by fistp in the round-to-zero mode with a 64-bit result; NaN
// and values outside the 64-bit range give the integer indefinite value
i64 Ftol(double value)
{
    if (std::isnan(value) || value >= kInt64Limit || value < -kInt64Limit)
    {
        return std::numeric_limits<i64>::min();
    }

    return static_cast<i64>(value);
}

// swd32.exe 0x0048A1F0: floor, frndint in the round-down mode; an infinity or a NaN goes through the math error
// handlers below and comes back as it is (only errno, which the game never reads, is set), so std::floor gives the same
// swd32.exe 0x0048F9A0: _frnd, frndint in the current rounding mode (inline here)
// swd32.exe 0x0048F9C0: _handle_qnan1, a quiet NaN argument (EDOM) (inline here)
// swd32.exe 0x0048FA20: _except1, a floating exception of a math function (inline here)
// swd32.exe 0x0048FAE0: _raise_exc, the IEEE exception record raised (inline here)
// swd32.exe 0x0048FFB0: _handle_exc, the masked exceptions handled (inline here)
// swd32.exe 0x00490330: _umatherr, the user _matherr called (inline here)
// swd32.exe 0x004903D0: _set_errno, EDOM or ERANGE (inline here)
// swd32.exe 0x00490410: _getfpename, the function name from the table at 0x004A85E0 (inline here)
// swd32.exe 0x00490450: _errcode, the _matherr type of the exception flags (inline here)
// swd32.exe 0x004904D0: _set_exp, a double with its exponent replaced (inline here)
// swd32.exe 0x00490510: _sptype, infinity or NaN kind of a double (inline here)
// swd32.exe 0x00490590: _decomp, mantissa and exponent of a double (inline here)
// swd32.exe 0x004906D0: _statfp, the FPU status word (inline here)
// swd32.exe 0x004906F0: _clrfp, the FPU status word read and cleared (inline here)
// swd32.exe 0x00490710: _ctrlfp, the FPU control word set under a mask (inline here)
// swd32.exe 0x00490750: _set_statfp, an exception flag raised (inline here)
// swd32.exe 0x00494800: _matherr, the default handler (0: not handled) (inline here)
double Floor(double value)
{
    return std::floor(value);
}

// swd32.exe 0x00489B50: difftime, the 32-bit difference as a double
double Difftime(i32 end, i32 start)
{
    return static_cast<double>(static_cast<i32>(static_cast<u32>(end) - static_cast<u32>(start)));
}

// swd32.exe 0x00489B70
i32 Time()
{
    return static_cast<i32>(std::time(nullptr));
}

// swd32.exe 0x00489B10
void Random::Seed(u32 seed)
{
    this->holdrand = seed;
}

// swd32.exe 0x00489B20
int Random::Next()
{
    this->holdrand = kRandMultiplier * this->holdrand + kRandIncrement;
    return static_cast<int>((this->holdrand >> kRandShift) & kRandMask);
}

}  // namespace swd3::crt
