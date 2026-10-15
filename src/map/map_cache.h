#pragma once

#include <cstdint>

#include "pic/rle.h"

namespace swd3 {

int LoadCachedMapPicture(std::uint32_t unit, Picture* picture);
int MapDecompress(const std::uint8_t* packed, std::uint8_t* out, std::uint32_t packedSize, std::uint32_t* outSize);

}  // namespace swd3
