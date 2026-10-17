#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

#include <SDL3/SDL_log.h>

namespace swd3
{

// The rebuild's log: <directory>/<prefix>-<local date and time>-<process id><extension> (by default logs/<program>-...
// .log beside the executable), a line per message ("MM-DDTHH:MM:SS.mmm [LEVEL] [thread=id] [file:line] text") in
// UTF-8. It takes SDL's log output while it exists, so SDL's own messages and those of every thread go to it too;
// the messages below `level` are left out, each line is written through at once.
class LogFile
{
public:
    LogFile(
        const std::filesystem::path& directory,
        std::u8string_view prefix,
        std::u8string_view extension,
        SDL_LogPriority level
    );
    ~LogFile();
    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;

    const std::filesystem::path& Path() const;

private:
    static void Output(void* data, int category, SDL_LogPriority priority, const char* message);
    void Write(int category, SDL_LogPriority priority, const char* message);

    std::filesystem::path path;
    std::ofstream stream;
    std::mutex lock;
    SDL_LogOutputFunction previous = nullptr;
    void* previousData = nullptr;
};

// The crashes and the terminations logged before the process ends (a fault, an abort, an unhandled exception).
void InstallCrashLog();

// The directory of the executable (the working directory when SDL cannot tell).
std::filesystem::path ProgramDirectory();

// The log level of SDL's application messages and, when stricter than their defaults, of SDL's own categories.
void SetLogLevel(SDL_LogPriority level);

}  // namespace swd3
