#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::resource_io {

// Physical Save/*.sav blocks only. This does not restore game state.
enum class LegacySaveContainerStatus : compat::u8 {
    ready,
    truncated,
    invalid_length,
    decompression_failed,
};

struct LegacySaveDecodedBlock {
    compat::u32 declared_size{};
    std::vector<compat::u8> bytes;
};

struct LegacySaveContainer {
    std::array<compat::u8, 12U> timestamp{};
    std::array<compat::u8, 0x9600U> preview{};
    std::array<compat::u8, 0x20U> label{};
    // Block order: preview state, primary state, u16 state, Fame, tail pair.
    std::array<LegacySaveDecodedBlock, 5U> blocks{};
    std::array<bool, 5U> block_present{};
    std::array<compat::u8, 0x1CU> raw_after_primary{};
    std::array<compat::u8, 0x84U> extension_a{};
    std::array<compat::u8, 0x180U> extension_b{};
    std::array<compat::u8, 0x84U> extension_c{};
    bool extension_a_present{};
    bool extension_b_present{};
    bool extension_c_present{};
    std::size_t consumed_bytes{};
};

struct LegacySaveContainerResult {
    LegacySaveContainerStatus status{LegacySaveContainerStatus::truncated};
    LegacySaveContainer container;
};

[[nodiscard]] LegacySaveContainerResult
read_legacy_save_container(std::span<const compat::u8> bytes);

struct LegacySaveFameGroup {
    compat::u32 declared_span{};
    compat::u16 count{};
    std::array<compat::u8, 4U> header_tail{};
    std::vector<std::array<compat::u8, 14U>> records;
};

struct LegacySaveFameGroupsResult {
    bool complete{};
    std::array<LegacySaveFameGroup, 3U> groups;
    std::size_t consumed_bytes{};
};

// 0x00477FBE–0x004780ED; only the physical three-group record layout.
// This does not populate the three battle chain roots.
[[nodiscard]] LegacySaveFameGroupsResult
read_legacy_save_fame_groups(const LegacySaveContainer& save);

}  // namespace openswd3::resource_io
