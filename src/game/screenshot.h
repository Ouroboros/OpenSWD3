#pragma once

#include <functional>
#include <span>
#include <string_view>

#include "core/types.h"

namespace swd3
{

class Painter;

int SaveScreenshot(
    const Painter& painter,
    std::span<const u16> pixels,
    int width,
    int height,
    std::string_view fileName,
    const std::function<void()>& serveAudio
);

}  // namespace swd3
