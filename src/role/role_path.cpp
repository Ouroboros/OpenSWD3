#include "role/role_path.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <span>

#include "core/debug_log.h"
#include "core/fault.h"
#include "core/format.h"
#include "core/words.h"
#include "crt/msvc.h"
#include "game/game.h"
#include "map/blocking_map.h"
#include "role/path_finder.h"

namespace swd3
{

namespace
{

constexpr u16 kFree = 0xFFFF;
constexpr u8 kPathEnd = 0xFF;
constexpr u32 kFastWalk = 0x4000000;
constexpr u32 kBlockedTries = 8;  // after this many blocked steps a role walks through other roles
constexpr i32 kWalkAction = 8;
constexpr int kNearMargin = 0x50;  // a follower or moved role appears this far around the camera
constexpr int kFarMargin = 0xA0;
constexpr int kStepsPerTile = MapHeader::TileSize / 4;  // RolePath::StepX and StepY move 4 pixels
constexpr int kFollowerSpeed = 2;
constexpr u32 kTrailSpacing = 2;   // the trail points between two followers
constexpr u32 kWanderRoll = 0x50;  // a wandering role rolls 0..79: directions below 8, a rest above kRestAbove
constexpr int kRestAbove = 0x3C;
constexpr int kUnkMoveSlot = 0x20;  // FindPath: false only for this slot, which the full table never reaches
constexpr int kFastWalkGap = 0x80;  // a follower further than this from its target walks fast

// path.dat: a table of the start of every path (dwords relative to kPathData), the paths from kPathData. A path is a
// list of commands: a command word, then word operands; jump targets are dwords.
constexpr std::size_t kPathData = 0x200;
constexpr std::size_t kPathWordBytes = sizeof(u16);
constexpr std::size_t kPathTargetBytes = sizeof(u32);
constexpr int kOpcodeWords = 1;
constexpr int kTargetWords = 2;
constexpr int kStepShift = 1;  // path offsets in bytes to steps in words
constexpr u16 kFlagListEnd = 0xFFFF;
constexpr u16 kKeepAct = 0xFFFF;      // SetAct: the act, action or direction left as it is
constexpr u16 kLabelEnd = 0x5125;     // "%Q" ends the text of a label
constexpr u16 kLabelColors = 0xA;     // a label colour past it becomes 0
constexpr i32 kVariableLimit = 1000;  // path variables are capped here

enum class PathCommand : u16
{
    Restart = 0,
    Stop = 1,
    SetAction = 2,
    SetDirection = 3,
    SetCount = 4,
    WaitCount = 5,
    WaitCountAnimated = 6,
    FindPath = 7,
    Step = 8,
    WaitClock = 9,
    JumpIfClockReached = 0xA,
    SetClock = 0xB,
    Jump = 0xC,
    JumpIfFlag = 0xD,
    JumpUnlessFlag = 0xE,
    JumpIfAllFlags = 0xF,
    JumpIfAnyFlag = 0x10,
    SetFlag = 0x11,
    ClearFlag = 0x12,
    SetVariable = 0x13,
    AddVariable = 0x14,
    SubtractVariable = 0x15,
    JumpIfVariableAtLeast = 0x16,
    JumpIfVariableAtMost = 0x17,
    StartPath = 0x18,
    JumpAfterMark = 0x19,
    MarkClock = 0x1A,
    HideRole = 0x1B,
    ShowRole = 0x1C,
    PlaceRole = 0x1D,
    Wander = 0x1E,
    SetAct = 0x1F,
    PlaySound = 0x20,
    FollowRole = 0x21,
    ShowLabel = 0x22,
    ClearLabel = 0x23,
    FollowBeside = 0x24,
};

// The words of a command with `operands` word operands and `targets` jump targets.
constexpr int CommandWords(int operands, int targets = 0)
{
    return kOpcodeWords + operands + targets * kTargetWords;
}

u16 PathWord(std::span<const u8> data, std::size_t at)
{
    return at + 1 < data.size() ? static_cast<u16>(data[at] | data[at + 1] << CHAR_BIT) : u16{0};
}

u32 PathDword(std::span<const u8> data, std::size_t at)
{
    return MakeWords(PathWord(data, at + kPathWordBytes), PathWord(data, at));
}

}  // namespace

RolePath::RolePath(Game& game)
    : game(game)
{
}

BlockingMap RolePath::Grid() const
{
    const MapHeader& map = this->game.state.map;
    BlockingMap grid;
    grid.width = map.width;
    grid.height = map.height;
    grid.cells = map.blocks;
    return grid;
}

// swd32.exe 0x0040C060: the id of role `index`; an index past the roles in use shows "Illegal Role id" first
u16 RolePath::RoleId(u32 index)
{
    GameState& state = this->game.state;
    const u16 id = state.roles[std::min<std::size_t>(index, GameState::Roles - 1)].id;

    if (index >= static_cast<u32>(state.roleCount))
    {
        this->game.errors.ShowError(Utf8Format(u8"%d", {static_cast<u32>(id)}), u8"Illegal Role id");
    }

    return id;
}

// swd32.exe 0x00405430: every visible role but the first: followers trail the leader, held roles (flag bit 31 or
// talking) and roles without a path keep their cells (a role with flag 0x1000 steps its act unless controlled), the
// others run their path script
void RolePath::Update()
{
    GameState& state = this->game.state;
    const auto leader = static_cast<u16>(this->game.unk4B7BC4);

    for (u32 index = 1; index < static_cast<u32>(state.roleCount) && index < GameState::Roles; ++index)
    {
        Role& role = state.roles[index];

        if ((role.flags & Role::Visible) == 0)
        {
            continue;
        }

        if ((role.flags & Role::Follower) != 0)
        {
            this->Follow(static_cast<int>(index));
            continue;
        }

        if (static_cast<i32>(role.flags) < 0 || role.talkState == Role::TalkWaiting)
        {
            if (role.act.id != 0)
            {
                this->game.field.MarkRoleCells(role, leader);
            }

            continue;
        }

        if (role.unk1C == 0)
        {
            if ((role.flags & Role::StepsAct) == 0 || index == static_cast<u32>(state.playerRole))
            {
                continue;
            }

            this->game.acts.Run(role.act);

            if (role.act.id != 0)
            {
                this->game.field.MarkRoleCells(role, leader);
            }

            continue;
        }

        this->RunScript(static_cast<int>(index));
    }
}

// swd32.exe 0x00406390: a path from role `index` to tile (tileX, tileY) into a free move; a target past the map is
// refused, an unreachable one moves the role there at once. False only when the move taken is the 33rd.
bool RolePath::FindPath(int index, u16 tileX, u16 tileY)
{
    GameState& state = this->game.state;
    int slot = 0;

    while (state.moves[static_cast<std::size_t>(slot)].role != kFree)
    {
        if (++slot >= RolePath::Moves)
        {
            return slot != kUnkMoveSlot;
        }
    }

    Role& role = state.roles[static_cast<std::size_t>(index)];
    const BlockingMap grid = this->Grid();
    PathFinder finder(this->game.pathNodes, grid, this->game.errors);
    const u32 x = MapHeader::TileSize * tileX;
    const u32 y = MapHeader::TileSize * tileY;

    if (x > MapHeader::TileSize * static_cast<u32>(state.map.width) ||
        y > MapHeader::TileSize * static_cast<u32>(state.map.height))
    {
        DebugLog(
            u8"path target outside",
            slot,
            index,
            role.x >> MapHeader::TileShift,
            role.y >> MapHeader::TileShift,
            x >> MapHeader::TileShift,
            y >> MapHeader::TileShift
        );
        return false;
    }

    MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

    if (finder.FindPath(
            role.x,
            role.y,
            static_cast<int>(x),
            static_cast<int>(y),
            role.act.width,
            role.act.height,
            move.directions
        ) == 1)
    {
        move.act = -1;
        move.action = -1;
        move.direction = -1;
        move.role = static_cast<u16>(index);
        move.step = 0;
        move.x = static_cast<u16>(x);
        move.y = static_cast<u16>(y);
        move.SetKind(MoveKind::Path);
    }
    else
    {
        DebugLog(
            u8"path not found",
            slot,
            this->RoleId(static_cast<u32>(index)),
            role.x >> MapHeader::TileShift,
            role.y >> MapHeader::TileShift,
            x >> MapHeader::TileShift,
            y >> MapHeader::TileShift
        );
        role.x = static_cast<i32>(x);
        role.y = static_cast<i32>(y);
    }

    return slot != kUnkMoveSlot;
}

// swd32.exe 0x00406580: the next step of role `index` along its move: held while it talks; at the path's end the move
// is freed (2); otherwise the step is taken unless the cells ahead are blocked (walls only after 8 tries) (1). 0 when
// the move is a scripted one, 2 when it has none.
int RolePath::Step(int index)
{
    GameState& state = this->game.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    int slot = 0;

    for (; slot < RolePath::Moves; ++slot)
    {
        const MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

        if (move.role == index)
        {
            if (move.Kind() == MoveKind::Scripted)
            {
                return 0;
            }

            if (move.Kind() == MoveKind::Path)
            {
                break;
            }
        }
    }

    if (slot == RolePath::Moves)
    {
        DebugLog(u8"no path", this->RoleId(static_cast<u32>(index)), role.act.id, role.act.action, role.unk1C);
        return RolePath::PathEnded;
    }

    MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

    if (role.talkState == Role::TalkWaiting)
    {
        move.step |= MoveRecord::Done;
        return 0;
    }

    const u8 direction = move.CurrentDirection();

    if (direction == kPathEnd)
    {
        move.Clear();
        return RolePath::PathEnded;
    }

    const u32 mask = move.unk1A > kBlockedTries ? MapHeader::Wall : MapHeader::Wall | MapHeader::SolidRole;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    move.step |= MoveRecord::Done;
    ++move.unk1A;
    move.stepY = 0;
    move.stepX = 0;

    if (direction < RolePath::Directions && (probe & RolePath::BlockedBy[direction]) == 0)
    {
        move.step &= static_cast<u16>(~MoveRecord::Done);
        move.unk1A = 0;
        move.stepX = static_cast<i16>(RolePath::StepX[direction]);
        move.stepY = static_cast<i16>(RolePath::StepY[direction]);
    }

    return 1;
}

// swd32.exe 0x00406710: the script of role `index` continues at `offset` of path.dat; its step is set from the start of
// its path. The new position is returned.
std::size_t RolePath::JumpTo(int index, u32 offset)
{
    Role& role = this->game.state.roles[static_cast<std::size_t>(index)];
    const std::span<const u8> data = this->game.databases.paths.View();
    const std::size_t table = kPathTargetBytes * static_cast<std::size_t>(role.unk1C);
    u32 start = 0;

    if (kPathData + table + kPathTargetBytes <= data.size())
    {
        start = PathDword(data, kPathData + table);
    }

    role.unk18 =
        static_cast<i32>(static_cast<std::ptrdiff_t>(offset) - static_cast<std::ptrdiff_t>(start)) >> kStepShift;
    return kPathData + static_cast<std::size_t>(offset);
}

// swd32.exe 0x00406770: a random step of role `index` (four in five frames it rests): a move of one step is taken
// (a new one from a free record when it has none, a scripted move leaves it alone); the direction is new one time in
// ten. False when no record is free.
int RolePath::Wander(int index)
{
    GameState& state = this->game.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    this->game.crtRandom.Next();
    const auto chance = static_cast<int>(this->game.random.Below(kWanderRoll));
    bool found = false;
    int slot = 0;

    for (int at = 0; at < RolePath::Moves; ++at)
    {
        const MoveRecord& move = state.moves[static_cast<std::size_t>(at)];

        if (move.role == index)
        {
            if (move.Kind() == MoveKind::Scripted)
            {
                return 1;
            }

            if (move.Kind() == MoveKind::Path)
            {
                found = true;
                slot = at;
            }
        }
    }

    if (!found)
    {
        slot = 0;

        while (state.moves[static_cast<std::size_t>(slot)].role != kFree)
        {
            if (++slot >= RolePath::Moves)
            {
                DebugLog(Utf8Format(
                    u8"Pth:rndwlk,g(%d)NoFreePath",
                    {static_cast<u32>(this->RoleId(static_cast<u32>(index)))}
                ));
                return 0;
            }
        }

        MoveRecord& fresh = state.moves[static_cast<std::size_t>(slot)];
        fresh.role = static_cast<u16>(index);
        fresh.step = 0;
        fresh.x = 0;
        fresh.y = 0;
        fresh.SetKind(MoveKind::Path);
        fresh.directions[0] = 1;
    }

    MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

    if (role.talkState == Role::TalkWaiting || chance > kRestAbove)
    {
        move.step |= MoveRecord::Done;
        role.act.action = 0;
        return 1;
    }

    role.act.action = kWalkAction;

    if (chance < RolePath::Directions)
    {
        move.directions[0] = static_cast<u8>(chance);
    }

    const u8 direction = move.directions[0];

    if (direction == kPathEnd)
    {
        return 1;
    }

    const u32 mask = move.unk1A > kBlockedTries ? MapHeader::Wall : MapHeader::Wall | MapHeader::SolidRole;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    ++move.unk1A;
    move.step = MoveRecord::Done;
    move.stepY = 0;
    move.stepX = 0;

    if (direction < RolePath::Directions && (probe & RolePath::BlockedBy[direction]) == 0)
    {
        move.step = 0;
        move.unk1A = 0;
        move.stepX = static_cast<i16>(RolePath::StepX[direction]);
        move.stepY = static_cast<i16>(RolePath::StepY[direction]);
    }

    return 1;
}

// swd32.exe 0x00406960: follower `index` walks after the leader: on a tile it finds a path to its place in the
// leader's trail (unless its move still has steps), skipping the steps off screen at once; then it takes the next step
// at double speed unless blocked (walls are ignored with story flag 0x4F), facing the leader's way at the end
void RolePath::Follow(int index)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Role& role = state.roles[static_cast<std::size_t>(index)];

    if (((role.x | role.y) & MapHeader::TileFraction) != 0)
    {
        return;
    }

    u32 follower = 1;

    while (follower < static_cast<u32>(view.unk4BABA0) && follower < view.unk4B7C8C.size() &&
           view.unk4B7C8C[follower] != index)
    {
        ++follower;
    }

    if (follower == static_cast<u32>(view.unk4BABA0) || follower >= view.followerMoves.size())
    {
        DebugLog(u8"CompanyWalk", role.id);
        return;
    }

    MoveRecord& move = view.followerMoves[follower];
    const u16 leaderId = static_cast<u16>(owner.unk4B7BC4);

    auto direction = [&move]()
    {
        return move.CurrentDirection();
    };

    if (move.role == kFree || move.unk1A != 0 || direction() == kPathEnd)
    {
        const BlockingMap grid = this->Grid();
        PathFinder finder(owner.pathNodes, grid, owner.errors);
        move.Clear();
        const i32 targetX = view.trailX[kTrailSpacing * follower];
        const i32 targetY = view.trailY[kTrailSpacing * follower];

        if (state.GameFlag(GameState::FollowersPassWalls))
        {
            finder.SetMask(0);
        }

        if (finder.FindPath(role.x, role.y, targetX, targetY, role.act.width, role.act.height, move.directions) == 0)
        {
            ErrorLog(
                u8"CmpyWalk PathFind fail ,G(%d) sx(%d/%d) sy(%d/%d) dx(%d/%d) dy(%d/%d)",
                role.id,
                role.x,
                role.x >> MapHeader::TileShift,
                role.y,
                role.y >> MapHeader::TileShift,
                targetX,
                targetX >> MapHeader::TileShift,
                targetY,
                targetY >> MapHeader::TileShift
            );
        }
        else
        {
            move.SetKind(MoveKind::Path);
            move.role = static_cast<u16>(index);
            move.step = 0;
            move.x = static_cast<u16>(targetX);
            move.y = static_cast<u16>(targetY);
            move.unk1A = 0;
            owner.field.ClearRoleCells(role, leaderId);
            const Field& field = owner.field;
            const auto bottom = static_cast<u32>(field.cameraBottom + kFarMargin);
            auto top = static_cast<u32>(field.cameraTop - kNearMargin);
            auto left = static_cast<u32>(field.cameraLeft - kNearMargin);
            const auto right = static_cast<u32>(field.cameraRight + kNearMargin);

            if (field.cameraTop - kNearMargin < 0)
            {
                top = 0;
            }

            if (field.cameraLeft - kNearMargin < 0)
            {
                left = 0;
            }

            for (;;)
            {
                const u8 step = direction();
                const auto x = static_cast<u32>(role.x);
                const auto y = static_cast<u32>(role.y);

                if (x > left && x < right && y > top && y < bottom)
                {
                    if (step != kPathEnd && step < RolePath::Directions)
                    {
                        role.act.direction = RolePath::Facing[step];
                    }

                    break;
                }

                if (step == kPathEnd || step >= RolePath::Directions)
                {
                    break;
                }

                ++move.step;
                role.x += kStepsPerTile * RolePath::StepX[step];
                role.y += kStepsPerTile * RolePath::StepY[step];
            }

            move.step |= MoveRecord::Done;
            const std::size_t at =
                (static_cast<u32>(role.x) >> MapHeader::TileShift) +
                static_cast<std::size_t>(state.map.width) * (static_cast<u32>(role.y) >> MapHeader::TileShift);
            role.cell = at < state.map.blocks.size() ? &state.map.blocks[at] : nullptr;
            owner.field.MarkRoleCells(role, leaderId);
            owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
        }
    }

    const u32 mask = state.GameFlag(GameState::FollowersPassWalls) ? 0u : MapHeader::Wall;
    const u8 step = direction();
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = owner.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    move.step |= MoveRecord::Done;
    const u16 taken = move.step;
    move.stepY = 0;
    move.stepX = 0;

    if (step == kPathEnd)
    {
        role.act.action = 0;
        role.act.direction = state.roles[static_cast<std::size_t>(state.playerRole)].act.direction;
    }
    else if (step >= RolePath::Directions || (probe & RolePath::BlockedBy[step]) != 0)
    {
        role.act.action = 0;
    }
    else
    {
        move.step = static_cast<u16>(taken & MoveRecord::StepMask);
        ++move.unk1A;
        role.act.action = kWalkAction;
        move.stepX = static_cast<i16>(kFollowerSpeed * RolePath::StepX[step]);
        move.stepY = static_cast<i16>(kFollowerSpeed * RolePath::StepY[step]);
    }
}

// swd32.exe 0x00405500: the path script of role `index` (path.dat, its path at the role's step): commands run until
// one waits for the next frame (an act change, a count, a step, a move, a label, a sound). Command 0xB takes one word
// only, so its operand runs as the next command; command 0x24 keeps no offset for a role facing direction 1 (here 0,
// the original reads leftovers of the stack). A role of command 0x18 that is not found is left alone here (the
// original writes before the role table).
void RolePath::RunScript(int index)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    const std::span<const u8> data = owner.databases.paths.View();
    auto& variables = owner.party.variables;
    const u16 leaderId = static_cast<u16>(owner.unk4B7BC4);

    auto word = [&data](std::size_t at)
    {
        return PathWord(data, at);
    };

    auto dword = [&data](std::size_t at)
    {
        return PathDword(data, at);
    };

    std::size_t at = kPathData + dword(kPathData + kPathTargetBytes * static_cast<std::size_t>(role.unk1C)) +
                     kPathWordBytes * static_cast<std::size_t>(role.unk18);

    auto operand = [&data, &at](std::size_t index)
    {
        return PathWord(data, at + kPathWordBytes * index);
    };

    auto target = [&data, &at](std::size_t index)
    {
        return PathDword(data, at + kPathWordBytes * index);
    };

    i32& time = owner.party.clock;

    auto advance = [&role, &at](int words)
    {
        at += kPathWordBytes * static_cast<std::size_t>(words);
        role.unk18 += words;
    };

    auto jump = [this, index, &at](u32 offset)
    {
        at = this->JumpTo(index, offset);
    };

    for (;;)
    {
        const u16 code = word(at);
        const auto command = static_cast<PathCommand>(code);

        switch (command)
        {
            case PathCommand::Restart:
                role.unk18 = 0;
                return;

            case PathCommand::Stop:
                role.unk1C = 0;
                role.unk18 = 0;
                return;

            case PathCommand::SetAction:
                role.act.action = operand(1);
                role.unk18 += CommandWords(1);
                owner.acts.Run(role.act);
                return;

            case PathCommand::SetDirection:
                role.act.direction = operand(1);
                role.unk18 += CommandWords(1);
                owner.acts.Run(role.act);
                return;

            case PathCommand::SetCount:
                role.unk22 = operand(1);
                advance(CommandWords(1));
                continue;

            case PathCommand::WaitCount:
                if (role.unk22 != 0)
                {
                    --role.unk22;
                }
                else
                {
                    ++role.unk18;
                }

                return;

            case PathCommand::WaitCountAnimated:
                if (role.unk22 != 0)
                {
                    --role.unk22;
                }
                else
                {
                    ++role.unk18;
                }

                owner.acts.Run(role.act);
                return;

            case PathCommand::FindPath:
                this->FindPath(index, operand(1), operand(2));
                advance(CommandWords(2));
                continue;

            case PathCommand::Step:
                if (((role.x | role.y) & MapHeader::TileFraction) != 0 || this->Step(index) != RolePath::PathEnded)
                {
                    return;
                }

                advance(CommandWords(0));
                role.flags &= ~kFastWalk;
                continue;

            case PathCommand::WaitClock:
            {
                bool passed = false;

                if (operand(1) <= static_cast<u32>(time))
                {
                    advance(CommandWords(1));
                    passed = true;
                }

                if ((role.flags & Role::StepsAct) != 0)
                {
                    owner.acts.Run(role.act);
                }

                if (passed)
                {
                    continue;
                }

                return;
            }

            case PathCommand::JumpIfClockReached:
                if (operand(1) > static_cast<u16>(time))
                {
                    advance(CommandWords(1, 1));
                    continue;
                }

                jump(target(2));
                continue;

            case PathCommand::SetClock:
                time = operand(1);

                if (static_cast<u32>(time) > Party::ClockLimit)
                {
                    time = 0;
                }

                advance(CommandWords(0));
                continue;

            case PathCommand::Jump:
                jump(target(1));
                continue;

            case PathCommand::JumpIfFlag:
            case PathCommand::JumpUnlessFlag:
                if ((static_cast<int>(state.GameFlag(operand(1))) ^
                     static_cast<int>(command == PathCommand::JumpUnlessFlag)) == 1)
                {
                    jump(target(2));
                    continue;
                }

                advance(CommandWords(1, 1));
                continue;

            case PathCommand::JumpIfAllFlags:
            case PathCommand::JumpIfAnyFlag:
            {
                int count = 0;
                int set = 0;
                std::size_t flag = at + kPathWordBytes * CommandWords(0);

                while (word(flag) != kFlagListEnd && flag < data.size())
                {
                    if (state.GameFlag(word(flag)))
                    {
                        ++set;
                    }

                    ++count;
                    flag += kPathWordBytes;
                }

                const bool taken =
                    command == PathCommand::JumpIfAllFlags ? count == 0 || count == set : count != 0 && set != 0;

                if (taken)
                {
                    jump(dword(flag + kPathWordBytes));
                    continue;
                }

                advance(CommandWords(count + 1, 1));
                continue;
            }

            case PathCommand::SetFlag:
                state.SetGameFlag(operand(1));
                advance(CommandWords(1));
                continue;

            case PathCommand::ClearFlag:
                state.ClearGameFlag(operand(1));
                advance(CommandWords(1));
                continue;

            case PathCommand::SetVariable:
            case PathCommand::AddVariable:
            case PathCommand::SubtractVariable:
            case PathCommand::JumpIfVariableAtLeast:
            case PathCommand::JumpIfVariableAtMost:
            {
                const u16 slot = operand(1);
                const u16 value = operand(2);

                if (slot >= Party::Variables || slot >= variables.size())
                {
                    DebugLog(u8"path variable", slot, value);
                    return;
                }

                if (value > kVariableLimit)
                {
                    DebugLog(u8"path value", slot, value);
                }

                i32& variable = variables[slot];
                int length = 0;

                switch (command)
                {
                    case PathCommand::SetVariable:
                        variable = value;
                        length = CommandWords(2);
                        break;

                    case PathCommand::AddVariable:
                    {
                        const i32 before = variable;
                        variable = value + before;

                        if (static_cast<u32>(value) + static_cast<u32>(before) > kVariableLimit)
                        {
                            variable = kVariableLimit;
                        }

                        length = CommandWords(2);
                        break;
                    }

                    case PathCommand::SubtractVariable:
                    {
                        const i32 before = variable;
                        variable = before - value;

                        if (before - value < 0)
                        {
                            variable = 0;
                        }

                        length = CommandWords(2);
                        break;
                    }

                    default:
                        length = CommandWords(2, 1);

                        if (command == PathCommand::JumpIfVariableAtLeast ? variable >= value : variable <= value)
                        {
                            jump(target(3));
                            length = 0;
                        }

                        break;
                }

                advance(length);
                continue;
            }

            case PathCommand::StartPath:
            {
                u32 target = 0;
                state.FindRoleOrPlayer(operand(1), target);

                if (target < GameState::Roles)
                {
                    Role& other = state.roles[target];

                    for (int slot = 0; slot < RolePath::Moves; ++slot)
                    {
                        MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

                        if (move.role != target)
                        {
                            continue;
                        }

                        if (move.Kind() == MoveKind::Scripted)
                        {
                            move.nextRole = MoveRecord::NoRole;
                            move.unk0A = MoveRecord::Unused;
                            move.nextX = MoveRecord::Unused;
                            move.nextY = MoveRecord::Unused;
                        }

                        if (move.Kind() == MoveKind::Path)
                        {
                            if (((other.x | other.y) & MapHeader::TileFraction) != 0)
                            {
                                const int row = other.y >> MapHeader::TileShift;
                                owner.field.ClearRoleCells(other, leaderId);
                                const u8 back = move.directions
                                                    [static_cast<std::size_t>(move.step & MoveRecord::StepMask) %
                                                     move.directions.size()];

                                while ((other.x & MapHeader::TileFraction) != 0 && back < RolePath::Directions)
                                {
                                    other.x -= RolePath::StepX[back];
                                }

                                while ((other.y & MapHeader::TileFraction) != 0 && back < RolePath::Directions)
                                {
                                    other.y -= RolePath::StepY[back];
                                }

                                owner.field
                                    .RemoveRole(other.id, static_cast<int>(other.flags & Role::LayerMask), row, 0);
                            }

                            move.Clear();
                        }

                        break;
                    }
                }

                role.unk18 += CommandWords(2);
                at += kPathWordBytes * CommandWords(2);
                const u16 path = word(at - kPathWordBytes);

                if (target < GameState::Roles)
                {
                    Role& other = state.roles[target];
                    other.unk1C = path;
                    other.unk18 = 0;

                    if (target == static_cast<u32>(index))
                    {
                        at = kPathData + dword(kPathData + kPathTargetBytes * static_cast<std::size_t>(path));
                    }
                }

                continue;
            }

            case PathCommand::JumpAfterMark:
                if (static_cast<u32>(time) > static_cast<u32>(operand(1) + this->timeMark))
                {
                    jump(target(2));
                    continue;
                }

                advance(CommandWords(1, 1));
                continue;

            case PathCommand::MarkClock:
                this->timeMark = time;
                advance(CommandWords(0));
                continue;

            case PathCommand::HideRole:
            {
                u32 target = 0;
                state.FindRoleOrPlayer(operand(1), target);

                if (target < GameState::Roles)
                {
                    Role& other = state.roles[target];
                    other.flags &= ~(Role::Visible | Role::Solid);
                    owner.field.ClearRoleCells(other, leaderId);
                }

                advance(CommandWords(1));
                continue;
            }

            case PathCommand::ShowRole:
            {
                u32 target = 0;
                state.FindRoleOrPlayer(operand(1), target);

                if (target < GameState::Roles)
                {
                    state.roles[target].flags |= Role::Visible | Role::Solid;
                }

                advance(CommandWords(1));
                continue;
            }

            case PathCommand::PlaceRole:
            {
                owner.field.ClearRoleCells(role, leaderId);
                role.x = MapHeader::TileSize * operand(1);
                role.y = MapHeader::TileSize * operand(2);
                const std::size_t cell =
                    (static_cast<u32>(role.x) >> MapHeader::TileShift) +
                    static_cast<std::size_t>(state.map.width) * (static_cast<u32>(role.y) >> MapHeader::TileShift);
                role.cell = cell < state.map.blocks.size() ? &state.map.blocks[cell] : nullptr;
                owner.acts.Run(role.act);
                owner.field.MarkRoleCells(role, leaderId);
                role.unk18 += CommandWords(2);
                owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
                return;
            }

            case PathCommand::Wander:
                if ((role.x & MapHeader::TileFraction) == 0 && (role.y & MapHeader::TileFraction) == 0)
                {
                    this->Wander(index);
                }

                return;

            case PathCommand::SetAct:
                if (operand(1) != kKeepAct)
                {
                    role.act.id = operand(1);
                }

                if (operand(2) != kKeepAct)
                {
                    role.act.action = operand(2);
                }

                if (operand(3) != kKeepAct)
                {
                    role.act.direction = operand(3);
                }

                role.unk18 += CommandWords(3);
                owner.acts.Run(role.act);
                return;

            case PathCommand::PlaySound:
                owner.PlaySoundAt(operand(1), role.x, role.y);
                role.unk18 += CommandWords(1);
                return;

            case PathCommand::FollowRole:
            case PathCommand::FollowBeside:
            {
                u32 target = 0;

                if (!state.FindRoleOrPlayer(operand(1), target) || target >= GameState::Roles)
                {
                    DebugLog(u8"Path(Follow)", operand(1));
                    role.unk18 += CommandWords(1);
                    return;
                }

                const Role& other = state.roles[target];
                const i32 otherX = other.x;
                const i32 otherY = other.y;

                if (((otherX | otherY) & MapHeader::TileFraction) != 0)
                {
                    return;
                }

                u16 tileX = static_cast<u16>(otherX >> MapHeader::TileShift);
                u16 tileY = static_cast<u16>(otherY >> MapHeader::TileShift);

                if (command == PathCommand::FollowBeside)
                {
                    const int ahead = static_cast<i16>(operand(2));
                    const int side = static_cast<i16>(operand(3));
                    int offsetX = 0;
                    int offsetY = 0;

                    switch (other.act.direction)
                    {
                        case ActState::Up:
                            offsetX = ahead;
                            offsetY = -side;
                            break;

                        case ActState::Left:
                            offsetX = -side;
                            offsetY = ahead;
                            break;

                        case ActState::Right:
                            offsetX = side;
                            offsetY = -ahead;
                            break;

                        case ActState::UpLeft:
                            offsetX = side + ahead;
                            offsetY = side - ahead;
                            break;

                        case ActState::DownRight:
                            offsetX = -(side + ahead);
                            offsetY = ahead - side;
                            break;

                        case ActState::DownLeft:
                            offsetX = side - ahead;
                            offsetY = -(ahead + side);
                            break;

                        case ActState::UpRight:
                            offsetX = ahead - side;
                            offsetY = ahead + side;
                            break;

                        default:
                            break;
                    }

                    tileY = static_cast<u16>((otherY >> MapHeader::TileShift) + offsetY);
                    tileX = static_cast<u16>((otherX >> MapHeader::TileShift) + offsetX);
                    const std::size_t cell = tileX + static_cast<std::size_t>(state.map.width) * tileY;
                    const std::vector<u32>& blocks = state.map.blocks;
                    const u32 here = cell < blocks.size() ? blocks[cell] : MapHeader::Outside;
                    const u32 next = cell + 1 < blocks.size() ? blocks[cell + 1] : MapHeader::Outside;

                    if ((here & MapHeader::Wall) != 0 || (next & MapHeader::Wall) != 0)
                    {
                        tileX = static_cast<u16>(otherX >> MapHeader::TileShift);
                        tileY = static_cast<u16>(otherY >> MapHeader::TileShift);
                    }
                }

                this->FindPath(index, tileX, tileY);
                advance(CommandWords(1));

                if (command == PathCommand::FollowBeside)
                {
                    advance(CommandWords(3) - CommandWords(1));
                }

                const auto own = static_cast<u32>(role.y) * static_cast<u32>(role.y) +
                                 static_cast<u32>(role.x) * static_cast<u32>(role.x);
                const auto theirs = static_cast<u32>(other.x) * static_cast<u32>(other.x) +
                                    static_cast<u32>(other.y) * static_cast<u32>(other.y);
                const double ownRoot = std::sqrt(static_cast<double>(own));
                const auto theirRoot = static_cast<i32>(crt::Ftol(std::sqrt(static_cast<double>(theirs))));
                const auto gap = static_cast<i32>(crt::Ftol(ownRoot - static_cast<double>(theirRoot)));

                if (std::abs(gap) > kFastWalkGap)
                {
                    role.flags |= kFastWalk;
                }

                continue;
            }

            case PathCommand::ShowLabel:
            {
                role.label.clear();
                role.labelColor = operand(1);

                if (static_cast<u32>(role.labelColor) > kLabelColors)
                {
                    role.labelColor = 0;
                }

                std::size_t end = at + kPathWordBytes * CommandWords(1);

                while (end + 1 < data.size() && word(end) != kLabelEnd)
                {
                    ++end;
                }

                role.unk18 += static_cast<u16>((end - at + kPathWordBytes) / kPathWordBytes);
                role.label.assign(
                    data.begin() + static_cast<std::ptrdiff_t>(at + kPathWordBytes * CommandWords(1)),
                    data.begin() + static_cast<std::ptrdiff_t>(end)
                );
                return;
            }

            case PathCommand::ClearLabel:
                role.label.clear();
                role.labelColor = 0;
                advance(CommandWords(0));
                continue;

            default:
                DebugLog(u8"path command", this->RoleId(static_cast<u32>(index)), code);
                return;
        }
    }
}

// swd32.exe 0x0042D920: a held role (flag bit 31) leaving an event: its scripted move takes the move queued after it
// (a new path from where the role stands) or, with none queued, is freed. False when it has no scripted move.
bool RolePath::ResumeMove(int index)
{
    GameState& state = this->game.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];

    if (static_cast<i32>(role.flags) >= 0)
    {
        return true;
    }

    int slot = 0;

    while (state.moves[static_cast<std::size_t>(slot)].role != index ||
           state.moves[static_cast<std::size_t>(slot)].Kind() <= MoveKind::Path)
    {
        if (++slot >= RolePath::Moves)
        {
            return false;
        }
    }

    MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

    if (move.nextRole == kFree)
    {
        if (move.Kind() != MoveKind::Path)
        {
            move.Clear();
        }

        return true;
    }

    const BlockingMap grid = this->Grid();
    PathFinder finder(this->game.pathNodes, grid, this->game.errors);
    move.role = move.nextRole;
    move.x = move.nextX;
    move.y = move.nextY;
    move.step = 0;
    move.SetKind(MoveKind::Path);

    if (finder.FindPath(role.x, role.y, move.x, move.y, role.act.width, role.act.height, move.directions) != 1)
    {
        DebugLog(
            u8"resume path not found",
            index,
            role.x >> MapHeader::TileShift,
            role.y >> MapHeader::TileShift,
            move.x / MapHeader::TileSize,
            move.y / MapHeader::TileSize
        );
    }

    return true;
}

// The controlled role brought back onto its tile against its step (without a step the original loops for ever).
void RolePath::AlignPlayer(Role& role)
{
    Game& owner = this->game;
    const int stepX = owner.unk4A9488 + owner.unk4B7518;
    const int stepY = owner.unk4A948C + owner.unk4B7920;

    if (((role.x & MapHeader::TileFraction) != 0 && stepX == 0) ||
        ((role.y & MapHeader::TileFraction) != 0 && stepY == 0))
    {
        Hang();
    }

    while ((role.x & MapHeader::TileFraction) != 0)
    {
        role.x -= stepX;
    }

    while ((role.y & MapHeader::TileFraction) != 0)
    {
        role.y -= stepY;
    }
}

// swd32.exe 0x0042E5A0: role `index` held for an event (flag bit 31): the controlled role is put back on its tile and
// stops; a path move is queued again from where it is (its target kept), a role between tiles goes back to its last one
int RolePath::HoldRole(int index)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    const auto leader = static_cast<u16>(owner.unk4B7BC4);

    if (index == state.playerRole)
    {
        const int row = static_cast<int>(static_cast<u32>(role.y) >> MapHeader::TileShift) - 1;
        this->AlignPlayer(role);
        owner.unk4B7920 = 0;
        owner.unk4B7518 = 0;
        owner.unk4A948C = 0;
        owner.unk4A9488 = 0;
        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), row, 0);
    }

    MoveRecord* move = nullptr;

    for (MoveRecord& candidate : state.moves)
    {
        if (candidate.role == index)
        {
            move = &candidate;
            break;
        }
    }

    if (move != nullptr)
    {
        if (move->Kind() == MoveKind::Path)
        {
            move->step |= MoveRecord::Done;
            move->nextRole = move->role;
            move->nextX = move->x;
            move->nextY = move->y;
        }

        if ((role.x & MapHeader::TileFraction) != 0 || (role.y & MapHeader::TileFraction) != 0)
        {
            owner.field.ClearRoleCells(role, leader);

            if (index != state.playerRole)
            {
                const int row = static_cast<int>(static_cast<u32>(role.y) >> MapHeader::TileShift) - 1;
                const u8 back = move->CurrentDirection();

                while ((role.x & MapHeader::TileFraction) != 0 && back < RolePath::Directions)
                {
                    role.x -= RolePath::StepX[back];
                }

                while ((role.y & MapHeader::TileFraction) != 0 && back < RolePath::Directions)
                {
                    role.y -= RolePath::StepY[back];
                }

                owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), row, 0);
            }
        }
    }

    role.flags |= Role::Held;
    return 1;
}

// swd32.exe 0x0042E280: the next step of role `index` along its scripted move: at the path's end the role stops (2;
// the controlled role also stops the keys and recentres); otherwise the step is taken unless blocked (walls ignored
// with flag 0x40000) (1). 0 when it has no scripted move.
int RolePath::ScriptStep(int index)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    MoveRecord* found = nullptr;

    for (MoveRecord& move : state.moves)
    {
        if (move.role == index && move.Kind() == MoveKind::Scripted)
        {
            found = &move;
            break;
        }
    }

    if (found == nullptr)
    {
        return RolePath::NoScriptedMove;
    }

    MoveRecord& move = *found;
    const u8 direction = move.CurrentDirection();

    if (direction == kPathEnd)
    {
        if (role.x != move.x || role.y != move.y)
        {
            DebugLog(u8"script move ends off its target", role.id, role.x, role.y, move.x, move.y);
        }

        role.act.speed = 0;
        role.flags &= ~(Role::Stepping | Role::FastMove);

        if (index == state.playerRole)
        {
            owner.unk4B7920 = 0;
            owner.controls.buttons = {};
            owner.unk4B7518 = 0;
            owner.unk4A948C = 0;
            owner.unk4A9488 = 0;

            if ((owner.scene.screenFlags & Scene::CameraHeld) == 0)
            {
                owner.field.CenterCamera();
            }
        }

        return RolePath::PathEnded;
    }

    u32 probe = 0;

    if ((role.flags & Role::PassWalls) == 0)
    {
        const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
        probe = static_cast<u8>(owner.field.ProbeCells(cell, role.act.width, role.act.height, MapHeader::Wall));
    }

    move.step |= MoveRecord::Done;
    ++move.unk1A;

    if (direction < RolePath::Directions && (probe & RolePath::BlockedBy[direction]) == 0)
    {
        move.step &= static_cast<u16>(~MoveRecord::Done);
        move.stepX = static_cast<i16>(RolePath::StepX[direction]);
        move.stepY = static_cast<i16>(RolePath::StepY[direction]);
        move.unk1A = 0;
    }

    role.flags |= Role::Stepping;
    return RolePath::Stepped;
}

// swd32.exe 0x0042DAF0: role `index` sent to (x, y) by the script, taking the act, action and direction given there
// (0xFFFF keeps them): the controlled role first stops; a path move keeps its target as the next move; with option
// bits 0..3 or no path the role is put there at once, otherwise a scripted move (mode 2) follows a new path, its steps
// off the camera skipped. Option 0x8000 lets the event go on meanwhile. 0 when there is no free move or the target is
// past the map.
int RolePath::MoveRoleTo(int index, u16 x, u16 y, u32 options, i16 act, i16 action, i16 direction)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    const auto leader = static_cast<u16>(owner.unk4B7BC4);
    const BlockingMap grid = this->Grid();
    PathFinder finder(owner.pathNodes, grid, owner.errors);
    role.flags &= ~Role::Arrived;

    auto placeCell = [&state](Role& moved)
    {
        const std::size_t at =
            (static_cast<u32>(moved.x) >> MapHeader::TileShift) +
            static_cast<std::size_t>(state.map.width) * (static_cast<u32>(moved.y) >> MapHeader::TileShift);
        moved.cell = at < state.map.blocks.size() ? &state.map.blocks[at] : nullptr;
    };

    auto stopKeys = [&owner]()
    {
        owner.unk4B7920 = 0;
        owner.controls.buttons = {};
        owner.unk4B7518 = 0;
        owner.unk4A948C = 0;
        owner.unk4A9488 = 0;
    };

    if (index == state.playerRole)
    {
        const int row = static_cast<int>(static_cast<u32>(role.y) >> MapHeader::TileShift) - 1;
        this->AlignPlayer(role);
        stopKeys();
        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), row, 0);
    }

    int slot = 0;

    while (slot < RolePath::Moves && state.moves[static_cast<std::size_t>(slot)].role != index)
    {
        ++slot;
    }

    if (slot < RolePath::Moves)
    {
        MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

        if (move.Kind() == MoveKind::Path)
        {
            move.act = act;
            move.action = action;
            move.direction = direction;
            move.nextRole = move.role;
            move.nextX = move.x;
            move.nextY = move.y;

            if ((options & RolePath::EventGoesOn) != 0)
            {
                move.mode &= static_cast<u8>(~MoveRecord::HoldAtEnd);
            }
        }

        if ((role.x & MapHeader::TileFraction) != 0 || (role.y & MapHeader::TileFraction) != 0)
        {
            owner.field.ClearRoleCells(role, leader);

            if (index != state.playerRole)
            {
                const int row = static_cast<int>(static_cast<u32>(role.y) >> MapHeader::TileShift) - 1;
                const u8 back = move.CurrentDirection();

                while ((role.x & MapHeader::TileFraction) != 0 && back < static_cast<int>(RolePath::StepX.size()))
                {
                    role.x -= RolePath::StepX[back];
                }

                while ((role.y & MapHeader::TileFraction) != 0 && back < static_cast<int>(RolePath::StepX.size()))
                {
                    role.y -= RolePath::StepY[back];
                }

                owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), row, 0);
            }
        }
    }

    if ((options & RolePath::PlaceMask) == 0 && slot == RolePath::Moves)
    {
        slot = 0;

        while (slot < RolePath::Moves && state.moves[static_cast<std::size_t>(slot)].role != MoveRecord::NoRole)
        {
            ++slot;
        }
    }

    if (slot == RolePath::Moves)
    {
        if (state.GameFlag(GameState::DebugReports) && (options & RolePath::PlaceMask) == 0)
        {
            DebugLog(u8"no free move", options, this->RoleId(static_cast<u32>(index)));
        }

        owner.field.ClearRoleCells(role, leader);
        role.x = x;
        role.y = y;
        placeCell(role);
        owner.field.MarkRoleCells(role, leader);
        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);

        if (index == state.playerRole)
        {
            stopKeys();

            if ((owner.scene.screenFlags & Scene::CameraHeld) == 0)
            {
                owner.field.CenterCamera();
            }
        }

        return 0;
    }

    MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];
    move.act = act;
    move.action = action;
    move.direction = direction;
    move.role = static_cast<u16>(index);
    move.step = 0;
    move.x = x;
    move.y = y;
    move.SetKind(MoveKind::Scripted);

    if ((options & RolePath::EventGoesOn) != 0)
    {
        move.mode &= static_cast<u8>(~MoveRecord::HoldAtEnd);
    }

    if (x > MapHeader::TileSize * static_cast<u32>(state.map.width) ||
        y > MapHeader::TileSize * static_cast<u32>(state.map.height))
    {
        return 0;
    }

    auto putThere = [&]()
    {
        move.stepX = 0;
        move.stepY = 0;
        role.act.speed = 0;
        role.x = x;
        role.y = y;
        owner.field.ClearRoleCells(role, leader);
        placeCell(role);
        owner.field.MarkRoleCells(role, leader);

        if (index == state.playerRole && (owner.scene.screenFlags & Scene::CameraHeld) == 0)
        {
            owner.field.CenterCamera();
        }

        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
    };

    if ((options & RolePath::PlaceMask) != 0)
    {
        putThere();
        role.flags |= Role::Held;
        return 1;
    }

    if ((role.flags & Role::PassWalls) != 0)
    {
        finder.SetMask(0);
    }

    if (finder.FindPath(role.x, role.y, x, y, role.act.width, role.act.height, move.directions) != 1)
    {
        DebugLog(
            u8"script path not found",
            slot,
            this->RoleId(static_cast<u32>(index)),
            role.x >> MapHeader::TileShift,
            role.y >> MapHeader::TileShift,
            x >> MapHeader::TileShift,
            y >> MapHeader::TileShift
        );
        putThere();
        role.flags |= Role::Held;
        return 1;
    }

    owner.field.ClearRoleCells(role, leader);
    const Field& field = owner.field;
    const auto bottom = static_cast<u32>(field.cameraBottom + kFarMargin);
    const auto right = static_cast<u32>(field.cameraRight + kFarMargin);
    const u32 top = field.cameraTop - kNearMargin < 0 ? 0u : static_cast<u32>(field.cameraTop - kNearMargin);
    const u32 left = field.cameraLeft - kFarMargin < 0 ? 0u : static_cast<u32>(field.cameraLeft - kFarMargin);
    u8 step = kPathEnd;

    for (;;)
    {
        step = move.directions[static_cast<std::size_t>(move.step) % move.directions.size()];
        const auto atX = static_cast<u32>(role.x);
        const auto atY = static_cast<u32>(role.y);

        if (atX > left && atX < right && atY > top && atY < bottom)
        {
            break;
        }

        if (step == kPathEnd || step >= RolePath::Directions)
        {
            break;
        }

        ++move.step;
        role.x += kStepsPerTile * RolePath::StepX[step];
        role.y += kStepsPerTile * RolePath::StepY[step];
    }

    if (step != kPathEnd && step < RolePath::Directions)
    {
        role.act.direction = RolePath::Facing[step];
    }
    else
    {
        if (move.act != -1)
        {
            role.act.id = move.act;
        }

        if (move.action != -1)
        {
            role.act.action = move.action;
        }

        if (move.direction != -1)
        {
            role.act.direction = move.direction;
        }
    }

    move.step |= MoveRecord::Done;
    placeCell(role);
    owner.field.MarkRoleCells(role, leader);
    owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
    role.flags |= Role::Held;
    return 1;
}

}  // namespace swd3
