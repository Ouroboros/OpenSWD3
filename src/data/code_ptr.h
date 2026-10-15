#pragma once

#include <cstdint>

namespace swd3 {

// Code pointers stored in the data image are 32-bit values (the Linux and Windows targets are both 32-bit).
using code_ptr = std::uint32_t;

template <typename F>
F* codeAs(code_ptr value) {
    return reinterpret_cast<F*>(static_cast<std::uintptr_t>(value));
}

template <typename F>
code_ptr codeOf(F* function) {
    return static_cast<code_ptr>(reinterpret_cast<std::uintptr_t>(function));
}

}  // namespace swd3
