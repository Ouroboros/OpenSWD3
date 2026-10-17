#include "platform/system_info.h"

#include <csignal>
#include <ctime>
#include <initializer_list>
#include <limits>

#include <SDL3/SDL.h>
#include <unistd.h>

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

u32 ProcessId()
{
    return static_cast<u32>(getpid());
}

namespace
{

// A fault signal: logged, then the default action ends the process.
void FaultSignal(int signal)
{
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "fault signal %d ends the process", signal);
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

}  // namespace

void InstallFaultLog()
{
    for (const int signal : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT})
    {
        std::signal(signal, &FaultSignal);
    }
}

}  // namespace swd3
