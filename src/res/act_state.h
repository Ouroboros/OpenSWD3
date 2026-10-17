#pragma once

#include <memory>
#include <vector>

#include "core/types.h"
#include "core/words.h"

namespace swd3
{

// An action script: a stream of 16-bit words, two-letter opcodes stored as ASCII (low byte first), most followed by
// one operand word.
using ActScript = std::shared_ptr<const std::vector<u8>>;

// A two-letter opcode as the script stores it, the first letter in the low byte.
constexpr u16 ScriptTag(char first, char second)
{
    return MakeBytes(static_cast<u8>(second), static_cast<u8>(first));
}

// Opcodes of the action scripts.
enum class ActOpcode : u16
{
    EndFrame = ScriptTag('D', 'E'),  // ends the steps of this frame
    Loop = ScriptTag('V', 'O'),      // back to the start (stays while held)
    Stop = ScriptTag('2', 'O'),      // stays here; waiting once no delay is left
    View = ScriptTag('V', 'W'),
    Position = ScriptTag('Y', 'X'),  // x, y
    Size = ScriptTag('H', 'W'),      // width, height
    Counter = ScriptTag('N', 'T'),
    Wait = ScriptTag('W', 'T'),
    Invisible = ScriptTag('I', 'V'),
    Mask = ScriptTag('M', 'S'),
    Frame = ScriptTag('F', 'R'),
    Rotation = ScriptTag('O', 'R'),
    Delay = ScriptTag('D', 'S'),
    EffectQ = ScriptTag('E', 'Q'),
    OffsetX = ScriptTag('X', 'O'),
    OffsetY = ScriptTag('Y', 'O'),
    Picture = ScriptTag('A', 'P'),  // picture number, counts the rounds of the counter
    Action = ScriptTag('A', 'O'),
    Signal = ScriptTag('S', 'G'),
    DrawLevel = ScriptTag('D', 'L'),
    Visible = ScriptTag('O', 'N'),
    Lights = ScriptTag('L', 'F'),  // seven values
    Green = ScriptTag('G', 'C'),
    ClearColors = ScriptTag('L', 'C'),
    Red = ScriptTag('R', 'C'),
    Blue = ScriptTag('B', 'C'),
    EffectA = ScriptTag('E', 'A'),
    Half = ScriptTag('H', 'A'),
    MaskBlend = ScriptTag('M', 'A'),
    NoBlend = ScriptTag('N', 'A'),
    Target = ScriptTag('T', 'A'),
    AnchorX = ScriptTag('X', 'A'),
    AnchorY = ScriptTag('Y', 'A'),
};

// Playback of an action script (0x98 bytes in the original; also the tail of a role at +0x40). Field offsets of the
// original are given for reference.
struct ActState
{
    // `counter`: the low byte is the round limit, the high byte the round.
    static constexpr int Lights = 7;  // the values of opcode LF
    static constexpr u16 RoundsMask = 0xFF;
    static constexpr int RoundShift = 8;
    static constexpr i16 FixedDelay = static_cast<i16>(0x8000);  // in `speed`: the low bits are a fixed delay
    static constexpr i32 NotSaved = -1;                          // in `savedAction` and `savedDirection`
    static constexpr i32 NoLast = -1;  // in `lastAction` and `lastDirection`: the act runs from the start

    // The act directions (RolePath::Facing maps the steps onto them).
    enum Direction : u16
    {
        Up = 0,
        Down = 1,
        Left = 2,
        Right = 3,
        UpLeft = 4,
        DownRight = 5,
        DownLeft = 6,
        UpRight = 7,
    };

    // Flags of ActState::flags.
    enum Flag : u32
    {
        Invisible = 0x1,  // 'IV' sets, 'ON' clears
        KeptOnRestart = 0x80000003u,
        KeptOnTurn = 0x80000000u,
        MaskBlend = 0x4,   // 'MA'
        Half = 0x8,        // 'HA'
        DrawLevel = 0x10,  // 'DL'
        Signal = 0x14,     // 'SG'
        NoBlend = 0x2C,    // 'NA'
    };

    i32 id = 0;               // +0x00 act id (id / 3000: archive, id % 3000: entry)
    i32 lastId = 0;           // +0x04
    i32 action = 0;           // +0x08 first sub entry of the action
    i32 lastAction = 0;       // +0x0C
    i32 x = 0;                // +0x10 'YX'
    i32 y = 0;                // +0x14
    u32 flags = 0;            // +0x18 ActState::Flag
    i32 unknown1C = 0;        // +0x1C
    i32 savedAction = 0;      // +0x20 the action before the role turned to face someone
    i32 effectA = 0;          // +0x24 'EA'
    i32 effectQ = 0;          // +0x28 'EQ'
    i32 width = 0;            // +0x2C 'HW'
    i32 height = 0;           // +0x30
    i32 direction = 0;        // +0x34 added to the action for the sub entry
    i32 lastDirection = 0;    // +0x38
    i32 savedDirection = 0;   // +0x3C the direction before it turned
    u16 counter = 0;          // +0x40 'NT': low byte the round limit, high byte the round
    u16 position = 0;         // +0x42 script position in words
    u16 delay = 0;            // +0x44 frames left on the current frame
    u16 frameDelay = 0;       // +0x46 'DS'
    i16 speed = 0;            // +0x48 FixedDelay: a fixed delay in the low bits
    u16 frame = 0;            // +0x4A 'FR'
    u16 picture = 0;          // +0x4C 'AP'
    u16 rotation = 0;         // +0x4E 'OR'
    u16 actionCode = 0;       // +0x50 'AO', cleared before every step
    u16 unknown52 = 0;        // +0x52
    ActScript script;         // +0x54
    u16 view = 0;             // +0x58 'VW'
    u16 target = 0;           // +0x5A 'TA'
    u16 unknown5C = 0;        // +0x5C
    u16 offsetX = 0;          // +0x5E 'XO'
    u16 offsetY = 0;          // +0x60 'YO'
    u16 drawLevel = 0;        // +0x62 'DL'
    u16 red = 0;              // +0x64 'RC'
    u16 green = 0;            // +0x66 'GC'
    u16 blue = 0;             // +0x68 'BC'
    u16 unknown6A[3] = {};    // +0x6A
    u16 red2 = 0;             // +0x70 'RC' second value, cleared by 'LC'
    u16 green2 = 0;           // +0x72
    u16 blue2 = 0;            // +0x74
    u16 anchorX = 0;          // +0x76 'XA'
    u16 anchorY = 0;          // +0x78 'YA'
    u16 lights[Lights] = {};  // +0x7A 'LF'
    u8 wait = 0;              // +0x88 'WT'
    u8 unknown89 = 0;         // +0x89
    u8 signal = 0;            // +0x8A 'SG'
    u8 unknown8B = 0;         // +0x8B
    i32 waiting = 0;          // +0x8C '2O' reached with no delay
    i32 hold = 0;             // +0x90 1: stay on the current step
    i32 mask = 0;             // +0x94 'MS'
};

// Back to the start of the script: the step position, delays, speed and hold cleared.
void ResetPlayback(ActState& state);
void FillUnused(ActState& state);

}  // namespace swd3
