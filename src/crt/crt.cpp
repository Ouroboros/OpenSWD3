#include "crt/crt.h"

#include "data/code_ptr.h"
#include "data/globals.h"

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace swd3::crt {
namespace {

// The user size is kept in a 16-byte prefix so that allocation works during static initialisation.
constexpr std::size_t kPrefix = 16;

void* allocate(std::uint32_t size) {
    auto* raw = static_cast<std::uint8_t*>(std::malloc(kPrefix + size));
    if (!raw) {
        return nullptr;
    }
    std::memcpy(raw, &size, sizeof(size));
    std::memset(raw + kPrefix, kCleanLandFill, size);
    return raw + kPrefix;
}

void release(void* block) {
    if (!block) {
        return;
    }
    std::uint8_t* raw = static_cast<std::uint8_t*>(block) - kPrefix;
    std::uint32_t size;
    std::memcpy(&size, raw, sizeof(size));
    std::memset(block, kDeadLandFill, size);
    std::free(raw);
}

// swd32.exe 0x004A833C (holdrand, initial value 1)
std::uint32_t g_holdrand = 1;

}  // namespace

// swd32.exe 0x00487C10 malloc -> _nh_malloc_dbg(size, _newmode, _NORMAL_BLOCK, NULL, 0)
void* malloc(std::uint32_t size) { return allocate(size); }

// swd32.exe 0x004885A0 free -> _free_dbg(block, _NORMAL_BLOCK)
void free(void* block) { release(block); }

// swd32.exe 0x00489E90 operator new -> _nh_malloc(size, 1)
void* operator_new(std::uint32_t size) { return allocate(size); }

// swd32.exe 0x00489D00 operator delete -> _free_dbg(block, pHead->nBlockUse)
void operator_delete(void* block) { release(block); }

// swd32.exe 0x00489B10
void srand(std::uint32_t seed) { g_holdrand = seed; }

// swd32.exe 0x00489B20
int rand() {
    g_holdrand = g_holdrand * 0x343FDu + 0x269EC3u;
    return static_cast<int>((g_holdrand >> 16) & 0x7FFFu);
}

std::uint32_t randSeed() { return g_holdrand; }

// swd32.exe 0x00489654 _ftol: fistp with round-toward-zero, 64-bit result
std::int64_t ftol(double value) {
    if (std::isnan(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0) {
        return std::numeric_limits<std::int64_t>::min();
    }
    return static_cast<std::int64_t>(value);
}

// swd32.exe 0x00489B50 difftime: (double)(int)(end - start)
double difftime(std::int32_t end, std::int32_t start) {
    return static_cast<double>(static_cast<std::int32_t>(static_cast<std::uint32_t>(end) - static_cast<std::uint32_t>(start)));
}

// swd32.exe 0x00489B70 time: seconds since 1970 (the original converts the local time with the time zone)
std::int32_t time(std::int32_t* result) {
    const auto now = static_cast<std::int32_t>(std::time(nullptr));
    if (result != nullptr) {
        *result = now;
    }
    return now;
}

// swd32.exe 0x00487BA0 atexit
int atexit(void (*function)()) { return std::atexit(function) == 0 ? 0 : -1; }

// swd32.exe 0x0048A490 _initterm
void initterm(const std::uint32_t* begin, const std::uint32_t* end) {
    for (; begin < end; ++begin) {
        if (*begin != 0) {
            codeAs<void()>(*begin)();
        }
    }
}

// swd32.exe 0x0048A330 _cinit: the floating point hook and the CRT's own __xi initializers are host runtime
// work; the C++ initializers (__xc_a 0x0049E000 .. __xc_z 0x0049E070) are run here.
void cinit() { initterm(&G.cppInitializers[0], &G.cppInitializers[0] + 28); }

}  // namespace swd3::crt
