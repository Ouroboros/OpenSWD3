#pragma once

#include <cstdint>

namespace openswd3::compat {

using u8 = std::uint8_t;
using i8 = std::int8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using i16 = std::int16_t;
using i32 = std::int32_t;

static_assert(sizeof(u8) == 1);
static_assert(sizeof(i8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(i16) == 2);
static_assert(sizeof(i32) == 4);

}  // namespace openswd3::compat
