#pragma once

#include <cstddef>
#include <memory_resource>
#include <new>

namespace openswd3::test {

class FixedNodeMemoryResource final : public std::pmr::memory_resource {
public:
    bool allocation_enabled{true};
    std::size_t outstanding_blocks{};

private:
    void*
    do_allocate(const std::size_t bytes, const std::size_t alignment) override {
        if (!allocation_enabled) {
            throw std::bad_alloc{};
        }

        void* const block =
            std::pmr::new_delete_resource()->allocate(bytes, alignment);
        ++outstanding_blocks;
        return block;
    }

    void do_deallocate(
        void* const block, const std::size_t bytes, const std::size_t alignment
    ) override {
        std::pmr::new_delete_resource()->deallocate(block, bytes, alignment);
        --outstanding_blocks;
    }

    bool do_is_equal(
        const std::pmr::memory_resource& other
    ) const noexcept override {
        return this == &other;
    }
};

}  // namespace openswd3::test
