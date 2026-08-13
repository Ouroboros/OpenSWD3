#include "openswd3/resource_io/legacy_memory_manager.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace openswd3::resource_io {
namespace {

constexpr compat::u32 kGuard = 0xABCDDCBAU;

[[nodiscard]] compat::u32 read_u32(const std::byte* const address) noexcept {
    compat::u32 value{};
    std::memcpy(&value, address, sizeof(value));
    return value;
}

}  // namespace

LegacyMemoryManager::LegacyMemoryManager() noexcept {
#ifdef _WIN32
    SYSTEM_INFO system_info{};
    GetSystemInfo(&system_info);
    private_heap_ = HeapCreate(HEAP_NO_SERIALIZE, system_info.dwPageSize, 0U);
#else
    const long queried_page_size = ::sysconf(_SC_PAGESIZE);
    const std::size_t page_size = queried_page_size > 0
        ? static_cast<std::size_t>(queried_page_size)
        : 4096U;
    private_heap_ = std::malloc(page_size);
#endif

    reserved_counter_ = 0U;
    allocated_payload_bytes_ = 0U;
    peak_node_count_ = 0U;
    current_node_count_ = 0U;
    for (Bucket& bucket : buckets_) {
        bucket.metadata = 0U;
        bucket.head = nullptr;
    }

    for (compat::u32 index = 0U; index < kInitialNodeCount; ++index) {
        Node* const node = create_node();
        node->next = free_nodes_;
        free_nodes_ = node;
    }
}

LegacyMemoryManager::~LegacyMemoryManager() {
    for (Bucket& bucket : buckets_) {
        while (bucket.head != nullptr) {
            Node* const node = bucket.head;
            bucket.head = node->next;

            static_cast<void>(std::snprintf(
                error_.data(),
                error_.size(),
                "Resource not release ,Size[%d]",
                static_cast<int>(node->payload_size)
            ));
            report_error("MemMgr report", error_.data());

            if (!release_contained_block(node)) {
                report_error("MemMgr", error_.data());
            }
            node->next = free_nodes_;
            free_nodes_ = node;
        }
    }

    while (free_nodes_ != nullptr) {
        Node* const node = free_nodes_;
        free_nodes_ = node->next;
        if (!release_node(node)) {
            report_error("MemMgr", error_.data());
        }
    }

    if (current_node_count_ != 0U) {
        report_error("MemMgr", "Resource node not match");
    }
    if (!destroy_private_heap()) {
        set_system_error();
        report_error("MemMgr", error_.data());
    }
}

LegacyMemoryManagerSnapshot LegacyMemoryManager::snapshot() const noexcept {
    LegacyMemoryManagerSnapshot result{
        .private_heap_created = private_heap_ != nullptr,
        .allocated_payload_bytes = allocated_payload_bytes_,
        .peak_node_count = peak_node_count_,
        .current_node_count = current_node_count_,
        .free_node_count = 0U,
        .active_node_count = 0U,
        .bucket_metadata_zero = true,
    };

    for (const Node* node = free_nodes_; node != nullptr; node = node->next) {
        ++result.free_node_count;
    }
    for (const Bucket& bucket : buckets_) {
        result.bucket_metadata_zero =
            result.bucket_metadata_zero && bucket.metadata == 0U;
        for (const Node* node = bucket.head; node != nullptr;
             node = node->next) {
            ++result.active_node_count;
        }
    }
    return result;
}

std::string_view LegacyMemoryManager::error_message() const noexcept {
    return std::string_view{error_.data()};
}

LegacyMemoryManager::Node* LegacyMemoryManager::create_node() noexcept {
    auto* const node = static_cast<Node*>(std::malloc(sizeof(Node)));
    if (node == nullptr) {
        std::abort();
    }

    node->block = nullptr;
    node->payload_size = 0U;
    node->reserved = 0U;
    node->next = nullptr;

    ++current_node_count_;
    if (peak_node_count_ < current_node_count_) {
        peak_node_count_ = current_node_count_;
    }
    return node;
}

bool LegacyMemoryManager::release_node(Node* const node) noexcept {
    if (node == nullptr || !release_contained_block(node)) {
        return false;
    }

    std::free(node);
    --current_node_count_;
    return true;
}

bool LegacyMemoryManager::release_contained_block(Node* const node) noexcept {
    if (node == nullptr) {
        set_error("Internal error : NodeContainFree");
        return false;
    }
    if (node->block == nullptr) {
        return true;
    }

    std::byte* const suffix =
        node->block + 8U + static_cast<std::size_t>(node->payload_size);
    const bool guards_intact = read_u32(node->block) == kGuard &&
        read_u32(node->block + 4U) == kGuard && read_u32(suffix) == kGuard &&
        read_u32(suffix + 4U) == kGuard;

    if (!guards_intact) {
        const std::byte* const payload = node->block + 8U;
        std::array<unsigned int, 16> bytes{};
        std::transform(
            payload,
            payload + bytes.size(),
            bytes.begin(),
            [](const std::byte value) {
                return std::to_integer<unsigned int>(value);
            }
        );
        static_cast<void>(std::snprintf(
            error_.data(),
            error_.size(),
            "block[0x%x][%d] overwrited! " "[%2X %2X %2X %2X %2X %2X %2X %2X " "%2X %2X %2X %2X %2X %2X %2X %2X]",
            static_cast<unsigned int>(
                reinterpret_cast<std::uintptr_t>(payload)
            ),
            static_cast<int>(node->payload_size),
            bytes[0],
            bytes[1],
            bytes[2],
            bytes[3],
            bytes[4],
            bytes[5],
            bytes[6],
            bytes[7],
            bytes[8],
            bytes[9],
            bytes[10],
            bytes[11],
            bytes[12],
            bytes[13],
            bytes[14],
            bytes[15]
        ));
    }

    std::free(node->block);
    allocated_payload_bytes_ -= node->payload_size;
    node->payload_size = 0U;
    node->reserved = 0U;
    node->block = nullptr;
    node->next = nullptr;
    return guards_intact;
}

bool LegacyMemoryManager::destroy_private_heap() noexcept {
#ifdef _WIN32
    const bool destroyed = HeapDestroy(private_heap_) != 0;
    if (destroyed) {
        private_heap_ = nullptr;
    }
    return destroyed;
#else
    const bool destroyed = private_heap_ != nullptr;
    std::free(private_heap_);
    private_heap_ = nullptr;
    return destroyed;
#endif
}

void LegacyMemoryManager::set_error(const std::string_view message) noexcept {
    error_.fill('\0');
    const std::size_t size = std::min(message.size(), error_.size() - 1U);
    std::memcpy(error_.data(), message.data(), size);
}

void LegacyMemoryManager::set_system_error() noexcept {
#ifdef _WIN32
    error_.fill('\0');
    static_cast<void>(FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        GetLastError(),
        0U,
        error_.data(),
        static_cast<DWORD>(error_.size()),
        nullptr
    ));
#else
    set_error(std::strerror(errno));
#endif
}

void LegacyMemoryManager::report_error(
    const char* const caption, const char* const message
) noexcept {
#ifdef _WIN32
    static_cast<void>(MessageBoxA(nullptr, message, caption, MB_ICONERROR));
#else
    std::fprintf(stderr, "%s: %s\n", caption, message);
#endif
}

}  // namespace openswd3::resource_io
