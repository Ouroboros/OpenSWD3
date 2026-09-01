#pragma once

#include "openswd3/battle/legacy_battle_level_requirement.hpp"
#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <array>
#include <filesystem>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleLevelProfileBytes = 0x38U;
inline constexpr compat::u32 kLegacyBattleLevelItemNodeBytes = 0xB0U;
inline constexpr compat::u32 kLegacyBattleLevelItemDefinitionOffset = 0x0CU;
inline constexpr compat::u32 kLegacyBattleLevelCaptionToken = 0x0053C154U;
inline constexpr compat::u32 kLegacyBattleLevelTransitionModeToken =
    0x0053BFFCU;
inline constexpr compat::u32 kLegacyBattleLevelZeroItemTextToken = 0x004A7D04U;
inline constexpr compat::u32 kLegacyBattleLevelZeroItemSourceToken =
    0x004A7D18U;
inline constexpr compat::u32 kLegacyBattleLevelZeroItemSourceLine = 0x330U;

enum class LegacyBattleLevelProfileCall : compat::u8 {
    report_zero_item,
    allocate_item_node,
};

struct LegacyBattleLevelProfileCallRequest {
    LegacyBattleLevelProfileCall call{
        LegacyBattleLevelProfileCall::report_zero_item
    };
    compat::u32 window_token{};
    compat::u32 text_token{};
    compat::u32 flags{};
    compat::u32 source_token{};
    compat::u32 source_line{};
    compat::u32 allocation_size{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleLevelProfileCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
    bool allocation_failed{};
    bool publish_allocation_token{};
    compat::u32 allocation_token{};
};

class LegacyBattleLevelProfilePort
    : public virtual LegacyBattleLevelDatabasePort,
      public virtual LegacyBattleMonDatabasePort {
public:
    ~LegacyBattleLevelProfilePort() override = default;

    [[nodiscard]] virtual world_map::LegacyWorldItemListState*
    battle_level_profile_item_list_state() noexcept {
        auto* const owner =
            dynamic_cast<world_map::LegacyWorldItemListStatePort*>(this);
        return owner == nullptr ? nullptr : &owner->world_item_list_state();
    }

    [[nodiscard]] virtual LegacyBattleLevelProfileCallReply
    invoke_level_profile(const LegacyBattleLevelProfileCallRequest& request) {
        LegacyBattleLevelProfileCallReply reply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call == LegacyBattleLevelProfileCall::allocate_item_node) {
            auto* const state = battle_level_profile_item_list_state();
            if (state == nullptr) {
                reply.allocation_failed = true;
                return reply;
            }
            reply.eax = state->next_battle_level_item_node_token;
            state->next_battle_level_item_node_token +=
                kLegacyBattleLevelItemNodeBytes;
        }
        return reply;
    }
};

struct LegacyBattleLevelProfileLoadRequest {
    std::filesystem::path path{"level.dat"};
    std::filesystem::path mon_path{"mon.dat"};
    compat::u32 party_number_one_based{};
    compat::u32 level{};
    compat::u32 output_token{};
    compat::u32 file_name_token{kLegacyBattleLevelPathBufferToken};
    compat::u32 caption_token{kLegacyBattleLevelCaptionToken};
    compat::u32 transition_mode_token{kLegacyBattleLevelTransitionModeToken};
    compat::u32 window_token{};
    compat::u32 stale_directory_offset{};
    compat::u32 number_of_bytes_read_token{};
    compat::u32 mon_file_name_token{kLegacyBattleMonPathBufferToken};
    compat::u32 mon_directory_buffer_token{};
    compat::u32 mon_stale_directory_probe_value{};
    compat::u32 mon_stale_relative_offset_value{};
    compat::u32 mon_number_of_bytes_read_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 output_accessible_bytes{kLegacyBattleLevelProfileBytes};
    compat::u32 caption_accessible_bytes{24U};
    bool transition_mode_accessible{true};
    bool host_item_node_allocation_succeeds{true};
};

enum class LegacyBattleLevelProfileLoadStatus : compat::u8 {
    completed,
    open_failed,
    stream_zero_typed_stop,
    stream_access_typed_stop,
    output_access_typed_stop,
    party_index_typed_stop,
    party_sentinel_typed_stop,
    diagnostic_typed_stop,
    item_node_typed_stop,
    item_allocation_typed_stop,
    host_item_allocation_typed_stop,
    mon_definition_load_typed_stop,
    transition_mode_typed_stop,
    caption_source_typed_stop,
    caption_destination_typed_stop,
};

[[nodiscard]] constexpr bool legacy_battle_level_profile_load_stopped(
    const LegacyBattleLevelProfileLoadStatus status
) noexcept {
    return status != LegacyBattleLevelProfileLoadStatus::completed &&
        status != LegacyBattleLevelProfileLoadStatus::open_failed;
}

struct LegacyBattleLevelProfileLoadResult {
    LegacyBattleLevelProfileLoadStatus status{
        LegacyBattleLevelProfileLoadStatus::completed
    };
    compat::u32 handle{};
    compat::u32 party_number_one_based{};
    compat::u32 level{};
    compat::u32 directory_entry_offset{};
    compat::u32 record_relative_offset{};
    compat::u32 record_file_offset{};
    compat::u32 stream_token{};
    compat::u32 stream_cursor{};
    compat::u32 stopped_stream_offset{};
    compat::u32 stopped_output_offset{};
    compat::u32 stopped_caption_offset{};
    compat::u32 stopped_item_token{};
    compat::u32 temporary_party_head_token{};
    compat::u32 open_calls{};
    compat::u32 seek_calls{};
    compat::u32 read_calls{};
    compat::u32 stream_allocation_calls{};
    compat::u32 stream_release_calls{};
    compat::u32 diagnostic_calls{};
    compat::u32 item_allocation_calls{};
    compat::u32 item_definition_load_calls{};
    compat::u32 traversed_item_nodes{};
    compat::u32 appended_item_nodes{};
    compat::u32 output_bytes_copied{};
    compat::u32 output_write_count{};
    compat::u32 caption_bytes_copied{};
    compat::u32 transition_mode_writes{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    bool record_found{};
    bool party_root_restored{true};
    LegacyBattleMonDefinitionLoadResult mon_definition_load{};
};

// Typed closure of legacy 0x00477400.
[[nodiscard]] LegacyBattleLevelProfileLoadResult
load_legacy_battle_level_profile(
    world_map::LegacyWorldStoryPartyMemberResources& output,
    std::array<compat::u8, 24U>& growth_caption,
    compat::u32& transition_mode,
    LegacyBattleLevelProfilePort& port,
    const LegacyBattleLevelProfileLoadRequest& request
);

}  // namespace openswd3::battle
