#pragma once

#include "openswd3/battle/legacy_battle_mon_profile.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <memory>
#include <vector>

namespace openswd3::battle {

// File handles identify owned open files. Profile and definition loaders share
// this runtime through their existing LegacyBattleMonDatabasePort.
class LegacyBattleMonFileRuntime {
public:
    [[nodiscard]] LegacyBattleMonDatabaseCallReply invoke(
        const LegacyBattleMonDatabaseCallRequest& request,
        std::span<compat::u8> destination,
        const std::filesystem::path& data_directory = {}
    );

private:
    [[nodiscard]] resource_io::LegacyFile*
    find_file(compat::u32 handle) noexcept;

    std::vector<std::unique_ptr<resource_io::LegacyFile>> files_;
};

}  // namespace openswd3::battle
