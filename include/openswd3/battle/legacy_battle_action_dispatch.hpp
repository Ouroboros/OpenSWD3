#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_frame_deformation.hpp"
#include "openswd3/battle/legacy_battle_frame_effect.hpp"
#include "openswd3/battle/legacy_battle_scale_scan.hpp"
#include "openswd3/battle/legacy_battle_status_indicator.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_countdown.hpp"

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActionGroupABaseToken = 0x005029D0U;
inline constexpr compat::u32 kLegacyBattleActionGroupBBaseToken = 0x00525508U;
inline constexpr compat::u32 kLegacyBattleActionGroupAStride = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleActionGroupBStride = 0x2B28U;

struct LegacyBattleActionCallRequest {
    compat::u32 callee_token{};
    std::array<compat::u32, 8> arguments{};
};

struct LegacyBattleActionCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, 8> outputs{};
    bool publish_accumulator{};
    compat::u32 accumulator{};
    bool publish_selection_word{};
    compat::u16 selection_word{};
    bool publish_selection_high_word{};
    compat::u16 selection_high_word{};
    bool publish_opponent_special_action{};
    compat::u16 opponent_special_action{};
    bool publish_opponent_spawn_count{};
    compat::u16 opponent_spawn_count{};
    compat::u32 object_flags{};
};

class LegacyBattleActionDispatchPort {
public:
    virtual ~LegacyBattleActionDispatchPort() = default;

    [[nodiscard]] virtual LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) = 0;
};

struct LegacyBattleOpponentRecord {
    compat::u16 action_id{};
    compat::u16 x{};
    compat::u16 y{};
    compat::u32 runtime_value{};
};

struct LegacyBattleActionDispatchState {
    compat::u32 side_mode{};
    compat::i32 group_a_count{};
    compat::i32 group_b_count{};
    compat::i32 available_actor_count{};
    compat::u32 blocking_effect{};

    compat::u16 current_actor_index{0xFFFFU};
    compat::u16 selected_target_index{0xFFFFU};
    compat::u16 stored_group_b_index{0xFFFFU};
    compat::u16 stored_group_a_index{0xFFFFU};
    compat::u32 packed_action_state{};

    compat::u32 action_accumulator{};
    compat::u16 selection_word{};
    compat::u16 selection_high_word{};
    compat::u32 selection_source{};
    compat::u32 selection_context{};
    compat::u16 computed_selection_word{};
    compat::i32 signed_action_value{};
    compat::u32 action_pending{};
    compat::u32 action_pending_aux{};
    compat::u32 frame_refresh_pending{};

    compat::u32 action_runtime_flags{};
    compat::u16 side_selection_word{};
    compat::u32 scene_value{};
    compat::u32 scene_gate{};
    compat::u32 input_mode{};
    compat::u32 battle_flags{};
    compat::u32 active_actor_count{};
    compat::u16 active_actor_snapshot{};
    compat::u32 phase_counter{};
    compat::u16 special_phase{};
    compat::u32 phase_condition{};
    compat::u32 phase_condition_aux{};
    compat::u32 phase_terminal{};
    compat::u32 scan_runtime{};
    compat::u32 scan_push_state{};
    compat::u32 scan_dialog_state{};
    compat::u16 scan_word{};
    compat::u32 battle_mode{};
    compat::u32 battle_submode{};

    compat::u32 message_gate{};
    compat::u32 message_state{};
    compat::u32 message_aux{};
    compat::u32 choice_state{};
    compat::u32 choice_cursor{};
    compat::u32 choice_commit{};
    compat::u32 overlay_gate{};
    compat::u32 result_mode{};

    compat::u32 packed_actor_counter{};
    compat::u32 group_a_special_count{};
    compat::u16 opponent_special_action{};
    compat::u16 opponent_spawn_count{};
    compat::u32 opponent_processed_counter{};
    compat::u32 mirror_group_b_spawn{};
    std::array<LegacyBattleOpponentRecord, 8> opponent_records{};
    std::array<std::byte, 0xA4> opponent_scratch{};
    std::array<compat::u32, 0x7E> opponent_workspace{};
    compat::u32 active_target_code{};
    compat::u32 active_effect_target{};
    compat::u32 active_effect_gate{};
    compat::u32 post_battle_counter{};
    compat::u32 current_summon_index{};
    compat::u32 summon_packed{};
    compat::u32 summon_gate{};
    compat::u32 summon_x{};
    compat::u32 summon_y{};
    compat::u8 summon_status{};

    compat::u32 temporary_record_flags{};
    compat::u8 temporary_record_mode{};
    std::array<compat::u8, 0x28> temporary_record{};
    asset_runtime::LegacyActionRecord persistent_action_record{};

    std::array<compat::u32, 10> group_a_to_actor{};
    std::array<compat::u32, 18> target_identity{};
    std::array<compat::u32, 18> selection_workspace{};
    std::array<compat::u32, 18> selected_group_b_identity{};
    std::array<compat::u32, 10> actor_effect_score{};
    std::array<compat::u32, 8> available_group_b_indices{};
    std::array<compat::u32, 10> summon_runtime{};
    std::array<compat::u16, 10> summon_target_x{};
    std::array<compat::u16, 10> summon_target_y{};
    std::array<compat::u16, 100> group_a_event_slots{};
    std::array<compat::u16, 8> group_b_status_words{};
    std::array<compat::u16, 10> group_a_status_words{};

    std::unique_ptr<asset_runtime::LegacyDeformationNode> deformation;
    compat::u32 deformation_owner_token{};
    bool deformation_active{};

    LegacyBattleFrameEffectState frame_effect{};
    LegacyBattleStatusIndicatorState status_indicator{};
    LegacyBattleScaleScanState scale_scan{};
    rendering::LegacyCountdownState countdown{};
};

struct LegacyBattleActionDispatchContext {
    rendering::LegacyFramebuffer& framebuffer;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
    LegacyBattleBoundedRandomPort& bounded_random;
    LegacyBattleIndicatorSoundPort& indicator_sound;
    rendering::LegacyCountdownFlagPorts& countdown_flags;
    std::span<compat::u8> internal_flags;
    compat::u32 status_indicator_action_eax_snapshot{};
};

enum class LegacyBattleActionDispatchStatus : compat::u8 {
    completed,
    group_a_index_typed_stop,
    group_b_index_typed_stop,
    actor_map_typed_stop,
    target_table_typed_stop,
    effect_score_typed_stop,
    target_object_typed_stop,
    event_slot_typed_stop,
    framebuffer_typed_stop,
    status_indicator_typed_stop,
    scale_scan_typed_stop,
    internal_flag_typed_stop,
    effect_record_typed_stop,
};

struct LegacyBattleActionDispatchResult {
    LegacyBattleActionDispatchStatus status{
        LegacyBattleActionDispatchStatus::completed
    };
    compat::u32 return_value{};
    compat::u16 action_code{};
    compat::u32 port_calls{};
    compat::u32 framebuffer_clear_calls{};
    compat::u32 group_a_iterations{};
    compat::u32 group_b_iterations{};
    compat::u32 terminal_resets{};
    LegacyBattleStatusIndicatorResult status_indicator{};
    compat::u32 status_indicator_calls{};
    LegacyBattleScaleScanResult scale_scan{};
    compat::u32 scale_scan_calls{};
    compat::u32 action_record_clear_calls{};
};

// sub_4539B0: dispatch one action code for the selected group-A actor and
// group-B target, preserving the original special-code predispatch and the
// sparse 1..36 ordinary switch.
[[nodiscard]] LegacyBattleActionDispatchResult dispatch_legacy_battle_action(
    LegacyBattleActionDispatchState& state,
    LegacyBattleActionDispatchPort& port,
    LegacyBattleActionDispatchContext& context,
    compat::u32 group_a_index,
    compat::u32 group_b_index
);

}  // namespace openswd3::battle
