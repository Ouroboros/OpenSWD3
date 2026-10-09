#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <memory>
#include <vector>

namespace openswd3::battle {

enum class LegacyBattleMonSeekOrigin : compat::u32 {
    begin,
    current,
    end,
};

struct LegacyBattleMonReadResult {
    bool succeeded{};
    compat::u32 bytes_read{};
};

class LegacyBattleMonFileRuntime {
public:
    [[nodiscard]] compat::u32 open_file(
        const std::filesystem::path& file_path,
        const std::filesystem::path& data_directory = {}
    );

    [[nodiscard]] compat::u32 seek_file(
        compat::u32 handle,
        compat::i32 distance,
        LegacyBattleMonSeekOrigin origin
    );

    [[nodiscard]] LegacyBattleMonReadResult read_file(
        compat::u32 handle,
        std::span<compat::u8> destination,
        compat::u32 requested_bytes
    );

private:
    [[nodiscard]] resource_io::LegacyFile*
    find_file(compat::u32 handle) noexcept;

    std::vector<std::unique_ptr<resource_io::LegacyFile>> files_;
};

}  // namespace openswd3::battle
