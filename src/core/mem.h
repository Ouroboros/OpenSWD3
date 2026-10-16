#pragma once

#include <cstdint>
#include <cstring>

// Unaligned little-endian access to raw game records at byte offsets.
namespace swd3::mem {

inline std::uint8_t* at(void* base, std::intptr_t offset) { return static_cast<std::uint8_t*>(base) + offset; }

inline std::uint16_t u16(const void* base, std::intptr_t offset = 0) {
    std::uint16_t value;
    std::memcpy(&value, static_cast<const std::uint8_t*>(base) + offset, 2);
    return value;
}

inline std::int16_t s16(const void* base, std::intptr_t offset = 0) { return static_cast<std::int16_t>(u16(base, offset)); }

inline std::uint32_t u32(const void* base, std::intptr_t offset = 0) {
    std::uint32_t value;
    std::memcpy(&value, static_cast<const std::uint8_t*>(base) + offset, 4);
    return value;
}

inline std::int32_t s32(const void* base, std::intptr_t offset = 0) { return static_cast<std::int32_t>(u32(base, offset)); }

inline void put16(void* base, std::intptr_t offset, std::uint32_t value) {
    const auto v = static_cast<std::uint16_t>(value);
    std::memcpy(static_cast<std::uint8_t*>(base) + offset, &v, 2);
}

inline void put32(void* base, std::intptr_t offset, std::uint32_t value) {
    std::memcpy(static_cast<std::uint8_t*>(base) + offset, &value, 4);
}

// Typed reference to a field of a raw record (the build disables strict aliasing; x86 tolerates the unaligned
// fields of the original layouts).
template <typename T>
T& ref(void* base, std::intptr_t offset) {
    return *reinterpret_cast<T*>(static_cast<std::uint8_t*>(base) + offset);
}

template <typename T>
T* ptr(const void* base, std::intptr_t offset = 0) {
    return reinterpret_cast<T*>(static_cast<std::uintptr_t>(u32(base, offset)));
}

template <typename T>
void putPtr(void* base, std::intptr_t offset, T* value) {
    put32(base, offset, static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value)));
}

}  // namespace swd3::mem
