#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_tiled_frame.hpp"

namespace openswd3::asset_runtime {

inline constexpr compat::u32 kLegacyAniFollowerServiceId = 0x13U;
inline constexpr compat::u32 kLegacyAniFollowerActionId = 0x232BU;
inline constexpr compat::u32 kLegacyAniFollowerFirstVariant = 0x4EU;
inline constexpr compat::u32 kLegacyAniFollowerSecondVariant = 0x4FU;

struct LegacyAniFollowerState {
    compat::i32 current_x{320};
    compat::i32 current_y{240};
    compat::i32 target_x{320};
    compat::i32 target_y{240};
    compat::i32 velocity_x{};
    compat::i32 velocity_y{};
};

class LegacyAniFollowerPorts {
public:
    virtual ~LegacyAniFollowerPorts() = default;

    [[nodiscard]] virtual LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) = 0;
    [[nodiscard]] virtual bool load_frame_piece(
        compat::u16 resource_id,
        compat::u16 frame_index,
        rendering::LegacyFramePiece& piece
    ) = 0;
    virtual void set_clip_rectangle(
        compat::i32 left, compat::i32 top, compat::i32 right, compat::i32 bottom
    ) noexcept = 0;
    [[nodiscard]] virtual rendering::LegacyBlitExecutionStatus draw_frame_piece(
        const rendering::LegacyFramePiece& piece,
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 flags
    ) noexcept = 0;
};

class LegacyAniFollowerRuntimePorts final : public LegacyAniFollowerPorts {
public:
    LegacyAniFollowerRuntimePorts(
        LegacyActionUpdater& action_updater,
        LegacyTswRuntime& tsw_runtime,
        rendering::LegacyFramebuffer& framebuffer,
        rendering::LegacyRasterGeometryState& raster,
        const rendering::LegacyBlitEffectState& effects,
        rendering::LegacyRleRowJitterState& jitter
    ) noexcept;

    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override;
    [[nodiscard]] bool load_frame_piece(
        compat::u16 resource_id,
        compat::u16 frame_index,
        rendering::LegacyFramePiece& piece
    ) override;
    void set_clip_rectangle(
        compat::i32 left, compat::i32 top, compat::i32 right, compat::i32 bottom
    ) noexcept override;
    [[nodiscard]] rendering::LegacyBlitExecutionStatus draw_frame_piece(
        const rendering::LegacyFramePiece& piece,
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 flags
    ) noexcept override;

private:
    LegacyActionUpdater& action_updater_;
    LegacyTswRuntime& tsw_runtime_;
    rendering::LegacyFramebuffer& framebuffer_;
    rendering::LegacyRasterGeometryState& raster_;
    const rendering::LegacyBlitEffectState& effects_;
    rendering::LegacyRleRowJitterState& jitter_;
};

enum class LegacyAniFollowerStatus {
    ready,
    disabled,
    action_update_failed,
    frame_load_failed,
};

struct LegacyAniFollowerResult {
    LegacyAniFollowerStatus status{LegacyAniFollowerStatus::ready};
    compat::u32 failed_variant{};
    compat::u32 action_update_count{};
    compat::u32 frame_request_count{};
    compat::u32 draw_count{};
    compat::u32 blit_failure_count{};
    rendering::LegacyBlitExecutionStatus last_blit_status{
        rendering::LegacyBlitExecutionStatus::completed
    };
};

[[nodiscard]] LegacyAniFollowerResult update_draw_legacy_ani_follower(
    bool enabled,
    LegacyAniFollowerState& state,
    LegacyActionRecord& action_record,
    LegacyAniFollowerPorts& ports
);

}  // namespace openswd3::asset_runtime
