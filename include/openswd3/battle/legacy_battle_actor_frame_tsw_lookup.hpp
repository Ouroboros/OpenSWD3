#pragma once

#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

namespace openswd3::asset_runtime {
class LegacyTswRuntime;
}

namespace openswd3::battle {

// Bridges the existing action-record update and the real TSW cache lookup.
// A failed/unmodeled frame load does not claim a normal sub_4315D0 return.
class LegacyBattleActorFrameTswUpdatePort final
    : public LegacyBattleActorFrameUpdatePort {
public:
    LegacyBattleActorFrameTswUpdatePort(
        asset_runtime::LegacyTswRuntime& tsw,
        LegacyBattleActorFrameUpdatePort& action_update
    ) noexcept;

    [[nodiscard]] bool models_fast_action_return() const noexcept override {
        return true;
    }

    [[nodiscard]] LegacyBattleActorFrameUpdateReply update(
        asset_runtime::LegacyActionRecord& record,
        compat::u32 record_token,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx
    ) override;

    [[nodiscard]] LegacyBattleActorFrameUpdateReply lookup_frame(
        compat::u32 action_value,
        compat::u32 argument_zero,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx
    ) override;

private:
    asset_runtime::LegacyTswRuntime& tsw_;
    LegacyBattleActorFrameUpdatePort& action_update_;
};

}  // namespace openswd3::battle
