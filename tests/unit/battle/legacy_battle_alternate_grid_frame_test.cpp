#include "openswd3/battle/legacy_battle_alternate_grid_frame.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::i16;
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
    std::array<std::vector<Reply>, 9> replies;
    std::array<std::size_t, 9> reply_offsets{};
    std::function<void(const Request&)> on_call;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        auto& list = party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {{.token = 0x76000000U, .name = {}}};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleAlternateGridFrameBindings
    bindings() {
        return {
            .queued_actor_code = queued_actor_code,
            .panel_row_limit = panel_row_limit,
            .selection_input_gate = selection_input_gate,
            .target_argument = target_argument,
            .primary_text_color = primary_text_color,
            .party = party,
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
    std::array<openswd3::battle::LegacyBattlePartyStartupRecord, 10> party{};
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
    auto& list = fixture.party[0U].actor_list;
    constexpr u32 root = 0x76000000U;
    constexpr std::size_t prefix_rows = 3U;
    const std::size_t row_count = prefix_rows + nonzero;
    list.next_resource_head_token = root;
    list.resources = {{.token = root, .next_token = root + 0x10U, .name = {}}};
    for (std::size_t index = 0U; index < row_count; ++index) {
        const u32 token = root + static_cast<u32>(index + 1U) * 0x10U;
        list.resources.push_back({
            .token = token,
            .next_token = index + 1U == row_count ? 0U : token + 0x10U,
            .secondary_quantity = static_cast<i16>(
                index == prefix_rows + 1U ? -3
                                          : static_cast<int>(index) -
                        static_cast<int>(prefix_rows) + 5
            ),
            .name = index < prefix_rows
                ? "prefix"
                : std::string(1U, static_cast<char>('A' + index - prefix_rows)),
            .capacity_gate_flags = 0x2000U,
        });
    }
}

[[nodiscard]] bool
same_text(const std::array<u8, 20>& value, const std::string_view expected) {
    return std::ranges::equal(
        value | std::views::take(expected.size()), expected
    );
}

void test_alternate_grid_resource_item_count(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    constexpr std::array<u32, 6> masks{
        0x10U, 0x0CU, 0x1001U, 0x0800U, 0x2000U, 0x08000000U
    };

    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const u32 selector : {4U}) {
            for (u32 outcome = 0U; outcome < 4U; ++outcome) {
                Fixture fixture;
                fixture.queued_actor_code = actor_index + 8U;
                auto& list = fixture.party[actor_index].actor_list;
                const u32 root_token = 0x88000000U + actor_index * 0x100U;
                list.resource_head_token = 0xDEAD0000U;
                list.next_resource_head_token = root_token;
                list.selected_resource_token = 0xCAFEBABEU;
                const bool ordinary = outcome == 1U || outcome == 3U;
                const bool extra = outcome >= 2U;
                list.resources = {
                    {.token = root_token,
                     .next_token = root_token + 0x10U,
                     .name = {}},
                    {.token = root_token + 0x10U,
                     .resource_id = 0x0300U,
                     .tertiary_quantity = 1,
                     .name = std::string(100U, 'n'),
                     .category_mask = ordinary ? masks[selector] : 0U,
                     .mode_flags = static_cast<u8>(ordinary ? 1U : 2U),
                     .capacity_gate_flags =
                         static_cast<u16>(extra ? 0x2000U : 0U)},
                };

                const auto* storage = list.resources.data();
                const auto result = draw_legacy_battle_alternate_grid_frame(
                    fixture.state, fixture.bindings(), fixture.port, request()
                );
                const u16 expected = static_cast<u16>(
                    static_cast<u32>(ordinary) +
                    static_cast<u32>(selector == 4U && extra)
                );
                test.expect_true(
                    result.status ==
                            LegacyBattleAlternateGridFrameStatus::completed &&
                        result.resource_query_failure ==
                            LegacyBattleActorListQueryStatus::completed &&
                        fixture.panel_row_limit == expected &&
                        list.resource_head_token == root_token &&
                        list.next_resource_head_token == root_token &&
                        list.selected_resource_token == 0xCAFEBABEU &&
                        list.resources.data() == storage &&
                        list.resources.size() == 2U &&
                        list.resources[1U].name.size() == 100U &&
                        result.displayed_rows == 0U &&
                        fixture.selection_input_gate == 0U &&
                        fixture.target_argument == 0U &&
                        result.font_calls == 4U && result.text_draw_calls == 1U,
                    "each actual grid actor clears and accumulates the shared word and continues through zero counts without a register gate"
                );
            }
        }
    }

    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const bool partial : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;

            fixture.selection_input_gate = 0xFACE0001U;
            fixture.target_argument = 0xABCDEF12U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root_token = 0x89000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root_token;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = root_token,
                 .next_token = partial ? root_token + 0x10U : 0xFEDCBA98U,
                 .name = {}},
                {.token = root_token + 0x10U,
                 .next_token = 0xFEDCBA98U,
                 .tertiary_quantity = 1,
                 .name = std::string(100U, 'n'),
                 .category_mask = 0x2000U,
                 .mode_flags = 0x01U,
                 .capacity_gate_flags = 0x2000U},
            };

            const auto* storage = list.resources.data();
            const auto result = draw_legacy_battle_alternate_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
            test.expect_true(
                result.status ==
                        LegacyBattleAlternateGridFrameStatus::
                            group_a_actor_typed_stop &&
                    result.resource_query_failure ==
                        LegacyBattleActorListQueryStatus::
                            resource_owner_typed_stop &&
                    fixture.panel_row_limit == (partial ? 2U : 0U) &&
                    list.resource_head_token == 0xFEDCBA98U &&
                    list.next_resource_head_token == root_token &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    list.resources.data() == storage &&
                    list.resources.size() == 2U &&
                    result.displayed_rows == 0U &&
                    result.text_draw_calls == 1U && result.font_calls == 3U &&
                    fixture.state.row_text[0U] == 0xFFU &&
                    fixture.selection_input_gate == 0xFACE0001U &&
                    fixture.target_argument == 0xABCDEF12U,
                "actual grid count failure preserves the published head and partial word and blocks root reset, row drawing and input changes"
            );
        }
    }

    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        Fixture fixture;
        const u32 previous_index = (actor_index + 1U) % 10U;
        fixture.queued_actor_code = previous_index + 8U;
        auto& previous = fixture.party[previous_index].actor_list;
        previous.resource_head_token = 0xDEAD0000U;
        auto& list = fixture.party[actor_index].actor_list;
        const u32 root_token = 0x8A000000U + actor_index * 0x100U;
        list.next_resource_head_token = root_token;
        list.resources = {
            {.token = root_token, .next_token = root_token + 0x10U, .name = {}},
            {.token = root_token + 0x10U,
             .tertiary_quantity = 1,
             .name = {},
             .category_mask = 0x2000U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x2000U},
        };

        fixture.port.on_call = [&fixture, actor_index](const Request& call) {
            if (call.call == Call::configure_font_width) {
                fixture.queued_actor_code = actor_index + 8U;
            }
        };

        const auto result = draw_legacy_battle_alternate_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status == LegacyBattleAlternateGridFrameStatus::completed &&
                fixture.panel_row_limit == 2U &&
                previous.resource_head_token == 0xDEAD0000U &&
                list.resource_head_token == root_token &&
                list.resources.size() == 2U &&
                fixture.queued_actor_code == actor_index + 8U,
            "resource count borrows the live party after the real font predecessor instead of the actor present at frame entry"
        );
    }
}

}  // namespace

void test_battle_alternate_grid_frame(openswd3::test::Context& test) {
    test_alternate_grid_resource_item_count(test);
    struct QuantityVector {
        u16 secondary;
        u16 tertiary;
        u16 expected;
        std::string_view formatted;
    };

    constexpr std::array<QuantityVector, 7> quantities{{
        {0U, 0U, 0U, " 0"},
        {1U, 2U, 3U, " 3"},
        {0xFFFFU, 1U, 0U, " 0"},
        {0x7FFFU, 1U, 0x8000U, "-32768"},
        {0x8000U, 0U, 0x8000U, "-32768"},
        {0xFFFFU, 0xFFFFU, 0xFFFEU, "-2"},
        {0x8000U, 0x8000U, 0U, " 0"},
    }};
    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const auto& quantity : quantities) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x81000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = root, .next_token = root + 0x10U, .name = {}},
                {.token = root + 0x10U,
                 .next_token = root + 0x20U,
                 .secondary_quantity = std::bit_cast<i16>(quantity.secondary),
                 .tertiary_quantity = std::bit_cast<i16>(quantity.tertiary),
                 .name = std::string(19U, 'n'),
                 .capacity_gate_flags = 0x2000U},
                {.token = root + 0x20U,
                 .secondary_quantity = std::bit_cast<i16>(quantity.secondary),
                 .tertiary_quantity = std::bit_cast<i16>(quantity.tertiary),
                 .name = "b",
                 .capacity_gate_flags = 0x2000U},
            };

            auto bindings = fixture.bindings();
            auto selected = request();
            selected.scroll_offset = 0U;
            selected.selected_row = 2U;
            const auto result =
                openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                    fixture.state, bindings, fixture.port, selected
                );
            auto expected_tail = result.rows[0U].row_text;
            expected_tail[0U] = static_cast<u8>('b');
            expected_tail[1U] = 0U;
            const auto draws = fixture.port.calls_of(Call::draw_text);
            test.expect_true(
                result.status ==
                        openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                            completed &&
                    result.resource_query_failure ==
                        openswd3::battle::LegacyBattleActorListQueryStatus::
                            completed &&
                    result.displayed_rows == 2U &&
                    result.final_iterator == 3U &&
                    result.rows[0U].value == quantity.expected &&
                    result.rows[1U].value == quantity.expected &&
                    same_text(
                        result.rows[0U].numeric_text, quantity.formatted
                    ) &&
                    same_text(
                        result.rows[1U].numeric_text, quantity.formatted
                    ) &&
                    same_text(
                        result.rows[0U].row_text, std::string(19U, 'n')
                    ) &&
                    result.rows[1U].row_text == expected_tail &&
                    fixture.state.row_text == expected_tail &&
                    fixture.state.row_value == quantity.expected &&
                    fixture.panel_row_limit == 2U &&
                    fixture.selection_input_gate == 1U &&
                    fixture.target_argument == 2U &&
                    list.resource_head_token == root &&
                    list.next_resource_head_token == root &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    list.resources.size() == 3U && draws.size() == 6U &&
                    draws[1U].arguments[4U] == fixture.primary_text_color,
                "all ten actual actors display signed word sums, retain shorter-name tails and last outputs after real exhaustion"
            );
        }
    }

    for (const u32 scroll : {0xFFFFFFFFU, 1U}) {
        Fixture fixture;
        auto& list = fixture.party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .name = "ordinary"},
            {.token = 0x76000020U,
             .secondary_quantity = 3,
             .name = "marked",
             .capacity_gate_flags = 0x2000U},
        };

        auto bindings = fixture.bindings();
        auto selected = request();
        selected.scroll_offset = scroll;
        selected.selected_row = 1U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, bindings, fixture.port, selected
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.displayed_rows == (scroll == 0xFFFFFFFFU ? 2U : 0U) &&
                list.resource_head_token == 0x76000000U &&
                (scroll != 0xFFFFFFFFU ||
                 (result.rows[0U].iterator == 0U &&
                  result.rows[0U].value == 0U &&
                  same_text(result.rows[0U].row_text, "ordinary") &&
                  fixture.target_argument == 0U &&
                  fixture.selection_input_gate == 1U)),
            "actual grid retains zero-ordinal stale success after DWORD scroll wrap and missing-ordinal exhaustion"
        );
    }

    {
        Fixture fixture;
        auto& list = fixture.party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .secondary_quantity = 7,
             .name = std::string(20U, 'n'),
             .capacity_gate_flags = 0x2000U},
        };

        auto bindings = fixture.bindings();
        auto selected = request();
        selected.scroll_offset = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, bindings, fixture.port, selected
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        group_a_actor_typed_stop &&
                result.resource_query_failure ==
                    openswd3::battle::LegacyBattleActorListQueryStatus::
                        list_text_typed_stop &&
                result.displayed_rows == 0U && fixture.state.row_value == 0U &&
                fixture.state.row_text[0U] == 0xFFU &&
                list.resource_head_token == 0x76000010U &&
                fixture.panel_row_limit == 1U && result.text_draw_calls == 1U &&
                result.font_calls == 3U && fixture.selection_input_gate == 0U,
            "real text-boundary failure preserves title and count prefix and blocks quantity, rows and exhaustion reset"
        );
    }

    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const bool exhausted : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x80000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = root, .next_token = root + 0x10U, .name = {}},
            };

            const u32 row_count = exhausted ? 1U : 8U;
            for (u32 row = 1U; row <= row_count; ++row) {
                list.resources.push_back({
                    .token = root + row * 0x10U,
                    .next_token =
                        row == row_count ? 0U : root + (row + 1U) * 0x10U,
                    .secondary_quantity = 1,
                    .name = "row",
                    .capacity_gate_flags = 0x2000U,
                });
            }

            auto bindings = fixture.bindings();
            auto selected = request();
            selected.scroll_offset = 0U;
            selected.selected_row = 0U;
            const auto result =
                openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                    fixture.state, bindings, fixture.port, selected
                );
            test.expect_true(
                result.status ==
                        openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                            completed &&
                    result.displayed_rows == (exhausted ? 1U : 7U) &&
                    list.resource_head_token ==
                        (exhausted ? root : root + 0x80U) &&
                    list.next_resource_head_token == root &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    list.resources.size() == row_count + 1U,
                "each alternate-grid actor retains the eighth queried node on a full page and resets only after exhaustion"
            );
        }
    }

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
                result.return_edx == 0U &&
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
        const auto draws = fixture.port.calls_of(Call::draw_text);
        const auto fonts = fixture.port.calls_of(Call::configure_font_style);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.font_calls == 12U &&
                result.displayed_rows == 7U && result.text_draw_calls == 16U &&
                result.selection_rectangle_calls == 1U,
            "alternate grid completes its panel, actor and seven-row call counts"
        );
        test.expect_true(
            result.final_iterator == 11U && result.selected_iterator == 5U &&
                fixture.panel_row_limit == 11U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 5U &&
                fixture.party[0U].actor_list.resource_head_token ==
                    0x760000B0U &&
                same_text(fixture.state.row_text, "H") &&
                fixture.state.row_value == 12U,
            "alternate grid performs the eighth query and publishes the selected iterator"
        );
        test.expect_true(
            draws.size() == 16U && draws[0U].arguments[1U] == 304U &&
                draws[0U].arguments[2U] == 134U &&
                draws[0U].text_token == 0x004A76A0U &&
                draws[0U].arguments[4U] == 0xFFC0U &&
                draws[1U].arguments[1U] == 240U &&
                draws[1U].arguments[2U] == 170U &&
                draws[1U].arguments[4U] == 0x2222U &&
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
        auto& list = fixture.party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .secondary_quantity = 2,
             .tertiary_quantity = 3,
             .name = "typed",
             .capacity_gate_flags = 0x2000U},
        };

        auto bindings = fixture.bindings();
        auto typed_request = request();
        typed_request.selected_row = 1U;
        typed_request.scroll_offset = 0U;
        const auto result =
            openswd3::battle::draw_legacy_battle_alternate_grid_frame(
                fixture.state, bindings, fixture.port, typed_request
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleAlternateGridFrameStatus::
                        completed &&
                fixture.panel_row_limit == 1U && result.displayed_rows == 1U &&
                result.rows[0U].value == 5U &&
                same_text(result.rows[0U].row_text, "typed") &&
                same_text(fixture.state.row_text, "typed") &&
                fixture.state.row_value == 5U &&
                list.resource_head_token == 0x76000000U,
            "alternate grid production path enumerates bit-thirteen resources through the typed owner"
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
                fixture.panel_row_limit == 0U && title_calls.size() == 1U &&
                fixture.party[0U].actor_list.resource_head_token == 0U &&
                fixture.party[0U].actor_list.resources.size() == 1U,
            "alternate grid invalid one-before group-A code stops only at list initialization"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture, 2U);
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
                result.displayed_rows == 2U &&
                same_text(fixture.state.row_text, "B") &&
                fixture.state.row_value == 0xFFFDU &&
                fixture.party[0U].actor_list.resource_head_token ==
                    fixture.party[0U].actor_list.next_resource_head_token &&
                result.final_iterator == 6U && result.font_calls == 6U,
            "alternate grid zero query refreshes the actor and enters the final font tail"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::draw_text &&
                fixture.state.displayed_rows == 1U) {
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
                result.displayed_rows == 1U &&
                fixture.party[0U].actor_list.resource_head_token ==
                    0x76000050U &&
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
