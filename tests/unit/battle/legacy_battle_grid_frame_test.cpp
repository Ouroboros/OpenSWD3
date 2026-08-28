#include "openswd3/battle/legacy_battle_grid_frame.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using Call = openswd3::battle::LegacyBattleGridFrameCall;
using Reply = openswd3::battle::LegacyBattleGridFrameCallReply;
using Request = openswd3::battle::LegacyBattleGridFrameCallRequest;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    ActionStreamProvider() {
        set_resource(0x0066U);
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool
    ) override {
        action_ids.push_back(action_id);
        variants.push_back(variant_index);
        if (action_id == failing_action_id) {
            return {};
        }
        set_resource(action_id == 0x233BU ? 0x0077U : 0x0066U);
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    void set_resource(const u16 resource) {
        constexpr std::array<u16, 8> kTemplate{
            0x5246U, 0U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        auto words = kTemplate;
        words[1U] = resource;
        bytes.clear();
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    std::vector<u8> bytes;
    std::vector<u32> action_ids;
    std::vector<u32> variants;
    u32 failing_action_id{0xFFFFFFFFU};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x1200U + index);
            storage[index] = {
                static_cast<u8>(color),
                static_cast<u8>(color >> 8U),
            };
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        piece_indices.push_back(piece_index);
        if (resource_id == failing_resource || piece_index >= storage.size()) {
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
    std::vector<u32> piece_indices;
    u32 failing_resource{0xFFFFFFFFU};
};

class GridPort final : public openswd3::battle::LegacyBattleGridFramePort {
public:
    void reply(const Call call, const Reply& value) {
        replies[static_cast<std::size_t>(call)].push_back(value);
    }

    [[nodiscard]] Reply invoke_grid_frame(const Request& request) override {
        calls.push_back(request);
        if (request.call == Call::query_row && on_query) {
            on_query(calls_of(Call::query_row).size() - 1U);
        }
        const std::size_t index = static_cast<std::size_t>(request.call);
        const std::size_t offset = reply_offsets[index]++;
        return offset < replies[index].size() ? replies[index][offset]
                                              : Reply{};
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
    std::array<std::vector<Reply>, 8> replies;
    std::array<std::size_t, 8> reply_offsets{};
    std::function<void(std::size_t)> on_query;
};

[[nodiscard]] std::array<u8, 20> text(const std::string_view source) {
    std::array<u8, 20> result{};
    std::ranges::transform(source, result.begin(), [](const char value) {
        return static_cast<u8>(value);
    });
    return result;
}

struct Fixture {
    Fixture()
        : maps_payload(0x70U, 0U), action_updater(action_streams),
          raster(framebuffer.geometry()) {
        port.battle_offset_action_frame_draw_state().result_latch = 1U;
        maps_payload[0x4CU] = 0x50U;
        maps_payload[0x54U] = 0x58U;
        constexpr std::array<u8, 9> kDescription{
            'A', 'B', 'C', 'D', 'E', 'F', 'G', '%', 'Q'
        };
        std::ranges::copy(kDescription, maps_payload.begin() + 0x58);
        actor_description_record_tokens[0U] = 0xDEADC0DEU;
        actor_description_text_indices[0U] = 1U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGridFrameBindings bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .action_category_index = action_category_index,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .target_argument = target_argument,
            .primary_text_color = primary_text_color,
            .secondary_text_color = secondary_text_color,
            .actor_description_record_tokens = actor_description_record_tokens,
            .actor_description_text_indices = actor_description_text_indices,
            .maps_payload = maps_payload,
            .shared_text = shared_text,
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

    openswd3::battle::LegacyBattleGridFrameState state;
    u32 queued_actor_code{8U};
    u32 action_category_index{2U};
    u16 panel_row_limit{0xFFFFU};
    u32 selection_input_gate{};
    u32 target_argument{};
    u16 primary_text_color{0x2222U};
    u16 secondary_text_color{0x1111U};
    std::array<u32, 10> actor_description_record_tokens{};
    std::array<u16, 10> actor_description_text_indices{};
    std::vector<u8> maps_payload;
    std::array<u8, 128> shared_text{};
    openswd3::asset_runtime::LegacyActionRecord panel_action_record;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    GridPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleGridFrameRequest request() {
    openswd3::battle::LegacyBattleGridFrameRequest value{
        .origin_x = 224U,
        .origin_y = 126U,
        .selected_row = 2U,
        .scroll_offset = 3U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .row_text_token = 0x0012FFCCU,
        .row_flags_token = 0x0012FFD0U,
        .row_value_token = 0x0012FFD4U,
        .numeric_text_token = 0x0012FFE0U,
        .panel_rectangle_return_registers =
            {
                .eax = 0x44000001U,
                .ecx = 0xABCD1234U,
                .edx = 0x44000003U,
            },
        .tiled_frame_return_registers = {
            .eax = 0x55000001U,
            .ecx = 0x55000002U,
            .edx = 0x55000003U,
        },
    };
    for (std::size_t index = 0U;
         index < value.action_frame_return_registers.size();
         ++index) {
        value.action_update_edx_snapshots[index] = static_cast<u32>(index);
        value.action_frame_return_registers[index] = {
            .eax = 0xA0000000U + static_cast<u32>(index),
            .ecx = 0xB0000000U + static_cast<u32>(index),
            .edx = 0xC0000000U + static_cast<u32>(index),
        };
    }
    value.format_return_registers[0U] = {
        .ecx = 0xAAAA0000U, .edx = 0xAAAA0001U
    };
    value.format_return_registers[1U] = {
        .ecx = 0xBBBB0000U, .edx = 0xBBBB0001U
    };
    return value;
}

void prepare_rows(Fixture& fixture) {
    fixture.port.reply(
        Call::initialize_rows,
        {
            .eax = 0x10000001U,
            .ecx = 0x10000002U,
            .edx = 0x10000003U,
            .publish_panel_row_limit = true,
            .panel_row_limit = 9U,
        }
    );
    fixture.port.reply(
        Call::query_row,
        {
            .eax = 1U,
            .publish_row_flags = true,
            .row_flags = 0xC000U,
            .publish_row_value = true,
            .row_value = 5U,
            .publish_row_text = true,
            .row_text = text("Alpha"),
        }
    );
    fixture.port.reply(
        Call::query_row,
        {
            .eax = 1U,
            .publish_row_flags = true,
            .row_flags = 0U,
            .publish_row_value = true,
            .row_value = 99U,
            .publish_row_text = true,
            .row_text = text("Hidden"),
        }
    );
    fixture.port.reply(
        Call::query_row,
        {
            .eax = 1U,
            .publish_row_flags = true,
            .row_flags = 0x8000U,
            .publish_row_value = true,
            .row_value = 0xFFFFFFFDU,
            .publish_row_text = true,
            .row_text = text("Beta"),
        }
    );
    fixture.port.reply(Call::query_row, {});
    fixture.port.reply(Call::draw_text, {.edx = 0xA1000001U});
    fixture.port.reply(Call::draw_text, {.edx = 0xA2000002U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB1000003U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB2000004U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB3000005U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB4000006U});
}

}  // namespace

void test_battle_grid_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        prepare_rows(fixture);
        std::ranges::fill(
            fixture.framebuffer.physical_pixels(), static_cast<u16>(0xFFFFU)
        );
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        const std::array<u32, 6> expected_actions{
            0x2394U, 0x2394U, 0x2394U, 0x2394U, 0x2394U, 0x233BU
        };
        const std::array<u32, 6> expected_variants{3U, 2U, 1U, 0U, 6U, 0U};
        const auto text_calls = fixture.port.calls_of(Call::draw_text);
        const auto query_calls = fixture.port.calls_of(Call::query_row);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                result.action_frame_calls == 5U && result.font_calls == 9U &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                result.actor_initialization_calls == 1U &&
                result.actor_refresh_calls == 2U &&
                result.row_query_calls == 4U && result.scanned_rows == 3U &&
                result.hidden_rows == 1U && result.displayed_rows == 2U &&
                result.text_draw_calls == 6U &&
                result.selection_rectangle_calls == 2U &&
                result.shared_text_resolution_calls == 1U &&
                result.shared_text_length_calls == 1U &&
                result.final_iterator == 7U && result.selected_iterator == 6U,
            "grid frame completes five action tabs, filtered rows, selection panels, and description"
        );
        test.expect_true(
            fixture.action_streams.action_ids ==
                    std::vector<u32>(
                        expected_actions.begin(), expected_actions.end()
                    ) &&
                fixture.action_streams.variants ==
                    std::vector<u32>(
                        expected_variants.begin(), expected_variants.end()
                    ) &&
                result.action_frames[0U].draw_x == 348 &&
                result.action_frames[1U].draw_x == 306 &&
                result.action_frames[2U].draw_x == 264 &&
                result.action_frames[3U].draw_x == 222 &&
                result.action_frames[4U].draw_x == 306 &&
                result.action_frames[4U].draw_y == 155 &&
                fixture.panel_action_record.action_id == 0x233BU &&
                fixture.panel_action_record.base_variant == 0U &&
                fixture.panel_action_record.field_4a == 0x0077U &&
                std::ranges::all_of(
                    fixture.frame_provider.resource_ids | std::views::drop(5),
                    [](const u32 resource) { return resource == 0xABCD0077U; }
                ),
            "grid frame preserves action variants, positions, panel record, and stale tiled resource high word"
        );
        test.expect_true(
            fixture.panel_row_limit == 9U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 6U && query_calls.size() == 4U &&
                query_calls[0U].arguments[1U] == 4U &&
                query_calls[1U].arguments[1U] == 5U &&
                query_calls[2U].arguments[1U] == 6U &&
                query_calls[3U].arguments[1U] == 7U &&
                result.rows[0U].iterator == 4U &&
                result.rows[0U].secondary_color &&
                result.rows[1U].iterator == 6U &&
                !result.rows[1U].secondary_color &&
                result.rows[0U].numeric_text[0U] == ' ' &&
                result.rows[0U].numeric_text[1U] == '5' &&
                result.rows[1U].numeric_text[0U] == '-' &&
                result.rows[1U].numeric_text[1U] == '3',
            "grid frame skips hidden iterators while numbering and formatting only visible rows"
        );
        test.expect_true(
            text_calls.size() == 6U && text_calls[0U].arguments[1U] == 368U &&
                text_calls[0U].arguments[2U] == 170U &&
                text_calls[0U].arguments[4U] == 0xAAAA1111U &&
                text_calls[1U].arguments[1U] == 240U &&
                text_calls[1U].arguments[4U] == 0xA1001111U &&
                text_calls[2U].arguments[2U] == 190U &&
                text_calls[2U].arguments[4U] == 0xBBBB2222U &&
                text_calls[3U].text_bytes[0U] == 'B' &&
                text_calls[4U].arguments[1U] == 239U &&
                text_calls[4U].arguments[2U] == 189U &&
                text_calls[4U].arguments[4U] == 0xB2002222U &&
                text_calls[5U].arguments[1U] == 296U &&
                text_calls[5U].arguments[2U] == 360U &&
                std::ranges::equal(
                    fixture.shared_text | std::views::take(7),
                    std::array<u8, 7>{'A', 'B', 'C', 'D', 'E', 'F', 'G'}
                ) &&
                fixture.framebuffer.row_pixels(200U)[300U] != 0xFFFFU &&
                fixture.framebuffer.row_pixels(100U)[100U] == 0xFFFFU,
            "grid frame preserves text geometry, color high words, selected redraw, rectangles, and centered shared text"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 0U;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                result.port_calls == 0U && result.action_frame_calls == 0U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0U && fixture.panel_row_limit == 0xFFFFU,
            "zero queued actor returns zeroed registers before all panel work"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x2394U;
        auto call = request();
        call.action_frame_return_registers[0U] = {
            .eax = 0x11112222U,
            .ecx = 0x33334444U,
            .edx = 0x55556666U,
        };
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        action_frame_typed_stop &&
                result.action_frame_calls == 1U && result.port_calls == 0U &&
                result.panel_action_update_calls == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U,
            "first action tab stop preserves its register prefix and blocks the panel"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                result.panel_action_update.return_value == 0U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.panel_action_record.field_4a == 0U &&
                std::ranges::all_of(
                    fixture.frame_provider.resource_ids | std::views::drop(5),
                    [](const u32 resource) { return resource == 0xABCD0000U; }
                ),
            "panel action update failure remains non-branching and feeds the cleared resource low word"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        panel_rectangle_typed_stop &&
                result.action_frame_calls == 5U && result.font_calls == 2U &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 0U &&
                result.return_eax == 0x44000001U &&
                result.return_ecx == 0xABCD1234U &&
                result.return_edx == 0x44000003U,
            "invalid panel rectangle geometry preserves the header and record prefix before tiled drawing"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xABCD0077U;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        tiled_frame_typed_stop &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U && result.font_calls == 2U &&
                result.actor_initialization_calls == 0U &&
                result.return_eax == 0x55000001U &&
                result.return_ecx == 0x55000002U &&
                result.return_edx == 0x55000003U,
            "missing tiled resource preserves the completed panel rectangle and blocks actor access"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 7U;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        group_a_actor_typed_stop &&
                result.action_frame_calls == 5U && result.font_calls == 5U &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                result.actor_initialization_calls == 0U &&
                fixture.panel_row_limit == 0U,
            "invalid group-A code stops at row initialization after the complete panel prefix"
        );
    }

    {
        Fixture fixture;
        for (u32 index = 0U; index < 9U; ++index) {
            fixture.port.reply(
                Call::query_row,
                {
                    .eax = 1U,
                    .publish_row_flags = true,
                    .row_flags = 0U,
                }
            );
        }
        for (u32 index = 0U; index < 7U; ++index) {
            fixture.port.reply(
                Call::query_row,
                {
                    .eax = 1U,
                    .publish_row_flags = true,
                    .row_flags = 0x8000U,
                    .publish_row_value = true,
                    .row_value = index,
                    .publish_row_text = true,
                    .row_text = text("Visible"),
                }
            );
        }
        auto call = request();
        call.selected_row = 0U;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                result.row_query_calls == 16U && result.scanned_rows == 16U &&
                result.hidden_rows == 9U && result.displayed_rows == 7U &&
                result.final_iterator == 20U &&
                fixture.port.calls_of(Call::query_row).size() == 16U,
            "hidden rows do not consume the seven-row limit or impose a modern scan cap"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        fixture.port.on_query = [&fixture](const std::size_t index) {
            if (index == 2U) {
                fixture.raster.surface.pitch_bytes = 800;
            }
        };
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        first_selection_rectangle_typed_stop &&
                result.selection_rectangle_calls == 1U &&
                fixture.selection_input_gate == 0U &&
                fixture.target_argument == 0U &&
                result.shared_text_resolution_calls == 0U,
            "first selection rectangle stop preserves selected text but blocks input publication and description"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        fixture.raster.surface.pitch_bytes = 960;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        second_selection_rectangle_typed_stop &&
                result.selection_rectangle_calls == 2U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 6U &&
                result.shared_text_resolution_calls == 0U,
            "second selection rectangle stop preserves the first rectangle and pre-call input publication"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        fixture.maps_payload.clear();
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::
                        shared_text_typed_stop &&
                result.selection_rectangle_calls == 2U &&
                result.shared_text_resolution_calls == 1U &&
                result.shared_text_length_calls == 0U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 6U && result.displayed_rows == 1U,
            "missing MAPS directory stops after selected rectangles and input publication but before row completion"
        );
    }
}
