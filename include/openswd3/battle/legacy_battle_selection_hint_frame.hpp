#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_color_fade.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleSelectionHintGroupBBaseToken =
    0x005229E0U;
inline constexpr compat::u32 kLegacyBattleSelectionHintTextObjectToken =
    0x004C9A28U;
inline constexpr compat::u32 kLegacyBattleSelectionHintFontToken = 0x004CD76CU;
inline constexpr compat::u32 kLegacyBattleSelectionHintMetricOwnerToken =
    0x004B9F00U;
inline constexpr std::array<compat::u8, 5>
    kLegacyBattleSelectionHintVitalityPrefix{0xA5U, 0xCDU, 0xA9U, 0x52U, 0x3AU};

struct LegacyBattleSelectionHintFrameState {
    LegacyBattleColorFadeState color_fade{};
};

enum class LegacyBattleSelectionHintFrameCall : compat::u8 {
    query_actor_label,
    configure_font_width,
    draw_text,
    query_metric_source,
    resolve_metric_value,
    query_metric_pair,
    query_fade_width,
    query_fade_color,
};

struct LegacyBattleSelectionHintFrameCallRequest {
    LegacyBattleSelectionHintFrameCall call{
        LegacyBattleSelectionHintFrameCall::query_actor_label
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 text_token{};
    std::array<compat::u8, 20> text_bytes{};
    compat::u32 text_length{};
};

struct LegacyBattleSelectionHintFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 text_length{};
    bool publish_metric_pair{};
    compat::u32 metric_current{};
    compat::u32 metric_limit{};
};

class LegacyBattleSelectionHintFramePort {
public:
    virtual ~LegacyBattleSelectionHintFramePort() = default;

    [[nodiscard]] virtual LegacyBattleSelectionHintFrameCallReply
    invoke_selection_hint_frame(
        const LegacyBattleSelectionHintFrameCallRequest& request
    ) = 0;
};

struct LegacyBattleSelectionHintFrameBindings {
    LegacyBattleSelectionHintFrameState& state;
    compat::u32 queued_actor_code{};
    std::span<const compat::u32> party_source_words;
    compat::u32 target_selection_block{};
    compat::u32 published_actor_code{};
    compat::u32 group_b_count{};
    compat::u32 mirror_mode{};
    asset_runtime::LegacyActionRecord& panel_action_record;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    asset_runtime::LegacyActionUpdater& action_updater;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleSelectionHintRegisterSnapshot {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleSelectionHintFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 local_text_token{};
    compat::u32 local_current_token{};
    compat::u32 local_limit_token{};
    LegacyBattleSelectionHintRegisterSnapshot
        panel_rectangle_return_registers{};
    LegacyBattleSelectionHintRegisterSnapshot tiled_frame_return_registers{};
    LegacyBattleSelectionHintRegisterSnapshot color_fade_return_registers{};
};

enum class LegacyBattleSelectionHintFrameStatus : compat::u8 {
    completed,
    party_source_typed_stop,
    group_b_actor_typed_stop,
    panel_rectangle_typed_stop,
    tiled_frame_typed_stop,
    format_buffer_typed_stop,
    color_fade_typed_stop,
};

struct LegacyBattleSelectionHintFrameResult {
    LegacyBattleSelectionHintFrameStatus status{
        LegacyBattleSelectionHintFrameStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 actor_label_query_calls{};
    compat::u32 font_width_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 metric_source_calls{};
    compat::u32 metric_value_calls{};
    compat::u32 metric_pair_calls{};
    compat::u32 fade_width_calls{};
    compat::u32 fade_color_calls{};
    compat::u32 panel_action_update_calls{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    compat::u32 panel_rectangle_calls{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    compat::u32 tiled_frame_calls{};
    rendering::LegacyTiledFrameResult tiled_frame{};
    compat::u32 color_fade_calls{};
    rendering::LegacyBlitExecutionStatus color_fade_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
    compat::u32 actor_code{};
    compat::u32 actor_token{};
    compat::u32 label_token{};
    compat::u32 label_length{};
    compat::u32 label_character_count{};
    compat::u32 panel_x{};
    compat::u32 panel_y{};
    compat::u32 metric_value{};
    compat::u32 fade_width{};
    compat::u32 fade_color{};
    std::array<compat::u8, 20> formatted_text{};
    compat::u32 formatted_text_length{};
    bool label_drawn{};
    bool metric_text_drawn{};
    bool fade_drawn{};
};

[[nodiscard]] LegacyBattleSelectionHintFrameResult
draw_legacy_battle_selection_hint_frame(
    LegacyBattleSelectionHintFrameBindings bindings,
    LegacyBattleSelectionHintFramePort& port,
    const LegacyBattleSelectionHintFrameRequest& request
);

}  // namespace openswd3::battle
