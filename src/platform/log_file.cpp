#include "platform/log_file.h"

#include <array>
#include <cstdio>
#include <exception>
#include <string>
#include <system_error>

#include <SDL3/SDL.h>

#include "core/log.h"
#include "platform/system_info.h"

namespace swd3
{

namespace
{

constexpr std::string_view kLogDirectory = "logs";
constexpr std::string_view kLogExtension = ".log";
constexpr int kNanosecondsPerMillisecond = 1000000;
constexpr std::size_t kPrefixBytes = 0x60;

// The name of a level in the log.
const char* LevelName(SDL_LogPriority priority)
{
    switch (priority)
    {
        case SDL_LOG_PRIORITY_TRACE:
        case SDL_LOG_PRIORITY_VERBOSE:
            return "TRACE";
        case SDL_LOG_PRIORITY_DEBUG:
            return "DEBUG";
        case SDL_LOG_PRIORITY_INFO:
            return "INFO";
        case SDL_LOG_PRIORITY_WARN:
            return "WARN";
        case SDL_LOG_PRIORITY_ERROR:
            return "ERROR";
        case SDL_LOG_PRIORITY_CRITICAL:
            return "FATAL";
        default:
            return "LOG";
    }
}

SDL_LogPriority Priority(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:
            return SDL_LOG_PRIORITY_DEBUG;
        case LogLevel::Info:
            return SDL_LOG_PRIORITY_INFO;
        case LogLevel::Warning:
            return SDL_LOG_PRIORITY_WARN;
        default:
            return SDL_LOG_PRIORITY_ERROR;
    }
}

// The local date and time now.
SDL_DateTime Now()
{
    SDL_Time ticks = 0;
    SDL_DateTime now{};

    if (SDL_GetCurrentTime(&ticks))
    {
        SDL_TimeToDateTime(ticks, &now, true);
    }

    return now;
}

// The file name of the log: the program, the local date and time, the process id.
std::string LogName(std::string_view program)
{
    const SDL_DateTime now = Now();
    std::array<char, kPrefixBytes> stamp = {};
    std::snprintf(
        stamp.data(),
        stamp.size(),
        "-%04d-%02d-%02d_%02d-%02d-%02d-%u",
        now.year,
        now.month,
        now.day,
        now.hour,
        now.minute,
        now.second,
        ProcessId()
    );
    return std::string(program) + stamp.data() + std::string(kLogExtension);
}

// The directory of the executable.
std::filesystem::path ProgramDirectory()
{
    const char* base = SDL_GetBasePath();

    if (base == nullptr)
    {
        std::error_code error;
        return std::filesystem::current_path(error);
    }

    const std::string text(base);
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

void Terminated()
{
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "std::terminate called (an unhandled exception)");
    std::abort();
}

}  // namespace

LogFile::LogFile(std::string_view program)
{
    std::error_code error;
    const std::filesystem::path directory = ProgramDirectory() / kLogDirectory;
    std::filesystem::create_directories(directory, error);
    this->path = directory / LogName(program.empty() ? std::string_view("swd3r") : program);
    this->stream.open(this->path, std::ios::out | std::ios::trunc | std::ios::binary);
    SDL_GetLogOutputFunction(&this->previous, &this->previousData);
    SDL_SetLogOutputFunction(&LogFile::Output, this);
    SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG);
}

LogFile::~LogFile()
{
    SDL_SetLogOutputFunction(this->previous, this->previousData);
}

const std::filesystem::path& LogFile::Path() const
{
    return this->path;
}

void LogFile::Output(void* data, int category, SDL_LogPriority priority, const char* message)
{
    static_cast<LogFile*>(data)->Write(category, priority, message);
}

void LogFile::Write(int category, SDL_LogPriority priority, const char* message)
{
    const SDL_DateTime now = Now();
    std::array<char, kPrefixBytes> prefix = {};
    std::snprintf(
        prefix.data(),
        prefix.size(),
        "%02d-%02dT%02d:%02d:%02d.%03d [%s] [thread=%llu] ",
        now.month,
        now.day,
        now.hour,
        now.minute,
        now.second,
        now.nanosecond / kNanosecondsPerMillisecond,
        LevelName(priority),
        static_cast<unsigned long long>(SDL_GetCurrentThreadID())
    );

    {
        std::lock_guard<std::mutex> guard(this->lock);

        if (this->stream.is_open())
        {
            this->stream << prefix.data();

            if (category != SDL_LOG_CATEGORY_APPLICATION)
            {
                this->stream << "[sdl] ";
            }

            this->stream << message << '\n';
            this->stream.flush();
        }
    }

    if (this->previous != nullptr)
    {
        this->previous(this->previousData, category, priority, message);
    }
}

void WriteLog(LogLevel level, const std::source_location& where, std::string_view text)
{
    std::string_view file = where.file_name();
    const std::size_t slash = file.find_last_of("/\\");

    if (slash != std::string_view::npos)
    {
        file.remove_prefix(slash + 1);
    }

    SDL_LogMessage(
        SDL_LOG_CATEGORY_APPLICATION,
        Priority(level),
        "[%.*s:%u] %.*s",
        static_cast<int>(file.size()),
        file.data(),
        static_cast<unsigned>(where.line()),
        static_cast<int>(text.size()),
        text.data()
    );
}

void InstallCrashLog()
{
    std::set_terminate(&Terminated);
    InstallFaultLog();
}

}  // namespace swd3
