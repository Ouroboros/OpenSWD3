#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_effect_frame.hpp"
#include "legacy_battle_frame_refresh_fixture.hpp"
#include "test.hpp"

#include <bit>
#include <memory>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::test::LegacyBattleFrameRefreshFixture;

class FrameRefreshPort final : public LegacyBattleActionDispatchPort,
                               public LegacyBattleEffectCallPort,
                               public LegacyBattleFrameEffectPort,
                               public LegacyBattleFrameRefreshFixture {
public:
    LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest&) override {
        return {};
    }

    LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest&) override {
        return {};
    }

    LegacyBattleActionRotationUpdateSnapshot
    update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        return {.domain_token = 1U};
    }

    LegacyBattleFrameEffectSurfaceReply surface_operation(
        const LegacyBattleFrameEffectSurfaceRequest& request
    ) override {
        surface_calls.push_back(request);
        return {.callee_returned = true};
    }

    std::vector<LegacyBattleFrameEffectSurfaceRequest> surface_calls;
};

}  // namespace

void test_battle_frame_refresh(openswd3::test::Context& test) {
    {
        FrameRefreshPort port;
        LegacyBattleActionDispatchPort& action = port;
        LegacyBattleEffectCallPort& effect = port;
        action.frame_refresh_state().snapshot_word_36 = 0x1234U;
        action.frame_effect_control_state().red_factor = -32768;
        test.expect_true(
            &action.frame_refresh_state() == &effect.frame_refresh_state() &&
                &action.frame_effect_control_state() ==
                    &effect.frame_effect_control_state() &&
                effect.frame_refresh_state().snapshot_word_36 == 0x1234U &&
                effect.frame_effect_control_state().red_factor == -32768 &&
                effect.frame_refresh_state().active_surface_token ==
                    0xFFFFFFFFU,
            "action and effect ports share refresh and color storage"
        );
    }

    {
        LegacyBattleFrameRefreshFixture port;
        auto& state = port.frame_refresh_state();
        auto& control = port.frame_effect_control_state();
        state.snapshot_word_36 = 0x1234U;
        state.snapshot_word_38 = 0x5678U;
        state.snapshot_word_3a = 0x9ABCU;
        control.red_factor = 0x1234;
        control.green_factor = 0x5678;
        control.blue_factor =
            std::bit_cast<openswd3::compat::i16>(u16{0x9ABCU});
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            result.status == LegacyBattleFrameRefreshStatus::completed &&
                !result.refreshed && port.refresh_source_queries == 0U &&
                port.refresh_events.empty(),
            "equal colors return without accessing the background or calling a service"
        );
    }

    {
        LegacyBattleFrameRefreshFixture port;
        auto& state = port.frame_refresh_state();
        state.surface_tokens = {0x1111U, 0x2222U};
        state.viewport_token = 0x4444U;
        port.refresh_lock_results = {0xA110U, 0xA220U, 0xA330U};
        port.frame_effect_control_state().red_factor = 3;
        bool published_before_unlock = true;
        port.on_refresh_unlock = [&](u32, u32 pixels) {
            published_before_unlock =
                published_before_unlock && port.refresh_pixels == pixels;
            port.refresh_background[0] =
                port.refresh_unlocks.size() == 1U ? 0xA100U : 0xA200U;
        };
        port.on_refresh_draw = [&](const auto&) {
            port.refresh_pixels =
                port.refresh_draws.size() == 1U ? 0xB100U : 0xC100U;
        };
        port.on_refresh_red = [&](const auto&) {
            port.refresh_pixels += 0x100U;
        };
        port.on_refresh_green = [&](const auto&) {
            port.refresh_pixels += 0x100U;
        };
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            result.status == LegacyBattleFrameRefreshStatus::completed &&
                published_before_unlock && port.refresh_draws.size() == 2U &&
                port.refresh_draws[0].source == 0xA100U &&
                port.refresh_draws[1].source == 0xA200U &&
                port.refresh_draws[0].pixels == 0xA110U &&
                port.refresh_draws[1].pixels == 0xA220U &&
                port.refresh_draws[0].width == 640U &&
                port.refresh_draws[0].height == 480U &&
                port.refresh_blit.source_token == 0xA200U &&
                port.refresh_pixels == 0xA330U,
            "refresh publishes lock results before unlock and reloads each background after unlock"
        );
        test.expect_true(
            port.refresh_red.size() == 2U && port.refresh_green.size() == 2U &&
                port.refresh_blue.size() == 2U &&
                port.refresh_red[0].pixels == 0xB100U &&
                port.refresh_green[0].pixels == 0xB200U &&
                port.refresh_blue[0].pixels == 0xB300U &&
                port.refresh_red[1].pixels == 0xC100U &&
                port.refresh_green[1].pixels == 0xC200U &&
                port.refresh_blue[1].pixels == 0xC300U,
            "each color channel reloads the shared pixel address"
        );
    }

    {
        LegacyBattleFrameRefreshFixture port;
        auto& state = port.frame_refresh_state();
        auto& control = port.frame_effect_control_state();
        control.red_factor = 3;
        control.green_factor = 4;
        control.blue_factor = 5;
        std::size_t audio_count{};
        bool snapshots_before_lock{};
        port.on_refresh_audio = [&] {
            control.red_factor = ++audio_count == 1U ? 7 : -32768;
        };
        port.on_refresh_red = [&](const auto&) {
            control.green_factor = port.refresh_red.size() == 1U ? -9 : 32767;
        };
        port.on_refresh_green = [&](const auto&) {
            control.blue_factor = port.refresh_green.size() == 1U ? -11 : -1;
        };
        port.on_refresh_blue = [&](const auto&) {
            if (port.refresh_blue.size() == 2U) {
                control.red_factor = -3;
                control.green_factor = 5;
                control.blue_factor = -7;
            }
        };
        port.on_refresh_lock = [&](u32) {
            if (port.refresh_locks.size() == 3U) {
                snapshots_before_lock = state.snapshot_word_36 == 0xFFFDU &&
                    state.snapshot_word_38 == 5U &&
                    state.snapshot_word_3a == 0xFFF9U &&
                    state.refresh_pending == 1U;
                control.red_factor = 0;
                control.green_factor = 0;
                control.blue_factor = 0;
            }
        };
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            result.status == LegacyBattleFrameRefreshStatus::completed &&
                snapshots_before_lock && port.refresh_red[0].amount == 3 &&
                port.refresh_red[1].amount == -32768 &&
                port.refresh_green[0].amount == -5 &&
                port.refresh_green[1].amount == 32766 &&
                port.refresh_blue[0].amount == -6 &&
                port.refresh_blue[1].amount == -2 && control.red_factor == 0 &&
                state.snapshot_word_36 == 0xFFFDU,
            "signed halves round down and each channel reads current colors while snapshots precede final lock"
        );
    }

    for (std::size_t ordinal = 0U; ordinal < 16U; ++ordinal) {
        LegacyBattleFrameRefreshFixture port;
        port.frame_effect_control_state().red_factor = 1;
        port.refresh_stop_ordinal = ordinal;
        port.refresh_lock_results = {0x100U, 0x200U, 0x300U};
        port.refresh_pixels = 0xDEADU;
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            result.status != LegacyBattleFrameRefreshStatus::completed &&
                port.refresh_events.size() == ordinal + 1U &&
                result.surface_iterations ==
                    (ordinal < 7U        ? 0U
                         : ordinal < 14U ? 1U
                                         : 2U) &&
                port.frame_refresh_state().refresh_pending ==
                    (ordinal >= 14U ? 1U : 0U),
            "every service failure stops before later operations and retains only completed iterations and snapshots"
        );
    }

    {
        LegacyBattleFrameRefreshFixture port;
        port.frame_effect_control_state().red_factor = 1;
        port.refresh_source_available = false;
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            result.status ==
                    LegacyBattleFrameRefreshStatus::source_binding_typed_stop &&
                port.refresh_unlocks.empty(),
            "missing source stops at the first pixel publication after audio and lock"
        );
    }

    {
        FrameRefreshPort port;
        auto& state = port.frame_refresh_state();
        auto& control = port.frame_effect_control_state();
        state.surface_tokens = {0x1111U, 0x2222U};
        state.viewport_token = 0x4444U;
        state.final_surface_token = 0x5555U;
        port.refresh_lock_results = {0xAAAAU, 0xBBBBU, 0xCCCCU};
        control.red_factor = 3;
        control.green_factor = -1;
        control.blue_factor = -3;
        const auto result = refresh_legacy_battle_frame(port);
        test.expect_true(
            result.refreshed && result.surface_iterations == 2U &&
                port.refresh_red[0].amount == 1 &&
                port.refresh_red[1].amount == 2 &&
                port.refresh_green[0].amount == -1 &&
                port.refresh_green[1].amount == -2 &&
                port.refresh_blue[0].amount == -2 &&
                port.refresh_blue[1].amount == -4 &&
                port.refresh_red[0].pixel_count == 0x3C000U &&
                port.refresh_locks ==
                    std::vector<u32>{0x1111U, 0x2222U, 0x4444U} &&
                port.refresh_unlocks.back() ==
                    std::pair<u32, u32>{0x4444U, 0xCCCCU} &&
                state.active_surface_token == 0x5555U &&
                port.refresh_pixels == 0xCCCCU &&
                port.refresh_events ==
                    std::vector<std::string>{
                        "audio",
                        "lock",
                        "unlock",
                        "background",
                        "red",
                        "green",
                        "blue",
                        "audio",
                        "lock",
                        "unlock",
                        "background",
                        "red",
                        "green",
                        "blue",
                        "lock",
                        "unlock"
                    },
            "two surfaces retain the exact call order and publish the final viewport"
        );
        LegacyBattleFrameEffectState effect;
        LegacyBattleActionRotationCacheState rotation_cache;
        control.primary_suppression = 1U;
        auto action = std::make_unique<LegacyBattleActionDispatchState>();
        action->current_actor_index = 0U;
        port.actor_metric_state().priority_actor_index = 0U;
        effect.cadence = 2;
        openswd3::rendering::LegacyFramebuffer framebuffer;
        openswd3::rendering::LegacyRasterGeometryState raster{};
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
        openswd3::rendering::LegacyBlitRequest blit{};
        openswd3::rendering::LegacyBlitEffectState effects{};
        openswd3::rendering::LegacyRleRowJitterState jitter{};
        LegacyBattleFrameEffectContext context{
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_request = blit,
            .shared_effects = effects,
            .jitter = jitter,
            .pending_rotation = port.effect_shift_state().actor_delta,
            .flash = port.screen_flash_state(),
            .refresh = state,
            .control = control,
            .current_actor_index = action->current_actor_index,
            .priority_actor_index =
                port.actor_metric_state().priority_actor_index,
            .color_initialization_gate =
                port.battle_color_initialization_gate(),
            .rotation_cache = rotation_cache,
        };
        const std::array<u32, 3> surfaces{0xA000U, 0xA100U, 0xA200U};
        LegacyBattleBackgroundState background;
        LegacyBattleBackgroundFrameEffectImagePort images{
            background, rotation_cache
        };
        const LegacyBattleFrameEffectSource source{
            .record = background.image_record, .images = images
        };
        const auto growth = update_legacy_battle_frame_effect(
            effect, port, context, source, surfaces, 0
        );
        test.expect_true(
            growth.status == LegacyBattleFrameEffectStatus::completed &&
                growth.surface_operation_calls == 1U &&
                port.surface_calls.back().source_token == 0xA100U &&
                state.refresh_pending == 2U &&
                state.active_surface_token == 0x5555U,
            "the next effect consumes the actual published refresh stage"
        );
        action->current_actor_index = 1U;
        effect.fade_active = 1U;
        const auto fade = update_legacy_battle_frame_effect(
            effect, port, context, source, surfaces, 0
        );
        test.expect_true(
            fade.status == LegacyBattleFrameEffectStatus::completed &&
                fade.surface_operation_calls == 1U &&
                port.surface_calls.back().source_token == 0xA100U &&
                port.surface_calls.back().effect_flags == 0U &&
                state.refresh_pending == 1U &&
                state.active_surface_token == 0x5555U,
            "fade reads the same refresh stage and active surface sentinel"
        );
    }
}
