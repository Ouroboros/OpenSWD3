#pragma once

#include <array>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/types.h"
#include "crt/msvc.h"
#include "game/game_state.h"
#include "res/act_state.h"
#include "role/role.h"

namespace swd3
{

// A talk window (0x4C bytes, the list at 0x004ACF48): its frame and place (growing out of the speaker or a point
// over four frames), the text with the page shown and the typing position, the colours, the timeout and the name.
struct TalkWindow
{
    static constexpr std::size_t kClosed = static_cast<std::size_t>(-1);
    static constexpr u16 kNoRole = 0xFFFD;

    const ActState* frameAct = nullptr;  // +0x00 its frame picks the window frame
    const ActState* nameAct = nullptr;   // +0x04 its frame picks the frame of the name plate
    u32 flags = 0;                       // +0x08
    u32 timeout = 0;                     // +0x0C tenths of a second (0xFFFF: none)
    u32 shownAt = 0;                     // +0x10 the time the page was complete
    u16 openStep = 0;                    // +0x14 0..4
    u16 role = 0;                        // +0x16 the speaking role (kNoRole: the point fromX, fromY)
    u16 counter = 0;                     // +0x18
    u16 x = 0;                           // +0x1A screen pixels
    u16 y = 0;                           // +0x1C
    u16 fromX = 0;                       // +0x1E
    u16 fromY = 0;                       // +0x20
    u16 width = 0;                       // +0x22
    u16 height = 0;                      // +0x24
    u16 unk26 = 0;                       // +0x26
    u16 speed = 0;                       // +0x28 frames per character
    u16 delay = 0;                       // +0x2A frames to the next character (bit 15: typing)
    u16 color = 0;                       // +0x2C an index into the text colours
    u16 shade = 0;                       // +0x2E an index into the shadow colours
    u16 pageColor = 0;                   // +0x30 the colours at the start of the next page
    u16 pageShade = 0;                   // +0x32
    u16 style = 0;                       // +0x34 low byte the text style, high byte the style of the next page
    u16 unk36 = 0;                       // +0x36
    std::string text;                    // +0x38 Big5 with % codes, ended by "%Q"
    std::size_t page = 0;                // +0x3C the start of the page shown (kClosed: the window is done)
    std::size_t typed = 0;               // +0x40 the typing position
    std::string name;                    // +0x44 the name plate (empty: none)
    std::unique_ptr<TalkWindow> next;    // +0x48
};

// An area of the map with its encounters (0x18 bytes; the list starts with a node of its own at 0x004A9A18).
struct MapTrigger
{
    i32 left = 0;                      // +0x00 tiles
    i32 top = 0;                       // +0x04
    i32 right = 0;                     // +0x08
    i32 bottom = 0;                    // +0x0C
    u32 table = 0;                     // +0x10 an offset into maps.dat
    std::unique_ptr<MapTrigger> next;  // +0x14
};

// A bubble rising from a role's place (0x10 bytes): position in 1/16 pixels across, speed and frames left.
struct RoleMarkNode
{
    std::unique_ptr<RoleMarkNode> next;  // +0x00
    i16 x = 0;                           // +0x04 1/16 pixels
    i16 y = 0;                           // +0x06
    i16 stepX = 0;                       // +0x08
    i16 stepY = 0;                       // +0x0A
    i16 life = 0;                        // +0x0C
};

// The bubbles of a role (0x10 bytes, four at 0x004CACE0).
struct RoleMark
{
    static constexpr u16 kEmitting = 1;  // in `flags`

    std::unique_ptr<RoleMarkNode> nodes;  // +0x00
    u16 x = 0;                            // +0x04 the role's place
    u16 y = 0;                            // +0x06
    i16 unk08 = 0;                        // +0x08 bubbles start while below 0x18
    u16 flags = 0;                        // +0x0A
    u16 role = 0;                         // +0x0C
    u16 unk0E = 0;                        // +0x0E
};

// A falling or rising line of the screen (0x18 bytes, the list at 0x004BAB9C): `count` columns from (x, y) whose
// offsets and lengths the mode moves (high byte of `mode`: 0x80 down, 0x40 up, 0x20 open, 0x10 close, 0x08 still).
struct ScriptEffect
{
    i16 x = 0;                           // +0x00
    i16 y = 0;                           // +0x02
    i16 limit = 0;                       // +0x04
    i16 count = 0;                       // +0x06
    u16 mode = 0;                        // +0x08
    i16 color = 0;                       // +0x0A an index into the text colours
    std::vector<i16> offsets;            // +0x0C per column
    std::vector<i16> lengths;            // +0x10 per column
    std::unique_ptr<ScriptEffect> next;  // +0x14
};

// An act moving over the map (0xB0 bytes; the lists at 0x004AD3E8 and 0x004BA6E0). Sprites move by (stepX, stepY)
// until within 32 pixels of the target; the role heads of the second list slide along x towards targetX.
struct ScriptSprite
{
    ActState act;                        // +0x00
    i16 headX = 0;                       // +0x98
    i16 headSpeed = 0;                   // +0x9A
    i16 targetX = 0;                     // +0x9C
    i16 targetY = 0;                     // +0x9E the head's y in the second list
    float stepX = 0;                     // +0xA0
    float stepY = 0;                     // +0xA4
    float x = 0;                         // +0xA8 map pixels
    float y = 0;                         // +0xAC
    std::unique_ptr<ScriptSprite> next;  // +0xB0
};

// An act played at a place of the screen until its script ends (0xA4 bytes; the lists at 0x004B7C70 and
// 0x004B8968).
struct ScriptAnimation
{
    i16 x = 0;                              // +0x00 screen pixels
    i16 y = 0;                              // +0x02
    i32 unk04 = 0;                          // +0x04
    ActState act;                           // +0x08
    std::unique_ptr<ScriptAnimation> next;  // +0xA0
};

// A ripple over the screen (0x2C bytes, the list at 0x004AC9B8): a grid of heights in two pages over a copy of the
// screen, refracting it until the waves die out.
struct ScriptObject
{
    i32 pitch = 0;                       // +0x00 pixels per line of the screen copy
    i32 lines = 0;                       // +0x04
    i32 left = 0;                        // +0x08
    i32 top = 0;                         // +0x0C
    i32 gridWidth = 0;                   // +0x10
    i32 gridHeight = 0;                  // +0x14
    i32 damping = 0;                     // +0x18 a shift
    i32 page = 0;                        // +0x1C
    std::vector<i16> heights;            // +0x20 two pages of gridWidth x gridHeight
    std::vector<u16> screen;             // +0x24
    std::unique_ptr<ScriptObject> next;  // +0x28

    ScriptObject() = default;
    ScriptObject(int pitch, int lines, int left, int top, int gridWidth, int gridHeight);
    ~ScriptObject();
    void Splash(int x, int y, int radius, int depth, crt::Random& random);

    void CopyScreen(std::span<const u16> pixels);
    void Refract(std::span<u16> pixels) const;
    bool Step();
};

// A particle of the weather or the leaves: its place on the map and its speed.
struct Particle
{
    i32 x = 0;
    i32 y = 0;
    i32 stepX = 0;
    i32 stepY = 0;
};

// The shade of a weather particle (0x10 bytes, four at 0x004ACDC0).
struct WeatherShade
{
    i32 tint = 0;        // +0x00
    i32 unk04 = 0;       // +0x04
    i32 tintTarget = 0;  // +0x08
    i32 unk0C = 0;       // +0x0C
};

// The motion of a weather particle (0x10 bytes, four at 0x004ACE00).
struct WeatherMotion
{
    i32 counter = 0;      // +0x00 frames since the last move
    i32 action = 0;       // +0x04 the action of act 0x232B
    i32 speedTarget = 0;  // +0x08
    i32 speed = 0;        // +0x0C frames between moves
};

// A streak drawn into the screen (0x10 bytes): the lightning bolts and the fireflies.
struct Streak
{
    i16 x = 0;       // +0x00 1/16 pixels
    i16 y = 0;       // +0x02 lines
    i16 slope = 0;   // +0x04 1/16 pixels per line
    i16 speed = 0;   // +0x06 lines per frame
    i16 length = 0;  // +0x08 lines
    i16 life = 0;    // +0x0A
    i16 phase = 0;   // +0x0C
    u16 active = 0;  // +0x0E bit 0
};

// The colour fade of the screen: the steps added each frame, the levels reached and the steps restored once the
// frames run out.
struct ColorFade
{
    i32 frames = 0;        // 0x004A9934
    float stepRed = 0;     // 0x004A9A00
    float stepGreen = 0;   // 0x004A94B8
    float stepBlue = 0;    // 0x004C97F0
    float red = 0;         // 0x004A93E0
    float green = 0;       // 0x004B6C70
    float blue = 0;        // 0x004C9A00
    float startRed = 0;    // 0x004A93DC
    float startGreen = 0;  // 0x004B6C74
    float startBlue = 0;   // 0x004C9A04
};

// The state of the story scenes that a new game starts from (named by the original's addresses until their readers
// are restored): the pointer roles, the animations of the screen elements and tables of maps.dat.
struct Scene
{
    static constexpr int kPointerAct = 0x2329;
    static constexpr u32 kPointerFlags = 0x9000;

    i32 unk4A93D4 = 0;  // the event run when the first timer runs out
    i32 unk4BAB78 = 0;
    i32 unk4BAB7C = 0;
    i32 unk4C97E8 = 0;
    i32 unk4C97EC = 0;
    i32 unk4A9918 = 0;
    i32 unk4A9930 = 0;
    i32 unk4A9938 = 0;
    ActState unk4A9A30;
    std::array<ActState, 4> unk4A9120;
    ActState unk4A93E8;
    u32 unk4A94B0 = 0;
    i32 unk4AB788 = 0;
    ActState unk4ACAF8;
    i32 viewWidth = 0;   // 0x004ACBC0
    i32 viewHeight = 0;  // 0x004ACBC4
    i32 unk4ACBC8 = 0;
    i32 unk4ACBCC = 0;
    Role pointerRole;  // 0x004ACCD8 the mouse pointer (act 0x2329, action 0)
    i32 unk4ACDB8 = 0;
    i32 unk4ACDBC = 0;
    u32 unk4ACE40 = 0;                 // an offset into maps.dat (its table 1)
    std::array<i16, 0x40> shake = {};  // 0x004ACE70 camera offsets of the quake in pairs, ended by 0xCFCF
    i32 shakeIndex = 0;                // 0x004B7EB0
    i32 shakeFrames = 0;               // 0x004A94A4
    i32 shakeHold = 0;                 // 0x004AD0D0
    i32 unk4A93E4 = 0;                 // the camera before the quake
    i32 unk4A992C = 0;
    i32 unk4A99F0 = 0;                        // the pan still to go across
    i32 unk4A98C0 = 0;                        // and down
    i32 unk4A9480 = 0;                        // the step of the pan across
    i32 unk4A93D8 = 0;                        // and down
    std::unique_ptr<TalkWindow> talkWindows;  // 0x004ACF48
    std::array<ActState, 4> unk4AD0D8;
    ActState unk4AD3F0;
    std::optional<std::string> unk4B74F4;  // a name of up to 0x33 bytes (flag 0x4E)
    std::optional<std::string> unk4B751C;  // (flag 0x4D)
    std::array<i32, 6> unk4B7C74 = {};
    std::array<ActState, 8> unk4B9F68;
    ActState unk4C83A8;
    i32 unk4C8BE0 = 0;
    u16 unk4C8BD4 = 0;                     // the bytes of maps.dat's table 1
    Role pointerButtonRole;                // 0x004C96C8 (action 0xC)
    std::array<Particle, 4> weather = {};  // 0x004C97A0 the weather particles (filled with 0x0F bytes)
    u32 unk4C97E4 = 0;                     // an offset into maps.dat (its table 0x18)
    std::array<ActState, 4> unk4CAA78;
    i32 unk4CAD20 = 0;
    Role pointerPageRole;        // 0x004CADA0 (action 0xE)
    std::vector<u16> unk4CAE78;  // records of 8 words from maps.dat's table 0x12, their count first
    i32 unk4CAE88 = 0;

    // Set by LoadCity (0x0040C130).
    i32 unk49E0BC = 0;
    i32 unk49E0C0 = 0;
    i32 unk4A94A8 = 0;
    i32 unk4AB97C = 0;
    i32 unk4AB994 = 0;
    i32 unk4ACB98 = 0;
    i32 unk4ACDB4 = 0;
    std::array<WeatherShade, 4> weatherShades = {};   // 0x004ACDC0
    std::array<WeatherMotion, 4> weatherMotion = {};  // 0x004ACE00
    i32 unk4ACE48 = 0;
    i32 unk4ACE4C = 0;
    i32 unk4ACE50 = 0;
    i32 unk4ACE54 = 0;
    i32 unk4ACE58 = 0;
    i32 unk4ACE5C = 0;
    u16 unk4ACEF0 = 0;
    i16 lightningCount = 0;          // 0x004AAEC8 bolts wanted (up to 8)
    i16 firefliesShown = 0;          // 0x004B737C
    std::array<Streak, 0x40> bolts;  // 0x004B7520 (a new map clears the first 0x30)
    i32 unk4B7944 = 0;
    std::array<i32, 0x20> trailY = {};          // 0x004B7998 the places of the leader, the latest first
    std::array<i32, 0x20> trailDirection = {};  // 0x004C8C00
    std::array<i32, 0x20> trailX = {};          // 0x004B7A18
    i32 unk4B7A98 = 0;
    std::array<i32, 8> unk4B7C8C = {};  // the roles following the leader
    i32 unk4B7EB4 = 0;
    std::array<Particle, 4> leaves = {};  // 0x004B86F8 (x 0x7FFFFFFF: to be placed)
    i32 unk4B873C = 0;
    i32 unk4B88C0 = 0;
    i16 lightningShown = 0;  // 0x004B88BC
    i32 unk4B896C = 0;
    std::array<MoveRecord, 8> followerMoves;  // 0x004B8E18
    i32 unk4B9EF8 = 0;
    i32 unk4B9EFC = 0;
    std::array<i16, 0x100> soundRise = {};      // 0x004BA430 by role: how far below the controlled role
    i16 fireflyCount = 0;                       // 0x004BAB80 (up to 1)
    std::array<i16, 0x100> soundDistance = {};  // 0x004C97F8 by role
    std::array<Streak, 0x60> fireflies;         // 0x004C8C80
    std::array<i16, 0x40> heatLengths = {};     // 0x004AB8FC bytes copied per line
    std::array<i16, 0x40> heatCounts = {};      // 0x004AB2F8 lines
    std::array<i32, 0x30> heatOffsets = {};     // 0x004AC9BC pixels
    u16 heatFrame = 0;                          // 0x004CAE90
    ColorFade fade;
    i32 unk4C9A08 = 0;  // shown as dbg1
    i32 unk4ACE64 = 0;  // shown as dbg2
    i32 unk4BABA0 = 0;  // followers in unk4B7C8C
    i32 unk4C8BD8 = 0;
    u16 unk4C97F6 = 0;
    i32 unk4C9A18 = 0;
    i32 unk4C9A20 = 0;
    i32 unk4CAE80 = 0;
    i32 unk4CC2DC = 0;
    i32 unk4A9920 = 0;                             // 0x004A9920 bit 15 kept across maps
    MapTrigger triggers;                           // 0x004A9A18 (the list from +0x14)
    std::array<RoleMark, 4> marks;                 // 0x004CACE0
    std::unique_ptr<ScriptEffect> effects;         // 0x004BAB9C
    std::unique_ptr<ScriptSprite> spritesA;        // 0x004AD3E8
    std::unique_ptr<ScriptSprite> spritesB;        // 0x004BA6E0
    std::unique_ptr<ScriptAnimation> animationsA;  // 0x004B7C70
    std::unique_ptr<ScriptAnimation> animationsB;  // 0x004B8968
    std::unique_ptr<ScriptObject> objects;         // 0x004AC9B8

    void FreeEffects();
    void FreeSpritesA();
    void FreeSpritesB();
    void FreeNodes();
    void FreeAnimations();
    void FreeMarks();
    void ResetLeaves();
    void FreeObjects();
};

}  // namespace swd3
