#include "platform/file_system.h"

#include <cerrno>
#include <cstdint>

#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include "platform/wide_win.h"

namespace swd3
{

std::u8string ResolvePath(std::u8string_view gamePath)
{
    return DrivePath(gamePath);
}

void MakeGameDirectory(std::u8string_view path)
{
    ::CreateDirectoryW(Utf16(path).c_str(), nullptr);
}

void ClearReadOnly(const std::u8string& path)
{
    const std::wstring wide = Utf16(path);
    const DWORD attributes = ::GetFileAttributesW(wide.c_str());

    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
    {
        ::SetFileAttributesW(wide.c_str(), attributes & ~static_cast<DWORD>(FILE_ATTRIBUTE_READONLY));
    }
}

std::FILE* OpenSharedFile(const std::u8string& path, std::string_view mode)
{
    const bool update = mode.find('+') != std::string_view::npos;
    const bool create = !mode.empty() && mode.front() == 'w';
    const bool reads = update || !create;
    const bool writes = update || create;
    const DWORD access = (reads ? GENERIC_READ : 0) | (writes ? GENERIC_WRITE : 0);
    const HANDLE handle = ::CreateFileW(
        Utf16(path).c_str(),
        access,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        create ? CREATE_ALWAYS : OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (handle == INVALID_HANDLE_VALUE)
    {
        const DWORD failure = ::GetLastError();
        errno = failure == ERROR_FILE_NOT_FOUND || failure == ERROR_PATH_NOT_FOUND ? ENOENT : EACCES;
        return nullptr;
    }

    const int descriptor = ::_open_osfhandle(
        reinterpret_cast<intptr_t>(handle),
        _O_BINARY | (reads && writes ? _O_RDWR : (writes ? _O_WRONLY : _O_RDONLY))
    );

    if (descriptor == -1)
    {
        ::CloseHandle(handle);
        return nullptr;
    }

    std::FILE* file = ::_fdopen(descriptor, std::string(mode).c_str());

    if (file == nullptr)
    {
        ::_close(descriptor);
    }

    return file;
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
