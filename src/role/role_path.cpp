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
#include "role/path_script.h"

namespace swd3
{

namespace
{

constexpr u16 kFree = 0xFFFF;
constexpr u8 kPathEnd = 0xFF;
constexpr u32 kBlockedTries = 8;   // after this many blocked steps a role walks through other roles
constexpr int kNearMargin = 0x50;  // a follower or moved role appears this far around the camera
constexpr int kFarMargin = 0xA0;
constexpr int kStepsPerTile = MapHeader::TileSize / 4;  // RolePath::StepX and StepY move 4 pixels
constexpr int kFollowerSpeed = 2;
constexpr u32 kTrailSpacing = 2;   // the trail points between two followers
constexpr u32 kWanderRoll = 0x50;  // a wandering role rolls 0..79: directions below 8, a rest above kRestAbove
constexpr int kRestAbove = 0x3C;
constexpr int kUnkMoveSlot = 0x20;  // FindPath: false only for this slot, which the full table never reaches
constexpr int kFastWalkGap = 0x80;  // a follower further than this from its target walks fast
constexpr u16 kFlagListEnd = 0xFFFF;
constexpr u16 kKeepAct = 0xFFFF;      // SetAct: the act, action or direction left as it is
constexpr u16 kLabelEnd = 0x5125;     // "%Q" ends the text of a label
constexpr u16 kLabelColors = 0xA;     // a label colour past it becomes 0
constexpr i32 kVariableLimit = 1000;  // path variables are capped here

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
        this->game.errors.ShowError(u8"Illegal Role id", Utf8Format(u8"%d", {static_cast<u32>(id)}));
    }

    return id;
}

i32 RolePath::TileStart(i32 place, i32 step)
{
    const i32 fraction = place & MapHeader::TileFraction;

    if (fraction == 0)
    {
        return place;
    }

    if (step == 0)
    {
        Hang();
    }

    return step > 0 ? place - fraction : place + MapHeader::TileSize - fraction;
}

// swd32.exe 0x00405430: every visible role but the first: followers trail the leader, held roles (flag bit 31 or
// talking) and roles without a path keep their cells (a role with flag 0x1000 steps its act unless controlled), the
// others run their path script
void RolePath::Update()
{
    GameState& state = this->game.state;
    const i32 leader = this->game.partyLeaderId;

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

        if (role.pathNumber == 0)
        {
            if ((role.flags & Role::StepsAct) == 0 || index == static_cast<u32>(state.playerRole))
            {
                continue;
            }

            if (this->game.acts.Run(role.act) == 0)
            {
                DebugLog(
                    u8"Act Err(Path:NoPathCycle) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
                    role.id,
                    role.act.id,
                    role.act.action,
                    role.act.direction
                );
            }

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
// swd32.exe 0x00497FD0 (EH unwind: the path finder, freed by its destructor)
// swd32.exe 0x00497FD8 (EH handler of 0x00406390)
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
            u8"路徑系統(piSetpos) 路徑終點超出範圍 >> P(%d) RId(%d) Start(%d/%d) Dest(%d/%d)",
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
            u8"路徑系統(piSetpos) 路徑無法產生 >> P(%d) GUID(%d) Start(%d/%d) Dest(%d/%d)",
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
        DebugLog(
            u8"路徑 路徑沒有設定 >> GUID(%d)\n ACT (%d)\nQQ (%d)\nPATH (%d)",
            this->RoleId(static_cast<u32>(index)),
            role.act.id,
            role.act.action,
            role.pathNumber
        );
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

    const u32 mask = move.blockedTries > kBlockedTries ? MapHeader::Wall : MapHeader::Wall | MapHeader::SolidRole;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    move.step |= MoveRecord::Done;
    ++move.blockedTries;
    move.stepY = 0;
    move.stepX = 0;

    if (direction < RolePath::Directions && (probe & RolePath::BlockedBy[direction]) == 0)
    {
        move.step &= static_cast<u16>(~MoveRecord::Done);
        move.blockedTries = 0;
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
    const std::size_t table = PathScript::TargetBytes * static_cast<std::size_t>(role.pathNumber);
    u32 start = 0;

    if (PathScript::DataAt + table + PathScript::TargetBytes <= data.size())
    {
        start = PathScript::Dword(data, PathScript::DataAt + table);
    }

    role.pathStep = static_cast<i32>(static_cast<std::ptrdiff_t>(offset) - static_cast<std::ptrdiff_t>(start)) >>
                    PathScript::StepShift;
    return PathScript::DataAt + static_cast<std::size_t>(offset);
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

    role.act.action = Role::WalkAction;

    if (chance < RolePath::Directions)
    {
        move.directions[0] = static_cast<u8>(chance);
    }

    const u8 direction = move.directions[0];

    if (direction == kPathEnd)
    {
        return 1;
    }

    const u32 mask = move.blockedTries > kBlockedTries ? MapHeader::Wall : MapHeader::Wall | MapHeader::SolidRole;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    ++move.blockedTries;
    move.step = MoveRecord::Done;
    move.stepY = 0;
    move.stepX = 0;

    if (direction < RolePath::Directions && (probe & RolePath::BlockedBy[direction]) == 0)
    {
        move.step = 0;
        move.blockedTries = 0;
        move.stepX = static_cast<i16>(RolePath::StepX[direction]);
        move.stepY = static_cast<i16>(RolePath::StepY[direction]);
    }

    return 1;
}

// swd32.exe 0x00406960: follower `index` walks after the leader: on a tile it finds a path to its place in the
// leader's trail (unless its move still has steps), skipping the steps off screen at once; then it takes the next step
// at double speed unless blocked (walls are ignored with story flag 0x4F), facing the leader's way at the end
// swd32.exe 0x00497FF0 (EH unwind: the path finder, freed by its destructor)
// swd32.exe 0x00497FF8 (EH handler of 0x00406960)
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

    while (follower < static_cast<u32>(view.followerCount) && follower < view.followers.size() &&
           view.followers[follower] != index)
    {
        ++follower;
    }

    if (follower == static_cast<u32>(view.followerCount) || follower >= view.followerMoves.size())
    {
        DebugLog(u8"CompanyWalk 夥伴紀錄中找不到角色(%d)", role.id);
        return;
    }

    MoveRecord& move = view.followerMoves[follower];
    const i32 leaderId = owner.partyLeaderId;

    auto direction = [&move]()
    {
        return move.CurrentDirection();
    };

    if (move.role == kFree || move.blockedTries != 0 || direction() == kPathEnd)
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
            move.blockedTries = 0;
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
        ++move.blockedTries;
        role.act.action = Role::WalkAction;
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
    const i32 leaderId = owner.partyLeaderId;

    auto word = [&data](std::size_t at)
    {
        return PathScript::Word(data, at);
    };

    auto dword = [&data](std::size_t at)
    {
        return PathScript::Dword(data, at);
    };

    std::size_t at = PathScript::DataAt +
                     dword(PathScript::DataAt + PathScript::TargetBytes * static_cast<std::size_t>(role.pathNumber)) +
                     PathScript::WordBytes * static_cast<std::size_t>(role.pathStep);

    auto operand = [&data, &at](std::size_t index)
    {
        return PathScript::Word(data, at + PathScript::WordBytes * index);
    };

    auto target = [&data, &at](std::size_t index)
    {
        return PathScript::Dword(data, at + PathScript::WordBytes * index);
    };

    i32& time = owner.party.clock;

    auto advance = [&role, &at](int words)
    {
        at += PathScript::WordBytes * static_cast<std::size_t>(words);
        role.pathStep += words;
    };

    auto jump = [this, index, &at](u32 offset)
    {
        at = this->JumpTo(index, offset);
    };

    auto runAct = [&owner, &role](std::u8string_view error)
    {
        if (owner.acts.Run(role.act) == 0)
        {
            DebugLog(error, role.id, role.act.id, role.act.action, role.act.direction);
        }
    };

    for (;;)
    {
        const u16 code = word(at);
        const auto command = static_cast<PathCommand>(code);

        switch (command)
        {
            case PathCommand::Restart:
                role.pathStep = 0;
                return;

            case PathCommand::Stop:
                role.pathNumber = 0;
                role.pathStep = 0;
                return;

            case PathCommand::SetAction:
                role.act.action = operand(1);
                role.pathStep += PathScript::CommandWords(1);
                runAct(u8"Act Err(Path:ChangQQ) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]");
                return;

            case PathCommand::SetDirection:
                role.act.direction = operand(1);
                role.pathStep += PathScript::CommandWords(1);
                runAct(u8"Act Err(Path:chang_dir) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]");
                return;

            case PathCommand::SetCount:
                role.pathWait = operand(1);
                advance(PathScript::CommandWords(1));
                continue;

            case PathCommand::WaitCount:
                if (role.pathWait != 0)
                {
                    --role.pathWait;
                }
                else
                {
                    ++role.pathStep;
                }

                return;

            case PathCommand::WaitCountAnimated:
                if (role.pathWait != 0)
                {
                    --role.pathWait;
                }
                else
                {
                    ++role.pathStep;
                }

                runAct(u8"Act Err(Path:Active) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]");
                return;

            case PathCommand::FindPath:
                this->FindPath(index, operand(1), operand(2));
                advance(PathScript::CommandWords(2));
                continue;

            case PathCommand::Step:
                if (((role.x | role.y) & MapHeader::TileFraction) != 0 || this->Step(index) != RolePath::PathEnded)
                {
                    return;
                }

                advance(PathScript::CommandWords(0));
                role.flags &= ~Role::FastMove;
                continue;

            case PathCommand::WaitClock:
            {
                bool passed = false;

                if (operand(1) <= static_cast<u32>(time))
                {
                    advance(PathScript::CommandWords(1));
                    passed = true;
                }

                if ((role.flags & Role::StepsAct) != 0)
                {
                    runAct(u8"Act Err(Path:WTime) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]");
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
                    advance(PathScript::CommandWords(1, 1));
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

                advance(PathScript::CommandWords(0));
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

                advance(PathScript::CommandWords(1, 1));
                continue;

            case PathCommand::JumpIfAllFlags:
            case PathCommand::JumpIfAnyFlag:
            {
                int count = 0;
                int set = 0;
                std::size_t flag = at + PathScript::WordBytes * PathScript::CommandWords(0);

                while (word(flag) != kFlagListEnd && flag < data.size())
                {
                    if (state.GameFlag(word(flag)))
                    {
                        ++set;
                    }

                    ++count;
                    flag += PathScript::WordBytes;
                }

                const bool taken =
                    command == PathCommand::JumpIfAllFlags ? count == 0 || count == set : count != 0 && set != 0;

                if (taken)
                {
                    jump(dword(flag + PathScript::WordBytes));
                    continue;
                }

                advance(PathScript::CommandWords(count + 1, 1));
                continue;
            }

            case PathCommand::SetFlag:
                state.SetGameFlag(operand(1));
                advance(PathScript::CommandWords(1));
                continue;

            case PathCommand::ClearFlag:
                state.ClearGameFlag(operand(1));
                advance(PathScript::CommandWords(1));
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
                    DebugLog(u8"變數不合法(%d)(%d)\n", slot, value);
                    return;
                }

                if (value > kVariableLimit)
                {
                    DebugLog(u8"數值不合法(%d)(%d)\n", slot, value);
                }

                i32& variable = variables[slot];
                int length = 0;

                switch (command)
                {
                    case PathCommand::SetVariable:
                        variable = value;
                        length = PathScript::CommandWords(2);
                        break;

                    case PathCommand::AddVariable:
                    {
                        const i32 before = variable;
                        variable = value + before;

                        if (static_cast<u32>(value) + static_cast<u32>(before) > kVariableLimit)
                        {
                            variable = kVariableLimit;
                        }

                        length = PathScript::CommandWords(2);
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

                        length = PathScript::CommandWords(2);
                        break;
                    }

                    default:
                        length = PathScript::CommandWords(2, 1);

                        if (command == PathCommand::JumpIfVariableAtLeast ? static_cast<u32>(variable) >= value
                                                                          : static_cast<u32>(variable) <= value)
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

                                if (back < RolePath::Directions)
                                {
                                    other.x = RolePath::TileStart(other.x, RolePath::StepX[back]);
                                    other.y = RolePath::TileStart(other.y, RolePath::StepY[back]);
                                }

                                owner.field
                                    .RemoveRole(other.id, static_cast<int>(other.flags & Role::LayerMask), row, 0);
                            }

                            move.Clear();
                        }

                        break;
                    }
                }

                role.pathStep += PathScript::CommandWords(2);
                at += PathScript::WordBytes * PathScript::CommandWords(2);
                const u16 path = word(at - PathScript::WordBytes);

                if (target < GameState::Roles)
                {
                    Role& other = state.roles[target];
                    other.pathNumber = path;
                    other.pathStep = 0;

                    if (target == static_cast<u32>(index))
                    {
                        at = PathScript::DataAt +
                             dword(PathScript::DataAt + PathScript::TargetBytes * static_cast<std::size_t>(path));
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

                advance(PathScript::CommandWords(1, 1));
                continue;

            case PathCommand::MarkClock:
                this->timeMark = time;
                advance(PathScript::CommandWords(0));
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

                advance(PathScript::CommandWords(1));
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

                advance(PathScript::CommandWords(1));
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
                runAct(u8"Act Err(Path:MoveRole) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]");
                owner.field.MarkRoleCells(role, leaderId);
                role.pathStep += PathScript::CommandWords(2);
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

                role.pathStep += PathScript::CommandWords(3);
                runAct(u8"Act Err(Path:TakeActdata) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]");
                return;

            case PathCommand::PlaySound:
                owner.PlaySoundAt(operand(1), role.x, role.y);
                role.pathStep += PathScript::CommandWords(1);
                return;

            case PathCommand::FollowRole:
            case PathCommand::FollowBeside:
            {
                u32 target = 0;

                if (!state.FindRoleOrPlayer(operand(1), target) || target >= GameState::Roles)
                {
                    DebugLog(u8"Path(Follow) 角色不在地圖上 GUID[%d]", operand(1));
                    role.pathStep += PathScript::CommandWords(1);
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
                advance(PathScript::CommandWords(1));

                if (command == PathCommand::FollowBeside)
                {
                    advance(PathScript::CommandWords(3) - PathScript::CommandWords(1));
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
                    role.flags |= Role::FastMove;
                }

                continue;
            }

            case PathCommand::ShowLabel:
            {
                role.label.reset();
                role.labelColor = operand(1);

                if (static_cast<u32>(role.labelColor) > kLabelColors)
                {
                    role.labelColor = 0;
                }

                std::size_t end = at + PathScript::WordBytes * PathScript::CommandWords(1);

                while (end + 1 < data.size() && word(end) != kLabelEnd)
                {
                    ++end;
                }

                role.pathStep += static_cast<u16>((end - at + PathScript::WordBytes) / PathScript::WordBytes);
                role.label.emplace(
                    data.begin() +
                        static_cast<std::ptrdiff_t>(at + PathScript::WordBytes * PathScript::CommandWords(1)),
                    data.begin() + static_cast<std::ptrdiff_t>(end)
                );
                return;
            }

            case PathCommand::ClearLabel:
                if (role.label.has_value())
                {
                    role.label.reset();
                    role.labelColor = 0;
                }

                advance(PathScript::CommandWords(0));
                continue;

            default:
                DebugLog(u8"路徑系統 無效的指令\nGUID(%d)-(%d)", this->RoleId(static_cast<u32>(index)), code);
                return;
        }
    }
}

// swd32.exe 0x0042D920: a held role (flag bit 31) leaving an event: its scripted move takes the move queued after it
// (a new path from where the role stands) or, with none queued, is freed. False when it has no scripted move.
// swd32.exe 0x004982B0 (EH unwind: the path finder, freed by its destructor)
// swd32.exe 0x004982B8 (EH handler of 0x0042D920)
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
            u8"對話系統(unlock) 路徑無法還原 >> RId(%d)\n Start(%d/%d) Dest(%d/%d)",  // 0x004A1C20 + 0x004A1C34
            index,
            static_cast<u32>(role.x) >> MapHeader::TileShift,
            static_cast<u32>(role.y) >> MapHeader::TileShift,
            move.x / MapHeader::TileSize,
            move.y / MapHeader::TileSize
        );
    }

    return true;
}

// The rebuild (deviations.md): a role let go between tiles on its path walks on with the step it was taking when it was
// held (HoldRole left it there), arriving on its tile as the step would have.
void RolePath::WalkOn(int index)
{
    GameState& state = this->game.state;
    const Role& role = state.roles[static_cast<std::size_t>(index)];

    if (index == state.playerRole || ((role.x | role.y) & MapHeader::TileFraction) == 0)
    {
        return;
    }

    for (MoveRecord& move : state.moves)
    {
        if (move.role == index && move.Kind() == MoveKind::Path)
        {
            move.step &= static_cast<u16>(~MoveRecord::Done);
            return;
        }
    }
}

// The controlled role brought back onto its tile against its step (without a step the original loops for ever).
void RolePath::AlignPlayer(Role& role)
{
    Game& owner = this->game;
    const int stepX = owner.walkStepX + owner.scrollStepX;
    const int stepY = owner.walkStepY + owner.scrollStepY;

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
// stops; a path move is queued again from where it is (its target kept), a role between tiles goes back to its last
// one. The rebuild (deviations.md): a role between tiles on its path stops where it is, its step taken up again when it
// is let go (WalkOn).
int RolePath::HoldRole(int index)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    const i32 leader = owner.partyLeaderId;

    if (index == state.playerRole)
    {
        const int row = static_cast<int>(static_cast<u32>(role.y) >> MapHeader::TileShift) - 1;
        this->AlignPlayer(role);
        owner.scrollStepY = 0;
        owner.scrollStepX = 0;
        owner.walkStepY = 0;
        owner.walkStepX = 0;
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

                if (back < RolePath::Directions && move->Kind() != MoveKind::Path)
                {
                    role.x = RolePath::TileStart(role.x, RolePath::StepX[back]);
                    role.y = RolePath::TileStart(role.y, RolePath::StepY[back]);
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
            DebugLog(
                u8"對話系統(iMove) 沒有到達終點\nGUID(%d)\nrx%d,ry%d\ndx%d,dy%d",
                role.id,
                role.x,
                role.y,
                move.x,
                move.y
            );  // 0x004A1D10 + 0x004A1D20
        }

        role.act.speed = 0;
        role.flags &= ~(Role::Stepping | Role::FastMove);

        if (index == state.playerRole)
        {
            owner.scrollStepY = 0;
            owner.controls.buttons = {};
            owner.scrollStepX = 0;
            owner.walkStepY = 0;
            owner.walkStepX = 0;

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
    ++move.blockedTries;

    if (direction < RolePath::Directions && (probe & RolePath::BlockedBy[direction]) == 0)
    {
        move.step &= static_cast<u16>(~MoveRecord::Done);
        move.stepX = static_cast<i16>(RolePath::StepX[direction]);
        move.stepY = static_cast<i16>(RolePath::StepY[direction]);
        move.blockedTries = 0;
    }

    role.flags |= Role::Stepping;
    return RolePath::Stepped;
}

// swd32.exe 0x0042DAF0: role `index` sent to (x, y) by the script, taking the act, action and direction given there
// (0xFFFF keeps them): the controlled role first stops; a path move keeps its target as the next move; with option
// bits 0..3 or no path the role is put there at once, otherwise a scripted move (mode 2) follows a new path, its steps
// off the camera skipped. Option 0x8000 lets the event go on meanwhile. 0 when there is no free move or the target is
// past the map.
// swd32.exe 0x004982D0 (EH unwind: the path finder, freed by its destructor)
// swd32.exe 0x004982D8 (EH handler of 0x0042DAF0)
int RolePath::MoveRoleTo(int index, u16 x, u16 y, u32 options, i16 act, i16 action, i16 direction)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    const i32 leader = owner.partyLeaderId;
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
        owner.scrollStepY = 0;
        owner.controls.buttons = {};
        owner.scrollStepX = 0;
        owner.walkStepY = 0;
        owner.walkStepX = 0;
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

                if (back < RolePath::Directions)
                {
                    role.x = RolePath::TileStart(role.x, RolePath::StepX[back]);
                    role.y = RolePath::TileStart(role.y, RolePath::StepY[back]);
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
            // The options (pushed last, 0x0042DDA3) fill the GUID of the text; the id (0x0042DDA2) is left over.
            DebugLog(
                u8"警告:對話系統(isetpos:mode0) GUID(%d) 找不到可以使用的路徑\n角色將會直接移到目的地",
                options,
                this->RoleId(static_cast<u32>(index))
            );  // 0x004A1CB8 + 0x004A1CD8
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
            u8"對話系統(isetpos) 路徑無法產生 >> P(%d) crId(%d)\n Start(%d/%d) Dest(%d/%d)",  // 0x004A1C68 + 0x004A1C7C
            slot,
            this->RoleId(static_cast<u32>(index)),
            static_cast<u32>(role.x) >> MapHeader::TileShift,
            static_cast<u32>(role.y) >> MapHeader::TileShift,
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
