#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace openswd3::resource_io {

struct LegacyMemoryManagerSnapshot {
    bool private_heap_created{};
    compat::u32 allocated_payload_bytes{};
    compat::u32 peak_node_count{};
    compat::u32 current_node_count{};
    compat::u32 free_node_count{};
    compat::u32 active_node_count{};
    bool bucket_metadata_zero{};
};

class LegacyMemoryManager final {
public:
    static constexpr std::size_t kBucketCount = 32U;
    static constexpr std::size_t kErrorCapacity = 0x400U;
    static constexpr compat::u32 kInitialNodeCount = 32U;

    LegacyMemoryManager() noexcept;
    ~LegacyMemoryManager();

    LegacyMemoryManager(const LegacyMemoryManager&) = delete;
    LegacyMemoryManager& operator=(const LegacyMemoryManager&) = delete;
    LegacyMemoryManager(LegacyMemoryManager&&) = delete;
    LegacyMemoryManager& operator=(LegacyMemoryManager&&) = delete;

    [[nodiscard]] LegacyMemoryManagerSnapshot snapshot() const noexcept;
    [[nodiscard]] std::string_view error_message() const noexcept;

private:
    struct Node {
        std::byte* block{};
        compat::u32 payload_size{};
        compat::u32 reserved{};
        Node* next{};
    };

    struct Bucket {
        compat::u32 metadata{};
        Node* head{};
    };

    [[nodiscard]] Node* create_node() noexcept;
    [[nodiscard]] bool release_node(Node* node) noexcept;
    [[nodiscard]] bool release_contained_block(Node* node) noexcept;
    [[nodiscard]] bool destroy_private_heap() noexcept;
    void set_error(std::string_view message) noexcept;
    void set_system_error() noexcept;
    static void report_error(const char* caption, const char* message) noexcept;

    void* private_heap_{};
    compat::u32 reserved_counter_{};
    compat::u32 allocated_payload_bytes_{};
    compat::u32 peak_node_count_{};
    compat::u32 current_node_count_{};
    std::array<Bucket, kBucketCount> buckets_{};
    Node* free_nodes_{};
    std::array<char, kErrorCapacity> error_{};
};

}  // namespace openswd3::resource_io
