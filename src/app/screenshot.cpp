#include "openswd3/app/screenshot.hpp"

#include <string>

namespace openswd3::app {

std::string make_legacy_screenshot_filename(const compat::u32 sequence) {
    std::string digits = std::to_string(sequence);
    if (digits.size() < 5U) {
        digits.insert(0U, 5U - digits.size(), '0');
    }
    digits += ".bmp";
    return digits;
}

void capture_legacy_screenshot(ScreenshotPorts& ports) {
    ports.beep();

    compat::u32 candidate = 0U;
    while (true) {
        ports.maintain_audio();
        if (!ports.open_existing_numbered_bmp(candidate)) {
            break;
        }

        ports.close_existing_numbered_bmp();
        const compat::u32 next = candidate + 1U;
        if (next >= kLegacyScreenshotProbeLimit) {
            break;
        }
        candidate = next;
    }

    ports.save_framebuffer_as_numbered_bmp(candidate);
}

}  // namespace openswd3::app
