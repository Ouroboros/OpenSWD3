#include "single_instance.hpp"

#include <utility>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <filesystem>
#include <system_error>

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace openswd3::platform_sdl3 {

SingleInstanceGuard::SingleInstanceGuard(std::string identity)
    : identity_(std::move(identity)) {}

SingleInstanceGuard::~SingleInstanceGuard() {
#if defined(_WIN32)
    if (native_handle_ != nullptr) {
        static_cast<void>(CloseHandle(static_cast<HANDLE>(native_handle_)));
    }
#else
    if (descriptor_ >= 0) {
        static_cast<void>(flock(descriptor_, LOCK_UN));
        static_cast<void>(close(descriptor_));
    }
#endif
}

bool SingleInstanceGuard::matching_instance_exists() {
    if (checked_) {
        return existing_;
    }
    checked_ = true;

#if defined(_WIN32)
    const std::string mutex_name = "Local\\" + identity_;
    HANDLE handle = CreateMutexA(nullptr, FALSE, mutex_name.c_str());
    if (handle == nullptr) {
        return false;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        static_cast<void>(CloseHandle(handle));
        existing_ = true;
        return true;
    }
    native_handle_ = handle;
#else
    std::error_code error;
    const std::filesystem::path temporary_directory =
        std::filesystem::temp_directory_path(error);
    if (error) {
        return false;
    }
    const std::filesystem::path lock_path =
        temporary_directory / (identity_ + ".instance.lock");
    descriptor_ = open(lock_path.string().c_str(), O_CREAT | O_RDWR, 0600);
    if (descriptor_ < 0) {
        return false;
    }
    if (flock(descriptor_, LOCK_EX | LOCK_NB) != 0) {
        existing_ = errno == EWOULDBLOCK || errno == EAGAIN;
        static_cast<void>(close(descriptor_));
        descriptor_ = -1;
        return existing_;
    }
#endif

    return false;
}

}  // namespace openswd3::platform_sdl3
