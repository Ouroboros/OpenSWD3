#pragma once

#include "openswd3/audio_video/legacy_sample_manager.hpp"
#include "openswd3/battle/legacy_battle_color_accumulation.hpp"
#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_pair_transition.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace openswd3::battle {

enum class LegacyBattleGlobalResetCall : compat::u8 {
    release_conditional_allocation,
    release_pre_battle_resource_431960,
    release_pre_battle_resource_433010,
    suspend_audio_stream_485710,
    initialize_post_reset_4776a0,
};

struct LegacyBattleGlobalResetCallReply {
    compat::u32 eax{};
};

class LegacyBattleGlobalResetRuntimePort
    : public LegacyBattleStartupPort,
      public virtual LegacyBattleColorAccumulationStatePort,
      public virtual LegacyBattlePairTransitionStatePort,
      public virtual LegacyBattleEffectCoordinatorStatePort,
      public LegacyBattleActionRotationReleasePort,
      public LegacyBattleRenderAuxiliaryBufferReleaser {
public:
    ~LegacyBattleGlobalResetRuntimePort() override = default;

    [[nodiscard]] virtual LegacyBattleGlobalResetCallReply
    invoke_reset(LegacyBattleGlobalResetCall call, compat::u32 argument) = 0;

    [[nodiscard]] virtual audio_video::LegacySampleManager&
    sample_manager() noexcept = 0;
};

struct LegacyBattleGlobalResetWrite {
    compat::u32 address{};
    compat::u32 size{};
    compat::u32 count{};
    compat::u32 value{};
};

struct LegacyBattleGlobalResetState {
    std::unordered_map<compat::u32, compat::u8> unmapped_bytes;
    std::vector<LegacyBattleGlobalResetWrite> write_trace;
};

enum class LegacyBattleGlobalResetCallStage : compat::u8 {
    display_surfaces,
    rotation_cache,
    render_resources,
    conditional_allocation,
    pre_battle_resource_431960,
    pre_battle_resource_433010,
    all_samples,
    audio_stream,
    post_reset_initialization,
};

struct LegacyBattleGlobalResetResult {
    LegacyBattleDisplaySurfaceReleaseResult display_surfaces{};
    LegacyBattleActionRotationReleaseResult rotation_cache{};
    LegacyBattleRenderCleanupResult render_resources{};
    compat::u32 conditional_allocation_token{};
    bool conditional_allocation_released{};
    std::array<LegacyBattleGlobalResetCallStage, 9> call_order{};
    compat::u32 call_count{};
    compat::u32 write_operations{};
    compat::u32 physical_writes{};
    compat::u32 bytes_written{};
    compat::u32 return_value{};
};

// sub_45B630.
[[nodiscard]] LegacyBattleGlobalResetResult reset_legacy_battle_globals(
    LegacyBattleGlobalResetState& state,
    LegacyBattleStartupState& startup,
    LegacyBattleFinalActorStepState& final_actor,
    LegacyBattleActionDispatchState& action,
    LegacyBattleGlobalResetRuntimePort& port
);

}  // namespace openswd3::battle
