#include "openswd3/asset_runtime/legacy_ani_activity.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>

namespace openswd3::asset_runtime {
namespace {

[[nodiscard]] bool activity_is_blocked(
    const LegacyAniActivityState& state,
    const LegacyAniActivityBlockers& blockers
) noexcept {
    return blockers.first != 0U || blockers.second != 0U ||
        blockers.third != 0U || (state.flags & kLegacyAniSuspendFlag) != 0U;
}

void copy_framebuffer(
    const std::span<const compat::u8> source,
    const std::span<compat::u8> destination
) noexcept {
    std::ranges::copy(
        source.first(kLegacyAniFramebufferBytes), destination.begin()
    );
}

void clear_edge_rows(
    const std::span<compat::u8> framebuffer,
    const compat::u32 pitch_bytes,
    const compat::u32 row_count
) noexcept {
    const std::size_t bytes = static_cast<std::size_t>(pitch_bytes) * row_count;
    std::ranges::fill(framebuffer.first(bytes), compat::u8{});
    const std::size_t bottom =
        static_cast<std::size_t>(kLegacyAniViewportHeight - row_count) *
        pitch_bytes;
    std::ranges::fill(framebuffer.subspan(bottom, bytes), compat::u8{});
}

}  // namespace

LegacyAniActivityStartResult LegacyAniActivity::start(
    const std::filesystem::path& archive_path,
    const compat::u8 flags,
    const compat::u32 process_flags,
    const compat::u32 scene_flags,
    const rendering::LegacyPixelConversionState pixel_conversion
) {
    close();

    LegacyAniActivityStartResult result;
    result.open_status = archive_.open(archive_path, pixel_conversion);
    if (result.open_status != LegacyAniOpenStatus::ready) {
        return result;
    }

    try {
        framebuffer_backup_.assign(kLegacyAniFramebufferBytes, 0U);
    } catch (const std::bad_alloc&) {
        archive_.close();
        result.status = LegacyAniActivityStartStatus::allocation_failed;
        return result;
    }

    current_frame_ = archive_.load_frame(1U);
    result.frame_status = current_frame_.status;
    if (current_frame_.status != LegacyAniFrameLoadStatus::ready) {
        close();
        result.status = LegacyAniActivityStartStatus::initial_frame_failed;
        return result;
    }

    pixel_conversion_ = pixel_conversion;
    state_.active_extent = archive_.header().display_width;
    state_.phase =
        (flags & kLegacyAniSkipRevealFlag) != 0U ? 1 : kLegacyAniRevealStart;
    state_.process_flags = process_flags | kLegacyAniProcessActiveFlag;
    state_.scene_flags = scene_flags;
    state_.flags = flags;
    state_.snapshot_saved = false;
    result.status = LegacyAniActivityStartStatus::ready;
    return result;
}

LegacyAniActivityStatus LegacyAniActivity::validate_framebuffer(
    const std::span<const compat::u8> framebuffer, const compat::u32 pitch_bytes
) const noexcept {
    if (pitch_bytes == 0U || (pitch_bytes & 1U) != 0U) {
        return LegacyAniActivityStatus::invalid_pitch;
    }
    const std::uint64_t pitched_bytes =
        static_cast<std::uint64_t>(pitch_bytes) * kLegacyAniViewportHeight;
    const std::uint64_t required =
        std::max<std::uint64_t>(kLegacyAniFramebufferBytes, pitched_bytes);
    if (required > framebuffer.size()) {
        return LegacyAniActivityStatus::framebuffer_too_small;
    }
    return LegacyAniActivityStatus::ready;
}

LegacyAniSpanResult LegacyAniActivity::render_current_frame(
    const std::span<compat::u8> framebuffer, const compat::u32 pitch_bytes
) const noexcept {
    return apply_legacy_ani_spans(
        current_frame_.command_stream,
        current_frame_.node.span_count,
        current_frame_.palette,
        framebuffer,
        pitch_bytes,
        archive_.header().display_height,
        pixel_conversion_
    );
}

LegacyAniActivityResult LegacyAniActivity::update(
    const std::span<compat::u8> framebuffer,
    const compat::u32 pitch_bytes,
    const LegacyAniActivityBlockers& blockers,
    LegacyAniActivityPorts& ports
) {
    LegacyAniActivityResult result;
    result.phase_before = state_.phase;
    result.phase_after = state_.phase;
    if (state_.active_extent == 0U) {
        return result;
    }

    result.status = validate_framebuffer(framebuffer, pitch_bytes);
    if (result.status != LegacyAniActivityStatus::ready) {
        return result;
    }

    if (activity_is_blocked(state_, blockers)) {
        if (!state_.snapshot_saved) {
            copy_framebuffer(framebuffer, framebuffer_backup_);
            state_.snapshot_saved = true;
            result.path = LegacyAniActivityPath::snapshot_saved;
        } else {
            copy_framebuffer(framebuffer_backup_, framebuffer);
            result.path =
                LegacyAniActivityPath::snapshot_restored_while_blocked;
        }
        return result;
    }

    if (state_.snapshot_saved) {
        copy_framebuffer(framebuffer_backup_, framebuffer);
        state_.snapshot_saved = false;
        result.path = LegacyAniActivityPath::snapshot_restored_on_resume;
        return result;
    }

    if (state_.phase <= 0) {
        const LegacyAniSpanResult rendered =
            render_current_frame(framebuffer, pitch_bytes);
        result.span_status = rendered.status;
        if (rendered.status != LegacyAniSpanStatus::completed) {
            result.status = LegacyAniActivityStatus::span_failed;
            return result;
        }

        const compat::u32 reveal_rows =
            static_cast<compat::u32>(state_.phase * 4 + 52);
        clear_edge_rows(framebuffer, pitch_bytes, reveal_rows);
        ++state_.phase;
        result.phase_after = state_.phase;
        result.path = LegacyAniActivityPath::reveal_frame;
        return result;
    }

    if (state_.phase < kLegacyAniEndingStart) {
        LegacyAniFrameLoadResult loaded =
            archive_.load_frame(static_cast<compat::u32>(state_.phase));
        result.frame_status = loaded.status;
        if (loaded.legacy_return_value != 0U) {
            state_.phase = kLegacyAniEndingStart;
            result.phase_after = state_.phase;
            result.path = LegacyAniActivityPath::playback_exhausted;
            return result;
        }
        if (loaded.status != LegacyAniFrameLoadStatus::ready) {
            result.status = LegacyAniActivityStatus::frame_load_failed;
            return result;
        }

        current_frame_ = loaded;
        const LegacyAniSpanResult rendered =
            render_current_frame(framebuffer, pitch_bytes);
        result.span_status = rendered.status;
        if (rendered.status != LegacyAniSpanStatus::completed) {
            result.status = LegacyAniActivityStatus::span_failed;
            return result;
        }

        ++state_.phase;
        result.phase_after = state_.phase;
        result.path = LegacyAniActivityPath::playback_frame;
        return result;
    }

    const bool ending_effect =
        (state_.flags & kLegacyAniEndingEffectFlag) != 0U;
    if (!ending_effect) {
        const compat::u32 saved_active_extent = state_.active_extent;
        state_.active_extent = 0U;
        state_.scene_flags &= ~compat::u32{1U};
        ports.redraw_scene_without_ani();
        state_.scene_flags |= 1U;
        state_.active_extent = saved_active_extent;

        const compat::u32 ending_rows = static_cast<compat::u32>(
            (kLegacyAniEndingDefaultLast - state_.phase) * 2
        );
        clear_edge_rows(framebuffer, pitch_bytes, ending_rows);
    } else {
        result.ending_adjustment = (kLegacyAniEndingStart - state_.phase) * 2;
        ports.apply_ending_color_adjustment(
            framebuffer.first(kLegacyAniFramebufferBytes),
            kLegacyAniFramebufferPixels,
            result.ending_adjustment,
            result.ending_adjustment,
            result.ending_adjustment
        );
    }

    ++state_.phase;
    result.legacy_return_value = 1U;
    const compat::i32 last = ending_effect ? kLegacyAniEndingEffectLast
                                           : kLegacyAniEndingDefaultLast;
    if (state_.phase > last) {
        finalize(ports);
        result.path = LegacyAniActivityPath::finalized;
        result.finalized = true;
    } else {
        result.path = LegacyAniActivityPath::ending_frame;
    }
    result.phase_after = state_.phase;
    return result;
}

void LegacyAniActivity::finalize(LegacyAniActivityPorts& ports) {
    current_frame_ = LegacyAniFrameLoadResult{};
    archive_.close();
    framebuffer_backup_.clear();
    state_.active_extent = 0U;
    state_.phase = 0;
    state_.process_flags &= ~kLegacyAniProcessActiveFlag;
    state_.snapshot_saved = false;
    ports.finalize_service(kLegacyAniFinalServiceId);
}

void LegacyAniActivity::close() noexcept {
    current_frame_ = LegacyAniFrameLoadResult{};
    archive_.close();
    framebuffer_backup_.clear();
    state_.active_extent = 0U;
    state_.phase = 0;
    state_.process_flags &= ~kLegacyAniProcessActiveFlag;
    state_.snapshot_saved = false;
}

bool LegacyAniActivity::is_active() const noexcept {
    return state_.active_extent != 0U;
}

LegacyAniActivityState& LegacyAniActivity::state() noexcept {
    return state_;
}

const LegacyAniActivityState& LegacyAniActivity::state() const noexcept {
    return state_;
}

const LegacyAniArchive& LegacyAniActivity::archive() const noexcept {
    return archive_;
}

}  // namespace openswd3::asset_runtime
