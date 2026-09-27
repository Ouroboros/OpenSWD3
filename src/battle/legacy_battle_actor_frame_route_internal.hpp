#pragma once

#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

namespace openswd3::battle::actor_frame_route_detail {

enum class RouteStepOutcome { unhandled, advance, stop };

[[nodiscard]] RouteStepOutcome advance_phase_1(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
);

[[nodiscard]] RouteStepOutcome advance_phase_2(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
);

[[nodiscard]] RouteStepOutcome advance_phase_3(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
);

[[nodiscard]] RouteStepOutcome advance_phase_4(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
);

[[nodiscard]] RouteStepOutcome advance_phase_5(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
);

[[nodiscard]] RouteStepOutcome advance_phase_6(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameEntryResult& prefix
);

}  // namespace openswd3::battle::actor_frame_route_detail
