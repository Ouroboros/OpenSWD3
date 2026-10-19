#pragma once

#include <array>
#include <cstddef>

#include "core/types.h"
#include "map/blocking_map.h"
#include "res/act_state.h"

namespace swd3
{

class Game;
class PathFinder;
struct BlockingMap;
struct Role;

// The roles walking on the map (the original's Path, 0x00405430..0x00406960): each role runs its script of path.dat
// (moves, waits, jumps on time, flags and variables, follows, labels), steps along its found path or wanders, and
// the followers trail the leader.
class RolePath
{
public:
    // swd32.exe 0x0049941C (a copy at 0x0049961C): the probe bits (Field::ProbeCells) that block a step in each path
    // direction.
    static constexpr std::array<u32, 8> BlockedBy = {
        BlockingMap::ProbeDownRight,
        BlockingMap::ProbeDown,
        BlockingMap::ProbeDownLeft,
        BlockingMap::ProbeLeft,
        BlockingMap::ProbeUpLeft,
        BlockingMap::ProbeUp,
        BlockingMap::ProbeUpRight,
        BlockingMap::ProbeRight,
    };

    // swd32.exe 0x0049943C and 0x0049945C (copies at 0x0049963C and 0x0049965C, 0x004994AC and 0x004994CC, 0x00499380
    // and 0x004993A0): the pixels of a step in each path direction.
    static constexpr std::array<i32, 8> StepX = {4, 0, -4, -4, -4, 0, 4, 4};
    static constexpr std::array<i32, 8> StepY = {4, 4, 4, 0, -4, -4, -4, 0};
    static constexpr int FollowerSpeed = 2;  // a follower steps at double speed (0x00406960)
    // swd32.exe 0x0049947C (a copy at 0x0049967C): the act direction of each path direction.
    static constexpr std::array<i32, 8> Facing = {
        ActState::DownRight,
        ActState::Down,
        ActState::DownLeft,
        ActState::Left,
        ActState::UpLeft,
        ActState::Up,
        ActState::UpRight,
        ActState::Right,
    };

    static constexpr int Moves = 72;
    static constexpr u8 Directions = 8;

    explicit RolePath(Game& game);

    void Update();
    void RunScript(int index);
    bool FindPath(int index, u16 tileX, u16 tileY);
    int Step(int index);
    std::size_t JumpTo(int index, u32 offset);
    int Wander(int index);
    void Follow(int index);
    u16 RoleId(u32 index);
    bool ResumeMove(int index);
    // The options of MoveRoleTo: any of bits 0..3 puts the role there at once (PlaceThere), none walks it there.
    static constexpr u32 WalkThere = 0;
    static constexpr u32 PlaceThere = 1;
    static constexpr u32 PlaceMask = 0xF;
    static constexpr u32 EventGoesOn = 0x8000;  // the event does not wait for the move
    static constexpr i16 Keep = -1;             // an act, action or direction left as it is

    int MoveRoleTo(int index, u16 x, u16 y, u32 options, i16 act, i16 action, i16 direction);
    // The results of ScriptStep.
    static constexpr int NoScriptedMove = 0;
    static constexpr int Stepped = 1;
    static constexpr int PathEnded = 2;

    int ScriptStep(int index);
    int HoldRole(int index);

    i32 timeMark = 0;  // 0x004BA42C

private:
    BlockingMap Grid() const;
    void AlignPlayer(Role& role);

    Game& game;
};

}  // namespace swd3
