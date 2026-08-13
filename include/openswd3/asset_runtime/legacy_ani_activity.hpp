#pragma once

#include "openswd3/asset_runtime/legacy_ani_archive.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyAniFramebufferBytes = 0x96000U;
inline constexpr compat::u32 kLegacyAniFramebufferPixels = 0x4B000U;
inline constexpr compat::i32 kLegacyAniRevealStart = -13;
inline constexpr compat::i32 kLegacyAniEndingStart = 10000;
inline constexpr compat::i32 kLegacyAniEndingDefaultLast = 10030;
inline constexpr compat::i32 kLegacyAniEndingEffectLast = 10015;
inline constexpr compat::u8 kLegacyAniEndingEffectFlag = 0x01U;
inline constexpr compat::u8 kLegacyAniSkipRevealFlag = 0x02U;
inline constexpr compat::u8 kLegacyAniSuspendFlag = 0x10U;
inline constexpr compat::u32 kLegacyAniProcessActiveFlag = 0x02U;
inline constexpr compat::u32 kLegacyAniFinalServiceId = 0x23U;

struct LegacyAniActivityBlockers {
    compat::u32 first{};
    compat::u32 second{};
    compat::u32 third{};
};

struct LegacyAniActivityState {
    compat::u32 active_extent{};
    compat::i32 phase{};
    compat::u32 process_flags{};
    compat::u32 scene_flags{};
    compat::u8 flags{};
    bool snapshot_saved{};
};

class LegacyAniActivityPorts {
public:
    virtual ~LegacyAniActivityPorts() = default;

    virtual void redraw_scene_without_ani() = 0;
    virtual void apply_ending_color_adjustment(
        std::span<compat::u8> framebuffer,
        compat::u32 pixel_count,
        compat::i32 first,
        compat::i32 second,
        compat::i32 third
    ) = 0;
    virtual void finalize_service(compat::u32 service_id) = 0;
};

enum class LegacyAniActivityStartStatus {
    ready,
    archive_open_failed,
    initial_frame_failed,
    allocation_failed,
};

struct LegacyAniActivityStartResult {
    LegacyAniActivityStartStatus status{
        LegacyAniActivityStartStatus::archive_open_failed
    };
    LegacyAniOpenStatus open_status{LegacyAniOpenStatus::file_open_failed};
    LegacyAniFrameLoadStatus frame_status{
        LegacyAniFrameLoadStatus::archive_not_open
    };
};

enum class LegacyAniActivityStatus {
    ready,
    invalid_pitch,
    framebuffer_too_small,
    frame_load_failed,
    span_failed,
};

enum class LegacyAniActivityPath {
    inactive,
    snapshot_saved,
    snapshot_restored_while_blocked,
    snapshot_restored_on_resume,
    reveal_frame,
    playback_frame,
    playback_exhausted,
    ending_frame,
    finalized,
};

struct LegacyAniActivityResult {
    LegacyAniActivityStatus status{LegacyAniActivityStatus::ready};
    LegacyAniActivityPath path{LegacyAniActivityPath::inactive};
    LegacyAniFrameLoadStatus frame_status{LegacyAniFrameLoadStatus::ready};
    LegacyAniSpanStatus span_status{LegacyAniSpanStatus::completed};
    compat::u32 legacy_return_value{};
    compat::i32 phase_before{};
    compat::i32 phase_after{};
    compat::i32 ending_adjustment{};
    bool finalized{};
};

class LegacyAniActivity final {
public:
    LegacyAniActivity() = default;

    LegacyAniActivity(const LegacyAniActivity&) = delete;
    LegacyAniActivity& operator=(const LegacyAniActivity&) = delete;
    LegacyAniActivity(LegacyAniActivity&&) = delete;
    LegacyAniActivity& operator=(LegacyAniActivity&&) = delete;

    [[nodiscard]] LegacyAniActivityStartResult start(
        const std::filesystem::path& archive_path,
        compat::u8 flags,
        compat::u32 process_flags,
        compat::u32 scene_flags,
        rendering::LegacyPixelConversionState pixel_conversion = {}
    );

    [[nodiscard]] LegacyAniActivityResult update(
        std::span<compat::u8> framebuffer,
        compat::u32 pitch_bytes,
        const LegacyAniActivityBlockers& blockers,
        LegacyAniActivityPorts& ports
    );

    void close() noexcept;

    [[nodiscard]] bool is_active() const noexcept;
    [[nodiscard]] LegacyAniActivityState& state() noexcept;
    [[nodiscard]] const LegacyAniActivityState& state() const noexcept;
    [[nodiscard]] const LegacyAniArchive& archive() const noexcept;

private:
    [[nodiscard]] LegacyAniActivityStatus validate_framebuffer(
        std::span<const compat::u8> framebuffer, compat::u32 pitch_bytes
    ) const noexcept;
    [[nodiscard]] LegacyAniSpanResult render_current_frame(
        std::span<compat::u8> framebuffer, compat::u32 pitch_bytes
    ) const noexcept;
    void finalize(LegacyAniActivityPorts& ports);

    LegacyAniArchive archive_;
    LegacyAniFrameLoadResult current_frame_;
    LegacyAniActivityState state_;
    rendering::LegacyPixelConversionState pixel_conversion_;
    std::vector<compat::u8> framebuffer_backup_;
};

}  // namespace openswd3::asset_runtime
