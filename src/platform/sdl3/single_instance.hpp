#pragma once

#include "openswd3/app/process_startup.hpp"

#include <string>

namespace openswd3::platform_sdl3 {

class SingleInstanceGuard final : public app::ExistingInstancePorts {
public:
    explicit SingleInstanceGuard(std::string identity = "OpenSWD3");
    ~SingleInstanceGuard() override;

    SingleInstanceGuard(const SingleInstanceGuard&) = delete;
    SingleInstanceGuard& operator=(const SingleInstanceGuard&) = delete;

    [[nodiscard]] bool matching_instance_exists() override;

private:
    std::string identity_;
    bool checked_{};
    bool existing_{};
#if defined(_WIN32)
    void* native_handle_{};
#else
    int descriptor_{-1};
#endif
};

}  // namespace openswd3::platform_sdl3
