#include "platform/system_info.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <limits>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <windows.h>

#include <dbghelp.h>
#include <mmsystem.h>

namespace swd3
{

namespace
{

constexpr int kMegabyteShift = 20;  // SDL_GetSystemRAM counts MiB

}  // namespace

u32 TotalPhysicalMemory()
{
    const u64 bytes = static_cast<u64>(SDL_GetSystemRAM()) << kMegabyteShift;
    return bytes > std::numeric_limits<u32>::max() ? std::numeric_limits<u32>::max() : static_cast<u32>(bytes);
}

u32 SystemUptime()
{
    return static_cast<u32>(::timeGetTime());
}

void UseFineTimer()
{
    ::timeBeginPeriod(1);
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

constexpr std::size_t kNameChars = 260;
constexpr int kFrames = 64;
constexpr DWORD kCppException = 0xE06D7363;  // the exception code of a C++ throw (MSVC)
constexpr ULONG_PTR kCppMagic = 0x19930520;  // ExceptionInformation[0] of a C++ throw
constexpr std::string_view kStdException = ".?AVexception@std@@";

// The parts of a C++ throw MSVC passes (32-bit: absolute pointers).
struct TypeDescriptor
{
    const void* vftable;
    void* spare;
    char name[1];
};

struct CatchableType
{
    unsigned properties;
    TypeDescriptor* type;
    int memberDisplacement;
    int virtualBaseDisplacement;
    int displacementWithin;
    int size;
    void* copy;
};

struct CatchableTypeArray
{
    int count;
    CatchableType* types[1];
};

struct ThrowInfo
{
    unsigned attributes;
    void* unwind;
    void* forwardCompatibility;
    CatchableTypeArray* catchable;
};

// The file name of a module (its directory left out).
std::string ModuleName(HMODULE module)
{
    std::array<char, kNameChars> path = {};
    ::GetModuleFileNameA(module, path.data(), static_cast<DWORD>(path.size()));
    const std::string text(path.data());
    const std::size_t slash = text.find_last_of("\\/");
    return slash == std::string::npos ? text : text.substr(slash + 1);
}

// "module+0xoffset function+0xoffset (file:line)" of a code address, as far as its symbols tell.
std::string Describe(HANDLE process, DWORD64 address)
{
    std::array<char, kNameChars * 2> text = {};
    HMODULE module = nullptr;
    std::string where = "?";

    if (::GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(static_cast<std::uintptr_t>(address)),
            &module
        ) != 0)
    {
        std::snprintf(
            text.data(),
            text.size(),
            "%s+0x%llX",
            ModuleName(module).c_str(),
            static_cast<unsigned long long>(address - reinterpret_cast<std::uintptr_t>(module))
        );
        where = text.data();
    }

    std::array<std::byte, sizeof(SYMBOL_INFO) + kNameChars> storage = {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage.data());
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = kNameChars;
    DWORD64 symbolOffset = 0;

    if (::SymFromAddr(process, address, &symbolOffset, symbol) != 0)
    {
        std::snprintf(
            text.data(),
            text.size(),
            " %s+0x%llX",
            symbol->Name,
            static_cast<unsigned long long>(symbolOffset)
        );
        where += text.data();
    }

    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    DWORD lineOffset = 0;

    if (::SymGetLineFromAddr64(process, address, &lineOffset, &line) != 0)
    {
        std::snprintf(
            text.data(),
            text.size(),
            " (%s:%lu)",
            line.FileName,
            static_cast<unsigned long>(line.LineNumber)
        );
        where += text.data();
    }

    return where;
}

// The type of a C++ exception and, for a std::exception, its message.
void LogCppException(const EXCEPTION_RECORD& record)
{
    if (record.NumberParameters < 3 || record.ExceptionInformation[0] != kCppMagic)
    {
        return;
    }

    const auto* thrown = reinterpret_cast<const ThrowInfo*>(record.ExceptionInformation[2]);
    auto* object = reinterpret_cast<std::byte*>(record.ExceptionInformation[1]);

    if (thrown == nullptr || thrown->catchable == nullptr || thrown->catchable->count < 1)
    {
        return;
    }

    std::array<char, kNameChars> name = {};
    const char* decorated = thrown->catchable->types[0]->type->name;
    const bool named =
        ::UnDecorateSymbolName(decorated + 1, name.data(), static_cast<DWORD>(name.size()), UNDNAME_NO_ARGUMENTS) != 0;
    std::string message;

    for (int index = 0; index < thrown->catchable->count; ++index)
    {
        const CatchableType& type = *thrown->catchable->types[index];

        if (type.type->name == kStdException)
        {
            message = reinterpret_cast<const std::exception*>(object + type.memberDisplacement)->what();
            break;
        }
    }

    SDL_LogCritical(
        SDL_LOG_CATEGORY_APPLICATION,
        "C++ exception %s%s%s",
        named ? name.data() : decorated,
        message.empty() ? "" : ": ",
        message.c_str()
    );
}

// The call stack of `thread` from `faulted`, frame by frame with the PDB beside the executable.
void LogStack(HANDLE process, HANDLE thread, const CONTEXT& faulted, SDL_LogPriority priority)
{
    CONTEXT context = faulted;
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrStack.Mode = AddrModeFlat;

    for (int index = 0; index < kFrames; ++index)
    {
        if (::StackWalk64(
                IMAGE_FILE_MACHINE_I386,
                process,
                thread,
                &frame,
                &context,
                nullptr,
                ::SymFunctionTableAccess64,
                ::SymGetModuleBase64,
                nullptr
            ) == 0 ||
            frame.AddrPC.Offset == 0)
        {
            break;
        }

        SDL_LogMessage(
            SDL_LOG_CATEGORY_APPLICATION,
            priority,
            "  #%02d %s",
            index,
            Describe(process, index == 0 ? frame.AddrPC.Offset : frame.AddrPC.Offset - 1).c_str()
        );
    }
}

// A minidump of the process beside the logs: logs\swd3r-crash-<local date and time>-<process id>.dmp.
void WriteDump(EXCEPTION_POINTERS* exception)
{
    std::array<char, kNameChars> path = {};
    ::GetModuleFileNameA(nullptr, path.data(), static_cast<DWORD>(path.size()));
    std::string directory(path.data());
    directory = directory.substr(0, directory.find_last_of("\\/") + 1) + "logs\\";
    ::CreateDirectoryA(directory.c_str(), nullptr);
    SYSTEMTIME now{};
    ::GetLocalTime(&now);
    const auto [year, month, dayOfWeek, day, hour, minute, second, milliseconds] = now;
    std::array<char, kNameChars> name = {};
    std::snprintf(
        name.data(),
        name.size(),
        "%sswd3r-crash-%04u-%02u-%02u_%02u-%02u-%02u-%lu.dmp",
        directory.c_str(),
        year,
        month,
        day,
        hour,
        minute,
        second,
        static_cast<unsigned long>(::GetCurrentProcessId())
    );
    const HANDLE file =
        ::CreateFileA(name.data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "minidump %s not written", name.data());
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION information{::GetCurrentThreadId(), exception, FALSE};
    const BOOL written = ::MiniDumpWriteDump(
        ::GetCurrentProcess(),
        ::GetCurrentProcessId(),
        file,
        static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo),
        &information,
        nullptr,
        nullptr
    );
    ::CloseHandle(file);
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "minidump %s%s", name.data(), written != 0 ? "" : " failed");
}

// An unhandled structured exception (a fault, or a C++ exception nothing caught): its code and place, the type and
// message of a C++ exception, the access of an access violation, the registers and the call stack with its symbols
// logged, and a minidump written; the system then ends the process.
LONG WINAPI FaultFilter(EXCEPTION_POINTERS* exception)
{
    const EXCEPTION_RECORD& record = *exception->ExceptionRecord;
    const HANDLE process = ::GetCurrentProcess();
    std::array<char, kNameChars> directory = {};
    ::GetModuleFileNameA(nullptr, directory.data(), static_cast<DWORD>(directory.size()));
    std::string symbols(directory.data());
    symbols = symbols.substr(0, symbols.find_last_of("\\/"));
    ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);
    const bool initialized = ::SymInitialize(process, symbols.c_str(), TRUE) != 0;
    const bool symbolsReady = initialized || ::GetLastError() == ERROR_INVALID_PARAMETER;  // the hang watch has them
    SDL_LogCritical(
        SDL_LOG_CATEGORY_APPLICATION,
        "unhandled exception 0x%08lX at %s",
        static_cast<unsigned long>(record.ExceptionCode),
        Describe(process, reinterpret_cast<std::uintptr_t>(record.ExceptionAddress)).c_str()
    );

    if (record.ExceptionCode == kCppException)
    {
        LogCppException(record);
    }

    if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record.NumberParameters >= 2)
    {
        SDL_LogCritical(
            SDL_LOG_CATEGORY_APPLICATION,
            "access violation: %s address 0x%08lX",
            record.ExceptionInformation[0] == 0 ? "read" : (record.ExceptionInformation[0] == 1 ? "write" : "execute"),
            static_cast<unsigned long>(record.ExceptionInformation[1])
        );
    }

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
    SDL_LogCritical(
        SDL_LOG_CATEGORY_APPLICATION,
        "call stack (symbols %s from %s):",
        symbolsReady ? "loaded" : "unavailable",
        symbols.c_str()
    );
    LogStack(process, ::GetCurrentThread(), context, SDL_LOG_PRIORITY_CRITICAL);
    WriteDump(exception);

    if (initialized)
    {
        ::SymCleanup(process);
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void InstallFaultLog()
{
    ::SetUnhandledExceptionFilter(&FaultFilter);
}

HangWatch::HangWatch()
{
    HANDLE self = nullptr;
    ::DuplicateHandle(
        ::GetCurrentProcess(),
        ::GetCurrentThread(),
        ::GetCurrentProcess(),
        &self,
        THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
        FALSE,
        0
    );
    this->watched = self;
    this->thread = std::thread(&HangWatch::Watch, this);
}

HangWatch::~HangWatch()
{
    this->stopping = true;

    if (this->thread.joinable())
    {
        this->thread.join();
    }

    if (this->watched != nullptr)
    {
        ::CloseHandle(static_cast<HANDLE>(this->watched));
    }
}

void HangWatch::Beat()
{
    this->beats.fetch_add(1, std::memory_order_relaxed);
}

// Every quarter second: a main loop without a beat for StallSeconds has its stack logged (suspended for the walk).
void HangWatch::Watch()
{
    constexpr u32 kPollMilliseconds = 250;
    constexpr u32 kMillisecondsPerSecond = 1000;
    u32 seen = this->beats.load(std::memory_order_relaxed);
    u32 since = ::timeGetTime();
    u32 reported = 0;
    bool symbolsReady = false;
    const HANDLE process = ::GetCurrentProcess();

    while (!this->stopping)
    {
        ::Sleep(kPollMilliseconds);
        const u32 now = ::timeGetTime();
        const u32 beats = this->beats.load(std::memory_order_relaxed);

        if (beats != seen)
        {
            if (reported != 0)
            {
                SDL_LogWarn(
                    SDL_LOG_CATEGORY_APPLICATION,
                    "main loop running again after %lu ms",
                    static_cast<unsigned long>(now - since)
                );
            }

            seen = beats;
            since = now;
            reported = 0;
            continue;
        }

        const u32 stalled = now - since;
        const u32 due = reported == 0 ? HangWatch::StallSeconds : reported + HangWatch::RepeatSeconds;

        if (stalled < due * kMillisecondsPerSecond || this->watched == nullptr)
        {
            continue;
        }

        reported = stalled / kMillisecondsPerSecond;

        if (!symbolsReady)
        {
            std::array<char, kNameChars> directory = {};
            ::GetModuleFileNameA(nullptr, directory.data(), static_cast<DWORD>(directory.size()));
            std::string symbols(directory.data());
            symbols = symbols.substr(0, symbols.find_last_of("\\/"));
            ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS);
            symbolsReady = ::SymInitialize(process, symbols.c_str(), TRUE) != 0;
        }

        const auto main = static_cast<HANDLE>(this->watched);

        if (::SuspendThread(main) == static_cast<DWORD>(-1))
        {
            continue;
        }

        CONTEXT context{};
        context.ContextFlags = CONTEXT_FULL;
        const bool got = ::GetThreadContext(main, &context) != 0;
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION,
            "main loop stalled for %lu s (a hang, or a modal dialog); its call stack%s:",
            static_cast<unsigned long>(reported),
            got ? "" : " cannot be read"
        );

        if (got)
        {
            LogStack(process, main, context, SDL_LOG_PRIORITY_WARN);
        }

        ::ResumeThread(main);
    }

    if (symbolsReady)
    {
        ::SymCleanup(process);
    }
}

}  // namespace swd3
