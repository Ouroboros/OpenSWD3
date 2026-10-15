#pragma once

#include <cstdint>

#include "pic/rle.h"
#include "swd3/w32.h"

namespace swd3 {

// all_*.tsw picture archives. File: header, 0x2C-byte directory entries at 0x1C + 0x2C * (index - 1),
// each entry's data: 12-byte header (word[2] = 0xABCD, word[4] = format), optional 0x200-byte palette for
// 8-bit pictures, 0x24-byte frame entries, then LZO-compressed RLE frames.
struct TswDirEntry {
    char name[0x18];
    std::uint32_t offset;
    std::uint8_t rest[0x10];
};

struct TswFrameEntry {
    std::uint32_t offset;
    std::uint32_t packedSize;
    std::uint32_t size;
    std::uint32_t flag;
    std::uint32_t reserved[4];
    std::uint16_t width;
    std::uint16_t height;
};

// Picture being assembled by the readers (the 0x18-byte local of 0x00433380/0x00433720).
struct TswFrameInfo {
    std::uint16_t* pixels;
    std::uint16_t* palette;
    std::uint32_t reserved8;
    std::uint32_t size;
    std::uint16_t format;
    std::uint16_t width;
    std::uint16_t height;
    std::uint16_t reserved16;
};

struct TswCacheNode {
    std::uint16_t id;
    std::uint16_t frame;
    TswCacheNode* next;
    Picture picture;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(TswDirEntry) == 0x2C);
static_assert(sizeof(TswFrameEntry) == 0x24);
static_assert(sizeof(TswFrameInfo) == 0x18);
static_assert(sizeof(TswCacheNode) == 0x20);
#endif

void TswSetCacheLimit(int bytes);
Picture* TswGetPicture(std::uint32_t id, std::uint32_t frame);
Picture* TswLoadPictureDirect(std::uint32_t id, std::uint32_t frame);
int TswCloseAll();
Picture* TswFindPicture(std::uint32_t id, std::uint32_t frame);
void TswReadMagicDirectory(HANDLE file);
void TswCacheMagicFrames(HANDLE file, int index);
int TswCacheLoad(std::uint32_t id, std::uint32_t frame);
Picture* TswCacheFind(std::uint32_t id, std::uint32_t frame);
void TswCacheEvict();
void TswCacheClear();
HANDLE TswOpenFile(LPCSTR fileName);
int TswReadHeader(HANDLE file, TswFrameInfo* info, std::uint8_t* header);
int TswLoadFromFile(LPCSTR fileName, int index, DWORD frame, Picture* picture);
int TswLoadPicture(HANDLE file, int index, DWORD frame, Picture* picture);
int TswReadFrame(HANDLE file, int base, TswFrameInfo* info, int frame);
int TswLoadPictureB(HANDLE file, int index, DWORD frame, Picture* picture);
int TswReadFrameB(HANDLE file, int base, TswFrameInfo* info, int frame);
void TswFreeBuffers();

}  // namespace swd3
