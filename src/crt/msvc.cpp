#include "crt/msvc.h"

#include <cmath>
#include <cstdlib>
#include <ctime>

namespace swd3::crt
{

namespace
{

constexpr int kRandMask = 0x7FFF;

}  // namespace

// swd32.exe 0x00489654: _ftol
i64 Ftol(double value)
{
    return static_cast<i64>(std::llrint(std::trunc(value)));
}

// swd32.exe 0x0048A1F0: floor with its math error handlers below
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

// swd32.exe 0x00489B50: difftime
double Difftime(i32 end, i32 start)
{
    return std::difftime(static_cast<std::time_t>(end), static_cast<std::time_t>(start));
}

// swd32.exe 0x00489B70: time
i32 Time()
{
    return static_cast<i32>(std::time(nullptr));
}

// swd32.exe 0x00489B10: srand
void Random::Seed(u32 seed)
{
    std::srand(seed);
}

// swd32.exe 0x00489B20: rand, in the 15 bits the game's arithmetic assumes (RAND_MAX of the Windows runtime)
int Random::Next()
{
    return std::rand() & kRandMask;
}

}  // namespace swd3::crt
