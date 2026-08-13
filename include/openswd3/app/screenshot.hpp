#pragma once

#include "openswd3/compat/types.hpp"

#include <string>

namespace openswd3::app {

inline constexpr compat::u32 kLegacyScreenshotProbeLimit = 0x1869EU;

[[nodiscard]] std::string make_legacy_screenshot_filename(compat::u32 sequence);

class ScreenshotPorts {
public:
    virtual ~ScreenshotPorts() = default;

    virtual void beep() = 0;
    virtual void maintain_audio() = 0;
    [[nodiscard]] virtual bool
    open_existing_numbered_bmp(compat::u32 sequence) = 0;
    virtual void close_existing_numbered_bmp() = 0;
    virtual void save_framebuffer_as_numbered_bmp(compat::u32 sequence) = 0;
};

void capture_legacy_screenshot(ScreenshotPorts& ports);

}  // namespace openswd3::app
