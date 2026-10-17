#include "memmgr/memory_manager.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include "core/format.h"
#include "platform/message_box.h"

namespace swd3
{

namespace
{

constexpr std::u8string_view kTitle = u8"MemMgr";
constexpr std::size_t kGuardBytes = 2 * sizeof(u32);  // two guard dwords before and after a block
constexpr std::size_t kShownBytes = 16;               // the user bytes a damaged block reports
constexpr std::size_t kShortMessage = 64;
constexpr std::size_t kLongMessage = 160;

u32 ReadDword(const std::vector<u8>& block, std::size_t offset)
{
    u32 value = 0;

    if (offset + 4 <= block.size())
    {
        std::memcpy(&value, block.data() + offset, 4);
    }

    return value;
}

}  // namespace

// swd32.exe 0x00438650: the private heap (with the page size as initial size) and 32 spare nodes
MemoryManager::MemoryManager(ErrorReporter& errors)
    : errors(errors)
{
    for (int i = 0; i < MemoryManager::SpareNodes; ++i)
    {
        std::unique_ptr<MemoryNode> node = this->NewNode();

        if (node != nullptr)
        {
            this->spareNodes.push_back(std::move(node));
        }
        else
        {
            this->errors.ShowError(kTitle, u8"MemMgrObject node Create Failed");
        }
    }
}

// swd32.exe 0x004386E0: reports and frees every block still held, then the spare nodes and the heap
MemoryManager::~MemoryManager()
{
    for (auto& bucket : this->buckets)
    {
        while (!bucket.empty())
        {
            std::unique_ptr<MemoryNode> node = std::move(bucket.back());
            bucket.pop_back();
            char message[kShortMessage];
            std::snprintf(message, sizeof(message), "Resource not release ,Size[%d]", static_cast<int>(node->size));
            this->text = Utf8Bytes(message);
            this->errors.ShowError(u8"MemMgr report", this->text);

            if (!this->FreeBlock(node.get()))
            {
                this->errors.ShowError(kTitle, this->text);
            }

            this->spareNodes.push_back(std::move(node));
        }
    }

    while (!this->spareNodes.empty())
    {
        std::unique_ptr<MemoryNode> node = std::move(this->spareNodes.back());
        this->spareNodes.pop_back();

        if (!this->FreeNode(std::move(node)))
        {
            this->errors.ShowError(kTitle, this->text);
        }
    }

    if (this->nodeCount != 0)
    {
        this->errors.ShowError(kTitle, u8"Resource node not match");
    }
}

// swd32.exe 0x00438800: counts the node and the peak count
std::unique_ptr<MemoryNode> MemoryManager::NewNode()
{
    auto node = std::make_unique<MemoryNode>();
    ++this->nodeCount;

    if (this->peakNodes < this->nodeCount)
    {
        this->peakNodes = this->nodeCount;
    }

    return node;
}

// swd32.exe 0x00438840
bool MemoryManager::FreeNode(std::unique_ptr<MemoryNode> node)
{
    if (node == nullptr || !this->FreeBlock(node.get()))
    {
        return false;
    }

    --this->nodeCount;
    return true;
}

// swd32.exe 0x00438880: releases the block of a node; a damaged guard word is reported with the first 16 user bytes
bool MemoryManager::FreeBlock(MemoryNode* node)
{
    if (node == nullptr)
    {
        this->text = u8"Internal error : NodeContainFree";
        return false;
    }

    bool intact = true;

    if (!node->block.empty())
    {
        const std::vector<u8>& block = node->block;
        const std::size_t tail = node->size;

        if (ReadDword(block, 0) != MemoryManager::Guard || ReadDword(block, sizeof(u32)) != MemoryManager::Guard ||
            ReadDword(block, tail + kGuardBytes) != MemoryManager::Guard ||
            ReadDword(block, tail + kGuardBytes + sizeof(u32)) != MemoryManager::Guard)
        {
            char message[kLongMessage];
            std::snprintf(message, sizeof(message), "block[0x%x][%d] overwrited! [", 0u, static_cast<int>(node->size));
            std::string shown = message;

            for (std::size_t i = 0; i < kShownBytes; ++i)
            {
                const u8 value = kGuardBytes + i < block.size() ? block[kGuardBytes + i] : u8{0};
                char cell[kShortMessage];
                std::snprintf(cell, sizeof(cell), i == 0 ? "%2X" : " %2X", value);
                shown += cell;
            }

            shown += ']';
            this->text = Utf8Bytes(shown);
            intact = false;
        }

        this->allocatedBytes -= node->size;
        node->block.clear();
        node->size = 0;
        node->unknown8 = 0;
        node->next = nullptr;
    }

    return intact;
}

// swd32.exe 0x004389A0: the text of the last system error
u32 MemoryManager::FormatLastError()
{
    this->text = Utf8Bytes(std::strerror(errno));
    return static_cast<u32>(this->text.size());
}

int MemoryManager::NodeCount() const
{
    return this->nodeCount;
}

}  // namespace swd3
