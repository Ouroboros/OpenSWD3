#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <initializer_list>

namespace openswd3::battle {

struct LegacyBattleHudCallRequest {
    compat::u32 callee_token{};
    std::array<compat::u32, 12> arguments{};
};

struct LegacyBattleHudCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    std::array<compat::u32, 8> outputs{};
};

class LegacyBattleHudCallPort {
public:
    virtual ~LegacyBattleHudCallPort() = default;

    [[nodiscard]] virtual LegacyBattleHudCallReply
    invoke_hud(const LegacyBattleHudCallRequest&) {
        return {};
    }
};

struct LegacyBattleHudFrameState {
    compat::i32 active_actor_count{};
    compat::u32 side_mode{};
    std::array<compat::u32, 10> actor_active{};

    std::array<compat::u32, 10> actor_skip_primary{};
    std::array<compat::u32, 10> actor_skip_secondary{};
    std::array<compat::u32, 10> actor_status_mode{};
    std::array<compat::u32, 10> display_order{};
    std::array<compat::i32, 10> status_x{};
    std::array<compat::i32, 10> value_x{};
    std::array<compat::i32, 10> bar_x{};

    compat::i32 selected_actor_code{};
    compat::u8 selected_pulse{};
    compat::u32 selected_pulse_counter{};
    compat::i8 top_pulse{};
    std::array<compat::u32, 10> status_blink_counter{};

    std::array<compat::u32, 10> actor_value_tokens{};
    std::array<compat::i32, 10> actor_value{};
    std::array<compat::i32, 10> actor_value_display{};
    std::array<compat::i32, 10> actor_value_target{};

    std::array<compat::i32, 10> primary_value_snapshot{};
    std::array<compat::i32, 10> primary_delta{};
    std::array<compat::i32, 10> primary_step{};
    std::array<compat::i32, 10> primary_display{};
    std::array<compat::i32, 10> primary_display_target{};

    std::array<compat::i32, 10> secondary_value_snapshot{};
    std::array<compat::i32, 10> secondary_delta{};
    std::array<compat::i32, 10> secondary_step{};
    std::array<compat::i32, 10> secondary_display{};
    std::array<compat::i32, 10> secondary_display_target{};

    std::array<compat::i32, 10> tertiary_value_snapshot{};
    std::array<compat::i32, 10> tertiary_delta{};
    std::array<compat::i32, 10> tertiary_step{};
    std::array<compat::i32, 10> tertiary_display{};
    std::array<compat::i32, 10> tertiary_display_target{};

    compat::u32 footer_mode{};
    compat::i32 footer_position{};
    compat::i32 footer_delta{};

    LegacyBattleHudFrameState() noexcept;
};

enum class LegacyBattleHudFrameStatus : compat::u8 {
    completed,
    actor_index_typed_stop,
    display_order_typed_stop,
    display_table_typed_stop,
    actor_value_typed_stop,
};

struct LegacyBattleHudFrameResult {
    LegacyBattleHudFrameStatus status{LegacyBattleHudFrameStatus::completed};
    compat::u32 return_value{};
    compat::u32 port_calls{};
    compat::u32 top_actor_rows{};
    compat::u32 actor_rows{};
    compat::u32 x87_conversions{};
};

// Typed closure of legacy 0x00459D10. One call renders and advances the battle
// HUD while preserving fixed ten-actor storage, low-32-bit arithmetic, signed
// byte pulses, bit-27 delta handling, and the original x87 conversion domain.
[[nodiscard]] LegacyBattleHudFrameResult advance_legacy_battle_hud_frame(
    LegacyBattleHudFrameState& state, LegacyBattleHudCallPort& port
);

}  // namespace openswd3::battle
