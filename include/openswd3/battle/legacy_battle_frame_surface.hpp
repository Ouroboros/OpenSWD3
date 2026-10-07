#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <cstddef>
#include <optional>
#include <span>

namespace openswd3::battle {

struct LegacyBattleFrameCoordinatorState;

struct LegacyBattleFrameSurfaceReply {
    compat::u32 eax{};
    bool callee_returned{};
};

class LegacyBattleFrameSurfacePort {
public:
    virtual ~LegacyBattleFrameSurfacePort() = default;
    [[nodiscard]] virtual LegacyBattleFrameSurfaceReply
    lock_frame_surface(compat::u32 surface) = 0;
    [[nodiscard]] virtual LegacyBattleFrameSurfaceReply
    unlock_frame_surface(compat::u32 surface, compat::u32 pixels) = 0;
};

enum class LegacyBattleFrameSurfaceStatus : compat::u8 {
    lock_stopped,
    unlock_stopped,
    continue_frame,
    render_aborted,
};

struct LegacyBattleFrameSurfaceResult {
    LegacyBattleFrameSurfaceStatus status{
        LegacyBattleFrameSurfaceStatus::lock_stopped
    };
    compat::u32 lock_calls{};
    compat::u32 unlock_calls{};
    compat::u32 return_value{};
};

// 45325E..453286 and the abort return at 453569. Zero from Lock is a
// normal return: publish it and still call Unlock before reading the latch.
[[nodiscard]] LegacyBattleFrameSurfaceResult
prepare_legacy_battle_frame_surface(
    LegacyBattleFrameCoordinatorState& state, LegacyBattleFrameSurfacePort& port
);

// Stable software storage replaces DirectDraw's Lock/Unlock lease. This owns
// only a guest identity; all pixels still belong to the supplied framebuffer.
// The framebuffer is nonmovable and has no resizing API.
class LegacyBattleFramebufferSurface final
    : public LegacyBattleFrameSurfacePort {
public:
    LegacyBattleFramebufferSurface(
        rendering::LegacyFramebuffer& framebuffer, compat::u32 surface_token
    ) noexcept;

    [[nodiscard]] LegacyBattleFrameSurfaceReply
    lock_frame_surface(compat::u32 surface) override;
    [[nodiscard]] LegacyBattleFrameSurfaceReply
    unlock_frame_surface(compat::u32 surface, compat::u32 pixels) override;
    [[nodiscard]] std::span<std::byte>
    pixel_bytes(compat::u32 address) noexcept;
    [[nodiscard]] compat::i32 pitch_shadow() const noexcept;

private:
    rendering::LegacyFramebuffer& framebuffer_;
    compat::u32 surface_token_;
    std::optional<compat::u32> pixel_token_;
    compat::i32 pitch_shadow_{};
};

}  // namespace openswd3::battle
