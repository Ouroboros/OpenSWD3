#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "core/types.h"

namespace swd3
{

class ErrorReporter;

// A tracked allocation: the block holds two guard words, the user bytes and two guard words.
struct MemoryNode
{
    std::vector<u8> block;
    u32 size = 0;
    u32 unknown8 = 0;
    MemoryNode* next = nullptr;
};

// "MemMgr" (the global object at 0x004B6C78): a private heap with 32 buckets of tracked blocks and a pool of spare
// nodes. The game only constructs and destroys it; the destructor reports blocks that were not released.
class MemoryManager
{
public:
    static constexpr u32 kGuard = 0xABCDDCBAu;
    static constexpr int kBuckets = 32;
    static constexpr int kSpareNodes = 32;

    explicit MemoryManager(ErrorReporter& errors);
    ~MemoryManager();
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    std::unique_ptr<MemoryNode> NewNode();
    bool FreeNode(std::unique_ptr<MemoryNode> node);
    bool FreeBlock(MemoryNode* node);
    u32 FormatLastError();
    int NodeCount() const;

private:
    ErrorReporter& errors;
    u32 allocatedBytes = 0;
    int peakNodes = 0;
    int nodeCount = 0;
    std::array<std::vector<std::unique_ptr<MemoryNode>>, kBuckets> buckets;
    std::vector<std::unique_ptr<MemoryNode>> spareNodes;
    std::u8string text;
};

}  // namespace swd3
