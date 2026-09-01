#include "openswd3/battle/legacy_battle_database_shutdown.hpp"

namespace openswd3::battle {

LegacyBattleDatabaseShutdownResult shutdown_legacy_battle_databases(
    LegacyBattleDatabaseShutdownPort& port,
    const LegacyBattleDatabaseShutdownRequest& request
) {
    auto& mon_database = port.legacy_battle_mon_database_state();
    auto& level_database = port.legacy_battle_level_database_state();

    LegacyBattleDatabaseShutdownResult result;
    result.initial_mon_handle = mon_database.handle;
    result.initial_level_handle = level_database.handle;

    compat::u32 eax = mon_database.handle;
    compat::u32 ecx = request.entry_ecx;
    compat::u32 edx = request.entry_edx;
    if (eax != 0xFFFFFFFFU && eax != 0U) {
        const auto reply = port.close_legacy_battle_database_handle({
            .handle = eax,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        mon_database.handle = 0xFFFFFFFFU;
        result.mon_handle_closed = true;
        ++result.close_calls;
    }

    eax = level_database.handle;
    if (eax != 0xFFFFFFFFU && eax != 0U) {
        const auto reply = port.close_legacy_battle_database_handle({
            .handle = eax,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        level_database.handle = 0xFFFFFFFFU;
        result.level_handle_closed = true;
        ++result.close_calls;
    }

    mon_database.open = false;
    level_database.open = false;

    result.final_mon_handle = mon_database.handle;
    result.final_level_handle = level_database.handle;
    result.return_eax = eax;
    result.return_ecx = ecx;
    result.return_edx = edx;
    return result;
}

}  // namespace openswd3::battle
