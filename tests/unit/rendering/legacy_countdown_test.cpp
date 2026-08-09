#include "test.hpp"

#include "openswd3/rendering/legacy_countdown.hpp"

#include <array>
#include <cstddef>
#include <span>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyBlitSource;
using openswd3::rendering::LegacyCountdownDisplayRequest;
using openswd3::rendering::LegacyCountdownDisplayResult;
using openswd3::rendering::LegacyCountdownDisplayStatus;
using openswd3::rendering::LegacyCountdownFlagPorts;
using openswd3::rendering::LegacyCountdownInitializationRequest;
using openswd3::rendering::LegacyCountdownPieceProvider;
using openswd3::rendering::LegacyCountdownState;
using openswd3::rendering::LegacyFramePiece;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacyRasterGeometryState;
using openswd3::rendering::LegacyRleRowJitterState;
using openswd3::rendering::LegacySurfaceGeometry;

class RecordingFlags final : public LegacyCountdownFlagPorts {
public:
    [[nodiscard]] bool query_internal_flag(
        const u32 index
    ) noexcept override {
        queried[query_count] = index;
        ++query_count;
        return enabled[index];
    }

    void set_internal_flag(const u32 index) noexcept override {
        set_indices[set_count] = index;
        ++set_count;
        enabled[index] = true;
    }

    void enable(const u32 index) noexcept {
        enabled[index] = true;
    }

    std::array<bool, 128> enabled{};
    std::array<u32, 8> queried{};
    std::array<u32, 8> set_indices{};
    std::size_t query_count{};
    std::size_t set_count{};
};

class RecordingPieceProvider final : public LegacyCountdownPieceProvider {
public:
    RecordingPieceProvider() {
        for (std::size_t index = 0U; index < pixels.size(); ++index) {
            const u16 color = static_cast<u16>(index + 1U);
            pixels[index][0] = static_cast<u8>(color);
            pixels[index][1] = static_cast<u8>(color >> 8U);
        }
    }

    [[nodiscard]] bool load_countdown_piece(
        const u32 action_id,
        const i32 action_index,
        LegacyFramePiece& piece
    ) noexcept override {
        action_ids[request_count] = action_id;
        action_indices[request_count] = action_index;
        ++request_count;
        if (action_index < 0 || action_index >= 16 ||
            action_index == unavailable_index) {
            return false;
        }

        piece = LegacyFramePiece{
            .source = LegacyBlitSource{
                .bytes = pixels[static_cast<std::size_t>(action_index)],
            },
            .width = static_cast<u16>(
                action_index == zero_width_index ? 0U : 1U
            ),
            .height = 1U,
        };
        return true;
    }

    std::array<std::array<u8, 2>, 16> pixels{};
    std::array<u32, 8> action_ids{};
    std::array<i32, 8> action_indices{};
    std::size_t request_count{};
    i32 unavailable_index{-1};
    i32 zero_width_index{-1};
};

[[nodiscard]] LegacyFramebuffer make_framebuffer() {
    return LegacyFramebuffer{LegacySurfaceGeometry{
        .pitch_bytes = 32,
        .width = 16,
        .height = 4,
    }};
}

[[nodiscard]] LegacyCountdownDisplayResult draw(
    LegacyFramebuffer& framebuffer,
    const LegacyCountdownState& state,
    RecordingFlags& flags,
    RecordingPieceProvider& provider,
    const i32 mode = 0
) {
    const LegacyRasterGeometryState raster = framebuffer.geometry();
    const LegacyBlitEffectState effects;
    LegacyRleRowJitterState jitter;
    return openswd3::rendering::draw_legacy_countdown(
        framebuffer,
        raster,
        state,
        flags,
        provider,
        LegacyCountdownDisplayRequest{
            .destination_x = 2,
            .destination_y = 1,
            .mode = mode,
        },
        effects,
        jitter
    );
}

template <std::size_t Size>
void expect_piece_order(
    openswd3::test::Context& test,
    const RecordingPieceProvider& provider,
    const std::array<i32, Size>& expected,
    const char* message
) {
    test.expect_equal(provider.request_count, expected.size(), message);
    for (std::size_t index = 0U; index < expected.size(); ++index) {
        test.expect_equal(provider.action_ids[index], 0x232CU, message);
        test.expect_equal(provider.action_indices[index], expected[index], message);
    }
}

void test_initialization_modes_and_wrapping(openswd3::test::Context& test) {
    LegacyCountdownState state;
    state.primary_value_004c97e8 = 1U;
    state.primary_value_004c97ec = 2U;
    state.secondary_value_004bab78 = 3U;
    state.secondary_value_004bab7c = 4U;
    RecordingFlags flags;

    openswd3::rendering::initialize_legacy_countdown(
        state,
        flags,
        LegacyCountdownInitializationRequest{
            .minutes = 2,
            .seconds = 3,
            .primary_transition_value = 0x12345678U,
            .mode = 0,
        }
    );
    test.expect_equal(state.primary_ticks, 3690U, "primary ticks use 30 Hz");
    test.expect_equal(state.primary_transition_value, 0x12345678U, "primary transition value");
    test.expect_equal(state.primary_value_004c97e8, 0U, "primary auxiliary one clears");
    test.expect_equal(state.primary_value_004c97ec, 0U, "primary auxiliary two clears");
    test.expect_equal(flags.set_count, 2U, "primary sets two flags");
    test.expect_equal(flags.set_indices[0], 0x10U, "primary flag first");
    test.expect_equal(flags.set_indices[1], 0x12U, "primary companion flag second");

    const u32 old_transition = state.primary_transition_value;
    flags.set_count = 0U;
    openswd3::rendering::initialize_legacy_countdown(
        state,
        flags,
        LegacyCountdownInitializationRequest{
            .minutes = 0x40000000,
            .seconds = 7,
            .primary_transition_value = 0xFFFFFFFFU,
            .mode = -1,
        }
    );
    const u32 expected_ticks = 30U *
        (7U + 60U * static_cast<u32>(0x40000000U));
    test.expect_equal(state.secondary_ticks, expected_ticks, "secondary arithmetic wraps at 32 bits");
    test.expect_equal(state.primary_transition_value, old_transition, "secondary ignores transition value");
    test.expect_equal(state.secondary_value_004bab78, 0U, "secondary auxiliary one clears");
    test.expect_equal(state.secondary_value_004bab7c, 0U, "secondary auxiliary two clears");
    test.expect_equal(flags.set_count, 1U, "secondary sets one flag");
    test.expect_equal(flags.set_indices[0], 0x4AU, "secondary active flag");
}

void test_five_piece_minutes_and_seconds(openswd3::test::Context& test) {
    LegacyCountdownState state;
    state.primary_ticks = 754U * 30U;
    RecordingFlags flags;
    flags.enable(0x10U);
    RecordingPieceProvider provider;
    LegacyFramebuffer framebuffer = make_framebuffer();

    const LegacyCountdownDisplayResult result = draw(
        framebuffer,
        state,
        flags,
        provider
    );
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::completed, "12:34 draw");
    test.expect_equal(result.displayed_seconds, 754, "ticks divide by 30");
    test.expect_equal(result.draw_call_count, 5U, "five visible pieces");
    constexpr std::array<i32, 5> kExpected{1, 2, 10, 3, 4};
    expect_piece_order(test, provider, kExpected, "12:34 action order");
    for (std::size_t index = 0U; index < kExpected.size(); ++index) {
        test.expect_equal(
            framebuffer.row_pixels(1U)[2U + index],
            static_cast<u16>(kExpected[index] + 1),
            "pieces advance by returned width"
        );
    }
    test.expect_equal(flags.query_count, 2U, "primary queries active then suppression");
    test.expect_equal(flags.queried[0], 0x10U, "primary active query");
    test.expect_equal(flags.queried[1], 0x4CU, "suppression query");
}

void test_omitted_leading_digit_and_negative_clamp(
    openswd3::test::Context& test
) {
    LegacyCountdownState state;
    state.primary_ticks = 65U * 30U;
    RecordingFlags flags;
    flags.enable(0x10U);
    RecordingPieceProvider provider;
    LegacyFramebuffer framebuffer = make_framebuffer();
    auto result = draw(framebuffer, state, flags, provider);
    constexpr std::array<i32, 4> kExpected{1, 10, 0, 5};
    expect_piece_order(test, provider, kExpected, "1:05 omits leading zero");
    test.expect_equal(result.draw_call_count, 4U, "four pieces without minute tens");

    state.primary_ticks = 0xFFFFFFE1U;
    flags.query_count = 0U;
    provider = RecordingPieceProvider{};
    LegacyFramebuffer negative_framebuffer = make_framebuffer();
    result = draw(negative_framebuffer, state, flags, provider);
    constexpr std::array<i32, 4> kZero{0, 10, 0, 0};
    expect_piece_order(test, provider, kZero, "negative seconds clamp to zero");
    test.expect_equal(result.displayed_seconds, 0, "negative division result clamps");
}

void test_visibility_gates_and_nonstandard_mode(
    openswd3::test::Context& test
) {
    LegacyCountdownState state;
    state.secondary_ticks = 754U * 30U;
    RecordingFlags flags;
    RecordingPieceProvider provider;
    LegacyFramebuffer framebuffer = make_framebuffer();

    auto result = draw(framebuffer, state, flags, provider, 0);
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::hidden_inactive, "primary inactive");
    test.expect_equal(flags.query_count, 1U, "inactive primary returns before suppression");

    flags = RecordingFlags{};
    result = draw(framebuffer, state, flags, provider, 1);
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::hidden_inactive, "secondary inactive");
    test.expect_equal(flags.queried[0], 0x4AU, "mode one checks secondary flag");

    flags = RecordingFlags{};
    flags.enable(0x10U);
    flags.enable(0x4CU);
    result = draw(framebuffer, state, flags, provider, 0);
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::hidden_suppressed, "global suppression");
    test.expect_equal(flags.query_count, 2U, "suppression follows active query");

    flags = RecordingFlags{};
    provider = RecordingPieceProvider{};
    LegacyFramebuffer nonstandard_framebuffer = make_framebuffer();
    result = draw(nonstandard_framebuffer, state, flags, provider, 2);
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::completed, "mode other than zero/one bypasses active gates");
    test.expect_equal(flags.query_count, 1U, "nonstandard mode only queries suppression");
    test.expect_equal(flags.queried[0], 0x4CU, "nonstandard suppression query");
}

void test_piece_and_geometry_failures(openswd3::test::Context& test) {
    LegacyCountdownState state;
    state.primary_ticks = 65U * 30U;
    RecordingFlags flags;
    flags.enable(0x10U);
    LegacyFramebuffer framebuffer = make_framebuffer();

    RecordingPieceProvider unavailable;
    unavailable.unavailable_index = 10;
    auto result = draw(framebuffer, state, flags, unavailable);
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::piece_unavailable, "missing colon piece");
    test.expect_equal(result.piece_request_count, 2U, "failure occurs on second request");
    test.expect_equal(result.draw_call_count, 1U, "only first piece was drawn");

    RecordingPieceProvider invalid;
    invalid.zero_width_index = 1;
    result = draw(framebuffer, state, flags, invalid);
    test.expect_equal(result.status, LegacyCountdownDisplayStatus::invalid_piece_geometry, "zero-width piece isolated");
    test.expect_equal(result.draw_call_count, 0U, "invalid geometry is rejected before blit");
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_initialization_modes_and_wrapping(test);
    test_five_piece_minutes_and_seconds(test);
    test_omitted_leading_digit_and_negative_clamp(test);
    test_visibility_gates_and_nonstandard_mode(test);
    test_piece_and_geometry_failures(test);
    return test.exit_code();
}
