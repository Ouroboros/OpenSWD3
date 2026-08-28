#include "openswd3/battle/legacy_battle_narrow_grid_frame.hpp"

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
            const u16 color = static_cast<u16>(0x5100U + index);
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

[[nodiscard]] std::array<u8, 20>
workspace_text(const std::array<u32, 5>& workspace) {
    std::array<u8, 20> result{};
    for (std::size_t word_index = 0U; word_index < workspace.size();
         ++word_index) {
        for (std::size_t byte_index = 0U; byte_index < 4U; ++byte_index) {
            result[word_index * 4U + byte_index] = static_cast<u8>(
                workspace[word_index] >> static_cast<u32>(byte_index * 8U)
            );
        }
    }
    return result;
}

struct Fixture {
    Fixture()
        : action_updater(action_streams), raster(framebuffer.geometry()) {}

    [[nodiscard]] openswd3::battle::LegacyBattleNarrowGridFrameBindings
    bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .candidate_argument = candidate_argument,
            .primary_text_color = primary_text_color,
            .selection_workspace = selection_workspace,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleNarrowGridFrameState state;
    u32 queued_actor_code{8U};
    u8 panel_row_limit{0xAAU};
    u32 selection_input_gate{};
    u32 candidate_argument{9U};
    u16 primary_text_color{0x2222U};
    std::array<u32, 5> selection_workspace{
        0x04030201U, 0x08070605U, 0x0C0B0A09U, 0x100F0E0DU, 0x14131211U
    };
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

[[nodiscard]] openswd3::battle::LegacyBattleNarrowGridFrameRequest request() {
    openswd3::battle::LegacyBattleNarrowGridFrameRequest value{
        .origin_x = 224U,
        .origin_y = 126U,
        .selected_row = 2U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .row_text_token = 0x0053C184U,
        .row_value_token = 0x0012FFD0U,
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
    return value;
}

void prepare_success(Fixture& fixture) {
    fixture.port.reply(
        Call::initialize_narrow_rows,
        {
            .eax = 0x101U,
            .ecx = 0x102U,
            .edx = 0x103U,
            .publish_panel_row_limit = true,
            .panel_row_limit = 7U,
        }
    );
    fixture.port.reply(
        Call::refresh_actor, {.eax = 0x201U, .ecx = 0x202U, .edx = 0x203U}
    );
    fixture.port.reply(
        Call::query_narrow_row,
        {
            .eax = 0x301U,
            .ecx = 0x302U,
            .edx = 0x303U,
            .publish_row_value = true,
            .row_value = 0xDEAD0000U,
            .publish_row_text = true,
            .row_text = text("SKIP"),
        }
    );
    fixture.port.reply(
        Call::query_narrow_row,
        {
            .eax = 0x401U,
            .ecx = 0x402U,
            .edx = 0x403U,
            .publish_row_value = true,
            .row_value = 0xABCD0005U,
            .publish_row_text = true,
            .row_text = text("ROW1"),
        }
    );
    fixture.port.reply(
        Call::query_narrow_row,
        {
            .eax = 0x501U,
            .ecx = 0x502U,
            .edx = 0x503U,
            .publish_row_value = true,
            .row_value = 0xCAFE0006U,
            .publish_row_text = true,
            .row_text = text("ROW2"),
        }
    );
    fixture.port.reply(
        Call::query_narrow_row,
        {
            .eax = 0x601U,
            .ecx = 0x602U,
            .edx = 0x603U,
            .publish_row_value = true,
            .row_value = 0x1234FFFFU,
            .publish_row_text = true,
            .row_text = text("END"),
        }
    );
    fixture.port.reply(
        Call::refresh_actor, {.eax = 0x701U, .ecx = 0x702U, .edx = 0x703U}
    );
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(
        Call::draw_text, {.eax = 0x801U, .ecx = 0xBEEF7777U, .edx = 0x803U}
    );
    fixture.port.reply(Call::draw_text, {});
}

void prepare_sentinel(Fixture& fixture) {
    fixture.port.reply(Call::initialize_narrow_rows, {});
    fixture.port.reply(Call::refresh_actor, {});
    fixture.port.reply(
        Call::query_narrow_row,
        {.publish_row_value = true, .row_value = 0xFFFFU}
    );
    fixture.port.reply(Call::refresh_actor, {});
}

}  // namespace

void test_battle_narrow_grid_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.queued_actor_code = 0U;
        const auto before = fixture.selection_workspace;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U && result.port_calls == 0U &&
                fixture.panel_row_limit == 0xAAU &&
                fixture.selection_workspace == before,
            "narrow grid queued-zero exit preserves ECX, EDX and the shared selection workspace"
        );
    }

    {
        Fixture fixture;
        prepare_success(fixture);
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        const auto initializes =
            fixture.port.calls_of(Call::initialize_narrow_rows);
        const auto refreshes = fixture.port.calls_of(Call::refresh_actor);
        const auto queries = fixture.port.calls_of(Call::query_narrow_row);
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.font_calls == 5U &&
                result.actor_initialization_calls == 1U &&
                result.actor_refresh_calls == 2U &&
                result.row_query_calls == 4U && result.text_draw_calls == 4U &&
                result.selection_rectangle_calls == 1U &&
                result.displayed_rows == 2U && result.final_iterator == 4U &&
                fixture.panel_row_limit == 7U &&
                fixture.selection_input_gate == 1U &&
                fixture.candidate_argument == 3U,
            "narrow grid skips empty rows, draws two valid rows and publishes the selected source iterator"
        );
        test.expect_true(
            initializes.size() == 1U && initializes[0U].arguments[0U] == 0U &&
                initializes[0U].arguments[1U] == 1U &&
                initializes[0U].arguments[2U] == 0x0053BDF3U &&
                initializes[0U].eax == 0U && initializes[0U].edx == 8U &&
                refreshes.size() == 2U && refreshes[0U].eax == 0U &&
                refreshes[0U].edx == 0U && queries.size() == 4U &&
                queries[0U].arguments[0U] == 0U &&
                queries[0U].arguments[1U] == 1U &&
                queries[0U].arguments[2U] == 1U &&
                queries[0U].arguments[3U] == 0x0053C184U &&
                queries[0U].arguments[4U] == 0x0012FFD0U &&
                queries[0U].eax == 0U && queries[0U].edx == 8U,
            "narrow grid preserves distinct initialization, query and refresh actor register shapes"
        );
        test.expect_true(
            result.rows[0U].iterator == 2U &&
                result.rows[0U].row_value == 0xABCD0005U &&
                result.rows[0U].x == 240U && result.rows[0U].y == 166U &&
                result.rows[1U].iterator == 3U &&
                result.rows[1U].row_value == 0xCAFE0006U &&
                result.rows[1U].x == 240U && result.rows[1U].y == 188U &&
                result.rows[1U].selected,
            "narrow grid bases row geometry on displayed rows while preserving sparse source iterators"
        );
        test.expect_true(
            draws.size() == 4U && draws[0U].arguments[1U] == 304U &&
                draws[0U].arguments[2U] == 134U &&
                draws[0U].text_token ==
                    openswd3::battle::kLegacyBattleNarrowGridTitleToken &&
                draws[1U].arguments[4U] == 0xABCD2222U &&
                draws[1U].eax == 0x004CD76CU && draws[1U].ecx == 0x004C9A28U &&
                draws[1U].edx == 240U && draws[3U].arguments[1U] == 239U &&
                draws[3U].arguments[2U] == 187U &&
                draws[3U].arguments[4U] == 0xBEEF2222U &&
                draws[3U].eax == 239U && draws[3U].edx == 187U,
            "narrow grid preserves row-value and prior-text-call high words plus callsite registers"
        );
        test.expect_true(
            workspace_text(fixture.selection_workspace) == text("END") &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xAAAA0077U &&
                fixture.frame_provider.resource_ids.back() == 0xBBBB0077U,
            "narrow grid reuses the physical selection workspace and both stale resource high words"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        prepare_sentinel(fixture);
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.tiled_frame_calls == 2U && result.row_query_calls == 1U,
            "narrow grid panel action update failure remains non-branching through sentinel refresh"
        );
    }

    {
        Fixture fixture;
        fixture.raster.surface.pitch_bytes = 1;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        panel_rectangle_typed_stop &&
                result.panel_action_update_calls == 1U &&
                result.tiled_frame_calls == 0U &&
                result.return_eax == 0x44000001U &&
                result.return_edx == 0xAAAA1234U,
            "narrow grid panel rectangle stop preserves the action and rectangle prefix"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xAAAA0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        first_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 1U && result.font_calls == 0U,
            "narrow grid first tiled stop blocks title and actor access"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.failing_resource = 0xBBBB0077U;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        second_tiled_frame_typed_stop &&
                result.tiled_frame_calls == 2U && result.font_calls == 0U,
            "narrow grid second tiled stop preserves the completed top band"
        );
    }

    {
        Fixture fixture;
        fixture.queued_actor_code = 7U;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        group_a_actor_typed_stop &&
                result.text_draw_calls == 1U && result.font_calls == 3U &&
                result.actor_initialization_calls == 0U &&
                fixture.panel_row_limit == 0U &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 7U,
            "narrow grid invalid group-A code stops at initialization after clearing the shared byte"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(Call::initialize_narrow_rows, {});
        fixture.port.reply(Call::refresh_actor, {});
        for (u32 iterator = 1U; iterator <= 9U; ++iterator) {
            fixture.port.reply(
                Call::query_narrow_row,
                {
                    .publish_row_value = true,
                    .row_value = iterator <= 2U ? 0U : iterator,
                    .publish_row_text = true,
                    .row_text = text("ROW"),
                }
            );
        }
        auto no_selection = request();
        no_selection.selected_row = 9U;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, no_selection
            );
        const auto styles = fixture.port.calls_of(Call::configure_font_style);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.row_query_calls == 9U && result.displayed_rows == 7U &&
                result.final_iterator == 10U &&
                result.actor_refresh_calls == 1U &&
                result.rows[0U].iterator == 3U &&
                result.rows[6U].iterator == 9U && styles.size() == 2U &&
                styles.back().eax == 9U,
            "narrow grid excludes zero rows from the seven-row limit and keeps the unmatched selection in final EAX"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(Call::initialize_narrow_rows, {});
        fixture.port.reply(Call::refresh_actor, {});
        fixture.port.reply(
            Call::query_narrow_row,
            {
                .publish_row_value = true,
                .row_value = 1U,
                .publish_row_text = true,
                .row_text = text("SELECT"),
            }
        );
        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::query_narrow_row) {
                fixture.raster.surface.pitch_bytes = 600;
            }
        };
        auto selected = request();
        selected.selected_row = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, selected
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        selection_rectangle_typed_stop &&
                result.text_draw_calls == 3U &&
                result.selection_rectangle_calls == 1U &&
                result.displayed_rows == 0U &&
                fixture.selection_input_gate == 0U &&
                fixture.candidate_argument == 9U &&
                result.return_eax == 0x77000001U &&
                result.return_ecx == 0x77000002U &&
                result.return_edx == 0x77000003U,
            "narrow grid selection rectangle stop preserves both text draws but blocks count and publication"
        );
    }
}
