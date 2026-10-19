#include "role/role_control.h"

#include <array>

#include "game/game.h"

namespace swd3
{

namespace
{

constexpr u32 kNone = 0xFFFFFFFF;
constexpr u32 kSolid = MapHeader::Wall | MapHeader::SolidRole;
constexpr u32 kSlideMask = MapHeader::Wall | MapHeader::SolidRole | MapHeader::Unk800000;

// The tables of a slide (on the stack of 0x004040B0): a straight step has the cell offset ahead (replaced by the
// role's size when it moves right or down) and the probe bits of the side steps; a diagonal one its step and the probe
// bits of the diagonal, the horizontal and the vertical part.
constexpr std::array<i32, 6> kSlideLeft = {
    -1,
    BlockingMap::ProbeUp,
    BlockingMap::ProbeUpLeft,
    BlockingMap::ProbeLeft,
    BlockingMap::ProbeDownLeft,
    BlockingMap::ProbeDown
};

constexpr std::array<i32, 6> kSlideRight = {
    1,
    BlockingMap::ProbeUp,
    BlockingMap::ProbeUpRight,
    BlockingMap::ProbeRight,
    BlockingMap::ProbeDownRight,
    BlockingMap::ProbeDown
};

constexpr std::array<i32, 6> kSlideUp = {
    -1,
    BlockingMap::ProbeLeft,
    BlockingMap::ProbeUpLeft,
    BlockingMap::ProbeUp,
    BlockingMap::ProbeUpRight,
    BlockingMap::ProbeRight
};

constexpr std::array<i32, 6> kSlideDown = {
    1,
    BlockingMap::ProbeLeft,
    BlockingMap::ProbeDownLeft,
    BlockingMap::ProbeDown,
    BlockingMap::ProbeDownRight,
    BlockingMap::ProbeRight
};

constexpr std::array<i32, 4> kSlideUpLeft =
    {-1, BlockingMap::ProbeUpLeft, BlockingMap::ProbeLeft, BlockingMap::ProbeUp};
constexpr std::array<i32, 4> kSlideDownRight =
    {1, BlockingMap::ProbeDownRight, BlockingMap::ProbeRight, BlockingMap::ProbeDown};
constexpr std::array<i32, 4> kSlideDownLeft =
    {-1, BlockingMap::ProbeDownLeft, BlockingMap::ProbeLeft, BlockingMap::ProbeDown};
constexpr std::array<i32, 4> kSlideUpRight =
    {1, BlockingMap::ProbeUpRight, BlockingMap::ProbeRight, BlockingMap::ProbeUp};
constexpr std::size_t kAheadOffset = 0;  // the columns of a straight slide
constexpr std::size_t kFarBackSide = 1;
constexpr std::size_t kNearBackSide = 2;
constexpr std::size_t kStraight = 3;
constexpr std::size_t kNearForwardSide = 4;
constexpr std::size_t kFarForwardSide = 5;
constexpr std::size_t kDiagonalStep = 0;  // the columns of a diagonal slide
constexpr std::size_t kDiagonal = 1;
constexpr std::size_t kAcross = 2;
constexpr std::size_t kUpDown = 3;

// FreeLanes: the lane straight ahead, the lanes one cell back and on, and those further.
constexpr u32 kStraightLane = 0x80;
constexpr u32 kNearBackLane = kStraightLane >> 1;
constexpr u32 kNearForwardLane = kStraightLane << 1;
constexpr u32 kFarBackLanes = kNearBackLane - 1;
constexpr u32 kFarForwardLanes = 0xFE00;
constexpr int kLaneShifts = 2;
constexpr u32 kAllFree = 0xFFFFFFFF;
constexpr int kAlongX = 0;
constexpr int kAlongY = 1;

// A step (stepX, stepY) of -1..1 as one number (StepEvent, FindRoleAhead).
constexpr int kStepRow = 3;
constexpr int kStepCenter = kStepRow + 1;

constexpr int StepCase(int stepX, int stepY)
{
    return kStepRow * stepY + stepX;
}

constexpr int AheadCase(int stepX, int stepY)
{
    return StepCase(stepX, stepY) + kStepCenter;
}

constexpr u32 kLookAhead = 5;  // FindRoleAhead looks this many tiles
constexpr u32 kBothSides = 2;  // a row or column one tile wider on each side

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
                                                                         : MapHeader::Outside;
}

// swd32.exe 0x00404510: which lanes beside a move are free of walls (cell bits 29 and 30): for shifts 0..2 the lane
// of `length` cells (`stride` apart) starting `shift` cells back sets bit 0x80 >> shift, the lane starting `shift`
// cells on sets bit 0x80 << shift (while inside `limit` along the axis)
u32 RoleControl::FreeLanes(int x, int y, int length, int stride, int limit, int axis) const
{
    const int along = axis == kAlongY ? y : x;
    const std::ptrdiff_t start = x + static_cast<std::ptrdiff_t>(y) * this->game.state.map.width;
    std::ptrdiff_t back = start;
    std::ptrdiff_t forward = start;
    int coordinate = along;
    u32 lanes = 0;

    auto lane = [this, length, stride](std::ptrdiff_t from)
    {
        u32 free = kAllFree;

        for (int step = 0; step < length; ++step, from += stride)
        {
            if ((this->Cell(from) & kSolid) != 0)
            {
                free = 0;
            }
        }

        return free;
    };

    for (int shift = 0; shift <= kLaneShifts; ++shift)
    {
        if (coordinate >= 0)
        {
            lanes |= lane(back) & (kStraightLane >> shift);
        }

        if (length + shift + along < limit)
        {
            lanes |= lane(forward) & (kStraightLane << shift);
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
void RoleControl::Slide(const Role& role, int& stepX, int& stepY) const
{
    const GameState& state = this->game.state;
    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;
    const u32 probe = this->game.field.ProbeCells(cell, role.act.width, role.act.height, kSlideMask);

    auto blocked = [probe](i32 bits)
    {
        return (probe & static_cast<u32>(bits)) != 0;
    };

    if (stepX != 0 && stepY == 0)
    {
        std::array<i32, 6> rule = stepX == -1 ? kSlideLeft : kSlideRight;

        if (stepX != -1)
        {
            rule[kAheadOffset] = role.act.width;
        }

        const u32 lanes = this->FreeLanes(
            rule[kAheadOffset] + (role.x >> MapHeader::TileShift),
            role.y >> MapHeader::TileShift,
            role.act.height,
            state.map.width,
            state.map.height,
            kAlongY
        );

        if (lanes == 0)
        {
            stepX = 0;
            return;
        }

        if ((lanes & kStraightLane) != 0 && !blocked(rule[kStraight]))
        {
            return;
        }

        if ((lanes & kNearBackLane) != 0 && !blocked(rule[kNearBackSide]))
        {
            stepY = -1;
            return;
        }

        if ((lanes & kNearForwardLane) != 0 && !blocked(rule[kNearForwardSide]))
        {
            stepY = 1;
            return;
        }

        if ((lanes & kFarBackLanes) != 0 && !blocked(rule[kFarBackSide]))
        {
            stepX = 0;
            stepY = -1;
            return;
        }

        if ((lanes & kFarForwardLanes) != 0 && !blocked(rule[kFarForwardSide]))
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
        std::array<i32, 6> rule = stepY == -1 ? kSlideUp : kSlideDown;

        if (stepY != -1)
        {
            rule[kAheadOffset] = role.act.height;
        }

        const u32 lanes = this->FreeLanes(
            role.x >> MapHeader::TileShift,
            rule[kAheadOffset] + (role.y >> MapHeader::TileShift),
            role.act.width,
            1,
            state.map.width,
            kAlongX
        );

        if (lanes == 0)
        {
            stepY = 0;
            return;
        }

        if ((lanes & kStraightLane) != 0 && !blocked(rule[kStraight]))
        {
            return;
        }

        if ((lanes & kNearBackLane) != 0 && !blocked(rule[kNearBackSide]))
        {
            stepX = -1;
            return;
        }

        if ((lanes & kNearForwardLane) != 0 && !blocked(rule[kNearForwardSide]))
        {
            stepX = 1;
            return;
        }

        if ((lanes & kFarBackLanes) != 0 && !blocked(rule[kFarBackSide]))
        {
            stepY = 0;
            stepX = -1;
            return;
        }

        if ((lanes & kFarForwardLanes) != 0 && !blocked(rule[kFarForwardSide]))
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
        falling ? (stepX == -1 ? kSlideUpLeft : kSlideDownRight) : (stepX == -1 ? kSlideDownLeft : kSlideUpRight);

    if (!blocked(rule[kDiagonal]))
    {
        return;
    }

    if (!blocked(rule[kAcross]))
    {
        stepY = 0;
        stepX = rule[kDiagonalStep];
        return;
    }

    if (!blocked(rule[kUpDown]))
    {
        stepX = 0;
        stepY = falling ? rule[kDiagonalStep] : -rule[kDiagonalStep];
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

    for (u32 index = 1; index < static_cast<u32>(state.roleCount) && index < GameState::Roles; ++index)
    {
        const Role& role = state.roles[index];

        if (role.event == 0 || (role.flags & Role::Visible) == 0)
        {
            continue;
        }

        const u32 left = static_cast<u32>(role.x) >> MapHeader::TileShift;
        const u32 top = static_cast<u32>(role.y) >> MapHeader::TileShift;

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

    if (index != kNone && (this->game.state.roles[index].flags & Role::TouchEvent) == 0)
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
    const u32 x = static_cast<u32>(walker.x) >> MapHeader::TileShift;
    const u32 y = static_cast<u32>(walker.y) >> MapHeader::TileShift;
    role = kNone;
    u32 last = kNone;

    auto region = [this](std::ptrdiff_t at) -> u32
    {
        const std::vector<u32>& blocks = this->game.state.map.blocks;
        return at >= 0 && static_cast<std::size_t>(at) < blocks.size()
                   ? blocks[static_cast<std::size_t>(at)] & MapHeader::RegionMask
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

    switch (StepCase(stepX, stepY))
    {
        case StepCase(-1, -1):
            found = row(cell - (mapWidth + 1), x - 1, y - 1, width, hit);

            if (hit)
            {
                return found;
            }

            return column(cell - (mapWidth + 1), x - 1, y - 1, hit);

        case StepCase(0, -1):
            return row(cell - mapWidth, x, y - 1, width, hit);

        case StepCase(1, -1):
            found = row(cell - mapWidth + 1, x + 1, y - 1, width, hit);

            if (hit)
            {
                return found;
            }

            return column(cell + width - mapWidth, x + static_cast<u32>(width) + 1, y - 1, hit);

        case StepCase(-1, 0):
            return column(cell - 1, x - 1, y, hit);

        case StepCase(1, 0):
            return column(cell + width, x + static_cast<u32>(width), y, hit);

        case StepCase(-1, 1):
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

        case StepCase(0, 1):
            return row(
                cell + static_cast<std::ptrdiff_t>(height) * mapWidth,
                x,
                y + static_cast<u32>(height),
                width,
                hit
            );

        case StepCase(1, 1):
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
    const u32 x = static_cast<u32>(walker.x) >> MapHeader::TileShift;
    const u32 y = static_cast<u32>(walker.y) >> MapHeader::TileShift;
    u32 nearX = kLookAhead;
    u32 nearY = kLookAhead;

    if (x <= kLookAhead)
    {
        nearX = x;
    }

    if (static_cast<u32>(state.map.width) - x <= kLookAhead)
    {
        nearX = static_cast<u32>(state.map.width) - x;
    }

    if (y <= kLookAhead)
    {
        nearY = y;
    }

    if (static_cast<u32>(state.map.height) - y <= kLookAhead)
    {
        nearY = static_cast<u32>(state.map.height) - y;
    }

    u32 found = kNone;

    auto look = [this, &found](u32 tileX, u32 tileY)
    {
        found = this->FindRoleAt(tileX, tileY);
        return found != kNone;
    };

    switch (AheadCase(stepX, stepY))
    {
        case AheadCase(-1, -1):
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

        case AheadCase(0, -1):
            for (u32 far = 1; nearY != 0 && far <= nearY; ++far)
            {
                for (u32 side = 0; side < width + kBothSides; ++side)
                {
                    if (look(side + x - 1, y - far))
                    {
                        return found;
                    }
                }
            }

            break;

        case AheadCase(1, -1):
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

        case AheadCase(-1, 0):
            for (u32 far = 1; far <= nearX; ++far)
            {
                for (u32 side = 0; side < height + kBothSides; ++side)
                {
                    if (look(x - far, side + y - 1))
                    {
                        return found;
                    }
                }
            }

            break;

        case AheadCase(1, 0):
            for (u32 far = 0, column = x + width; nearX != 0 && far < nearX; ++far, ++column)
            {
                for (u32 side = 0; side < height + kBothSides; ++side)
                {
                    if (look(column, side + y - 1))
                    {
                        return found;
                    }
                }
            }

            break;

        case AheadCase(-1, 1):
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

        case AheadCase(0, 1):
            for (u32 far = 0, line = y + height; nearY != 0 && far < nearY; ++far, ++line)
            {
                for (u32 side = 0; side < width + kBothSides; ++side)
                {
                    if (look(side + x - 1, line))
                    {
                        return found;
                    }
                }
            }

            break;

        case AheadCase(1, 1):
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
