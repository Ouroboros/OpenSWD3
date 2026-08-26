#pragma once

#include "openswd3/battle/legacy_battle_actor_metrics.hpp"
#include "openswd3/battle/legacy_battle_debug_hotkeys.hpp"
#include "openswd3/battle/legacy_battle_effect_coordinator.hpp"
#include "openswd3/battle/legacy_battle_final_actor_step.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <array>
#include <optional>
#include <string>

namespace openswd3::battle {

enum class LegacyBattleDebugOverlayCall : compat::u8 {
    font_style,
    font_reset,
    resolve_group_b_actor,
    query_group_b_vitality,
    query_actor_command,
    query_actor_lock,
    query_marker_position,
    query_marker_width,
};

struct LegacyBattleDebugOverlayCallRequest {
    LegacyBattleDebugOverlayCall call{};
    compat::u32 object_token{};
    std::array<compat::u32, 4> arguments{};
    compat::u32 argument_count{};
};

struct LegacyBattleDebugOverlayCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 output_mask{};
    compat::u32 output_0{};
    compat::u32 output_1{};
};

struct LegacyBattleDebugOverlayTextRequest {
    compat::u32 font_token{};
    compat::u32 surface_token{};
    compat::u32 x{};
    compat::u32 y{};
    std::string text;
    compat::u32 foreground{};
    compat::u32 height{};
};

class LegacyBattleDebugOverlayPort {
public:
    virtual ~LegacyBattleDebugOverlayPort() = default;

    [[nodiscard]] virtual LegacyBattleDebugOverlayCallReply
    invoke_debug_overlay(const LegacyBattleDebugOverlayCallRequest&) {
        return {};
    }

    [[nodiscard]] virtual std::optional<compat::u16>
    read_debug_actor_level_word_54(compat::u32) {
        return std::nullopt;
    }

    [[nodiscard]] virtual LegacyBattleDebugOverlayCallReply
    draw_debug_overlay_text(const LegacyBattleDebugOverlayTextRequest&) {
        return {};
    }
};

struct LegacyBattleDebugOverlayState {
    compat::u32 gate{};
    compat::u32 resolved_actor_token{};
    std::array<compat::u32, 18> selection_order{};

    compat::i16 battle_selector{-1};
    compat::u32 battle_mode{};
    compat::u32 message_status{};
    compat::u32 selection_status{};
    compat::u32 lock_count{};
    compat::u32 tsw_cache_bytes{};
    compat::i16 initial_mode{-1};
    compat::i8 world_level{};
    compat::u32 battle_frame{};
    compat::i32 frame_divisor{1};

    compat::i16 marker_x{};
    compat::i16 marker_row{};

    std::array<char, 255> text_buffer{};
};

struct LegacyBattleDebugOverlayBindings {
    LegacyBattleDebugOverlayState& overlay;
    LegacyBattleDebugHotkeyState& hotkeys;
    LegacyBattleActorMetricState& metrics;
    LegacyBattleStartupState& startup;
    LegacyBattleFinalActorStepState& final_actor;
    compat::u32& message_state;
    LegacyBattleEffectCoordinatorState& effects;
    rendering::LegacyFramebuffer& framebuffer;
};

struct LegacyBattleDebugOverlayRequest {
    compat::u32 vitality_stack_snapshot{};
};

enum class LegacyBattleDebugOverlayStatus : compat::u8 {
    completed,
    resolved_actor_word_typed_stop,
    startup_record_typed_stop,
    actor_order_typed_stop,
    selection_order_typed_stop,
    framebuffer_typed_stop,
    frame_divisor_zero,
};

struct LegacyBattleDebugOverlayResult {
    LegacyBattleDebugOverlayStatus status{
        LegacyBattleDebugOverlayStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 formatted_texts{};
    compat::u32 text_draws{};
    compat::u32 group_b_rows{};
    compat::u32 group_a_rows{};
    compat::u32 startup_order_rows{};
    compat::u32 actor_order_rows{};
    compat::u32 selection_order_rows{};
    compat::u32 marker_actors{};
    compat::u32 marker_pixels{};
};

// The typed battle debug overlay preserves the fixed font prologue/tail,
// CP950 diagnostic strings, dynamic actor-count rereads, direct two-row marker
// writes, and the original signed frame-rate division point.
[[nodiscard]] LegacyBattleDebugOverlayResult draw_legacy_battle_debug_overlay(
    LegacyBattleDebugOverlayBindings bindings,
    LegacyBattleDebugOverlayPort& port,
    const LegacyBattleDebugOverlayRequest& request = {}
);

}  // namespace openswd3::battle
