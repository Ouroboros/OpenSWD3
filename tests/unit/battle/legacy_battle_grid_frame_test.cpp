#include "legacy_battle_inventory_fixture.hpp"
#include "openswd3/battle/legacy_battle_grid_frame.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "openswd3/battle/legacy_battle_startup.hpp"
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
    std::array<std::vector<Reply>, 8> replies;
    std::array<std::size_t, 8> reply_offsets{};
    std::function<void(const Request&)> on_call;
};

[[nodiscard]] std::array<u8, 20> text(const std::string_view source) {
    std::array<u8, 20> result{};
    std::ranges::transform(source, result.begin(), [](const char value) {
        return static_cast<u8>(value);
    });
    return result;
}

struct Fixture {
    openswd3::test::LegacyBattleInventoryFixture inventories;
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
        actor_description_record_tokens.fill(0xDEADC0DEU);
        actor_description_text_indices.fill(1U);
        for (u32 index = 0U; index < party.size(); ++index) {
            party[index].configuration.actor_record_token =
                0x00600000U + index * 0x100U;
            auto& list = party[index].actor_list;
            const u32 root_token = 0x79000000U + index * 0x100U;
            list.next_resource_head_token = root_token;
            inventories.assign(list, {{.token = root_token, .name = {}}});
        }
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
            .party = party,
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
    std::array<openswd3::battle::LegacyBattlePartyStartupRecord, 10> party{};
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
    auto& list = fixture.party[0U].actor_list;
    constexpr u32 root_token = 0x79000000U;
    openswd3::test::inventory_node(list, 0U).legacy_next_token =
        root_token + 0x10U;
    for (u32 index = 1U; index <= 5U; ++index) {
        openswd3::test::append_inventory_item(
            list,
            {
                .token = root_token + index * 0x10U,
                .next_token =
                    index == 5U ? 0U : root_token + (index + 1U) * 0x10U,
                .secondary_quantity =
                    static_cast<openswd3::compat::i16>(index == 5U ? 32766 : 0),
                .tertiary_quantity = static_cast<openswd3::compat::i16>(
                    index == 5U ? 32767 : (index == 4U ? 5 : 1)
                ),
                .name = index == 4U ? "Alpha" : "Beta",
                .category_mask = 0x1001U,
                .mode_flags = 0x01U,
                .capacity_gate_flags =
                    static_cast<u16>(index == 4U ? 0x4001U : 0U),
            }
        );
    }

    fixture.port.reply(Call::draw_text, {.edx = 0xA1000001U});
    fixture.port.reply(Call::draw_text, {.edx = 0xA2000002U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB1000003U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB2000004U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB3000005U});
    fixture.port.reply(Call::draw_text, {.edx = 0xB4000006U});
}

void test_grid_resource_item_count(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    constexpr std::array<u32, 6> masks{
        0x10U, 0x0CU, 0x1001U, 0x0800U, 0x2000U, 0x08000000U
    };

    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const u32 selector : {0U, 1U, 2U, 3U, 4U, 5U}) {
            for (u32 outcome = 0U; outcome < 4U; ++outcome) {
                Fixture fixture;
                fixture.queued_actor_code = actor_index + 8U;
                fixture.action_category_index = selector;
                auto& list = fixture.party[actor_index].actor_list;
                const u32 root_token = 0x88000000U + actor_index * 0x100U;
                list.resource_head_token = 0xDEAD0000U;
                list.next_resource_head_token = root_token;
                list.selected_resource_token = 0xCAFEBABEU;
                const bool ordinary = outcome == 1U || outcome == 3U;
                const bool extra = outcome >= 2U;
                fixture.inventories.assign(
                    list,
                    {
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
                    }
                );

                const auto* storage = openswd3::test::inventory_data(list);
                const auto original_definition =
                    openswd3::test::inventory_node(list, 1U)
                        .definition_snapshot;
                const auto result = draw_legacy_battle_grid_frame(
                    fixture.state, fixture.bindings(), fixture.port, request()
                );
                const u16 expected = static_cast<u16>(
                    static_cast<u32>(ordinary) +
                    static_cast<u32>(selector == 4U && extra)
                );
                test.expect_true(
                    result.status == LegacyBattleGridFrameStatus::completed &&
                        result.resource_query_failure ==
                            LegacyBattleActorListQueryStatus::completed &&
                        fixture.panel_row_limit == expected &&
                        list.resource_head_token == root_token &&
                        list.next_resource_head_token == root_token &&
                        list.selected_resource_token == 0xCAFEBABEU &&
                        openswd3::test::inventory_data(list) == storage &&
                        openswd3::test::inventory_nodes(list).size() == 2U &&
                        openswd3::test::inventory_node(list, 1U)
                                .definition_snapshot == original_definition &&
                        result.displayed_rows == 0U &&
                        fixture.selection_input_gate == 0U &&
                        fixture.target_argument == 0U &&
                        fixture.state.row_flags == 0U &&
                        fixture.state.row_value == 0U,
                    "each actual grid actor clears and accumulates the shared word and continues through zero counts without a register gate"
                );
            }
        }
    }

    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const bool partial : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            fixture.action_category_index = 4U;
            fixture.selection_input_gate = 0xFACE0001U;
            fixture.target_argument = 0xABCDEF12U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root_token = 0x89000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root_token;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list,
                {
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
                }
            );

            const auto* storage = openswd3::test::inventory_data(list);
            const auto result = draw_legacy_battle_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
            test.expect_true(
                result.status ==
                        LegacyBattleGridFrameStatus::group_a_actor_typed_stop &&
                    result.resource_query_failure ==
                        LegacyBattleActorListQueryStatus::
                            resource_owner_typed_stop &&
                    fixture.panel_row_limit == (partial ? 2U : 0U) &&
                    list.resource_head_token == 0xFEDCBA98U &&
                    list.next_resource_head_token == root_token &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    openswd3::test::inventory_data(list) == storage &&
                    openswd3::test::inventory_nodes(list).size() == 2U &&
                    result.displayed_rows == 0U &&
                    result.text_draw_calls == 0U && result.font_calls == 5U &&
                    fixture.state.text_storage[0U] == 0xFFU &&
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
        fixture.inventories.assign(
            list,
            {
                {.token = root_token,
                 .next_token = root_token + 0x10U,
                 .name = {}},
                {.token = root_token + 0x10U,
                 .tertiary_quantity = 1,
                 .name = {},
                 .category_mask = 0x2000U,
                 .mode_flags = 0x01U,
                 .capacity_gate_flags = 0x2000U},
            }
        );

        fixture.port.on_call = [&fixture, actor_index](const Request& call) {
            if (call.call == Call::configure_font_width) {
                fixture.queued_actor_code = actor_index + 8U;
                fixture.action_category_index = 4U;
            }
        };

        const auto result = draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, request()
        );
        test.expect_true(
            result.status == LegacyBattleGridFrameStatus::completed &&
                fixture.panel_row_limit == 2U &&
                previous.resource_head_token == 0xDEAD0000U &&
                list.resource_head_token == root_token &&
                openswd3::test::inventory_nodes(list).size() == 2U &&
                fixture.queued_actor_code == actor_index + 8U,
            "resource count borrows the live party after the real font predecessor instead of the actor present at frame entry"
        );
    }
}

void test_grid_resource_rows(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (u32 initial = 0U; initial < 10U; ++initial) {
        Fixture fixture;
        fixture.queued_actor_code = initial + 8U;
        fixture.action_category_index = 0U;
        for (u32 actor = 0U; actor < 10U; ++actor) {
            auto& list = fixture.party[actor].actor_list;
            const u32 root = 0x94000000U + actor * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list, {{.token = root, .next_token = root + 0x10U, .name = {}}}
            );

            for (u32 ordinal = 1U; ordinal <= 8U; ++ordinal) {
                openswd3::test::append_inventory_item(
                    list,
                    {
                        .token = root + ordinal * 0x10U,
                        .next_token =
                            ordinal == 8U ? 0U : root + (ordinal + 1U) * 0x10U,
                        .tertiary_quantity = static_cast<openswd3::compat::i16>(
                            actor * 10U + ordinal
                        ),
                        .name = "p" + std::to_string(actor) + "r" +
                            std::to_string(ordinal),
                        .category_mask = 0x1011U,
                        .mode_flags = 1U,
                    }
                );
            }
        }

        std::size_t previous_draws = 0U;
        fixture.port.on_call = [&fixture,
                                &previous_draws](const Request& request) {
            const auto draws = fixture.port.calls_of(Call::draw_text).size();
            if (request.call == Call::configure_font_style &&
                draws > previous_draws) {
                previous_draws = draws;
                fixture.queued_actor_code =
                    8U + (fixture.queued_actor_code - 7U) % 10U;
                fixture.action_category_index =
                    fixture.action_category_index == 0U ? 2U : 0U;
            }
        };

        auto call = request();
        call.scroll_offset = 0U;
        call.selected_row = 0U;
        const auto result = draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        bool rows_match = true;
        for (u32 row = 0U; row < 7U; ++row) {
            const u32 actor = (initial + row) % 10U;
            const auto& list = fixture.party[actor].actor_list;
            const auto name =
                "p" + std::to_string(actor) + "r" + std::to_string(row + 1U);
            rows_match = rows_match && result.rows[row].iterator == row + 1U &&
                result.rows[row].value == actor * 10U + row + 1U &&
                result.rows[row].row_text == text(name) &&
                list.resource_head_token ==
                    list.next_resource_head_token + (row + 1U) * 0x10U &&
                list.selected_resource_token == 0xCAFEBABEU;
        }

        const auto& next = fixture.party[(initial + 7U) % 10U].actor_list;
        test.expect_true(
            result.status == LegacyBattleGridFrameStatus::completed &&
                result.displayed_rows == 7U && result.final_iterator == 8U &&
                fixture.panel_row_limit == 8U && rows_match &&
                next.resource_head_token == 0xDEAD0000U,
            "each actual row borrows the party and category published by its font predecessor and the seventh row does not query the next party"
        );
    }

    for (u32 initial = 0U; initial < 10U; ++initial) {
        for (const bool failure : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = initial + 8U;
            fixture.action_category_index = 0U;
            const u32 next_actor = (initial + 1U) % 10U;
            auto& list = fixture.party[initial].actor_list;
            const u32 root = 0x95000000U + initial * 0x100U;
            list.next_resource_head_token = root;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                    {.token = root + 0x10U,
                     .tertiary_quantity = 1,
                     .name = "R",
                     .category_mask = 0x10U,
                     .mode_flags = 1U},
                }
            );

            auto& next = fixture.party[next_actor].actor_list;
            const u32 next_root = next.next_resource_head_token;
            next.resource_head_token = 0xDEAD0000U;
            if (failure) {
                next.next_resource_head_token = 0xFEDCBA98U;
            }

            fixture.port.on_call = [&fixture,
                                    next_actor](const Request& request) {
                if (request.call == Call::configure_font_style &&
                    fixture.port.calls_of(Call::draw_text).size() == 2U) {
                    fixture.queued_actor_code = next_actor + 8U;
                }
            };

            auto call = request();
            call.scroll_offset = 0U;
            call.selected_row = 0U;
            const auto result = draw_legacy_battle_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, call
            );
            test.expect_true(
                result.status ==
                        (failure ? LegacyBattleGridFrameStatus::
                                       group_a_actor_typed_stop
                                 : LegacyBattleGridFrameStatus::completed) &&
                    result.resource_query_failure ==
                        (failure
                             ? LegacyBattleActorListQueryStatus::
                                   resource_node_typed_stop
                             : LegacyBattleActorListQueryStatus::completed) &&
                    result.displayed_rows == 1U &&
                    result.font_calls == (failure ? 6U : 7U) &&
                    list.resource_head_token == root + 0x10U &&
                    next.resource_head_token ==
                        (failure ? 0xFEDCBA98U : next_root) &&
                    fixture.state.text_storage[0U] == 'R' &&
                    fixture.state.text_storage[1U] == 0U &&
                    fixture.state.row_value == 1U &&
                    fixture.state.row_flags == 0U &&
                    fixture.selection_input_gate == 0U &&
                    fixture.target_argument == 0U,
                "live exhaustion resets the newly selected party while a subsequent owner failure preserves the previous row and blocks tail font restoration"
            );
        }
    }

    for (u32 actor = 0U; actor < 10U; ++actor) {
        Fixture fixture;
        fixture.queued_actor_code = actor + 8U;
        fixture.action_category_index = 0U;
        auto& list = fixture.party[actor].actor_list;
        const u32 root = 0x96000000U + actor * 0x100U;
        list.next_resource_head_token = root;
        fixture.inventories.assign(
            list,
            {
                {.token = root, .next_token = root + 0x10U, .name = {}},
                {.token = root + 0x10U,
                 .next_token = root + 0x20U,
                 .name = "Hidden"},
                {.token = root + 0x20U,
                 .tertiary_quantity = 1,
                 .name = "R",
                 .category_mask = 0x10U,
                 .mode_flags = 1U},
            }
        );

        auto call = request();
        call.scroll_offset = 0xFFFFFFFFU;
        call.selected_row = 0U;
        const auto result = draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        test.expect_true(
            result.status == LegacyBattleGridFrameStatus::completed &&
                result.hidden_rows == 1U && result.scanned_rows == 2U &&
                result.displayed_rows == 1U && result.rows[0U].iterator == 1U &&
                result.rows[0U].row_text[0U] == 'R' &&
                result.final_iterator == 2U && fixture.panel_row_limit == 1U &&
                list.resource_head_token == root,
            "wrapped scroll produces a real stale invisible ordinal before the first qualified resource and does not consume a displayed row"
        );
    }

    for (const std::size_t length : {19U, 20U, 21U, 31U, 39U}) {
        Fixture fixture;
        fixture.action_category_index = 0U;
        auto& list = fixture.party[0U].actor_list;
        const u32 root = list.next_resource_head_token;
        openswd3::test::inventory_node(list, 0U).legacy_next_token =
            root + 0x10U;
        openswd3::test::append_inventory_item(
            list,
            {
                .token = root + 0x10U,
                .tertiary_quantity = 5,
                .name = std::string(length, 'N'),
                .category_mask = 0x10U,
                .mode_flags = 1U,
            }
        );
        auto call = request();
        call.scroll_offset = 0U;
        call.selected_row = 0U;
        const auto result = draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status == LegacyBattleGridFrameStatus::completed &&
                result.displayed_rows == 1U && draws.size() == 2U &&
                draws[1U].text_bytes[0U] == 'N' &&
                draws[1U].text_bytes[19U] == (length == 19U ? 0U : 'N') &&
                draws[1U].text_bytes[20U] == ' ' &&
                draws[1U].text_bytes[21U] == '5' &&
                draws[1U].text_bytes[22U] == 0U &&
                (length <= 23U || draws[1U].text_bytes[23U] == 'N') &&
                fixture.state.text_storage == draws[1U].text_bytes,
            "long names share the numeric buffer and formatting overwrites only digits and terminator before name drawing"
        );
    }
}

}  // namespace

void test_battle_grid_frame(openswd3::test::Context& test) {
    test_grid_resource_item_count(test);
    test_grid_resource_rows(test);
    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const bool exhausted : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            fixture.action_category_index = 0U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x80000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                }
            );

            const u32 row_count = exhausted ? 1U : 7U;
            for (u32 row = 1U; row <= row_count; ++row) {
                openswd3::test::append_inventory_item(
                    list,
                    {
                        .token = root + row * 0x10U,
                        .next_token =
                            row == row_count ? 0U : root + (row + 1U) * 0x10U,
                        .tertiary_quantity = 1,
                        .name = "row",
                        .category_mask = 0x10U,
                        .mode_flags = 0x01U,
                    }
                );
            }

            auto bindings = fixture.bindings();
            auto selected = request();
            selected.scroll_offset = 0U;
            selected.selected_row = 0U;
            const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
                fixture.state, bindings, fixture.port, selected
            );
            test.expect_true(
                result.status ==
                        openswd3::battle::LegacyBattleGridFrameStatus::
                            completed &&
                    result.displayed_rows == row_count &&
                    list.resource_head_token ==
                        (exhausted ? root : root + 0x70U) &&
                    list.next_resource_head_token == root &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    openswd3::test::inventory_nodes(list).size() ==
                        row_count + 1U,
                "each grid actor reads its actual next-root chain and resets the head only on the exhaustion branch"
            );
        }
    }

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
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                result.action_frame_calls == 5U && result.font_calls == 9U &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.party[0U].actor_list.resource_head_token ==
                    fixture.party[0U].actor_list.next_resource_head_token &&
                result.scanned_rows == 2U && result.hidden_rows == 0U &&
                result.displayed_rows == 2U && result.text_draw_calls == 6U &&
                result.selection_rectangle_calls == 2U &&
                result.shared_text_resolution_calls == 1U &&
                result.shared_text_length_calls == 1U &&
                result.final_iterator == 6U && result.selected_iterator == 5U,
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
            fixture.panel_row_limit == 5U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 5U &&
                result.rows[0U].iterator == 4U &&
                result.rows[0U].secondary_color &&
                result.rows[1U].iterator == 5U &&
                !result.rows[1U].secondary_color &&
                result.rows[0U].numeric_text[0U] == ' ' &&
                result.rows[0U].numeric_text[1U] == '5' &&
                result.rows[1U].numeric_text[0U] == '-' &&
                result.rows[1U].numeric_text[1U] == '3',
            "grid frame reads actual row ordinals and formats positive 32-bit quantities through their signed output words"
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
                result.action_frame_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0U &&
                fixture.panel_row_limit == 0xFFFFU,
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
                result.action_frame_calls == 1U &&
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
                fixture.panel_row_limit == 0U,
            "invalid group-A code stops at row initialization after the complete panel prefix"
        );
    }

    {
        Fixture fixture;
        auto& list = fixture.party[0U].actor_list;
        constexpr u32 root = 0x79000000U;
        openswd3::test::inventory_node(list, 0U).legacy_next_token =
            root + 0x10U;
        for (u32 index = 1U; index <= 519U; ++index) {
            openswd3::test::append_inventory_item(
                list,
                {
                    .token = root + index * 0x10U,
                    .next_token =
                        index == 519U ? 0U : root + (index + 1U) * 0x10U,
                    .tertiary_quantity = 1,
                    .name = "Visible",
                    .category_mask = index > 512U ? 0x1001U : 0U,
                    .mode_flags = 0x01U,
                }
            );
        }

        auto call = request();
        call.selected_row = 0U;
        call.scroll_offset = 0U;
        const auto result = openswd3::battle::draw_legacy_battle_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, call
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleGridFrameStatus::completed &&
                result.scanned_rows == 7U && result.hidden_rows == 0U &&
                result.displayed_rows == 7U && result.final_iterator == 8U &&
                list.resource_head_token == root + 519U * 0x10U &&
                openswd3::test::inventory_nodes(list).size() == 520U,
            "actual row queries scan past 512 ineligible nodes and stop after the seventh visible row without an extra query"
        );
    }

    {
        Fixture fixture;
        prepare_rows(fixture);
        fixture.port.on_call = [&fixture](const Request& request) {
            if (request.call == Call::configure_font_style &&
                fixture.port.calls_of(Call::draw_text).size() == 2U) {
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
                fixture.target_argument == 5U &&
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
                fixture.target_argument == 5U && result.displayed_rows == 1U,
            "missing MAPS directory stops after selected rectangles and input publication but before row completion"
        );
    }
}
