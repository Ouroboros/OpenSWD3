#pragma once

#include <cstdint>

namespace swd3 {

// LZO1X decompressor (non-safe variant: no input/output bounds checks).
// Returns 0, -8 (input not consumed) or -4 (input overrun); *outLength receives the decoded size.
int Lzo1xDecompress(const std::uint8_t* in, std::uint32_t inLength, std::uint8_t* out, std::uint32_t* outLength);

}  // namespace swd3
