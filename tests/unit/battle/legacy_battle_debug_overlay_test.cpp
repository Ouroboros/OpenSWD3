#include "openswd3/battle/legacy_battle_debug_overlay.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
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
    LegacyBattleDebugOverlayCallReply font_reset_reply{};
    LegacyBattleDebugOverlayCallReply draw_reply{};
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
        case LegacyBattleDebugOverlayCall::reserved_query_marker_position:
            return {.output_mask = 3U, .output_0 = 1U, .output_1 = 0U};
        case LegacyBattleDebugOverlayCall::reserved_query_marker_width:
            return {.eax = 2U};
        case LegacyBattleDebugOverlayCall::font_reset:
            return font_reset_reply;
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
        return draw_reply;
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
        : framebuffer(surface) {
        startup.window_rectangle = {0, 0, surface.width, surface.height};
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
        for (auto& actor : *startup.group_b_lifecycle) {
            actor.action_composition.action_kind = 3U;
            actor.action_execution.start_gate = 4U;
            actor.action_execution.position_x = 1U;
            actor.action_execution.position_y = 0U;
        }
        for (auto& actor : action.group_a_action_execution) {
            actor.action_kind = 3U;
            actor.start_gate = 4U;
        }
    }

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

}  // namespace

void test_battle_debug_overlay(openswd3::test::Context& test) {
    for (const bool enemy : {false, true}) {
        for (const u16 value : std::array<u16, 7>{
                 0U, 1U, 0xFFU, 0x100U, 0x7FFFU, 0x8000U, 0xFFFFU
             }) {
            for (const unsigned phase : {0U, 1U, 2U}) {
                auto fixture = std::make_unique<Fixture>();
                fixture->hotkeys.toggle_5244e0 = 1U;
                fixture->metrics.group_a_count = enemy ? 0U : 2U;
                fixture->metrics.group_b_count = enemy ? 2U : 0U;
                openswd3::battle::LegacyBattleDebugOverlayRequest request;
                auto& access = enemy ? request.group_b_action_kind_access
                                     : request.group_a_action_kind_access;
                auto& field = enemy
                    ? (*fixture->startup.group_b_lifecycle)[1U]
                          .action_composition.action_kind
                    : fixture->action.group_a_action_execution[1U].action_kind;
                field = 0x1234U;
                OverlayPort port;
                port.draw_reply.edx = 0xAABBCCDDU;
                port.on_draw = [&](const auto&) {
                    if (port.texts.size() == 1U) {
                        field = value;
                        access.action_kind_readable = phase != 1U;
                        access.return_address_readable = phase != 2U;
                    }
                };
                const auto result = draw_legacy_battle_debug_overlay(
                    fixture->bindings(), port, request
                );
                test.expect_true(
                    result.actor_action_kind.has_value() && field == value &&
                        (enemy ? result.group_b_rows : result.group_a_rows) ==
                            (phase == 0U ? 2U : 1U),
                    "action query observes the preceding draw's shared WORD write and retains completed rows"
                );
                if (phase == 0U) {
                    test.expect_true(
                        result.status ==
                                LegacyBattleDebugOverlayStatus::completed &&
                            result.actor_action_kind->value == value &&
                            port.texts.size() >= 2U &&
                            port.texts[1U].text.find(std::to_string(value)) !=
                                std::string::npos,
                        "both actor groups print the complete unsigned action WORD"
                    );
                } else {
                    using Status =
                        openswd3::battle::LegacyBattleActorActionKindStatus;
                    const u32 entry_value = enemy ? 0x10528030U : 2U;
                    const u32 expected_value = phase == 1U
                        ? entry_value
                        : (entry_value & 0xFFFF0000U) | value;
                    test.expect_true(
                        result.status ==
                                LegacyBattleDebugOverlayStatus::
                                    actor_action_kind_typed_stop &&
                            result.actor_action_kind->status ==
                                (phase == 1U
                                     ? Status::action_kind_read_typed_stop
                                     : Status::
                                           return_address_read_typed_stop) &&
                            result.actor_action_kind->value ==
                                (phase == 1U ? std::optional<u16>{}
                                             : std::optional<u16>{value}) &&
                            result.return_value == expected_value &&
                            result.return_ecx ==
                                (enemy ? 0x00528030U : 0x00505904U) &&
                            result.return_edx == (enemy ? 51U : 0xAABBCCDDU) &&
                            result.actor_start_gate.has_value() &&
                            result.actor_start_gate->value == 4U &&
                            result.formatted_texts == 1U &&
                            result.text_draws == 1U && port.texts.size() == 1U,
                        "second action read and return faults preserve parent values and suppress that row and later work"
                    );
                }
            }
        }
    }

    for (const bool enemy : {false, true}) {
        for (const u16 count :
             std::array<u16, 4>{0U, 0x100U, 0x8000U, 0xFFFFU}) {
            for (const unsigned phase : {0U, 1U, 2U}) {
                auto fixture = std::make_unique<Fixture>();
                fixture->hotkeys.toggle_5244e0 = 1U;
                fixture->metrics.group_a_count = enemy ? 0U : 2U;
                fixture->metrics.group_b_count = enemy ? 2U : 0U;
                openswd3::battle::LegacyBattleDebugOverlayRequest request;
                auto& access = enemy ? request.group_b_start_gate_access
                                     : request.group_a_start_gate_access;
                auto& field = enemy
                    ? (*fixture->startup.group_b_lifecycle)[1U]
                          .action_execution.start_gate
                    : fixture->action.group_a_action_execution[1U].start_gate;
                field = 0x1234U;
                OverlayPort port;
                port.on_draw = [&](const auto&) {
                    if (port.texts.size() == 1U) {
                        field = count;
                        access.start_gate_readable = phase != 1U;
                        access.return_address_readable = phase != 2U;
                    }
                };
                const auto result = draw_legacy_battle_debug_overlay(
                    fixture->bindings(), port, request
                );
                test.expect_true(
                    result.actor_start_gate.has_value() && field == count &&
                        (enemy ? result.group_b_rows : result.group_a_rows) ==
                            (phase == 0U ? 2U : 1U),
                    "overlay rereads the second actor after drawing the first and retains completed rows on failure"
                );
                if (phase == 0U) {
                    test.expect_true(
                        result.status ==
                                LegacyBattleDebugOverlayStatus::completed &&
                            result.actor_start_gate->value == count &&
                            port.texts.size() >= 2U &&
                            port.texts[1U].text.find(std::to_string(count)) !=
                                std::string::npos,
                        "overlay prints the complete unsigned WORD without boolean or signed conversion"
                    );
                } else {
                    using Status =
                        openswd3::battle::LegacyBattleActorStartGateStatus;
                    test.expect_true(
                        result.status ==
                                LegacyBattleDebugOverlayStatus::
                                    actor_start_gate_typed_stop &&
                            result.actor_start_gate->status ==
                                (phase == 1U
                                     ? Status::start_gate_read_typed_stop
                                     : Status::
                                           return_address_read_typed_stop) &&
                            result.actor_start_gate->value ==
                                (phase == 1U ? std::optional<u16>{}
                                             : std::optional<u16>{count}) &&
                            result.return_value == (phase == 1U ? 3U : count) &&
                            result.formatted_texts == 1U &&
                            result.text_draws == 1U && port.texts.size() == 1U,
                        "second actor read and return failures suppress formatting, drawing and the remaining overlay tail"
                    );
                }
            }
        }
    }

    {
        Fixture fixture({.pitch_bytes = 16, .width = 8, .height = 4});
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.hotkeys.toggle_53af68 = 1U;
        fixture.metrics.group_b_count = 1U;
        fixture.startup.window_rectangle = {12, 34, 3, 99};
        fixture.startup.enemies[0U].progress.progress = 30U;
        (*fixture.startup.group_b_lifecycle)[0U].action_execution.position_y =
            1U;
        auto pixels = fixture.framebuffer.physical_pixels();
        std::fill(pixels.begin(), pixels.end(), 0x1234U);
        OverlayPort port;
        port.metrics = &fixture.metrics;
        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);
        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.marker_actors == 1U && result.marker_pixels == 4U,
            "debug marker rows use the shared window right edge rather than framebuffer width"
        );
        for (std::size_t index = 0U; index < pixels.size(); ++index) {
            const bool marked =
                index == 4U || index == 5U || index == 7U || index == 8U;
            test.expect_true(
                pixels[index] == (marked ? 0xEEEEU : 0x1234U),
                "debug marker addressing ignores window left top and bottom and preserves other pixels"
            );
        }
    }

    {
        Fixture fixture;
        fixture.overlay.text_buffer[0] = 'x';
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.text_draws == 0U && result.formatted_texts == 0U &&
                result.return_value == 0x11112222U &&
                port.count(LegacyBattleDebugOverlayCall::font_reset) == 1U &&
                port.count(LegacyBattleDebugOverlayCall::font_style) == 1U &&
                fixture.overlay.text_buffer[0] == 'x',
            "disabled debug overlay skips every body read but still restores the font in reset-style order"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_b_count = 1U;
        (*fixture.startup.group_b_lifecycle)[0U]
            .action_composition.action_kind = 0x4567U;
        OverlayPort port;
        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {
            }
        );
        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.actor_action_kind.has_value() &&
                result.actor_action_kind->value == 0x4567U &&
                result.group_b_rows == 1U &&
                port.texts[0U].text.find("17767") != std::string::npos &&
                result.actor_start_gate.has_value() &&
                result.actor_start_gate->value == 4U,
            "Group-B overlay row formats the complete shared action WORD after the resolved actor prefix"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 1U;
        fixture.action.group_a_action_execution[0U].action_kind = 0x6789U;
        OverlayPort port;
        port.font_reset_reply.edx = 0xAABBCCDDU;
        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {
            }
        );
        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.actor_action_kind.has_value() &&
                result.actor_action_kind->value == 0x6789U &&
                result.group_a_rows == 1U &&
                port.texts[0U].text.find("26505") != std::string::npos &&
                result.actor_start_gate.has_value() &&
                result.actor_start_gate->value == 4U,
            "Group-A overlay row formats the complete shared action and startup WORDs"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 2U;
        fixture.action.group_a_action_execution[0U].action_kind = 0x1111U;
        fixture.action.group_a_action_execution[1U].action_kind = 0x2222U;
        OverlayPort port;
        port.draw_reply.edx = 0xCAFEBABEU;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.actor_action_kind.has_value() &&
                result.actor_action_kind->value == 0x2222U &&
                port.texts[1U].text.find("8738") != std::string::npos &&
                result.group_a_rows == 2U &&
                result.actor_start_gate.has_value() &&
                result.actor_start_gate->value == 4U,
            "later Group-A overlay rows read their own action WORD before the startup count and formatting"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_b_count = 1U;
        OverlayPort port;
        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {
                .group_b_action_kind_access = {.action_kind_readable = false},
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        actor_action_kind_typed_stop &&
                result.return_value == 0x10525508U &&
                result.return_ecx == 0x00525508U && result.return_edx == 50U &&
                result.actor_action_kind.has_value() &&
                !result.actor_action_kind->value.has_value() &&
                result.group_b_rows == 0U && result.formatted_texts == 0U &&
                result.text_draws == 0U && !result.actor_start_gate.has_value(),
            "Group-B overlay field stop preserves resolve and vitality prefix while suppressing lock, formatting, and remaining rows"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 1U;
        fixture.action.group_a_action_execution[0U].action_kind = 0x789AU;
        OverlayPort port;
        port.font_reset_reply.edx = 0x11223344U;
        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {
                .group_a_action_kind_access = {
                    .return_address_readable = false
                },
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        actor_action_kind_typed_stop &&
                result.return_value == 0x789AU &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x11223344U &&
                result.actor_action_kind.has_value() &&
                result.actor_action_kind->value == 0x789AU &&
                result.group_a_rows == 0U && result.formatted_texts == 0U &&
                result.text_draws == 0U && !result.actor_start_gate.has_value(),
            "Group-A overlay RET stop preserves replaced AX and loop entry residues while suppressing the row suffix"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_b_count = 1U;
        OverlayPort port;
        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {
                .group_b_start_gate_access = {.start_gate_readable = false},
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        actor_start_gate_typed_stop &&
                result.actor_action_kind.has_value() &&
                result.actor_start_gate.has_value() &&
                result.actor_start_gate->status ==
                    openswd3::battle::LegacyBattleActorStartGateStatus::
                        start_gate_read_typed_stop &&
                !result.actor_start_gate->value.has_value() &&
                result.return_value == 3U && result.return_ecx == 0x00525508U &&
                result.return_edx == 50U && result.group_b_rows == 0U &&
                result.formatted_texts == 0U && result.text_draws == 0U,
            "Group-B start-gate field stop preserves command and level prefix while suppressing formatting, drawing, and remaining rows"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 1U;
        fixture.action.group_a_action_execution[0U].action_kind = 0x789AU;
        fixture.action.group_a_action_execution[0U].start_gate = 0xBEEFU;
        OverlayPort port;
        port.font_reset_reply.edx = 0x11223344U;
        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {
                .group_a_start_gate_access = {.return_address_readable = false},
            }
        );
        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        actor_start_gate_typed_stop &&
                result.actor_action_kind.has_value() &&
                result.actor_start_gate.has_value() &&
                result.actor_start_gate->status ==
                    openswd3::battle::LegacyBattleActorStartGateStatus::
                        return_address_read_typed_stop &&
                result.actor_start_gate->value == 0xBEEFU &&
                result.return_value == 0xBEEFU &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x11223344U && result.group_a_rows == 0U &&
                result.formatted_texts == 0U && result.text_draws == 0U,
            "Group-A start-gate RET stop preserves replaced AX and caller EDX while suppressing the current row and common suffix"
        );
    }

    {
        Fixture fixture({.pitch_bytes = 16, .width = 8, .height = 2});
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.hotkeys.toggle_53af68 = 1U;
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
        fixture.action.selection_cache_gate_b = 2U;
        fixture.action.action_pending_aux = 3U;
        fixture.effects.group_a_feedback_actor = 0xFFFFU;
        fixture.overlay.selection_order[0] = 31U;
        fixture.overlay.selection_order[1] = 32U;
        fixture.overlay.selection_order[2] = 33U;
        fixture.overlay.selection_order[3] = 34U;
        fixture.final_actor.active_actor_code = 9U;
        fixture.action.current_actor_index = 0xFFFEU;
        fixture.action.frame_enabled = 3U;
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
                result.return_edx == 0x87654321U && result.text_draws == 27U &&
                result.formatted_texts == 24U && result.group_b_rows == 2U &&
                result.group_a_rows == 2U && result.startup_order_rows == 4U &&
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
                port.texts[18].text.find("UM:-2") != std::string::npos &&
                port.texts[19].text == "fMenu:4 mMove5" &&
                port.texts[20].text == "MsD:120 dRole1:65535 CanS:17185" &&
                port.texts[21].text == "MS:6 Stop:3 mStop2" &&
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
            result.actor_action_kind.has_value() &&
                result.actor_action_kind->value.has_value() &&
                result.actor_start_gate.has_value() &&
                result.actor_start_gate->status ==
                    openswd3::battle::LegacyBattleActorStartGateStatus::
                        completed &&
                port.count(
                    LegacyBattleDebugOverlayCall::reserved_query_marker_position
                ) == 0U &&
                port.count(
                    LegacyBattleDebugOverlayCall::reserved_query_marker_width
                ) == 0U &&
                result.current_coordinate_query_calls == 2U &&
                result.actor_progress_width.scaled_width == 2 &&
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
                result.text_draws == 0U &&
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
                    LegacyBattleDebugOverlayStatus::
                        actor_action_kind_typed_stop &&
                result.actor_action_kind.has_value() &&
                result.actor_action_kind->status ==
                    openswd3::battle::LegacyBattleActorActionKindStatus::
                        action_kind_read_typed_stop &&
                !result.actor_action_kind->value.has_value() &&
                result.group_a_rows == 10U && result.startup_order_rows == 0U &&
                result.text_draws == 10U &&
                port.count(LegacyBattleDebugOverlayCall::font_style) == 1U,
            "the eleventh group-A action query stops before the downstream dynamic order rows"
        );
    }

    {
        Fixture fixture;
        fixture.hotkeys.toggle_5244e0 = 1U;
        fixture.metrics.group_a_count = 10U;
        OverlayPort port;

        const auto result =
            draw_legacy_battle_debug_overlay(fixture.bindings(), port);

        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                result.group_a_rows == 10U &&
                result.actor_action_kind.has_value() &&
                result.startup_order_rows == 10U &&
                result.actor_order_rows == 10U &&
                result.selection_order_rows == 10U,
            "all ten canonical group-A actors reach the complete downstream dynamic order rows"
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
        fixture.overlay.marker_x = 9;
        fixture.overlay.marker_row = 10;
        auto& actor = (*fixture.startup.group_b_lifecycle)[0U].action_execution;
        actor.position_x = 7U;
        actor.position_y_read_accessible = false;
        OverlayPort port;

        const auto result = draw_legacy_battle_debug_overlay(
            fixture.bindings(),
            port,
            {.current_coordinate_query = {.entry_edx = 0xAABBCCDDU}}
        );

        test.expect_true(
            result.status ==
                    LegacyBattleDebugOverlayStatus::
                        current_coordinate_typed_stop &&
                result.current_coordinate_query.status ==
                    openswd3::battle::
                        LegacyBattleActorCurrentCoordinateQueryStatus::
                            position_y_read_typed_stop &&
                result.current_coordinate_query.return_eax == 7U &&
                result.current_coordinate_query.return_ecx == 0x00525508U &&
                result.current_coordinate_query.return_edx == 0x0053BF4AU &&
                !result.current_coordinate_query.flags.parity &&
                !result.current_coordinate_query.flags.zero &&
                !result.current_coordinate_query.flags
                     .auxiliary_carry_defined &&
                fixture.overlay.marker_x == 7 &&
                fixture.overlay.marker_row == 10 &&
                result.current_coordinate_query_calls == 1U &&
                result.marker_actors == 0U && result.marker_pixels == 0U &&
                port.count(
                    LegacyBattleDebugOverlayCall::reserved_query_marker_position
                ) == 0U,
            "debug marker query preserves its X prefix and blocks progress width and pixels on Y stop"
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
                result.text_draws == 7U &&
                result.current_coordinate_query_calls == 1U &&
                result.marker_actors == 0U && result.marker_pixels == 0U &&
                port.count(
                    LegacyBattleDebugOverlayCall::reserved_query_marker_position
                ) == 0U &&
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

    for (const u16 actor_word :
         std::array<u16, 4>{0U, 0x7FFFU, 0x8000U, 0xFFFFU}) {
        auto fixture = std::make_unique<Fixture>();
        fixture->hotkeys.toggle_5244e0 = 1U;
        fixture->hotkeys.toggle_53af68 = 1U;
        fixture->action.current_actor_index = 0x1234U;
        OverlayPort port;
        port.on_draw = [&](const auto& text) {
            if (text.x == 240U && text.y == 70U) {
                fixture->action.current_actor_index = actor_word;
            }
        };
        const auto result =
            draw_legacy_battle_debug_overlay(fixture->bindings(), port);
        const int signed_actor = actor_word < 0x8000U
            ? static_cast<int>(actor_word)
            : static_cast<int>(actor_word) - 0x10000;
        const auto summary =
            std::ranges::find_if(port.texts, [](const auto& text) {
                return text.x == 240U && text.y == 30U;
            });
        test.expect_true(
            result.status == LegacyBattleDebugOverlayStatus::completed &&
                summary != port.texts.end() &&
                summary->text.find("UM:" + std::to_string(signed_actor)) !=
                    std::string::npos &&
                fixture->action.current_actor_index == actor_word,
            "45E2F1 displays the current shared WORD with MOVSX after the preceding text callback"
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
