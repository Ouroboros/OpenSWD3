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

LocalDateTime LocalTime()
{
    SYSTEMTIME now{};
    ::GetLocalTime(&now);
    const auto [year, month, dayOfWeek, day, hour, minute, second, milliseconds] = now;
    return {year, month, day, hour, minute};
}

u32 ProcessId()
{
    return static_cast<u32>(::GetCurrentProcessId());
}

namespace
{

// An unhandled structured exception: its code and address (with the executable's base, to find it in the map) logged;
// the system then ends the process.
LONG WINAPI FaultFilter(EXCEPTION_POINTERS* exception)
{
    const EXCEPTION_RECORD& record = *exception->ExceptionRecord;
    SDL_LogCritical(
        SDL_LOG_CATEGORY_APPLICATION,
        "unhandled exception 0x%08lX at %p (executable at %p)",
        static_cast<unsigned long>(record.ExceptionCode),
        record.ExceptionAddress,
        static_cast<void*>(::GetModuleHandleW(nullptr))
    );
    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void InstallFaultLog()
{
    ::SetUnhandledExceptionFilter(&FaultFilter);
}

}  // namespace swd3
