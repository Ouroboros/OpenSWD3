#pragma once

#include "core/types.h"

// The parts of the statically linked MSVC runtime whose results the game can observe.
namespace swd3::crt
{

i64 Ftol(double value);
double Difftime(i32 end, i32 start);
// time(): seconds since 1970-01-01 UTC (the original converts the local time back through the time zone).
i32 Time();

// rand() and srand() of the single-threaded runtime: one state (0x004A833C, initially 1).
class Random
{
public:
    static constexpr u32 kInitialSeed = 1;

    void Seed(u32 seed);
    int Next();

private:
    u32 holdrand = Random::kInitialSeed;
};

}  // namespace swd3::crt
