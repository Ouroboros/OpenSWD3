#include "memmgr/memory_manager.h"
#include "recording_reporter.h"
#include "test.h"

namespace
{

constexpr std::size_t kUserBytes = 4;
constexpr std::size_t kGuardWords = 4;  // two guard dwords before the user bytes and two after

}  // namespace

TEST_CASE("memory manager: the spare nodes are released without reports")
{
    swd3test::RecordingReporter errors;
    {
        swd3::MemoryManager manager(errors);
        CHECK_EQ(manager.NodeCount(), swd3::MemoryManager::SpareNodes);
    }

    CHECK(errors.messages.empty());
}

TEST_CASE("memory manager: a damaged guard word is reported")
{
    swd3test::RecordingReporter errors;
    swd3::MemoryManager manager(errors);
    swd3::MemoryNode node;
    node.size = kUserBytes;
    node.block.assign(kUserBytes + kGuardWords * sizeof(swd3::u32), 0);
    CHECK(!manager.FreeBlock(&node));
    CHECK(node.block.empty());
    CHECK(!manager.FreeBlock(nullptr));
}
