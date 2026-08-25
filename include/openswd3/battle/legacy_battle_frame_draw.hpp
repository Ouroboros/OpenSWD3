#pragma once

#include <array>

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::battle {

struct LegacyBattleFrameDrawState {
    bool frame_record_published{};
    bool frame_record_available{};
    bool source_published{};
    compat::u32 current_frame_index{};
    rendering::LegacyFramePiece current_frame{};
    rendering::LegacyBlitSource current_source{};
};

enum class LegacyBattleFrameDrawStatus : compat::u8 {
    completed,
    width_nonpositive,
    frame_unavailable,
    blit_typed_stop,
};

struct LegacyBattleFrameDrawResult {
    LegacyBattleFrameDrawStatus status{LegacyBattleFrameDrawStatus::completed};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    rendering::LegacyBlitExecutionStatus blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

enum class LegacyBattleLayeredFrameDrawStatus : compat::u8 {
    completed,
    first_frame_typed_stop,
    second_frame_typed_stop,
};

enum class LegacyBattleDecimalPlaceStatus : compat::u8 {
    completed,
    typed_stop,
};

struct LegacyBattleTenPlaceDecimalState {
    compat::u32 packed_color_state{};
    compat::i32 remaining_value{};
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 leading_digit_seen{};
};

struct LegacyBattleDecimalPlaceResult {
    LegacyBattleDecimalPlaceStatus status{
        LegacyBattleDecimalPlaceStatus::completed
    };
    compat::u32 return_value{};
};

class LegacyBattleDecimalPlacePort {
public:
    virtual ~LegacyBattleDecimalPlacePort() = default;

    [[nodiscard]] virtual LegacyBattleDecimalPlaceResult draw_place(
        LegacyBattleTenPlaceDecimalState& state, compat::u32 divisor
    ) noexcept = 0;
};

enum class LegacyBattleTenPlaceDecimalStatus : compat::u8 {
    completed,
    place_typed_stop,
};

struct LegacyBattleTenPlaceDecimalResult {
    LegacyBattleTenPlaceDecimalStatus status{
        LegacyBattleTenPlaceDecimalStatus::completed
    };
    std::array<compat::u32, 10> divisors{};
    std::array<compat::u32, 10> place_returns{};
    std::array<compat::u16, 10> x_advances{};
    compat::u32 call_count{};
    compat::u32 stopped_place_index{};
    compat::i32 final_x{};
    compat::u32 legacy_return_value{};
};

enum class LegacyBattleDecimalFrameDrawStatus : compat::u8 {
    completed,
    frame_typed_stop,
};

struct LegacyBattleDecimalFrameDrawState {
    compat::i32 entry_x{};
    compat::i32 entry_y{};
    compat::i32 current_remainder{};
    bool leading_digit_seen{};
    std::array<compat::i32, 4> digit_quotients{};
    compat::u32 digit_count{};
    LegacyBattleFrameDrawState frame{};
};

struct LegacyBattleDecimalFrameDrawResult {
    LegacyBattleDecimalFrameDrawStatus status{
        LegacyBattleDecimalFrameDrawStatus::completed
    };
    LegacyBattleFrameDrawResult last_frame{};
    std::array<compat::u32, 4> frame_indices{};
    compat::u32 decomposition_iterations{};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::u32 drawn_digit_count{};
    compat::i32 final_x{};
};

struct LegacyBattleLayeredFrameDrawResult {
    LegacyBattleLayeredFrameDrawStatus status{
        LegacyBattleLayeredFrameDrawStatus::completed
    };
    LegacyBattleFrameDrawResult first{};
    LegacyBattleFrameDrawResult second{};
    compat::u32 frame_load_calls{};
    compat::u32 frame_draw_calls{};
    compat::i32 second_source_width{};
    compat::u32 legacy_return_value{};
};

// sub_450270: query frame zero and draw it once at the supplied coordinates.
[[nodiscard]] LegacyBattleFrameDrawResult draw_legacy_battle_frame_zero(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 x,
    compat::i32 y
) noexcept;

// sub_4507A0: coordinate ten decimal places from one billion through one.
[[nodiscard]] LegacyBattleTenPlaceDecimalResult
coordinate_legacy_battle_ten_place_decimal(
    LegacyBattleTenPlaceDecimalState& state,
    LegacyBattleDecimalPlacePort& place_port,
    compat::u32 color_stack_slot,
    compat::i32 value,
    compat::i32 x,
    compat::i32 y
) noexcept;

// sub_4506B0: decompose a signed four-digit value and draw digit frames right-to-left.
[[nodiscard]] LegacyBattleDecimalFrameDrawResult
draw_legacy_battle_decimal_frames(
    LegacyBattleDecimalFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 value,
    compat::i32 x,
    compat::i32 y
) noexcept;

// sub_450530: draw frame zero, then optionally draw frame one by an explicit width.
[[nodiscard]] LegacyBattleLayeredFrameDrawResult
draw_legacy_battle_layered_resource_frames(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 x,
    compat::i32 y,
    compat::i32 second_width,
    compat::u32 second_frame_index = 1U,
    compat::u32 legacy_return_value = 0U
) noexcept;

// sub_450630: draw frame zero, then frame one by low-word width plus two.
[[nodiscard]] LegacyBattleLayeredFrameDrawResult
draw_legacy_battle_layered_low_word_width(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 x,
    compat::i32 y,
    compat::u32 width_seed
) noexcept;

// sub_4505B0: draw frame zero, then optionally draw frame two and return one.
[[nodiscard]] LegacyBattleLayeredFrameDrawResult
draw_legacy_battle_layered_resource_frame_two(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::i32 x,
    compat::i32 y,
    compat::i32 second_width
) noexcept;

// sub_4504E0: query and draw the selected frame at the supplied coordinates.
[[nodiscard]] LegacyBattleFrameDrawResult draw_legacy_battle_resource_frame(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::u32 frame_index,
    compat::i32 x,
    compat::i32 y
) noexcept;

// sub_450490: query the selected frame and draw an explicit width by its height.
[[nodiscard]] LegacyBattleFrameDrawResult
draw_legacy_battle_resource_frame_width(
    LegacyBattleFrameDrawState& state,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyBlitClipRectangle& clip,
    rendering::LegacyBlitRequest& shared_request,
    rendering::LegacyBlitEffectState& shared_effects,
    rendering::LegacyRleRowJitterState& jitter,
    rendering::LegacyFramePieceProvider& frame_provider,
    compat::u32 resource_id,
    compat::u32 frame_index,
    compat::i32 x,
    compat::i32 y,
    compat::i32 explicit_width,
    bool skip_nonpositive_width = true
) noexcept;

}  // namespace openswd3::battle
