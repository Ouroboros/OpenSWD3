#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include "swd3/w32.h"
#include "platform/path_posix.h"

namespace w32 {
namespace {

thread_local DWORD t_lastError = ERROR_SUCCESS;

enum class Kind { File, Mapping, Event };

struct Object {
    Kind kind;
    int fd;
    DWORD protect;
    std::uint64_t mapSize;
};

std::mutex g_viewsMutex;
std::map<const void*, std::size_t> g_views;

DWORD errnoToWin32(int err) {
    switch (err) {
    case ENOENT:
        return ERROR_FILE_NOT_FOUND;
    case ENOTDIR:
        return ERROR_PATH_NOT_FOUND;
    case EACCES:
    case EPERM:
    case EROFS:
        return ERROR_ACCESS_DENIED;
    case EEXIST:
        return ERROR_FILE_EXISTS;
    case EBADF:
        return ERROR_INVALID_HANDLE;
    case ENOMEM:
        return ERROR_NOT_ENOUGH_MEMORY;
    default:
        return ERROR_INVALID_PARAMETER;
    }
}

BOOL fail(DWORD code) {
    t_lastError = code;
    return 0;
}

Object* asObject(HANDLE handle, Kind kind) {
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return nullptr;
    }
    auto* object = static_cast<Object*>(handle);
    return object->kind == kind ? object : nullptr;
}

}  // namespace

DWORD GetLastError() { return t_lastError; }

void SetLastError(DWORD code) { t_lastError = code; }

BOOL CloseHandle(HANDLE handle) {
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return fail(ERROR_INVALID_HANDLE);
    }
    auto* object = static_cast<Object*>(handle);
    if (object->fd >= 0) {
        ::close(object->fd);
    }
    delete object;
    return 1;
}

// Events are only created for DirectInput notifications, which the emulation never signals.
HANDLE CreateEventA(void*, BOOL, BOOL, LPCSTR) { return new Object{Kind::Event, -1, 0, 0}; }

HANDLE CreateFileA(LPCSTR fileName, DWORD desiredAccess, DWORD, void*, DWORD creationDisposition, DWORD,
                   HANDLE) {
    std::string path = posix::resolvePath(fileName);
    int flags = 0;
    if ((desiredAccess & GENERIC_READ) && (desiredAccess & GENERIC_WRITE)) {
        flags = O_RDWR;
    } else if (desiredAccess & GENERIC_WRITE) {
        flags = O_WRONLY;
    } else {
        flags = O_RDONLY;
    }
    bool existed = ::access(path.c_str(), F_OK) == 0;
    switch (creationDisposition) {
    case CREATE_NEW:
        flags |= O_CREAT | O_EXCL;
        break;
    case CREATE_ALWAYS:
        flags |= O_CREAT | O_TRUNC;
        break;
    case OPEN_EXISTING:
        break;
    case OPEN_ALWAYS:
        flags |= O_CREAT;
        break;
    case TRUNCATE_EXISTING:
        flags |= O_TRUNC;
        break;
    default:
        fail(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    int fd = ::open(path.c_str(), flags | O_CLOEXEC, 0644);
    if (fd < 0) {
        fail(errnoToWin32(errno));
        return INVALID_HANDLE_VALUE;
    }
    t_lastError = ((creationDisposition == CREATE_ALWAYS || creationDisposition == OPEN_ALWAYS) && existed)
                      ? ERROR_ALREADY_EXISTS
                      : ERROR_SUCCESS;
    return new Object{Kind::File, fd, 0, 0};
}

BOOL SetFileAttributesA(LPCSTR fileName, DWORD) {
    std::string path = posix::resolvePath(fileName);
    struct stat st {};
    if (::stat(path.c_str(), &st) != 0) {
        return fail(errnoToWin32(errno));
    }
    if (::chmod(path.c_str(), st.st_mode | S_IWUSR) != 0) {
        return fail(errnoToWin32(errno));
    }
    return 1;
}

BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD toRead, LPDWORD read, void*) {
    Object* object = asObject(file, Kind::File);
    if (read) {
        *read = 0;
    }
    if (!object) {
        return fail(ERROR_INVALID_HANDLE);
    }
    DWORD total = 0;
    auto* out = static_cast<char*>(buffer);
    while (total < toRead) {
        ssize_t n = ::read(object->fd, out + total, toRead - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (read) {
                *read = total;
            }
            return fail(errnoToWin32(errno));
        }
        if (n == 0) {
            break;
        }
        total += static_cast<DWORD>(n);
    }
    if (read) {
        *read = total;
    }
    return 1;
}

BOOL WriteFile(HANDLE file, LPCVOID buffer, DWORD toWrite, LPDWORD written, void*) {
    Object* object = asObject(file, Kind::File);
    if (written) {
        *written = 0;
    }
    if (!object) {
        return fail(ERROR_INVALID_HANDLE);
    }
    DWORD total = 0;
    const auto* in = static_cast<const char*>(buffer);
    while (total < toWrite) {
        ssize_t n = ::write(object->fd, in + total, toWrite - total);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (written) {
                *written = total;
            }
            return fail(errnoToWin32(errno));
        }
        total += static_cast<DWORD>(n);
    }
    if (written) {
        *written = total;
    }
    return 1;
}

DWORD SetFilePointer(HANDLE file, LONG distance, PLONG distanceHigh, DWORD moveMethod) {
    Object* object = asObject(file, Kind::File);
    if (!object) {
        fail(ERROR_INVALID_HANDLE);
        return 0xFFFFFFFFu;
    }
    std::int64_t offset = distance;
    if (distanceHigh) {
        offset = (static_cast<std::int64_t>(*distanceHigh) << 32) | static_cast<std::uint32_t>(distance);
    }
    int whence = moveMethod == FILE_CURRENT ? SEEK_CUR : moveMethod == FILE_END ? SEEK_END : SEEK_SET;
    off_t base = moveMethod == FILE_BEGIN ? 0 : ::lseek(object->fd, 0, whence);
    if (base + offset < 0) {
        fail(ERROR_NEGATIVE_SEEK);
        return 0xFFFFFFFFu;
    }
    off_t result = ::lseek(object->fd, static_cast<off_t>(offset), whence);
    if (result < 0) {
        fail(errnoToWin32(errno));
        return 0xFFFFFFFFu;
    }
    if (distanceHigh) {
        *distanceHigh = static_cast<LONG>(static_cast<std::uint64_t>(result) >> 32);
    }
    t_lastError = ERROR_SUCCESS;
    return static_cast<DWORD>(result);
}

DWORD GetFileSize(HANDLE file, LPDWORD sizeHigh) {
    Object* object = asObject(file, Kind::File);
    struct stat st {};
    if (!object || ::fstat(object->fd, &st) != 0) {
        fail(ERROR_INVALID_HANDLE);
        return 0xFFFFFFFFu;
    }
    if (sizeHigh) {
        *sizeHigh = static_cast<DWORD>(static_cast<std::uint64_t>(st.st_size) >> 32);
    }
    return static_cast<DWORD>(st.st_size);
}

BOOL SetEndOfFile(HANDLE file) {
    Object* object = asObject(file, Kind::File);
    if (!object) {
        return fail(ERROR_INVALID_HANDLE);
    }
    off_t pos = ::lseek(object->fd, 0, SEEK_CUR);
    if (pos < 0 || ::ftruncate(object->fd, pos) != 0) {
        return fail(errnoToWin32(errno));
    }
    return 1;
}

BOOL GetFileTime(HANDLE file, LPFILETIME creation, LPFILETIME lastAccess, LPFILETIME lastWrite) {
    Object* object = asObject(file, Kind::File);
    struct stat st {};
    if (!object || ::fstat(object->fd, &st) != 0) {
        return fail(ERROR_INVALID_HANDLE);
    }
    auto convert = [](const timespec& ts, LPFILETIME out) {
        if (!out) {
            return;
        }
        std::uint64_t ticks = (static_cast<std::uint64_t>(ts.tv_sec) + 11644473600ull) * 10000000ull +
                              static_cast<std::uint64_t>(ts.tv_nsec) / 100u;
        out->dwLowDateTime = static_cast<DWORD>(ticks);
        out->dwHighDateTime = static_cast<DWORD>(ticks >> 32);
    };
    convert(st.st_mtim, creation);
    convert(st.st_atim, lastAccess);
    convert(st.st_mtim, lastWrite);
    return 1;
}

HANDLE CreateFileMappingA(HANDLE file, void*, DWORD protect, DWORD sizeHigh, DWORD sizeLow, LPCSTR) {
    Object* object = asObject(file, Kind::File);
    if (!object) {
        fail(ERROR_INVALID_HANDLE);
        return nullptr;
    }
    struct stat st {};
    ::fstat(object->fd, &st);
    std::uint64_t size = (static_cast<std::uint64_t>(sizeHigh) << 32) | sizeLow;
    if (size == 0) {
        size = static_cast<std::uint64_t>(st.st_size);
    }
    if (size == 0) {
        fail(1006);
        return nullptr;
    }
    int fd = ::dup(object->fd);
    if (fd < 0) {
        fail(errnoToWin32(errno));
        return nullptr;
    }
    t_lastError = ERROR_SUCCESS;
    return new Object{Kind::Mapping, fd, protect, size};
}

LPVOID MapViewOfFile(HANDLE mapping, DWORD access, DWORD offsetHigh, DWORD offsetLow, SIZE_T bytes) {
    Object* object = asObject(mapping, Kind::Mapping);
    if (!object) {
        fail(ERROR_INVALID_HANDLE);
        return nullptr;
    }
    std::uint64_t offset = (static_cast<std::uint64_t>(offsetHigh) << 32) | offsetLow;
    if (offset % 65536u != 0 || offset > object->mapSize) {
        fail(ERROR_INVALID_PARAMETER);
        return nullptr;
    }
    std::size_t length = bytes == 0 ? static_cast<std::size_t>(object->mapSize - offset) : bytes;
    if (offset + length > object->mapSize) {
        fail(ERROR_ACCESS_DENIED);
        return nullptr;
    }
    int prot = PROT_READ;
    int flags = MAP_SHARED;
    if (access != FILE_MAP_READ && object->protect == PAGE_READWRITE) {
        prot |= PROT_WRITE;
    }
    void* view = ::mmap(nullptr, length, prot, flags, object->fd, static_cast<off_t>(offset));
    if (view == MAP_FAILED) {
        fail(errnoToWin32(errno));
        return nullptr;
    }
    std::lock_guard<std::mutex> lock(g_viewsMutex);
    g_views[view] = length;
    t_lastError = ERROR_SUCCESS;
    return view;
}

BOOL UnmapViewOfFile(LPCVOID base) {
    std::size_t length = 0;
    {
        std::lock_guard<std::mutex> lock(g_viewsMutex);
        auto it = g_views.find(base);
        if (it == g_views.end()) {
            return fail(487);
        }
        length = it->second;
        g_views.erase(it);
    }
    ::munmap(const_cast<void*>(base), length);
    return 1;
}

DWORD FormatMessageA(DWORD, LPCVOID, DWORD messageId, DWORD, LPSTR buffer, DWORD size, void*) {
    if (!buffer || size == 0) {
        return 0;
    }
    int n = std::snprintf(buffer, size, "Win32 error %u\r\n", messageId);
    if (n < 0 || static_cast<DWORD>(n) >= size) {
        buffer[0] = '\0';
        return 0;
    }
    return static_cast<DWORD>(n);
}

int lstrlenA(LPCSTR string) { return string ? static_cast<int>(std::strlen(string)) : 0; }

LPSTR lstrcpyA(LPSTR dest, LPCSTR src) { return std::strcpy(dest, src); }

LPSTR lstrcatA(LPSTR dest, LPCSTR src) { return std::strcat(dest, src); }

int lstrcmpA(LPCSTR a, LPCSTR b) {
    int r = std::strcmp(a, b);
    return r < 0 ? -1 : r > 0 ? 1 : 0;
}

}  // namespace w32
