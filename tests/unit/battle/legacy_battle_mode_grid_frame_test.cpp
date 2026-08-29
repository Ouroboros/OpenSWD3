#include "openswd3/battle/legacy_battle_mode_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

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
            const u16 color = static_cast<u16>(0x4200U + index);
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
    u32 failing_resource{0xFFFFFFFFU};
};

class GridPort final : public openswd3::battle::LegacyBattleGridFramePort {
public:
    void reply(const Call call, const Reply& value) {
        replies[static_cast<std::size_t>(call)].push_back(value);
    }

    [[nodiscard]] Reply invoke_grid_frame(const Request& request) override {
        calls.push_back(request);
        if (on_call) {
            on_call(request);
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
    std::array<std::vector<Reply>, 13> replies;
    std::array<std::size_t, 13> reply_offsets{};
    std::function<void(const Request&)> on_call;
};

[[nodiscard]] std::array<u8, 20> text(const std::string_view source) {
    std::array<u8, 20> result{};
    std::ranges::transform(source, result.begin(), [](const char value) {
        return static_cast<u8>(value);
    });
    return result;
}

[[nodiscard]] bool starts_with(
    const std::array<u8, 20>& value, const std::span<const u8> expected
) {
    return std::ranges::equal(
        value | std::views::take(expected.size()), expected
    );
}

struct Fixture {
    Fixture()
        : action_updater(action_streams), raster(framebuffer.geometry()) {}

    [[nodiscard]] openswd3::battle::LegacyBattleModeGridFrameBindings
    bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .target_argument = target_argument,
            .primary_text_color = primary_text_color,
            .party = party,
            .scripted_port_test_compat = true,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleModeGridFrameState state;
    u32 queued_actor_code{8U};
    u16 panel_row_limit{0xFFFFU};
    u32 selection_input_gate{};
    u32 target_argument{};
    u16 primary_text_color{0x2222U};
    std::array<openswd3::battle::LegacyBattlePartyStartupRecord, 4> party{};
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

[[nodiscard]] openswd3::battle::LegacyBattleModeGridFrameRequest request() {
    openswd3::battle::LegacyBattleModeGridFrameRequest value{
        .origin_x = 224U,
        .origin_y = 126U,
        .selected_cell = 3U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .row_text_token = 0x0012FFCCU,
        .primary_count_token = 0x0012FFD0U,
        .secondary_count_token = 0x0012FFD4U,
        .panel_rectangle_return_registers =
            {
                .eax = 0xAAAA1234U,
                .ecx = 0x44000002U,
                .edx = 0x44000003U,
            },
        .selection_rectangle_return_registers = {
            .eax = 0xCCCC1234U,
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
    return value;
}

void prepare_success(Fixture& fixture) {
    fixture.port.reply(
        Call::query_mode_row,
        {
            .eax = 0x101U,
            .ecx = 0x102U,
            .edx = 0x103U,
            .publish_row_value = true,
            .row_value = 2U,
            .publish_row_text = true,
            .row_text = text("PRI"),
        }
    );
    fixture.port.reply(
        Call::refresh_actor, {.eax = 0x201U, .ecx = 0x202U, .edx = 0x203U}
    );
    fixture.port.reply(
        Call::query_mode_secondary_count,
        {
            .eax = 0x301U,
            .ecx = 0x302U,
            .edx = 0xDEADBEEFU,
            .publish_row_value = true,
            .row_value = 4U,
        }
    );
    fixture.port.reply(
        Call::refresh_actor, {.eax = 0x401U, .ecx = 0x402U, .edx = 0x403U}
    );
    fixture.port.reply(
        Call::query_mode_row,
        {
            .eax = 1U,
            .ecx = 0x501U,
            .edx = 0x502U,
            .publish_row_value = true,
            .row_value = 2U,
            .publish_row_text = true,
            .row_text = text("S1"),
        }
    );
    fixture.port.reply(
        Call::query_mode_row,
        {
            .eax = 1U,
            .ecx = 0x601U,
            .edx = 0x602U,
            .publish_row_value = true,
            .row_value = 2U,
            .publish_row_text = true,
            .row_text = text("S2"),
        }
    );
    fixture.port.reply(
        Call::query_mode_row,
        {
            .eax = 1U,
            .ecx = 0x701U,
            .edx = 0x702U,
            .publish_row_value = true,
            .row_value = 3U,
            .publish_row_text = true,
            .row_text = text("NEXT"),
        }
    );
    fixture.port.reply(
        Call::query_mode_row,
        {
            .eax = 1U,
            .ecx = 0x801U,
            .edx = 0x802U,
            .publish_row_value = true,
            .row_value = 3U,
            .publish_row_text = true,
            .row_text = text("LASTTAIL"),
        }
    );
}

}  // namespace

void test_battle_mode_grid_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.queued_actor_code = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x33333333U && result.port_calls == 0U &&
                fixture.panel_row_limit == 0xFFFFU &&
                fixture.state.row_text[0U] == 0xFFU,
            "mode grid queued-zero exit clears EAX and ECX while preserving entry EDX"
        );
    }

    {
        Fixture fixture;
        prepare_success(fixture);
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        const auto mode_rows = fixture.port.calls_of(Call::query_mode_row);
        const auto refreshes = fixture.port.calls_of(Call::refresh_actor);
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.font_calls == 15U &&
                result.primary_query_calls == 1U &&
                result.secondary_count_query_calls == 1U &&
                result.secondary_row_query_calls == 4U &&
                result.actor_refresh_calls == 2U &&
                result.text_copy_calls == 4U && result.text_draw_calls == 12U &&
                result.selection_rectangle_calls == 1U &&
                fixture.panel_row_limit == 6U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 2U && result.selected_page == 2U &&
                result.selected_group_index == 1U,
            "mode grid completes its panel, ten cells, selected overlay and total-count publication"
        );
        test.expect_true(
            mode_rows.size() == 5U && mode_rows[0U].arguments[0U] == 0U &&
                mode_rows[0U].arguments[1U] == 1U &&
                mode_rows[1U].arguments[0U] == 1U &&
                mode_rows[1U].arguments[1U] == 1U &&
                mode_rows[2U].arguments[1U] == 1U &&
                mode_rows[3U].arguments[1U] == 2U &&
                mode_rows[4U].arguments[1U] == 2U &&
                mode_rows[1U].edx == 0x0012FFCCU && refreshes.size() == 2U &&
                refreshes[0U].edx == 0U && refreshes[1U].edx == 0xDEADBEEFU,
            "mode grid preserves primary, secondary-page and asymmetric refresh register shapes"
        );
        test.expect_true(
            result.cells[0U].x == 240U && result.cells[0U].y == 170U &&
                result.cells[4U].x == 240U && result.cells[4U].y == 250U &&
                result.cells[5U].x == 352U && result.cells[5U].y == 170U &&
                result.cells[9U].x == 352U && result.cells[9U].y == 250U &&
                result.cells[2U].queried_secondary &&
                result.cells[2U].selected &&
                result.cells[2U].group_index == 1U &&
                result.cells[3U].page == 2U && result.cells[6U].missing &&
                result.cells[9U].missing,
            "mode grid preserves two-column five-row geometry and page/group advancement"
        );
        constexpr std::array<u8, 3> kMissing{0xB5U, 0x4CU, 0U};
        test.expect_true(
            starts_with(result.cells[6U].row_text, kMissing) &&
                result.cells[6U].row_text[3U] == static_cast<u8>('T') &&
                result.cells[9U].row_text[3U] == static_cast<u8>('T') &&
                draws.size() == 12U && draws[0U].arguments[1U] == 330U &&
                draws[0U].arguments[2U] == 134U &&
                draws[0U].text_token ==
                    openswd3::battle::kLegacyBattleStaticActionTextTokens[9U] &&
                draws[0U].arguments[4U] == 0xFFC0U &&
                draws[3U].arguments[4U] == 0xCCCC2222U &&
                draws[4U].arguments[4U] == 0x00002222U &&
                result.return_ecx == 4U,
            "mode grid preserves CP950 missing-copy tail, title token, selected color high word and final ECX low word"
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
                ),
            "mode grid preserves separate stale resource high words across both tiled bands"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.tiled_frame_calls == 2U &&
                result.text_copy_calls == 10U && fixture.panel_row_limit == 0U,
            "mode grid panel action and empty queries remain non-branching through ten missing cells"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        panel_rectangle_typed_stop &&
                result.panel_action_update_calls == 1U &&
                result.tiled_frame_calls == 0U &&
                result.return_eax == 0xAAAA1234U &&
                result.return_edx == 0x44000003U,
            "mode grid panel rectangle stop preserves the action and rectangle prefix"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xAAAA0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        first_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 1U && result.font_calls == 0U,
            "mode grid first tiled stop blocks title and actor access"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xBBBB0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        second_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 2U && result.font_calls == 0U,
            "mode grid second tiled stop preserves the completed top band"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 7U;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        group_a_actor_typed_stop &&
                result.text_draw_calls == 1U && result.font_calls == 3U &&
                result.primary_query_calls == 0U &&
                fixture.panel_row_limit == 0xFFFFU &&
                result.return_eax == 0xFFFFFC11U &&
                result.return_ecx == 0x004FFA9CU &&
                result.return_edx == 0xFFFFF433U,
            "mode grid invalid group-A code stops at the first primary query after the full panel prefix"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            Call::query_mode_row,
            {
                .publish_row_value = true,
                .row_value = 1U,
                .publish_row_text = true,
                .row_text = text("ONE"),
            }
        );
        fixture.port.reply(Call::refresh_actor, {});
        fixture.port.reply(
            Call::query_mode_secondary_count,
            {.publish_row_value = true, .row_value = 0U}
        );
        fixture.port.reply(Call::refresh_actor, {});
        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::query_mode_secondary_count) {
                fixture.raster.surface.pitch_bytes = 600;
            }
        };
        auto selected = request();
        selected.selected_cell = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, selected
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        selection_rectangle_typed_stop &&
                result.selection_rectangle_calls == 1U &&
                fixture.selection_input_gate == 0U &&
                fixture.target_argument == 1U &&
                result.return_eax == 0xCCCC1234U &&
                result.return_ecx == 0x77000002U &&
                result.return_edx == 0x77000003U,
            "mode grid selection rectangle stop preserves selected cell prefix but blocks gate and target rewrite"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            Call::query_mode_row,
            {
                .publish_row_value = true,
                .row_value = 0U,
                .publish_row_text = true,
                .row_text = text("EMPTY"),
            }
        );
        fixture.port.reply(Call::refresh_actor, {});
        fixture.port.reply(
            Call::query_mode_secondary_count,
            {.publish_row_value = true, .row_value = 1U}
        );
        fixture.port.reply(Call::refresh_actor, {});
        fixture.port.reply(
            Call::query_mode_row,
            {
                .publish_row_value = true,
                .row_value = 1U,
                .publish_row_text = true,
                .row_text = text("SECOND"),
            }
        );
        auto selected = request();
        selected.selected_cell = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, selected
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.cells[0U].group_index == 0U &&
                result.cells[0U].page == 2U && fixture.target_argument == 1U &&
                fixture.panel_row_limit == 1U,
            "mode grid selected target applies page advance before the zero-primary decrement"
        );
    }

    {
        Fixture fixture;
        auto bindings = fixture.bindings();
        bindings.scripted_port_test_compat = false;
        auto& actor_list = fixture.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x78000000U;
        actor_list.resources = {
            {.token = 0x78000000U, .next_token = 0x78000010U, .name = {}},
            {.token = 0x78000010U,
             .next_token = 0x78000020U,
             .resource_id = 0x0300U,
             .secondary_quantity = 2,
             .name = "PRIMARY",
             .mode_flags = 0x01U},
            {.token = 0x78000020U,
             .resource_id = 0x0222U,
             .secondary_quantity = 2,
             .name = "GROUP",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
        };
        fixture.port.reply(
            Call::query_mode_secondary_count,
            {.edx = 0xDEADBEEFU, .publish_row_value = true, .row_value = 2U}
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, bindings, fixture.port, request()
            );
        const auto primary_queries =
            fixture.port.calls_of(Call::query_mode_row);
        const auto refreshes = fixture.port.calls_of(Call::refresh_actor);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.primary_query_calls == 1U &&
                result.primary_query.outputs_published &&
                result.primary_query.copied_name == "PRIMARY" &&
                result.secondary_row_query_calls == 2U &&
                result.secondary_row_query.outputs_published &&
                result.secondary_row_query.copied_name == "GROUP" &&
                result.actor_refresh_calls == 2U &&
                result.actor_refreshes[0U].head_writes == 1U &&
                result.actor_refreshes[1U].head_writes == 1U &&
                fixture.panel_row_limit == 4U && primary_queries.empty() &&
                refreshes.empty(),
            "mode grid production queries and resource-head refreshes use the startup party typed owner"
        );
        const auto primary_text = text("PRIMARY");
        const auto group_text = text("GROUP");
        test.expect_true(
            starts_with(result.cells[0U].row_text, primary_text) &&
                starts_with(result.cells[1U].row_text, primary_text) &&
                starts_with(result.cells[2U].row_text, group_text) &&
                starts_with(result.cells[3U].row_text, group_text) &&
                actor_list.selected_resource_token == 0x78000020U,
            "mode grid production preserves primary rows and repeats the typed group label for its returned slot count"
        );
    }
}
