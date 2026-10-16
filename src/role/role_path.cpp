#include "role/role_path.h"

#include <cmath>
#include <cstdlib>
#include <span>

#include "core/debug_log.h"
#include "core/fault.h"
#include "core/format.h"
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
            return slot != 0x20;
        }
    }

    Role& role = state.roles[static_cast<std::size_t>(index)];
    const BlockingMap grid = this->Grid();
    PathFinder finder(this->game.pathNodes, grid, this->game.errors);
    const u32 x = 0x10u * tileX;
    const u32 y = 0x10u * tileY;

    if (x > 0x10u * static_cast<u32>(state.map.width) || y > 0x10u * static_cast<u32>(state.map.height))
    {
        DebugLog(u8"path target outside", slot, index, role.x >> 4, role.y >> 4, x >> 4, y >> 4);
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
            role.x >> 4,
            role.y >> 4,
            x >> 4,
            y >> 4
        );
        role.x = static_cast<i32>(x);
        role.y = static_cast<i32>(y);
    }

    return slot != 0x20;
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
        return 2;
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
        return 2;
    }

    const u32 mask = move.unk1A > 8 ? MapHeader::Wall : MapHeader::Wall | MapHeader::SolidRole;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    move.step |= MoveRecord::Done;
    ++move.unk1A;
    move.stepY = 0;
    move.stepX = 0;

    if (direction < 8 && (probe & RolePath::BlockedBy[direction]) == 0)
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
    const std::size_t table = 0x200 + 4 * static_cast<std::size_t>(role.unk1C);
    u32 start = 0;

    if (table + 4 <= data.size())
    {
        start = static_cast<u32>(data[table] | data[table + 1] << 8 | data[table + 2] << 16 | data[table + 3] << 24);
    }

    role.unk18 = static_cast<i32>(static_cast<std::ptrdiff_t>(offset) - static_cast<std::ptrdiff_t>(start)) >> 1;
    return 0x200 + static_cast<std::size_t>(offset);
}

// swd32.exe 0x00406770: a random step of role `index` (four in five frames it rests): a move of one step is taken
// (a new one from a free record when it has none, a scripted move leaves it alone); the direction is new one time in
// ten. False when no record is free.
int RolePath::Wander(int index)
{
    GameState& state = this->game.state;
    Role& role = state.roles[static_cast<std::size_t>(index)];
    this->game.crtRandom.Next();
    const auto chance = static_cast<int>(this->game.random.Below(0x50));
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

    if (role.talkState == Role::TalkWaiting || chance > 0x3C)
    {
        move.step |= MoveRecord::Done;
        role.act.action = 0;
        return 1;
    }

    role.act.action = 8;

    if (chance < 8)
    {
        move.directions[0] = static_cast<u8>(chance);
    }

    const u8 direction = move.directions[0];

    if (direction == kPathEnd)
    {
        return 1;
    }

    const u32 mask = move.unk1A > 8 ? MapHeader::Wall : MapHeader::Wall | MapHeader::SolidRole;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, mask);
    ++move.unk1A;
    move.step = MoveRecord::Done;
    move.stepY = 0;
    move.stepX = 0;

    if (direction < 8 && (probe & RolePath::BlockedBy[direction]) == 0)
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

    if (((role.x | role.y) & 0xF) != 0)
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
        const i32 targetX = view.trailX[2 * follower];
        const i32 targetY = view.trailY[2 * follower];

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
                role.x >> 4,
                role.y,
                role.y >> 4,
                targetX,
                targetX >> 4,
                targetY,
                targetY >> 4
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
            const auto bottom = static_cast<u32>(field.cameraBottom + 0xA0);
            auto top = static_cast<u32>(field.cameraTop - 0x50);
            auto left = static_cast<u32>(field.cameraLeft - 0x50);
            const auto right = static_cast<u32>(field.cameraRight + 0x50);

            if (field.cameraTop - 0x50 < 0)
            {
                top = 0;
            }

            if (field.cameraLeft - 0x50 < 0)
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
                    if (step != kPathEnd && step < 8)
                    {
                        role.act.direction = RolePath::Facing[step];
                    }

                    break;
                }

                if (step == kPathEnd || step >= 8)
                {
                    break;
                }

                ++move.step;
                role.x += 4 * RolePath::StepX[step];
                role.y += 4 * RolePath::StepY[step];
            }

            move.step |= MoveRecord::Done;
            const std::size_t at = (static_cast<u32>(role.x) >> 4) +
                                   static_cast<std::size_t>(state.map.width) * (static_cast<u32>(role.y) >> 4);
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
    else if (step >= 8 || (probe & RolePath::BlockedBy[step]) != 0)
    {
        role.act.action = 0;
    }
    else
    {
        move.step = static_cast<u16>(taken & MoveRecord::StepMask);
        ++move.unk1A;
        role.act.action = 8;
        move.stepX = static_cast<i16>(2 * RolePath::StepX[step]);
        move.stepY = static_cast<i16>(2 * RolePath::StepY[step]);
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

    auto word = [&data](std::size_t at) -> u16
    {
        return at + 1 < data.size() ? static_cast<u16>(data[at] | data[at + 1] << 8) : u16{0};
    };

    auto dword = [&word](std::size_t at) -> u32
    {
        return static_cast<u32>(word(at) | static_cast<u32>(word(at + 2)) << 16);
    };

    std::size_t at =
        0x200 + dword(0x200 + 4 * static_cast<std::size_t>(role.unk1C)) + 2 * static_cast<std::size_t>(role.unk18);
    i32& time = owner.party.clock;

    auto advance = [&role, &at](int words)
    {
        at += 2 * static_cast<std::size_t>(words);
        role.unk18 += words;
    };

    auto jump = [this, index, &at](u32 offset)
    {
        at = this->JumpTo(index, offset);
    };

    for (;;)
    {
        const u16 code = word(at);

        switch (code)
        {
            case 0:
                role.unk18 = 0;
                return;

            case 1:
                role.unk1C = 0;
                role.unk18 = 0;
                return;

            case 2:
                role.act.action = word(at + 2);
                role.unk18 += 2;
                owner.acts.Run(role.act);
                return;

            case 3:
                role.act.direction = word(at + 2);
                role.unk18 += 2;
                owner.acts.Run(role.act);
                return;

            case 4:
                role.unk22 = word(at + 2);
                advance(2);
                continue;

            case 5:
                if (role.unk22 != 0)
                {
                    --role.unk22;
                }
                else
                {
                    ++role.unk18;
                }

                return;

            case 6:
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

            case 7:
                this->FindPath(index, word(at + 2), word(at + 4));
                advance(3);
                continue;

            case 8:
                if (((role.x | role.y) & 0xF) != 0 || this->Step(index) != 2)
                {
                    return;
                }

                advance(1);
                role.flags &= ~kFastWalk;
                continue;

            case 9:
            {
                bool passed = false;

                if (word(at + 2) <= static_cast<u32>(time))
                {
                    advance(2);
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

            case 0xA:
                if (word(at + 2) > static_cast<u16>(time))
                {
                    advance(4);
                    continue;
                }

                jump(dword(at + 4));
                continue;

            case 0xB:
                time = word(at + 2);

                if (static_cast<u32>(time) > 0x3E8)
                {
                    time = 0;
                }

                advance(1);
                continue;

            case 0xC:
                jump(dword(at + 2));
                continue;

            case 0xD:
            case 0xE:
                if ((static_cast<int>(state.GameFlag(word(at + 2))) ^ static_cast<int>(code == 0xE)) == 1)
                {
                    jump(dword(at + 4));
                    continue;
                }

                advance(4);
                continue;

            case 0xF:
            case 0x10:
            {
                int count = 0;
                int set = 0;
                std::size_t flag = at + 2;

                while (word(flag) != 0xFFFF && flag < data.size())
                {
                    if (state.GameFlag(word(flag)))
                    {
                        ++set;
                    }

                    ++count;
                    flag += 2;
                }

                const bool taken = code == 0xF ? count == 0 || count == set : count != 0 && set != 0;

                if (taken)
                {
                    jump(dword(flag + 2));
                    continue;
                }

                advance(count + 4);
                continue;
            }

            case 0x11:
                state.SetGameFlag(word(at + 2));
                advance(2);
                continue;

            case 0x12:
                state.ClearGameFlag(word(at + 2));
                advance(2);
                continue;

            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            {
                const u16 slot = word(at + 2);
                const u16 value = word(at + 4);

                if (slot >= 0x40 || slot >= variables.size())
                {
                    DebugLog(u8"path variable", slot, value);
                    return;
                }

                if (value > 0x3E8)
                {
                    DebugLog(u8"path value", slot, value);
                }

                i32& variable = variables[slot];
                int length = 0;

                switch (code)
                {
                    case 0x13:
                        variable = value;
                        length = 3;
                        break;

                    case 0x14:
                    {
                        const i32 before = variable;
                        variable = value + before;

                        if (static_cast<u32>(value) + static_cast<u32>(before) > 0x3E8)
                        {
                            variable = 0x3E8;
                        }

                        length = 3;
                        break;
                    }

                    case 0x15:
                    {
                        const i32 before = variable;
                        variable = before - value;

                        if (before - value < 0)
                        {
                            variable = 0;
                        }

                        length = 3;
                        break;
                    }

                    default:
                        length = 5;

                        if (code == 0x16 ? variable >= value : variable <= value)
                        {
                            jump(dword(at + 6));
                            length = 0;
                        }

                        break;
                }

                advance(length);
                continue;
            }

            case 0x18:
            {
                u32 target = 0;
                state.FindRoleOrPlayer(word(at + 2), target);

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
                            move.nextRole = 0xFFFF;
                            move.unk0A = 0xFFFF;
                            move.nextX = 0xFFFF;
                            move.nextY = 0xFFFF;
                        }

                        if (move.Kind() == MoveKind::Path)
                        {
                            if (((other.x | other.y) & 0xF) != 0)
                            {
                                const int row = other.y >> 4;
                                owner.field.ClearRoleCells(other, leaderId);
                                const u8 back = move.directions
                                                    [static_cast<std::size_t>(move.step & MoveRecord::StepMask) %
                                                     move.directions.size()];

                                while ((other.x & 0xF) != 0 && back < 8)
                                {
                                    other.x -= RolePath::StepX[back];
                                }

                                while ((other.y & 0xF) != 0 && back < 8)
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

                role.unk18 += 3;
                at += 6;
                const u16 path = word(at - 2);

                if (target < GameState::Roles)
                {
                    Role& other = state.roles[target];
                    other.unk1C = path;
                    other.unk18 = 0;

                    if (target == static_cast<u32>(index))
                    {
                        at = 0x200 + dword(0x200 + 4 * static_cast<std::size_t>(path));
                    }
                }

                continue;
            }

            case 0x19:
                if (static_cast<u32>(time) > static_cast<u32>(word(at + 2) + this->timeMark))
                {
                    jump(dword(at + 4));
                    continue;
                }

                advance(4);
                continue;

            case 0x1A:
                this->timeMark = time;
                advance(1);
                continue;

            case 0x1B:
            {
                u32 target = 0;
                state.FindRoleOrPlayer(word(at + 2), target);

                if (target < GameState::Roles)
                {
                    Role& other = state.roles[target];
                    other.flags &= ~(Role::Visible | Role::Solid);
                    owner.field.ClearRoleCells(other, leaderId);
                }

                advance(2);
                continue;
            }

            case 0x1C:
            {
                u32 target = 0;
                state.FindRoleOrPlayer(word(at + 2), target);

                if (target < GameState::Roles)
                {
                    state.roles[target].flags |= Role::Visible | Role::Solid;
                }

                advance(2);
                continue;
            }

            case 0x1D:
            {
                owner.field.ClearRoleCells(role, leaderId);
                role.x = 0x10 * word(at + 2);
                role.y = 0x10 * word(at + 4);
                const std::size_t cell = (static_cast<u32>(role.x) >> 4) +
                                         static_cast<std::size_t>(state.map.width) * (static_cast<u32>(role.y) >> 4);
                role.cell = cell < state.map.blocks.size() ? &state.map.blocks[cell] : nullptr;
                owner.acts.Run(role.act);
                owner.field.MarkRoleCells(role, leaderId);
                role.unk18 += 3;
                owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
                return;
            }

            case 0x1E:
                if ((role.x & 0xF) == 0 && (role.y & 0xF) == 0)
                {
                    this->Wander(index);
                }

                return;

            case 0x1F:
                if (word(at + 2) != 0xFFFF)
                {
                    role.act.id = word(at + 2);
                }

                if (word(at + 4) != 0xFFFF)
                {
                    role.act.action = word(at + 4);
                }

                if (word(at + 6) != 0xFFFF)
                {
                    role.act.direction = word(at + 6);
                }

                role.unk18 += 4;
                owner.acts.Run(role.act);
                return;

            case 0x20:
                owner.PlaySoundAt(word(at + 2), role.x, role.y);
                role.unk18 += 2;
                return;

            case 0x21:
            case 0x24:
            {
                u32 target = 0;

                if (!state.FindRoleOrPlayer(word(at + 2), target) || target >= GameState::Roles)
                {
                    DebugLog(u8"Path(Follow)", word(at + 2));
                    role.unk18 += 2;
                    return;
                }

                const Role& other = state.roles[target];
                const i32 otherX = other.x;
                const i32 otherY = other.y;

                if (((otherX | otherY) & 0xF) != 0)
                {
                    return;
                }

                u16 tileX = static_cast<u16>(otherX >> 4);
                u16 tileY = static_cast<u16>(otherY >> 4);

                if (code == 0x24)
                {
                    const int ahead = static_cast<i16>(word(at + 4));
                    const int side = static_cast<i16>(word(at + 6));
                    int offsetX = 0;
                    int offsetY = 0;

                    switch (other.act.direction)
                    {
                        case 0:
                            offsetX = ahead;
                            offsetY = -side;
                            break;

                        case 2:
                            offsetX = -side;
                            offsetY = ahead;
                            break;

                        case 3:
                            offsetX = side;
                            offsetY = -ahead;
                            break;

                        case 4:
                            offsetX = side + ahead;
                            offsetY = side - ahead;
                            break;

                        case 5:
                            offsetX = -(side + ahead);
                            offsetY = ahead - side;
                            break;

                        case 6:
                            offsetX = side - ahead;
                            offsetY = -(ahead + side);
                            break;

                        case 7:
                            offsetX = ahead - side;
                            offsetY = ahead + side;
                            break;

                        default:
                            break;
                    }

                    tileY = static_cast<u16>((otherY >> 4) + offsetY);
                    tileX = static_cast<u16>((otherX >> 4) + offsetX);
                    const std::size_t cell = tileX + static_cast<std::size_t>(state.map.width) * tileY;
                    const std::vector<u32>& blocks = state.map.blocks;
                    const u32 here = cell < blocks.size() ? blocks[cell] : MapHeader::Outside;
                    const u32 next = cell + 1 < blocks.size() ? blocks[cell + 1] : MapHeader::Outside;

                    if ((here & MapHeader::Wall) != 0 || (next & MapHeader::Wall) != 0)
                    {
                        tileX = static_cast<u16>(otherX >> 4);
                        tileY = static_cast<u16>(otherY >> 4);
                    }
                }

                this->FindPath(index, tileX, tileY);
                advance(2);

                if (code == 0x24)
                {
                    advance(2);
                }

                const auto own = static_cast<u32>(role.y) * static_cast<u32>(role.y) +
                                 static_cast<u32>(role.x) * static_cast<u32>(role.x);
                const auto theirs = static_cast<u32>(other.x) * static_cast<u32>(other.x) +
                                    static_cast<u32>(other.y) * static_cast<u32>(other.y);
                const double ownRoot = std::sqrt(static_cast<double>(own));
                const auto theirRoot = static_cast<i32>(crt::Ftol(std::sqrt(static_cast<double>(theirs))));
                const auto gap = static_cast<i32>(crt::Ftol(ownRoot - static_cast<double>(theirRoot)));

                if (std::abs(gap) > 0x80)
                {
                    role.flags |= kFastWalk;
                }

                continue;
            }

            case 0x22:
            {
                role.label.clear();
                role.labelColor = word(at + 2);

                if (static_cast<u32>(role.labelColor) > 0xA)
                {
                    role.labelColor = 0;
                }

                std::size_t end = at + 4;

                while (end + 1 < data.size() && word(end) != 0x5125)
                {
                    ++end;
                }

                role.unk18 += static_cast<u16>((end - at + 2) / 2);
                role.label.assign(
                    data.begin() + static_cast<std::ptrdiff_t>(at + 4),
                    data.begin() + static_cast<std::ptrdiff_t>(end)
                );
                return;
            }

            case 0x23:
                role.label.clear();
                role.labelColor = 0;
                advance(1);
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
        DebugLog(u8"resume path not found", index, role.x >> 4, role.y >> 4, move.x / 0x10, move.y / 0x10);
    }

    return true;
}

// The controlled role brought back onto its tile against its step (without a step the original loops for ever).
void RolePath::AlignPlayer(Role& role)
{
    Game& owner = this->game;
    const int stepX = owner.unk4A9488 + owner.unk4B7518;
    const int stepY = owner.unk4A948C + owner.unk4B7920;

    if (((role.x & 0xF) != 0 && stepX == 0) || ((role.y & 0xF) != 0 && stepY == 0))
    {
        Hang();
    }

    while ((role.x & 0xF) != 0)
    {
        role.x -= stepX;
    }

    while ((role.y & 0xF) != 0)
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
        const int row = static_cast<int>(static_cast<u32>(role.y) >> 4) - 1;
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

        if ((role.x & 0xF) != 0 || (role.y & 0xF) != 0)
        {
            owner.field.ClearRoleCells(role, leader);

            if (index != state.playerRole)
            {
                const int row = static_cast<int>(static_cast<u32>(role.y) >> 4) - 1;
                const u8 back = move->CurrentDirection();

                while ((role.x & 0xF) != 0 && back < 8)
                {
                    role.x -= RolePath::StepX[back];
                }

                while ((role.y & 0xF) != 0 && back < 8)
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

    if (direction == 0xFF)
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

            if ((owner.scene.unk4C9A18 & 2) == 0)
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

    if (direction < 8 && (probe & RolePath::BlockedBy[direction]) == 0)
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
        const std::size_t at = (static_cast<u32>(moved.x) >> 4) +
                               static_cast<std::size_t>(state.map.width) * (static_cast<u32>(moved.y) >> 4);
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
        const int row = static_cast<int>(static_cast<u32>(role.y) >> 4) - 1;
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
                const int row = static_cast<int>(static_cast<u32>(role.y) >> 4) - 1;
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

            if ((owner.scene.unk4C9A18 & 2) == 0)
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

    if ((options & 0x8000) != 0)
    {
        move.mode &= static_cast<u8>(~MoveRecord::HoldAtEnd);
    }

    if (x > 0x10u * static_cast<u32>(state.map.width) || y > 0x10u * static_cast<u32>(state.map.height))
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

        if (index == state.playerRole && (owner.scene.unk4C9A18 & 2) == 0)
        {
            owner.field.CenterCamera();
        }

        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
    };

    if ((options & 0xF) != 0)
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
            role.x >> 4,
            role.y >> 4,
            x >> 4,
            y >> 4
        );
        putThere();
        role.flags |= Role::Held;
        return 1;
    }

    owner.field.ClearRoleCells(role, leader);
    const Field& field = owner.field;
    const auto bottom = static_cast<u32>(field.cameraBottom + 0xA0);
    const auto right = static_cast<u32>(field.cameraRight + 0xA0);
    const u32 top = field.cameraTop - 0x50 < 0 ? 0u : static_cast<u32>(field.cameraTop - 0x50);
    const u32 left = field.cameraLeft - 0xA0 < 0 ? 0u : static_cast<u32>(field.cameraLeft - 0xA0);
    u8 step = 0xFF;

    for (;;)
    {
        step = move.directions[static_cast<std::size_t>(move.step) % move.directions.size()];
        const auto atX = static_cast<u32>(role.x);
        const auto atY = static_cast<u32>(role.y);

        if (atX > left && atX < right && atY > top && atY < bottom)
        {
            break;
        }

        if (step == 0xFF || step >= 8)
        {
            break;
        }

        ++move.step;
        role.x += 4 * RolePath::StepX[step];
        role.y += 4 * RolePath::StepY[step];
    }

    if (step != 0xFF && step < 8)
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
