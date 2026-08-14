#include "test.hpp"

#include "openswd3/rendering/legacy_action_renderers.hpp"

#include <array>
#include <cstddef>
#include <list>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramebufferPackedRowDrawPorts;
using openswd3::rendering::LegacyPackedRowDrawPorts;
using openswd3::rendering::LegacyPackedRowEffect;
using openswd3::rendering::LegacyPackedRowEffectReleaseResult;
using openswd3::rendering::LegacyPackedRowEffectResult;
using openswd3::rendering::LegacyPixelConversionState;

struct PackedRowCall {
    i32 x{};
    i32 y{};
    u32 color{};
    i32 length{};
};

class RecordingPackedRowPorts final : public LegacyPackedRowDrawPorts {
public:
    [[nodiscard]] openswd3::rendering::LegacyPackedRowBlendStatus
    draw_legacy_packed_row(
        const i32 destination_x,
        const i32 destination_y,
        const u32 color_pattern,
        const i32 length
    ) noexcept override {
        calls.push_back(
            PackedRowCall{
                .x = destination_x,
                .y = destination_y,
                .color = color_pattern,
                .length = length,
            }
        );
        return openswd3::rendering::LegacyPackedRowBlendStatus::completed;
    }

    std::vector<PackedRowCall> calls;
};

void test_packed_row_modes(openswd3::test::Context& test) {
    std::list<LegacyPackedRowEffect> effects{
        LegacyPackedRowEffect{
            .base_x = 10,
            .base_y = 20,
            .limit = 8,
            .row_count = 2,
            .mode = 0x0801U,
        },
        LegacyPackedRowEffect{
            .base_x = 30,
            .base_y = 40,
            .limit = 10,
            .row_count = 2,
            .mode = 0x80AAU,
            .row_offsets = {1, 2},
            .row_lengths = {9, 9},
        },
        LegacyPackedRowEffect{
            .limit = 10,
            .row_count = 1,
            .mode = 0x4002U,
            .row_offsets = {1},
            .row_lengths = {0},
        },
        LegacyPackedRowEffect{
            .limit = 10,
            .row_count = 1,
            .mode = 0x2000U,
            .row_offsets = {9},
            .row_lengths = {0},
        },
        LegacyPackedRowEffect{
            .limit = 10,
            .row_count = 1,
            .mode = 0x1000U,
            .row_offsets = {4},
            .row_lengths = {3},
        },
    };
    constexpr std::array<u32, 1> kColors{0x11223344U};
    LegacySecondaryRng random;
    random.seed(0x12345678U);
    RecordingPackedRowPorts draw_ports;

    const LegacyPackedRowEffectResult result =
        openswd3::rendering::update_draw_legacy_packed_row_effects(
            effects, kColors, random, draw_ports
        );

    test.expect_equal(result.visited_count, 5U, "all row effects visit");
    test.expect_equal(
        result.random_request_count, 5U, "one RNG call per dynamic row"
    );
    test.expect_equal(
        result.draw_count, 7U, "all rows draw in their active mode"
    );
    test.expect_equal(
        result.transitioned_to_simple_count,
        2U,
        "grow and retract modes transition"
    );
    test.expect_equal(
        result.removed_count, 2U, "outward modes remove at completion"
    );
    test.expect_equal(
        effects.size(),
        std::size_t{3U},
        "simple and transitioned records remain"
    );
    test.expect_equal(
        draw_ports.calls[0].x, 10, "simple mode ignores offset array"
    );
    test.expect_equal(
        draw_ports.calls[0].y, 20, "simple row order begins at zero"
    );
    test.expect_equal(draw_ports.calls[1].y, 21, "simple row order is forward");
    test.expect_equal(
        draw_ports.calls[2].x, 32, "dynamic modes start at final row"
    );
    test.expect_equal(
        draw_ports.calls[2].y, 41, "dynamic row order is reverse"
    );
    auto transitioned = effects.begin();
    ++transitioned;
    test.expect_equal(
        transitioned->mode,
        static_cast<u16>(0x08AAU),
        "transition preserves low mode byte"
    );
}

void test_packed_row_framebuffer_port(openswd3::test::Context& test) {
    LegacyFramebuffer framebuffer;
    std::span<u16> row = framebuffer.row_pixels(7U);
    row[5U] = 0x7FFFU;
    row[6U] = 0x001FU;
    row[7U] = 0x1234U;
    const LegacyPixelConversionState format;
    LegacyFramebufferPackedRowDrawPorts draw_ports{framebuffer, format};
    const u32 color =
        openswd3::rendering::legacy_pack_color_pair(format, 31, 0, 0);

    test.expect_equal(
        draw_ports.draw_legacy_packed_row(5, 7, color, 3),
        openswd3::rendering::LegacyPackedRowBlendStatus::completed,
        "framebuffer row port completes"
    );
    test.expect_equal(
        row[5U], static_cast<u16>(0x6A73U), "row port first pixel"
    );
    test.expect_equal(
        row[6U], static_cast<u16>(0x1C13U), "row port second pixel"
    );
    test.expect_equal(
        row[7U], static_cast<u16>(0x1234U), "row port keeps odd tail"
    );
    test.expect_equal(
        draw_ports.draw_legacy_packed_row(-1, 7, color, 2),
        openswd3::rendering::LegacyPackedRowBlendStatus::
            destination_out_of_bounds,
        "framebuffer row port isolates negative destination"
    );
}

void test_packed_row_effect_release(openswd3::test::Context& test) {
    std::list<LegacyPackedRowEffect> effects{
        LegacyPackedRowEffect{},
        LegacyPackedRowEffect{
            .row_offsets = {1, 2},
            .row_lengths = {3, 4},
        },
        LegacyPackedRowEffect{
            .row_offsets = {5},
        },
    };

    const LegacyPackedRowEffectReleaseResult released =
        openswd3::rendering::release_legacy_packed_row_effects(effects);

    test.expect_true(
        effects.empty() && released.node_release_count == 3U,
        "sub_40F500 drains every node from the list head"
    );
    test.expect_true(
        released.row_offset_release_calls == 3U &&
            released.row_offset_owners_released == 2U &&
            released.row_length_release_calls == 3U &&
            released.row_length_owners_released == 1U,
        "sub_40F500 releases both owned arrays even when either is null"
    );

    const LegacyPackedRowEffectReleaseResult empty =
        openswd3::rendering::release_legacy_packed_row_effects(effects);
    test.expect_true(
        empty.node_release_count == 0U &&
            empty.row_offset_release_calls == 0U &&
            empty.row_length_release_calls == 0U,
        "sub_40F500 leaves an empty root untouched"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_packed_row_modes(test);
    test_packed_row_framebuffer_port(test);
    test_packed_row_effect_release(test);
    return test.exit_code();
}
