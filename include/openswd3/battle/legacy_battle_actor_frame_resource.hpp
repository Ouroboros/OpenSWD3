#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <cstddef>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr std::size_t kLegacyBattleActorFrameResourceDwords =
    asset_runtime::kLegacyActionRecordSize / sizeof(compat::u32);

struct LegacyBattleActorFrameResourceView {
    const asset_runtime::LegacyActionRecord* source_action{};
    asset_runtime::LegacyActionRecord* prepared_action{};
    const std::byte* source_action_bytes{};
    std::byte* prepared_action_bytes{};
    compat::u32* frame_token{};
    const bool* frame_token_write_accessible{};
};

struct LegacyBattleActorFrameResourceOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

[[nodiscard]] LegacyBattleActorFrameResourceView
resolve_legacy_battle_actor_frame_resource(
    const LegacyBattleActorFrameResourceOwners& owners, compat::u32 actor_token
) noexcept;

enum class LegacyBattleActorFrameResourceStatus : compat::u8 {
    completed,
    push_ebx_write_typed_stop,
    push_esi_write_typed_stop,
    push_edi_write_typed_stop,
    direction_flag_contract_typed_stop,
    source_dword_read_typed_stop,
    destination_dword_write_typed_stop,
    action_argument_push_typed_stop,
    action_call_return_push_typed_stop,
    prepared_frame_word_read_typed_stop,
    prepared_resource_word_read_typed_stop,
    frame_argument_push_typed_stop,
    resource_argument_push_typed_stop,
    frame_call_return_push_typed_stop,
    frame_token_write_typed_stop,
    pop_edi_read_typed_stop,
    pop_esi_read_typed_stop,
    pop_ebx_read_typed_stop,
    early_return_address_read_typed_stop,
    success_return_address_read_typed_stop,
};

struct LegacyBattleActorFrameResourceStackAccess {
    bool push_ebx_writable{true};
    bool push_esi_writable{true};
    bool push_edi_writable{true};
    bool action_argument_push_writable{true};
    bool action_call_return_push_writable{true};
    bool frame_argument_push_writable{true};
    bool resource_argument_push_writable{true};
    bool frame_call_return_push_writable{true};
    bool pop_edi_readable{true};
    bool pop_esi_readable{true};
    bool pop_ebx_readable{true};
    bool early_return_address_readable{true};
    bool success_return_address_readable{true};
};

struct LegacyBattleActorFrameResourceRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 entry_ebx{};
    compat::u32 entry_esi{};
    compat::u32 entry_edi{};
    compat::u32 entry_esp{0x70001000U};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{true};
    bool direction_flag{};
    std::array<bool, kLegacyBattleActorFrameResourceDwords>
        source_dword_readable = [] {
            std::array<bool, kLegacyBattleActorFrameResourceDwords> values{};
            values.fill(true);
            return values;
        }();
    std::array<bool, kLegacyBattleActorFrameResourceDwords>
        destination_dword_writable = [] {
            std::array<bool, kLegacyBattleActorFrameResourceDwords> values{};
            values.fill(true);
            return values;
        }();
    bool prepared_frame_word_readable{true};
    bool prepared_resource_word_readable{true};
    LegacyBattleActorFrameResourceStackAccess stack_access{};
    compat::u32 action_updater_return_eax{};
    compat::u32 action_updater_return_ecx{};
    compat::u32 action_updater_return_edx{};
    LegacyBattleActorCoordinateFlags action_updater_flags{};
    bool override_action_updater_return_eax{};
    bool action_updater_flags_known{};
    compat::u32 frame_provider_return_eax{1U};
    compat::u32 frame_provider_return_ecx{};
    compat::u32 frame_provider_return_edx{};
    LegacyBattleActorCoordinateFlags frame_provider_flags{};
    bool frame_provider_flags_known{};
};

struct LegacyBattleActorFrameResourceResult {
    LegacyBattleActorFrameResourceStatus status{
        LegacyBattleActorFrameResourceStatus::completed
    };
    asset_runtime::LegacyActionUpdateResult action_update{};
    rendering::LegacyFramePiece frame{};
    std::array<compat::u32, 8> stack_writes{};
    std::array<compat::u32, 6> stack_reads{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_ebx{};
    compat::u32 return_esi{};
    compat::u32 return_edi{};
    compat::u32 return_esp{};
    compat::u32 return_eip{};
    compat::u32 source_token{};
    compat::u32 destination_token{};
    compat::u32 frame_id{};
    compat::u32 resource_id{};
    compat::u32 action_updater_entry_eax{};
    compat::u32 action_updater_entry_ecx{};
    compat::u32 action_updater_entry_edx{};
    compat::u32 frame_provider_entry_eax{};
    compat::u32 frame_provider_entry_ecx{};
    compat::u32 frame_provider_entry_edx{};
    compat::u32 source_reads{};
    compat::u32 destination_writes{};
    compat::u32 copied_dwords{};
    compat::u32 action_update_calls{};
    compat::u32 frame_lookup_calls{};
    compat::u32 actor_writes{};
    compat::u32 stack_write_count{};
    compat::u32 stack_read_count{};
    std::size_t fault_dword_index{kLegacyBattleActorFrameResourceDwords};
    bool frame_available{};
    bool frame_token_committed{};
    bool returned_early{};
    bool returned{};
    bool flags_known{true};
    bool direction_flag{};
    LegacyBattleActorCoordinateFlags flags{};
};

// Typed closure of legacy 0x00478620. The source and destination records are
// distinct physical slots in one actor. The legacy REP MOVSD executes with the
// platform ABI direction flag clear and commits one dword at a time.
[[nodiscard]] LegacyBattleActorFrameResourceResult
prepare_legacy_battle_actor_frame_resource(
    const LegacyBattleActorFrameResourceView& actor,
    asset_runtime::LegacyActionUpdater& action_updater,
    rendering::LegacyFramePieceProvider& frame_provider,
    const LegacyBattleActorFrameResourceRequest& request = {}
);

}  // namespace openswd3::battle
