#pragma once

#include <memory>
#include <vector>

#include "core/types.h"

namespace swd3
{

// An action script: a stream of 16-bit words, two-letter opcodes stored as ASCII (low byte first), most followed by
// one operand word.
using ActScript = std::shared_ptr<const std::vector<u8>>;

// Opcodes of the action scripts.
enum class ActOpcode : u16
{
    EndFrame = 'D' | 'E' << 8,  // ends the steps of this frame
    Loop = 'V' | 'O' << 8,      // back to the start (stays while held)
    Stop = '2' | 'O' << 8,      // stays here; waiting once no delay is left
    View = 'V' | 'W' << 8,
    Position = 'Y' | 'X' << 8,  // x, y
    Size = 'H' | 'W' << 8,      // width, height
    Counter = 'N' | 'T' << 8,
    Wait = 'W' | 'T' << 8,
    Invisible = 'I' | 'V' << 8,
    Mask = 'M' | 'S' << 8,
    Frame = 'F' | 'R' << 8,
    Rotation = 'O' | 'R' << 8,
    Delay = 'D' | 'S' << 8,
    EffectQ = 'E' | 'Q' << 8,
    OffsetX = 'X' | 'O' << 8,
    OffsetY = 'Y' | 'O' << 8,
    Picture = 'A' | 'P' << 8,  // picture number, counts the rounds of the counter
    Action = 'A' | 'O' << 8,
    Signal = 'S' | 'G' << 8,
    DrawLevel = 'D' | 'L' << 8,
    Visible = 'O' | 'N' << 8,
    Lights = 'L' | 'F' << 8,  // seven values
    Green = 'G' | 'C' << 8,
    ClearColors = 'L' | 'C' << 8,
    Red = 'R' | 'C' << 8,
    Blue = 'B' | 'C' << 8,
    EffectA = 'E' | 'A' << 8,
    Half = 'H' | 'A' << 8,
    MaskBlend = 'M' | 'A' << 8,
    NoBlend = 'N' | 'A' << 8,
    Target = 'T' | 'A' << 8,
    AnchorX = 'X' | 'A' << 8,
    AnchorY = 'Y' | 'A' << 8,
};

// Flags of ActState::flags.
namespace act_flag
{

constexpr u32 kInvisible = 0x1;  // 'IV' sets, 'ON' clears
constexpr u32 kKeptOnRestart = 0x80000003u;
constexpr u32 kKeptOnTurn = 0x80000000u;
constexpr u32 kMaskBlend = 0x4;   // 'MA'
constexpr u32 kHalf = 0x8;        // 'HA'
constexpr u32 kDrawLevel = 0x10;  // 'DL'
constexpr u32 kSignal = 0x14;     // 'SG'
constexpr u32 kNoBlend = 0x2C;    // 'NA'

}  // namespace act_flag

// Playback of an action script (0x98 bytes in the original; also the tail of a role at +0x40). Field offsets of the
// original are given for reference.
struct ActState
{
    i32 id = 0;              // +0x00 act id (id / 3000: archive, id % 3000: entry)
    i32 lastId = 0;          // +0x04
    i32 action = 0;          // +0x08 first sub entry of the action
    i32 lastAction = 0;      // +0x0C
    i32 x = 0;               // +0x10 'YX'
    i32 y = 0;               // +0x14
    u32 flags = 0;           // +0x18 act_flag
    i32 unknown1C = 0;       // +0x1C
    i32 savedAction = 0;     // +0x20 the action before the role turned to face someone
    i32 effectA = 0;         // +0x24 'EA'
    i32 effectQ = 0;         // +0x28 'EQ'
    i32 width = 0;           // +0x2C 'HW'
    i32 height = 0;          // +0x30
    i32 direction = 0;       // +0x34 added to the action for the sub entry
    i32 lastDirection = 0;   // +0x38
    i32 savedDirection = 0;  // +0x3C the direction before it turned
    u16 counter = 0;         // +0x40 'NT': low byte the round limit, high byte the round
    u16 position = 0;        // +0x42 script position in words
    u16 delay = 0;           // +0x44 frames left on the current frame
    u16 frameDelay = 0;      // +0x46 'DS'
    i16 speed = 0;           // +0x48 bit 15: a fixed delay in the low bits
    u16 frame = 0;           // +0x4A 'FR'
    u16 picture = 0;         // +0x4C 'AP'
    u16 rotation = 0;        // +0x4E 'OR'
    u16 actionCode = 0;      // +0x50 'AO', cleared before every step
    u16 unknown52 = 0;       // +0x52
    ActScript script;        // +0x54
    u16 view = 0;            // +0x58 'VW'
    u16 target = 0;          // +0x5A 'TA'
    u16 unknown5C = 0;       // +0x5C
    u16 offsetX = 0;         // +0x5E 'XO'
    u16 offsetY = 0;         // +0x60 'YO'
    u16 drawLevel = 0;       // +0x62 'DL'
    u16 red = 0;             // +0x64 'RC'
    u16 green = 0;           // +0x66 'GC'
    u16 blue = 0;            // +0x68 'BC'
    u16 unknown6A[3] = {};   // +0x6A
    u16 red2 = 0;            // +0x70 'RC' second value, cleared by 'LC'
    u16 green2 = 0;          // +0x72
    u16 blue2 = 0;           // +0x74
    u16 anchorX = 0;         // +0x76 'XA'
    u16 anchorY = 0;         // +0x78 'YA'
    u16 lights[7] = {};      // +0x7A 'LF'
    u8 wait = 0;             // +0x88 'WT'
    u8 unknown89 = 0;        // +0x89
    u8 signal = 0;           // +0x8A 'SG'
    u8 unknown8B = 0;        // +0x8B
    i32 waiting = 0;         // +0x8C '2O' reached with no delay
    i32 hold = 0;            // +0x90 1: stay on the current step
    i32 mask = 0;            // +0x94 'MS'
};

// Back to the start of the script: the step position, delays, speed and hold cleared.
void ResetPlayback(ActState& state);
void FillUnused(ActState& state);

}  // namespace swd3
