#include "platform/system_info.h"

#include <array>

#include <SDL3/SDL.h>
#include <windows.h>

namespace swd3
{

std::vector<char> CdRomDrives()
{
    std::vector<char> drives;
    std::array<char, 4> root = {'a', ':', '\\', '\0'};

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
    const u64 bytes = static_cast<u64>(SDL_GetSystemRAM()) << 20;
    return bytes > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<u32>(bytes);
}

}  // namespace swd3
