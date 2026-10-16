#include "role/path_finder.h"

#include <algorithm>
#include <cmath>

#include "core/debug_log.h"
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

constexpr int kTile = 0x10;
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
    } while (this->free < PathNodePool::kInitial);

    this->free = PathNodePool::kInitial;
    this->peak = PathNodePool::kInitial;
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

    put(PathFinder::kEnd);
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
        const int dy = (current->y >> 4) - (next->y >> 4);
        const int dx = (current->x >> 4) - (next->x >> 4);
        const int index = PathFinder::kDeltaY[static_cast<std::size_t>(dy + 1)] +
                          PathFinder::kDeltaX[static_cast<std::size_t>(dx + 1)];
        put(static_cast<u8>(PathFinder::kDirectionByDelta[static_cast<std::size_t>(index + 4)]));
        ++written;

        if (++steps > PathFinder::kMaxSteps)
        {
            this->reporter.ShowError(Big5(kOverflowCaption), Big5(kOverflowText));
        }

        current = next;
    }

    put(PathFinder::kEnd);
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
    return (x >> 4) + this->map.width * (y >> 4);
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
        DebugLog("aStar ,Path find fail");
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
        this->TryNeighbour(node, x - kTile, y - kTile, targetX, targetY, 0);
    }

    if (this->blocked[North] == 0)
    {
        this->TryNeighbour(node, node.x, node.y - kTile, targetX, targetY, 1);
    }

    if (this->blocked[NorthEast] == 0)
    {
        this->TryNeighbour(node, node.x + kTile, node.y - kTile, targetX, targetY, 2);
    }

    if (this->blocked[East] == 0)
    {
        this->TryNeighbour(node, node.x + kTile, node.y, targetX, targetY, 3);
    }

    if (this->blocked[SouthEast] == 0)
    {
        this->TryNeighbour(node, node.x + kTile, node.y + kTile, targetX, targetY, 4);
    }

    if (this->blocked[South] == 0)
    {
        this->TryNeighbour(node, node.x, node.y + kTile, targetX, targetY, 5);
    }

    if (this->blocked[SouthWest] == 0)
    {
        this->TryNeighbour(node, node.x - kTile, node.y + kTile, targetX, targetY, 6);
    }

    if (this->blocked[West] == 0)
    {
        this->TryNeighbour(node, node.x - kTile, node.y, targetX, targetY, 7);
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

        parent.children[0]->direction = PathFinder::kOppositeDirection[static_cast<std::size_t>(direction)];
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
    const int tx = x >> 4;
    const int above = (y - kTile) >> 4;
    const int w = this->width;
    const int h = this->height;
    const int topBase = above * mapWidth + tx - 1;
    const int bottomBase = ((y >> 4) + h) * mapWidth + tx - 1;
    const std::size_t horizontal = static_cast<std::size_t>(std::max(w + 2, 4));
    std::vector<u32> top(horizontal, 0);
    std::vector<u32> bottom(horizontal, 0);

    if (w <= 2)
    {
        for (int i = 0; i < 3; ++i)
        {
            top[static_cast<std::size_t>(i)] |= this->map.Cell(topBase + i) & this->mask;
            bottom[static_cast<std::size_t>(i)] |= this->map.Cell(bottomBase + i) & this->mask;
        }

        if (w == 2)
        {
            top[3] |= this->map.Cell(topBase + 3) & this->mask;
            bottom[3] |= this->map.Cell(bottomBase + 3) & this->mask;
        }

        this->blocked[SouthWest] = bottom[0];
        this->blocked[NorthWest] = top[0];
        this->blocked[North] = top[1];
        this->blocked[NorthEast] = top[2];
        this->blocked[South] = bottom[1];
        this->blocked[SouthEast] = bottom[2];

        if (w == 2)
        {
            this->blocked[NorthWest] = top[0] | top[1];
            this->blocked[North] = top[1] | top[2];
            this->blocked[NorthEast] = top[2] | top[3];
            this->blocked[SouthWest] = bottom[0] | bottom[1];
            this->blocked[South] = bottom[1] | bottom[2];
            this->blocked[SouthEast] = bottom[2] | bottom[3];
        }
    }
    else
    {
        for (int i = 0; i < w + 2; ++i)
        {
            top[static_cast<std::size_t>(i)] |= this->map.Cell(topBase + i) & this->mask;
            bottom[static_cast<std::size_t>(i)] |= this->map.Cell(bottomBase + i) & this->mask;
        }

        for (std::size_t i = 0; i < static_cast<std::size_t>(w); ++i)
        {
            this->blocked[North] |= top[i + 1];
            this->blocked[NorthWest] |= top[i];
            this->blocked[NorthEast] |= top[i + 2];
            this->blocked[South] |= bottom[i + 1];
            this->blocked[SouthWest] |= bottom[i];
            this->blocked[SouthEast] |= bottom[i + 2];
        }
    }

    const int leftBase = above * mapWidth + ((x - kTile) >> 4);
    const int rightBase = above * mapWidth + tx + w;
    const std::size_t vertical = static_cast<std::size_t>(std::max(h + 2, 4));
    std::vector<u32> left(vertical, 0);
    std::vector<u32> right(vertical, 0);

    if (h <= 2)
    {
        for (int i = 0; i < 3; ++i)
        {
            left[static_cast<std::size_t>(i)] |= this->map.Cell(leftBase + i * mapWidth) & this->mask;
            right[static_cast<std::size_t>(i)] |= this->map.Cell(rightBase + i * mapWidth) & this->mask;
        }

        if (h == 2)
        {
            left[3] |= this->map.Cell(leftBase + 3 * mapWidth) & this->mask;
            right[3] |= this->map.Cell(rightBase + 3 * mapWidth) & this->mask;
        }

        this->blocked[NorthWest] |= left[0];
        this->blocked[West] |= left[1];
        this->blocked[SouthWest] |= left[2];
        this->blocked[NorthEast] |= right[0];
        this->blocked[East] |= right[1];
        this->blocked[SouthEast] |= right[2];

        if (h == 2)
        {
            this->blocked[NorthWest] |= left[1];
            this->blocked[West] |= left[2];
            this->blocked[SouthWest] |= left[3];
            this->blocked[NorthEast] |= right[1];
            this->blocked[East] |= right[2];
            this->blocked[SouthEast] |= right[3];
        }

        return;
    }

    for (int i = 0; i < h + 2; ++i)
    {
        left[static_cast<std::size_t>(i)] |= this->map.Cell(leftBase + i * mapWidth) & this->mask;
        right[static_cast<std::size_t>(i)] |= this->map.Cell(rightBase + i * mapWidth) & this->mask;
    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(h); ++i)
    {
        this->blocked[West] |= left[i + 1];
        this->blocked[NorthWest] |= left[i];
        this->blocked[SouthWest] |= left[i + 2];
        this->blocked[East] |= right[i + 1];
        this->blocked[NorthEast] |= right[i];
        this->blocked[SouthEast] |= right[i + 2];
    }
}

// swd32.exe 0x00402F70
int PathFinder::SetMask(int mask)
{
    this->mask = static_cast<u32>(mask);
    return mask;
}

}  // namespace swd3
