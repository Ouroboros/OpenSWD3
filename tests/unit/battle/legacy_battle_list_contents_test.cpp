#include "openswd3/battle/legacy_battle_list_contents.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <ranges>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleListContentsCall;
using openswd3::battle::LegacyBattleListContentsCallReply;
using openswd3::battle::LegacyBattleListContentsCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final : public openswd3::battle::LegacyBattleListContentsPort {
public:
    [[nodiscard]] LegacyBattleListContentsCallReply invoke_list_contents(
        const LegacyBattleListContentsCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return default_reply;
        }
        return found->second[index++];
    }

    void reply(
        const LegacyBattleListContentsCall call,
        const LegacyBattleListContentsCallReply value
    ) {
        replies[call].push_back(value);
    }

    LegacyBattleListContentsCallReply default_reply{};
    std::vector<LegacyBattleListContentsCallRequest> calls;
    std::map<
        LegacyBattleListContentsCall,
        std::vector<LegacyBattleListContentsCallReply>>
        replies;
    std::map<LegacyBattleListContentsCall, std::size_t> indices;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resources.push_back(resource_id);
        frames.push_back(piece_index);
        if (resource_id != 0x241CU || piece_index != 1U) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                    .palette = {},
                },
            .width = 1,
            .height = 1,
        };
        return true;
    }

    std::array<u8, 2> storage{0x34U, 0x12U};
    std::vector<u32> resources;
    std::vector<u32> frames;
};

struct Fixture {
    explicit Fixture(const i32 framebuffer_height = 480)
        : framebuffer({
              .pitch_bytes = 1280,
              .width = 640,
              .height = framebuffer_height,
          }),
          maps_payload(0x70U, 0U) {
        maps_payload[0x4CU] = 0x50U;
        maps_payload[0x54U] = 0x58U;
        constexpr std::array<u8, 9> kDescription{
            'A', 'B', 'C', 'D', 'E', 'F', 'G', '%', 'Q'
        };
        std::ranges::copy(kDescription, maps_payload.begin() + 0x58);
        raster.surface = {
            .pitch_bytes = 1280,
            .width = 640,
            .height = 480,
        };
        raster.clip_width = 640;
        raster.clip_height = 480;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleListContentsBindings
    bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .action_category_index = action_category_index,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .candidate_argument = candidate_argument,
            .primary_text_color = primary_text_color,
            .secondary_text_color = secondary_text_color,
            .actor_description_record_tokens = actor_description_record_tokens,
            .actor_description_text_indices = actor_description_text_indices,
            .maps_payload = maps_payload,
            .shared_text = shared_text,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = shared_request,
            .shared_effects = shared_effects,
            .jitter = jitter,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleListContentsState state;
    u32 queued_actor_code{8U};
    u32 action_category_index{3U};
    u8 panel_row_limit{0xEEU};
    u32 selection_input_gate{};
    u32 candidate_argument{};
    u16 primary_text_color{0x1111U};
    u16 secondary_text_color{0x2222U};
    std::array<u32, 10> actor_description_record_tokens{};
    std::array<u16, 10> actor_description_text_indices{};
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyBlitClipRectangle clip{
        .left = 0,
        .top = 0,
        .width = 640,
        .height = 480,
    };
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState shared_effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    std::vector<u8> maps_payload;
    std::array<u8, 128> shared_text{};
    FrameProvider frame_provider;
    Port port;
};

[[nodiscard]] std::size_t
count_call(const Port& port, const LegacyBattleListContentsCall call) {
    return static_cast<std::size_t>(std::ranges::count_if(
        port.calls, [call](const auto& request) { return request.call == call; }
    ));
}

[[nodiscard]] std::vector<LegacyBattleListContentsCallRequest>
calls_of(const Port& port, const LegacyBattleListContentsCall call) {
    std::vector<LegacyBattleListContentsCallRequest> result;
    std::ranges::copy_if(
        port.calls, std::back_inserter(result), [call](const auto& request) {
            return request.call == call;
        }
    );
    return result;
}

[[nodiscard]] openswd3::battle::LegacyBattleListContentsRequest request() {
    openswd3::battle::LegacyBattleListContentsRequest value{
        .origin_x = 232U,
        .origin_y = 134U,
        .selected_row = 1U,
        .scroll_offset = 10U,
        .entry_eax = 0x01020304U,
        .entry_ecx = 0x11121314U,
        .entry_edx = 0x21222324U,
        .local_value_token = 0x70001000U,
        .local_limit_word_token = 0x70001004U,
        .local_limit_byte_token = 0x70001006U,
        .numeric_text_token = 0x70001008U,
        .initial_limit_word = 0xABCDU,
        .initial_limit_byte = 0xEFU,
    };
    value.resource_frame_return_registers[0U] = {
        .eax = 0x11223344U,
        .ecx = 0x55667788U,
        .edx = 0xAABBCCDDU,
    };
    value.format_return_registers[0U] = {
        .ecx = 0xCAFE0000U,
        .edx = 0xBEEF0000U,
    };
    value.format_return_registers[1U] = {
        .ecx = 0xFACE0000U,
        .edx = 0xDEAD0000U,
    };
    return value;
}

}  // namespace

void test_battle_list_contents(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.actor_description_record_tokens[0U] = 0xDEADC0DEU;
        fixture.actor_description_text_indices[0U] = 1U;
        fixture.port.reply(
            LegacyBattleListContentsCall::configure_font_style,
            {.eax = 0x10000001U, .ecx = 0x10000002U, .edx = 0x10000003U}
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::configure_font_style,
            {.eax = 0x20000001U, .ecx = 0x20000002U, .edx = 0x20000003U}
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::configure_font_style,
            {.eax = 0xAABBCCDDU, .ecx = 0x11223344U, .edx = 0x55667788U}
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::initialize_rows,
            {
                .publish_panel_row_limit = true,
                .panel_row_limit = 0xFEU,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 2U,
                .publish_row_value = true,
                .row_value = 0x8005U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 0xFFFFU,
                .publish_row_value = true,
                .row_value = 10U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 0x12345678U,
                .publish_row_value = true,
                .row_value = 0xABCDFFFFU,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::resolve_negative_row,
            {
                .eax = 0xA1000000U,
                .ecx = 0xA2000000U,
                .edx = 0xA3000000U,
                .publish_limit_word = true,
                .limit_word = 7U,
                .publish_limit_byte = true,
                .limit_byte = 11U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::resolve_regular_row,
            {
                .eax = 0xCAFE1234U,
                .ecx = 0xB2000000U,
                .edx = 0xB3000000U,
                .publish_limit_word = true,
                .limit_word = 4U,
                .publish_limit_byte = true,
                .limit_byte = 12U,
            }
        );
        for (u32 index = 0U; index < 6U; ++index) {
            fixture.port.reply(
                LegacyBattleListContentsCall::draw_text,
                {
                    .eax = 0xD0000000U + index,
                    .ecx = 0xE0000000U + index,
                    .edx = 0xF0000000U + index,
                }
            );
        }
        std::ranges::fill(
            fixture.framebuffer.physical_pixels(), static_cast<u16>(0U)
        );
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        const auto text_calls =
            calls_of(fixture.port, LegacyBattleListContentsCall::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListContentsStatus::
                        completed &&
                result.port_calls == 19U && result.completed_rows == 2U &&
                result.row_query_calls == 3U &&
                result.row_resolver_calls == 2U &&
                result.resource_frame_calls == 1U &&
                result.rectangle_calls == 2U && result.text_draw_calls == 6U &&
                result.actor_initialization_calls == 1U &&
                result.actor_refresh_calls == 2U &&
                result.shared_text_resolution_calls == 1U &&
                result.shared_text_length_calls == 1U &&
                fixture.panel_row_limit == 0xFEU &&
                fixture.selection_input_gate == 1U &&
                fixture.candidate_argument == 11U &&
                result.selected_iterator == 11U &&
                result.final_iterator == 13U &&
                result.rows[0U].raw_value == 0x8005U &&
                result.rows[0U].displayed_value == 5U &&
                result.rows[0U].negative_selector &&
                result.rows[0U].resource_drawn &&
                !result.rows[0U].limit_is_less && result.rows[0U].selected &&
                result.rows[1U].displayed_value == 10U &&
                result.rows[1U].limit_is_less && !result.rows[1U].selected &&
                result.return_eax == 0xAABBCCDDU &&
                result.return_ecx == 0x11223344U &&
                result.return_edx == 0x55667788U,
            "list contents preserves row queries, signed value branches, selected panel ordering, and final registers"
        );
        test.expect_true(
            fixture.frame_provider.resources == std::vector<u32>{0x241CU} &&
                fixture.frame_provider.frames == std::vector<u32>{1U} &&
                text_calls.size() == 6U &&
                text_calls[0U].arguments[1U] == 244U &&
                text_calls[0U].arguments[2U] == 171U &&
                text_calls[0U].arguments[4U] == 0xAABB1111U &&
                text_calls[0U].eax == 244U &&
                text_calls[0U].edx == 0xAABB1111U &&
                text_calls[1U].arguments[1U] == 376U &&
                text_calls[1U].text_length == 3U &&
                text_calls[1U].text_bytes[0U] == ' ' &&
                text_calls[1U].text_bytes[1U] == ' ' &&
                text_calls[1U].text_bytes[2U] == '5' &&
                text_calls[2U].arguments[1U] == 243U &&
                text_calls[2U].arguments[2U] == 170U &&
                text_calls[3U].arguments[1U] == 296U &&
                text_calls[3U].arguments[2U] == 360U &&
                text_calls[4U].arguments[4U] == 0xCAFE2222U &&
                text_calls[4U].eax == 0xCAFE2222U &&
                text_calls[4U].edx == 0x004CD76CU &&
                text_calls[5U].text_bytes[0U] == ' ' &&
                text_calls[5U].text_bytes[1U] == '1' &&
                text_calls[5U].text_bytes[2U] == '0' &&
                std::ranges::equal(
                    fixture.shared_text | std::views::take(7),
                    std::array<u8, 7>{'A', 'B', 'C', 'D', 'E', 'F', 'G'}
                ) &&
                fixture.shared_text[7U] == 0U &&
                fixture.framebuffer.row_pixels(200U)[300U] == 0U &&
                fixture.framebuffer.row_pixels(360U)[300U] != 0U,
            "list contents preserves geometry, colors, numeric formatting, actor description, and rectangle coverage"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 7U;
        fixture.action_category_index = 0x55667788U;
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListContentsStatus::
                        group_a_actor_typed_stop &&
                result.port_calls == 3U && fixture.panel_row_limit == 0U &&
                result.actor_initialization_calls == 0U &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU &&
                result.return_edx == 0x55667788U,
            "invalid group-A code stops at the first actor access after font and row-limit prefixes"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            LegacyBattleListContentsCall::initialize_rows,
            {
                .publish_panel_row_limit = true,
                .panel_row_limit = 9U,
            }
        );
        for (u32 value = 1U; value <= 7U; ++value) {
            fixture.port.reply(
                LegacyBattleListContentsCall::query_row,
                {
                    .eax = 0xFFFFU,
                    .publish_row_value = true,
                    .row_value = value,
                }
            );
            fixture.port.reply(
                LegacyBattleListContentsCall::resolve_regular_row,
                {
                    .publish_limit_word = true,
                    .limit_word = 10U,
                }
            );
        }
        auto call = request();
        call.selected_row = 0U;
        call.scroll_offset = 0U;
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListContentsStatus::
                        completed &&
                result.completed_rows == 7U && result.row_query_calls == 7U &&
                result.row_resolver_calls == 7U &&
                result.resource_frame_calls == 0U &&
                result.text_draw_calls == 14U &&
                result.actor_refresh_calls == 1U &&
                result.final_iterator == 8U && fixture.panel_row_limit == 9U &&
                count_call(
                    fixture.port, LegacyBattleListContentsCall::query_row
                ) == 7U,
            "list contents stops after the original seven low-word row iterations without an eighth query"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 0U,
                .publish_row_value = true,
                .row_value = 1U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::resolve_regular_row,
            {
                .publish_limit_word = true,
                .limit_word = 2U,
            }
        );
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListContentsStatus::
                        resource_frame_typed_stop &&
                result.completed_rows == 0U &&
                result.resource_frame_calls == 1U &&
                result.text_draw_calls == 0U && result.rectangle_calls == 0U &&
                fixture.selection_input_gate == 0U &&
                fixture.candidate_argument == 0U,
            "missing selected resource frame stops before every row text and selected-panel side effect"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 0xFFFFU,
                .publish_row_value = true,
                .row_value = 1U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::resolve_regular_row,
            {
                .publish_limit_word = true,
                .limit_word = 2U,
            }
        );
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleListContentsStatus::
                    first_rectangle_typed_stop,
            "invalid raster geometry stops at the first selected-row rectangle"
        );
        test.expect_true(
            result.completed_rows == 0U && result.text_draw_calls == 3U &&
                result.rectangle_calls == 1U &&
                result.shared_text_resolution_calls == 0U &&
                fixture.selection_input_gate == 0U &&
                fixture.candidate_argument == 0U,
            "first selected-row rectangle stop preserves three completed text draws and blocks description publication"
        );
    }

    {
        Fixture fixture(360);
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 0xFFFFU,
                .publish_row_value = true,
                .row_value = 1U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::resolve_regular_row,
            {
                .publish_limit_word = true,
                .limit_word = 2U,
            }
        );
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                openswd3::battle::LegacyBattleListContentsStatus::
                    second_rectangle_typed_stop,
            "short physical framebuffer stops at the second selected-row rectangle"
        );
        test.expect_true(
            result.completed_rows == 0U && result.rectangle_calls == 2U &&
                result.shared_text_resolution_calls == 0U &&
                fixture.selection_input_gate == 0U &&
                fixture.candidate_argument == 0U,
            "second selected-row rectangle stop preserves the first rectangle and blocks shared description work"
        );
    }

    {
        Fixture fixture;
        fixture.maps_payload.clear();
        fixture.actor_description_record_tokens[0U] = 0xDEADC0DEU;
        fixture.actor_description_text_indices[0U] = 1U;
        fixture.port.reply(
            LegacyBattleListContentsCall::query_row,
            {
                .eax = 0xFFFFU,
                .publish_row_value = true,
                .row_value = 1U,
            }
        );
        fixture.port.reply(
            LegacyBattleListContentsCall::resolve_regular_row,
            {
                .publish_limit_word = true,
                .limit_word = 2U,
            }
        );
        const auto result = openswd3::battle::draw_legacy_battle_list_contents(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleListContentsStatus::
                        shared_text_typed_stop &&
                result.completed_rows == 0U && result.text_draw_calls == 3U &&
                result.rectangle_calls == 2U &&
                result.shared_text_resolution_calls == 1U &&
                result.shared_text_length_calls == 0U &&
                fixture.selection_input_gate == 0U &&
                fixture.candidate_argument == 0U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0xDEADC0DEU,
            "missing MAPS directory stops at the closed shared-text read after both selected rectangles"
        );
    }
}
