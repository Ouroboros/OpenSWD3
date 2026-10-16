#include "role/path_finder.h"

#include <algorithm>
#include <cmath>

#include "core/debug_log.h"
#include "game/game_state.h"
#include "map/blocking_map.h"
#include "platform/message_box.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

enum Side
{
    North,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
};

constexpr int kTile = MapHeader::TileSize;

// The direction TryNeighbour stores for the neighbour on each side (the order of PathNode::children).
enum Neighbour : int
{
    NorthWestNeighbour,
    NorthNeighbour,
    NorthEastNeighbour,
    EastNeighbour,
    SouthEastNeighbour,
    SouthNeighbour,
    SouthWestNeighbour,
    WestNeighbour,
};

constexpr std::size_t kDeltaOffset = 1;  // DeltaX and DeltaY are indexed by the step plus kDeltaOffset
constexpr int kDeltaCenter = 4;          // DirectionByDelta by DeltaY + DeltaX + kDeltaCenter
constexpr int kCorners = 2;              // an edge runs one tile past the footprint at each end
constexpr int kSmallFootprint = 2;       // footprints up to this size take the unrolled path
constexpr int kSmallEdge = kSmallFootprint + kCorners;
constexpr std::size_t kBefore = 0;  // the cells of a small edge: the corner before, the footprint, the corner after
constexpr std::size_t kFirst = 1;
constexpr std::size_t kAfterOne = 2;  // the corner after a one-tile footprint (its second tile for two)
constexpr std::size_t kAfterTwo = 3;
// "!!! 危險 步數超過510 !!!" and "路徑計算" (swd32.exe 0x0049F520, 0x0049F53C).
constexpr std::u8string_view kOverflowText = u8"!!! 危險 步數超過510 !!!";
constexpr std::u8string_view kOverflowCaption = u8"路徑計算";

void AttachChild(PathNode& parent, PathNode& child)
{
    for (PathNode*& slot : parent.children)
    {
        if (slot == nullptr)
        {
            slot = &child;
            return;
        }
    }
}

double Distance(int dx, int dy)
{
    return std::sqrt(static_cast<double>(dx * dx + dy * dy));
}

}  // namespace

PathNode* PathNodePool::NewNode()
{
    this->storage.push_back(std::make_unique<PathNode>());
    return this->storage.back().get();
}

// swd32.exe 0x00402030: the pool head plus 8 free nodes
void PathNodePool::Init()
{
    this->storage.clear();
    this->head = this->NewNode();
    this->free = 0;

    do
    {
        PathNode* node = this->NewNode();
        node->poolNext = this->head->poolNext;
        this->head->poolNext = node;
        ++this->free;
    } while (this->free < PathNodePool::Initial);

    this->free = PathNodePool::Initial;
    this->peak = PathNodePool::Initial;
    this->unused = 0;
}

// swd32.exe 0x004020C0: frees the pool; true when every node had been returned
bool PathNodePool::Release()
{
    for (PathNode* node = this->head->poolNext; node != nullptr; node = this->head->poolNext)
    {
        this->head->poolNext = node->poolNext;
        --this->free;
    }

    this->storage.clear();
    this->head = nullptr;
    return this->free == 0;
}

// swd32.exe 0x00402120: a cleared node; the pool grows when empty
PathNode* PathNodePool::Alloc()
{
    if (this->head->poolNext == nullptr)
    {
        PathNode* node = this->NewNode();
        node->poolNext = this->head->poolNext;
        this->head->poolNext = node;
        const int count = this->free + 1;
        ++this->free;

        if (this->peak < this->free)
        {
            this->peak = count;
        }
    }

    PathNode* node = this->head->poolNext;
    this->head->poolNext = node->poolNext;
    --this->free;
    *node = PathNode{};
    return node;
}

// swd32.exe 0x004021A0
int PathNodePool::Free(PathNode* node)
{
    if (node == nullptr)
    {
        return 0;
    }

    node->poolNext = this->head->poolNext;
    this->head->poolNext = node;
    return ++this->free;
}

// swd32.exe 0x004021D0
PathFinder::PathFinder(PathNodePool& pool, const BlockingMap& map, ErrorReporter& reporter)
    : pool(pool),
      map(map),
      reporter(reporter)
{
}

// swd32.exe 0x00402200
PathFinder::~PathFinder()
{
    this->Reset();
}

// swd32.exe 0x00402230: the directions from (fromX, fromY) to (toX, toY), ended by 0xFF. A search result is accepted
// only when no path was found before or the path is empty (found is left from the last call). A path of more than
// 510 steps shows a message box per extra step; the original goes on writing past the 512-byte buffer, here the
// directions stop at its end.
int PathFinder::FindPath(int fromX, int fromY, int toX, int toY, int width, int height, std::span<u8> directions)
{
    std::size_t written = 0;

    auto put = [&](u8 direction)
    {
        if (written < directions.size())
        {
            directions[written] = direction;
        }
    };

    put(PathFinder::End);
    this->width = width;
    this->height = height;

    if (fromX == toX && fromY == toY)
    {
        return 1;
    }

    if (this->IsWalkable(toX, toY) == 0 || this->TileIndex(fromX, fromY) == this->TileIndex(toX, toY))
    {
        this->found = 0;
        this->Reset();
        return 0;
    }

    this->Reset();
    this->Search(fromX, fromY, toX, toY);

    if (this->IsUsable() == 0)
    {
        return 0;
    }

    int steps = 0;
    const PathNode* current = this->result;

    for (const PathNode* next = current->parent; next != nullptr; next = next->parent)
    {
        const int dy = (current->y >> MapHeader::TileShift) - (next->y >> MapHeader::TileShift);
        const int dx = (current->x >> MapHeader::TileShift) - (next->x >> MapHeader::TileShift);
        const int index = PathFinder::DeltaY[static_cast<std::size_t>(dy) + kDeltaOffset] +
                          PathFinder::DeltaX[static_cast<std::size_t>(dx) + kDeltaOffset];
        put(static_cast<u8>(PathFinder::DirectionByDelta[static_cast<std::size_t>(index + kDeltaCenter)]));
        ++written;

        if (++steps > PathFinder::MaxSteps)
        {
            this->reporter.ShowError(kOverflowCaption, kOverflowText);
        }

        current = next;
    }

    put(PathFinder::End);
    this->found = 1;
    this->Reset();
    return 1;
}

// swd32.exe 0x00402390
int PathFinder::IsUsable() const
{
    if (this->result == nullptr)
    {
        return 0;
    }

    if (this->found == 0)
    {
        return 1;
    }

    return this->result->parent == nullptr ? 1 : 0;
}

// swd32.exe 0x004023C0
int PathFinder::TileIndex(int x, int y) const
{
    return (x >> MapHeader::TileShift) + this->map.width * (y >> MapHeader::TileShift);
}

// swd32.exe 0x004023E0
int PathFinder::IsWalkable(int x, int y) const
{
    return (this->mask & this->map.Cell(this->TileIndex(x, y))) == 0 ? 1 : 0;
}

// swd32.exe 0x00402410: returns every node of the search (chained through the open sentinel) to the pool
void PathFinder::Reset()
{
    if (this->open == nullptr)
    {
        return;
    }

    while (PathNode* node = this->open->poolNext)
    {
        this->open->poolNext = node->poolNext;
        this->pool.Free(node);
    }

    this->pool.Free(this->open);
    this->open = nullptr;
}

// swd32.exe 0x00402450: searches from the target back to the origin, so the parents of the result lead from
// (fromX, fromY) to (toX, toY)
PathNode* PathFinder::Search(int fromX, int fromY, int toX, int toY)
{
    const int originTile = this->TileIndex(fromX, fromY);
    this->open = this->pool.Alloc();
    this->closed = this->pool.Alloc();
    PathNode* target = this->pool.Alloc();
    this->open->poolNext = this->closed;
    this->closed->poolNext = target;
    target->g = 0.0;
    target->poolNext = nullptr;
    const double h = Distance(toX - fromX, toY - fromY);
    target->h = h;
    target->f = h;
    target->tile = this->TileIndex(toX, toY);
    target->x = toX;
    target->y = toY;
    this->open->listNext = target;
    PathNode* node = this->PopBest();

    while (node != nullptr)
    {
        if (node->tile == originTile)
        {
            break;
        }

        this->Expand(*node, fromX, fromY);
        node = this->PopBest();
    }

    this->result = node;
    return node;
}

// swd32.exe 0x00402520
PathNode* PathFinder::PopBest()
{
    PathNode* node = this->open->listNext;

    if (node == nullptr)
    {
        DebugLog(u8"aStar ,Path find fail");
        return nullptr;
    }

    this->open->listNext = node->listNext;
    node->listNext = this->closed->listNext;
    this->closed->listNext = node;
    return node;
}

// swd32.exe 0x00402560
void PathFinder::Expand(PathNode& node, int targetX, int targetY)
{
    this->UpdateBlocked(node.x, node.y);
    const int x = node.x;
    const int y = node.y;

    if (this->blocked[NorthWest] == 0)
    {
        this->TryNeighbour(node, x - kTile, y - kTile, targetX, targetY, NorthWestNeighbour);
    }

    if (this->blocked[North] == 0)
    {
        this->TryNeighbour(node, node.x, node.y - kTile, targetX, targetY, NorthNeighbour);
    }

    if (this->blocked[NorthEast] == 0)
    {
        this->TryNeighbour(node, node.x + kTile, node.y - kTile, targetX, targetY, NorthEastNeighbour);
    }

    if (this->blocked[East] == 0)
    {
        this->TryNeighbour(node, node.x + kTile, node.y, targetX, targetY, EastNeighbour);
    }

    if (this->blocked[SouthEast] == 0)
    {
        this->TryNeighbour(node, node.x + kTile, node.y + kTile, targetX, targetY, SouthEastNeighbour);
    }

    if (this->blocked[South] == 0)
    {
        this->TryNeighbour(node, node.x, node.y + kTile, targetX, targetY, SouthNeighbour);
    }

    if (this->blocked[SouthWest] == 0)
    {
        this->TryNeighbour(node, node.x - kTile, node.y + kTile, targetX, targetY, SouthWestNeighbour);
    }

    if (this->blocked[West] == 0)
    {
        this->TryNeighbour(node, node.x - kTile, node.y, targetX, targetY, WestNeighbour);
    }
}

// swd32.exe 0x00402690: an improved open node keeps its place in the open list; for a closed node the direction is
// written to the parent's first child instead of the node
void PathFinder::TryNeighbour(PathNode& parent, int x, int y, int targetX, int targetY, int direction)
{
    const double g = parent.g + 1.0;
    const int tile = this->TileIndex(x, y);

    if (PathNode* openNode = this->FindOpen(tile))
    {
        AttachChild(parent, *openNode);

        if (g < openNode->g)
        {
            const double f = g + openNode->h;
            openNode->g = g;
            openNode->parent = &parent;
            openNode->f = f;
        }

        return;
    }

    if (PathNode* closedNode = this->FindClosed(tile))
    {
        AttachChild(parent, *closedNode);

        if (g < closedNode->g)
        {
            const double f = g + closedNode->h;
            closedNode->g = g;
            closedNode->parent = &parent;
            closedNode->f = f;
            this->Propagate(*closedNode);
        }

        parent.children.front()->direction = PathFinder::OppositeDirection[static_cast<std::size_t>(direction)];
        return;
    }

    PathNode* node = this->pool.Alloc();
    node->poolNext = this->open->poolNext;
    this->open->poolNext = node;
    node->g = g;
    node->parent = &parent;
    node->x = x;
    node->tile = tile;
    const double h = Distance(x - targetX, y - targetY);
    node->y = y;
    node->direction = direction;
    node->h = h;
    node->f = h + g;
    this->InsertOpen(*node);
    AttachChild(parent, *node);
}

// swd32.exe 0x00402940
PathNode* PathFinder::FindOpen(int tile) const
{
    for (PathNode* node = this->open->listNext; node != nullptr; node = node->listNext)
    {
        if (node->tile == tile)
        {
            return node;
        }
    }

    return nullptr;
}

// swd32.exe 0x00402960
PathNode* PathFinder::FindClosed(int tile) const
{
    for (PathNode* node = this->closed->listNext; node != nullptr; node = node->listNext)
    {
        if (node->tile == tile)
        {
            return node;
        }
    }

    return nullptr;
}

// swd32.exe 0x00402980: before the first node whose f is not smaller
void PathFinder::InsertOpen(PathNode& node)
{
    PathNode* previous = this->open;
    PathNode* current = this->open->listNext;

    if (current == nullptr)
    {
        this->open->listNext = &node;
        return;
    }

    while (current != nullptr && !(node.f <= current->f))
    {
        previous = current;
        current = current->listNext;
    }

    node.listNext = current;
    previous->listNext = &node;
}

// swd32.exe 0x004029C0: pushes improved costs down to the children of `node`
void PathFinder::Propagate(PathNode& node)
{
    auto relax = [this](PathNode& parent)
    {
        for (PathNode* child : parent.children)
        {
            if (child == nullptr)
            {
                break;
            }

            const double g = parent.g + 1.0;

            if (g < child->g)
            {
                child->g = g;
                const double f = g + child->h;
                child->parent = &parent;
                child->f = f;
                this->Push(*child);
            }
        }
    };

    relax(node);

    while (!this->stack.empty())
    {
        relax(*this->Pop());
    }
}

// swd32.exe 0x00402A90
void PathFinder::Push(PathNode& node)
{
    this->stack.push_back(&node);
}

// swd32.exe 0x00402AC0
PathNode* PathFinder::Pop()
{
    PathNode* node = this->stack.back();
    this->stack.pop_back();
    return node;
}

// swd32.exe 0x00402AE0: the blocking bits around the footprint at (x, y): the rows above and below and the columns
// left and right, each footprint + 2 tiles long, merged per direction. Footprints of at most 2 tiles take an unrolled
// path that treats sizes below 1 like 1. The original keeps the four edges in 8-entry stack arrays that overlap for
// footprints above 6 tiles; here they are separate.
void PathFinder::UpdateBlocked(int x, int y)
{
    this->blocked.fill(0);
    const int mapWidth = this->map.width;
    const int tx = x >> MapHeader::TileShift;
    const int above = (y - kTile) >> MapHeader::TileShift;
    const int w = this->width;
    const int h = this->height;
    const int topBase = above * mapWidth + tx - 1;
    const int bottomBase = ((y >> MapHeader::TileShift) + h) * mapWidth + tx - 1;
    const std::size_t horizontal = static_cast<std::size_t>(std::max(w + kCorners, kSmallEdge));
    std::vector<u32> top(horizontal, 0);
    std::vector<u32> bottom(horizontal, 0);

    if (w <= kSmallFootprint)
    {
        for (int i = 0; i < static_cast<int>(kAfterTwo); ++i)
        {
            top[static_cast<std::size_t>(i)] |= this->map.Cell(topBase + i) & this->mask;
            bottom[static_cast<std::size_t>(i)] |= this->map.Cell(bottomBase + i) & this->mask;
        }

        if (w == kSmallFootprint)
        {
            top[kAfterTwo] |= this->map.Cell(topBase + static_cast<int>(kAfterTwo)) & this->mask;
            bottom[kAfterTwo] |= this->map.Cell(bottomBase + static_cast<int>(kAfterTwo)) & this->mask;
        }

        this->blocked[SouthWest] = bottom[kBefore];
        this->blocked[NorthWest] = top[kBefore];
        this->blocked[North] = top[kFirst];
        this->blocked[NorthEast] = top[kAfterOne];
        this->blocked[South] = bottom[kFirst];
        this->blocked[SouthEast] = bottom[kAfterOne];

        if (w == kSmallFootprint)
        {
            this->blocked[NorthWest] = top[kBefore] | top[kFirst];
            this->blocked[North] = top[kFirst] | top[kAfterOne];
            this->blocked[NorthEast] = top[kAfterOne] | top[kAfterTwo];
            this->blocked[SouthWest] = bottom[kBefore] | bottom[kFirst];
            this->blocked[South] = bottom[kFirst] | bottom[kAfterOne];
            this->blocked[SouthEast] = bottom[kAfterOne] | bottom[kAfterTwo];
        }
    }
    else
    {
        for (int i = 0; i < w + kCorners; ++i)
        {
            top[static_cast<std::size_t>(i)] |= this->map.Cell(topBase + i) & this->mask;
            bottom[static_cast<std::size_t>(i)] |= this->map.Cell(bottomBase + i) & this->mask;
        }

        for (std::size_t i = 0; i < static_cast<std::size_t>(w); ++i)
        {
            this->blocked[North] |= top[i + kFirst];
            this->blocked[NorthWest] |= top[i];
            this->blocked[NorthEast] |= top[i + kAfterOne];
            this->blocked[South] |= bottom[i + kFirst];
            this->blocked[SouthWest] |= bottom[i];
            this->blocked[SouthEast] |= bottom[i + kAfterOne];
        }
    }

    const int leftBase = above * mapWidth + ((x - kTile) >> MapHeader::TileShift);
    const int rightBase = above * mapWidth + tx + w;
    const std::size_t vertical = static_cast<std::size_t>(std::max(h + kCorners, kSmallEdge));
    std::vector<u32> left(vertical, 0);
    std::vector<u32> right(vertical, 0);

    if (h <= kSmallFootprint)
    {
        for (int i = 0; i < static_cast<int>(kAfterTwo); ++i)
        {
            left[static_cast<std::size_t>(i)] |= this->map.Cell(leftBase + i * mapWidth) & this->mask;
            right[static_cast<std::size_t>(i)] |= this->map.Cell(rightBase + i * mapWidth) & this->mask;
        }

        if (h == kSmallFootprint)
        {
            left[kAfterTwo] |= this->map.Cell(leftBase + static_cast<int>(kAfterTwo) * mapWidth) & this->mask;
            right[kAfterTwo] |= this->map.Cell(rightBase + static_cast<int>(kAfterTwo) * mapWidth) & this->mask;
        }

        this->blocked[NorthWest] |= left[kBefore];
        this->blocked[West] |= left[kFirst];
        this->blocked[SouthWest] |= left[kAfterOne];
        this->blocked[NorthEast] |= right[kBefore];
        this->blocked[East] |= right[kFirst];
        this->blocked[SouthEast] |= right[kAfterOne];

        if (h == kSmallFootprint)
        {
            this->blocked[NorthWest] |= left[kFirst];
            this->blocked[West] |= left[kAfterOne];
            this->blocked[SouthWest] |= left[kAfterTwo];
            this->blocked[NorthEast] |= right[kFirst];
            this->blocked[East] |= right[kAfterOne];
            this->blocked[SouthEast] |= right[kAfterTwo];
        }

        return;
    }

    for (int i = 0; i < h + kCorners; ++i)
    {
        left[static_cast<std::size_t>(i)] |= this->map.Cell(leftBase + i * mapWidth) & this->mask;
        right[static_cast<std::size_t>(i)] |= this->map.Cell(rightBase + i * mapWidth) & this->mask;
    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(h); ++i)
    {
        this->blocked[West] |= left[i + kFirst];
        this->blocked[NorthWest] |= left[i];
        this->blocked[SouthWest] |= left[i + kAfterOne];
        this->blocked[East] |= right[i + kFirst];
        this->blocked[NorthEast] |= right[i];
        this->blocked[SouthEast] |= right[i + kAfterOne];
    }
}

// swd32.exe 0x00402F70
int PathFinder::SetMask(int mask)
{
    this->mask = static_cast<u32>(mask);
    return mask;
}

}  // namespace swd3
