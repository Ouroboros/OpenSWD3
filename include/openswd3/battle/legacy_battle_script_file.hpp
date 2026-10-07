#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>

namespace openswd3::battle {

struct LegacyBattleAssets;

enum class LegacyBattleScriptSeekOrigin {
    begin,
    current,
};

struct LegacyBattleScriptFileReadReply {
    compat::u32 eax{};
    // Proven bytes supplied to the destination, not an uninitialized API count.
    compat::u32 bytes_written{};
    // Host window cannot represent the live destination range. No API reply.
    bool destination_unavailable{};
};

class LegacyBattleScriptFilePort {
public:
    virtual ~LegacyBattleScriptFilePort() = default;

    // CreateFileA: GENERIC_READ, FILE_SHARE_READ, OPEN_ALWAYS, NORMAL.
    [[nodiscard]] virtual compat::u32
    open_script_file(const std::filesystem::path& path) = 0;
    [[nodiscard]] virtual compat::u32 seek_script_file(
        compat::u32 handle,
        compat::i32 distance,
        LegacyBattleScriptSeekOrigin origin
    ) = 0;
    [[nodiscard]] virtual LegacyBattleScriptFileReadReply
    read_script_file(compat::u32 handle, std::span<compat::u8> destination) = 0;
    [[nodiscard]] virtual compat::u32 close_script_file(compat::u32 handle) = 0;
};

class LegacyBattleScriptFileRuntime final : public LegacyBattleScriptFilePort {
public:
    [[nodiscard]] compat::u32
    open_script_file(const std::filesystem::path& path) override;
    [[nodiscard]] compat::u32 seek_script_file(
        compat::u32 handle,
        compat::i32 distance,
        LegacyBattleScriptSeekOrigin origin
    ) override;
    [[nodiscard]] LegacyBattleScriptFileReadReply read_script_file(
        compat::u32 handle, std::span<compat::u8> destination
    ) override;
    [[nodiscard]] compat::u32 close_script_file(compat::u32 handle) override;

private:
    std::unordered_map<compat::u32, std::unique_ptr<resource_io::LegacyFile>>
        files_;
};

struct LegacyBattleScriptWindowRequest {
    std::filesystem::path data_root;
    compat::u32 battle_id{};
    compat::u32 enabled{};  // Borrow the value at 4A7B5C on entry.
    // Original Buffer is uninitialized. Required only for an unread suffix.
    std::optional<std::array<compat::u8, 4>> offset_stack_bytes;
};

enum class LegacyBattleScriptWindowStatus {
    completed,
    offset_stack_unavailable,
};

struct LegacyBattleScriptWindowResult {
    LegacyBattleScriptWindowStatus status{
        LegacyBattleScriptWindowStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 offset_bytes_written{};
    LegacyBattleScriptFileReadReply window_read;
};

// 46E0B0. Ordinary file failures are not caller success gates.
[[nodiscard]] LegacyBattleScriptWindowResult
load_legacy_battle_script_window_file(
    LegacyBattleAssets& assets,
    compat::u32& cursor,
    LegacyBattleScriptFilePort& files,
    const LegacyBattleScriptWindowRequest& request
);

// 46E1E0. Uses the same open file; return the actual ReadFile reply.
[[nodiscard]] LegacyBattleScriptFileReadReply
load_legacy_battle_script_page_file(
    LegacyBattleAssets& assets,
    compat::u32& cursor,
    LegacyBattleScriptFilePort& files,
    compat::u32 data_offset
);

// 46E260 prefix, before the existing script-state reset and window release.
void close_legacy_battle_script_file(
    LegacyBattleAssets& assets, LegacyBattleScriptFilePort& files
);

}  // namespace openswd3::battle
