#include "openswd3/resource_io/legacy_file.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace openswd3::resource_io {

struct LegacyFile::State {
#ifdef _WIN32
    HANDLE file{INVALID_HANDLE_VALUE};
    HANDLE mapping{};
#else
    int file{-1};
    bool mapping{};
#endif
    const compat::u8* view{};
    std::size_t view_size{};
    std::filesystem::path path;
    LegacyFileAccess access{LegacyFileAccess::read};
    std::array<char, 64> error{};
};

LegacyFile::LegacyFile() : state_(std::make_unique<State>()) {}

LegacyFile::~LegacyFile() {
    static_cast<void>(close());
}

bool LegacyFile::open(
    const std::filesystem::path& path,
    const LegacyFileCreation creation,
    const LegacyFileAccess access
) {
#ifdef _WIN32
    if (state_->file != INVALID_HANDLE_VALUE) {
        static_cast<void>(CloseHandle(state_->file));
    }

    static_cast<void>(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL));

    DWORD desired_access{};
    switch (access) {
    case LegacyFileAccess::read:
        desired_access = GENERIC_READ;
        break;
    case LegacyFileAccess::write:
        desired_access = GENERIC_WRITE;
        break;
    case LegacyFileAccess::read_write:
        desired_access = GENERIC_READ | GENERIC_WRITE;
        break;
    }

    const DWORD creation_disposition =
        creation == LegacyFileCreation::open_existing ? OPEN_EXISTING
                                                      : OPEN_ALWAYS;
    state_->file = CreateFileW(
        path.c_str(),
        desired_access,
        0U,
        nullptr,
        creation_disposition,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );
    if (state_->file == INVALID_HANDLE_VALUE) {
        set_system_error();
        return false;
    }

    state_->path = path;
    state_->access = access;
    static_cast<void>(SetFilePointer(state_->file, 0, nullptr, FILE_BEGIN));
#else
    if (state_->file != -1) {
        static_cast<void>(::close(state_->file));
    }

    int flags{};
    switch (access) {
    case LegacyFileAccess::read:
        flags = O_RDONLY;
        break;
    case LegacyFileAccess::write:
        flags = O_WRONLY;
        break;
    case LegacyFileAccess::read_write:
        flags = O_RDWR;
        break;
    }
    if (creation == LegacyFileCreation::open_always) {
        flags |= O_CREAT;
    }

    state_->file = ::open(path.c_str(), flags, 0666);
    if (state_->file == -1) {
        set_system_error();
        return false;
    }

    state_->path = path;
    state_->access = access;
    static_cast<void>(::lseek(state_->file, 0, SEEK_SET));
#endif
    return true;
}

bool LegacyFile::close() noexcept {
    if (!close_mapping()) {
        return false;
    }

#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE) {
        return true;
    }
    if (CloseHandle(state_->file) == 0) {
        set_system_error();
        return false;
    }
    state_->file = INVALID_HANDLE_VALUE;
#else
    if (state_->file == -1) {
        return true;
    }
    if (::close(state_->file) != 0) {
        set_system_error();
        return false;
    }
    state_->file = -1;
#endif

    state_->path.clear();
    state_->error.fill('\0');
    return true;
}

bool LegacyFile::create_read_only_mapping() noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE) {
        return false;
    }

    state_->mapping = CreateFileMappingW(
        state_->file,
        nullptr,
        PAGE_READONLY,
        0U,
        size(),
        nullptr
    );
    if (state_->mapping == nullptr) {
        set_system_error();
        return false;
    }
#else
    if (state_->file == -1) {
        return false;
    }
    if (state_->access == LegacyFileAccess::write) {
        errno = EACCES;
        set_system_error();
        return false;
    }

    const compat::u32 file_size = size();
    if (file_size == 0U || file_size == std::numeric_limits<compat::u32>::max()) {
        errno = EINVAL;
        set_system_error();
        return false;
    }
    state_->mapping = true;
#endif
    return true;
}

bool LegacyFile::close_mapping() noexcept {
#ifdef _WIN32
    if (state_->mapping == nullptr) {
        return true;
    }

    if (state_->view != nullptr) {
        if (UnmapViewOfFile(state_->view) == 0) {
            set_system_error();
            return false;
        }
        state_->view = nullptr;
        state_->view_size = 0U;
    }
    if (CloseHandle(state_->mapping) == 0) {
        set_system_error();
        return false;
    }
    state_->mapping = nullptr;
#else
    if (!state_->mapping) {
        return true;
    }

    if (state_->view != nullptr) {
        if (::munmap(
                const_cast<compat::u8*>(state_->view),
                state_->view_size
            ) != 0) {
            set_system_error();
            return false;
        }
        state_->view = nullptr;
        state_->view_size = 0U;
    }
    state_->mapping = false;
#endif
    return true;
}

const compat::u8* LegacyFile::map_view(
    const compat::u32 offset,
    const compat::u32 size_to_map
) noexcept {
    if (state_->view != nullptr) {
#ifdef _WIN32
        if (UnmapViewOfFile(state_->view) == 0) {
            set_system_error();
            return nullptr;
        }
#else
        if (::munmap(
                const_cast<compat::u8*>(state_->view),
                state_->view_size
            ) != 0) {
            set_system_error();
            return nullptr;
        }
#endif
    }

#ifdef _WIN32
    state_->view = static_cast<const compat::u8*>(MapViewOfFile(
        state_->mapping,
        FILE_MAP_READ,
        0U,
        offset,
        size_to_map
    ));
    state_->view_size = size_to_map;
#else
    state_->view = nullptr;
    state_->view_size = 0U;
    if (!state_->mapping) {
        return nullptr;
    }

    const compat::u32 file_size = size();
    if (file_size == std::numeric_limits<compat::u32>::max() ||
        offset >= file_size) {
        return nullptr;
    }
    const std::size_t mapped_size =
        size_to_map == 0U ? static_cast<std::size_t>(file_size - offset)
                          : static_cast<std::size_t>(size_to_map);
    void* const view = ::mmap(
        nullptr,
        mapped_size,
        PROT_READ,
        MAP_SHARED,
        state_->file,
        static_cast<off_t>(offset)
    );
    if (view != MAP_FAILED) {
        state_->view = static_cast<const compat::u8*>(view);
        state_->view_size = mapped_size;
    }
#endif
    return state_->view;
}

bool LegacyFile::close_view(const compat::u8* const view) noexcept {
    if (view != state_->view) {
        return false;
    }
    if (view != nullptr) {
#ifdef _WIN32
        if (UnmapViewOfFile(view) == 0) {
            set_system_error();
            return false;
        }
#else
        if (::munmap(
                const_cast<compat::u8*>(view),
                state_->view_size
            ) != 0) {
            set_system_error();
            return false;
        }
#endif
    }
    state_->view = nullptr;
    state_->view_size = 0U;
    return true;
}

compat::u32 LegacyFile::size() const noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE) {
        return std::numeric_limits<compat::u32>::max();
    }
    return GetFileSize(state_->file, nullptr);
#else
    if (state_->file == -1) {
        return std::numeric_limits<compat::u32>::max();
    }
    struct stat status {};
    if (::fstat(state_->file, &status) != 0) {
        return std::numeric_limits<compat::u32>::max();
    }
    return static_cast<compat::u32>(status.st_size);
#endif
}

bool LegacyFile::truncate_at_current_position() noexcept {
#ifdef _WIN32
    return state_->file != INVALID_HANDLE_VALUE &&
           SetEndOfFile(state_->file) != 0;
#else
    if (state_->file == -1) {
        return false;
    }
    const off_t position = ::lseek(state_->file, 0, SEEK_CUR);
    return position != static_cast<off_t>(-1) &&
           ::ftruncate(state_->file, position) == 0;
#endif
}

bool LegacyFile::read(
    const std::span<compat::u8> buffer,
    compat::u32& in_out_size
) noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE || buffer.empty() ||
        in_out_size == 0U || in_out_size > buffer.size()) {
#else
    if (state_->file == -1 || buffer.empty() || in_out_size == 0U ||
        in_out_size > buffer.size()) {
#endif
        set_error("invalid read request");
        return false;
    }

#ifdef _WIN32
    DWORD actual_size{};
    if (ReadFile(
            state_->file,
            buffer.data(),
            in_out_size,
            &actual_size,
            nullptr
        ) == 0) {
        in_out_size = 0U;
        set_system_error();
        return false;
    }
    in_out_size = actual_size;
#else
    const ssize_t actual_size =
        ::read(state_->file, buffer.data(), in_out_size);
    if (actual_size == -1) {
        in_out_size = 0U;
        set_system_error();
        return false;
    }
    in_out_size = static_cast<compat::u32>(actual_size);
#endif
    return true;
}

bool LegacyFile::write(
    const std::span<const compat::u8> buffer,
    compat::u32& in_out_size
) noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE || buffer.empty() ||
        in_out_size == 0U || in_out_size > buffer.size()) {
#else
    if (state_->file == -1 || buffer.empty() || in_out_size == 0U ||
        in_out_size > buffer.size()) {
#endif
        set_error("invalid write request");
        return false;
    }

#ifdef _WIN32
    DWORD actual_size{};
    if (WriteFile(
            state_->file,
            buffer.data(),
            in_out_size,
            &actual_size,
            nullptr
        ) == 0) {
        in_out_size = 0U;
        set_system_error();
        return false;
    }
    in_out_size = actual_size;
#else
    const ssize_t actual_size =
        ::write(state_->file, buffer.data(), in_out_size);
    if (actual_size == -1) {
        in_out_size = 0U;
        set_system_error();
        return false;
    }
    in_out_size = static_cast<compat::u32>(actual_size);
#endif
    return true;
}

compat::u32 LegacyFile::seek_raw(
    const SeekOrigin origin,
    const compat::i32 distance
) noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE) {
        set_error("invalid file handle");
        return 0U;
    }

    DWORD move_method{};
    switch (origin) {
    case SeekOrigin::begin:
        move_method = FILE_BEGIN;
        break;
    case SeekOrigin::current:
        move_method = FILE_CURRENT;
        break;
    case SeekOrigin::end:
        move_method = FILE_END;
        break;
    }
    LONG distance_high = distance < 0 ? -1L : 0L;
    const DWORD position =
        SetFilePointer(state_->file, distance, &distance_high, move_method);
    if (position == INVALID_SET_FILE_POINTER) {
        set_system_error();
    }
    return position;
#else
    if (state_->file == -1) {
        set_error("invalid file handle");
        return 0U;
    }

    int native_origin{};
    switch (origin) {
    case SeekOrigin::begin:
        native_origin = SEEK_SET;
        break;
    case SeekOrigin::current:
        native_origin = SEEK_CUR;
        break;
    case SeekOrigin::end:
        native_origin = SEEK_END;
        break;
    }
    const off_t position = ::lseek(state_->file, distance, native_origin);
    if (position == static_cast<off_t>(-1)) {
        set_system_error();
        return std::numeric_limits<compat::u32>::max();
    }
    return static_cast<compat::u32>(position);
#endif
}

compat::u32 LegacyFile::seek_current_one_based(
    const compat::i32 distance
) noexcept {
    return seek_raw(SeekOrigin::current, distance) + 1U;
}

compat::u32 LegacyFile::seek_begin_one_based(
    const compat::i32 distance
) noexcept {
    return seek_raw(SeekOrigin::begin, distance) + 1U;
}

compat::u32 LegacyFile::seek_end_one_based(const compat::i32 distance) noexcept {
    return seek_raw(SeekOrigin::end, distance) + 1U;
}

bool LegacyFile::current_position(compat::u32& position) noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE) {
        set_error("invalid file handle");
        return false;
    }
    LONG high{};
    const DWORD current =
        SetFilePointer(state_->file, 0, &high, FILE_CURRENT);
    if (current == INVALID_SET_FILE_POINTER) {
        set_system_error();
        return false;
    }
    position = current;
#else
    if (state_->file == -1) {
        set_error("invalid file handle");
        return false;
    }
    const off_t current = ::lseek(state_->file, 0, SEEK_CUR);
    if (current == static_cast<off_t>(-1)) {
        set_system_error();
        return false;
    }
    position = static_cast<compat::u32>(current);
#endif
    return true;
}

bool LegacyFile::write_exact(
    const std::span<const compat::u8> buffer
) noexcept {
    if (buffer.size() > std::numeric_limits<compat::u32>::max()) {
        set_error("write size exceeds the legacy 32-bit range");
        return false;
    }

    const compat::u32 requested = static_cast<compat::u32>(buffer.size());
    compat::u32 actual = requested;
    if (!write(buffer, actual)) {
        return false;
    }
    if (actual != requested) {
        set_error("short write");
        return false;
    }
    return true;
}

bool LegacyFile::write_u8(const compat::u8 value) noexcept {
    const std::array bytes{value};
    return write_exact(bytes);
}

bool LegacyFile::write_u32(const compat::u32 value) noexcept {
    const std::array<compat::u8, 4> bytes{
        static_cast<compat::u8>(value),
        static_cast<compat::u8>(value >> 8U),
        static_cast<compat::u8>(value >> 16U),
        static_cast<compat::u8>(value >> 24U),
    };
    return write_exact(bytes);
}

bool LegacyFile::read_u8(compat::u8& value) noexcept {
    std::array bytes{value};
    compat::u32 requested = 1U;
    if (!read(bytes, requested)) {
        return false;
    }
    value = bytes[0];
    return true;
}

bool LegacyFile::read_u32(compat::u32& value) noexcept {
    std::array<compat::u8, 4> bytes{
        static_cast<compat::u8>(value),
        static_cast<compat::u8>(value >> 8U),
        static_cast<compat::u8>(value >> 16U),
        static_cast<compat::u8>(value >> 24U),
    };
    compat::u32 requested = 4U;
    if (!read(bytes, requested)) {
        return false;
    }
    value = static_cast<compat::u32>(bytes[0]) |
            (static_cast<compat::u32>(bytes[1]) << 8U) |
            (static_cast<compat::u32>(bytes[2]) << 16U) |
            (static_cast<compat::u32>(bytes[3]) << 24U);
    return true;
}

bool LegacyFile::last_write_time(LegacyFileTime& time) const noexcept {
#ifdef _WIN32
    if (state_->file == INVALID_HANDLE_VALUE) {
        return false;
    }
    FILETIME native_time{};
    if (GetFileTime(state_->file, nullptr, nullptr, &native_time) == 0) {
        return false;
    }
    time.low = native_time.dwLowDateTime;
    time.high = native_time.dwHighDateTime;
#else
    if (state_->file == -1) {
        return false;
    }
    struct stat status {};
    if (::fstat(state_->file, &status) != 0) {
        return false;
    }
#if defined(__APPLE__)
    const std::int64_t seconds = status.st_mtimespec.tv_sec;
    const std::int64_t nanoseconds = status.st_mtimespec.tv_nsec;
#else
    const std::int64_t seconds = status.st_mtim.tv_sec;
    const std::int64_t nanoseconds = status.st_mtim.tv_nsec;
#endif
    constexpr std::int64_t kWindowsEpochOffsetSeconds = 11'644'473'600LL;
    if (seconds < -kWindowsEpochOffsetSeconds) {
        return false;
    }
    const std::uint64_t ticks =
        static_cast<std::uint64_t>(seconds + kWindowsEpochOffsetSeconds) *
            10'000'000ULL +
        static_cast<std::uint64_t>(nanoseconds) / 100ULL;
    time.low = static_cast<compat::u32>(ticks);
    time.high = static_cast<compat::u32>(ticks >> 32U);
#endif
    return true;
}

std::string_view LegacyFile::error_message() const noexcept {
    return state_->error.data();
}

void LegacyFile::set_error(const std::string_view message) noexcept {
    state_->error.fill('\0');
    const std::size_t size =
        std::min(message.size(), state_->error.size() - 1U);
    std::copy_n(message.begin(), size, state_->error.begin());
}

void LegacyFile::set_system_error() noexcept {
#ifdef _WIN32
    const DWORD error = GetLastError();
    state_->error.fill('\0');
    if (FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            error,
            0U,
            state_->error.data(),
            static_cast<DWORD>(state_->error.size()),
            nullptr
        ) == 0U) {
        const int result = std::snprintf(
            state_->error.data(),
            state_->error.size(),
            "system error %lu",
            static_cast<unsigned long>(error)
        );
        static_cast<void>(result);
    }
#else
    const char* const message = std::strerror(errno);
    set_error(message != nullptr ? message : "system error");
#endif
}

}  // namespace openswd3::resource_io
