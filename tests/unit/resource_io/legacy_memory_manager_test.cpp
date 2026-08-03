#include "test.hpp"

#include "openswd3/resource_io/legacy_memory_manager.hpp"

#include <cstddef>

namespace {

using openswd3::resource_io::LegacyMemoryManager;

void expect_initial_state(openswd3::test::Context& test) {
    LegacyMemoryManager manager;
    const auto state = manager.snapshot();

    test.expect_true(
        state.private_heap_created,
        "the process-private heap is created"
    );
    test.expect_equal(
        state.allocated_payload_bytes,
        0U,
        "no guarded payload is reachable in the current binary"
    );
    test.expect_equal(
        state.peak_node_count,
        LegacyMemoryManager::kInitialNodeCount,
        "the constructor records a 32-node peak"
    );
    test.expect_equal(
        state.current_node_count,
        LegacyMemoryManager::kInitialNodeCount,
        "the constructor owns 32 nodes"
    );
    test.expect_equal(
        state.free_node_count,
        LegacyMemoryManager::kInitialNodeCount,
        "all initial nodes are on the free list"
    );
    test.expect_equal(
        state.active_node_count,
        0U,
        "all 32 active bucket lists start empty"
    );
    test.expect_true(
        state.bucket_metadata_zero,
        "all 32 bucket metadata fields start at zero"
    );
    test.expect_true(
        manager.error_message().empty(),
        "the 0x400-byte report buffer starts empty"
    );
    test.expect_equal(
        LegacyMemoryManager::kErrorCapacity,
        std::size_t{0x400U},
        "the report buffer retains its assembly size"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    for (int iteration = 0; iteration < 4; ++iteration) {
        expect_initial_state(test);
    }
    return test.exit_code();
}
