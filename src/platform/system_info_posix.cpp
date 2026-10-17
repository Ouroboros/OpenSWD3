#include "platform/system_info.h"

#include <ctime>
#include <limits>

#include <SDL3/SDL.h>

namespace swd3
{

namespace
{

constexpr int kMegabyteShift = 20;  // SDL_GetSystemRAM counts MiB
constexpr u64 kMillisecondsPerSecond = 1000;
constexpr u64 kNanosecondsPerMillisecond = 1000000;
constexpr int kTmYearBase = 1900;  // std::tm::tm_year counts from it
constexpr int kTmMonthBase = 1;    // std::tm::tm_mon counts from 0

}  // namespace

std::vector<char> CdRomDrives()
{
    return {};
}

u32 TotalPhysicalMemory()
{
    const u64 bytes = static_cast<u64>(SDL_GetSystemRAM()) << kMegabyteShift;
    return bytes > std::numeric_limits<u32>::max() ? std::numeric_limits<u32>::max() : static_cast<u32>(bytes);
}

u32 SystemUptime()
{
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<u32>(
        static_cast<u64>(now.tv_sec) * kMillisecondsPerSecond +
        static_cast<u64>(now.tv_nsec) / kNanosecondsPerMillisecond
    );
}

void SleepMilliseconds(u32 milliseconds)
{
    SDL_Delay(milliseconds);
}

LocalDateTime LocalTime()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    return {local.tm_year + kTmYearBase, local.tm_mon + kTmMonthBase, local.tm_mday, local.tm_hour, local.tm_min};
}

}  // namespace swd3
