#pragma once

#include <array>
#include <memory>
#include <span>
#include <vector>

#include "core/types.h"

namespace swd3
{

class ErrorReporter;
struct BlockingMap;

// An A* search node (0x60 bytes in the original pool).
struct PathNode
{
    i32 unk00 = 0;
    i32 x = 0;          // +0x04 pixels
    i32 y = 0;          // +0x08
    i32 tile = 0;       // +0x0C
    i32 direction = 0;  // +0x10 the direction from the parent
    i32 unk14 = 0;
    double f = 0;                            // +0x18 g + h
    double h = 0;                            // +0x20 the distance to the target
    double g = 0;                            // +0x28 steps from the search origin
    PathNode* poolNext = nullptr;            // +0x30 the free pool, or the nodes owned by a search
    PathNode* parent = nullptr;              // +0x34
    PathNode* listNext = nullptr;            // +0x38 the open or closed list
    std::array<PathNode*, 8> children = {};  // +0x3C
    i32 unk5C = 0;
};

// The node pool shared by every path finder (0x004CB1F0..0x004CB1FC): a free list growing on demand.
class PathNodePool
{
public:
    static constexpr int kInitial = 8;

    void Init();
    bool Release();
    PathNode* Alloc();
    int Free(PathNode* node);

    i32 unused = 0;  // 0x004CB1F0
    i32 free = 0;    // 0x004CB1F4
    i32 peak = 0;    // 0x004CB1F8

private:
    PathNode* NewNode();

    std::vector<std::unique_ptr<PathNode>> storage;
    PathNode* head = nullptr;  // 0x004CB1FC
};

// An A* path finder over the blocking map for a footprint of width x height tiles (0x48 bytes in the original).
class PathFinder
{
public:
    static constexpr int kMaxSteps = 0x1FE;
    static constexpr u8 kEnd = 0xFF;
    // The direction byte of a step, by pathDeltaY[dy + 1] + pathDeltaX[dx + 1] + 4 (swd32.exe 0x00499348..0x0049936B).
    static constexpr std::array<i8, 9> kDirectionByDelta = {0, 1, 2, 7, 0, 3, 6, 5, 4};
    static constexpr std::array<i32, 3> kDeltaX = {-1, 0, 1};
    static constexpr std::array<i32, 3> kDeltaY = {-3, 0, 3};
    // swd32.exe 0x0049936C
    static constexpr std::array<u8, 8> kOppositeDirection = {4, 5, 6, 7, 0, 1, 2, 3};

    PathFinder(PathNodePool& pool, const BlockingMap& map, ErrorReporter& reporter);
    ~PathFinder();
    PathFinder(const PathFinder&) = delete;
    PathFinder& operator=(const PathFinder&) = delete;

    int FindPath(int fromX, int fromY, int toX, int toY, int width, int height, std::span<u8> directions);
    int IsUsable() const;
    int TileIndex(int x, int y) const;
    int IsWalkable(int x, int y) const;
    void Reset();
    PathNode* Search(int fromX, int fromY, int toX, int toY);
    PathNode* PopBest();
    void Expand(PathNode& node, int targetX, int targetY);
    void TryNeighbour(PathNode& parent, int x, int y, int targetX, int targetY, int direction);
    PathNode* FindOpen(int tile) const;
    PathNode* FindClosed(int tile) const;
    void InsertOpen(PathNode& node);
    void Propagate(PathNode& node);
    void Push(PathNode& node);
    PathNode* Pop();
    void UpdateBlocked(int x, int y);
    int SetMask(int mask);

    PathNode* open = nullptr;      // +0x00 sentinel; its poolNext chains every node of the search
    PathNode* closed = nullptr;    // +0x04 sentinel
    PathNode* result = nullptr;    // +0x08 the node reached at the search origin
    std::vector<PathNode*> stack;  // +0x0C
    i32 found = 0;                 // +0x10 1 after a successful FindPath, 0 after an impossible one
    i32 unk14 = 0;
    i32 unk18 = 0;
    i32 width = 0;                    // +0x1C tiles
    i32 height = 0;                   // +0x20
    u32 mask = 0x40800000;            // +0x24 blocking bits
    std::array<u32, 8> blocked = {};  // +0x28 N, NE, E, SE, S, SW, W, NW

private:
    PathNodePool& pool;
    const BlockingMap& map;
    ErrorReporter& reporter;
};

}  // namespace swd3
