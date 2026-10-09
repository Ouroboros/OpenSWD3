#pragma once

#include "openswd3/battle/legacy_battle_frame_effect_control.hpp"
#include "openswd3/battle/legacy_battle_frame_surface.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"

#include <array>
#include <optional>

namespace openswd3::battle {

struct LegacyBattleFrameRefreshState {
    std::array<compat::u32, 2> surface_tokens{};
    compat::u16 snapshot_word_36{};
    compat::u16 snapshot_word_38{};
    compat::u16 snapshot_word_3a{};
    compat::u32 viewport_token{};
    compat::u32 final_surface_token{};
    compat::u32 active_surface_token{0xFFFFFFFFU};
    compat::u16 refresh_pending{};
};

struct LegacyBattleFrameRefreshSource {
    const std::array<compat::u32, 5>& background_record;
    rendering::LegacyBlitRequest& shared_request;
    compat::u32& target_pixel_address;
};

class LegacyBattleFrameRefreshStatePort
    : public virtual LegacyBattleFrameSurfacePort,
      public virtual LegacyBattleFrameEffectControlStatePort {
public:
    [[nodiscard]] virtual LegacyBattleFrameRefreshState&
    frame_refresh_state() noexcept {
        return frame_refresh_state_;
    }

    [[nodiscard]] virtual const LegacyBattleFrameRefreshState&
    frame_refresh_state() const noexcept {
        return frame_refresh_state_;
    }

    [[nodiscard]] virtual std::optional<LegacyBattleFrameRefreshSource>
    frame_refresh_source() noexcept {
        return frame_refresh_source_;
    }

    [[nodiscard]] std::optional<LegacyBattleFrameRefreshSource>
    bind_frame_refresh_source(
        std::optional<LegacyBattleFrameRefreshSource> source
    ) noexcept {
        const auto previous = frame_refresh_source_;
        frame_refresh_source_.reset();
        if (source.has_value()) {
            frame_refresh_source_.emplace(*source);
        }

        return previous;
    }

    [[nodiscard]] virtual bool serve_refresh_audio();
    [[nodiscard]] virtual bool draw_refresh_background(
        compat::u32 pixels,
        compat::u32 source,
        compat::u32 width,
        compat::u32 height
    );
    [[nodiscard]] virtual bool apply_refresh_red(
        compat::u32 pixels, compat::u32 pixel_count, compat::i32 amount
    );
    [[nodiscard]] virtual bool apply_refresh_green(
        compat::u32 pixels, compat::u32 pixel_count, compat::i32 amount
    );
    [[nodiscard]] virtual bool apply_refresh_blue(
        compat::u32 pixels, compat::u32 pixel_count, compat::i32 amount
    );

protected:
    LegacyBattleFrameRefreshStatePort() = default;
    ~LegacyBattleFrameRefreshStatePort() = default;

private:
    LegacyBattleFrameRefreshState frame_refresh_state_{};
    std::optional<LegacyBattleFrameRefreshSource> frame_refresh_source_;
};

enum class LegacyBattleFrameRefreshStatus : compat::u8 {
    completed,
    source_binding_typed_stop,
    audio_stopped,
    lock_stopped,
    unlock_stopped,
    background_stopped,
    red_stopped,
    green_stopped,
    blue_stopped,
};

struct LegacyBattleFrameRefreshResult {
    LegacyBattleFrameRefreshStatus status{
        LegacyBattleFrameRefreshStatus::completed
    };
    compat::u32 surface_iterations{};
    bool refreshed{};
};

[[nodiscard]] LegacyBattleFrameRefreshResult
refresh_legacy_battle_frame(LegacyBattleFrameRefreshStatePort& port);

}  // namespace openswd3::battle
