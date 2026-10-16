#pragma once

#include <cstdint>

namespace swd3 {

// A* search node (0x60 bytes, pooled in G.pathNodePool).
struct PathNode {
    std::int32_t unknown00;     // +0x00
    std::int32_t x;             // +0x04 pixels
    std::int32_t y;             // +0x08
    std::int32_t tile;          // +0x0C (y >> 4) * map width + (x >> 4)
    std::int32_t direction;     // +0x10 direction from the parent
    std::int32_t unknown14;     // +0x14
    double f;                   // +0x18 g + h
    double h;                   // +0x20 distance to the target
    double g;                   // +0x28 steps from the search origin
    PathNode* poolNext;         // +0x30 free pool / nodes owned by a search
    PathNode* parent;           // +0x34
    PathNode* listNext;         // +0x38 open / closed list
    PathNode* children[8];      // +0x3C
    std::int32_t unknown5C;     // +0x5C
};

static_assert(sizeof(PathNode) == 0x60);

struct PathStackEntry {
    PathNode* node;
    PathStackEntry* next;
};

struct PathStack {
    std::int32_t unknown0;
    PathStackEntry* top;
};

void PathPoolInit();
bool PathPoolRelease();
PathNode* PathPoolAlloc();
int PathPoolFree(PathNode* node);

// A* path finder over the blocking map G.mapBlocking for a role of m_width x m_height tiles (size 0x48).
class PathFinder {
public:
    PathFinder();
    ~PathFinder();
    int FindPath(int fromX, int fromY, int toX, int toY, int width, int height, std::uint8_t* directions);
    int IsUsable();
    static int TileIndex(int x, int y);
    int IsWalkable(int x, int y);
    void Reset();
    PathNode* Search(int fromX, int fromY, int toX, int toY);
    PathNode* PopBest();
    void Expand(PathNode* node, int targetX, int targetY);
    void TryNeighbour(PathNode* parent, int x, int y, int targetX, int targetY, int direction);
    PathNode* FindOpen(int tile);
    PathNode* FindClosed(int tile);
    void InsertOpen(PathNode* node);
    void Propagate(PathNode* node);
    PathStackEntry* Push(PathNode* node);
    PathNode* Pop();
    void UpdateBlocked(int x, int y);
    int SetMask(int mask);

    PathNode* m_open;            // +0x00 sentinel; its poolNext chains every node of the search
    PathNode* m_closed;          // +0x04 sentinel
    PathNode* m_result;          // +0x08 node reached at the search origin
    PathStack* m_stack;          // +0x0C
    std::int32_t m_found;        // +0x10 1 after a successful FindPath, 0 after an impossible one
    std::int32_t unknown14;      // +0x14
    std::int32_t unknown18;      // +0x18
    std::int32_t m_width;        // +0x1C tiles
    std::int32_t m_height;       // +0x20
    std::uint32_t m_mask;        // +0x24 blocking bits
    std::uint32_t m_blocked[8];  // +0x28 N, NE, E, SE, S, SW, W, NW
};

static_assert(sizeof(PathFinder) == 0x48);

}  // namespace swd3
