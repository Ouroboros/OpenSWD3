#include "memmgr/memory_manager.h"

#include <cstddef>
#include <cstring>

#include "crt/crt.h"

namespace swd3 {

// swd32.exe 0x004B6C78; construction 0x00401000/0x00401010, atexit registration 0x00401020,
// destruction 0x00401030
MemoryManager g_memoryManager;

namespace {

constexpr DWORD kGuard = 0xABCDDCBAu;

DWORD readDword(const std::uint8_t* p) {
    DWORD value;
    std::memcpy(&value, p, sizeof(value));
    return value;
}

}  // namespace

// swd32.exe 0x00438650
MemoryManager::MemoryManager() {
    SYSTEM_INFO info;
    w32::GetSystemInfo(&info);
    m_heap = w32::HeapCreate(1, info.dwPageSize, 0);
    m_allocatedBytes = 0;
    m_reserved4 = 0;
    m_peakNodes = 0;
    m_nodeCount = 0;
    for (MemoryBucket& bucket : m_buckets) {
        bucket.reserved0 = 0;
        bucket.head = nullptr;
    }
    for (int i = 0x20; i != 0; --i) {
        MemoryNode* node = NewNode();
        if (node) {
            node->next = m_freeNodes;
            m_freeNodes = node;
        } else {
            w32::MessageBoxA(nullptr, "MemMgrObject node Create Failed", "MemMgr", MB_ICONHAND);
        }
    }
}

// swd32.exe 0x004386E0
MemoryManager::~MemoryManager() {
    for (MemoryBucket& bucket : m_buckets) {
        while (bucket.head) {
            MemoryNode* node = bucket.head;
            bucket.head = node->next;
            w32::wsprintfA(m_text, "Resource not release ,Size[%d]", node->size);
            w32::MessageBoxA(nullptr, m_text, "MemMgr report", MB_ICONHAND);
            if (!FreeBlock(node)) {
                w32::MessageBoxA(nullptr, m_text, "MemMgr", MB_ICONHAND);
            }
            node->next = m_freeNodes;
            m_freeNodes = node;
        }
    }
    while (MemoryNode* node = m_freeNodes) {
        m_freeNodes = node->next;
        if (!FreeNode(node)) {
            w32::MessageBoxA(nullptr, m_text, "MemMgr", MB_ICONHAND);
        }
    }
    if (m_nodeCount) {
        w32::MessageBoxA(nullptr, "Resource node not match", "MemMgr", MB_ICONHAND);
    }
    if (!w32::HeapDestroy(m_heap)) {
        FormatLastError();
        w32::MessageBoxA(nullptr, m_text, "MemMgr", MB_ICONHAND);
    }
}

// swd32.exe 0x00438800
MemoryNode* MemoryManager::NewNode() {
    auto* node = static_cast<MemoryNode*>(crt::malloc(sizeof(MemoryNode)));
    node->block = nullptr;
    node->size = 0;
    node->reserved = 0;
    node->next = nullptr;
    if (!node) {
        return node;
    }
    ++m_nodeCount;
    if (m_peakNodes < m_nodeCount) {
        m_peakNodes = m_nodeCount;
    }
    return node;
}

// swd32.exe 0x00438840
bool MemoryManager::FreeNode(MemoryNode* node) {
    if (!node) {
        return false;
    }
    if (!FreeBlock(node)) {
        return false;
    }
    crt::free(node);
    --m_nodeCount;
    return true;
}

// swd32.exe 0x00438880
bool MemoryManager::FreeBlock(MemoryNode* node) {
    if (!node) {
        std::strcpy(m_text, "Internal error : NodeContainFree");
        return false;
    }
    bool ok = true;
    std::uint8_t* block = node->block;
    if (block) {
        const std::uint8_t* tail = block + node->size;
        if (readDword(block) != kGuard || readDword(block + 4) != kGuard || readDword(tail + 8) != kGuard ||
            readDword(tail + 0xC) != kGuard) {
            const std::uint8_t* user = block + 8;
            w32::wsprintfA(m_text,
                           "block[0x%x][%d] overwrited! [%2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X "
                           "%2X %2X]",
                           static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(user)), node->size, user[0], user[1],
                           user[2], user[3], user[4], user[5], user[6], user[7], user[8], user[9], user[10],
                           user[11], user[12], user[13], user[14], user[15]);
            ok = false;
        }
        crt::free(node->block);
        m_allocatedBytes -= node->size;
        node->size = 0;
        node->reserved = 0;
        node->block = nullptr;
        node->next = nullptr;
    }
    return ok;
}

// swd32.exe 0x004389A0
DWORD MemoryManager::FormatLastError() {
    return w32::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, w32::GetLastError(), 0, m_text, 0x400, nullptr);
}

}  // namespace swd3
