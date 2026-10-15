#include "map/map_cache.h"

#include <cstring>

#include "codec/lzo.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "swd3/mss.h"

namespace swd3 {

// swd32.exe 0x0040AD10: reads one unit of the cached map file (data\%d.cm) as a picture
int LoadCachedMapPicture(std::uint32_t unit, Picture* picture) {
    picture->pixels = nullptr;
    if (unit > static_cast<std::uint32_t>(G.mapUnitCount)) {
        return 0;
    }
    DWORD size = static_cast<DWORD>(G.mapUnitSizes[unit] + 0x10);
    auto* block = static_cast<std::uint8_t*>(crt::malloc(size));
    mss::AIL_serve();
    G.mapCacheFile.SeekBegin(G.mapUnitOffsets[unit] + G.mapUnitBase);
    mss::AIL_serve();
    G.mapCacheFile.Read(block, &size);
    std::uint16_t word;
    std::memcpy(&word, block + 2, 2);
    picture->width = static_cast<std::uint16_t>(word << 4);
    std::memcpy(&word, block + 4, 2);
    picture->height = static_cast<std::uint16_t>(word << 4);
    std::uint16_t format;
    std::memcpy(&format, block + 6, 2);
    std::uint32_t unpacked;
    std::uint32_t packed;
    std::memcpy(&unpacked, block + 8, 4);
    std::memcpy(&packed, block + 0xC, 4);
    picture->pixels = static_cast<std::uint16_t*>(crt::malloc(unpacked));
    picture->size = unpacked;
    std::uint32_t outSize;
    MapDecompress(block + 0x10, reinterpret_cast<std::uint8_t*>(picture->pixels), packed, &outSize);
    crt::free(block);
    if (format == 0x10) {
        RleCheck16(picture->pixels, &picture->width, &picture->height, &format);
    } else {
        RleConvertTile8To16(&picture->pixels, &picture->size);
    }
    mss::AIL_serve();
    return 1;
}

// swd32.exe 0x00426820
int MapDecompress(const std::uint8_t* packed, std::uint8_t* out, std::uint32_t packedSize, std::uint32_t* outSize) {
    return Lzo1xDecompress(packed, packedSize, out, outSize);
}

}  // namespace swd3
