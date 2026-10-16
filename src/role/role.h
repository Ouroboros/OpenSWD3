#pragma once

#include <cstdint>

#include "res/act_anim.h"

namespace swd3 {

// Map character (0xD8 bytes, G.roles[256] at 0x004BABA8): position and state followed by its action playback.
struct Role {
    std::int32_t unknown00;       // +0x00
    std::int32_t x;               // +0x04 pixels (16 per tile)
    std::int32_t y;               // +0x08
    std::int32_t layer;           // +0x0C
    std::uint32_t flags;          // +0x10
    std::int32_t unknown14;       // +0x14
    std::int32_t unknown18;       // +0x18
    std::uint16_t unknown1C;      // +0x1C
    std::uint16_t event;          // +0x1E event run when the role is triggered
    std::uint16_t unknown20;      // +0x20
    std::uint16_t unknown22;      // +0x22
    std::uint16_t id;             // +0x24 role id (0xFFFF: unused)
    std::uint16_t unknown26;      // +0x26
    std::int32_t unknown28[6];    // +0x28
    ActState act;                 // +0x40
};

static_assert(sizeof(Role) == 0xD8);

}  // namespace swd3
