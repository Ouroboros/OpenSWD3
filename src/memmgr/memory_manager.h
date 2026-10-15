#pragma once

#include "swd3/w32.h"

namespace swd3 {

struct MemoryNode {
    std::uint8_t* block;
    DWORD size;
    DWORD reserved;
    MemoryNode* next;
};

struct MemoryBucket {
    DWORD reserved0;
    DWORD reserved4;
    DWORD reserved8;
    MemoryNode* head;
};

// Global "MemMgr" object (0x004B6C78). The game only constructs and destroys it.
class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    MemoryNode* NewNode();
    bool FreeNode(MemoryNode* node);
    bool FreeBlock(MemoryNode* node);
    DWORD FormatLastError();

    HANDLE m_heap;
    DWORD m_reserved4;
    DWORD m_allocatedBytes;
    int m_peakNodes;
    int m_nodeCount;
    MemoryBucket m_buckets[32];
    DWORD m_reserved214[3];
    MemoryNode* m_freeNodes;
    char m_text[0x400];
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(MemoryNode) == 0x10);
static_assert(offsetof(MemoryManager, m_buckets) == 0x14);
static_assert(offsetof(MemoryManager, m_freeNodes) == 0x220);
static_assert(offsetof(MemoryManager, m_text) == 0x224);
#endif

extern MemoryManager g_memoryManager;

}  // namespace swd3
