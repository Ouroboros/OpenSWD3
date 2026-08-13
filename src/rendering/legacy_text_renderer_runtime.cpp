#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"

#include <new>

namespace openswd3::rendering {
namespace {

[[nodiscard]] constexpr compat::i32
initial_horizontal_advance(const compat::u32 point_size) noexcept {
    if (point_size == 20U) {
        return 24;
    }
    if (point_size == 16U) {
        return 18;
    }
    // sub_435160 establishes 16 before sub_40F340 applies the 12-point
    // font. The latter has no 12-point advance override.
    return 16;
}

}  // namespace

bool LegacyTextRendererBinding::ready() const noexcept {
    return framebuffer != nullptr && glyph_cache != nullptr &&
        glyph_provider != nullptr && state != nullptr;
}

LegacyTextRendererRuntimeStatus LegacyTextRendererRuntime::rebuild(
    const compat::u32 point_size,
    LegacyFramebuffer& framebuffer,
    LegacyGlyphProvider& glyph_provider
) noexcept {
    Slot* const slot = find_slot(point_size);
    if (slot == nullptr) {
        return LegacyTextRendererRuntimeStatus::unsupported_point_size;
    }

    // sub_40F340 always invokes sub_4351F0 before constructing the new
    // renderer resources, even when the slot was already released.
    release_slot(*slot);

    std::unique_ptr<LegacyGlyphCache> glyph_cache;
    try {
        const auto glyph_size = static_cast<compat::i32>(point_size);
        glyph_cache =
            std::make_unique<LegacyGlyphCache>(glyph_size, glyph_size);
    } catch (const std::bad_alloc&) {
        return LegacyTextRendererRuntimeStatus::allocation_failed;
    }

    const LegacySurfaceGeometry& geometry = framebuffer.geometry().surface;
    slot->state = LegacyTextRendererState{
        .horizontal_advance = initial_horizontal_advance(point_size),
        .secondary_color = 0U,
        .background_color = 0xFFFEU,
        .clip = {
            .left = 0,
            .top = 0,
            .width = geometry.width,
            .height = geometry.height,
        },
    };
    slot->framebuffer = &framebuffer;
    slot->glyph_provider = &glyph_provider;
    slot->glyph_cache = std::move(glyph_cache);
    return LegacyTextRendererRuntimeStatus::completed;
}

LegacyTextRendererRuntimeStatus
LegacyTextRendererRuntime::release(const compat::u32 point_size) noexcept {
    Slot* const slot = find_slot(point_size);
    if (slot == nullptr) {
        return LegacyTextRendererRuntimeStatus::unsupported_point_size;
    }
    release_slot(*slot);
    return LegacyTextRendererRuntimeStatus::completed;
}

LegacyTextRendererRuntimeStatus
LegacyTextRendererRuntime::set_horizontal_advance(
    const compat::u32 point_size, const compat::i32 horizontal_advance
) noexcept {
    Slot* const slot = find_slot(point_size);
    if (slot == nullptr) {
        return LegacyTextRendererRuntimeStatus::unsupported_point_size;
    }
    slot->state.horizontal_advance = horizontal_advance;
    return LegacyTextRendererRuntimeStatus::completed;
}

void LegacyTextRendererRuntime::release_all() noexcept {
    for (Slot& slot : slots_) {
        release_slot(slot);
    }
}

bool LegacyTextRendererRuntime::ready(
    const compat::u32 point_size
) const noexcept {
    const Slot* const slot = find_slot(point_size);
    return slot != nullptr && slot->glyph_cache != nullptr &&
        slot->framebuffer != nullptr && slot->glyph_provider != nullptr;
}

LegacyTextRendererBinding
LegacyTextRendererRuntime::binding(const compat::u32 point_size) noexcept {
    Slot* const slot = find_slot(point_size);
    if (slot == nullptr || !ready(point_size)) {
        return {};
    }
    return LegacyTextRendererBinding{
        .framebuffer = slot->framebuffer,
        .glyph_cache = slot->glyph_cache.get(),
        .glyph_provider = slot->glyph_provider,
        .state = &slot->state,
    };
}

const LegacyTextRendererState*
LegacyTextRendererRuntime::state(const compat::u32 point_size) const noexcept {
    const Slot* const slot = find_slot(point_size);
    return slot != nullptr && ready(point_size) ? &slot->state : nullptr;
}

const LegacyGlyphCache* LegacyTextRendererRuntime::glyph_cache(
    const compat::u32 point_size
) const noexcept {
    const Slot* const slot = find_slot(point_size);
    return slot != nullptr && ready(point_size) ? slot->glyph_cache.get()
                                                : nullptr;
}

LegacyTextRendererRuntime::Slot*
LegacyTextRendererRuntime::find_slot(const compat::u32 point_size) noexcept {
    for (Slot& slot : slots_) {
        if (slot.point_size == point_size) {
            return &slot;
        }
    }
    return nullptr;
}

const LegacyTextRendererRuntime::Slot* LegacyTextRendererRuntime::find_slot(
    const compat::u32 point_size
) const noexcept {
    for (const Slot& slot : slots_) {
        if (slot.point_size == point_size) {
            return &slot;
        }
    }
    return nullptr;
}

void LegacyTextRendererRuntime::release_slot(Slot& slot) noexcept {
    slot.glyph_cache.reset();
    slot.framebuffer = nullptr;
    slot.glyph_provider = nullptr;
}

}  // namespace openswd3::rendering
