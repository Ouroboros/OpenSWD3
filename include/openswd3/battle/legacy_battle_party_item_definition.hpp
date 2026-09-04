#pragma once

#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"

#include <array>
#include <filesystem>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattlePartyItemHeadArrayToken = 0x004A9490U;
inline constexpr compat::u32 kLegacyBattlePartyItemCaptionToken = 0x0053C154U;
inline constexpr compat::u32 kLegacyBattlePartyItemZeroTextToken = 0x004A7D38U;
inline constexpr compat::u32 kLegacyBattlePartyItemSourceToken = 0x004A7D18U;
inline constexpr compat::u32 kLegacyBattlePartyItemZeroSourceLine = 0x50AU;
inline constexpr compat::u32 kLegacyBattlePartyItemDefinitionOffset = 0x0CU;
inline constexpr compat::u16 kLegacyBattlePartyItemNoAllocationId = 0x8000U;

using LegacyBattlePartyItemAllocationWords = std::array<
    compat::u32,
    world_map::kLegacyWorldItemNodeBytes / sizeof(compat::u32)>;

enum class LegacyBattlePartyItemDefinitionCall : compat::u8 {
    report_zero_item,
    allocate_item_node,
    copy_caption,
};

struct LegacyBattlePartyItemDefinitionCallRequest {
    LegacyBattlePartyItemDefinitionCall call{
        LegacyBattlePartyItemDefinitionCall::report_zero_item
    };
    compat::u32 window_token{};
    compat::u32 text_token{};
    compat::u32 flags{};
    compat::u32 source_file_token{};
    compat::u32 source_line{};
    compat::u32 allocation_size{};
    compat::u32 destination_token{};
    compat::u32 source_token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattlePartyItemDefinitionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattlePartyItemAllocationWords allocation_words{};
    compat::u32 allocation_accessible_bytes{
        world_map::kLegacyWorldItemNodeBytes
    };
    bool typed_stop{};
};

class LegacyBattlePartyItemDefinitionPort {
public:
    virtual ~LegacyBattlePartyItemDefinitionPort() = default;

    [[nodiscard]] virtual LegacyBattlePartyItemDefinitionCallReply
    invoke(const LegacyBattlePartyItemDefinitionCallRequest& request) = 0;
};

struct LegacyBattlePartyItemDefinitionRequest {
    std::filesystem::path definition_path{"mon.dat"};
    compat::u32 party_index{};
    compat::u32 item_id{};
    compat::u32 head_array_token{kLegacyBattlePartyItemHeadArrayToken};
    compat::u32 caption_token{kLegacyBattlePartyItemCaptionToken};
    compat::u32 window_token{};
    compat::u32 mon_file_name_token{kLegacyBattleMonPathBufferToken};
    compat::u32 mon_directory_buffer_token{};
    compat::u32 mon_stale_directory_probe_value{};
    compat::u32 mon_stale_relative_offset_value{};
    compat::u32 mon_number_of_bytes_read_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 caption_accessible_bytes{24U};
    bool host_item_node_allocation_succeeds{true};
};

enum class LegacyBattlePartyItemDefinitionStatus : compat::u8 {
    completed,
    diagnostic_typed_stop,
    party_index_typed_stop,
    item_node_access_typed_stop,
    allocation_call_typed_stop,
    allocation_node_access_typed_stop,
    host_item_allocation_typed_stop,
    definition_load_typed_stop,
    caption_call_typed_stop,
    caption_source_typed_stop,
    caption_destination_typed_stop,
};

enum class LegacyBattlePartyItemDefinitionPath : compat::u8 {
    none,
    existing_head,
    existing_successor,
    missing_reserved,
    appended,
};

struct LegacyBattlePartyItemDefinitionResult {
    LegacyBattlePartyItemDefinitionStatus status{
        LegacyBattlePartyItemDefinitionStatus::completed
    };
    LegacyBattlePartyItemDefinitionPath path{
        LegacyBattlePartyItemDefinitionPath::none
    };
    compat::u32 party_index{};
    compat::u16 item_id{};
    compat::u32 original_head_token{};
    compat::u32 current_head_token{};
    compat::u32 matched_token{};
    compat::u32 allocation_token{};
    compat::u32 stopped_token{};
    compat::u32 stopped_offset{};
    compat::u32 stopped_address{};
    compat::u32 diagnostic_calls{};
    compat::u32 allocation_calls{};
    compat::u32 definition_load_calls{};
    compat::u32 caption_copy_calls{};
    compat::u32 item_key_reads{};
    compat::u32 item_key_writes{};
    compat::u32 chain_link_reads{};
    compat::u32 chain_link_writes{};
    compat::u32 head_writes{};
    compat::u32 head_restore_writes{};
    compat::u32 cleared_dwords{};
    compat::u32 traversed_nodes{};
    compat::u32 appended_nodes{};
    compat::u32 caption_bytes_copied{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool head_restored{true};
    LegacyBattleMonDefinitionLoadResult definition_load{};
};

// Typed closure of legacy 0x00477BD0. The zero-based party head is moved while
// scanning. An existing successor remains published as the new head; reserved
// misses and successfully appended nodes restore the entry head at 0x00477C84.
[[nodiscard]] LegacyBattlePartyItemDefinitionResult
prepare_legacy_battle_party_item_definition(
    world_map::LegacyWorldItemListState& item_state,
    std::span<compat::u8> growth_caption,
    LegacyBattlePartyItemDefinitionPort& call_port,
    LegacyBattleMonDatabasePort& mon_port,
    const LegacyBattlePartyItemDefinitionRequest& request
);

}  // namespace openswd3::battle
