#include "res/tsw.h"

#include <cstring>

#include "codec/lzo.h"
#include "core/debug_log.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "map/map_cache.h"
#include "swd3/mss.h"

namespace swd3 {
namespace {

constexpr UINT kErrorBox = 0x40010;

const char* str(const void* p) { return static_cast<const char*>(p); }

// Offset field of magic directory entry (index - 1); index 0 reads 0x004DAD14 (tswMagicSlots[1]).
std::int32_t& magicDirOffset(int index) { return image::at<std::int32_t>(0x004DAD14u + 0x2Cu * static_cast<std::uint32_t>(index)); }

TswFrameEntry& magicFrame(int slot, int frame) {
    return image::at<TswFrameEntry>(0x004CF988u + 0x24u * static_cast<std::uint32_t>(slot * 0xFF + frame));
}

void openArchives() {
    w32::lstrcpyA(G.tswPath, G.gameDir);
    w32::lstrcatA(G.tswPath, G.aAllCharTsw);
    G.tswFiles[0] = TswOpenFile(G.tswPath);
    w32::lstrcpyA(G.tswPath, G.gameDir);
    w32::lstrcatA(G.tswPath, G.aAllItemTsw);
    G.tswFiles[1] = TswOpenFile(G.tswPath);
    w32::lstrcpyA(G.tswPath, G.gameDir);
    w32::lstrcatA(G.tswPath, G.aAllMagicTsw);
    G.tswFiles[2] = TswOpenFile(G.tswPath);
    w32::lstrcpyA(G.tswPath, G.gameDir);
    w32::lstrcatA(G.tswPath, G.aAllSysTsw);
    G.tswFiles[3] = TswOpenFile(G.tswPath);
    w32::lstrcpyA(G.tswPath, G.gameDir);
    w32::lstrcatA(G.tswPath, G.aAllMap1Tsw);
    G.tswFiles[4] = TswOpenFile(G.tswPath);
    w32::lstrcpyA(G.tswPath, G.gameDir);
    w32::lstrcatA(G.tswPath, G.aAllMap2Tsw);
    G.tswFiles[5] = TswOpenFile(G.tswPath);
    std::memset(G.tswMagicDir, 0, sizeof(G.tswMagicDir));
    std::memset(G.tswMagicFrames, 0, sizeof(G.tswMagicFrames));
    TswReadMagicDirectory(G.tswFiles[2]);
    G.tswOpened = 1;
}

void freePicture(TswCacheNode* node) {
    crt::free(node->picture.pixels);
    crt::free(node->picture.reserved4);
    crt::free(node->picture.palette);
    crt::free(node);
}

}  // namespace

// swd32.exe 0x004315C0
void TswSetCacheLimit(int bytes) { G.tswCacheLimit = bytes; }

// swd32.exe 0x004315D0
Picture* TswGetPicture(std::uint32_t id, std::uint32_t frame) {
    if (!G.tswOpened) {
        openArchives();
    }
    if (G.tswCacheBytes >= G.tswCacheLimit) {
        TswCacheEvict();
    }
    if (!TswCacheFind(id, frame) && !TswCacheLoad(id, frame)) {
        G.tswCurrent = nullptr;
    }
    return G.tswCurrent;
}

// swd32.exe 0x00431760
Picture* TswLoadPictureDirect(std::uint32_t id, std::uint32_t frame) {
    if (!G.tswOpened) {
        openArchives();
    }
    const auto id16 = static_cast<std::uint16_t>(id);
    G.tswMagicMode = 0;
    if (id16 >= 0x1771 && id16 <= 0x2328) {
        G.tswMagicMode = 1;
        TswCacheMagicFrames(G.tswFiles[2], static_cast<int>(id16) % 0xBB8);
    }
    G.tswDirectPicture = static_cast<Picture*>(crt::malloc(0x14));
    const int id32 = static_cast<int>(id & 0xFFFF);
    TswLoadPictureB(G.tswFiles[id32 / 0xBB8], id32 % 0xBB8, frame & 0xFFFF, G.tswDirectPicture);
    RleConvert8To16(G.tswDirectPicture);
    G.tswMagicMode = 0;
    return G.tswDirectPicture;
}

// swd32.exe 0x00431960
int TswCloseAll() {
    for (HANDLE& file : G.tswFiles) {
        if (file != INVALID_HANDLE_VALUE) {
            w32::CloseHandle(file);
        }
        file = INVALID_HANDLE_VALUE;
    }
    if (G.tswExtraFile != INVALID_HANDLE_VALUE && G.tswExtraFile != nullptr) {
        w32::CloseHandle(G.tswExtraFile);
        G.tswExtraFile = INVALID_HANDLE_VALUE;
    }
    G.tswOpened = 0;
    G.tswMagicMode = 0;
    TswCacheClear();
    TswFreeBuffers();
    for (int i = 0; i < 5; ++i) {
        G.tswMagicAges[i] = 0;
        G.tswMagicSlots[i] = 0;
    }
    std::memset(G.tswMagicDir, 0, sizeof(G.tswMagicDir));
    std::memset(G.tswMagicFrames, 0, sizeof(G.tswMagicFrames));
    return 0;
}

// swd32.exe 0x00431A20
Picture* TswFindPicture(std::uint32_t id, std::uint32_t frame) {
    G.tswCurrent = nullptr;
    TswCacheFind(id, frame);
    return G.tswCurrent;
}

// swd32.exe 0x00431A50
void TswReadMagicDirectory(HANDLE file) {
    for (TswDirEntry& entry : G.tswMagicDir) {
        auto offset = static_cast<LONG>(0x1C + (reinterpret_cast<std::uint8_t*>(&entry) -
                                                reinterpret_cast<std::uint8_t*>(G.tswMagicDir)));
        w32::SetFilePointer(file, offset, nullptr, FILE_BEGIN);
        DWORD read;
        w32::ReadFile(file, &entry, 0x2C, &read, nullptr);
    }
}

// swd32.exe 0x00431AA0
void TswCacheMagicFrames(HANDLE file, int index) {
    std::uint8_t header[12];
    TswFrameInfo info;
    int slot = 0;
    for (;;) {
        int current = G.tswMagicSlots[slot];
        if (current == index) {
            w32::SetFilePointer(file, magicDirOffset(index), nullptr, FILE_BEGIN);
            TswReadHeader(file, &info, header);
            G.tswMagicAges[slot] = 0;
            G.tswMagicPalette = info.palette;
            return;
        }
        if (current == 0) {
            G.tswMagicSlots[slot] = index;
            G.tswMagicAges[slot] = 0;
            goto load;
        }
        ++G.tswMagicAges[slot];
        ++slot;
        if (slot >= 5) {
            break;
        }
    }
    {
        int oldest = G.tswMagicAges[0];
        slot = 0;
        for (int i = 0; i < 5; ++i) {
            if (oldest < G.tswMagicAges[i]) {
                oldest = G.tswMagicAges[i];
                slot = i;
            }
        }
        G.tswMagicSlots[slot] = index;
        G.tswMagicAges[slot] = 0;
    }
load:
    w32::SetFilePointer(file, magicDirOffset(index), nullptr, FILE_BEGIN);
    TswReadHeader(file, &info, header);
    G.tswMagicPalette = info.palette;
    std::uint16_t count;
    std::memcpy(&count, header + 6, 2);
    LONG skip = 0;
    for (int frame = 0; frame < count; ++frame) {
        TswFrameEntry& entry = magicFrame(slot, frame);
        w32::SetFilePointer(file, magicDirOffset(index), nullptr, FILE_BEGIN);
        TswReadHeader(file, &info, header);
        w32::SetFilePointer(file, skip, nullptr, FILE_CURRENT);
        DWORD read;
        w32::ReadFile(file, &entry, 0x24, &read, nullptr);
        entry.offset = w32::SetFilePointer(file, magicDirOffset(index) + static_cast<LONG>(entry.offset), nullptr, FILE_BEGIN);
        skip += 0x24;
    }
}

// swd32.exe 0x00431C50
int TswCacheLoad(std::uint32_t id, std::uint32_t frame) {
    const std::uint32_t id32 = id & 0xFFFF;
    int bucket = static_cast<int>(id32) % 10;
    if (static_cast<std::uint16_t>(id) == 0xFFFF) {
        bucket = static_cast<int>(frame & 0xFFFF) % 10;
    }
    bucket &= 0xFFFF;
    TswCacheNode* sentinel = &G.tswBuckets[bucket];
    ++G.tswBucketCounts[bucket];
    G.tswCursor = sentinel;
    TswCacheNode* oldHead = sentinel->next;
    sentinel->next = static_cast<TswCacheNode*>(crt::malloc(0x20));
    std::memset(G.tswCursor->next, 0, 0x20);
    G.tswCursor = G.tswCursor->next;
    G.tswCursor->id = static_cast<std::uint16_t>(id);
    G.tswCursor->frame = static_cast<std::uint16_t>(frame);
    G.tswCursor->next = oldHead;
    if (static_cast<std::uint16_t>(id) == 0xFFFF) {
        LoadCachedMapPicture(frame & 0xFFFF, &G.tswCursor->picture);
    } else {
        G.tswMagicMode = 0;
        if (static_cast<std::uint16_t>(id) >= 0x1771 && static_cast<std::uint16_t>(id) <= 0x2328) {
            G.tswMagicMode = 1;
            TswCacheMagicFrames(G.tswFiles[2], static_cast<int>(id32) % 0xBB8);
        }
        if (!TswLoadPicture(G.tswFiles[static_cast<int>(id32) / 0xBB8], static_cast<int>(id32) % 0xBB8, frame & 0xFFFF,
                            &G.tswCursor->picture)) {
            DebugLog(G.aMemErrorTswDPa, id32, frame & 0xFFFF);
            return 0;
        }
        RleConvert8To16(&G.tswCursor->picture);
        G.tswMagicMode = 0;
    }
    G.tswCurrent = &G.tswCursor->picture;
    G.tswCacheBytes += static_cast<std::int32_t>(G.tswCursor->picture.size);
    return 1;
}

// swd32.exe 0x00431DF0
Picture* TswCacheFind(std::uint32_t id, std::uint32_t frame) {
    const std::uint32_t id32 = id & 0xFFFF;
    int bucket = static_cast<int>(id32) % 10;
    G.tswCurrent = nullptr;
    if (static_cast<std::uint16_t>(id) == 0xFFFF) {
        bucket = static_cast<int>(frame & 0xFFFF) % 10;
    }
    bucket &= 0xFFFF;
    const std::uint32_t key = ((frame & 0xFFFF) << 16) | id32;
    TswCacheNode** headSlot = &G.tswBuckets[bucket].next;
    TswCacheNode* head = *headSlot;
    TswCacheNode* node = head;
    G.tswCursor = node;
    TswCacheNode* prev = node;
    if (!node) {
        return nullptr;
    }
    for (;;) {
        std::uint32_t nodeKey;
        std::memcpy(&nodeKey, node, 4);
        if (nodeKey == key) {
            break;
        }
        prev = node;
        node = node->next;
        G.tswCursor = node;
        if (!node) {
            return nullptr;
        }
    }
    if (node != head) {
        prev->next = node->next;
        G.tswCursor->next = *headSlot;
        node = G.tswCursor;
        *headSlot = G.tswCursor;
    }
    G.tswCurrent = &node->picture;
    return G.tswCurrent;
}

// swd32.exe 0x00431EA0
void TswCacheEvict() {
    std::uint16_t best = 0;
    for (std::uint16_t i = 0; i < 10; ++i) {
        if (G.tswBucketCounts[i] > G.tswBucketCounts[best]) {
            best = i;
        }
    }
    if (G.tswCacheBytes < G.tswCacheLimit) {
        return;
    }
    TswCacheNode* sentinel = &G.tswBuckets[best];
    do {
        TswCacheNode* node = sentinel;
        G.tswCursor = node;
        TswCacheNode* prev = node;
        while (node->next) {
            prev = node;
            node = node->next;
            G.tswCursor = node;
        }
        G.tswCacheBytes -= static_cast<std::int32_t>(node->picture.size);
        crt::free(node->picture.pixels);
        crt::free(G.tswCursor->picture.reserved4);
        crt::free(G.tswCursor->picture.palette);
        crt::free(G.tswCursor);
        prev->next = nullptr;
        if (--G.tswBucketCounts[best] == 0) {
            return;
        }
    } while (G.tswCacheBytes >= G.tswCacheLimit);
}

// swd32.exe 0x00431F80
void TswCacheClear() {
    for (int i = 0; i < 10; ++i) {
        TswCacheNode* node = G.tswBuckets[i].next;
        G.tswBuckets[i].next = nullptr;
        G.tswCursor = node;
        while (node) {
            TswCacheNode* next = node->next;
            G.tswCacheBytes -= static_cast<std::int32_t>(node->picture.size);
            freePicture(G.tswCursor);
            --G.tswBucketCounts[i];
            node = next;
            G.tswCursor = node;
        }
    }
}

// swd32.exe 0x00433270
HANDLE TswOpenFile(LPCSTR fileName) {
    HANDLE file = w32::CreateFileA(fileName, GENERIC_READ, 1, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    return file == INVALID_HANDLE_VALUE ? nullptr : file;
}

// swd32.exe 0x004332A0
int TswReadHeader(HANDLE file, TswFrameInfo* info, std::uint8_t* header) {
    mss::AIL_serve();
    DWORD read;
    w32::ReadFile(file, header, 0xC, &read, nullptr);
    std::uint16_t magic;
    std::memcpy(&magic, header + 4, 2);
    if (magic != 0xABCD) {
        return 0;
    }
    std::uint16_t format;
    std::memcpy(&format, header + 8, 2);
    info->format = format;
    if (format == 8) {
        info->palette = static_cast<std::uint16_t*>(crt::malloc(0x200));
        w32::ReadFile(file, info->palette, 0x200, &read, nullptr);
    }
    return 1;
}

// swd32.exe 0x00433320
int TswLoadFromFile(LPCSTR fileName, int index, DWORD frame, Picture* picture) {
    HANDLE file = TswOpenFile(fileName);
    if (!file) {
        w32::MessageBoxA(G.mainWindow, G.aTsw, str(&G.byte_4A60B0), kErrorBox);
        return 0;
    }
    int result = TswLoadPicture(file, index, frame, picture);
    if (!result) {
        return result;
    }
    w32::CloseHandle(file);
    return 1;
}

// swd32.exe 0x00433380. In magic mode the original leaves the directory-entry local uninitialised and an
// error message may print stack garbage; it is zero-initialised here.
int TswLoadPicture(HANDLE file, int index, DWORD frame, Picture* picture) {
    TswDirEntry entry{};
    TswFrameInfo info{};
    std::uint8_t header[12];
    if (G.tswMagicMode == 1) {
        int slot = 0;
        for (;; ++slot) {
            if (slot == 5) {
                slot = G.tswMagicSlot;
                break;
            }
            if (G.tswMagicSlots[slot] == index) {
                G.tswMagicSlot = slot;
                break;
            }
        }
        w32::SetFilePointer(file, static_cast<LONG>(magicFrame(slot, static_cast<int>(frame)).offset), nullptr, FILE_BEGIN);
    } else {
        mss::AIL_serve();
        w32::SetFilePointer(file, 0x2C * index - 0x10, nullptr, FILE_BEGIN);
        DWORD read;
        w32::ReadFile(file, &entry, 0x2C, &read, nullptr);
        w32::SetFilePointer(file, static_cast<LONG>(entry.offset), nullptr, FILE_BEGIN);
        if (!TswReadHeader(file, &info, header)) {
            w32::wsprintfA(G.tswErrorText, G.aTswD_0, index, entry.name);
            DebugLog(G.tswErrorText);
            w32::MessageBoxA(G.mainWindow, G.tswErrorText, str(&G.byte_4A60B0), kErrorBox);
            return 0;
        }
    }
    if (!TswReadFrame(file, static_cast<int>(entry.offset), &info, static_cast<int>(frame))) {
        w32::wsprintfA(G.tswErrorText, G.aTswD, index, entry.name, frame);
        DebugLog(G.tswErrorText);
        w32::MessageBoxA(G.mainWindow, G.tswErrorText, str(&G.byte_4A60B0), kErrorBox);
        return 0;
    }
    picture->pixels = info.pixels;
    picture->width = info.width;
    picture->height = info.height;
    picture->reserved4 = reinterpret_cast<void*>(static_cast<std::uintptr_t>(info.reserved8));
    picture->palette = info.palette;
    picture->size = info.size;
    return 1;
}

// swd32.exe 0x00433540
int TswReadFrame(HANDLE file, int base, TswFrameInfo* info, int frame) {
    DWORD read = 0;
    std::uint32_t outLength = 0;
    TswFrameEntry entry;
    if (G.tswMagicMode == 1) {
        entry = magicFrame(G.tswMagicSlot, frame);
    } else {
        mss::AIL_serve();
        const DWORD dirBytes = static_cast<DWORD>((frame * 9 + 9) * 4);
        auto* dir = static_cast<std::uint8_t*>(crt::malloc(dirBytes));
        w32::ReadFile(file, dir, dirBytes, &read, nullptr);
        std::memcpy(&entry, dir + frame * 0x24, sizeof(entry));
        w32::SetFilePointer(file, static_cast<LONG>(entry.offset) + base, nullptr, FILE_BEGIN);
        crt::free(dir);
    }
    const std::uint32_t packed = entry.packedSize;
    if (packed > G.tswPackedCap) {
        if (G.tswPackedBuf) {
            crt::free(G.tswPackedBuf);
        }
        G.tswPackedBuf = static_cast<std::uint8_t*>(crt::malloc(packed));
        G.tswPackedCap = packed;
    }
    mss::AIL_serve();
    w32::ReadFile(file, G.tswPackedBuf, packed, &read, nullptr);
    const std::uint32_t size = entry.size;
    auto* out = static_cast<std::uint16_t*>(crt::malloc(size));
    if (G.tswPackedBuf && out) {
        mss::AIL_serve();
        Lzo1xDecompress(G.tswPackedBuf, packed, reinterpret_cast<std::uint8_t*>(out), &outLength);
        mss::AIL_serve();
        if (outLength != size) {
            crt::free(G.tswPackedBuf);
            G.tswPackedBuf = nullptr;
            w32::MessageBoxA(G.mainWindow, str(&G.byte_4A61F4), str(&G.byte_4A60B0), kErrorBox);
            return 0;
        }
        info->size = size;
        if (entry.flag != 0) {
            info->reserved8 = 0;
        }
        info->pixels = out;
        info->width = entry.width;
        info->height = entry.height;
        return 1;
    }
    DebugLog(str(&G.byte_4A61DC));
    w32::MessageBoxA(G.mainWindow, str(&G.byte_4A61C4), str(&G.byte_4A60B0), kErrorBox);
    return 0;
}

// swd32.exe 0x00433720
int TswLoadPictureB(HANDLE file, int index, DWORD frame, Picture* picture) {
    TswDirEntry entry{};
    TswFrameInfo info{};
    std::uint8_t header[12];
    if (G.tswMagicMode == 1) {
        int slot = 0;
        for (;; ++slot) {
            if (slot == 5) {
                slot = G.tswMagicSlot;
                break;
            }
            if (G.tswMagicSlots[slot] == index) {
                G.tswMagicSlot = slot;
                break;
            }
        }
        w32::SetFilePointer(file, static_cast<LONG>(magicFrame(slot, static_cast<int>(frame)).offset), nullptr, FILE_BEGIN);
    } else {
        mss::AIL_serve();
        w32::SetFilePointer(file, 0x2C * index - 0x10, nullptr, FILE_BEGIN);
        DWORD read;
        w32::ReadFile(file, &entry, 0x2C, &read, nullptr);
        w32::SetFilePointer(file, static_cast<LONG>(entry.offset), nullptr, FILE_BEGIN);
        if (!TswReadHeader(file, &info, header)) {
            w32::wsprintfA(G.tswErrorText, G.aTswD_0, index, entry.name);
            w32::MessageBoxA(G.mainWindow, G.tswErrorText, str(&G.byte_4A60B0), kErrorBox);
            return 0;
        }
    }
    if (!TswReadFrameB(file, static_cast<int>(entry.offset), &info, static_cast<int>(frame))) {
        w32::wsprintfA(G.tswErrorText, G.aTswD, index, entry.name, frame);
        w32::MessageBoxA(G.mainWindow, G.tswErrorText, str(&G.byte_4A60B0), kErrorBox);
        return 0;
    }
    picture->pixels = info.pixels;
    picture->width = info.width;
    picture->height = info.height;
    picture->reserved4 = reinterpret_cast<void*>(static_cast<std::uintptr_t>(info.reserved8));
    if (G.tswMagicMode == 1) {
        picture->palette = G.tswMagicPalette;
    } else {
        picture->palette = info.palette;
    }
    picture->size = info.size;
    return 1;
}

// swd32.exe 0x004338F0
int TswReadFrameB(HANDLE file, int base, TswFrameInfo* info, int frame) {
    DWORD read = 0;
    std::uint32_t outLength = 0;
    TswFrameEntry entry;
    if (G.tswMagicMode == 1) {
        entry = magicFrame(G.tswMagicSlot, frame);
    } else {
        w32::SetFilePointer(file, frame * 0x24, nullptr, FILE_CURRENT);
        w32::ReadFile(file, &entry, 0x24, &read, nullptr);
        w32::SetFilePointer(file, static_cast<LONG>(entry.offset) + base, nullptr, FILE_BEGIN);
    }
    mss::AIL_serve();
    const std::uint32_t packed = entry.packedSize;
    if (packed > G.tswPackedCapB) {
        if (G.tswPackedBufB) {
            crt::free(G.tswPackedBufB);
        }
        G.tswPackedBufB = static_cast<std::uint8_t*>(crt::malloc(packed));
        G.tswPackedCapB = packed;
    }
    w32::ReadFile(file, G.tswPackedBufB, packed, &read, nullptr);
    const std::uint32_t size = entry.size;
    auto* out = static_cast<std::uint16_t*>(crt::malloc(size));
    if (G.tswPackedBufB && out) {
        Lzo1xDecompress(G.tswPackedBufB, packed, reinterpret_cast<std::uint8_t*>(out), &outLength);
        if (outLength != size) {
            crt::free(G.tswPackedBufB);
            crt::free(out);
            G.tswPackedBufB = nullptr;
            w32::MessageBoxA(G.mainWindow, str(&G.byte_4A61F4), str(&G.byte_4A60B0), kErrorBox);
            return 0;
        }
        info->size = size;
        if (entry.flag != 0) {
            info->reserved8 = 0;
        }
        info->pixels = out;
        info->width = entry.width;
        info->height = entry.height;
        return 1;
    }
    w32::MessageBoxA(G.mainWindow, str(&G.byte_4A61C4), str(&G.byte_4A60B0), kErrorBox);
    return 0;
}

// swd32.exe 0x00433BF0
void TswFreeBuffers() {
    if (G.tswPackedBuf) {
        crt::free(G.tswPackedBuf);
        G.tswPackedBuf = nullptr;
    }
    if (G.tswPackedBufB) {
        crt::free(G.tswPackedBufB);
        G.tswPackedBufB = nullptr;
    }
    G.tswPackedCap = 0;
    G.tswPackedCapB = 0;
}

}  // namespace swd3
