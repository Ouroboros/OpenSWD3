#include "pic/pixel_format.h"

#include "data/code_ptr.h"
#include "data/globals.h"

namespace swd3 {

// swd32.exe 0x004238B0: call through the converter selected for the display pixel format
void PixelConvert(std::uint16_t* pixels, int count) { codeAs<PixelConvertFn>(G.pixelConvert)(pixels, count); }

// swd32.exe 0x0044A240 (nullsub_1) as stored in G.pixelConvert / G.pixelConvertBack
void PixelConvertNone(std::uint16_t*, int) {}

}  // namespace swd3
