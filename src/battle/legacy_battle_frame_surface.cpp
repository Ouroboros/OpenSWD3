#include "openswd3/battle/legacy_battle_frame_surface.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"

namespace openswd3::battle {

LegacyBattleFrameSurfaceResult prepare_legacy_battle_frame_surface(
    LegacyBattleFrameCoordinatorState& state, LegacyBattleFrameSurfacePort& port
) {
    LegacyBattleFrameSurfaceResult result;
    ++result.lock_calls;
    const auto locked = port.lock_frame_surface(state.target_surface_token);
    if (!locked.callee_returned) {
        return result;
    }

    // 45326A reloads the surface before 453272 publishes the returned pointer.
    const auto surface = state.target_surface_token;
    state.current_target_pointer_token = locked.eax;
    ++result.unlock_calls;
    const auto unlocked = port.unlock_frame_surface(surface, locked.eax);
    if (!unlocked.callee_returned) {
        result.status = LegacyBattleFrameSurfaceStatus::unlock_stopped;
        return result;
    }

    if (state.render_abort_latch == 1U) {
        result.status = LegacyBattleFrameSurfaceStatus::render_aborted;
        result.return_value = state.active;
        return result;
    }

    result.status = LegacyBattleFrameSurfaceStatus::continue_frame;
    return result;
}

LegacyBattleFramebufferSurface::LegacyBattleFramebufferSurface(
    rendering::LegacyFramebuffer& framebuffer, const compat::u32 surface_token
) noexcept
    : framebuffer_(framebuffer), surface_token_(surface_token) {}

LegacyBattleFrameSurfaceReply
LegacyBattleFramebufferSurface::lock_frame_surface(const compat::u32 surface) {
    // Unknown identities are missing bindings, not a DirectDraw HRESULT.
    if (surface != surface_token_) {
        return {};
    }

    if (!pixel_token_.has_value()) {
        const auto bytes = std::as_writable_bytes(
            framebuffer_.physical_pixels_with_read_guard()
        );
        pixel_token_ = asset_runtime::reserve_legacy_guest_bytes(bytes.size());
        if (!pixel_token_.has_value()) {
            return {};
        }
    }

    pitch_shadow_ = framebuffer_.geometry().surface.pitch_bytes >> 1;
    return {.eax = *pixel_token_, .callee_returned = true};
}

LegacyBattleFrameSurfaceReply
LegacyBattleFramebufferSurface::unlock_frame_surface(
    const compat::u32 surface, const compat::u32 pixels
) {
    if (surface != surface_token_) {
        return {};
    }

    // No host lease exists for the software framebuffer. Keep the address
    // mapping alive after this original boundary, including a null argument.
    static_cast<void>(pixels);
    return {.eax = 0U, .callee_returned = true};
}

std::span<std::byte> LegacyBattleFramebufferSurface::pixel_bytes(
    const compat::u32 address
) noexcept {
    // A lookup may precede the first successful lock or name another block.
    if (!pixel_token_.has_value() || address < *pixel_token_) {
        return {};
    }

    const auto bytes =
        std::as_writable_bytes(framebuffer_.physical_pixels_with_read_guard());
    const auto offset = address - *pixel_token_;
    if (offset >= bytes.size()) {
        return {};
    }

    return bytes.subspan(offset);
}

compat::i32 LegacyBattleFramebufferSurface::pitch_shadow() const noexcept {
    return pitch_shadow_;
}

}  // namespace openswd3::battle
