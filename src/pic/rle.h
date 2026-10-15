#pragma once

#include <cstdint>

namespace swd3 {

// Run-length encoded picture: header {WORD magic, width, height, format}, then lines. Each line starts with
// a WORD (byte length | 0x8000 flag) followed by segment codes: 0x0000|n = n literal pixels follow,
// 0x8000|n = n "skip" pixels, 0xC000|n = n "fill" pixels; a zero code ends the line, a zero header ends
// the picture.

// TSW picture slot (0x18 bytes), e.g. TSW cache node payload.
struct Picture {
    std::uint16_t* pixels;
    void* reserved4;
    std::uint16_t* palette;
    std::uint16_t width;
    std::uint16_t height;
    std::uint32_t size;
    std::uint32_t reserved14;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(Picture) == 0x18);
#endif

void* RleEncode(const void* pixels, std::uint32_t* size, std::uint32_t width, std::uint32_t height, int format);
void* RleDecode(const std::uint16_t* rle, std::uint32_t* width, std::uint32_t* height, std::uint32_t* format);
int RleCheck16(std::uint16_t* rle, std::uint16_t* width, std::uint16_t* height, std::uint16_t* format);
// The original return values (picture pointer / check result) are not used by any caller.
void RleConvert8To16(Picture* picture);
void RleConvertTile8To16(std::uint16_t** data, std::uint32_t* size);

}  // namespace swd3
