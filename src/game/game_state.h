#pragma once

#include <array>
#include <list>
#include <vector>

#include "core/types.h"
#include "role/role.h"

namespace swd3
{

// The header of the current map (0x64 bytes at 0x004B7930); its two buffers are freed when the map is left.
struct MapHeader
{
    // The bits of a word in `blocks`.
    enum Cell : u32
    {
        RegionMask = 0xFF,        // the map region (event area) of the cell
        LeaderCell = 0x100,       // the leader stands on it
        Reflecting = 0x800,       // the roles on it are reflected
        ShadeMask = 0xF000,       // the shade of the roles on it (Role::CellShades >> ShadeShift)
        Unk800000 = 0x800000,     // under a role with Role::Unk10
        KeyedTile = 0x04000000,   // its tile is drawn with colour 0 transparent
        HiddenTile = 0x08000000,  // its tile is not drawn
        RoleCell = 0x10000000,    // a role stands on it
        SolidRole = 0x20000000,   // a solid role stands on it
        Wall = 0x40000000,
        Outside = 0xFFFFFFFF,  // read past the cells
    };

    static constexpr u32 ShadeShift = 8;  // from ShadeMask to Role::CellShades

    static constexpr i32 TrueColorBits = 16;                      // in `bitsPerPixel`: 16-bit tiles
    static constexpr i32 IndexedBits = 8;                         // indices into the city palette
    static constexpr int TileSize = 0x10;                         // pixels per tile side (one cell)
    static constexpr int TileShift = 4;                           // pixels to tiles
    static constexpr int TileFraction = TileSize - 1;             // the pixels inside a tile
    static constexpr int TileBytes = 2 * TileSize * TileSize;     // a 16-bit tile in the map unit
    static constexpr int IndexedTileBytes = TileSize * TileSize;  // an 8-bit tile
    static constexpr int PaletteBytes = 0x200;                    // the palette before the 8-bit tiles

    i32 width = 0;                     // +0x00 tiles
    i32 height = 0;                    // +0x04 tiles
    i32 bitsPerPixel = 0;              // +0x08 16: the map units hold 16-bit pixels, 8: indices into the city palette
    i32 unk0C = 0;                     // +0x0C
    i32 layers = 0;                    // +0x10 tile words per cell
    i32 unk14 = 0;                     // +0x14
    std::vector<u32> blocks;           // +0x18 a word per cell: walls, events and the roles on it
    std::vector<u16> tiles;            // +0x1C width x layers x height tile codes
    const u8* pixels = nullptr;        // +0x20 the cached unit of the map (a mapped view)
    std::array<char, 0x40> name = {};  // +0x24
};

// The kinds of move (the low nibble of MoveRecord::mode).
enum class MoveKind : u8
{
    None = 0,
    Path = 1,      // a walk along the role's path
    Scripted = 2,  // a move given by an event
};

// The movement of a role along a path (0x21C bytes, 72 of them at 0x004AD490); filled with 0xFF when unused.
struct MoveRecord
{
    static constexpr u16 NoRole = 0xFFFF;  // in `role`: an unused record

    // The bits of `step`.
    enum Step : u16
    {
        StepMask = 0x7FFF,  // the step reached
        Done = 0x8000,      // the step was taken
    };

    // The bits of `mode`.
    enum Mode : u8
    {
        KindMask = 0xF,    // the MoveKind
        HoldAtEnd = 0x80,  // the role stays held by the event when it arrives
    };

    static constexpr u8 EndOfPath = 0xFF;  // in `directions`

    u16 role = 0;                           // +0x00 the role moving (NoRole: unused)
    u16 step = 0;                           // +0x02 bits 0..14: the step reached, Done once taken
    u16 x = 0;                              // +0x04 the end of the move (pixels)
    u16 y = 0;                              // +0x06
    u16 nextRole = 0;                       // +0x08 a move queued after this one (0xFFFF: none)
    u16 unk0A = 0;                          // +0x0A
    u16 nextX = 0;                          // +0x0C
    u16 nextY = 0;                          // +0x0E
    i16 act = 0;                            // +0x10 the act at the end (-1: kept)
    i16 action = 0;                         // +0x12
    i16 direction = 0;                      // +0x14
    i16 stepX = 0;                          // +0x16 pixels per frame
    i16 stepY = 0;                          // +0x18
    u8 unk1A = 0;                           // +0x1A
    u8 mode = 0;                            // +0x1B low nibble: the kind of move, bit 7: the event keeps running
    std::array<u8, 0x200> directions = {};  // +0x1C the direction of every step (0xFF ends)

    void Clear();

    MoveKind Kind() const
    {
        return static_cast<MoveKind>(this->mode & MoveRecord::KindMask);
    }

    void SetKind(MoveKind kind)
    {
        this->mode = static_cast<u8>((this->mode & ~MoveRecord::KindMask) | static_cast<u8>(kind));
    }

    // The direction of the step reached (kEndOfPath at the end).
    u8 CurrentDirection() const
    {
        return this->directions[static_cast<std::size_t>(this->step & MoveRecord::StepMask) % this->directions.size()];
    }
};

// A clickable rectangle (0x10 bytes in the original list at 0x004C8BEC).
struct ClickRegion
{
    i32 unk00 = 0;   // +0x00
    u16 left = 0;    // +0x08
    u16 top = 0;     // +0x0A
    u16 right = 0;   // +0x0C
    u16 bottom = 0;  // +0x0E
};

// The clickable rectangles of the current screen and their state (0x004C8BE8..0x004C8BF8).
class ClickRegions
{
public:
    int Count() const;
    int Find(int pointerX, int pointerY, ClickRegion*& found);
    void Free();

    // In `flags`: the regions are the choices of a talk window (Space or Enter frees them).
    static constexpr i32 TalkChoices = 0x1000;

    i32 flags = 0;                   // 0x004C8BE8
    std::list<ClickRegion> regions;  // 0x004C8BEC, the head first
    std::array<i32, 3> state = {};   // 0x004C8BF0
};

class GameState
{
public:
    // The shared game state: the story flags, the pointer, the map characters and the player's control.
    // Story flags (GameState::flags) the code tests by number.
    enum StoryFlag : int
    {
        Running = 1,       // cleared when the debug quit key ends the game
        DebugReports = 2,  // the internal errors are reported
        Music = 3,         // music plays (cleared without DirectInput)
        SoundEffects = 4,  // (cleared without DirectInput)
        Weather = 5,       // the map has weather particles
        Leaves = 6,        // leaves blow across the map
        HeatHaze = 7,
        Lightning = 8,
        NoChoicePointer = 9,  // the pointer and the click choices of the talk windows are off
        NoPointer = 0xA,      // the pointer is not drawn
        HideActRoles = 0xB,   // roles drawn by an act alone are skipped
        Encounters = 0xC,     // enemies are met on the map
        OverlaysFirst = 0xF,  // the map's overlays are drawn before the roles (map effect bit 7)
        Timer = 0x10,         // the first timer runs
        TimerExpired = 0x11,
        TimerEvent = 0x12,  // the first timer runs an event when it expires
        Spotlight = 0x13,
        NoSlide = 0x14,  // steps into walls do not slide (debug key Ctrl+F3)
        Unk15 = 0x15,    // cleared by a new game
        Fireflies = 0x16,
        InParty = 0x1E,  // 0x1E..0x21: the four roles are in the party
        NoLoadingScreen = 0x46,
        PlainScreen = 0x48,  // the spotlight, the bubbles, the ripples and the screen effects are off
        SecondTimer = 0x4A,
        SecondTimerExpired = 0x4B,
        HideTimer = 0x4C,
        FileNameA = 0x4D,  // talk commands 0xAA..0xAD
        FileNameB = 0x4E,
        FollowersPassWalls = 0x4F,
        TextBox = 0x50,           // the keyboard goes to the text box
        HideHud = 0x51,           // the gold counter and the pointer are not drawn
        DebugKeys = 0x52,         // the debug keys work
        SecondDisc = 0x53,        // the minimal installation reads the second disc file
        UnkC9 = 0xC9,             // the battle start sets panel flag 2
        MapNoEncounter = 0x1770,  // + the map number: no enemies on that map
        Unk1BB0 = 0x1BB0,         // the timing window of the battle is 0x12 frames instead of 0x3C
    };

    static constexpr int Roles = 256;
    static constexpr int Moves = 72;
    static constexpr int FlagBytes = 0x400;
    // 1 << n (swd32.exe 0x004994EC).
    static constexpr std::array<u8, 8> BitMasks = {1, 2, 4, 8, 16, 32, 64, 128};

    bool GameFlag(int flag) const;
    u8 SetGameFlag(int flag);
    u8 ClearGameFlag(int flag);
    u8 SetGameFlagTo(int flag, int value);
    void SetWalkSpeed(int speed);
    int SetFrameInterval(int value);
    void ClearFrameInterval();
    int DirectionToPlayer(int x, int y) const;
    Role& Player();
    const Role& Player() const;
    void ClearWorld();
    void ClearRoles();
    u32 FindRole(u16 id) const;
    bool FindRole(u16 id, u32& index) const;
    bool FindRoleOrPlayer(u16 id, u32& index) const;

    std::array<u8, FlagBytes> flags = {};  // 0x004AB384 story flags, saved with the game
    i32 pointerX = 0;                      // 0x004A9924 pointer position on the screen
    i32 pointerY = 0;                      // 0x004A9928
    i32 playerRole = 0;                    // 0x004AB378 the controlled role
    std::array<Role, Roles> roles;         // 0x004BABA8
    Role leader;                           // 0x004B72A0 the leading role (filled with 0xFF when unused)
    i32 roleCount = 1;                     // 0x0049E0C4 roles in use
    MapHeader map;                         // 0x004B7930
    std::array<MoveRecord, Moves> moves;   // 0x004AD490
    i32 walkSpeed = 0;                     // 0x004AB2D0 the pixels of a step of the camera
    i32 frameInterval = 0;                 // 0x004B7BCC milliseconds between the frames of the main loop
    ClickRegions clickRegions;
};

}  // namespace swd3
