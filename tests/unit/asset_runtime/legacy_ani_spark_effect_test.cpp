#include "test.hpp"

#include "openswd3/asset_runtime/legacy_ani_spark_effect.hpp"
#include "openswd3/input_time_rng/legacy_secondary_rng.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>

namespace {

using openswd3::asset_runtime::kLegacyAniSparkServiceId;
using openswd3::asset_runtime::kLegacyAniSparkSlotCount;
using openswd3::asset_runtime::LegacyAniSparkEffect;
using openswd3::asset_runtime::LegacyAniSparkServicePort;
using openswd3::asset_runtime::LegacyAniSparkSlot;
using openswd3::asset_runtime::LegacyAniSparkStatus;
using openswd3::compat::i16;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyPixelConversionState;
using openswd3::rendering::LegacySurfaceGeometry;
using openswd3::rendering::legacy_framebuffer_logical_fnv1a64;

class FakeServices final : public LegacyAniSparkServicePort {
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

void test_loader_state_counter_reset_and_idle_rng(
    openswd3::test::Context& test
) {
    LegacyAniSparkEffect effect;
    test.expect_equal(effect.state().slots.size(), kLegacyAniSparkSlotCount,
                      "the physical scan owns 96 slots");
    test.expect_true(
        std::ranges::all_of(
            effect.state().slots,
            [](const LegacyAniSparkSlot& slot) {
                return slot.active_flags == 0;
            }
        ),
        "process storage begins loader-zeroed"
    );

    effect.state().slots[0U].active_flags = 3;
    effect.state().slots[95U].fixed_x = 123;
    effect.state().previous_live_count = 4;
    effect.state().target_spawn_count = 1;
    effect.reset_counters();
    test.expect_equal(effect.state().previous_live_count, i16{0},
                      "scene reset clears the previous live count");
    test.expect_equal(effect.state().target_spawn_count, i16{0},
                      "scene reset clears the target spawn count");
    test.expect_equal(effect.state().slots[0U].active_flags, i16{3},
                      "scene reset preserves an active slot");
    test.expect_equal(effect.state().slots[95U].fixed_x, i16{123},
                      "scene reset preserves the final physical slot");

    effect.state().slots[0U] = {};
    LegacySecondaryRng random;
    random.seed(0x12345678U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    const auto idle = effect.update(
        random, framebuffer, pixel_format, services
    );
    test.expect_equal(idle.status, LegacyAniSparkStatus::ready,
                      "idle update completes");
    test.expect_false(idle.scanned_slots,
                      "zero counters skip the 96-slot scan");
    test.expect_equal(random.index(), std::size_t{2U},
                      "idle update still consumes random 1000");
    test.expect_equal(services.call_count, u32{0U},
                      "random 478 does not query service 22");

    LegacyFramebuffer short_framebuffer{
        LegacySurfaceGeometry{.pitch_bytes = 0x500, .width = 640, .height = 1}
    };
    const std::size_t before_short = random.index();
    const auto short_result = effect.update(
        random, short_framebuffer, pixel_format, services
    );
    test.expect_equal(short_result.status,
                      LegacyAniSparkStatus::framebuffer_too_small,
                      "short physical framebuffer is isolated");
    test.expect_equal(random.index(), before_short,
                      "short framebuffer consumes no random values");

    LegacyAniSparkEffect final_slot_effect;
    LegacyAniSparkSlot& final_slot =
        final_slot_effect.state().slots[kLegacyAniSparkSlotCount - 1U];
    final_slot.vertical_step = 1;
    final_slot.point_count = 0;
    final_slot.remaining_height = 1;
    final_slot.active_flags = 1;
    final_slot_effect.state().previous_live_count = 1;
    LegacySecondaryRng final_slot_random;
    final_slot_random.seed(0x12345678U);
    const auto final_slot_result = final_slot_effect.update(
        final_slot_random, framebuffer, pixel_format, services
    );
    test.expect_equal(final_slot_result.visited_active_count, u32{1U},
                      "the scan reaches physical slot 95");
    test.expect_equal(final_slot.active_flags, i16{0},
                      "the final slot receives the normal frame-tail update");
}

void test_trigger_service_and_creation_vector(
    openswd3::test::Context& test
) {
    LegacyAniSparkEffect effect;
    LegacySecondaryRng random;
    random.seed(39U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    services.enabled = true;

    const auto result = effect.update(
        random, framebuffer, pixel_format, services
    );
    test.expect_equal(result.service_query_count, u32{1U},
                      "random 953 queries the service port");
    test.expect_equal(services.call_count, u32{1U},
                      "the service port is called once");
    test.expect_equal(services.last_service_id,
                      kLegacyAniSparkServiceId,
                      "the conditional query uses service 22");
    test.expect_true(result.scanned_slots,
                     "positive target enters the slot scan");
    test.expect_equal(result.created_count, u32{1U},
                      "target one creates one inactive slot");
    test.expect_equal(result.visited_active_count, u32{0U},
                      "a new slot is not processed in its creation frame");
    test.expect_equal(random.index(), std::size_t{10U},
                      "trigger plus four creation values consume ten words");
    test.expect_equal(effect.state().target_spawn_count, i16{1},
                      "enabled service raises the target to one");
    test.expect_equal(effect.state().previous_live_count, i16{0},
                      "new slots are absent from the live count");

    const LegacyAniSparkSlot& slot = effect.state().slots[0U];
    test.expect_equal(slot.fixed_x, i16{5712},
                      "random x 357 is stored in sixteenth pixels");
    test.expect_equal(slot.fixed_y, i16{0},
                      "new particles begin at row zero");
    test.expect_equal(slot.horizontal_step, i16{0},
                      "right-half zero drift remains zero");
    test.expect_equal(slot.vertical_step, i16{1},
                      "random vertical zero receives the plus-one bias");
    test.expect_equal(slot.point_count, i16{1},
                      "creation fixes the per-frame point count to one");
    test.expect_equal(slot.remaining_height, i16{97},
                      "height is vertical times 160 minus random 63");
    test.expect_equal(slot.phase, i16{0},
                      "creation clears the horizontal phase");
    test.expect_equal(slot.active_flags, i16{1},
                      "creation writes active word one");

    LegacyAniSparkEffect disabled_effect;
    LegacySecondaryRng disabled_random;
    disabled_random.seed(39U);
    FakeServices disabled_services;
    const auto disabled = disabled_effect.update(
        disabled_random, framebuffer, pixel_format, disabled_services
    );
    test.expect_equal(disabled.service_query_count, u32{1U},
                      "the same trigger queries a disabled service");
    test.expect_equal(disabled_effect.state().target_spawn_count, i16{0},
                      "decrement below zero is clamped to zero");
    test.expect_false(disabled.scanned_slots,
                      "disabled target and zero live count return early");
    test.expect_equal(disabled_random.index(), std::size_t{2U},
                      "disabled trigger consumes no creation values");
}

void test_nine_pixel_kernel_and_state_advance(
    openswd3::test::Context& test
) {
    LegacyAniSparkEffect effect;
    LegacyAniSparkSlot& slot = effect.state().slots[0U];
    slot.fixed_x = 1600;
    slot.fixed_y = 100;
    slot.horizontal_step = 1;
    slot.vertical_step = 2;
    slot.point_count = 1;
    slot.remaining_height = 480;
    slot.phase = 4;
    slot.active_flags = 1;
    effect.state().previous_live_count = 1;

    LegacySecondaryRng random;
    random.seed(0x12345678U);
    LegacyFramebuffer framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    const auto result = effect.update(
        random, framebuffer, pixel_format, services
    );

    test.expect_equal(result.visited_active_count, u32{1U},
                      "one active slot is visited");
    test.expect_equal(result.packed_color_count, u32{1U},
                      "one in-range point performs the unused color pack");
    test.expect_equal(result.adjusted_pixel_count, u32{9U},
                      "one point adjusts the full nine-pixel kernel");
    test.expect_equal(result.pixel_failure_count, u32{0U},
                      "the fixed kernel remains inside storage");
    test.expect_equal(result.invalid_phase_count, u32{0U},
                      "phase four selects a valid table entry");

    const auto row_99 = framebuffer.row_pixels(99U);
    const auto row_100 = framebuffer.row_pixels(100U);
    const auto row_101 = framebuffer.row_pixels(101U);
    test.expect_equal(row_100[101U], u16{0x7FFFU},
                      "center receives intensity 31");
    test.expect_equal(row_100[100U], u16{0x3DEFU},
                      "left neighbor receives intensity 15");
    test.expect_equal(row_100[102U], u16{0x3DEFU},
                      "right neighbor receives intensity 15");
    test.expect_equal(row_99[101U], u16{0x3DEFU},
                      "upper neighbor receives intensity 15");
    test.expect_equal(row_101[101U], u16{0x3DEFU},
                      "lower neighbor receives intensity 15");
    test.expect_equal(row_99[100U], u16{0x1CE7U},
                      "upper-left diagonal receives intensity seven");
    test.expect_equal(row_101[102U], u16{0x1CE7U},
                      "lower-right diagonal receives intensity seven");
    test.expect_equal(
        legacy_framebuffer_logical_fnv1a64(framebuffer),
        std::uint64_t{0xF7080E84910EFC5BULL},
        "the fixed nine-pixel framebuffer vector is stable"
    );

    test.expect_equal(slot.fixed_x, i16{1602},
                      "frame x advances by low-word horizontal times vertical");
    test.expect_equal(slot.fixed_y, i16{102},
                      "frame y advances by the vertical step");
    test.expect_equal(slot.phase, i16{5},
                      "phase advances once per frame");
    test.expect_equal(slot.remaining_height, i16{478},
                      "remaining height loses the vertical step");
    test.expect_equal(slot.active_flags, i16{1},
                      "positive remaining height stays active");
    test.expect_equal(effect.state().previous_live_count, i16{1},
                      "the surviving slot is counted");
}

void test_flattened_x_boundary_and_bottom_count_quirk(
    openswd3::test::Context& test
) {
    LegacyAniSparkEffect flattened_effect;
    LegacyAniSparkSlot& flattened = flattened_effect.state().slots[0U];
    flattened.fixed_x = -16;
    flattened.fixed_y = 100;
    flattened.vertical_step = 1;
    flattened.point_count = 1;
    flattened.remaining_height = 480;
    flattened.active_flags = 1;
    flattened_effect.state().previous_live_count = 1;

    LegacySecondaryRng flattened_random;
    flattened_random.seed(0x12345678U);
    LegacyFramebuffer flattened_framebuffer;
    LegacyPixelConversionState pixel_format;
    FakeServices services;
    static_cast<void>(flattened_effect.update(
        flattened_random,
        flattened_framebuffer,
        pixel_format,
        services
    ));
    test.expect_equal(
        flattened_framebuffer.row_pixels(99U)[639U],
        u16{0x7FFFU},
        "negative x crosses into the preceding physical scanline"
    );

    LegacyAniSparkEffect bottom_effect;
    LegacyAniSparkSlot& bottom = bottom_effect.state().slots[0U];
    bottom.fixed_y = 479;
    bottom.vertical_step = 1;
    bottom.point_count = 1;
    bottom.remaining_height = 10;
    bottom.phase = 28;
    bottom.active_flags = 3;
    bottom_effect.state().previous_live_count = 1;

    LegacySecondaryRng bottom_random;
    bottom_random.seed(0x12345678U);
    LegacyFramebuffer bottom_framebuffer;
    const auto bottom_result = bottom_effect.update(
        bottom_random, bottom_framebuffer, pixel_format, services
    );
    test.expect_equal(bottom_result.visited_active_count, u32{1U},
                      "bit zero alone marks a slot active");
    test.expect_equal(bottom_result.adjusted_pixel_count, u32{0U},
                      "row 479 is excluded from the nine-pixel kernel");
    test.expect_equal(bottom.active_flags, i16{0},
                      "row 479 clears the complete active word");
    test.expect_equal(bottom.fixed_y, i16{480},
                      "state still advances after the bounds clear");
    test.expect_equal(bottom.phase, i16{0},
                      "phase 28 advances and wraps to zero");
    test.expect_equal(bottom.remaining_height, i16{9},
                      "remaining height still decreases after bounds clear");
    test.expect_equal(bottom_effect.state().previous_live_count, i16{1},
                      "positive height is counted despite cleared active");

    bottom_effect.state().previous_live_count = 1;
    bottom.fixed_x = std::numeric_limits<i16>::max();
    bottom.fixed_y = 0;
    bottom.horizontal_step = 2;
    bottom.vertical_step = 2;
    bottom.point_count = 0;
    bottom.remaining_height = 1;
    bottom.active_flags = 1;
    static_cast<void>(bottom_effect.update(
        bottom_random, bottom_framebuffer, pixel_format, services
    ));
    test.expect_equal(bottom.fixed_x, i16{-32765},
                      "fixed x multiply-add wraps at 16 bits");
    test.expect_equal(bottom.remaining_height, i16{-1},
                      "height subtraction wraps before signed comparison");
    test.expect_equal(bottom.active_flags, i16{0},
                      "nonpositive height clears active");
    test.expect_equal(bottom_effect.state().previous_live_count, i16{0},
                      "nonpositive height is not counted as live");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_loader_state_counter_reset_and_idle_rng(test);
    test_trigger_service_and_creation_vector(test);
    test_nine_pixel_kernel_and_state_advance(test);
    test_flattened_x_boundary_and_bottom_count_quirk(test);
    return test.exit_code();
}
