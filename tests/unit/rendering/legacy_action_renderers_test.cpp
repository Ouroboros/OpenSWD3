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
using openswd3::compat::u8;
using openswd3::input_time_rng::LegacySecondaryRng;
using openswd3::rendering::LegacyActionRenderResult;
using openswd3::rendering::LegacyActionSpritePorts;
using openswd3::rendering::LegacyActionSpriteRecord;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyBlitSourceLayout;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyFramePieceProvider;
using openswd3::rendering::LegacyPackedRowDrawPorts;
using openswd3::rendering::LegacyPackedRowEffect;
using openswd3::rendering::LegacyPackedRowEffectResult;
using openswd3::rendering::LegacyRleRowJitterState;

class RecordingActionPorts final : public LegacyActionSpritePorts {
public:
    [[nodiscard]] bool update_action_frame(
        LegacyActionSpriteRecord&
    ) noexcept override {
        ++calls;
        return calls != failure_call;
    }

    u32 calls{};
    u32 failure_call{};
};

class RecordingFrameProvider final : public LegacyFramePieceProvider {
public:
    RecordingFrameProvider() {
        palette[0x34U] = 0x1234U;
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        piece_indices.push_back(piece_index);
        piece = LegacyFramePiece{
            .source = LegacyBlitSource{
                .bytes = bytes,
                .layout = LegacyBlitSourceLayout::indexed_8,
                .palette = palette,
            },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<u8, 2> bytes{0x34U, 0x12U};
    std::array<u16, 256> palette{};
    std::vector<u32> resource_ids;
    std::vector<u32> piece_indices;
};

struct PackedRowCall {
    i32 x{};
    i32 y{};
    u32 color{};
    i32 length{};
};

class RecordingPackedRowPorts final : public LegacyPackedRowDrawPorts {
public:
    void draw_legacy_packed_row(
        const i32 destination_x,
        const i32 destination_y,
        const u32 color_pattern,
        const i32 length
    ) noexcept override {
        calls.push_back(PackedRowCall{
            .x = destination_x,
            .y = destination_y,
            .color = color_pattern,
            .length = length,
        });
    }

    std::vector<PackedRowCall> calls;
};

void test_moving_action_visibility_update_and_removal(
    openswd3::test::Context& test
) {
    LegacyFramebuffer framebuffer;
    const auto raster = framebuffer.geometry();
    RecordingActionPorts action_ports;
    action_ports.failure_call = 2U;
    RecordingFrameProvider frame_provider;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    std::list<LegacyActionSpriteRecord> records{
        LegacyActionSpriteRecord{
            .draw_offset_x = 2,
            .draw_offset_y = 3,
            .resource_id = 0x1234U,
            .frame_index = 7U,
            .target_x = 101,
            .integer_y = 101,
            .velocity_x = 1.0F,
            .velocity_y = 1.0F,
            .position_x = 100.0F,
            .position_y = 100.0F,
        },
        LegacyActionSpriteRecord{
            .movement_hold = 1U,
            .position_x = -72.0F,
            .position_y = 0.0F,
        },
    };

    const LegacyActionRenderResult result =
        openswd3::rendering::update_draw_legacy_moving_action_sprites(
            records,
            0,
            0,
            action_ports,
            frame_provider,
            framebuffer,
            raster,
            effects,
            jitter
        );

    test.expect_equal(result.visited_count, 2U, "both records update");
    test.expect_equal(
        result.action_update_failure_count,
        1U,
        "action failure is reported without stopping"
    );
    test.expect_equal(result.draw_count, 1U, "strict -72 edge is invisible");
    test.expect_equal(result.removed_count, 1U, "target window removes mover");
    test.expect_equal(records.size(), std::size_t{1U}, "one record remains");
    test.expect_equal(
        frame_provider.resource_ids.front(),
        0x1234U,
        "resource id is forwarded"
    );
    test.expect_equal(
        framebuffer.row_pixels(97U)[98U],
        static_cast<u16>(0x1234U),
        "draw uses pre-movement coordinates and offsets"
    );
}

void test_role_head_easing_ballistic_and_direct_source(
    openswd3::test::Context& test
) {
    LegacyFramebuffer framebuffer;
    const auto raster = framebuffer.geometry();
    RecordingActionPorts action_ports;
    RecordingFrameProvider frame_provider;
    frame_provider.palette[0x34U] = 0x7777U;
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    std::list<LegacyActionSpriteRecord> records{
        LegacyActionSpriteRecord{
            .resource_id = 1U,
            .integer_x = 0,
            .target_x = 2,
            .integer_y = 20,
        },
        LegacyActionSpriteRecord{
            .resource_id = 2U,
            .integer_x = 759,
            .horizontal_velocity = 2,
            .integer_y = 21,
        },
        LegacyActionSpriteRecord{
            .resource_id = 3U,
            .integer_x = 10,
            .horizontal_velocity = static_cast<openswd3::compat::i16>(
                0x8000U
            ),
            .target_x = 13,
            .integer_y = 22,
        },
    };

    const LegacyActionRenderResult result =
        openswd3::rendering::update_draw_legacy_role_head_sprites(
            records,
            action_ports,
            frame_provider,
            framebuffer,
            raster,
            effects,
            jitter
        );

    test.expect_equal(result.draw_count, 3U, "all heads draw before movement");
    test.expect_equal(result.removed_count, 1U, "ballistic head leaves bounds");
    test.expect_equal(records.size(), std::size_t{2U}, "easing heads remain");
    auto current = records.begin();
    test.expect_equal(current->integer_x, static_cast<openswd3::compat::i16>(2), "small easing step snaps");
    ++current;
    test.expect_equal(current->integer_x, static_cast<openswd3::compat::i16>(12), "0x8000 velocity selects easing");
    test.expect_equal(
        framebuffer.row_pixels(20U)[0U],
        static_cast<u16>(0x1234U),
        "role head forces direct source instead of palette lookup"
    );
}

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
            effects,
            kColors,
            random,
            draw_ports
        );

    test.expect_equal(result.visited_count, 5U, "all row effects visit");
    test.expect_equal(result.random_request_count, 5U, "one RNG call per dynamic row");
    test.expect_equal(result.draw_count, 7U, "all rows draw in their active mode");
    test.expect_equal(result.transitioned_to_simple_count, 2U, "grow and retract modes transition");
    test.expect_equal(result.removed_count, 2U, "outward modes remove at completion");
    test.expect_equal(effects.size(), std::size_t{3U}, "simple and transitioned records remain");
    test.expect_equal(draw_ports.calls[0].x, 10, "simple mode ignores offset array");
    test.expect_equal(draw_ports.calls[0].y, 20, "simple row order begins at zero");
    test.expect_equal(draw_ports.calls[1].y, 21, "simple row order is forward");
    test.expect_equal(draw_ports.calls[2].x, 32, "dynamic modes start at final row");
    test.expect_equal(draw_ports.calls[2].y, 41, "dynamic row order is reverse");
    auto transitioned = effects.begin();
    ++transitioned;
    test.expect_equal(transitioned->mode, static_cast<u16>(0x08AAU), "transition preserves low mode byte");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_moving_action_visibility_update_and_removal(test);
    test_role_head_easing_ballistic_and_direct_source(test);
    test_packed_row_modes(test);
    return test.exit_code();
}
