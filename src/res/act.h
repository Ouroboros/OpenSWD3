#pragma once

#include <cstdint>

#include "swd3/w32.h"

namespace swd3 {

// all_*.act action archives: 0x2C-byte directory entries at 0x2C * entry - 0x10 (size at +0x14,
// offset at +0x18); each entry holds a WORD count and a table of DWORD offsets to sub-entries.
struct ActIndexNode {
    ActIndexNode* next;
    int file;
    int entry;
    std::uint32_t offset;
    std::uint32_t size;
};

struct ActCacheNode {
    ActCacheNode* next;
    int id;
    int sub;
    std::uint32_t size;
    std::uint32_t reserved10;
    std::uint8_t* data;
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(ActIndexNode) == 0x14);
static_assert(sizeof(ActCacheNode) == 0x18);
#endif

void ActSetCacheLimit(int bytes);
HANDLE ActOpenFile(LPCSTR fileName);
std::uint8_t* ActExtractEntry(std::uint16_t* data, int index, int dataSize, std::uint32_t* size);
std::uint8_t* ActLoad(HANDLE file, int fileIndex, int entry, int sub, std::uint32_t* size);
std::uint8_t* ActGet(int id, int sub);
std::uint8_t* ActLoadDirect(int id, int sub);
int ActCacheLoad(int id, int sub);
int ActCacheFind(int id, int sub);
void ActCacheEvict();
void ActCacheClear();
int ActCloseAll();
void ActSetOption(int value);
ActIndexNode* ActIndexInsert(HANDLE file, int fileIndex, int entry, std::uint32_t* offset, std::uint32_t* size);
void ActIndexLookup(int fileIndex, int entry, std::uint32_t* offset, std::uint32_t* size);
void ActIndexClear();

}  // namespace swd3
