#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_text_renderer.hpp"

#include <array>
#include <memory>

namespace openswd3::rendering {

inline constexpr std::array<compat::u32, 3> kLegacyTextRendererPointSizes{
    20U, 16U, 12U
};

enum class LegacyTextRendererRuntimeStatus : compat::u8 {
    completed,
    unsupported_point_size,
    allocation_failed,
};

struct LegacyTextRendererBinding {
    LegacyFramebuffer* framebuffer{};
    LegacyGlyphCache* glyph_cache{};
    LegacyGlyphProvider* glyph_provider{};
    LegacyTextRendererState* state{};

    [[nodiscard]] bool ready() const noexcept;
};

class LegacyTextRendererRuntime final {
public:
    LegacyTextRendererRuntime() = default;

    LegacyTextRendererRuntime(const LegacyTextRendererRuntime&) = delete;
    LegacyTextRendererRuntime&
    operator=(const LegacyTextRendererRuntime&) = delete;
    LegacyTextRendererRuntime(LegacyTextRendererRuntime&&) = delete;
    LegacyTextRendererRuntime& operator=(LegacyTextRendererRuntime&&) = delete;

    [[nodiscard]] LegacyTextRendererRuntimeStatus rebuild(
        compat::u32 point_size,
        LegacyFramebuffer& framebuffer,
        LegacyGlyphProvider& glyph_provider
    ) noexcept;

    [[nodiscard]] LegacyTextRendererRuntimeStatus
    release(compat::u32 point_size) noexcept;
    [[nodiscard]] LegacyTextRendererRuntimeStatus set_horizontal_advance(
        compat::u32 point_size, compat::i32 horizontal_advance
    ) noexcept;
    void release_all() noexcept;

    [[nodiscard]] bool ready(compat::u32 point_size) const noexcept;
    [[nodiscard]] LegacyTextRendererBinding
    binding(compat::u32 point_size) noexcept;
    [[nodiscard]] const LegacyTextRendererState*
    state(compat::u32 point_size) const noexcept;
    [[nodiscard]] const LegacyGlyphCache*
    glyph_cache(compat::u32 point_size) const noexcept;

private:
    struct Slot {
        compat::u32 point_size{};
        std::unique_ptr<LegacyGlyphCache> glyph_cache{};
        LegacyFramebuffer* framebuffer{};
        LegacyGlyphProvider* glyph_provider{};
        LegacyTextRendererState state{};
    };

    [[nodiscard]] Slot* find_slot(compat::u32 point_size) noexcept;
    [[nodiscard]] const Slot* find_slot(compat::u32 point_size) const noexcept;
    static void release_slot(Slot& slot) noexcept;

    std::array<Slot, 3> slots_{
        Slot{.point_size = 20U},
        Slot{.point_size = 16U},
        Slot{.point_size = 12U},
    };
};

}  // namespace openswd3::rendering
