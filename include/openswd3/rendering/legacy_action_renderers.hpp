#pragma once

#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_packed_row.hpp"

#include <list>
#include <span>
#include <vector>

namespace openswd3::rendering {

struct LegacyPackedRowEffect {
    compat::i16 base_x{};
    compat::i16 base_y{};
    compat::i16 limit{};
    compat::i16 row_count{};
    compat::u16 mode{};
    compat::i16 color_index{};
    std::vector<compat::i16> row_offsets{};
    std::vector<compat::i16> row_lengths{};
};

class LegacyPackedRowDrawPorts {
public:
    virtual ~LegacyPackedRowDrawPorts() = default;

    [[nodiscard]] virtual LegacyPackedRowBlendStatus draw_legacy_packed_row(
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 color_pattern,
        compat::i32 length
    ) noexcept = 0;
};

class LegacyFramebufferPackedRowDrawPorts final
    : public LegacyPackedRowDrawPorts {
public:
    LegacyFramebufferPackedRowDrawPorts(
        LegacyFramebuffer& framebuffer, const LegacyPixelConversionState& format
    ) noexcept;

    [[nodiscard]] LegacyPackedRowBlendStatus draw_legacy_packed_row(
        compat::i32 destination_x,
        compat::i32 destination_y,
        compat::u32 color_pattern,
        compat::i32 length
    ) noexcept override;

private:
    LegacyFramebuffer& framebuffer_;
    const LegacyPixelConversionState& format_;
};

struct LegacyPackedRowEffectResult {
    compat::u32 visited_count{};
    compat::u32 invalid_record_count{};
    compat::u32 random_request_count{};
    compat::u32 draw_count{};
    compat::u32 draw_failure_count{};
    compat::u32 transitioned_to_simple_count{};
    compat::u32 removed_count{};
    LegacyPackedRowBlendStatus last_draw_status{
        LegacyPackedRowBlendStatus::completed
    };
};

struct LegacyPackedRowEffectReleaseResult {
    compat::u32 node_release_count{};
    compat::u32 row_offset_release_calls{};
    compat::u32 row_offset_owners_released{};
    compat::u32 row_length_release_calls{};
    compat::u32 row_length_owners_released{};
};

// sub_40F500 (0x0040F500..0x0040F539): repeatedly detach the list head,
// release its two owned row arrays in +0x0C/+0x10 order, then release the
// 0x18-byte node itself.
[[nodiscard]] LegacyPackedRowEffectReleaseResult
release_legacy_packed_row_effects(
    std::list<LegacyPackedRowEffect>& effects
) noexcept;

// sub_414E50. This function owns the exact list, mode, RNG and removal
// behavior. The port keeps the coordinator independently testable; the live
// runtime binds it to the closed sub_417DE0 framebuffer implementation.
[[nodiscard]] LegacyPackedRowEffectResult update_draw_legacy_packed_row_effects(
    std::list<LegacyPackedRowEffect>& effects,
    std::span<const compat::u32> color_patterns,
    input_time_rng::LegacySecondaryRng& random,
    LegacyPackedRowDrawPorts& draw_ports
) noexcept;

}  // namespace openswd3::rendering
