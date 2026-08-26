#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <optional>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFileOwnerToken = 0x00521538U;
inline constexpr compat::u32 kLegacyBattleFileExitCleanupToken = 0x00451930U;

struct LegacyBattleFileOwner {
    std::optional<resource_io::LegacyFile> file;
};

class LegacyBattleFileExitRegistrationPort {
public:
    virtual ~LegacyBattleFileExitRegistrationPort() = default;

    [[nodiscard]] virtual compat::u32
    register_exit_cleanup(compat::u32 cleanup_token) = 0;
};

struct LegacyBattleFileConstructionResult {
    compat::u32 owner_token{};
    compat::u32 construction_calls{};
    compat::u32 return_value{};
};

struct LegacyBattleFileCleanupResult {
    compat::u32 owner_token{};
    compat::u32 cleanup_calls{};
    bool file_destroyed{};
};

struct LegacyBattleFileStaticInitializationResult {
    LegacyBattleFileConstructionResult construction{};
    compat::u32 exit_registration_calls{};
    compat::u32 return_value{};
};

// attached constructor wrapper sub_451910.
[[nodiscard]] LegacyBattleFileConstructionResult
construct_legacy_battle_file(LegacyBattleFileOwner& owner);

// attached destructor wrapper sub_451930.
[[nodiscard]] LegacyBattleFileCleanupResult
release_legacy_battle_file(LegacyBattleFileOwner& owner) noexcept;

// sub_451900 with external chunk loc_451920.
[[nodiscard]] LegacyBattleFileStaticInitializationResult
initialize_legacy_battle_file_static_lifecycle(
    LegacyBattleFileOwner& owner,
    LegacyBattleFileExitRegistrationPort& exit_registration_port
);

}  // namespace openswd3::battle
