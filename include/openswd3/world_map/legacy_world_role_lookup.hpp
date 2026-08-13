#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"

#include <span>

namespace openswd3::world_map {

inline constexpr compat::u32 kLegacyWorldGuidLookupSkipBit = 0x10000000U;
inline constexpr compat::u32 kLegacyWorldRoleNotFound = 0xFFFFFFFFU;
inline constexpr compat::u16 kLegacyWorldControlledRoleSelector = 0xFFFEU;

enum class LegacyWorldRoleGuidStatus : compat::u8 {
  ready,
  invalid_role_index,
};

struct LegacyWorldRoleGuidResult {
  LegacyWorldRoleGuidStatus status{
      LegacyWorldRoleGuidStatus::invalid_role_index};
  compat::u16 guid{};
};

// 0x0040C020: return the first matching role whose bit 28 is clear, or
// 0xFFFFFFFF when no role matches.
[[nodiscard]] compat::u32
find_legacy_world_role_by_guid(std::span<const LegacyWorldRoleRecord> roles,
                               compat::u16 guid) noexcept;

// 0x0040C060: convert an array index to its role GUID. The original diagnostic
// branch reads outside the role array before and after MessageBoxA; the modern
// boundary reports invalid_role_index instead of reproducing that unsafe read.
[[nodiscard]] LegacyWorldRoleGuidResult
legacy_world_role_guid_at(std::span<const LegacyWorldRoleRecord> roles,
                          compat::u32 role_index) noexcept;

// 0x0040C100: always store the 0x0040C020 result and return whether it differs
// from 0xFFFFFFFF.
[[nodiscard]] bool
lookup_legacy_world_role_by_guid(std::span<const LegacyWorldRoleRecord> roles,
                                 compat::u16 guid,
                                 compat::u32 &role_index) noexcept;

// 0x0040C0D0: initialize the output to zero, map selector 0xFFFE directly to
// the controlled-role index, otherwise delegate to 0x0040C100. The 0xFFFE
// branch returns true without validating the controlled-role index, matching
// the helper; callers must isolate an invalid index before array access.
[[nodiscard]] bool resolve_legacy_world_role_selector(
    std::span<const LegacyWorldRoleRecord> roles, compat::u16 selector,
    compat::u32 controlled_role_index, compat::u32 &role_index) noexcept;

} // namespace openswd3::world_map
