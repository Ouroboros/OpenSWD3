#pragma once

#include <cstdint>
#include <new>
#include <utility>

// Thin layer over the host C/C++ runtime for the MSVC CRT functions statically linked into swd32.exe.
// Only behaviour the game can observe differently from the host runtime is reproduced here.
namespace swd3::crt {

// MSVC debug heap: fresh blocks are filled with 0xCD, freed blocks with 0xDD.
constexpr std::uint8_t kCleanLandFill = 0xCD;
constexpr std::uint8_t kDeadLandFill = 0xDD;

void* malloc(std::uint32_t size);
void free(void* block);
void* operator_new(std::uint32_t size);
void operator_delete(void* block);

void srand(std::uint32_t seed);
int rand();
std::uint32_t randSeed();

std::int64_t ftol(double value);
double difftime(std::int32_t end, std::int32_t start);

// Registers a function run at exit, last registered first (host atexit).
int atexit(void (*function)());

// Calls the non-null function pointers of [begin, end) (code addresses of the data image, relocated).
void initterm(const std::uint32_t* begin, const std::uint32_t* end);
// Runs the C++ dynamic initializers of the game (__xc table).
void cinit();

template <typename T, typename... Args>
T* New(Args&&... args) {
    void* memory = operator_new(static_cast<std::uint32_t>(sizeof(T)));
    return memory ? new (memory) T(std::forward<Args>(args)...) : nullptr;
}

template <typename T>
void Delete(T* object) {
    if (object) {
        object->~T();
        operator_delete(object);
    }
}

}  // namespace swd3::crt
