#pragma once

#include <cstddef>
#include <cstdint>

// Copy of swd32.exe's .idata/.rdata/.data/.bss (VA 0x00499000-0x0053F000). Global variables live at
// their original offsets so that out-of-bounds indexing in the original code reaches the same neighbours.
namespace swd3::image {

constexpr std::uint32_t kBase = 0x00499000u;
constexpr std::uint32_t kEnd = 0x0053F000u;
constexpr std::uint32_t kSize = kEnd - kBase;
constexpr std::uint32_t kRawEnd = 0x004AA000u;
constexpr std::uint32_t kCodeBase = 0x00401000u;
constexpr std::uint32_t kCodeEnd = 0x00499000u;

extern std::uint8_t g_bytes[kSize];

enum class RelocKind { Data, Code };

inline void* host(std::uint32_t va) { return g_bytes + (va - kBase); }

template <typename T>
T& at(std::uint32_t va) {
    return *reinterpret_cast<T*>(g_bytes + (va - kBase));
}

std::uint32_t va(const void* hostAddress);
bool contains(const void* hostAddress);

// Host address of a function referenced from the data image, or the trap stub when the target has
// not been implemented yet.
void* codeTarget(std::uint32_t va);
[[noreturn]] void unimplementedCode(std::uint32_t va);

}  // namespace swd3::image
