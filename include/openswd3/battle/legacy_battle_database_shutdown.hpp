#pragma once

#include "openswd3/battle/legacy_battle_level_requirement.hpp"
#include "openswd3/battle/legacy_battle_mon_profile.hpp"

namespace openswd3::battle {

struct LegacyBattleDatabaseHandleCloseRequest {
    compat::u32 handle{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleDatabaseHandleCloseReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

class LegacyBattleDatabaseShutdownPort
    : public virtual LegacyBattleMonDatabasePort,
      public virtual LegacyBattleLevelDatabasePort {
public:
    ~LegacyBattleDatabaseShutdownPort() override = default;

    [[nodiscard]] virtual LegacyBattleDatabaseHandleCloseReply
    close_legacy_battle_database_handle(
        const LegacyBattleDatabaseHandleCloseRequest& request
    ) = 0;
};

struct LegacyBattleDatabaseShutdownRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleDatabaseShutdownResult {
    compat::u32 initial_mon_handle{};
    compat::u32 initial_level_handle{};
    compat::u32 final_mon_handle{};
    compat::u32 final_level_handle{};
    compat::u32 close_calls{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool mon_handle_closed{};
    bool level_handle_closed{};
};

// Typed closure of legacy 0x004776A0.
[[nodiscard]] LegacyBattleDatabaseShutdownResult
shutdown_legacy_battle_databases(
    LegacyBattleDatabaseShutdownPort& port,
    const LegacyBattleDatabaseShutdownRequest& request
);

}  // namespace openswd3::battle
