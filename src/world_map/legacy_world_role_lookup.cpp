#include "openswd3/world_map/legacy_world_role_lookup.hpp"

namespace openswd3::world_map {

using compat::u16;
using compat::u32;

u32 find_legacy_world_role_by_guid(
    const std::span<const LegacyWorldRoleRecord> roles,
    const u16 guid) noexcept {
  for (u32 index = 0U; index < roles.size(); ++index) {
    const LegacyWorldRoleRecord &role = roles[index];
    if (role.guid == guid &&
        (role.flags & kLegacyWorldGuidLookupSkipBit) == 0U) {
      return index;
    }
  }
  return kLegacyWorldRoleNotFound;
}

LegacyWorldRoleGuidResult
legacy_world_role_guid_at(const std::span<const LegacyWorldRoleRecord> roles,
                          const u32 role_index) noexcept {
  if (role_index >= roles.size()) {
    return {};
  }
  return {
      .status = LegacyWorldRoleGuidStatus::ready,
      .guid = roles[role_index].guid,
  };
}

bool lookup_legacy_world_role_by_guid(
    const std::span<const LegacyWorldRoleRecord> roles, const u16 guid,
    u32 &role_index) noexcept {
  role_index = find_legacy_world_role_by_guid(roles, guid);
  return role_index != kLegacyWorldRoleNotFound;
}

bool resolve_legacy_world_role_selector(
    const std::span<const LegacyWorldRoleRecord> roles, const u16 selector,
    const u32 controlled_role_index, u32 &role_index) noexcept {
  role_index = 0U;
  if (selector == kLegacyWorldControlledRoleSelector) {
    role_index = controlled_role_index;
    return true;
  }
  return lookup_legacy_world_role_by_guid(roles, selector, role_index);
}

} // namespace openswd3::world_map
