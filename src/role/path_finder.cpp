#include "role/path_finder.h"

#include <cmath>
#include <cstring>

#include "core/debug_log.h"
#include "crt/crt.h"
#include "data/globals.h"

namespace swd3 {

namespace {

constexpr int kPoolInitial = 8;
constexpr int kMaxSteps = 0x1FE;

enum Blocked { kN, kNE, kE, kSE, kS, kSW, kW, kNW };

std::uint32_t cell(int index) { return static_cast<std::uint32_t>(G.mapBlocking[index]); }

void attachChild(PathNode* parent, PathNode* child) {
    for (PathNode*& slot : parent->children) {
        if (slot == nullptr) {
            slot = child;
            return;
        }
    }
}

double distance(int dx, int dy) { return std::sqrt(static_cast<double>(dx * dx + dy * dy)); }

}  // namespace

// swd32.exe 0x00402030: pool head plus 8 free nodes
void PathPoolInit() {
    G.pathNodePool = static_cast<PathNode*>(crt::malloc(sizeof(PathNode)));
    std::memset(G.pathNodePool, 0, sizeof(PathNode));
    G.pathNodePool->poolNext = nullptr;
    G.pathPoolFree = 0;
    do {
        auto* node = static_cast<PathNode*>(crt::malloc(sizeof(PathNode)));
        std::memset(node, 0, sizeof(PathNode));
        node->poolNext = G.pathNodePool->poolNext;
        G.pathNodePool->poolNext = node;
        ++G.pathPoolFree;
    } while (G.pathPoolFree < kPoolInitial);
    G.pathPoolFree = kPoolInitial;
    G.pathPoolPeak = kPoolInitial;
    G.pathPoolUnused = 0;
}

// swd32.exe 0x004020C0: frees the pool; true when every node had been returned
bool PathPoolRelease() {
    for (PathNode* node = G.pathNodePool->poolNext; node != nullptr; node = G.pathNodePool->poolNext) {
        G.pathNodePool->poolNext = node->poolNext;
        crt::free(node);
        --G.pathPoolFree;
    }
    crt::free(G.pathNodePool);
    G.pathNodePool = nullptr;
    return G.pathPoolFree == 0;
}

// swd32.exe 0x00402120: zeroed node, growing the pool when empty
PathNode* PathPoolAlloc() {
    if (G.pathNodePool->poolNext == nullptr) {
        auto* node = static_cast<PathNode*>(crt::malloc(sizeof(PathNode)));
        std::memset(node, 0, sizeof(PathNode));
        node->poolNext = G.pathNodePool->poolNext;
        G.pathNodePool->poolNext = node;
        const int count = G.pathPoolFree + 1;
        ++G.pathPoolFree;
        if (G.pathPoolPeak < G.pathPoolFree) {
            G.pathPoolPeak = count;
        }
    }
    PathNode* node = G.pathNodePool->poolNext;
    G.pathNodePool->poolNext = node->poolNext;
    --G.pathPoolFree;
    std::memset(node, 0, sizeof(PathNode));
    return node;
}

// swd32.exe 0x004021A0
int PathPoolFree(PathNode* node) {
    if (node == nullptr) {
        return 0;
    }
    node->poolNext = G.pathNodePool->poolNext;
    G.pathNodePool->poolNext = node;
    return ++G.pathPoolFree;
}

// swd32.exe 0x004021D0
PathFinder::PathFinder() {
    m_stack = static_cast<PathStack*>(crt::malloc(sizeof(PathStack)));
    m_stack->unknown0 = 0;
    m_stack->top = nullptr;
    m_found = 0;
    m_open = nullptr;
    m_closed = nullptr;
    m_result = nullptr;
    m_mask = 0x40800000;
}

// swd32.exe 0x00402200
PathFinder::~PathFinder() {
    Reset();
    if (m_stack != nullptr) {
        crt::free(m_stack);
        m_stack = nullptr;
    }
}

// swd32.exe 0x00402230: writes the directions from (fromX, fromY) to (toX, toY) terminated by 0xFF. The result of
// a search is only accepted when no path was found before or the path is empty (m_found is left from the last
// call); a path longer than 510 steps shows a message box per extra step.
int PathFinder::FindPath(int fromX, int fromY, int toX, int toY, int width, int height, std::uint8_t* directions) {
    *directions = 0xFF;
    m_width = width;
    m_height = height;
    if (fromX == toX && fromY == toY) {
        return 1;
    }
    if (!IsWalkable(toX, toY) || TileIndex(fromX, fromY) == TileIndex(toX, toY)) {
        m_found = 0;
        Reset();
        return 0;
    }
    Reset();
    Search(fromX, fromY, toX, toY);
    if (!IsUsable()) {
        return 0;
    }
    int steps = 0;
    const PathNode* current = m_result;
    for (const PathNode* next = current->parent; next != nullptr; next = next->parent) {
        const int dy = (current->y >> 4) - (next->y >> 4);
        const int dx = (current->x >> 4) - (next->x >> 4);
        const int index = G.pathDeltaY[dy + 1] + G.pathDeltaX[dx + 1];
        *directions++ = static_cast<std::uint8_t>(G.pathDirectionByDelta[index + 4]);
        if (++steps > kMaxSteps) {
            w32::MessageBoxA(G.mainWindow, G.pathOverflowText, G.pathOverflowCaption, w32::MB_OK);
        }
        current = next;
    }
    *directions = 0xFF;
    m_found = 1;
    Reset();
    return 1;
}

// swd32.exe 0x00402390
int PathFinder::IsUsable() {
    if (m_result == nullptr) {
        return 0;
    }
    if (m_found == 0) {
        return 1;
    }
    return m_result->parent == nullptr;
}

// swd32.exe 0x004023C0
int PathFinder::TileIndex(int x, int y) { return (x >> 4) + G.mapWidth * (y >> 4); }

// swd32.exe 0x004023E0
int PathFinder::IsWalkable(int x, int y) {
    return (m_mask & cell((x >> 4) + G.mapWidth * (y >> 4))) == 0;
}

// swd32.exe 0x00402410: returns every node of the search (chained through the open sentinel) to the pool
void PathFinder::Reset() {
    if (m_open == nullptr) {
        return;
    }
    while (PathNode* node = m_open->poolNext) {
        m_open->poolNext = node->poolNext;
        PathPoolFree(node);
    }
    PathPoolFree(m_open);
    m_open = nullptr;
}

// swd32.exe 0x00402450: searches from the target back to the origin, so following the parents of the result
// leads from (fromX, fromY) to (toX, toY)
PathNode* PathFinder::Search(int fromX, int fromY, int toX, int toY) {
    const int originTile = TileIndex(fromX, fromY);
    m_open = PathPoolAlloc();
    m_closed = PathPoolAlloc();
    PathNode* target = PathPoolAlloc();
    m_open->poolNext = m_closed;
    m_closed->poolNext = target;
    target->g = 0.0;
    target->poolNext = nullptr;
    const double h = distance(toX - fromX, toY - fromY);
    target->h = h;
    target->f = h;
    target->tile = TileIndex(toX, toY);
    target->x = toX;
    target->y = toY;
    m_open->listNext = target;
    PathNode* node;
    for (node = PopBest(); node != nullptr; node = PopBest()) {
        if (node->tile == originTile) {
            break;
        }
        Expand(node, fromX, fromY);
    }
    m_result = node;
    return node;
}

// swd32.exe 0x00402520
PathNode* PathFinder::PopBest() {
    PathNode* node = m_open->listNext;
    if (node == nullptr) {
        DebugLog("aStar ,Path find fail");
        return nullptr;
    }
    m_open->listNext = node->listNext;
    node->listNext = m_closed->listNext;
    m_closed->listNext = node;
    return node;
}

// swd32.exe 0x00402560
void PathFinder::Expand(PathNode* node, int targetX, int targetY) {
    UpdateBlocked(node->x, node->y);
    const int x = node->x;
    const int y = node->y;
    if (!m_blocked[kNW]) {
        TryNeighbour(node, x - 0x10, y - 0x10, targetX, targetY, 0);
    }
    if (!m_blocked[kN]) {
        TryNeighbour(node, node->x, node->y - 0x10, targetX, targetY, 1);
    }
    if (!m_blocked[kNE]) {
        TryNeighbour(node, node->x + 0x10, node->y - 0x10, targetX, targetY, 2);
    }
    if (!m_blocked[kE]) {
        TryNeighbour(node, node->x + 0x10, node->y, targetX, targetY, 3);
    }
    if (!m_blocked[kSE]) {
        TryNeighbour(node, node->x + 0x10, node->y + 0x10, targetX, targetY, 4);
    }
    if (!m_blocked[kS]) {
        TryNeighbour(node, node->x, node->y + 0x10, targetX, targetY, 5);
    }
    if (!m_blocked[kSW]) {
        TryNeighbour(node, node->x - 0x10, node->y + 0x10, targetX, targetY, 6);
    }
    if (!m_blocked[kW]) {
        TryNeighbour(node, node->x - 0x10, node->y, targetX, targetY, 7);
    }
}

// swd32.exe 0x00402690: an improved open node keeps its place in the open list; for a closed node the direction
// is written to the parent's first child instead of the node
void PathFinder::TryNeighbour(PathNode* parent, int x, int y, int targetX, int targetY, int direction) {
    const double g = parent->g + 1.0;
    const int tile = TileIndex(x, y);
    if (PathNode* open = FindOpen(tile)) {
        attachChild(parent, open);
        if (g < open->g) {
            const double f = g + open->h;
            open->g = g;
            open->parent = parent;
            open->f = f;
        }
        return;
    }
    if (PathNode* closed = FindClosed(tile)) {
        attachChild(parent, closed);
        if (g < closed->g) {
            const double f = g + closed->h;
            closed->g = g;
            closed->parent = parent;
            closed->f = f;
            Propagate(closed);
        }
        parent->children[0]->direction = G.oppositeDirection[direction];
        return;
    }
    PathNode* node = PathPoolAlloc();
    node->poolNext = m_open->poolNext;
    m_open->poolNext = node;
    node->g = g;
    node->parent = parent;
    node->x = x;
    node->tile = tile;
    const double h = distance(x - targetX, y - targetY);
    node->y = y;
    node->direction = direction;
    node->h = h;
    node->f = h + g;
    InsertOpen(node);
    attachChild(parent, node);
}

// swd32.exe 0x00402940
PathNode* PathFinder::FindOpen(int tile) {
    for (PathNode* node = m_open->listNext; node != nullptr; node = node->listNext) {
        if (node->tile == tile) {
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x00402960
PathNode* PathFinder::FindClosed(int tile) {
    for (PathNode* node = m_closed->listNext; node != nullptr; node = node->listNext) {
        if (node->tile == tile) {
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x00402980: before the first node with an f not smaller
void PathFinder::InsertOpen(PathNode* node) {
    PathNode* previous = m_open;
    PathNode* current = m_open->listNext;
    if (current == nullptr) {
        m_open->listNext = node;
        return;
    }
    while (current != nullptr && !(node->f <= current->f)) {
        previous = current;
        current = current->listNext;
    }
    node->listNext = current;
    previous->listNext = node;
}

// swd32.exe 0x004029C0: pushes improved costs down to the children of `node`
void PathFinder::Propagate(PathNode* node) {
    const double base = node->g;
    for (int i = 0; i < 8; ++i) {
        PathNode* child = node->children[i];
        if (child == nullptr) {
            break;
        }
        const double g = base + 1.0;
        if (g < child->g) {
            child->g = g;
            const double f = g + child->h;
            child->parent = node;
            child->f = f;
            Push(child);
        }
    }
    while (m_stack->top != nullptr) {
        PathNode* parent = Pop();
        for (int i = 0; i < 8; ++i) {
            PathNode* child = parent->children[i];
            if (child == nullptr) {
                break;
            }
            const double g = parent->g + 1.0;
            if (g < child->g) {
                child->g = g;
                const double f = g + child->h;
                child->parent = parent;
                child->f = f;
                Push(child);
            }
        }
    }
}

// swd32.exe 0x00402A90
PathStackEntry* PathFinder::Push(PathNode* node) {
    auto* entry = static_cast<PathStackEntry*>(crt::malloc(sizeof(PathStackEntry)));
    entry->node = nullptr;
    entry->next = nullptr;
    entry->node = node;
    entry->next = m_stack->top;
    m_stack->top = entry;
    return entry;
}

// swd32.exe 0x00402AC0
PathNode* PathFinder::Pop() {
    PathStackEntry* entry = m_stack->top;
    PathNode* node = entry->node;
    m_stack->top = entry->next;
    crt::free(entry);
    return node;
}

// swd32.exe 0x00402AE0: blocking bits around the role footprint at (x, y): the rows above and below and the
// columns left and right, each footprint + 2 cells long, merged per direction. Footprints of at most 2 tiles take
// an unrolled path that also treats sizes below 1 like 1.
void PathFinder::UpdateBlocked(int x, int y) {
    for (std::uint32_t& blocked : m_blocked) {
        blocked = 0;
    }
    std::uint32_t edges[32] = {};
    std::uint32_t* top = edges;
    std::uint32_t* bottom = edges + 8;
    std::uint32_t* left = edges + 16;
    std::uint32_t* right = edges + 24;
    const int width = G.mapWidth;
    const int tx = x >> 4;
    const int above = (y - 0x10) >> 4;
    const int w = m_width;
    const int h = m_height;
    const int topBase = above * width + tx - 1;
    const int bottomBase = ((y >> 4) + h) * width + tx - 1;
    if (w <= 2) {
        const std::uint32_t mask = m_mask;
        for (int i = 0; i < 3; ++i) {
            top[i] |= cell(topBase + i) & mask;
            bottom[i] |= cell(bottomBase + i) & mask;
        }
        if (w == 2) {
            top[3] |= cell(topBase + 3) & mask;
            bottom[3] |= cell(bottomBase + 3) & mask;
        }
        m_blocked[kSW] = bottom[0];
        m_blocked[kNW] = top[0];
        m_blocked[kN] = top[1];
        m_blocked[kNE] = top[2];
        m_blocked[kS] = bottom[1];
        m_blocked[kSE] = bottom[2];
        if (w == 2) {
            m_blocked[kNW] = top[0] | top[1];
            m_blocked[kN] = top[1] | top[2];
            m_blocked[kNE] = top[2] | top[3];
            m_blocked[kSW] = bottom[0] | bottom[1];
            m_blocked[kS] = bottom[1] | bottom[2];
            m_blocked[kSE] = bottom[2] | bottom[3];
        }
    } else {
        if (w + 1 >= 0) {
            for (int i = 0; i < w + 2; ++i) {
                top[i] |= cell(topBase + i) & m_mask;
                bottom[i] |= cell(bottomBase + i) & m_mask;
            }
        }
        for (int i = 0; i < w; ++i) {
            m_blocked[kN] |= top[i + 1];
            m_blocked[kNW] |= top[i];
            m_blocked[kNE] |= top[i + 2];
            m_blocked[kS] |= bottom[i + 1];
            m_blocked[kSW] |= bottom[i];
            m_blocked[kSE] |= bottom[i + 2];
        }
    }
    const int leftBase = above * width + ((x - 0x10) >> 4);
    const int rightBase = above * width + tx + w;
    if (h <= 2) {
        const std::uint32_t mask = m_mask;
        for (int i = 0; i < 3; ++i) {
            left[i] |= cell(leftBase + i * width) & mask;
            right[i] |= cell(rightBase + i * width) & mask;
        }
        if (h == 2) {
            left[3] |= cell(leftBase + 3 * width) & mask;
            right[3] |= cell(rightBase + 3 * width) & mask;
        }
        m_blocked[kNW] |= left[0];
        m_blocked[kW] |= left[1];
        m_blocked[kSW] |= left[2];
        m_blocked[kNE] |= right[0];
        m_blocked[kE] |= right[1];
        m_blocked[kSE] |= right[2];
        if (h == 2) {
            m_blocked[kNW] |= left[1];
            m_blocked[kW] |= left[2];
            m_blocked[kSW] |= left[3];
            m_blocked[kNE] |= right[1];
            m_blocked[kE] |= right[2];
            m_blocked[kSE] |= right[3];
        }
        return;
    }
    if (h + 1 >= 0) {
        for (int i = 0; i < h + 2; ++i) {
            left[i] |= cell(leftBase + i * width) & m_mask;
            right[i] |= cell(rightBase + i * width) & m_mask;
        }
    }
    for (int i = 0; i < h; ++i) {
        m_blocked[kW] |= left[i + 1];
        m_blocked[kNW] |= left[i];
        m_blocked[kSW] |= left[i + 2];
        m_blocked[kE] |= right[i + 1];
        m_blocked[kNE] |= right[i];
        m_blocked[kSE] |= right[i + 2];
    }
}

// swd32.exe 0x00402F70
int PathFinder::SetMask(int mask) {
    m_mask = static_cast<std::uint32_t>(mask);
    return mask;
}

}  // namespace swd3
