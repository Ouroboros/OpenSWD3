#include "data/image.h"

namespace swd3::image {
namespace {

struct CodeTarget {
    std::uint32_t va;
    void* host;
};

// Functions whose original address is stored in the data image (function pointer tables, callbacks).
const CodeTarget kTargets[] = {
    {0, nullptr},
};

[[noreturn]] void trap() { unimplementedCode(0); }

}  // namespace

void* codeTarget(std::uint32_t target) {
    for (const CodeTarget& t : kTargets) {
        if (t.va == target && t.host) {
            return t.host;
        }
    }
    return reinterpret_cast<void*>(&trap);
}

}  // namespace swd3::image
