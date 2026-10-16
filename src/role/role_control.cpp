#include "role/role_control.h"

#include <array>

#include "game/game.h"

namespace swd3
{

namespace
{

constexpr u32 kNone = 0xFFFFFFFF;
constexpr u32 kSolid = 0x60000000;
constexpr u32 kSlideMask = 0x60800000;

// The steps of a slide: the step taken, then the probe bits that must be clear (the tables on the stack of
// 0x004040B0).
struct SlideRule
{
    i32 step = 0;
    std::array<u32, 5> bits = {};
};

}  // namespace

RoleControl::RoleControl(Game& game)
    : game(game)
{
}

// A cell of the map's grid; the original reads around the grid, here a cell outside it is solid everywhere.
u32 RoleControl::Cell(std::ptrdiff_t index) const
{
    const std::vector<u32>& blocks = this->game.state.map.blocks;
    return index >= 0 && static_cast<std::size_t>(index) < blocks.size() ? blocks[static_cast<std::size_t>(index)]
                                                                         : 0xFFFFFFFF;
}

// swd32.exe 0x00404510: which lanes beside a move are free of walls (cell bits 29 and 30): for shifts 0..2 the lane
// of `length` cells (`stride` apart) starting `shift` cells back sets bit 0x80 >> shift, the lane starting `shift`
// cells on sets bit 0x80 << shift (while inside `limit` along the axis)
u32 RoleControl::FreeLanes(int x, int y, int length, int stride, int limit, int axis) const
{
    const int along = axis == 1 ? y : x;
    const std::ptrdiff_t start = x + static_cast<std::ptrdiff_t>(y) * this->game.state.map.width;
    std::ptrdiff_t back = start;
    std::ptrdiff_t forward = start;
    int coordinate = along;
    u32 lanes = 0;

    auto lane = [this, length, stride](std::ptrdiff_t from)
    {
        u32 free = 0xFFFFFFFF;

        for (int step = 0; step < length; ++step, from += stride)
        {
            if ((this->Cell(from) & kSolid) != 0)
            {
                free = 0;
            }
        }

        return free;
    };

    for (int shift = 0; shift <= 2; ++shift)
    {
        if (coordinate >= 0)
        {
            lanes |= lane(back) & (0x80u >> shift);
        }

        if (length + shift + along < limit)
        {
            lanes |= lane(forward) & (0x80u << shift);
        }

        back -= stride;
        forward += stride;
        --coordinate;
    }

    return lanes;
}

// swd32.exe 0x004040B0: a step into a wall turned into a slide along it: a straight step blocked ahead moves sideways
// into a free lane (or the diagonal around the corner), a diagonal one keeps the free one of its two parts; with no
// way the step is cleared
void RoleControl::Slide(Role& role, int& stepX, int& stepY) const
{
    const GameState& state = this->game.state;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, kSlideMask);
    std::array<i32, 6> left = {-1, 2, 1, 0x80, 0x40, 0x20};
    std::array<i32, 6> right = {1, 2, 4, 8, 0x10, 0x20};
    std::array<i32, 6> up = {-1, 0x80, 1, 2, 4, 8};
    std::array<i32, 6> down = {1, 0x80, 0x40, 0x20, 0x10, 8};
    constexpr std::array<i32, 4> kUpLeft = {-1, 1, 0x80, 2};
    constexpr std::array<i32, 4> kDownRight = {1, 0x10, 8, 0x20};
    constexpr std::array<i32, 4> kDownLeft = {-1, 0x40, 0x80, 0x20};
    constexpr std::array<i32, 4> kUpRight = {1, 4, 8, 2};

    auto blocked = [probe](i32 bits)
    {
        return (probe & static_cast<u32>(bits)) != 0;
    };

    if (stepX != 0 && stepY == 0)
    {
        std::array<i32, 6>& rule = stepX == -1 ? left : right;

        if (stepX != -1)
        {
            rule[0] = role.act.width;
        }

        const u32 lanes = this->FreeLanes(
            rule[0] + (role.x >> 4),
            role.y >> 4,
            role.act.height,
            state.map.width,
            state.map.height,
            1
        );

        if (lanes == 0)
        {
            stepX = 0;
            return;
        }

        if (static_cast<i8>(lanes) < 0 && !blocked(rule[3]))
        {
            return;
        }

        if ((lanes & 0x40) != 0 && !blocked(rule[2]))
        {
            stepY = -1;
            return;
        }

        if ((lanes & 0x100) != 0 && !blocked(rule[4]))
        {
            stepY = 1;
            return;
        }

        if ((lanes & 0x3F) != 0 && !blocked(rule[1]))
        {
            stepX = 0;
            stepY = -1;
            return;
        }

        if ((lanes & 0xFE00) != 0 && !blocked(rule[5]))
        {
            stepX = 0;
            stepY = 1;
            return;
        }

        stepY = 0;
        stepX = 0;
        return;
    }

    if (stepX == 0 && stepY != 0)
    {
        std::array<i32, 6>& rule = stepY == -1 ? up : down;

        if (stepY != -1)
        {
            rule[0] = role.act.height;
        }

        const u32 lanes = this->FreeLanes(role.x >> 4, rule[0] + (role.y >> 4), role.act.width, 1, state.map.width, 0);

        if (lanes == 0)
        {
            stepY = 0;
            return;
        }

        if (static_cast<i8>(lanes) < 0 && !blocked(rule[3]))
        {
            return;
        }

        if ((lanes & 0x40) != 0 && !blocked(rule[2]))
        {
            stepX = -1;
            return;
        }

        if ((lanes & 0x100) != 0 && !blocked(rule[4]))
        {
            stepX = 1;
            return;
        }

        if ((lanes & 0x3F) != 0 && !blocked(rule[1]))
        {
            stepY = 0;
            stepX = -1;
            return;
        }

        if ((lanes & 0xFE00) != 0 && !blocked(rule[5]))
        {
            stepY = 0;
            stepX = 1;
            return;
        }

        stepY = 0;
        stepX = 0;
        return;
    }

    const bool falling = stepX + stepY != 0;
    const std::array<i32, 4>& rule =
        falling ? (stepX == -1 ? kUpLeft : kDownRight) : (stepX == -1 ? kDownLeft : kUpRight);

    if (!blocked(rule[1]))
    {
        return;
    }

    if (!blocked(rule[2]))
    {
        stepY = 0;
        stepX = rule[0];
        return;
    }

    if (!blocked(rule[3]))
    {
        stepX = 0;
        stepY = falling ? rule[0] : -rule[0];
        return;
    }

    stepY = 0;
    stepX = 0;
}

// swd32.exe 0x00404FD0: the first visible role with an event (after role 0) whose footprint covers tile (x, y)
u32 RoleControl::FindRoleAt(u32 x, u32 y) const
{
    const GameState& state = this->game.state;

    if (static_cast<u32>(state.roleCount) <= 1)
    {
        return kNone;
    }

    for (u32 index = 1; index < static_cast<u32>(state.roleCount) && index < GameState::kRoles; ++index)
    {
        const Role& role = state.roles[index];

        if (role.event == 0 || (role.flags & 0x8000) == 0)
        {
            continue;
        }

        const u32 left = static_cast<u32>(role.x) >> 4;
        const u32 top = static_cast<u32>(role.y) >> 4;

        if (left <= x && left + static_cast<u32>(role.act.width) > x && top <= y &&
            top + static_cast<u32>(role.act.height) > y)
        {
            return index;
        }
    }

    return kNone;
}

// swd32.exe 0x004050B0: 0x00404FD0 limited to roles triggered by touch (flag 0x2000)
u32 RoleControl::FindTouchRoleAt(u32 x, u32 y) const
{
    const u32 index = this->FindRoleAt(x, y);

    if (index != kNone && (this->game.state.roles[index].flags & 0x2000) == 0)
    {
        return kNone;
    }

    return index;
}

// swd32.exe 0x00404610: what a step of role `index` runs into: the first map region (the low byte of a cell) along
// the cells ahead, or a role triggered by touch there (put in `role`, 0 returned). The columns ahead advance by the
// role's width rather than the map's, and a corner step looks at the row ahead and then that column.
u32 RoleControl::StepEvent(int index, int stepX, int stepY, u32& role) const
{
    const GameState& state = this->game.state;
    const Role& walker = state.roles[static_cast<std::size_t>(index)];
    const std::ptrdiff_t cell = walker.cell != nullptr ? walker.cell - state.map.blocks.data() : 0;
    const int mapWidth = state.map.width;
    const int width = walker.act.width;
    const int height = walker.act.height;
    const u32 x = static_cast<u32>(walker.x) >> 4;
    const u32 y = static_cast<u32>(walker.y) >> 4;
    role = kNone;
    u32 last = kNone;

    auto region = [this](std::ptrdiff_t at) -> u32
    {
        const std::vector<u32>& blocks = this->game.state.map.blocks;
        return at >= 0 && static_cast<std::size_t>(at) < blocks.size() ? blocks[static_cast<std::size_t>(at)] & 0xFF
                                                                       : 0;
    };

    auto row = [&](std::ptrdiff_t from, u32 touchX, u32 touchY, int count, bool& hit) -> u32
    {
        for (int step = 0; step < count; ++step)
        {
            last = region(from + step);

            if (last != 0)
            {
                hit = true;
                return last;
            }

            role = this->FindTouchRoleAt(touchX + static_cast<u32>(step), touchY);

            if (role != kNone)
            {
                hit = true;
                return 0;
            }
        }

        return last;
    };

    auto column = [&](std::ptrdiff_t from, u32 touchX, u32 touchY, bool& hit) -> u32
    {
        for (int step = 0; step < height; ++step)
        {
            last = region(from);

            if (last != 0)
            {
                hit = true;
                return last;
            }

            from += width;
            role = this->FindTouchRoleAt(touchX, touchY + static_cast<u32>(step));

            if (role != kNone)
            {
                hit = true;
                return 0;
            }
        }

        return last;
    };

    bool hit = false;
    u32 found = 0;

    switch (3 * stepY + stepX)
    {
        case -4:
            found = row(cell - (mapWidth + 1), x - 1, y - 1, width, hit);

            if (hit)
            {
                return found;
            }

            return column(cell - (mapWidth + 1), x - 1, y - 1, hit);

        case -3:
            return row(cell - mapWidth, x, y - 1, width, hit);

        case -2:
            found = row(cell - mapWidth + 1, x + 1, y - 1, width, hit);

            if (hit)
            {
                return found;
            }

            return column(cell + width - mapWidth, x + static_cast<u32>(width) + 1, y - 1, hit);

        case -1:
            return column(cell - 1, x - 1, y, hit);

        case 1:
            return column(cell + width, x + static_cast<u32>(width), y, hit);

        case 2:
            found =
                row(cell + static_cast<std::ptrdiff_t>(height) * mapWidth - 1,
                    x - 1,
                    y + static_cast<u32>(height),
                    width,
                    hit);

            if (hit)
            {
                return found;
            }

            return column(cell + mapWidth - 1, x - 1, y + 1, hit);

        case 3:
            return row(
                cell + static_cast<std::ptrdiff_t>(height) * mapWidth,
                x,
                y + static_cast<u32>(height),
                width,
                hit
            );

        case 4:
            found =
                row(cell + static_cast<std::ptrdiff_t>(height) * mapWidth + 1,
                    x + 1,
                    y + static_cast<u32>(height),
                    width,
                    hit);

            if (hit)
            {
                return found;
            }

            return column(cell + width, x + static_cast<u32>(width), y + 1, hit);

        default:
            return last;
    }
}

// swd32.exe 0x00404C00: the role with an event within 5 tiles in front of role `index` (fewer near the map's edges),
// by direction (3 * stepY + stepX); the corners and sides look at their own ranges and the role's own footprint is
// looked at last
u32 RoleControl::FindRoleAhead(int index, int stepX, int stepY) const
{
    const GameState& state = this->game.state;
    const Role& walker = state.roles[static_cast<std::size_t>(index)];
    const auto width = static_cast<u32>(walker.act.width);
    const auto height = static_cast<u32>(walker.act.height);
    const u32 x = static_cast<u32>(walker.x) >> 4;
    const u32 y = static_cast<u32>(walker.y) >> 4;
    u32 nearX = 5;
    u32 nearY = 5;

    if (x <= 5)
    {
        nearX = x;
    }

    if (static_cast<u32>(state.map.width) - x <= 5)
    {
        nearX = static_cast<u32>(state.map.width) - x;
    }

    if (y <= 5)
    {
        nearY = y;
    }

    if (static_cast<u32>(state.map.height) - y <= 5)
    {
        nearY = static_cast<u32>(state.map.height) - y;
    }

    u32 found = kNone;

    auto look = [this, &found](u32 tileX, u32 tileY)
    {
        found = this->FindRoleAt(tileX, tileY);
        return found != kNone;
    };

    switch (3 * stepY + stepX + 4)
    {
        case 0:
            for (u32 far = 1; far <= nearY; ++far)
            {
                for (u32 side = 1; side <= nearX; ++side)
                {
                    if (look(x - side, y - far))
                    {
                        return found;
                    }
                }
            }

            break;

        case 1:
            for (u32 far = 1; nearY != 0 && far <= nearY; ++far)
            {
                for (u32 side = 0; side < width + 2; ++side)
                {
                    if (look(side + x - 1, y - far))
                    {
                        return found;
                    }
                }
            }

            break;

        case 2:
            for (u32 far = 1; far <= nearY; ++far)
            {
                for (u32 side = 1; side < nearX; ++side)
                {
                    if (look(side + x, y - far))
                    {
                        return found;
                    }
                }
            }

            break;

        case 3:
            for (u32 far = 1; far <= nearX; ++far)
            {
                for (u32 side = 0; side < height + 2; ++side)
                {
                    if (look(x - far, side + y - 1))
                    {
                        return found;
                    }
                }
            }

            break;

        case 5:
            for (u32 far = 0, column = x + width; nearX != 0 && far < nearX; ++far, ++column)
            {
                for (u32 side = 0; side < height + 2; ++side)
                {
                    if (look(column, side + y - 1))
                    {
                        return found;
                    }
                }
            }

            break;

        case 6:
            for (u32 far = 1; far < nearY; ++far)
            {
                for (u32 side = 1; side <= nearX; ++side)
                {
                    if (look(x - side, far + y))
                    {
                        return found;
                    }
                }
            }

            break;

        case 7:
            for (u32 far = 0, line = y + height; nearY != 0 && far < nearY; ++far, ++line)
            {
                for (u32 side = 0; side < width + 2; ++side)
                {
                    if (look(side + x - 1, line))
                    {
                        return found;
                    }
                }
            }

            break;

        case 8:
            for (u32 far = 1; far < nearY; ++far)
            {
                for (u32 side = 1; side < nearX; ++side)
                {
                    if (look(side + x, far + y))
                    {
                        return found;
                    }
                }
            }

            break;

        default:
            break;
    }

    for (u32 line = 0; line < height; ++line)
    {
        for (u32 side = 0; side < width; ++side)
        {
            if (look(side + x, line + y))
            {
                return found;
            }
        }
    }

    return kNone;
}

}  // namespace swd3
