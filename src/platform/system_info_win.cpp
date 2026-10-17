#include "platform/system_info.h"

#include <array>
#include <cstdint>
#include <cstdio>
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

constexpr std::size_t kModuleNameChars = 260;
constexpr std::size_t kStackScanWords = 0x800;  // the stack words searched for return addresses
constexpr int kStackEntries = 48;
constexpr DWORD kExecutablePages = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;

// "module+0xoffset" for an address inside a loaded module (its file name only), "?" otherwise.
std::array<char, kModuleNameChars + 0x20> Located(const void* address)
{
    std::array<char, kModuleNameChars + 0x20> text = {'?', '\0'};
    HMODULE module = nullptr;

    if (::GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            static_cast<LPCSTR>(address),
            &module
        ) == 0)
    {
        return text;
    }

    std::array<char, kModuleNameChars> path = {};
    ::GetModuleFileNameA(module, path.data(), static_cast<DWORD>(path.size()));
    const char* name = path.data();

    for (const char* at = path.data(); *at != '\0'; ++at)
    {
        if (*at == '\\' || *at == '/')
        {
            name = at + 1;
        }
    }

    std::snprintf(
        text.data(),
        text.size(),
        "%s+0x%lX",
        name,
        static_cast<unsigned long>(reinterpret_cast<std::uintptr_t>(address) - reinterpret_cast<std::uintptr_t>(module))
    );
    return text;
}

// Whether `address` lies in executable code of a loaded module (a return address candidate).
bool InCode(std::uintptr_t address)
{
    MEMORY_BASIC_INFORMATION page{};

    if (address == 0 || ::VirtualQuery(reinterpret_cast<const void*>(address), &page, sizeof(page)) == 0)
    {
        return false;
    }

    return page.State == MEM_COMMIT && page.Type == MEM_IMAGE && (page.Protect & kExecutablePages) != 0;
}

// An unhandled structured exception: its code, the module and offset of the faulting instruction, the access of an
// access violation, the registers and the return addresses found on the stack (module and offset each, to look up in
// the PDB) logged; the system then ends the process.
LONG WINAPI FaultFilter(EXCEPTION_POINTERS* exception)
{
    const EXCEPTION_RECORD& record = *exception->ExceptionRecord;
    SDL_LogCritical(
        SDL_LOG_CATEGORY_APPLICATION,
        "unhandled exception 0x%08lX at %p %s (executable at %p)",
        static_cast<unsigned long>(record.ExceptionCode),
        record.ExceptionAddress,
        Located(record.ExceptionAddress).data(),
        static_cast<void*>(::GetModuleHandleW(nullptr))
    );

    if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record.NumberParameters >= 2)
    {
        SDL_LogCritical(
            SDL_LOG_CATEGORY_APPLICATION,
            "access violation: %s address 0x%08lX",
            record.ExceptionInformation[0] == 0 ? "read" : (record.ExceptionInformation[0] == 1 ? "write" : "execute"),
            static_cast<unsigned long>(record.ExceptionInformation[1])
        );
    }

#if defined(_M_IX86)
    const CONTEXT& context = *exception->ContextRecord;
    SDL_LogCritical(
        SDL_LOG_CATEGORY_APPLICATION,
        "registers eax=%08lX ebx=%08lX ecx=%08lX edx=%08lX esi=%08lX edi=%08lX ebp=%08lX esp=%08lX eip=%08lX",
        context.Eax,
        context.Ebx,
        context.Ecx,
        context.Edx,
        context.Esi,
        context.Edi,
        context.Ebp,
        context.Esp,
        context.Eip
    );
    const auto* stack = reinterpret_cast<const std::uintptr_t*>(context.Esp);
    const auto* top = reinterpret_cast<const NT_TIB*>(::NtCurrentTeb())->StackBase;
    int found = 0;

    for (std::size_t index = 0; index < kStackScanWords && found < kStackEntries; ++index)
    {
        if (static_cast<const void*>(stack + index + 1) > top)
        {
            break;
        }

        const std::uintptr_t value = stack[index];

        if (InCode(value))
        {
            SDL_LogCritical(
                SDL_LOG_CATEGORY_APPLICATION,
                "stack esp+0x%03lX %s",
                static_cast<unsigned long>(sizeof(std::uintptr_t) * index),
                Located(reinterpret_cast<const void*>(value)).data()
            );
            ++found;
        }
    }
#endif

    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void InstallFaultLog()
{
    ::SetUnhandledExceptionFilter(&FaultFilter);
}

}  // namespace swd3
