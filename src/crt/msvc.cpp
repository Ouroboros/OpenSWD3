#include "crt/msvc.h"

#include <cmath>
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

}  // namespace

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
