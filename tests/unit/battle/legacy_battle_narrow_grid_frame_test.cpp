#include "openswd3/battle/legacy_battle_narrow_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "legacy_battle_mon_database_fixture.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <ranges>
#include <span>
#include <string>
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

class GridPort final : public openswd3::battle::LegacyBattleGridFramePort,
                       public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleMonReadResult read_mon_file(
        const u32 handle, const std::span<u8> destination, const u32 size
    ) override {
        const auto result = LegacyBattleMonDatabaseFixture::read_mon_file(
            handle, destination, size
        );
        if (read_calls % 3U == 0U && on_profile_read) {
            on_profile_read();
        }

        return result;
    }

    std::function<void()> on_profile_read;

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
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        actors[0U].next_list_index = 0x10U;
        (*party)[0U].actor_list.owner_token = 0x10U;
    }

    void add_node(const std::string& name, const bool eligible = true) {
        auto& list = (*party)[0U].actor_list;
        const u32 token = 0x100U + static_cast<u32>(list.nodes.size());
        if (list.nodes.empty()) {
            list.head_token = token;
        } else {
            list.nodes.back().next_token = token;
        }

        list.nodes.push_back({
            .token = token,
            .category_flags = 0x10U,
            .mode_flags = static_cast<u8>(eligible ? 1U : 0U),
            .type = 0x1FU,
            .profile_id = 1U,
            .text = name,
        });
    }

    [[nodiscard]] openswd3::battle::LegacyBattleNarrowGridFrameBindings
    bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .candidate_argument = candidate_argument,
            .primary_text_color = primary_text_color,
            .selection_workspace = selection_workspace,
            .music_path = music_path,
            .panel_action_record = panel_action_record,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .party = *party,
            .actors = actors,
            .mon_database = port,
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
    openswd3::battle::LegacyBattleMusicPath music_path{};
    std::unique_ptr<
        std::array<openswd3::battle::LegacyBattlePartyStartupRecord, 10>>
        party{std::make_unique<std::array<
            openswd3::battle::LegacyBattlePartyStartupRecord,
            10>>()};
    std::array<openswd3::battle::LegacyBattleGroupAActionExecutionState, 10>
        actors{};
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
    fixture.add_node("SKIP", false);
    fixture.add_node("ROW1");
    fixture.add_node("ROW2");
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(Call::draw_text, {});
    fixture.port.reply(
        Call::draw_text, {.eax = 0x801U, .ecx = 0xBEEF7777U, .edx = 0x803U}
    );
    fixture.port.reply(Call::draw_text, {});
}

}  // namespace

void test_battle_narrow_grid_frame(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.add_node("ONE");
        (*fixture.party)[0U].actor_list.nodes.front().next_token = 0xDEADU;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        group_a_actor_typed_stop &&
                result.row_count_status ==
                    openswd3::battle::LegacyBattleActorListQueryStatus::
                        list_node_typed_stop &&
                fixture.panel_row_limit == 1U &&
                fixture.actors[0U].current_list_index == 0xDEADU &&
                result.font_calls == 3U && result.text_draw_calls == 1U &&
                fixture.port.requested_profile_ids.empty(),
            "narrow menu preserves the count failure prefix after its panel and title without loading rows"
        );
    }

    {
        Fixture fixture;
        fixture.add_node("12345678901234567890suffix");
        fixture.music_path.fill(0xCCU);
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.displayed_rows == 1U && fixture.music_path[0U] == 's' &&
                fixture.music_path[6U] == 0U &&
                fixture.music_path[7U] == 0xCCU && draws.size() >= 2U &&
                draws[1U].text_bytes[20U] == 's' &&
                draws[1U].text_bytes[26U] == 0U,
            "narrow rows draw the full name after its suffix overwrites the adjacent music path"
        );
    }

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
                result.return_edx == 0x33333333U &&
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
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.font_calls == 5U &&
                result.row_count_status ==
                    openswd3::battle::LegacyBattleActorListQueryStatus::
                        completed &&
                fixture.actors[0U].current_list_index == 0x10U &&
                result.text_draw_calls == 4U &&
                result.selection_rectangle_calls == 1U &&
                result.displayed_rows == 2U && result.final_iterator == 3U &&
                fixture.panel_row_limit == 2U &&
                fixture.selection_input_gate == 1U &&
                fixture.candidate_argument == 2U,
            "narrow grid filters an ineligible node, draws two matches and publishes the selected ordinal"
        );
        test.expect_true(
            fixture.port.requested_profile_ids == std::vector<u16>{1U, 1U},
            "narrow grid loads profiles for the two matching nodes after row initialization"
        );
        test.expect_true(
            result.rows[0U].iterator == 1U && result.rows[0U].row_value == 1U &&
                result.rows[0U].x == 240U && result.rows[0U].y == 166U &&
                result.rows[1U].iterator == 2U &&
                result.rows[1U].row_value == 1U && result.rows[1U].x == 240U &&
                result.rows[1U].y == 188U && result.rows[1U].selected,
            "narrow grid uses matching-node ordinals and the type-one value for both rows"
        );
        test.expect_true(
            draws.size() == 4U && draws[0U].arguments[1U] == 304U &&
                draws[0U].arguments[2U] == 134U &&
                draws[0U].text_token ==
                    openswd3::battle::kLegacyBattleNarrowGridTitleToken &&
                draws[1U].arguments[4U] == 0x2222U &&
                draws[1U].eax == 0x004CD76CU && draws[1U].ecx == 0x004C9A28U &&
                draws[1U].edx == 240U && draws[3U].arguments[1U] == 239U &&
                draws[3U].arguments[2U] == 187U &&
                draws[3U].arguments[4U] == 0xBEEF2222U &&
                draws[3U].eax == 239U && draws[3U].edx == 187U,
            "narrow grid preserves row-value and prior-text-call high words plus callsite registers"
        );
        test.expect_true(
            std::ranges::equal(
                workspace_text(fixture.selection_workspace) |
                    std::views::take(5),
                std::array<u8, 5>{'R', 'O', 'W', '2', 0U}
            ) && workspace_text(fixture.selection_workspace)[5U] == 6U &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xAAAA0077U &&
                fixture.frame_provider.resource_ids.back() == 0xBBBB0077U,
            "narrow grid reuses the physical selection workspace and both stale resource high words"
        );
    }

    {
        Fixture fixture;
        fixture.action_streams.failing_action_id = 0x233BU;
        const auto result =
            openswd3::battle::draw_legacy_battle_narrow_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleNarrowGridFrameStatus::
                        completed &&
                result.panel_action_update.return_value == 0U &&
                result.tiled_frame_calls == 2U &&
                fixture.actors[0U].current_list_index == 0x10U,
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
                fixture.panel_row_limit == 0U &&
                result.return_eax == 0xFFFFF433U &&
                result.return_ecx == 0x004FFA9CU && result.return_edx == 7U,
            "narrow grid invalid group-A code stops at initialization after clearing the shared byte"
        );
    }

    {
        Fixture fixture;
        for (u32 iterator = 1U; iterator <= 9U; ++iterator) {
            fixture.add_node("ROW", iterator > 2U);
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
                result.displayed_rows == 7U && result.final_iterator == 8U &&
                fixture.actors[0U].current_list_index == 0x108U &&
                result.rows[0U].iterator == 1U &&
                result.rows[6U].iterator == 7U && styles.size() == 2U &&
                styles.back().eax == 9U,
            "narrow grid filters nodes before the seven-row limit and keeps the unmatched selection in final EAX"
        );
    }

    {
        Fixture fixture;
        fixture.add_node("SELECT");
        fixture.port.on_profile_read = [&fixture]() {
            fixture.raster.surface.pitch_bytes = 600;
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
