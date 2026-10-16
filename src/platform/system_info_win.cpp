#include "platform/system_info.h"

#include <array>
#include <limits>

#include <SDL3/SDL.h>
#include <windows.h>

namespace swd3
{

namespace
{

constexpr int kMegabyteShift = 20;  // SDL_GetSystemRAM counts MiB

}  // namespace

std::vector<char> CdRomDrives()
{
    std::vector<char> drives;
    std::array root = {'a', ':', '\\', '\0'};

    for (char letter = 'a'; letter <= 'z'; ++letter)
    {
        root[0] = letter;

        if (::GetDriveTypeA(root.data()) == DRIVE_CDROM)
        {
            drives.push_back(letter);
        }
    }

    return drives;
}

u32 TotalPhysicalMemory()
{
    const u64 bytes = static_cast<u64>(SDL_GetSystemRAM()) << kMegabyteShift;
    return bytes > std::numeric_limits<u32>::max() ? std::numeric_limits<u32>::max() : static_cast<u32>(bytes);
}

u32 SystemUptime()
{
    return static_cast<u32>(::GetTickCount());
}

void SleepMilliseconds(u32 milliseconds)
{
    SDL_Delay(milliseconds);
}

}  // namespace swd3
