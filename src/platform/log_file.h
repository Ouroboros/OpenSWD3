#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

#include <SDL3/SDL_log.h>

namespace swd3
{

// The rebuild's log: logs/<program>-<local date and time>-<process id>.log beside the executable, a line per message
// ("MM-DDTHH:MM:SS.mmm [LEVEL] [thread=id] [file:line] text"). It takes SDL's log output while it exists, so SDL's own
// messages and those of every thread go to it too; each line is written through at once.
class LogFile
{
public:
    explicit LogFile(std::string_view program);
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

}  // namespace swd3
