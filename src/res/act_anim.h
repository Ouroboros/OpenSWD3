#pragma once

#include <cstdint>

namespace swd3 {

// Playback state of an ACT action script (0x98 bytes; also the tail of a Role at +0x40). Scripts are WORD
// streams of two-letter opcodes stored as big-endian ASCII (0x4544 'DE' ends a frame).
struct ActState {
    std::int32_t id;               // +0x00 act id
    std::int32_t lastId;           // +0x04
    std::int32_t action;           // +0x08 sub entry base
    std::int32_t lastAction;       // +0x0C
    std::int32_t x;                // +0x10 'YX'
    std::int32_t y;                // +0x14
    std::uint32_t flags;           // +0x18 bit 31 kept on every reset, bits 0-1 kept on action/id resets
    std::int32_t unknown1C;        // +0x1C
    std::int32_t unknown20;        // +0x20
    std::int32_t effectA;          // +0x24 'EA'
    std::int32_t effectQ;          // +0x28 'EQ'
    std::int32_t width;            // +0x2C 'HW'
    std::int32_t height;           // +0x30
    std::int32_t direction;        // +0x34 added to `action`
    std::int32_t lastDirection;    // +0x38
    std::int32_t unknown3C;        // +0x3C
    std::uint16_t counter;         // +0x40 'NT', 'AP' low byte limit / high byte counter
    std::uint16_t pc;              // +0x42 script position in WORDs
    std::uint16_t delay;           // +0x44 frames left on the current frame
    std::uint16_t frameDelay;      // +0x46 'SD'
    std::int16_t speed;            // +0x48 bit 15: fixed delay in the low bits
    std::uint16_t fr;              // +0x4A 'FR'
    std::uint16_t ap;              // +0x4C 'AP'
    std::uint16_t ro;              // +0x4E 'RO'
    std::uint16_t ao;              // +0x50 'AO', cleared before every step
    std::uint16_t unknown52;       // +0x52
    std::uint8_t* script;          // +0x54
    std::uint16_t vw;              // +0x58 'VW'
    std::uint16_t ta;              // +0x5A 'TA'
    std::uint16_t unknown5C;       // +0x5C
    std::uint16_t xo;              // +0x5E 'XO'
    std::uint16_t yo;              // +0x60 'YO'
    std::uint16_t dl;              // +0x62 'DL'
    std::uint16_t rc;              // +0x64 'RC'
    std::uint16_t gc;              // +0x66 'GC'
    std::uint16_t bc;              // +0x68 'BC'
    std::uint16_t unknown6A[3];    // +0x6A
    std::uint16_t rc2;             // +0x70 'RC' second value / 'LC'
    std::uint16_t gc2;             // +0x72 'GC' second value / 'LC'
    std::uint16_t bc2;             // +0x74 'BC' second value / 'LC'
    std::uint16_t xa;              // +0x76 'XA'
    std::uint16_t ya;              // +0x78 'YA'
    std::uint16_t lf[7];           // +0x7A 'LF'
    std::uint8_t wt;               // +0x88 'WT'
    std::uint8_t unknown89;        // +0x89
    std::uint8_t sg;               // +0x8A 'SG'
    std::uint8_t unknown8B;        // +0x8B
    std::int32_t waiting;          // +0x8C '2O' reached with no delay
    std::int32_t hold;             // +0x90 1: stay on the current frame
    std::int32_t ms;               // +0x94 'MS'
};

static_assert(sizeof(ActState) == 0x98);

int ActRun(ActState* state);

}  // namespace swd3
