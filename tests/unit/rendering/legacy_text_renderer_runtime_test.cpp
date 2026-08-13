#include "test.hpp"

#include "openswd3/rendering/legacy_text_renderer_runtime.hpp"

#include <span>

namespace {

using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyGlyphProvider;
using openswd3::rendering::LegacyGlyphProviderStatus;
using openswd3::rendering::LegacyRawCharacter;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::LegacyTextRendererRuntime;
using openswd3::rendering::LegacyTextRendererRuntimeStatus;

class GlyphProvider final : public LegacyGlyphProvider {
public:
    [[nodiscard]] LegacyGlyphProviderStatus provide_glyph_mask(
        const LegacyRawCharacter&,
        openswd3::compat::i32,
        openswd3::compat::i32,
        std::span<openswd3::compat::u8>
    ) noexcept override {
        return LegacyGlyphProviderStatus::completed;
    }
};

void test_three_default_slots(openswd3::test::Context& test) {
    LegacyTextRendererRuntime runtime;
    LegacyFramebuffer framebuffer;
    GlyphProvider provider;

    test.expect_true(!runtime.ready(20U), "20-point slot starts released");
    test.expect_true(!runtime.ready(16U), "16-point slot starts released");
    test.expect_true(!runtime.ready(12U), "12-point slot starts released");

    for (const auto point_size :
         openswd3::rendering::kLegacyTextRendererPointSizes) {
        test.expect_equal(
            runtime.rebuild(point_size, framebuffer, provider),
            LegacyTextRendererRuntimeStatus::completed,
            "known renderer slot rebuilds"
        );
        test.expect_true(runtime.ready(point_size), "rebuilt slot is ready");
    }

    const auto* const large_state = runtime.state(20U);
    const auto* const medium_state = runtime.state(16U);
    const auto* const small_state = runtime.state(12U);
    test.expect_true(large_state != nullptr, "20-point state is available");
    test.expect_true(medium_state != nullptr, "16-point state is available");
    test.expect_true(small_state != nullptr, "12-point state is available");
    if (large_state == nullptr || medium_state == nullptr ||
        small_state == nullptr) {
        return;
    }

    test.expect_equal(
        large_state->horizontal_advance,
        24,
        "20-point initial advance"
    );
    test.expect_equal(
        medium_state->horizontal_advance,
        18,
        "16-point initial advance"
    );
    test.expect_equal(
        small_state->horizontal_advance,
        16,
        "12-point keeps constructor advance"
    );
    test.expect_equal(
        large_state->secondary_color,
        static_cast<openswd3::compat::u16>(0U),
        "secondary color starts at zero"
    );
    test.expect_equal(
        large_state->background_color,
        static_cast<openswd3::compat::u16>(0xFFFEU),
        "background starts disabled"
    );
    test.expect_equal(
        large_state->clip.width,
        framebuffer.geometry().surface.width,
        "clip width binds the framebuffer"
    );
    test.expect_equal(
        large_state->clip.height,
        framebuffer.geometry().surface.height,
        "clip height binds the framebuffer"
    );

    test.expect_equal(
        runtime.glyph_cache(20U)->glyph_width(),
        20,
        "20-point cache width"
    );
    test.expect_equal(
        runtime.glyph_cache(16U)->glyph_height(),
        16,
        "16-point cache height"
    );
    test.expect_equal(
        runtime.glyph_cache(12U)->glyph_width(),
        12,
        "12-point cache width"
    );
}

void test_rebuild_rebinds_and_clears(openswd3::test::Context& test) {
    LegacyTextRendererRuntime runtime;
    LegacyFramebuffer first;
    LegacyFramebuffer second(LegacySurfaceGeometry{
        .pitch_bytes = 80,
        .width = 32,
        .height = 24,
    });
    GlyphProvider first_provider;
    GlyphProvider second_provider;

    static_cast<void>(runtime.rebuild(12U, first, first_provider));
    auto first_binding = runtime.binding(12U);
    test.expect_true(first_binding.ready(), "first binding is ready");
    if (!first_binding.ready()) {
        return;
    }
    static_cast<void>(first_binding.glyph_cache->insert_empty(0x0041U));
    first_binding.state->horizontal_advance = 99;

    test.expect_equal(
        runtime.rebuild(12U, second, second_provider),
        LegacyTextRendererRuntimeStatus::completed,
        "rebuild replaces an active slot"
    );
    const auto second_binding = runtime.binding(12U);
    test.expect_true(second_binding.ready(), "second binding is ready");
    if (!second_binding.ready()) {
        return;
    }
    test.expect_true(
        second_binding.framebuffer == &second,
        "rebuild binds the replacement framebuffer"
    );
    test.expect_true(
        second_binding.glyph_provider == &second_provider,
        "rebuild binds the replacement provider"
    );
    test.expect_equal(
        second_binding.glyph_cache->count(),
        0U,
        "rebuild clears the glyph cache"
    );
    test.expect_equal(
        second_binding.state->horizontal_advance,
        16,
        "rebuild restores the original 12-point advance"
    );
    test.expect_equal(
        second_binding.state->clip.width,
        32,
        "rebuild refreshes clip width"
    );
    test.expect_equal(
        second_binding.state->clip.height,
        24,
        "rebuild refreshes clip height"
    );
}

void test_gameplay_advance_override(openswd3::test::Context& test) {
    LegacyTextRendererRuntime runtime;
    LegacyFramebuffer framebuffer;
    GlyphProvider provider;
    for (const auto point_size :
         openswd3::rendering::kLegacyTextRendererPointSizes) {
        static_cast<void>(runtime.rebuild(point_size, framebuffer, provider));
    }

    test.expect_equal(
        runtime.set_horizontal_advance(20U, 0x16),
        LegacyTextRendererRuntimeStatus::completed,
        "gameplay 20-point advance override is accepted"
    );
    test.expect_equal(
        runtime.set_horizontal_advance(16U, 0x12),
        LegacyTextRendererRuntimeStatus::completed,
        "gameplay 16-point advance override is accepted"
    );
    test.expect_equal(
        runtime.set_horizontal_advance(12U, 0x10),
        LegacyTextRendererRuntimeStatus::completed,
        "gameplay 12-point advance override is accepted"
    );

    test.expect_equal(
        runtime.state(20U)->horizontal_advance,
        22,
        "sub_40A570 changes the gameplay dialogue advance to 22"
    );
    test.expect_equal(
        runtime.state(16U)->horizontal_advance,
        18,
        "sub_40A570 keeps the gameplay 16-point advance at 18"
    );
    test.expect_equal(
        runtime.state(12U)->horizontal_advance,
        16,
        "sub_40A570 keeps the gameplay 12-point advance at 16"
    );
    test.expect_equal(
        runtime.set_horizontal_advance(14U, 14),
        LegacyTextRendererRuntimeStatus::unsupported_point_size,
        "unknown gameplay advance override is isolated"
    );
}

void test_release_and_unknown_size(openswd3::test::Context& test) {
    LegacyTextRendererRuntime runtime;
    LegacyFramebuffer framebuffer;
    GlyphProvider provider;
    for (const auto point_size :
         openswd3::rendering::kLegacyTextRendererPointSizes) {
        static_cast<void>(runtime.rebuild(point_size, framebuffer, provider));
    }

    test.expect_equal(
        runtime.release(16U),
        LegacyTextRendererRuntimeStatus::completed,
        "known slot releases"
    );
    test.expect_true(!runtime.ready(16U), "released slot is not ready");
    test.expect_true(runtime.ready(20U), "other slots remain ready");
    test.expect_equal(
        runtime.release(16U),
        LegacyTextRendererRuntimeStatus::completed,
        "release is idempotent"
    );
    test.expect_equal(
        runtime.rebuild(14U, framebuffer, provider),
        LegacyTextRendererRuntimeStatus::unsupported_point_size,
        "unknown rebuild size is isolated"
    );
    test.expect_equal(
        runtime.release(14U),
        LegacyTextRendererRuntimeStatus::unsupported_point_size,
        "unknown release size is isolated"
    );

    runtime.release_all();
    test.expect_true(!runtime.ready(20U), "release-all clears 20-point slot");
    test.expect_true(!runtime.ready(16U), "release-all clears 16-point slot");
    test.expect_true(!runtime.ready(12U), "release-all clears 12-point slot");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_three_default_slots(test);
    test_rebuild_rebinds_and_clears(test);
    test_gameplay_advance_override(test);
    test_release_and_unknown_size(test);
    return test.exit_code();
}
