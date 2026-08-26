#include "openswd3/battle/legacy_battle_full_frame_darkening.hpp"

#include <algorithm>
#include <bit>
#include <limits>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleFullFrameDarkeningState;
using openswd3::battle::LegacyBattleFullFrameDarkeningStatus;
using openswd3::battle::update_legacy_battle_full_frame_darkening;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::rendering::LegacyBlitEffectState;
using openswd3::rendering::LegacyFrameColorStatus;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacySurfaceGeometry;

}  // namespace

void test_battle_full_frame_darkening(openswd3::test::Context& test) {
    {
        LegacyBattleFullFrameDarkeningState state{.channel_delta = -2};
        LegacyFramebuffer framebuffer;
        LegacyBlitEffectState effects;
        std::ranges::fill(framebuffer.physical_pixels(), u16{0x7FFFU});
        const u16 guard_before =
            framebuffer.physical_pixels_with_read_guard().back();

        const auto result = update_legacy_battle_full_frame_darkening(
            state, framebuffer, effects
        );

        test.expect_true(
            result.status == LegacyBattleFullFrameDarkeningStatus::completed &&
                result.channel_calls == 3U &&
                result.red_status == LegacyFrameColorStatus::completed &&
                result.green_status == LegacyFrameColorStatus::completed &&
                result.blue_status == LegacyFrameColorStatus::completed &&
                result.applied_delta == -2 && result.decremented_delta == -4 &&
                result.clamped_to_zero == false && result.return_value == 0U &&
                state.channel_delta == -4,
            "full-frame darkening publishes one fixed delta to all three closed channel helpers before decrementing"
        );
        test.expect_true(
            effects.red_offset == -2 && effects.green_offset == -2 &&
                effects.blue_offset == -2 &&
                framebuffer.physical_pixels().front() == 0x77BDU &&
                framebuffer.physical_pixels().back() == 0x77BDU &&
                framebuffer.physical_pixels_with_read_guard().back() ==
                    guard_before,
            "full-frame darkening updates the complete 4B000-pixel canvas and reads but does not write the guard"
        );
    }

    {
        LegacyBattleFullFrameDarkeningState state{.channel_delta = -30};
        LegacyFramebuffer framebuffer;
        LegacyBlitEffectState effects;
        std::ranges::fill(framebuffer.physical_pixels(), u16{0x7FFFU});

        const auto result = update_legacy_battle_full_frame_darkening(
            state, framebuffer, effects
        );

        test.expect_true(
            result.status == LegacyBattleFullFrameDarkeningStatus::completed &&
                result.applied_delta == -30 &&
                result.decremented_delta == -32 && result.clamped_to_zero &&
                result.return_value == 1U && state.channel_delta == 0 &&
                framebuffer.physical_pixels().front() == 0x0421U,
            "signed result at minus thirty-two clamps the shared delta to zero and returns one after all channel writes"
        );
    }

    {
        LegacyBattleFullFrameDarkeningState state{.channel_delta = -29};
        LegacyFramebuffer framebuffer;
        LegacyBlitEffectState effects;

        const auto result = update_legacy_battle_full_frame_darkening(
            state, framebuffer, effects
        );

        test.expect_true(
            result.status == LegacyBattleFullFrameDarkeningStatus::completed &&
                result.decremented_delta == -31 &&
                result.clamped_to_zero == false && result.return_value == 0U &&
                state.channel_delta == -31,
            "signed result greater than minus thirty-two remains live and returns zero"
        );
    }

    {
        LegacyBattleFullFrameDarkeningState state{
            .channel_delta = std::numeric_limits<i32>::min()
        };
        LegacyFramebuffer framebuffer;
        LegacyBlitEffectState effects;

        const auto result = update_legacy_battle_full_frame_darkening(
            state, framebuffer, effects
        );

        test.expect_true(
            result.status == LegacyBattleFullFrameDarkeningStatus::completed &&
                result.decremented_delta ==
                    std::numeric_limits<i32>::max() - 1 &&
                result.return_value == 0U &&
                state.channel_delta == std::numeric_limits<i32>::max() - 1,
            "delta decrement wraps in the low dword before the signed minus-thirty-two comparison"
        );
    }

    {
        LegacyBattleFullFrameDarkeningState state{.channel_delta = -8};
        LegacyFramebuffer framebuffer(
            LegacySurfaceGeometry{
                .pitch_bytes = 4,
                .width = 1,
                .height = 1,
            }
        );
        LegacyBlitEffectState effects;
        effects.red_offset = 1;
        effects.green_offset = 2;
        effects.blue_offset = 3;

        const auto result = update_legacy_battle_full_frame_darkening(
            state, framebuffer, effects
        );

        test.expect_true(
            result.status ==
                    LegacyBattleFullFrameDarkeningStatus::red_typed_stop &&
                result.channel_calls == 1U &&
                result.red_status ==
                    LegacyFrameColorStatus::buffer_out_of_bounds &&
                state.channel_delta == -8 && result.return_value == 0U &&
                effects.red_offset == -8 && effects.green_offset == -8 &&
                effects.blue_offset == -8,
            "typed stop occurs at the first full-frame channel access after all three legacy offsets are published"
        );
    }
}
