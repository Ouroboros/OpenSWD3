#include "openswd3/battle/legacy_battle_debug_overlay.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorMetricState;
using openswd3::battle::LegacyBattleDebugHotkeyState;
using openswd3::battle::LegacyBattleDebugOverlayBindings;
using openswd3::battle::LegacyBattleDebugOverlayCall;
using openswd3::battle::LegacyBattleDebugOverlayCallReply;
using openswd3::battle::LegacyBattleDebugOverlayCallRequest;
using openswd3::battle::LegacyBattleDebugOverlayPort;
using openswd3::battle::LegacyBattleDebugOverlayState;
using openswd3::battle::LegacyBattleDebugOverlayStatus;
using openswd3::battle::LegacyBattleDebugOverlayTextRequest;
using openswd3::battle::LegacyBattleEffectCoordinatorState;
using openswd3::battle::LegacyBattleFinalActorStepState;
using openswd3::battle::LegacyBattleStartupState;
using openswd3::battle::draw_legacy_battle_debug_overlay;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::rendering::LegacyFramebuffer;
using openswd3::rendering::LegacySurfaceGeometry;

class OverlayPort final : public LegacyBattleDebugOverlayPort {
public:
    LegacyBattleActorMetricState* metrics{};
    bool provide_actor_word{true};
    bool skip_second_vitality_output{};
    u32 style_calls{};
    u32 vitality_queries{};
    std::vector<LegacyBattleDebugOverlayCallRequest> calls;
    std::vector<LegacyBattleDebugOverlayTextRequest> texts;
    std::unordered_map<u32, u16> actor_words;
    std::function<void(const LegacyBattleDebugOverlayTextRequest&)> on_draw;

    [[nodiscard]] LegacyBattleDebugOverlayCallReply invoke_debug_overlay(
        const LegacyBattleDebugOverlayCallRequest& request
    ) override {
        calls.push_back(request);
        switch (request.call) {
        case LegacyBattleDebugOverlayCall::font_style:
            ++style_calls;
            return {
                .eax = style_calls == 2U ? 0xAABBCCDDU : 0x11112222U,
                .ecx = style_calls == 2U ? 0x12345678U : 0U,
                .edx = style_calls == 2U ? 0x87654321U : 0U,
            };
        case LegacyBattleDebugOverlayCall::resolve_group_b_actor: {
            const u32 token = request.object_token + 0x10000000U;
            actor_words[token] = static_cast<u16>(
                50U + (request.object_token - 0x00525508U) / 0x00002B28U
            );
            return {.eax = token};
        }
        case LegacyBattleDebugOverlayCall::query_group_b_vitality:
            ++vitality_queries;
            if (skip_second_vitality_output && vitality_queries == 2U) {
                return {};
            }
            return {
                .output_mask = 1U,
                .output_0 =
                    12U + (request.object_token - 0x00525508U) / 0x00002B28U,
            };
        case LegacyBattleDebugOverlayCall::query_actor_command:
            return {.eax = 3U};
        case LegacyBattleDebugOverlayCall::query_actor_lock:
            return {.eax = 4U};
        case LegacyBattleDebugOverlayCall::query_marker_position:
            return {.output_mask = 3U, .output_0 = 1U, .output_1 = 0U};
        case LegacyBattleDebugOverlayCall::reserved_query_marker_width:
            return {.eax = 2U};
        case LegacyBattleDebugOverlayCall::font_reset:
            return {};
        }
        return {};
    }

    [[nodiscard]] std::optional<u16>
    read_debug_actor_level_word_54(const u32 token) override {
        if (!provide_actor_word) {
            return std::nullopt;
        }
        const auto found = actor_words.find(token);
        return found == actor_words.end() ? std::nullopt
                                          : std::optional<u16>{found->second};
    }

    [[nodiscard]] LegacyBattleDebugOverlayCallReply draw_debug_overlay_text(
        const LegacyBattleDebugOverlayTextRequest& request
    ) override {
        texts.push_back(request);
        if (on_draw) {
            on_draw(request);
        }
        return {};
    }

    [[nodiscard]] u32 count(const LegacyBattleDebugOverlayCall call) const {
        return static_cast<u32>(std::ranges::count_if(
            calls, [call](const auto& request) { return request.call == call; }
        ));
    }
};

struct Fixture {
    LegacyBattleDebugOverlayState overlay;
    LegacyBattleDebugHotkeyState hotkeys;
    LegacyBattleActorMetricState metrics;
    LegacyBattleStartupState startup;
    LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleActionDispatchState action;
    u32 message_state{};
    LegacyBattleEffectCoordinatorState effects;
    LegacyFramebuffer framebuffer;

    explicit Fixture(
        const LegacySurfaceGeometry& surface = LegacySurfaceGeometry{}
    )
        : framebuffer(surface) {}

    [[nodiscard]] LegacyBattleDebugOverlayBindings bindings() {
        return {
            .overlay = overlay,
            .hotkeys = hotkeys,
            .metrics = metrics,
            .startup = startup,
            .final_actor = final_actor,
            .action = action,
            .message_state = message_state,
            .effects = effects,
            .framebuffer = framebuffer,
        };
    }
};

[[nodiscard]] bool has_call(
    const OverlayPort& port,
    const LegacyBattleDebugOverlayCall call,
    const u32 object,
    const u32 argument_count,
    const u32 argument_0 = 0U
) {
    return std::ranges::any_of(port.calls, [&](const auto& request) {
        return request.call == call && request.object_token == object &&
            request.argument_count == argument_count &&
            request.arguments[0] == argument_0;
    });
}

}  // namespace

void test_battle_debug_overlay(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.overlay.text_buffer[0] = 'x';
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.port_calls == 2U && result.text_draws == 0U &&
                result.formatted_texts == 0U &&
                result.return_value == 0x11112222U &&
                port.count(LegacyBattleDebugOverlayCall::font_reset) == 1U &&
                port.count(LegacyBattleDebugOverlayCall::font_style) == 1U &&
                fixture.overlay.text_buffer[0] == 'x',
            "disabled debug overlay skips every body read but still restores the font in reset-style order"
        );
    }

    {
        Fixture fixture({.pitch_bytes = 16, .width = 8, .height = 2});
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.action.opponent_workspace[14U] = 1U;
        fixture.metrics.group_b_count = 2U;
        fixture.metrics.group_a_count = 2U;
        fixture.startup.enemies[0U].progress.progress = 30U;
        fixture.startup.enemies[1U].progress.progress = 30U;
        fixture.metrics.priority_actor_index = 8U;
        fixture.startup.reset.records_524788[0].value_00 = 10U;
        fixture.startup.reset.records_524788[1].value_00 = 11U;
        fixture.startup.reset.records_524788[2].value_00 = 12U;
        fixture.startup.reset.records_524788[3].value_00 = 13U;
        fixture.final_actor.actor_order[0] = 21U;
        fixture.final_actor.actor_order[1] = 22U;
        fixture.final_actor.frame_gate_a = 2U;
        fixture.final_actor.frame_gate_b = 3U;
        fixture.effects.group_a_feedback_actor = 0xFFFFU;
        fixture.overlay.selection_order[0] = 31U;
        fixture.overlay.selection_order[1] = 32U;
        fixture.overlay.selection_order[2] = 33U;
        fixture.overlay.selection_order[3] = 34U;
        fixture.final_actor.active_actor_code = 9U;
        fixture.overlay.battle_selector = -2;
        fixture.overlay.battle_mode = 3U;
        fixture.message_state = 4U;
        fixture.final_actor.pre_frame_gate_b = 5U;
        fixture.action.packed_actor_counter = 0x12345678U;
        fixture.overlay.selection_status = 0xABCD4321U;
        fixture.final_actor.published_actor_code = 6U;
        fixture.overlay.lock_count = 7U;
        fixture.overlay.tsw_cache_bytes = 12'345U;
        fixture.overlay.world_level = -8;
        fixture.overlay.initial_mode = -9;
        fixture.overlay.battle_frame = 10U;
        fixture.overlay.frame_divisor = 20;
        OverlayPort port;
        port.metrics = &fixture.metrics;
        port.skip_second_vitality_output = true;

        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(), port, {.vitality_stack_snapshot = 0xDEADBEEFU}
        );

        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.return_value == 0xAABBCCDDU &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0x87654321U && result.port_calls == 45U &&
                result.text_draws == 27U && result.formatted_texts == 24U &&
                result.group_b_rows == 2U && result.group_a_rows == 2U &&
                result.startup_order_rows == 4U &&
                result.actor_order_rows == 2U &&
                result.selection_order_rows == 4U &&
                result.marker_actors == 2U && result.marker_pixels == 8U,
            "enabled overlay formats every dynamic row and fixed status line before restoring the final registers"
        );
        test.expect_true(
            port.texts.size() == 27U &&
                static_cast<unsigned char>(port.texts[0].text[0]) == 0xA5U &&
                static_cast<unsigned char>(port.texts[0].text[1]) == 0xCDU &&
                port.texts[0].text.substr(5U, 3U) == "12 " &&
                port.texts[0].text.ends_with("lv:50") &&
                port.texts[1].text.substr(5U, 3U) == "12 " &&
                port.texts[1].text.ends_with("lv:51") &&
                static_cast<unsigned char>(port.texts[4].text[0]) == 0xA7U &&
                port.texts[4].x == 10U && port.texts[4].y == 50U &&
                port.texts[17].x == 240U && port.texts[17].y == 70U &&
                port.texts[19].text == "fMenu:4 mMove5" &&
                port.texts[20].text == "MsD:120 dRole1:65535 CanS:17185" &&
                port.texts[21].text == "MS:6 Stop:2 mStop3" &&
                port.texts[23].text == "TswMem:12K" &&
                port.texts[24].text == "wLl:-8 iMn:-9" &&
                port.texts[26].text == "FRAME:50" &&
                port.texts[26].font_token == 0x004C9A28U &&
                port.texts[26].surface_token == 0x004CD76CU &&
                port.texts[26].foreground == 0xFFFFU &&
                port.texts[26].height == 0x10U,
            "overlay preserves fixed coordinates tokens CP950-backed ordering and signed decimal formatting"
        );
        test.expect_true(
            has_call(
                port,
                LegacyBattleDebugOverlayCall::query_actor_command,
                0x00525508U,
                1U,
                50U
            ) &&
                has_call(
                    port,
                    LegacyBattleDebugOverlayCall::query_actor_lock,
                    0x005029D0U,
                    1U,
                    3U
                ) &&
                port.count(
                    LegacyBattleDebugOverlayCall::query_marker_position
                ) == 2U &&
                port.count(
                    LegacyBattleDebugOverlayCall::reserved_query_marker_width
                ) == 0U &&
                result.actor_progress_width_calls == 2U &&
                result.actor_progress_width.return_eax == 2U &&
                fixture.framebuffer.physical_pixels()[1] == 0xEEEEU &&
                fixture.framebuffer.physical_pixels()[2] == 0xEEEEU &&
                fixture.framebuffer.physical_pixels()[9] == 0xEEEEU &&
                fixture.framebuffer.physical_pixels()[10] == 0xEEEEU,
            "group rows keep their distinct object arguments and marker writes use two raw raster rows"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_b_count = 1U;
        OverlayPort port;
        port.provide_actor_word = false;

        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(), port, {.vitality_stack_snapshot = 0x778899AAU}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        resolved_actor_word_typed_stop &&
                fixture.overlay.resolved_actor_token == 0x10525508U &&
                result.port_calls == 4U && result.text_draws == 0U &&
                port.count(LegacyBattleDebugOverlayCall::font_style) == 1U &&
                port.count(LegacyBattleDebugOverlayCall::font_reset) == 1U,
            "missing resolved actor storage stops at the original word-54 read after resolve publication and level query"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 19U;
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::startup_record_typed_stop &&
                result.group_a_rows == 19U &&
                result.startup_order_rows == 18U && result.text_draws == 38U &&
                port.count(LegacyBattleDebugOverlayCall::font_style) == 1U,
            "the nineteenth startup-order read stops after all group-A rows the header and eighteen record draws"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 11U;
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::actor_order_typed_stop &&
                result.startup_order_rows == 11U &&
                result.actor_order_rows == 10U,
            "the eleventh actor-order read stops after the waiting header and ten published values"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 1U;
        OverlayPort port;
        port.metrics = &fixture.metrics;
        port.on_draw = [&](const LegacyBattleDebugOverlayTextRequest& request) {
            if (request.text.size() == 9U &&
                static_cast<unsigned char>(request.text[0]) == 0xB1U) {
                fixture.metrics.group_a_count = 19U;
            }
        };

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        selection_order_typed_stop &&
                result.startup_order_rows == 1U &&
                result.actor_order_rows == 1U &&
                result.selection_order_rows == 18U,
            "selection loop reloads the post-header dynamic count and stops only at its nineteenth real read"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_b_count = 1U;
        fixture.startup.enemies[0U].progress.progress_read_accessible = false;
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        actor_progress_width_typed_stop &&
                result.port_calls == 14U && result.text_draws == 7U &&
                result.actor_progress_width_calls == 1U &&
                result.actor_progress_width.return_eax == 0U &&
                result.actor_progress_width.return_ecx == 0x00525508U &&
                result.actor_progress_width.return_edx == 0U &&
                result.marker_actors == 0U && result.marker_pixels == 0U &&
                port.count(
                    LegacyBattleDebugOverlayCall::query_marker_position
                ) == 1U &&
                port.count(
                    LegacyBattleDebugOverlayCall::reserved_query_marker_width
                ) == 0U,
            "debug markers stop at the typed progress read after the position and row prefix"
        );
    }

    {
        Fixture fixture({.pitch_bytes = 16, .width = 8, .height = 1});
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_b_count = 1U;
        fixture.startup.enemies[0U].progress.progress = 30U;
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::framebuffer_typed_stop &&
                result.marker_actors == 0U && result.marker_pixels == 1U &&
                fixture.framebuffer.physical_pixels()[1] == 0xEEEEU &&
                result.formatted_texts == 4U && result.text_draws == 7U,
            "marker bottom-row failure preserves the first top pixel and every preceding text draw"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.overlay.frame_divisor = 0;
        fixture.overlay.text_buffer[0] = 'x';
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::frame_divisor_zero &&
                result.text_draws == 12U && result.formatted_texts == 9U &&
                port.texts.back().text == "bf:0" &&
                port.count(LegacyBattleDebugOverlayCall::font_style) == 1U &&
                port.count(LegacyBattleDebugOverlayCall::font_reset) == 1U,
            "zero frame divisor stops at signed idiv after bf text without running the font restoration tail"
        );
    }
}
