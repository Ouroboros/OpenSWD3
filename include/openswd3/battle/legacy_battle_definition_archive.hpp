#pragma once

#include "openswd3/battle/legacy_battle_render_geometry.hpp"

#include <filesystem>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleDefinitionArchivePathBufferToken =
    0x004AAED0U;
inline constexpr compat::u32 kLegacyBattleDefinitionArchiveHeaderBytes =
    0x2714U;
inline constexpr compat::u32 kLegacyBattleDefinitionArchiveHeaderIndexOffset =
    0x1F48U;

struct LegacyBattleDefinitionArchiveApiReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleDefinitionArchiveOpenRequest {
    std::filesystem::path path;
    compat::u32 desired_access{0x80000000U};
    compat::u32 share_mode{};
    compat::u32 security_attributes_token{};
    compat::u32 creation_disposition{3U};
    compat::u32 flags_and_attributes{0x80U};
    compat::u32 template_file_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleDefinitionArchiveReadRequest {
    compat::u32 handle{};
    compat::u32 destination_token{};
    compat::u32 requested_bytes{kLegacyBattleDefinitionArchiveHeaderBytes};
    compat::u32 overlapped_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleDefinitionArchiveReadReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 bytes_read{};
};

struct LegacyBattleDefinitionArchiveCloseRequest {
    compat::u32 handle{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

class LegacyBattleDefinitionArchiveHeaderPort {
public:
    virtual ~LegacyBattleDefinitionArchiveHeaderPort() = default;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveApiReply
    open_header(const LegacyBattleDefinitionArchiveOpenRequest& request) = 0;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveReadReply read_header(
        const LegacyBattleDefinitionArchiveReadRequest& request,
        std::span<compat::u8> destination
    ) = 0;

    [[nodiscard]] virtual LegacyBattleDefinitionArchiveApiReply
    close_header(const LegacyBattleDefinitionArchiveCloseRequest& request) = 0;
};

struct LegacyBattleDefinitionArchiveHeaderLoadRequest {
    std::filesystem::path path;
    compat::u32 binding_object_token{
        kLegacyBattleRenderGeometryBindingObjectToken
    };
    compat::u32 output_token{};
    compat::u32 file_name_token{kLegacyBattleDefinitionArchivePathBufferToken};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 entry_edx{};
};

enum class LegacyBattleDefinitionArchiveHeaderLoadStatus : compat::u8 {
    completed,
    open_failed,
};

struct LegacyBattleDefinitionArchiveHeaderLoadResult {
    LegacyBattleDefinitionArchiveHeaderLoadStatus status{
        LegacyBattleDefinitionArchiveHeaderLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 open_calls{};
    compat::u32 read_calls{};
    compat::u32 close_calls{};
    compat::u32 bytes_read{};
    compat::u32 published_header_index_token{};
    bool header_index_published{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x0045F130.
[[nodiscard]] LegacyBattleDefinitionArchiveHeaderLoadResult
load_legacy_battle_definition_archive_header(
    LegacyBattleRenderGeometryBindingObject& object,
    compat::u32& published_header_index_token,
    LegacyBattleDefinitionArchiveHeaderPort& port,
    const LegacyBattleDefinitionArchiveHeaderLoadRequest& request
);

}  // namespace openswd3::battle
