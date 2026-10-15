#include "data/image.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace swd3::image {

alignas(16) std::uint8_t g_bytes[kSize] = {
#include "data/image_bytes.inc"
};

namespace {

struct Reloc {
    std::uint32_t va;
    std::uint32_t count;
    RelocKind kind;
};

const Reloc kRelocs[] = {
#include "data/relocs.inc"
    {0, 0, RelocKind::Data},
};


}  // namespace


std::uint32_t va(const void* hostAddress) {
    auto offset = static_cast<std::uint32_t>(static_cast<const std::uint8_t*>(hostAddress) - g_bytes);
    return kBase + offset;
}

bool contains(const void* hostAddress) {
    auto* p = static_cast<const std::uint8_t*>(hostAddress);
    return p >= g_bytes && p < g_bytes + kSize;
}

[[noreturn]] void unimplementedCode(std::uint32_t target) {
    std::fprintf(stderr, "swd3: call through data image to unimplemented code 0x%08X\n", target);
    std::abort();
}

namespace {

void relocate() {
    for (const Reloc& r : kRelocs) {
        for (std::uint32_t i = 0; i < r.count; ++i) {
            std::uint8_t* slot = g_bytes + (r.va - kBase) + 4 * i;
            std::uint32_t value;
            std::memcpy(&value, slot, 4);
            void* translated = nullptr;
            if (r.kind == RelocKind::Data && value >= kBase && value < kEnd) {
                translated = host(value);
            } else if (r.kind == RelocKind::Code && value >= kCodeBase && value < kCodeEnd) {
                translated = codeTarget(value);
            } else {
                continue;
            }
            auto hostValue = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(translated));
            std::memcpy(slot, &hostValue, 4);
        }
    }
}

struct Relocator {
    Relocator() { relocate(); }
};

#if defined(_MSC_VER)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
Relocator g_relocator;
#else
__attribute__((init_priority(101))) Relocator g_relocator;
#endif

}  // namespace

}  // namespace swd3::image
