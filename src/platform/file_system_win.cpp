#include "platform/file_system.h"

#include <io.h>
#include <windows.h>

namespace swd3
{

std::string ResolvePath(std::string_view gamePath)
{
    return DrivePath(gamePath);
}

void MakeGameDirectory(std::string_view path)
{
    ::CreateDirectoryA(std::string(path).c_str(), nullptr);
}

void ClearReadOnly(const std::string& path)
{
    const DWORD attributes = ::GetFileAttributesA(path.c_str());

    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
    {
        ::SetFileAttributesA(path.c_str(), attributes & ~static_cast<DWORD>(FILE_ATTRIBUTE_READONLY));
    }
}

bool TruncateFile(std::FILE* file, u64 size)
{
    return ::_chsize_s(::_fileno(file), static_cast<__int64>(size)) == 0;
}

bool FileWriteTime(std::FILE* file, u64& fileTime)
{
    const auto handle = reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(file)));
    FILETIME written{};

    if (handle == INVALID_HANDLE_VALUE || ::GetFileTime(handle, nullptr, nullptr, &written) == 0)
    {
        return false;
    }

    constexpr int kHighShift = 32;
    const auto [low, high] = written;
    fileTime = static_cast<u64>(high) << kHighShift | low;
    return true;
}

}  // namespace swd3
