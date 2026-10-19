#pragma once

#include <cstddef>

#include "battle/battle_state.h"
#include "core/types.h"
#include "platform/display.h"

// The constants of the battle frame, its background and its role panels that battle_frame.cpp, battle_scene.cpp,
// battle_panels.cpp and smooth/smooth_battle.cpp share.
namespace swd3
{

constexpr std::size_t kFieldPixels = static_cast<std::size_t>(Display::Width) * BattleState::FieldHeight;
}  // namespace swd3
