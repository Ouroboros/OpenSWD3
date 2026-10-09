#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/battle/legacy_battle_mon_text.hpp"
#include "openswd3/battle/legacy_battle_mon_file_runtime.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleMonProfileBytes = 0x28U;
inline constexpr compat::u32 kLegacyBattleMonStreamBytes = 0x400U;
inline constexpr compat::u32 kLegacyBattleMonPathBufferToken = 0x004AAED0U;
inline constexpr compat::u32 kLegacyBattleMonDefinitionScratchBytes = 0xA4U;

using LegacyBattleMonProfile =
    std::array<std::byte, kLegacyBattleMonProfileBytes>;

struct LegacyBattleMonDatabaseState {
    bool open{};
    compat::u32 handle{0xFFFFFFFFU};
    compat::u32 definition_text_allocation_bytes{};
};

struct LegacyBattleMonStreamAllocation {
    compat::u32 block_token{};
    std::span<compat::u8> bytes{};
};

struct LegacyBattleMonTextAllocation {
    compat::u32 block_token{};
    std::shared_ptr<LegacyBattleMonText::Storage> storage{};
    std::shared_ptr<const LegacyBattleMonText::Release> release{};
};

class LegacyBattleMonDatabasePort {
public:
    virtual ~LegacyBattleMonDatabasePort() = default;

    [[nodiscard]] virtual compat::u32
    open_mon_file(const std::filesystem::path& path);

    [[nodiscard]] virtual compat::u32 seek_mon_file(
        compat::u32 handle,
        compat::i32 distance,
        LegacyBattleMonSeekOrigin origin
    );

    [[nodiscard]] virtual LegacyBattleMonReadResult read_mon_file(
        compat::u32 handle,
        std::span<compat::u8> destination,
        compat::u32 requested_bytes
    );

    [[nodiscard]] virtual LegacyBattleMonStreamAllocation
    allocate_mon_stream(compat::u32 size);

    virtual void release_mon_stream(compat::u32 block_token);

    [[nodiscard]] virtual compat::u32 mon_text_size(compat::u32 block_token);

    [[nodiscard]] virtual LegacyBattleMonTextAllocation
    allocate_mon_text(compat::u32 size);

    virtual void release_mon_text(compat::u32 block_token);

    [[nodiscard]] virtual LegacyBattleMonDatabaseState&
    legacy_battle_mon_database_state() noexcept;

    [[nodiscard]] virtual LegacyBattleMonProfile&
    legacy_battle_mon_profile_scratch() noexcept;

    [[nodiscard]] virtual std::
        array<compat::u8, kLegacyBattleMonDefinitionScratchBytes>&
        legacy_battle_mon_definition_scratch() noexcept;

    [[nodiscard]] virtual LegacyBattleMonText&
    legacy_battle_mon_definition_scratch_description() noexcept;

private:
    LegacyBattleMonDatabaseState mon_database_state_{};
    LegacyBattleMonProfile mon_profile_scratch_{};
    std::array<compat::u8, kLegacyBattleMonDefinitionScratchBytes>
        mon_definition_scratch_{};
    LegacyBattleMonText mon_definition_scratch_description_;
};

struct LegacyBattleMonProfileLoadRequest {
    std::filesystem::path path;
    compat::u32 profile_id{};
    compat::u32 stale_root_buffer_value{};
};

enum class LegacyBattleMonProfileLoadStatus : compat::u8 {
    completed,
    open_failed,
    stream_zero_typed_stop,
    stream_access_typed_stop,
    output_access_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_mon_profile_load_stopped(
    const LegacyBattleMonProfileLoadStatus status
) noexcept {
    return status == LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop ||
        status == LegacyBattleMonProfileLoadStatus::stream_access_typed_stop ||
        status == LegacyBattleMonProfileLoadStatus::output_access_typed_stop;
}

struct LegacyBattleMonProfileLoadResult {
    LegacyBattleMonProfileLoadStatus status{
        LegacyBattleMonProfileLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 profile_id{};
    compat::u32 auxiliary_root{};
    compat::u32 profile_relative_offset{};
    compat::u32 profile_file_offset{};
    compat::u32 stream_token{};
    compat::u32 stream_cursor{};
    compat::u32 stopped_stream_offset{};
    compat::u32 stopped_output_offset{};
    compat::u32 open_calls{};
    compat::u32 seek_calls{};
    compat::u32 read_calls{};
    compat::u32 allocation_calls{};
    compat::u32 release_calls{};
    bool profile_found{};
};

// Typed closure of legacy 0x00476A80.
[[nodiscard]] LegacyBattleMonProfileLoadResult load_legacy_battle_mon_profile(
    std::span<std::byte> output,
    LegacyBattleMonDatabasePort& port,
    const LegacyBattleMonProfileLoadRequest& request
);

}  // namespace openswd3::battle
