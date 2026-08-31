#include "openswd3/battle/legacy_battle_control_panel_frame.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <ranges>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using Call = openswd3::battle::LegacyBattleControlPanelFrameCall;
using Reply = openswd3::battle::LegacyBattleControlPanelFrameCallReply;
using Request = openswd3::battle::LegacyBattleControlPanelFrameCallRequest;
using DefinitionLoadReply =
    openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadReply;
using DefinitionLoadRequest =
    openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadRequest;
using NameCopyReply =
    openswd3::battle::LegacyBattleGroupBActionItemNameCopyReply;
using NameCopyRequest =
    openswd3::battle::LegacyBattleGroupBActionItemNameCopyRequest;

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x6300U + index);
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
        calls.push_back({resource_id, piece_index});
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
    std::vector<std::array<u32, 2>> calls;
    bool fail{};
};

class ControlPort final
    : public openswd3::battle::LegacyBattleControlPanelFramePort {
public:
    void reply(const Call call, const Reply& value) {
        replies[call].push_back(value);
    }

    [[nodiscard]] Reply
    invoke_control_panel_frame(const Request& request) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request);
        }
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

    void reply_definition(const DefinitionLoadReply& value) {
        definition_replies.push_back(value);
    }

    [[nodiscard]] DefinitionLoadReply
    load_action_item_definition(const DefinitionLoadRequest& request) override {
        definition_requests.push_back(request);
        if (on_definition_load) {
            on_definition_load(request);
        }
        if (definition_reply_index >= definition_replies.size()) {
            return {};
        }
        return definition_replies[definition_reply_index++];
    }

    [[nodiscard]] NameCopyReply
    copy_action_item_name(const NameCopyRequest& request) override {
        copy_requests.push_back(request);
        if (copy_reply_index >= copy_replies.size()) {
            return {};
        }
        return copy_replies[copy_reply_index++];
    }

    std::vector<Request> calls;
    std::map<Call, std::vector<Reply>> replies;
    std::map<Call, std::size_t> reply_indices;
    std::function<void(const Request&)> on_call;
    std::vector<DefinitionLoadRequest> definition_requests;
    std::vector<DefinitionLoadReply> definition_replies;
    std::size_t definition_reply_index{};
    std::function<void(const DefinitionLoadRequest&)> on_definition_load;
    std::vector<NameCopyRequest> copy_requests;
    std::vector<NameCopyReply> copy_replies;
    std::size_t copy_reply_index{};
};

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>>
definition(const char marker) {
    auto bytes = std::make_shared<std::array<u8, 0xA4>>();
    (*bytes)[0U] = static_cast<u8>(marker);
    (*bytes)[1U] = static_cast<u8>('0');
    return bytes;
}

struct Fixture {
    Fixture() : raster(framebuffer.geometry()) {
        selection_text.fill(0xA5A5A5A5U);
        group_b_actors[1U].resource_token = 0x73001234U;
        shared_request.target_height = 3;
        shared_request.opacity_step = 15;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleControlPanelFrameBindings
    bindings() {
        return {
            .state = state,
            .shared_color_fade = color_fade,
            .alternate_selection_limit = alternate_selection_limit,
            .selected_group_b_index = selected_group_b_index,
            .group_b_actors = group_b_actors,
            .transition_value_a = transition_value_a,
            .transition_value_b = transition_value_b,
            .selection_text_workspace = selection_text,
            .framebuffer = framebuffer,
            .clip = clip,
            .shared_request = shared_request,
            .shared_effects = effects,
            .jitter = jitter,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleControlPanelFrameState state;
    openswd3::battle::LegacyBattleColorFadeState color_fade;
    u32 alternate_selection_limit{2U};
    u16 selected_group_b_index{1U};
    std::array<openswd3::battle::LegacyBattleActorGroupBElementState, 8>
        group_b_actors;
    u32 transition_value_a{0xAAAAAAAAU};
    u32 transition_value_b{0xBBBBBBBBU};
    std::array<u32, 6> selection_text{};
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
    FrameProvider frame_provider;
    ControlPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleControlPanelFrameRequest
request(const u32 selected = 1U) {
    openswd3::battle::LegacyBattleControlPanelFrameRequest value{
        .origin_x = 112U,
        .origin_y = 58U,
        .selected_index = selected,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .local_primary_value_token = 0x70002000U,
    };
    value.border_return_registers[0U] = {
        .eax = 0xA1000001U,
        .ecx = 0xA1000002U,
        .edx = 0xA1000003U,
    };
    value.border_return_registers[1U] = {
        .eax = 0xA2000001U,
        .ecx = 0xA2000002U,
        .edx = 0xA2000003U,
    };
    return value;
}

void prepare_no_options(Fixture& fixture) {
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::configure_font_reset, {});
    fixture.port.reply(Call::configure_font_style, {});
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::configure_font_style, {});
    fixture.port.reply(Call::draw_text, {});
    for (u32 index = 0U; index < 2U; ++index) {
        fixture.port.reply(Call::query_special_option, {.eax = 0U});
    }
    fixture.port.reply(Call::configure_font_style, {});
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::configure_font_style, {});
}

}  // namespace

void test_battle_control_panel_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        prepare_no_options(fixture);
        const auto result =
            openswd3::battle::draw_legacy_battle_control_panel_frame(
                fixture.bindings(), fixture.port, request(1U)
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        const auto styles = fixture.port.calls_of(Call::configure_font_style);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        completed &&
                result.border_calls == 2U && result.font_reset_calls == 1U &&
                result.primary_query_calls == 3U &&
                result.special_query_calls == 2U && result.primary_rows == 0U &&
                result.special_rows == 0U &&
                result.release_selected_index == 2U &&
                result.row_trace_count == 2U &&
                result.rows[0U].selected_index == 1U &&
                result.rows[0U].selected &&
                result.rows[1U].selected_index == 2U &&
                !result.rows[1U].selected,
            "control panel draws attack and release around absent compressed options"
        );
        test.expect_true(
            draws.size() == 4U &&
                draws[0U].text_token ==
                    openswd3::battle::
                        kLegacyBattleControlPanelControlTextToken &&
                draws[0U].arguments[1U] == 112U &&
                draws[0U].arguments[2U] == 26U &&
                draws[1U].text_token ==
                    openswd3::battle::
                        kLegacyBattleControlPanelAttackTextToken &&
                draws[2U].text_token ==
                    openswd3::battle::
                        kLegacyBattleControlPanelAttackTextToken &&
                draws[3U].text_token ==
                    openswd3::battle::
                        kLegacyBattleControlPanelReleaseTextToken &&
                draws[3U].arguments[2U] == 78U && styles.size() == 4U &&
                styles[0U].arguments[0U] == 0xFFFEU &&
                styles[1U].arguments[0U] == 0xF000U &&
                styles.back().arguments[0U] == 0xFFFEU,
            "control panel preserves fixed labels, selected redraw and final normal style"
        );
        test.expect_true(
            std::ranges::all_of(
                fixture.selection_text,
                [](const u32 value) { return value == 0U; }
            ) && fixture.transition_value_a == 0xAAAAAAAAU &&
                fixture.transition_value_b == 0xBBBBBBBBU,
            "control panel clears the shared twenty-four-byte text before every unavailable query"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_reset, {});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        auto& resource = fixture.group_b_actors[1U].resource_bytes;
        write_word(resource, 0x66U, 100U);
        write_word(resource, 0x6AU, 1U);
        write_word(resource, 0x6EU, 1U);
        fixture.port.reply_definition({.definition = definition('A')});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply_definition({.definition = definition('B')});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply_definition({.definition = definition('C')});
        std::size_t load_index = 0U;
        fixture.port.on_definition_load =
            [&fixture, &load_index](const DefinitionLoadRequest&) {
                constexpr std::array<u16, 3> values{100U, 200U, 300U};
                write_word(
                    fixture.group_b_actors[1U].resource_bytes,
                    0x66U,
                    values[load_index++]
                );
            };
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        for (u32 index = 0U; index < 2U; ++index) {
            fixture.port.reply(Call::query_special_option, {.eax = 1U});
            fixture.port.reply(Call::configure_font_style, {});
            fixture.port.reply(Call::draw_text, {});
        }
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_style, {});
        const auto result =
            openswd3::battle::draw_legacy_battle_control_panel_frame(
                fixture.bindings(), fixture.port, request(3U)
            );
        const auto& primary = fixture.port.definition_requests;
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        completed &&
                result.primary_rows == 3U && result.special_rows == 2U &&
                result.visible_option_rows == 5U &&
                result.release_selected_index == 7U &&
                result.row_trace_count == 7U &&
                result.rows[1U].selected_index == 2U &&
                result.rows[1U].y == 78U &&
                result.rows[2U].selected_index == 3U &&
                result.rows[2U].y == 98U && result.rows[2U].selected &&
                result.rows[5U].selected_index == 6U &&
                result.rows[6U].selected_index == 7U,
            "control panel compresses three primary and two special successes into one-based rows"
        );
        test.expect_true(
            fixture.transition_value_a == 200U &&
                fixture.transition_value_b == 0U && primary.size() == 3U &&
                primary[0U].destination_token ==
                    primary[0U].actor_token + 0x10U &&
                primary[0U].definition_argument == 100U &&
                primary[1U].definition_argument == 1U &&
                primary[2U].definition_argument == 0x73000001U &&
                fixture.port.calls_of(Call::reserved_query_primary_option_slot)
                    .empty(),
            "control panel publishes the selected primary value through the closed loader and preserves selector-two high bits"
        );
        test.expect_true(
            result.rows[4U].text_token ==
                    openswd3::battle::
                        kLegacyBattleControlPanelSharedTextToken &&
                result.final_text[0U] == 0xAFU &&
                result.final_text[1U] == 0x53U &&
                result.final_text[2U] == 0xAEU &&
                result.final_text[3U] == 0xEDU &&
                result.final_text[4U] == static_cast<u8>('2'),
            "control panel rewrites successful special rows as CP950 special plus one-based decimal"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_reset, {});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        write_word(fixture.group_b_actors[1U].resource_bytes, 0x66U, 1U);
        fixture.port.reply_definition({.definition = definition('P')});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::query_special_option, {.eax = 1U});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::query_special_option, {.eax = 1U});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_style, {});
        const auto result =
            openswd3::battle::draw_legacy_battle_control_panel_frame(
                fixture.bindings(), fixture.port, request(4U)
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        completed &&
                fixture.transition_value_a == 0U &&
                fixture.transition_value_b == 2U &&
                result.rows[3U].selected_index == 4U &&
                result.rows[3U].source_index == 1U && result.rows[3U].selected,
            "control panel selected second special row publishes zero primary and one-based special value"
        );
    }

    {
        Fixture fixture;
        fixture.selected_group_b_index = 0xFFFFU;
        fixture.port.reply(Call::draw_text, {});
        fixture.port.reply(Call::configure_font_reset, {});
        fixture.port.reply(Call::configure_font_style, {});
        fixture.port.reply(Call::draw_text, {});
        const auto result =
            openswd3::battle::draw_legacy_battle_control_panel_frame(
                fixture.bindings(), fixture.port, request(2U)
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        group_b_actor_typed_stop &&
                result.primary_query_calls == 1U &&
                result.text_draw_calls == 2U && result.return_eax == 0U &&
                result.return_ecx == 0x005229E0U && result.return_edx == 0U,
            "control panel sign-extends selected group-B index and stops at the callee's first actor-resource access"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.fail = true;
        const auto result =
            openswd3::battle::draw_legacy_battle_control_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        title_border_typed_stop &&
                result.border_calls == 1U && result.text_draw_calls == 0U,
            "control panel first border stop blocks the title and all later rows"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(Call::draw_text, {});
        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::draw_text) {
                fixture.frame_provider.fail = true;
            }
        };
        const auto result =
            openswd3::battle::draw_legacy_battle_control_panel_frame(
                fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleControlPanelFrameStatus::
                        body_border_typed_stop &&
                result.border_calls == 2U && result.text_draw_calls == 1U &&
                result.font_reset_calls == 0U,
            "control panel second border stop preserves the control-title prefix"
        );
    }
}
