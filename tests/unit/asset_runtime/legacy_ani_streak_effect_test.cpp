#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_streak_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>

namespace {

using openswd3::asset_runtime::kLegacyAniStreakResetSlotCount;
using openswd3::asset_runtime::kLegacyAniStreakServiceId;
using openswd3::asset_runtime::LegacyAniStreakEffect;
using openswd3::asset_runtime::LegacyAniStreakServicePort;
using openswd3::asset_runtime::LegacyAniStreakSlot;
using openswd3::asset_runtime::LegacyAniStreakStatus;
using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;

class FakeServices final : public LegacyAniStreakServicePort {
public:
    [[nodiscard]] bool service_enabled(const u32 service_id) override {
        ++call_count;
        last_service_id = service_id;
        return enabled;
    }

    bool enabled{};
    u32 call_count{};
    u32 last_service_id{};
};

void test_reset_boundary_and_idle_rng(openswd3::test::Context& test) {
    LegacyAniStreakEffect effect;
    test.expect_true(
        std::ranges::all_of(
            effect.state().slots,
            [](const LegacyAniStreakSlot& slot) {
                return slot.active_flags == 0;
            }
        ),
        "constructor starts with zero-filled process storage"
    );
    effect.state().slots[47U].active_flags = 7;
    effect.state().slots[48U].active_flags = 9;
    effect.state().slots[63U].fixed_x = 123;
    effect.state().previous_live_count = 4;
    effect.state().target_spawn_count = 5;
    effect.reset();
    test.expect_equal(
        effect.state().slots[47U].active_flags,
        i16{0},
        "reset clears the first 48 physical slots"
    );
    test.expect_equal(
        effect.state().slots[48U].active_flags,
        i16{9},
        "reset leaves slot 48 untouched"
    );
    test.expect_equal(
        effect.state().slots[63U].fixed_x,
        i16{123},
        "reset leaves all final 16 slot bytes untouched"
    );
    test.expect_equal(
        effect.state().previous_live_count,
        i16{0},
        "reset clears the previous live count"
    );
    test.expect_equal(
        effect.state().target_spawn_count,
        i16{0},
        "reset clears the target spawn count"
    );

    LegacySecondaryRng random;
    random.seed(0x12345678U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    const auto idle =
        effect.update(random, framebuffer, pixel_format, services);
    test.expect_equal(
        idle.status, LegacyAniStreakStatus::ready, "idle update completes"
    );
    test.expect_false(
        idle.scanned_slots, "zero target and live count skip the 64-slot scan"
    );
    test.expect_equal(
        random.index(),
        std::size_t{2U},
        "idle update still consumes bounded random 1000"
    );
    test.expect_equal(
        services.call_count, u32{0U}, "random 478 does not query service eight"
    );

    LegacyFramebuffer short_framebuffer{
        LegacySurfaceGeometry{.pitch_bytes = 0x500, .width = 640, .height = 1}
    };
    const std::size_t before_short = random.index();
    const auto short_result =
        effect.update(random, short_framebuffer, pixel_format, services);
    test.expect_equal(
        short_result.status,
        LegacyAniStreakStatus::framebuffer_too_small,
        "short physical framebuffer is isolated"
    );
    test.expect_equal(
        random.index(),
        before_short,
        "short framebuffer consumes no random values"
    );
}

void test_trigger_service_and_creation_vector(openswd3::test::Context& test) {
    LegacyAniStreakEffect effect;
    LegacySecondaryRng random;
    random.seed(39U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    services.enabled = true;

    const auto result =
        effect.update(random, framebuffer, pixel_format, services);
    test.expect_equal(
        result.service_query_count, u32{1U}, "random 953 queries service eight"
    );
    test.expect_equal(
        services.call_count, u32{1U}, "service port is called exactly once"
    );
    test.expect_equal(
        services.last_service_id,
        kLegacyAniStreakServiceId,
        "conditional query uses service id eight"
    );
    test.expect_true(
        result.scanned_slots, "positive target enters the slot scan"
    );
    test.expect_equal(
        result.created_count, u32{1U}, "target one creates one inactive slot"
    );
    test.expect_equal(
        result.visited_active_count,
        u32{0U},
        "new slot is not processed in its creation frame"
    );
    test.expect_equal(
        random.index(),
        std::size_t{10U},
        "trigger and four creation values consume ten raw words"
    );
    test.expect_equal(
        effect.state().target_spawn_count,
        i16{1},
        "enabled service raises the target count"
    );
    test.expect_equal(
        effect.state().previous_live_count,
        i16{0},
        "new slots are omitted from this frame live count"
    );

    const LegacyAniStreakSlot& slot = effect.state().slots[0U];
    test.expect_equal(
        slot.fixed_x,
        i16{5712},
        "random x 357 is stored in sixteenth-pixel units"
    );
    test.expect_equal(slot.fixed_y, i16{0}, "new streak begins on row zero");
    test.expect_equal(
        slot.horizontal_step,
        i16{0},
        "right-half zero drift remains zero after negation"
    );
    test.expect_equal(
        slot.vertical_step,
        i16{22},
        "random vertical step 6 receives the plus-16 bias"
    );
    test.expect_equal(
        slot.trail_limit,
        i16{23},
        "random trail 15 receives the plus-eight bias"
    );
    test.expect_equal(
        slot.remaining_frames,
        i16{32},
        "vertical steps above 15 select lifetime 32"
    );
    test.expect_equal(slot.field_c, i16{0}, "field +0x0c is cleared");
    test.expect_equal(
        slot.active_flags, i16{1}, "new slot writes active word one"
    );

    LegacyAniStreakEffect disabled_effect;
    LegacySecondaryRng disabled_random;
    disabled_random.seed(39U);
    FakeServices disabled_services;
    const auto disabled = disabled_effect.update(
        disabled_random, framebuffer, pixel_format, disabled_services
    );
    test.expect_equal(
        disabled.service_query_count,
        u32{1U},
        "the same trigger queries a disabled service"
    );
    test.expect_equal(
        disabled_effect.state().target_spawn_count,
        i16{0},
        "decrement below zero is clamped back to zero"
    );
    test.expect_false(
        disabled.scanned_slots,
        "disabled target with no live slots returns early"
    );
    test.expect_equal(
        disabled_random.index(),
        std::size_t{2U},
        "disabled trigger consumes no creation random values"
    );
}

void test_trail_pixels_and_state_advance(openswd3::test::Context& test) {
    LegacyAniStreakEffect effect;
    LegacySecondaryRng random;
    random.seed(39U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    services.enabled = true;
    static_cast<void>(
        effect.update(random, framebuffer, pixel_format, services)
    );

    const auto result =
        effect.update(random, framebuffer, pixel_format, services);
    test.expect_equal(
        result.visited_active_count,
        u32{1U},
        "second frame processes the first created slot"
    );
    test.expect_equal(
        result.created_count,
        u32{1U},
        "target one also creates one later inactive slot"
    );
    test.expect_equal(
        result.packed_color_count,
        u32{23U},
        "row zero is skipped from the inclusive 24-point trail"
    );
    test.expect_equal(
        result.adjusted_pixel_count,
        u32{23U},
        "rows one through 23 receive one-pixel adjustments"
    );
    test.expect_equal(
        result.pixel_failure_count,
        u32{0U},
        "the normal fixed vector remains inside storage"
    );
    test.expect_equal(
        effect.state().previous_live_count,
        i16{1},
        "one previously active slot survives"
    );
    test.expect_equal(
        effect.state().slots[0U].fixed_x,
        i16{5712},
        "zero horizontal drift keeps fixed x"
    );
    test.expect_equal(
        effect.state().slots[0U].fixed_y,
        i16{22},
        "outer state advances by vertical step once"
    );
    test.expect_equal(
        effect.state().slots[0U].remaining_frames,
        i16{31},
        "outer state decrements lifetime once"
    );
    test.expect_equal(
        effect.state().slots[1U].fixed_x,
        i16{7296},
        "second creation keeps the next bounded x value"
    );
    test.expect_equal(
        effect.state().slots[1U].horizontal_step,
        i16{-2},
        "right-half positive drift is negated"
    );
    test.expect_equal(
        effect.state().slots[1U].vertical_step,
        i16{21},
        "second creation keeps RNG call order"
    );
    test.expect_equal(
        effect.state().slots[1U].trail_limit,
        i16{10},
        "second creation keeps the final bounded value"
    );
    test.expect_equal(
        framebuffer.row_pixels(0U)[357U],
        u16{0U},
        "strict base-pointer comparison skips row zero"
    );
    test.expect_equal(
        framebuffer.row_pixels(1U)[357U],
        u16{0x0842U},
        "first drawn row adds grayscale channel value two"
    );
    test.expect_equal(
        framebuffer.row_pixels(15U)[357U],
        u16{0x7BDEU},
        "intensity 30 remains below channel saturation"
    );
    test.expect_equal(
        framebuffer.row_pixels(16U)[357U],
        u16{0x7FFFU},
        "intensity 32 saturates all effective channels"
    );
    test.expect_equal(
        framebuffer.row_pixels(23U)[357U],
        u16{0x7FFFU},
        "later trail pixels remain saturated"
    );
    test.expect_equal(
        legacy_framebuffer_logical_fnv1a64(framebuffer),
        std::uint64_t{0x7403975F3AB69BDDULL},
        "two fixed updates produce the assembly-derived framebuffer hash"
    );
}

void test_active_bit_wrap_and_bottom_count_quirk(
    openswd3::test::Context& test
) {
    LegacyAniStreakEffect effect;
    effect.state().target_spawn_count = 0;
    effect.state().previous_live_count = 1;
    LegacyAniStreakSlot& slot = effect.state().slots[0U];
    slot.fixed_x = 0;
    slot.fixed_y = 480;
    slot.horizontal_step = 2;
    slot.vertical_step = 16;
    slot.trail_limit = 0;
    slot.remaining_frames = 2;
    slot.active_flags = 3;

    LegacySecondaryRng random;
    random.seed(0x12345678U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    const auto result =
        effect.update(random, framebuffer, pixel_format, services);
    test.expect_equal(
        result.visited_active_count,
        u32{1U},
        "odd active word is treated as active"
    );
    test.expect_equal(
        result.adjusted_pixel_count,
        u32{0U},
        "row at fixed canvas end is not adjusted"
    );
    test.expect_equal(
        slot.active_flags,
        i16{0},
        "canvas-end row clears the complete active word"
    );
    test.expect_equal(
        slot.fixed_x,
        i16{32},
        "x advances by low-word horizontal times vertical"
    );
    test.expect_equal(
        slot.fixed_y, i16{496}, "y advances even after active is cleared"
    );
    test.expect_equal(
        slot.remaining_frames,
        i16{1},
        "lifetime decrements after the bounds clear"
    );
    test.expect_equal(
        effect.state().previous_live_count,
        i16{1},
        "nonzero lifetime is counted despite cleared active"
    );

    effect.state().previous_live_count = 1;
    slot.fixed_x = std::numeric_limits<i16>::max();
    slot.fixed_y = 0;
    slot.horizontal_step = 2;
    slot.vertical_step = 1;
    slot.trail_limit = -1;
    slot.remaining_frames = 1;
    slot.active_flags = 1;
    static_cast<void>(
        effect.update(random, framebuffer, pixel_format, services)
    );
    test.expect_equal(
        slot.fixed_x, i16{-32767}, "fixed x addition wraps at 16 bits"
    );
    test.expect_equal(
        slot.remaining_frames,
        i16{0},
        "lifetime reaches zero after a skipped trail"
    );
    test.expect_equal(slot.active_flags, i16{0}, "zero lifetime clears active");
    test.expect_equal(
        effect.state().previous_live_count,
        i16{0},
        "zero lifetime is not counted as live"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_reset_boundary_and_idle_rng(test);
    test_trigger_service_and_creation_vector(test);
    test_trail_pixels_and_state_advance(test);
    test_active_bit_wrap_and_bottom_count_quirk(test);
    return test.exit_code();
}
