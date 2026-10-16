#include "crt/msvc.h"

#include <cmath>
#include <ctime>
#include <limits>

namespace swd3::crt
{

// swd32.exe 0x00489654: _ftol, truncation towards zero by fistp in the round-to-zero mode with a 64-bit result; NaN
// and values outside the 64-bit range give the integer indefinite value
i64 Ftol(double value)
{
    if (std::isnan(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0)
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
    this->holdrand = 0x343FD * this->holdrand + 0x269EC3;
    return static_cast<int>((this->holdrand >> 16) & 0x7FFF);
}

}  // namespace swd3::crt
