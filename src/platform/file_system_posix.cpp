#include "platform/file_system.h"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <system_error>

#include <sys/stat.h>
#include <unistd.h>

namespace swd3
{

namespace fs = std::filesystem;

constexpr std::size_t kDriveRootBytes = 3;  // "E:/"

namespace
{

bool sameIgnoringCase(std::string_view left, std::string_view right)
{
    if (left.size() != right.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < left.size(); ++i)
    {
        if (std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i])))
        {
            return false;
        }
    }

    return true;
}

fs::path matchComponent(const fs::path& directory, const std::string& name)
{
    std::error_code error;
    fs::path direct = directory / name;

    if (fs::exists(direct, error) || name == "." || name == "..")
    {
        return direct;
    }

    const fs::path searched = directory.empty() ? fs::path(".") : directory;

    for (const auto& entry : fs::directory_iterator(searched, error))
    {
        const std::string entryName = entry.path().filename().string();

        if (sameIgnoringCase(entryName, name))
        {
            return directory / entryName;
        }
    }

    return direct;
}

}  // namespace

std::string ResolvePath(std::string_view gamePath)
{
    std::string path = DrivePath(gamePath);

    for (char& c : path)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }

    std::error_code error;

    if (path.empty() || fs::exists(path, error))
    {
        return path;
    }

    fs::path result = path.front() == '/' ? fs::path("/") : fs::path();
    std::size_t start = 0;

    // A drive ("E:/") starts from its root; joined as a component it would make the drive-relative "E:Game".
    if (path.size() >= kDriveRootBytes && path[1] == ':' && path[2] == '/')
    {
        result = fs::path(path.substr(0, kDriveRootBytes));
        start = kDriveRootBytes;
    }

    while (start <= path.size())
    {
        const std::size_t end = path.find('/', start);
        const std::string part = path.substr(start, end == std::string::npos ? std::string::npos : end - start);

        if (!part.empty())
        {
            result = matchComponent(result, part);
        }

        if (end == std::string::npos)
        {
            break;
        }

        start = end + 1;
    }

    return result.generic_string();
}

void MakeGameDirectory(std::string_view path)
{
    std::error_code error;
    fs::create_directory(ResolvePath(path), error);
}

void ClearReadOnly(const std::string& path)
{
    std::error_code ignored;
    fs::permissions(path, fs::perms::owner_write, fs::perm_options::add, ignored);
}

std::FILE* OpenSharedFile(const std::string& path, std::string_view mode)
{
    return std::fopen(path.c_str(), std::string(mode).c_str());
}

bool TruncateFile(std::FILE* file, u64 size)
{
    return ::ftruncate(::fileno(file), static_cast<off_t>(size)) == 0;
}

bool FileWriteTime(std::FILE* file, u64& fileTime)
{
    struct stat status{};

    if (::fstat(::fileno(file), &status) != 0)
    {
        return false;
    }

    constexpr u64 kTicksPerSecond = 10000000;  // FILETIME counts 100 ns ticks
    constexpr u64 kNanosecondsPerTick = 100;
    constexpr u64 kFrom1601To1970 = 116444736000000000;
    fileTime = static_cast<u64>(status.st_mtim.tv_sec) * kTicksPerSecond +
               static_cast<u64>(status.st_mtim.tv_nsec) / kNanosecondsPerTick + kFrom1601To1970;
    return true;
}

}  // namespace swd3
