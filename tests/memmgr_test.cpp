#include <cstring>

#include "crt/crt.h"
#include "memmgr/memory_manager.h"
#include "test.h"

using namespace swd3;

TEST_CASE("memmgr.global object has 32 preallocated nodes") {
    CHECK_EQ(g_memoryManager.m_nodeCount, 32);
    CHECK_EQ(g_memoryManager.m_peakNodes, 32);
    int count = 0;
    for (MemoryNode* node = g_memoryManager.m_freeNodes; node; node = node->next) {
        ++count;
    }
    CHECK_EQ(count, 32);
}

TEST_CASE("memmgr.free block validates guards") {
    MemoryManager& mgr = g_memoryManager;
    MemoryNode node{};
    CHECK(mgr.FreeBlock(&node));
    CHECK(!mgr.FreeBlock(nullptr));
    CHECK(std::strcmp(mgr.m_text, "Internal error : NodeContainFree") == 0);

    auto* block = static_cast<std::uint8_t*>(crt::malloc(8 + 4 + 8));
    const DWORD guard = 0xABCDDCBAu;
    std::memcpy(block, &guard, 4);
    std::memcpy(block + 4, &guard, 4);
    std::memcpy(block + 12, &guard, 4);
    std::memcpy(block + 16, &guard, 4);
    node.block = block;
    node.size = 4;
    DWORD before = mgr.m_allocatedBytes;
    CHECK(mgr.FreeBlock(&node));
    CHECK_EQ(mgr.m_allocatedBytes, before - 4);
    CHECK(node.block == nullptr);

    block = static_cast<std::uint8_t*>(crt::malloc(8 + 16 + 8));
    std::memset(block, 0, 8 + 16 + 8);
    node.block = block;
    node.size = 16;
    CHECK(!mgr.FreeBlock(&node));
    CHECK(std::strstr(mgr.m_text, "overwrited!") != nullptr);
    mgr.m_allocatedBytes = before;
}
