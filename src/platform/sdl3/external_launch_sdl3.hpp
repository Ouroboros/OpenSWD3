#pragma once

#include "openswd3/app/external_launch.hpp"

namespace openswd3::platform_sdl3 {

class SdlExternalLaunchPorts final : public app::ExternalLaunchPorts {
public:
    bool open_url(std::string_view target) override;
    bool open_document(std::string_view path) override;
};

}  // namespace openswd3::platform_sdl3
