#include "platform/system_info.h"

#include <ctime>

#include <SDL3/SDL.h>

namespace swd3
{

std::vector<char> CdRomDrives()
{
    return {};
}

u32 TotalPhysicalMemory()
{
    const u64 bytes = static_cast<u64>(SDL_GetSystemRAM()) << 20;
    return bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<u32>(bytes);
}

u32 SystemUptime()
{
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<u32>(static_cast<u64>(now.tv_sec) * 1000 + static_cast<u64>(now.tv_nsec) / 1000000);
}

void SleepMilliseconds(u32 milliseconds)
{
    SDL_Delay(milliseconds);
}

}  // namespace swd3
