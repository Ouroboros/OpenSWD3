#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/battle/legacy_battle_animation_collision.hpp"
#include "openswd3/battle/legacy_battle_color_accumulation.hpp"
#include "openswd3/battle/legacy_battle_frame_refresh.hpp"
#include "openswd3/battle/legacy_battle_group_a_reward_profile_state.hpp"
#include "openswd3/battle/legacy_battle_pair_transition.hpp"
#include "openswd3/battle/legacy_battle_shared_phase.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <initializer_list>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleEffectPrimaryBaseToken = 0x005202A8U;
inline constexpr compat::u32 kLegacyBattleEffectAlternateBaseToken =
    0x004FE600U;
inline constexpr compat::u32 kLegacyBattleEffectRecordStride = 0x98U;

struct LegacyBattleEffectCallRequest {
    compat::u32 callee_token{};
    std::array<compat::u32, 12> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleEffectCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, 8> outputs{};
    compat::u32 output_write_mask{};
};

class LegacyBattleEffectCallPort
    : public virtual LegacyBattleActorCoordinateBindingsStatePort,
      public virtual LegacyBattleActorMetricStatePort,
      public virtual LegacyBattleActorPublicationStatePort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public virtual LegacyBattlePairTransitionPort,
      public virtual LegacyBattleGroupARewardProfileStatePort,
      public virtual LegacyBattleSharedPhaseStatePort,
      public virtual LegacyBattleFrameRefreshStatePort {
public:
    virtual ~LegacyBattleEffectCallPort() = default;

    [[nodiscard]] virtual LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) = 0;
};

struct LegacyBattleIntensityEffectRecord {
    compat::u32 source_value{};
    compat::u32 value_04{};
    compat::u32 secondary_value{};
    compat::u32 value_0c{};
    compat::u32 x_offset{};
    compat::u32 y_offset{};
    compat::u32 render_flags{};
    std::array<compat::u8, 0x2E> unknown_1c{};
    compat::u16 lookup_key_a{};
    compat::u16 lookup_key_b{};
    std::array<compat::u8, 0x42> unknown_4e{};
    compat::u32 mode_snapshot{};
    compat::u32 value_94{};
};

static_assert(sizeof(LegacyBattleIntensityEffectRecord) == 0x98U);
static_assert(
    offsetof(LegacyBattleIntensityEffectRecord, lookup_key_a) == 0x4AU
);
static_assert(
    offsetof(LegacyBattleIntensityEffectRecord, mode_snapshot) == 0x90U
);

struct LegacyBattleEffectRecord {
    compat::u32 source_value{};
    compat::u32 zero_value{};
    compat::u32 base_offset{};
    compat::u16 base_y_offset{};
    compat::u32 render_flags{};
    compat::u32 resource_key_token{};
    compat::u32 resource_aux_value{};
    compat::u16 lookup_key_a{};
    compat::u16 lookup_key_b{};
    compat::u16 pan_value{};
    compat::u16 status_flags{};
    compat::u16 shared_word_36{};
    compat::u16 shared_word_38{};
    compat::u16 shared_word_3a{};
    compat::u16 width_adjustment{};
    compat::u16 y_adjustment{};
    std::array<compat::u16, 7> action_values{};
    compat::u32 complete{};
    compat::u32 mode_snapshot{};
};

enum class LegacyBattleIntensityEffectFrameStatus : compat::u8 {
    completed,
    slot_index_typed_stop,
    actor_coordinate_typed_stop,
    resource_owner_typed_stop,
};

struct LegacyBattleIntensityEffectFrameResult {
    LegacyBattleIntensityEffectFrameStatus status{
        LegacyBattleIntensityEffectFrameStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_edx{};
    compat::u32 port_calls{};
    LegacyBattleActorCoordinateQueryResult coordinate_query{};
    compat::u32 coordinate_query_calls{};
};

inline constexpr std::size_t kLegacyBattleEffectActorSlotCount = 18U;

struct LegacyBattleSharedEffectFrameState
    : public LegacyBattleAnimationCollisionState {
    std::array<LegacyBattleEffectRecord, kLegacyBattleEffectActorSlotCount>
        primary{};
    std::array<LegacyBattleEffectRecord, kLegacyBattleEffectActorSlotCount>
        alternate{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        alternate_active{};

    compat::u32 global_mode{};
    compat::u32 global_flip_mode{};
    compat::u32 sample_handle_value{};
    compat::u32 current_resource_value_token{};

    compat::u16 shared_word_36{};
    compat::u16 shared_word_38{};
    compat::u16 shared_word_3a{};
    compat::u32 primary_suppression{};
    compat::u32 split_suppression{};

    compat::u32 battle_gate{};

    compat::i32 reward_value{};
    compat::u32 reward_display_total{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        reward_auxiliary{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount> reward_total{};
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount> reward_high{};
};

struct LegacyBattleEffectFrameState
    : public virtual LegacyBattleSharedEffectFrameState {
    std::array<compat::u32, kLegacyBattleEffectActorSlotCount>
        animation_counter{};

    compat::u32 animation_mode{};
    compat::u32 effect_object_token{};
    compat::u32 target_surface_token{};

    compat::u32 battle_byte_flags{};
    compat::u32 resolved_actor_value{};

    std::array<compat::u32, kLegacyBattleEffectActorSlotCount> pending_step{};

    std::array<LegacyBattleIntensityEffectRecord, 8> intensity_records{};
    std::array<compat::i8, 8> intensity_values{};
    compat::i32 render_intensity_a{};
    compat::i32 render_intensity_b{};
    compat::i32 render_intensity_c{};
};

enum class LegacyBattleEffectFrameStatus : compat::u8 {
    completed,
    slot_index_typed_stop,
    argument_object_typed_stop,
    resource_owner_typed_stop,
    group_a_actor_typed_stop,
    group_b_actor_typed_stop,
    animation_collision_counter_typed_stop,
};

struct LegacyBattleEffectFrameResult {
    LegacyBattleEffectFrameStatus status{
        LegacyBattleEffectFrameStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 port_calls{};
    compat::u32 color_initialization_calls{};
    compat::u32 primary_animation_steps{};
    compat::u32 alternate_animation_steps{};
    LegacyBattleActorCoordinateQueryResult coordinate_query{};
    compat::u32 coordinate_query_calls{};
    LegacyBattleAnimationCollisionResult animation_collision{};
    compat::u32 animation_collision_calls{};
};

// Typed closure of legacy 0x00459BF0. The source-zero return precedes slot
// access; caller-supplied EDX is retained until an original callee overwrites
// it, and resource-owner access stops only at the original dereference.
[[nodiscard]] LegacyBattleIntensityEffectFrameResult
advance_legacy_battle_intensity_effect_frame(
    LegacyBattleEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    compat::u32 actor_token,
    compat::u32 source_value,
    compat::u32 secondary_value,
    compat::u32 slot_index
);

// Typed closure of legacy 0x004582B0. Physical addresses are published only
// as 32-bit tokens; record, argument-object, resource-owner, and stale-register
// effects stop at their original first access.
[[nodiscard]] LegacyBattleEffectFrameResult advance_legacy_battle_effect_frame(
    LegacyBattleEffectFrameState& state,
    LegacyBattleEffectCallPort& port,
    compat::u32 actor_index,
    compat::u32 argument_object_token,
    compat::u32 argument_mode_gate,
    compat::u32 source_value,
    compat::u32 slot_index
);

}  // namespace openswd3::battle
