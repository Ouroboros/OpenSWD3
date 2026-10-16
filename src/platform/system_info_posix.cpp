#include "platform/system_info.h"

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

}  // namespace swd3
