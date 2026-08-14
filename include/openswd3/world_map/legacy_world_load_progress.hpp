#pragma once

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"
#include "openswd3/rendering/legacy_presentation.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

namespace openswd3::world_map {

inline constexpr compat::u16 kLegacyWorldLoadProgressSuppressionFlag = 70U;
inline constexpr compat::u32 kLegacyWorldLoadProgressBackgroundActionId =
    0x232AU;

struct LegacyWorldLoadProgressState {
    asset_runtime::LegacyActionRecord background_action{};
    asset_runtime::LegacyActionRecord marker_action{};
    compat::i32 green_component{};
    compat::i32 progress{};
    compat::i32 red_component{};
    compat::i32 reserved_004cc2c0{};
    compat::i32 reserved_004cc2c4{};
    compat::i32 blue_component{};
    compat::u32 marker_action_id{};
};

class LegacyWorldLoadProgressPorts {
public:
    virtual ~LegacyWorldLoadProgressPorts() = default;

    [[nodiscard]] virtual compat::u32
    next_random_bounded(compat::u32 upper_bound) = 0;
    virtual void maintain_audio() = 0;
};

enum class LegacyWorldLoadProgressStatus : compat::u8 {
    completed,
    suppressed,
    framebuffer_too_small,
    progress_line_out_of_bounds,
};

struct LegacyWorldLoadProgressResult {
    LegacyWorldLoadProgressStatus status{
        LegacyWorldLoadProgressStatus::completed
    };
    compat::i32 requested_progress{};
    compat::i32 effective_progress{};
    compat::i32 column_limit{};
    compat::u32 drawn_column_count{};
    compat::u32 audio_maintenance_count{};
    asset_runtime::LegacyActionDrawResult background_draw{};
    asset_runtime::LegacyActionDrawResult marker_draw{};
    rendering::LegacyPresentationDispatchResult presentation{};
    bool suppression_flag_cleared{};
};

// sub_40ED60: initialize and redraw the map-loading progress screen. The
// original uses progress -1 as its state-reset sentinel and flag 70 to hide
// the first map load while still consuming the marker RNG draw.
[[nodiscard]] LegacyWorldLoadProgressResult update_legacy_world_load_progress(
    LegacyWorldLoadProgressState& state,
    LegacyWorldStoryVmState& story,
    rendering::LegacyFramebuffer& framebuffer,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    compat::i32 progress,
    LegacyWorldLoadProgressPorts& runtime_ports,
    asset_runtime::LegacyActionDrawPorts& action_ports,
    rendering::LegacyPresentationPorts& presentation_ports
);

}  // namespace openswd3::world_map
