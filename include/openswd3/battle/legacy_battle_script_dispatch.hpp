#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_assets.hpp"
#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_group_b_order.hpp"
#include "openswd3/battle/legacy_battle_input_dispatch.hpp"
#include "openswd3/battle/legacy_battle_message_phase.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/battle/legacy_battle_target_selection_runtime.hpp"
#include "openswd3/battle/legacy_battle_victory_rewards.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleScriptGroupABaseToken = 0x005029D0U;
inline constexpr compat::u32 kLegacyBattleScriptGroupAElementSize = 0x2F34U;
inline constexpr compat::u32 kLegacyBattleScriptGroupBBaseToken = 0x00525508U;
inline constexpr compat::u32 kLegacyBattleScriptGroupBElementSize = 0x2B28U;
inline constexpr compat::u32 kLegacyBattleScriptDynamicCommandSize = 76U;
inline constexpr compat::u32 kLegacyBattleScriptTextScanLimit = 255U;

struct LegacyBattleScriptDynamicCommand {
    compat::u32 token{};
    std::array<compat::u8, kLegacyBattleScriptDynamicCommandSize> bytes{};
};

struct LegacyBattleScriptPanelNode {
    compat::u32 token{};
    compat::u32 value_00{};
    compat::u32 value_04{};
    compat::u32 value_08{};
    compat::u32 value_0c{};
    compat::i16 display_x{};
    compat::i16 display_y{};
    compat::u16 state_98{};
    compat::u16 state_9a{};
    compat::u32 next_token{};
};

struct LegacyBattleScriptPlayerItemQuantity {
    compat::u32 token{};
    compat::u16 primary{};
    compat::u16 secondary{};
};

struct LegacyBattleScriptEffectNode {
    compat::u32 token{};
    compat::u16 type{};
    compat::u16 parameter{};
    compat::i16 x{};
    compat::i16 y{};
    compat::i16 width{};
    compat::i16 height{};
    std::vector<compat::u16> first_words;
    std::vector<compat::u16> second_words;
    compat::u32 next_token{};
};

struct LegacyBattleScriptWorkspace {
    std::vector<LegacyBattleScriptDynamicCommand> dynamic_commands;
    std::vector<LegacyBattleScriptPanelNode> panel_nodes;
    std::vector<LegacyBattleScriptEffectNode> effect_nodes;
    compat::u32 dynamic_command_token{};        // 0x0053CCCC
    compat::u32 dynamic_command_aux_token{};    // 0x0053CCD0
    compat::u32 dynamic_wait_state{};           // 0x0053CCD4
    compat::u32 dynamic_list_token{};           // 0x0053CCD8
    compat::u32 waiting_state{};                // 0x0053CCDC
    compat::u16 waiting_argument{};             // 0x0053CCE4
    compat::i32 value_a{};                      // 0x0053CCE8
    compat::i32 value_b{};                      // 0x0053CCEC
    compat::i32 value_c{};                      // 0x0053CCF0
    std::array<compat::u16, 18> list_words{};   // 0x0053CCF4..0x0053CD17
    std::array<compat::u8, 255> text_buffer{};  // 0x0053CD1C
    std::array<compat::u8, 32> short_text{};    // 0x0053CE3C
    compat::i32 coordinate_x{};                 // 0x0053CE6C
    compat::i32 coordinate_y{};                 // 0x0053CE70
    compat::u16 position_x{};                   // 0x0053CE74
    compat::u16 position_y{};                   // 0x0053CE76
    compat::u16 pair_x{};                       // 0x0053CE78
    compat::u16 pair_y{};                       // 0x0053CE7A
    compat::u32 object_token{};                 // 0x0053CE7C
    compat::u32 text_offset{};                  // 0x0053CE80
    compat::u32 cursor{};                 // 0x0053CE84, script-window offset
    compat::u32 packed_actor_state{};     // 0x0053CE8C
    compat::u32 packed_value_a{};         // 0x0053CE90
    compat::u32 packed_value_b{};         // 0x0053CE94
    compat::u16 word_a{};                 // 0x0053CE98
    compat::u16 word_b{};                 // 0x0053CE9A
    compat::u16 word_c{};                 // 0x0053CE9C
    compat::u16 word_d{};                 // 0x0053CE9E
    compat::u32 list_count{};             // 0x0053CEA0
    compat::u32 frame_after_move_gate{};  // 0x0053CEB0
    compat::u32 completion_gate{};        // 0x0053CEB8
};

// Globals referenced by 0x00469D20 that did not have a pre-existing typed
// owner before this work package. Existing owners stay in the bindings below.
struct LegacyBattleScriptSharedState {
    compat::u32 control_flags{};           // 0x0053BC24
    compat::u32 frame_gate{1U};            // 0x004A7B58
    compat::u32 frame_value{};             // 0x004A7B54
    compat::u32 script_completion_gate{};  // 0x004A7B5C
    compat::u32 script_phase_gate{};       // 0x0053C010
    compat::u32 script_aux_gate{};         // 0x0053C014
    compat::u32 selection_gate_a{};        // 0x0053BFCC
    compat::u32 selection_gate_b{};        // 0x0053BFD8
    compat::u32 selection_gate_c{};        // 0x0053BFDC
    compat::u32 action_completion_gate{};  // 0x0053BFE0
    compat::u32 mode_state{};              // 0x0053BD68
    compat::u16 comparison_word{};         // 0x0053BF34
    compat::u8 actor_mode_count{};         // 0x004A7BAC
    compat::u8 captured_group_a_count{};   // 0x0053BEFF
    compat::u8 published_group_b_count{};  // 0x0053BF00
    compat::u8 published_group_b_aux{};    // 0x0053BF02
    compat::u32 movement_frames{};         // 0x00520FB8
    compat::u32 random_seed{};
    compat::u32 selected_target{};
    compat::u32 action_state{};
    compat::u32 target_selection_block{};
    compat::u32 external_choice{};
    compat::u32 panel_head_token{};
    compat::u32 effect_head_token{};
    compat::u16 effect_mask{};
    compat::u32 attack_order_primary_gate{};
    compat::u32 attack_order_secondary_gate{};
    std::array<compat::u16, 18> actor_target_words{};  // 0x005028AC
    std::array<compat::u32, 18> actor_state_words{};
    std::array<compat::u32, 10> group_a_mirror_x{};          // 0x004FF558
    std::array<compat::u32, 10> group_a_coordinate_table{};  // 0x0052027C
    std::array<compat::u32, 10> group_a_field_2b00{};
    std::array<compat::u32, 10> group_a_field_2b04{};
    std::array<float, 3> movement_start{};
    std::array<float, 3> movement_target{};
    std::array<float, 3> movement_step{};
    std::array<compat::u32, 10> actor_order_workspace{};  // 0x00520DD0
    std::array<compat::u32, 126> attack_order_workspace{};
    std::array<compat::u8, 260> music_path{};  // 0x0053C198
    std::vector<LegacyBattleScriptPlayerItemQuantity> player_items;
};

struct LegacyBattleScriptDispatchBindings {
    const LegacyBattleAssets& assets;
    LegacyBattleStartupState& startup;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleFinalActorStepState& final_actor;
    LegacyBattleInputDispatchState& input_dispatch;
    LegacyBattleTargetSelectionRuntimeState& target_selection;
    LegacyBattleMessagePhaseState& message_phase;
    LegacyBattleVictoryRewardState& victory;
    LegacyBattleScriptSharedState& shared;
    compat::u32& message_state;
};

enum class LegacyBattleScriptDispatchCall : compat::u32 {
    noop_service = 0xFFFFFFFDU,
    message_box = 0xFFFFFFFEU,
    format_dynamic_text = 0x0040AFF0U,
    dispatch_named_text = 0x0040BAA0U,
    finalize_dynamic_text = 0x0040BB20U,
    random_bounded = 0x0040DC00U,
    random_bounded_secondary = 0x0040DC50U,
    random_bounded_tertiary = 0x0040DC80U,
    random_bounded_quaternary = 0x0040DCB0U,
    release_token = 0x00439070U,
    compare_text = 0x0044A240U,
    detach_player_item = 0x0044D620U,
    find_player_item = 0x0044D680U,
    initialize_background = 0x00451940U,
    initialize_battle = 0x00451B10U,
    visual_transition = 0x004527E0U,
    frame = 0x00453200U,
    actor_metrics = 0x0045B0E0U,
    actor_order = 0x0045B190U,
    group_b_order = 0x0045B5A0U,
    global_reset = 0x0045B630U,
    player_item_quantity = 0x0045D180U,
    attack_order_insert = 0x0045EE70U,
    script_page_load = 0x0046E1E0U,
    script_shutdown = 0x0046E260U,
    script_finalize = 0x0046E290U,
    pending_4707b0 = 0x004707B0U,
    pending_475820 = 0x00475820U,
    pending_476160 = 0x00476160U,
    pending_476250 = 0x00476250U,
    pending_476920 = 0x00476920U,
    pending_4769a0 = 0x004769A0U,
    pending_476a10 = 0x00476A10U,
    pending_477bd0 = 0x00477BD0U,
    pending_478330 = 0x00478330U,
    pending_4783b0 = 0x004783B0U,
    pending_478470 = 0x00478470U,
    pending_4785c0 = 0x004785C0U,
    pending_478600 = 0x00478600U,
    pending_478710 = 0x00478710U,
    pending_478780 = 0x00478780U,
    pending_4787d0 = 0x004787D0U,
    pending_4787f0 = 0x004787F0U,
    pending_478830 = 0x00478830U,
    pending_478a70 = 0x00478A70U,
    pending_478ab0 = 0x00478AB0U,
    pending_478ac0 = 0x00478AC0U,
    pending_47c660 = 0x0047C660U,
    pending_47ce80 = 0x0047CE80U,
    pending_47ceb0 = 0x0047CEB0U,
    pending_47cec0 = 0x0047CEC0U,
    pending_47d350 = 0x0047D350U,
    pending_47d640 = 0x0047D640U,
    pending_47d810 = 0x0047D810U,
    pending_47d830 = 0x0047D830U,
    pending_47d860 = 0x0047D860U,
    pending_47d880 = 0x0047D880U,
    pending_47d8b0 = 0x0047D8B0U,
    pending_47d8d0 = 0x0047D8D0U,
    pending_47d900 = 0x0047D900U,
    pending_47d950 = 0x0047D950U,
    pending_47da10 = 0x0047DA10U,
    pending_47da90 = 0x0047DA90U,
    pending_47e880 = 0x0047E880U,
    pending_47f150 = 0x0047F150U,
    pending_47f3a0 = 0x0047F3A0U,
    pending_47f900 = 0x0047F900U,
    pending_47f910 = 0x0047F910U,
    pending_482ec0 = 0x00482EC0U,
    pending_4838a0 = 0x004838A0U,
    pending_484500 = 0x00484500U,
    sample_play = 0x00485610U,
    stream_start = 0x004856C0U,
    stream_stop = 0x00485710U,
    stream_set_volume = 0x00485850U,
    allocate = 0x00487C10U,
    release_allocation = 0x004885A0U,
    x87_truncate = 0x00489654U,
};

struct LegacyBattleScriptDispatchCallRequest {
    LegacyBattleScriptDispatchCall call{LegacyBattleScriptDispatchCall::frame};
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 argument_count{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 cursor{};
};

struct LegacyBattleScriptDispatchCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool typed_stop{};
};

class LegacyBattleScriptDispatchPort {
public:
    virtual ~LegacyBattleScriptDispatchPort() = default;

    [[nodiscard]] virtual LegacyBattleScriptDispatchCallReply
    invoke_battle_script(
        LegacyBattleScriptWorkspace&,
        LegacyBattleScriptDispatchBindings&,
        const LegacyBattleScriptDispatchCallRequest& request
    ) {
        return {
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
    }
};

enum class LegacyBattleScriptDispatchStatus : compat::u8 {
    completed,
    script_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    shared_state_typed_stop,
    allocation_typed_stop,
    player_item_typed_stop,
    attack_order_typed_stop,
    divide_by_zero_typed_stop,
    divide_overflow_typed_stop,
    string_typed_stop,
    closed_callee_typed_stop,
    script_page_load_typed_stop,
};

struct LegacyBattleScriptDispatchRequest {
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

struct LegacyBattleScriptDispatchResult {
    LegacyBattleScriptDispatchStatus status{
        LegacyBattleScriptDispatchStatus::completed
    };
    compat::i32 opcode{};
    compat::u32 cursor_before{};
    compat::u32 cursor_after{};
    compat::u32 return_eax{1U};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 stopped_offset{};
    std::vector<LegacyBattleScriptDispatchCall> call_trace;
};

[[nodiscard]] LegacyBattleScriptDispatchResult
run_legacy_battle_script_dispatch(
    LegacyBattleScriptWorkspace& workspace,
    LegacyBattleScriptDispatchBindings bindings,
    LegacyBattleScriptDispatchPort& port,
    const LegacyBattleScriptDispatchRequest& request = {}
);

}  // namespace openswd3::battle
