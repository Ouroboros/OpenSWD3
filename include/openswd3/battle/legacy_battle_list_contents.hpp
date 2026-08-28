#pragma once

#include "openswd3/battle/legacy_battle_frame_draw.hpp"
#include "openswd3/rendering/legacy_rectangle_effect.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

enum class LegacyBattleListContentsCall : compat::u8 {
    configure_font_mode,
    configure_font_style,
    configure_font_width,
    initialize_rows,
    refresh_actor,
    query_row,
    resolve_negative_row,
    resolve_regular_row,
    draw_text,
};

struct LegacyBattleListContentsCallRequest {
    LegacyBattleListContentsCall call{
        LegacyBattleListContentsCall::configure_font_mode
    };
    compat::u32 object_token{};
    std::array<compat::u32, 8> arguments{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 text_token{};
    std::array<compat::u8, 10> text_bytes{};
    compat::u32 text_length{};
};

struct LegacyBattleListContentsCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool publish_panel_row_limit{};
    compat::u8 panel_row_limit{};
    bool publish_row_value{};
    compat::u32 row_value{};
    bool publish_limit_word{};
    compat::u16 limit_word{};
    bool publish_limit_byte{};
    compat::u8 limit_byte{};
};

class LegacyBattleListContentsPort {
public:
    virtual ~LegacyBattleListContentsPort() = default;

    [[nodiscard]] virtual LegacyBattleListContentsCallReply
    invoke_list_contents(
        const LegacyBattleListContentsCallRequest& request
    ) = 0;
};

struct LegacyBattleListContentsState {
    LegacyBattleFrameDrawState resource_frame{};
    std::array<compat::u8, 10> numeric_text{};
    compat::u32 numeric_text_length{};
    compat::u16 local_limit_word{};
    compat::u8 local_limit_byte{};
};

struct LegacyBattleListContentsBindings {
    const compat::u32& queued_actor_code;
    const compat::u32& action_category_index;
    compat::u8& panel_row_limit;
    compat::u32& selection_input_gate;
    compat::u32& candidate_argument;
    const compat::u16& primary_text_color;
    const compat::u16& secondary_text_color;
    const std::array<compat::u32, 10>& actor_description_record_tokens;
    const std::array<compat::u16, 10>& actor_description_text_indices;
    std::span<const compat::u8> maps_payload;
    std::span<compat::u8> shared_text;
    rendering::LegacyFramebuffer& framebuffer;
    const rendering::LegacyBlitClipRectangle& clip;
    rendering::LegacyRasterGeometryState& raster;
    rendering::LegacyBlitRequest& shared_request;
    rendering::LegacyBlitEffectState& shared_effects;
    rendering::LegacyRleRowJitterState& jitter;
    rendering::LegacyFramePieceProvider& frame_provider;
};

struct LegacyBattleListContentsRegisterSnapshot {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleListContentsRequest {
    compat::u32 origin_x{};
    compat::u32 origin_y{};
    compat::u32 selected_row{};
    compat::u32 scroll_offset{};
    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    compat::u32 local_value_token{};
    compat::u32 local_limit_word_token{};
    compat::u32 local_limit_byte_token{};
    compat::u32 numeric_text_token{};
    compat::u16 initial_limit_word{};
    compat::u8 initial_limit_byte{};
    std::array<LegacyBattleListContentsRegisterSnapshot, 7>
        resource_frame_return_registers{};
    std::array<LegacyBattleListContentsRegisterSnapshot, 7>
        format_return_registers{};
};

enum class LegacyBattleListContentsStatus : compat::u8 {
    completed,
    group_a_actor_typed_stop,
    resource_frame_typed_stop,
    first_rectangle_typed_stop,
    second_rectangle_typed_stop,
    shared_text_typed_stop,
};

struct LegacyBattleListContentsRowTrace {
    compat::u32 iterator{};
    compat::u32 query_return{};
    compat::u32 raw_value{};
    compat::u32 displayed_value{};
    compat::u16 limit_word{};
    compat::u8 limit_byte{};
    bool negative_selector{};
    bool resource_drawn{};
    bool limit_is_less{};
    bool selected{};
    compat::u32 numeric_text_length{};
    std::array<compat::u8, 10> numeric_text{};
};

struct LegacyBattleListContentsResult {
    LegacyBattleListContentsStatus status{
        LegacyBattleListContentsStatus::completed
    };
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 port_calls{};
    compat::u32 actor_initialization_calls{};
    compat::u32 actor_refresh_calls{};
    compat::u32 row_query_calls{};
    compat::u32 row_resolver_calls{};
    compat::u32 text_draw_calls{};
    compat::u32 resource_frame_calls{};
    compat::u32 rectangle_calls{};
    compat::u32 shared_text_resolution_calls{};
    compat::u32 shared_text_length_calls{};
    compat::u32 completed_rows{};
    compat::u32 final_iterator{};
    compat::u32 selected_iterator{};
    std::array<LegacyBattleListContentsRowTrace, 7> rows{};
    std::array<LegacyBattleFrameDrawResult, 7> resource_frames{};
    std::array<rendering::LegacyRectangleEffectStatus, 2> rectangle_statuses{
        rendering::LegacyRectangleEffectStatus::completed,
        rendering::LegacyRectangleEffectStatus::completed,
    };
};

[[nodiscard]] LegacyBattleListContentsResult draw_legacy_battle_list_contents(
    LegacyBattleListContentsState& state,
    LegacyBattleListContentsBindings bindings,
    LegacyBattleListContentsPort& port,
    const LegacyBattleListContentsRequest& request
);

}  // namespace openswd3::battle
