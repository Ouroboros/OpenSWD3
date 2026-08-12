#pragma once

#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldPathDefaultCollisionMask = 0x40800000U;
inline constexpr std::size_t kLegacyWorldPathBufferSize = 0x200U;

enum class LegacyWorldPathfindingStatus : compat::u8 {
  completed,
  invalid_map_dimensions,
  invalid_surface_grid,
  footprint_out_of_range,
  surface_access_out_of_bounds,
};

struct LegacyWorldPathfindingRequest {
  compat::i32 start_x{};
  compat::i32 start_y{};
  compat::i32 target_x{};
  compat::i32 target_y{};
  compat::u32 footprint_width{};
  compat::u32 footprint_height{};
  compat::u32 map_width{};
  compat::u32 map_height{};
  std::span<const compat::u8> surface_grid;
};

struct LegacyWorldPathfindingResult {
  LegacyWorldPathfindingStatus status{LegacyWorldPathfindingStatus::completed};
  std::vector<compat::u8> path{0xFFU};
  compat::u32 path_length{};
  compat::i32 legacy_return_value{};
  bool legacy_path_limit_exceeded{};
};

struct LegacyWorldPathNodePoolStatistics {
  compat::u32 available_nodes{};
  compat::u32 high_water_nodes{};
  compat::u32 allocated_nodes{};
};

// 0x00402030..0x004021C8: the process-wide 0x60-byte A* node pool. The
// original starts with eight reusable nodes and grows by one only when empty.
class LegacyWorldPathNodePool {
public:
  LegacyWorldPathNodePool();
  ~LegacyWorldPathNodePool();

  LegacyWorldPathNodePool(const LegacyWorldPathNodePool &) = delete;
  LegacyWorldPathNodePool &operator=(const LegacyWorldPathNodePool &) = delete;
  LegacyWorldPathNodePool(LegacyWorldPathNodePool &&) = delete;
  LegacyWorldPathNodePool &operator=(LegacyWorldPathNodePool &&) = delete;

  [[nodiscard]] LegacyWorldPathNodePoolStatistics statistics() const noexcept;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  friend class LegacyWorldPathfinder;
};

// 0x004021D0..0x00402F77: one legacy A* object. Original call sites construct
// a fresh object for each route. The success flag is intentionally not reset
// between find_path calls; reusing a successful object is a preserved legacy
// edge case and also leaves that failed search allocated until destruction.
class LegacyWorldPathfinder {
public:
  explicit LegacyWorldPathfinder(LegacyWorldPathNodePool &node_pool);
  ~LegacyWorldPathfinder();

  LegacyWorldPathfinder(const LegacyWorldPathfinder &) = delete;
  LegacyWorldPathfinder &operator=(const LegacyWorldPathfinder &) = delete;
  LegacyWorldPathfinder(LegacyWorldPathfinder &&) = delete;
  LegacyWorldPathfinder &operator=(LegacyWorldPathfinder &&) = delete;

  void set_collision_mask(compat::u32 collision_mask) noexcept;
  [[nodiscard]] compat::u32 collision_mask() const noexcept;
  [[nodiscard]] bool legacy_success_flag() const noexcept;

  [[nodiscard]] LegacyWorldPathfindingResult
  find_path(const LegacyWorldPathfindingRequest &request);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace openswd3::world_map
