#include "openswd3/world_map/legacy_world_pathfinding.hpp"

#include "openswd3/world_map/legacy_world_direction_adjustment.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace openswd3::world_map {
namespace {

using compat::i32;
using compat::u32;
using compat::u8;

struct PathNode {
    i32 world_x{};                         // legacy +0x04
    i32 world_y{};                         // legacy +0x08
    i32 cell_index{};                      // legacy +0x0C
    u32 direction{};                       // legacy +0x10
    double total_cost{};                   // legacy +0x18
    double heuristic_cost{};               // legacy +0x20
    double path_cost{};                    // legacy +0x28
    PathNode* active_or_free_next{};       // legacy +0x30
    PathNode* parent{};                    // legacy +0x34
    PathNode* list_next{};                 // legacy +0x38
    std::array<PathNode*, 8U> neighbours;  // legacy +0x3C..+0x58
};

[[nodiscard]] constexpr i32 tile_coordinate(const i32 coordinate) noexcept {
    const i32 quotient = coordinate / 16;
    return coordinate < 0 && coordinate % 16 != 0 ? quotient - 1 : quotient;
}

[[nodiscard]] constexpr i32
wrapping_add(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) + std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32
wrapping_subtract(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) - std::bit_cast<u32>(right)
    );
}

[[nodiscard]] constexpr i32
wrapping_multiply(const i32 left, const i32 right) noexcept {
    return std::bit_cast<i32>(
        std::bit_cast<u32>(left) * std::bit_cast<u32>(right)
    );
}

[[nodiscard]] i32 legacy_cell_index(
    const i32 world_x, const i32 world_y, const u32 map_width
) noexcept {
    return wrapping_add(
        wrapping_multiply(
            tile_coordinate(world_y), std::bit_cast<i32>(map_width)
        ),
        tile_coordinate(world_x)
    );
}

[[nodiscard]] double legacy_distance(
    const i32 x, const i32 y, const i32 target_x, const i32 target_y
) noexcept {
    const i32 delta_x = wrapping_subtract(x, target_x);
    const i32 delta_y = wrapping_subtract(y, target_y);
    const i32 squared = wrapping_add(
        wrapping_multiply(delta_x, delta_x), wrapping_multiply(delta_y, delta_y)
    );
    if (squared < 0) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::sqrt(static_cast<double>(squared));
}

[[nodiscard]] LegacyWorldPathfindingStatus
map_probe_status(const LegacyWorldDirectionProbeStatus status) noexcept {
    switch (status) {
    case LegacyWorldDirectionProbeStatus::completed:
        return LegacyWorldPathfindingStatus::completed;
    case LegacyWorldDirectionProbeStatus::invalid_map_dimensions:
        return LegacyWorldPathfindingStatus::invalid_map_dimensions;
    case LegacyWorldDirectionProbeStatus::invalid_surface_grid:
        return LegacyWorldPathfindingStatus::invalid_surface_grid;
    case LegacyWorldDirectionProbeStatus::footprint_out_of_range:
        return LegacyWorldPathfindingStatus::footprint_out_of_range;
    case LegacyWorldDirectionProbeStatus::surface_access_out_of_bounds:
        return LegacyWorldPathfindingStatus::surface_access_out_of_bounds;
    }
    return LegacyWorldPathfindingStatus::invalid_surface_grid;
}

[[nodiscard]] constexpr u8
path_direction(const PathNode& from, const PathNode& to) noexcept {
    const i32 delta_x =
        tile_coordinate(from.world_x) - tile_coordinate(to.world_x);
    const i32 delta_y =
        tile_coordinate(from.world_y) - tile_coordinate(to.world_y);
    if (delta_y < 0) {
        return delta_x < 0 ? 0U : (delta_x > 0 ? 2U : 1U);
    }
    if (delta_y > 0) {
        return delta_x < 0 ? 6U : (delta_x > 0 ? 4U : 5U);
    }
    return delta_x < 0 ? 7U : 3U;
}

constexpr std::array<u32, 8U> kClosedImprovementDirection{
    4U, 5U, 6U, 7U, 0U, 1U, 2U, 3U
};
constexpr std::array<i32, 8U> kStepX{-16, 0, 16, 16, 16, 0, -16, -16};
constexpr std::array<i32, 8U> kStepY{-16, -16, -16, 0, 16, 16, 16, 0};

}  // namespace

struct LegacyWorldPathNodePool::Impl {
    std::vector<std::unique_ptr<PathNode>> storage;
    PathNode* free_head{};
    u32 available_nodes{};
    u32 high_water_nodes{};

    Impl() {
        storage.reserve(8U);
        for (u32 index = 0U; index < 8U; ++index) {
            add_free_node();
        }
        available_nodes = 8U;
        high_water_nodes = 8U;
    }

    void add_free_node() {
        auto node = std::make_unique<PathNode>();
        node->active_or_free_next = free_head;
        free_head = node.get();
        storage.push_back(std::move(node));
    }

    [[nodiscard]] PathNode* acquire() {
        if (free_head == nullptr) {
            add_free_node();
            ++available_nodes;
            high_water_nodes = std::max(high_water_nodes, available_nodes);
        }
        PathNode* node = free_head;
        free_head = node->active_or_free_next;
        --available_nodes;
        *node = PathNode{};
        return node;
    }

    void release(PathNode* node) noexcept {
        if (node == nullptr) {
            return;
        }
        node->active_or_free_next = free_head;
        free_head = node;
        ++available_nodes;
    }
};

LegacyWorldPathNodePool::LegacyWorldPathNodePool()
    : impl_{std::make_unique<Impl>()} {}

LegacyWorldPathNodePool::~LegacyWorldPathNodePool() = default;

LegacyWorldPathNodePoolStatistics
LegacyWorldPathNodePool::statistics() const noexcept {
    return {
        .available_nodes = impl_->available_nodes,
        .high_water_nodes = impl_->high_water_nodes,
        .allocated_nodes = static_cast<u32>(impl_->storage.size()),
    };
}

struct LegacyWorldPathfinder::Impl {
    LegacyWorldPathNodePool::Impl& pool;
    PathNode* active_head{};
    PathNode* open_sentinel{};
    PathNode* closed_sentinel{};
    PathNode* current{};
    std::vector<PathNode*> update_stack;
    u32 collision_mask{kLegacyWorldPathDefaultCollisionMask};
    u32 footprint_width{};
    u32 footprint_height{};
    bool success_flag{};
    LegacyWorldPathfindingStatus status{
        LegacyWorldPathfindingStatus::completed
    };
    const LegacyWorldPathfindingRequest* request{};

    explicit Impl(LegacyWorldPathNodePool::Impl& node_pool) : pool{node_pool} {}

    ~Impl() {
        clear_nodes();
    }

    void clear_nodes() noexcept {
        if (active_head != nullptr) {
            while (active_head->active_or_free_next != nullptr) {
                PathNode* node = active_head->active_or_free_next;
                active_head->active_or_free_next = node->active_or_free_next;
                pool.release(node);
            }
            pool.release(active_head);
        }
        active_head = nullptr;
        open_sentinel = nullptr;
        closed_sentinel = nullptr;
        current = nullptr;
        update_stack.clear();
    }

    [[nodiscard]] bool
    validate_request(const LegacyWorldPathfindingRequest& candidate) noexcept {
        if (candidate.map_width == 0U || candidate.map_height == 0U) {
            status = LegacyWorldPathfindingStatus::invalid_map_dimensions;
            return false;
        }
        if (candidate.footprint_width > kLegacyWorldDirectionMaxFootprint ||
            candidate.footprint_height > kLegacyWorldDirectionMaxFootprint) {
            status = LegacyWorldPathfindingStatus::footprint_out_of_range;
            return false;
        }
        constexpr std::size_t bytes_per_cell = 4U;
        if (static_cast<std::size_t>(candidate.map_width) >
            std::numeric_limits<std::size_t>::max() / candidate.map_height) {
            status = LegacyWorldPathfindingStatus::invalid_surface_grid;
            return false;
        }
        const std::size_t cells =
            static_cast<std::size_t>(candidate.map_width) *
            candidate.map_height;
        if (cells > std::numeric_limits<std::size_t>::max() / bytes_per_cell ||
            candidate.surface_grid.size() < cells * bytes_per_cell) {
            status = LegacyWorldPathfindingStatus::invalid_surface_grid;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool read_surface_cell(const i32 index, u32& value) noexcept {
        const std::uint64_t cells =
            static_cast<std::uint64_t>(request->map_width) *
            request->map_height;
        if (index < 0 || static_cast<std::uint64_t>(index) >= cells) {
            status = LegacyWorldPathfindingStatus::surface_access_out_of_bounds;
            return false;
        }
        const std::size_t offset = static_cast<std::size_t>(index) * 4U;
        value = static_cast<u32>(request->surface_grid[offset]) |
            (static_cast<u32>(request->surface_grid[offset + 1U]) << 8U) |
            (static_cast<u32>(request->surface_grid[offset + 2U]) << 16U) |
            (static_cast<u32>(request->surface_grid[offset + 3U]) << 24U);
        return true;
    }

    [[nodiscard]] bool target_is_passable() noexcept {
        u32 value{};
        if (!read_surface_cell(
                legacy_cell_index(
                    request->target_x, request->target_y, request->map_width
                ),
                value
            )) {
            return false;
        }
        return (value & collision_mask) == 0U;
    }

    void link_active(PathNode* node) noexcept {
        node->active_or_free_next = active_head->active_or_free_next;
        active_head->active_or_free_next = node;
    }

    [[nodiscard]] PathNode*
    find_in_list(PathNode* sentinel, const i32 cell_index) const noexcept {
        for (PathNode* node = sentinel->list_next; node != nullptr;
             node = node->list_next) {
            if (node->cell_index == cell_index) {
                return node;
            }
        }
        return nullptr;
    }

    void insert_open(PathNode* node) noexcept {
        PathNode* previous = open_sentinel;
        PathNode* next = previous->list_next;
        while (next != nullptr && node->total_cost > next->total_cost) {
            previous = next;
            next = next->list_next;
        }
        node->list_next = next;
        previous->list_next = node;
    }

    [[nodiscard]] PathNode* pop_open() noexcept {
        PathNode* node = open_sentinel->list_next;
        if (node == nullptr) {
            return nullptr;
        }
        open_sentinel->list_next = node->list_next;
        node->list_next = closed_sentinel->list_next;
        closed_sentinel->list_next = node;
        return node;
    }

    static void append_neighbour(PathNode& node, PathNode* neighbour) noexcept {
        for (PathNode*& entry : node.neighbours) {
            if (entry == nullptr) {
                entry = neighbour;
                return;
            }
        }
    }

    void propagate_improvement(PathNode* start) {
        const auto relax = [this](PathNode* parent, PathNode* child) {
            const double candidate = parent->path_cost + 1.0;
            if (candidate < child->path_cost) {
                child->path_cost = candidate;
                child->total_cost = candidate + child->heuristic_cost;
                child->parent = parent;
                update_stack.push_back(child);
            }
        };

        for (PathNode* neighbour : start->neighbours) {
            if (neighbour == nullptr) {
                break;
            }
            relax(start, neighbour);
        }
        while (!update_stack.empty()) {
            PathNode* node = update_stack.back();
            update_stack.pop_back();
            for (PathNode* neighbour : node->neighbours) {
                if (neighbour == nullptr) {
                    break;
                }
                relax(node, neighbour);
            }
        }
    }

    void consider_neighbour(
        PathNode* parent,
        const i32 world_x,
        const i32 world_y,
        const i32 heuristic_target_x,
        const i32 heuristic_target_y,
        const u32 direction
    ) {
        const double candidate_cost = parent->path_cost + 1.0;
        const i32 cell_index =
            legacy_cell_index(world_x, world_y, request->map_width);

        if (PathNode* existing = find_in_list(open_sentinel, cell_index)) {
            append_neighbour(*parent, existing);
            if (candidate_cost < existing->path_cost) {
                existing->path_cost = candidate_cost;
                existing->parent = parent;
                existing->total_cost =
                    candidate_cost + existing->heuristic_cost;
            }
            return;
        }
        if (PathNode* existing = find_in_list(closed_sentinel, cell_index)) {
            append_neighbour(*parent, existing);
            if (candidate_cost < existing->path_cost) {
                existing->path_cost = candidate_cost;
                existing->parent = parent;
                existing->total_cost =
                    candidate_cost + existing->heuristic_cost;
                propagate_improvement(existing);
            }
            // 0x0040280A writes the opposite direction to the first neighbour of
            // the parent, even when the closed-node cost did not improve.
            parent->neighbours.front()->direction =
                kClosedImprovementDirection[direction];
            return;
        }

        PathNode* node = pool.acquire();
        link_active(node);
        node->path_cost = candidate_cost;
        node->parent = parent;
        node->world_x = world_x;
        node->world_y = world_y;
        node->cell_index = cell_index;
        node->direction = direction;
        node->heuristic_cost = legacy_distance(
            world_x, world_y, heuristic_target_x, heuristic_target_y
        );
        node->total_cost = node->heuristic_cost + candidate_cost;
        insert_open(node);
        append_neighbour(*parent, node);
    }

    [[nodiscard]] bool expand(
        PathNode* node,
        const i32 heuristic_target_x,
        const i32 heuristic_target_y
    ) {
        const auto occupancy = compute_legacy_world_directional_occupancy_mask(
            request->surface_grid,
            request->map_width,
            request->map_height,
            std::bit_cast<u32>(node->cell_index),
            footprint_width,
            footprint_height,
            collision_mask
        );
        if (occupancy.status != LegacyWorldDirectionProbeStatus::completed) {
            status = map_probe_status(occupancy.status);
            return false;
        }
        for (u32 direction = 0U; direction < 8U; ++direction) {
            if ((occupancy.mask & static_cast<u8>(1U << direction)) != 0U) {
                continue;
            }
            consider_neighbour(
                node,
                wrapping_add(node->world_x, kStepX[direction]),
                wrapping_add(node->world_y, kStepY[direction]),
                heuristic_target_x,
                heuristic_target_y,
                direction
            );
        }
        return true;
    }

    void initialize_search(
        const i32 start_x,
        const i32 start_y,
        const i32 target_x,
        const i32 target_y
    ) {
        open_sentinel = pool.acquire();
        closed_sentinel = pool.acquire();
        PathNode* target = pool.acquire();
        active_head = open_sentinel;
        open_sentinel->active_or_free_next = closed_sentinel;
        closed_sentinel->active_or_free_next = target;
        target->path_cost = 0.0;
        target->heuristic_cost =
            legacy_distance(target_x, target_y, start_x, start_y);
        target->total_cost = target->heuristic_cost;
        target->cell_index =
            legacy_cell_index(target_x, target_y, request->map_width);
        target->world_x = target_x;
        target->world_y = target_y;
        open_sentinel->list_next = target;

        const i32 start_index =
            legacy_cell_index(start_x, start_y, request->map_width);
        PathNode* node = pop_open();
        while (node != nullptr && node->cell_index != start_index) {
            if (!expand(node, start_x, start_y)) {
                node = nullptr;
                break;
            }
            node = pop_open();
        }
        current = node;
    }

    [[nodiscard]] bool legacy_search_result() const noexcept {
        if (current == nullptr) {
            return false;
        }
        if (!success_flag) {
            return true;
        }
        return current->parent == nullptr;
    }
};

LegacyWorldPathfinder::LegacyWorldPathfinder(LegacyWorldPathNodePool& node_pool)
    : impl_{std::make_unique<Impl>(*node_pool.impl_)} {}

LegacyWorldPathfinder::~LegacyWorldPathfinder() = default;

void LegacyWorldPathfinder::set_collision_mask(
    const u32 collision_mask
) noexcept {
    impl_->collision_mask = collision_mask;
}

u32 LegacyWorldPathfinder::collision_mask() const noexcept {
    return impl_->collision_mask;
}

bool LegacyWorldPathfinder::legacy_success_flag() const noexcept {
    return impl_->success_flag;
}

LegacyWorldPathfindingResult
LegacyWorldPathfinder::find_path(const LegacyWorldPathfindingRequest& request) {
    LegacyWorldPathfindingResult result;
    result.path.reserve(kLegacyWorldPathBufferSize);
    impl_->status = LegacyWorldPathfindingStatus::completed;
    impl_->request = &request;
    impl_->footprint_width = request.footprint_width;
    impl_->footprint_height = request.footprint_height;

    if (request.start_x == request.target_x &&
        request.start_y == request.target_y) {
        result.legacy_return_value = 1;
        return result;
    }
    if (!impl_->validate_request(request)) {
        result.status = impl_->status;
        return result;
    }
    if (!impl_->target_is_passable()) {
        impl_->success_flag = false;
        impl_->clear_nodes();
        result.status = impl_->status;
        return result;
    }
    if (legacy_cell_index(
            request.start_x, request.start_y, request.map_width
        ) ==
        legacy_cell_index(
            request.target_x, request.target_y, request.map_width
        )) {
        impl_->success_flag = false;
        impl_->clear_nodes();
        return result;
    }

    impl_->clear_nodes();
    impl_->initialize_search(
        request.start_x, request.start_y, request.target_x, request.target_y
    );
    if (impl_->status != LegacyWorldPathfindingStatus::completed) {
        result.status = impl_->status;
        impl_->clear_nodes();
        return result;
    }
    if (!impl_->legacy_search_result()) {
        return result;
    }

    PathNode* from = impl_->current;
    PathNode* to = from->parent;
    while (to != nullptr) {
        if (result.path_length == result.path.size()) {
            result.path.push_back(path_direction(*from, *to));
        } else {
            result.path[result.path_length] = path_direction(*from, *to);
        }
        ++result.path_length;
        if (result.path_length > 0x1FEU) {
            result.legacy_path_limit_exceeded = true;
        }
        from = to;
        to = to->parent;
    }
    if (result.path_length == result.path.size()) {
        result.path.push_back(0xFFU);
    } else {
        result.path[result.path_length] = 0xFFU;
    }
    impl_->success_flag = true;
    result.legacy_return_value = 1;
    impl_->clear_nodes();
    return result;
}

}  // namespace openswd3::world_map
