#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::rendering {

inline constexpr compat::u32 kLegacyPrimaryCountdownFlag = 0x10U;
inline constexpr compat::u32 kLegacyPrimaryCountdownCompanionFlag = 0x12U;
inline constexpr compat::u32 kLegacySecondaryCountdownFlag = 0x4AU;
inline constexpr compat::u32 kLegacyCountdownSuppressionFlag = 0x4CU;
inline constexpr compat::u32 kLegacyCountdownActionId = 0x232CU;

struct LegacyCountdownState {
    compat::u32 primary_ticks{0xFFFFFFFFU};
    compat::u32 secondary_ticks{0xFFFFFFFFU};
    compat::u32 primary_transition_value{};
    compat::u32 primary_value_004c97e8{};
    compat::u32 primary_value_004c97ec{};
    compat::u32 secondary_value_004bab78{};
    compat::u32 secondary_value_004bab7c{};
};

class LegacyCountdownFlagQueryPorts {
public:
    virtual ~LegacyCountdownFlagQueryPorts() = default;

    [[nodiscard]] virtual bool
    query_internal_flag(compat::u32 index) noexcept = 0;
};

class LegacyCountdownFlagPorts : public LegacyCountdownFlagQueryPorts {
public:
    ~LegacyCountdownFlagPorts() override = default;

    virtual void set_internal_flag(compat::u32 index) noexcept = 0;
};

struct LegacyCountdownInitializationRequest {
    compat::i32 minutes{};
    compat::i32 seconds{};
    compat::u32 primary_transition_value{};
    compat::i32 mode{};
};

void initialize_legacy_countdown(
    LegacyCountdownState& state,
    LegacyCountdownFlagPorts& flags,
    const LegacyCountdownInitializationRequest& request
) noexcept;

class LegacyCountdownPieceProvider {
public:
    virtual ~LegacyCountdownPieceProvider() = default;

    // Mirrors the static 0x232C action update followed by sub_4315D0.
    [[nodiscard]] virtual bool load_countdown_piece(
        compat::u32 action_id, compat::i32 action_index, LegacyFramePiece& piece
    ) noexcept = 0;
};

struct LegacyCountdownDisplayRequest {
    compat::i32 destination_x{};
    compat::i32 destination_y{};
    compat::i32 mode{};
};

enum class LegacyCountdownDisplayStatus : compat::u8 {
    completed,
    hidden_inactive,
    hidden_suppressed,
    piece_unavailable,
    invalid_piece_geometry,
    blit_failed,
};

struct LegacyCountdownDisplayResult {
    LegacyCountdownDisplayStatus status{
        LegacyCountdownDisplayStatus::completed
    };
    compat::i32 displayed_seconds{};
    compat::i32 piece_index{};
    compat::u32 piece_request_count{};
    compat::u32 draw_call_count{};
    LegacyBlitExecutionStatus blit_status{LegacyBlitExecutionStatus::completed};
};

[[nodiscard]] LegacyCountdownDisplayResult draw_legacy_countdown(
    LegacyFramebuffer& framebuffer,
    const LegacyRasterGeometryState& raster,
    const LegacyCountdownState& state,
    LegacyCountdownFlagQueryPorts& flags,
    LegacyCountdownPieceProvider& provider,
    const LegacyCountdownDisplayRequest& request,
    const LegacyBlitEffectState& effects,
    LegacyRleRowJitterState& jitter
) noexcept;

}  // namespace openswd3::rendering
