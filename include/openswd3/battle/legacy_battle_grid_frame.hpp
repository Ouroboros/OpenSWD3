#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_action_frame_draw.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

enum class LegacyBattleGridFrameCall : compat::u8 {
    configure_font_mode,
    configure_font_style,
    configure_font_width,
    initialize_rows,
    refresh_actor,
    query_row,
    draw_text,
    query_alternate_row,
    query_mode_row,
    query_mode_secondary_count,
    initialize_narrow_rows,
    query_narrow_row,
};

struct LegacyBattleGridFrameCallRequest {
    LegacyBattleGridFrameCall call{
        LegacyBattleGridFrameCall::configure_font_mode
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

struct LegacyBattleGridFrameCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_panel_row_limit{};
    compat::u16 panel_row_limit{};
    bool publish_row_flags{};
    compat::u32 row_flags{};
    bool publish_row_value{};
    compat::u32 row_value{};
    bool publish_row_text{};
    std::array<compat::u8, 20> row_text{};
};

class LegacyBattleGridFramePort
    : public virtual LegacyBattleOffsetActionFrameDrawStatePort {
public:
    virtual ~LegacyBattleGridFramePort() = default;

    [[nodiscard]] virtual LegacyBattleGridFrameCallReply
    invoke_grid_frame(const LegacyBattleGridFrameCallRequest& request) = 0;
};

struct LegacyBattleGridFrameState {
    std::array<compat::u8, 20> row_text{};
    std::array<compat::u8, 20> numeric_text{};
    compat::u32 row_flags{};
    compat::u32 row_value{};
    compat::u32 numeric_text_length{};
};

struct LegacyBattleGridFrameBindings {
    const compat::u32& queued_actor_code;
    const compat::u32& action_category_index;
    compat::u16& panel_row_limit;
    compat::u32& selection_input_gate;
    compat::u32& target_argument;
    const compat::u16& primary_text_color;
    const compat::u16& secondary_text_color;
    const std::array<compat::u32, 10>& actor_description_record_tokens;
    const std::array<compat::u16, 10>& actor_description_text_indices;
    std::span<const compat::u8> maps_payload;
    std::span<compat::u8> shared_text;
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

struct LegacyBattleGridFrameRegisterSnapshot {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleGridFrameRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 selected_row{};
    compat::u32 scroll_offset{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 row_text_token{};
    compat::u32 row_flags_token{};
    compat::u32 row_value_token{};
    compat::u32 numeric_text_token{};
    std::array<compat::u32, 5> action_update_edx_snapshots{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 5>
        action_frame_return_registers{};
    LegacyBattleGridFrameRegisterSnapshot panel_rectangle_return_registers{};
    LegacyBattleGridFrameRegisterSnapshot tiled_frame_return_registers{};
    std::array<LegacyBattleGridFrameRegisterSnapshot, 7>
        format_return_registers{};
};

enum class LegacyBattleGridFrameStatus : compat::u8 {
    completed,
    action_frame_typed_stop,
    panel_rectangle_typed_stop,
    tiled_frame_typed_stop,
    group_a_actor_typed_stop,
    first_selection_rectangle_typed_stop,
    second_selection_rectangle_typed_stop,
    shared_text_typed_stop,
};

struct LegacyBattleGridFrameRowTrace {
    compat::u32 iterator{};
    compat::u32 flags{};
    compat::u32 value{};
    compat::u32 displayed_row{};
    bool selected{};
    bool secondary_color{};
    compat::u32 numeric_text_length{};
    std::array<compat::u8, 20> row_text{};
    std::array<compat::u8, 20> numeric_text{};
};

struct LegacyBattleGridFrameResult {
    LegacyBattleGridFrameStatus status{LegacyBattleGridFrameStatus::completed};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 action_frame_calls{};
    compat::u32 font_calls{};
    compat::u32 panel_action_update_calls{};
    compat::u32 panel_rectangle_calls{};
    compat::u32 tiled_frame_calls{};
    compat::u32 actor_initialization_calls{};
    compat::u32 actor_refresh_calls{};
    compat::u32 row_query_calls{};
    compat::u32 scanned_rows{};
    compat::u32 hidden_rows{};
    compat::u32 displayed_rows{};
    compat::u32 text_draw_calls{};
    compat::u32 selection_rectangle_calls{};
    compat::u32 shared_text_resolution_calls{};
    compat::u32 shared_text_length_calls{};
    compat::u32 final_iterator{};
    compat::u32 selected_iterator{};
    std::array<LegacyBattleOffsetActionFrameDrawResult, 5> action_frames{};
    asset_runtime::LegacyActionUpdateResult panel_action_update{};
    rendering::LegacyRectangleEffectStatus panel_rectangle_status{
        rendering::LegacyRectangleEffectStatus::completed
    };
    rendering::LegacyTiledFrameResult tiled_frame{};
    std::array<LegacyBattleGridFrameRowTrace, 7> rows{};
    std::array<rendering::LegacyRectangleEffectStatus, 2>
        selection_rectangle_statuses{
            rendering::LegacyRectangleEffectStatus::completed,
            rendering::LegacyRectangleEffectStatus::completed,
        };
};

// Typed closure of legacy 0x004659C0.
[[nodiscard]] LegacyBattleGridFrameResult draw_legacy_battle_grid_frame(
    LegacyBattleGridFrameState& state,
    LegacyBattleGridFrameBindings bindings,
    LegacyBattleGridFramePort& port,
    const LegacyBattleGridFrameRequest& request
);

}  // namespace openswd3::battle
