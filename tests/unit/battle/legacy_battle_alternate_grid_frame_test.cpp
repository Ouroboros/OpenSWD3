#include "openswd3/battle/legacy_battle_alternate_grid_frame.hpp"

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
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(const u32 action_id, const u32, const bool) override {
        action_ids.push_back(action_id);
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
    std::vector<u32> action_ids;
    u32 failing_action_id{0xFFFFFFFFU};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            const u16 color = static_cast<u16>(0x3200U + index);
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
        if (request.call == Call::query_alternate_row && on_query) {
            on_query(calls_of(Call::query_alternate_row).size() - 1U);
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
    std::array<std::vector<Reply>, 10> replies;
    std::array<std::size_t, 10> reply_offsets{};
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
        : action_updater(action_streams), raster(framebuffer.geometry()) {}

    [[nodiscard]] openswd3::battle::LegacyBattleAlternateGridFrameBindings
    bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .target_argument = target_argument,
            .primary_text_color = primary_text_color,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleAlternateGridFrameState state;
    u32 queued_actor_code{8U};
    u16 panel_row_limit{0xFFFFU};
    u32 selection_input_gate{};
    u32 target_argument{};
    u16 primary_text_color{0x2222U};
    openswd3::asset_runtime::LegacyActionRecord panel_action_record;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    GridPort port;
};

[[nodiscard]] openswd3::battle::LegacyBattleAlternateGridFrameRequest
request() {
    openswd3::battle::LegacyBattleAlternateGridFrameRequest value{
        .origin_x = 224U,
        .origin_y = 126U,
        .selected_row = 2U,
        .scroll_offset = 3U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .row_text_token = 0x0012FFCCU,
        .row_value_token = 0x0012FFD0U,
        .numeric_text_token = 0x0012FFE0U,
        .panel_rectangle_return_registers =
            {
                .eax = 0x44000001U,
                .ecx = 0x44000002U,
                .edx = 0xAAAA1234U,
            },
        .selection_rectangle_return_registers = {
            .eax = 0x77000001U,
            .ecx = 0x77000002U,
            .edx = 0x77000003U,
        },
    };
    value.tiled_frame_return_registers[0U] = {
        .eax = 0x55000001U,
        .ecx = 0x55000002U,
        .edx = 0xBBBB5678U,
    };
    value.tiled_frame_return_registers[1U] = {
        .eax = 0x66000001U,
        .ecx = 0x66000002U,
        .edx = 0x66000003U,
    };
    for (std::size_t index = 0U; index < value.format_return_registers.size();
         ++index) {
        value.format_return_registers[index] = {
            .eax = 0U,
            .ecx = 0xC0000000U + static_cast<u32>(index) * 0x10000U,
            .edx = 0xD0000000U + static_cast<u32>(index),
        };
    }
    return value;
}

void prepare_rows(Fixture& fixture, const std::size_t nonzero = 8U) {
    fixture.port.reply(
        Call::initialize_rows,
        {
            .eax = 0x101U,
            .ecx = 0x102U,
            .edx = 0x103U,
            .publish_panel_row_limit = true,
            .panel_row_limit = 9U,
        }
    );
    fixture.port.reply(
        Call::refresh_actor, {.eax = 0x201U, .ecx = 0x202U, .edx = 0x203U}
    );
    for (std::size_t index = 0U; index < nonzero; ++index) {
        fixture.port.reply(
            Call::query_alternate_row,
            {
                .eax = 1U,
                .ecx = 0xA0000000U + static_cast<u32>(index) * 0x10000U,
                .edx = 0xB0000000U + static_cast<u32>(index),
                .publish_row_value = true,
                .row_value =
                    index == 1U ? 0xFFFFFFFDU : static_cast<u32>(index + 5U),
                .publish_row_text = true,
                .row_text =
                    text(std::string(1U, static_cast<char>('A' + index))),
            }
        );
    }
}

[[nodiscard]] bool
same_text(const std::array<u8, 20>& value, const std::string_view expected) {
    return std::ranges::equal(
        value | std::views::take(expected.size()), expected
    );
}

}  // namespace

void test_battle_alternate_grid_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.queued_actor_code = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0U && result.port_calls == 0U &&
                result.panel_action_update_calls == 0U &&
                fixture.panel_row_limit == 0xFFFFU &&
                fixture.state.row_text[0U] == 0xFFU,
            "alternate grid queued-zero exit clears registers and stack locals without reading arguments"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        const auto queries = fixture.port.calls_of(Call::query_alternate_row);
        const auto draws = fixture.port.calls_of(Call::draw_text);
        const auto fonts = fixture.port.calls_of(Call::configure_font_style);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.font_calls == 12U &&
                result.actor_initialization_calls == 1U &&
                result.actor_refresh_calls == 1U &&
                result.row_query_calls == 8U && result.scanned_rows == 8U &&
                result.displayed_rows == 7U && result.text_draw_calls == 16U &&
                result.selection_rectangle_calls == 1U,
            "alternate grid completes its panel, actor and seven-row call counts"
        );
        test.expect_true(
            result.final_iterator == 11U && result.selected_iterator == 5U &&
                fixture.panel_row_limit == 9U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 5U && queries.size() == 8U &&
                queries.front().arguments[0U] == 0U &&
                queries.front().arguments[1U] == 4U &&
                queries.back().arguments[1U] == 11U &&
                queries.front().eax == 0U &&
                queries.front().ecx == 0x005029D0U && queries.front().edx == 0U,
            "alternate grid performs the eighth query and publishes the selected iterator"
        );
        test.expect_true(
            draws.size() == 16U && draws[0U].arguments[1U] == 304U &&
                draws[0U].arguments[2U] == 134U &&
                draws[0U].text_token == 0x004A76A0U &&
                draws[0U].arguments[4U] == 0xFFC0U &&
                draws[1U].arguments[1U] == 240U &&
                draws[1U].arguments[2U] == 170U &&
                draws[1U].arguments[4U] == 0xA0002222U &&
                draws[2U].arguments[1U] == 368U &&
                draws[2U].arguments[4U] == 0xC0002222U &&
                same_text(draws[2U].text_bytes, " 5") &&
                draws[5U].arguments[1U] == 239U &&
                draws[5U].arguments[2U] == 189U &&
                same_text(draws[5U].text_bytes, "B") &&
                same_text(result.rows[1U].numeric_text, "-3") &&
                result.rows[1U].selected && result.rows[6U].iterator == 10U &&
                fonts.size() == 10U,
            "alternate grid preserves title, name, numeric and selected redraw geometry"
        );
        test.expect_true(
            fixture.panel_action_record.action_id == 0x233BU &&
                fixture.panel_action_record.base_variant == 0U,
            "alternate grid uses the shared panel action record"
        );
        test.expect_true(
            !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xAAAA0077U &&
                fixture.frame_provider.resource_ids.back() == 0xBBBB0077U &&
                std::ranges::all_of(
                    fixture.frame_provider.resource_ids,
                    [](const u32 resource) {
                        return resource == 0xAAAA0077U ||
                            resource == 0xBBBB0077U;
                    }
                ) &&
                std::ranges::count(
                    fixture.frame_provider.resource_ids, 0xAAAA0077U
                ) > 0 &&
                std::ranges::count(
                    fixture.frame_provider.resource_ids, 0xBBBB0077U
                ) > 0,
            "alternate grid preserves separate stale resource high words across both tiled bands"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                std::ranges::all_of(
                    fixture.frame_provider.resource_ids,
                    [](const u32 resource) {
                        return resource == 0xAAAA0000U ||
                            resource == 0xBBBB0000U;
                    }
                ),
            "alternate grid panel action failure remains non-branching and clears both resource low words"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        panel_rectangle_typed_stop &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 0U &&
                result.return_eax == 0x44000001U &&
                result.return_ecx == 0x44000002U &&
                result.return_edx == 0xAAAA1234U,
            "alternate grid invalid panel geometry preserves the action and rectangle prefix"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xAAAA0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        first_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 1U &&
                result.actor_initialization_calls == 0U &&
                result.return_eax == 0x55000001U &&
                result.return_edx == 0xBBBB5678U,
            "alternate grid first tiled stop blocks title and actor access"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xBBBB0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        second_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 2U &&
                result.actor_initialization_calls == 0U &&
                result.return_eax == 0x66000001U &&
                result.return_edx == 0x66000003U,
            "alternate grid second tiled stop preserves the completed top band"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 7U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        const auto title_calls = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        group_a_actor_typed_stop &&
                result.tiled_frame_calls == 2U && result.font_calls == 3U &&
                result.actor_initialization_calls == 0U &&
                fixture.panel_row_limit == 0U && title_calls.size() == 1U &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 7U,
            "alternate grid invalid one-before group-A code stops only at list initialization"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture, 2U);
        fixture.port.reply(
            Call::query_alternate_row, {.eax = 0U, .ecx = 0x301U, .edx = 0x302U}
        );
        fixture.port.reply(
            Call::refresh_actor, {.eax = 0x401U, .ecx = 0x402U, .edx = 0x403U}
        );
        auto zero_request = request();
        zero_request.selected_row = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, zero_request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.row_query_calls == 3U && result.scanned_rows == 2U &&
                result.displayed_rows == 2U &&
                result.actor_refresh_calls == 2U &&
                result.final_iterator == 6U && result.font_calls == 6U,
            "alternate grid zero query refreshes the actor and enters the final font tail"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        fixture.port.on_query = [&fixture](const std::size_t index) {
            if (index == 1U) {
                fixture.raster.surface.pitch_bytes = 800;
            }
        };
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        selection_rectangle_typed_stop &&
                result.row_query_calls == 2U && result.displayed_rows == 1U &&
                result.selection_rectangle_calls == 1U &&
                fixture.selection_input_gate == 0U &&
                fixture.target_argument == 0U &&
                result.return_eax == 0x77000001U &&
                result.return_ecx == 0x77000002U &&
                result.return_edx == 0x77000003U,
            "alternate grid selection rectangle stop preserves selected text but blocks iterator publication"
        );
    }
}
