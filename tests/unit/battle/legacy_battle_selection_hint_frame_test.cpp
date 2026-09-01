#include "openswd3/battle/legacy_battle_selection_hint_frame.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <ranges>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using Call = openswd3::battle::LegacyBattleSelectionHintFrameCall;
using Reply = openswd3::battle::LegacyBattleSelectionHintFrameCallReply;
using Request = openswd3::battle::LegacyBattleSelectionHintFrameCallRequest;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(const u32 action_id, const u32, const bool) override {
        constexpr std::array<u16, 8> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        if (action_id == failing_action_id) {
            return {};
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
    u32 failing_action_id{0xFFFFFFFFU};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x6200U + index);
            storage[index] = {
                static_cast<u8>(color), static_cast<u8>(color >> 8U)
            };
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (fail || piece_index >= storage.size()) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::vector<u8>, 9> storage;
    std::vector<u32> resource_ids;
    bool fail{};
};

class HintPort final
    : public openswd3::battle::LegacyBattleSelectionHintFramePort {
public:
    void reply(const Call call, const Reply& value) {
        replies[call].push_back(value);
    }

    [[nodiscard]] Reply
    invoke_selection_hint_frame(const Request& request) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return {};
        }
        return found->second[index++];
    }

    [[nodiscard]] std::vector<Request> calls_of(const Call call) const {
        std::vector<Request> selected;
        std::ranges::copy_if(
            calls,
            std::back_inserter(selected),
            [call](const Request& request) { return request.call == call; }
        );
        return selected;
    }

    std::vector<Request> calls;
    std::map<Call, std::vector<Reply>> replies;
    std::map<Call, std::size_t> reply_indices;
};

struct Fixture {
    Fixture() : raster(framebuffer.geometry()), action_updater(action_streams) {
        party_source.fill(0U);
        shared_request.target_height = 3;
        shared_request.opacity_step = 15;
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleSelectionHintFrameBindings bindings() {
        return {
            .state = state,
            .queued_actor_code = queued_actor_code,
            .party_source_words = party_source,
            .target_selection_block = target_selection_block,
            .published_actor_code = published_actor_code,
            .group_b_count = group_b_count,
            .mirror_mode = mirror_mode,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = shared_request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    u32 queued_actor_code{8U};
    std::array<u32, 0x32> party_source{};
    u32 target_selection_block{};
    u32 published_actor_code{2U};
    u32 group_b_count{8U};
    u32 mirror_mode{};
    openswd3::battle::LegacyBattleSelectionHintFrameState state;
    openswd3::asset_runtime::LegacyActionRecord panel_action_record;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    FrameProvider frame_provider;
    HintPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleSelectionHintFrameRequest
request() {
    return {
        .origin_x = 12U,
        .origin_y = 14U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .local_text_token = 0x70001000U,
        .local_current_token = 0x70001014U,
        .local_limit_token = 0x70001018U,
        .panel_rectangle_return_registers =
            {
                .eax = 0xAAAA0001U,
                .ecx = 0xBBBB0002U,
                .edx = 0xCCCC0003U,
            },
        .tiled_frame_return_registers =
            {
                .eax = 0xDDDD0001U,
                .ecx = 0xDDDD0002U,
                .edx = 0xDDDD0003U,
            },
        .color_fade_return_registers = {
            .eax = 0xEEEE0001U,
            .ecx = 0xEEEE0002U,
            .edx = 0xEEEE0003U,
        },
    };
}

void prepare_label_and_metric(Fixture& fixture, const u32 metric) {
    fixture.port.reply(
        Call::query_actor_label, {.eax = 0x00501000U, .text_length = 4U}
    );
    fixture.port.reply(Call::configure_font_width, {});
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::configure_font_width, {});
    fixture.port.reply(Call::query_metric_source, {.eax = 0x900U});
    fixture.port.legacy_battle_fixed_object_state().object_words[0U][1U] =
        (metric << 16U) | 0x900U;
}

}  // namespace

void test_battle_selection_hint_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.party_source[0U] = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.port_calls == 0U && result.return_eax == 1U &&
                result.return_ecx == 0U && result.return_edx == 0U,
            "selection hint skips party-source mode one with post-read registers"
        );
    }

    {
        Fixture fixture;
        fixture.target_selection_block = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.port_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0U,
            "selection hint skips the complete target-selection block value one"
        );
    }

    {
        Fixture zero;
        zero.published_actor_code = 0U;
        const auto zero_result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                zero.bindings(), zero.port, request()
            );
        Fixture too_large;
        too_large.group_b_count = 1U;
        const auto too_large_result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                too_large.bindings(), too_large.port, request()
            );
        test.expect_true(
            zero_result.port_calls == 0U && zero_result.return_ecx == 0U &&
                too_large_result.port_calls == 0U &&
                too_large_result.return_ecx == 2U,
            "selection hint preserves signed one-based published-code guards"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        party_source_typed_stop &&
                result.return_eax == 0x11111111U && result.return_ecx == 0U &&
                result.return_edx == 0xFFFFFFD8U,
            "selection hint stops at the first invalid five-dword party-source access"
        );
    }

    {
        Fixture fixture;
        fixture.published_actor_code = 9U;
        fixture.group_b_count = 9U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        group_b_actor_typed_stop &&
                result.actor_label_query_calls == 0U &&
                result.return_eax == 3105U && result.return_edx == 0U,
            "selection hint permits count nine through the legacy guard then stops at the first actor call"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 9U);
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto labels = fixture.port.calls_of(Call::query_actor_label);
        const auto widths = fixture.port.calls_of(Call::configure_font_width);
        const auto draws = fixture.port.calls_of(Call::draw_text);
        const auto metrics = fixture.port.calls_of(Call::query_metric_source);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.panel_x == 12U && result.panel_y == 14U &&
                result.label_character_count == 2U && result.label_drawn &&
                !result.metric_text_drawn && !result.fade_drawn &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.panel_action_record.action_id == 0x233BU &&
                fixture.panel_action_record.base_variant == 0U &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xBBBB0077U,
            "selection hint draws the nonmirrored name panel and stops below metric threshold ten"
        );
        test.expect_true(
            labels.size() == 1U && labels[0U].eax == 690U &&
                labels[0U].edx == 0U && widths.size() == 2U &&
                widths[0U].arguments[0U] == 20U &&
                widths[1U].arguments[0U] == 16U && draws.size() == 1U &&
                draws[0U].arguments[1U] == 14U &&
                draws[0U].arguments[2U] == 14U &&
                draws[0U].eax ==
                    openswd3::battle::kLegacyBattleSelectionHintFontToken &&
                draws[0U].edx == 14U && metrics.size() == 1U &&
                metrics[0U].eax == 2762U && metrics[0U].edx == 690U,
            "selection hint preserves actor scaling, width twenty then sixteen and label draw registers"
        );
        test.expect_true(
            result.return_eax == 9U && result.return_ecx == 0x004B9F00U &&
                result.return_edx == 0x00000900U &&
                result.fixed_count_lookup.path ==
                    openswd3::battle::LegacyBattleFixedCountPath::existing_root,
            "selection hint below ten returns the typed fixed-count lookup register state"
        );
    }

    {
        Fixture fixture;
        fixture.mirror_mode = 1U;
        fixture.port.reply(
            Call::query_actor_label, {.eax = 0x00501000U, .text_length = 6U}
        );
        fixture.port.reply(Call::configure_font_width, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_width, {});
        fixture.port.reply(Call::query_metric_source, {.eax = 0x900U});
        fixture.port.legacy_battle_fixed_object_state().object_words[0U][1U] =
            (10U << 16U) | 0x900U;
        fixture.port.reply(
            Call::query_metric_pair,
            {
                .publish_metric_pair = true,
                .metric_current = 123U,
                .metric_limit = 456U,
            }
        );
        fixture.port.reply(
            Call::draw_text,
            {.eax = 0xFA000001U, .ecx = 0xFA000002U, .edx = 0xFA000003U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        const std::array<u8, 12> expected{
            0xA5U, 0xCDU, 0xA9U, 0x52U, 0x3AU, '1', '2', '3', '/', '4', '5', '6'
        };
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.panel_x == 558U && result.metric_text_drawn &&
                !result.fade_drawn && result.formatted_text_length == 12U &&
                std::equal(
                    expected.begin(),
                    expected.end(),
                    result.formatted_text.begin()
                ) &&
                draws.size() == 2U && draws[1U].arguments[1U] == 418U &&
                draws[1U].arguments[2U] == 12U &&
                draws[1U].text_token == 0x70001000U &&
                draws[1U].eax ==
                    openswd3::battle::kLegacyBattleSelectionHintFontToken &&
                draws[1U].edx == 418U && result.return_eax == 0xFA000001U,
            "selection hint mirror one centers the name and draws signed vitality text on the mirrored side"
        );
    }

    {
        Fixture fixture;
        fixture.mirror_mode = 2U;
        prepare_label_and_metric(fixture, 10U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.panel_x == 12U && draws.size() == 2U &&
                draws[1U].arguments[1U] == 0xFFFFFF80U,
            "selection hint preserves mirror-equals-one name placement but mirror-nonzero metric placement"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair,
            {.metric_current = 12U, .metric_limit = 34U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::query_fade_width, {.eax = 20U});
        fixture.port.reply(Call::query_fade_color, {.eax = 0xA5A51234U});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        const auto colors = fixture.port.calls_of(Call::query_fade_color);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.metric_text_drawn && result.fade_drawn &&
                result.fade_width == 20U && result.fade_color == 0xA5A51234U &&
                result.color_fade_calls == 1U && colors.size() == 1U &&
                colors[0U].arguments[0U] == 0U &&
                colors[0U].arguments[1U] == 0U &&
                colors[0U].arguments[2U] == 24U &&
                fixture.state.color_fade.source_argument_slot ==
                    std::array<u8, 4>{0x34U, 0x12U, 0xA5U, 0xA5U} &&
                result.return_eax == 0xEEEE0001U &&
                result.return_ecx == 0xEEEE0002U &&
                result.return_edx == 0xEEEE0003U,
            "selection hint threshold fifteen draws the queried-width color fade and returns its configured registers"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(
            Call::query_fade_width,
            {.eax = 0U, .ecx = 0x91000002U, .edx = 0x91000003U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            !result.fade_drawn && result.fade_color_calls == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x91000002U &&
                result.return_edx == 0x91000003U,
            "selection hint zero fade width skips color and fade while retaining width-query registers"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 10U);
        fixture.port.reply(
            Call::query_metric_pair,
            {.metric_current = 0x80000000U, .metric_limit = 0x80000000U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        format_buffer_typed_stop &&
                result.formatted_text_length == 20U &&
                result.text_draw_calls == 1U && result.fade_width_calls == 0U,
            "selection hint formats signed decimals and stops at the first write beyond its twenty-byte stack buffer"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            Call::query_actor_label, {.eax = 0x00501000U, .text_length = 4U}
        );
        fixture.port.reply(Call::configure_font_width, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_width, {});
        fixture.port.reply(Call::query_metric_source, {.eax = 0x900U});
        fixture.port.legacy_battle_fixed_object_state().object_words[0U][0U] =
            0x78001234U;
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        fixed_count_typed_stop &&
                result.metric_source_calls == 1U &&
                result.metric_value_calls == 1U &&
                result.fixed_count_lookup.stopped_token == 0x78001234U &&
                result.metric_pair_calls == 0U &&
                result.fade_width_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0x78001234U &&
                result.return_edx == 0x00000900U,
            "selection hint preserves metric-source and lookup register prefixes when the fixed-count successor is unmapped"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        prepare_label_and_metric(fixture, 9U);
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        panel_rectangle_typed_stop &&
                result.panel_action_update_calls == 1U &&
                result.tiled_frame_calls == 0U && result.font_width_calls == 0U,
            "selection hint panel rectangle stop preserves only the actor and action-update prefix"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.fail = true;
        prepare_label_and_metric(fixture, 9U);
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        tiled_frame_typed_stop &&
                result.tiled_frame_calls == 1U && result.font_width_calls == 0U,
            "selection hint tiled frame stop blocks all text and metrics"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        prepare_label_and_metric(fixture, 9U);
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.tiled_frame_calls == 1U && result.label_drawn,
            "selection hint action update failure remains non-branching"
        );
    }

    {
        Fixture fixture;
        prepare_label_and_metric(fixture, 15U);
        fixture.port.reply(
            Call::query_metric_pair, {.metric_current = 1U, .metric_limit = 2U}
        );
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::query_fade_width, {.eax = 20U});
        fixture.port.reply(Call::query_fade_color, {.eax = 0x0000FFFFU});
        const auto result =
            openswd3::battle::draw_legacy_battle_selection_hint_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleSelectionHintFrameStatus::
                        color_fade_typed_stop &&
                result.color_fade_calls == 1U && !result.fade_drawn &&
                result.return_eax == 0xEEEE0001U,
            "selection hint propagates the closed color-fade unsupported-source stop after all prior calls"
        );
    }
}
