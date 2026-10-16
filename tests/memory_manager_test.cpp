#include "memmgr/memory_manager.h"
#include "recording_reporter.h"
#include "test.h"

TEST_CASE("memory manager: the spare nodes are released without reports")
{
    swd3test::RecordingReporter errors;
    {
        swd3::MemoryManager manager(errors);
        CHECK_EQ(manager.NodeCount(), 32);
    }

    CHECK(errors.messages.empty());
}

TEST_CASE("memory manager: a damaged guard word is reported")
{
    swd3test::RecordingReporter errors;
    swd3::MemoryManager manager(errors);
    swd3::MemoryNode node;
    node.size = 4;
    node.block.assign(20, 0);
    CHECK(!manager.FreeBlock(&node));
    CHECK(node.block.empty());
    CHECK(!manager.FreeBlock(nullptr));
}
