#include "legacy_battle_inventory_fixture.hpp"
#include "openswd3/battle/legacy_battle_mode_grid_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <ranges>
#include <span>
#include <string>
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
    std::array<std::vector<Reply>, 10> replies;
    std::array<std::size_t, 10> reply_offsets{};
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
    openswd3::test::LegacyBattleInventoryFixture inventories;
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        for (std::size_t actor_index = 0U; actor_index < party.size();
             ++actor_index) {
            auto& list = party[actor_index].actor_list;
            const u32 root =
                0x70000000U + static_cast<u32>(actor_index) * 0x100U;
            list.next_resource_head_token = root;
            inventories.assign(list, {{.token = root, .name = {}}});
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleModeGridFrameBindings
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

    openswd3::battle::LegacyBattleModeGridFrameState state;
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

[[nodiscard]] openswd3::battle::LegacyBattleModeGridFrameRequest request() {
    openswd3::battle::LegacyBattleModeGridFrameRequest value{
        .origin_x = 224U,
        .origin_y = 126U,
        .selected_cell = 3U,
        .entry_eax = 0x11111111U,
        .entry_ecx = 0x22222222U,
        .entry_edx = 0x33333333U,
        .row_text_token = 0x0012FFCCU,
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
    auto& list = fixture.party[0U].actor_list;
    list.next_resource_head_token = 0x76000000U;
    fixture.inventories.assign(
        list,
        {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .resource_id = 0x0300U,
             .secondary_quantity = 2,
             .name = "PRI",
             .mode_flags = 0x01U},
            {.token = 0x76000020U,
             .next_token = 0x76000030U,
             .resource_id = 0x0222U,
             .secondary_quantity = 2,
             .name = "S1",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x76000030U,
             .resource_id = 0x0333U,
             .secondary_quantity = 2,
             .name = "LASTTAIL",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
        }
    );
}

void test_mode_grid_resource_display(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    const std::array<std::array<u16, 3>, 7> quantity_cases{{
        {0U, 0U, 0U},
        {1U, 2U, 3U},
        {0xFFFFU, 1U, 0U},
        {0x7FFFU, 1U, 0x8000U},
        {0x8000U, 0U, 0x8000U},
        {0xFFFFU, 0xFFFFU, 0xFFFEU},
        {0x8000U, 0x8000U, 0U},
    }};
    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const auto& quantity_case : quantity_cases) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x81000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                    {.token = root + 0x10U,
                     .next_token = root + 0x20U,
                     .resource_id = 0x0300U,
                     .secondary_quantity = 1,
                     .name = std::string(19U, 'p'),
                     .mode_flags = 1U},
                    {.token = root + 0x20U,
                     .resource_id = 0x0222U,
                     .secondary_quantity = std::bit_cast<openswd3::compat::i16>(
                         quantity_case[0U]
                     ),
                     .tertiary_quantity = std::bit_cast<openswd3::compat::i16>(
                         quantity_case[1U]
                     ),
                     .name = "b",
                     .category_mask = 0x08000000U,
                     .mode_flags = 4U},
                }
            );

            const auto* original_storage = openswd3::test::inventory_data(list);
            auto selected = request();
            selected.selected_cell = 0U;
            const auto result = draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, selected
            );
            const auto secondary_rows =
                std::ranges::count_if(result.cells, [](const auto& cell) {
                    return cell.queried_secondary;
                });
            const bool has_secondary = quantity_case[2U] != 0U;
            test.expect_true(
                result.status == LegacyBattleModeGridFrameStatus::completed &&
                    result.resource_query_failure ==
                        LegacyBattleActorListQueryStatus::completed &&
                    fixture.state.primary_count == 1U &&
                    fixture.state.secondary_count == quantity_case[2U] &&
                    fixture.state.group_slot_count ==
                        (has_secondary ? quantity_case[2U] : 0U) &&
                    secondary_rows == std::min<u32>(9U, quantity_case[2U]) &&
                    fixture.panel_row_limit ==
                        static_cast<u16>(1U + quantity_case[2U]) &&
                    list.resource_head_token ==
                        (has_secondary ? root + 0x20U : root) &&
                    list.next_resource_head_token == root &&
                    list.selected_resource_token ==
                        (has_secondary ? root + 0x20U : 0xCAFEBABEU) &&
                    openswd3::test::inventory_data(list) == original_storage &&
                    openswd3::test::inventory_nodes(list).size() == 3U,
                "every actual party slot preserves quantity WORD wrap, signed group progression and resource ownership across mode rendering"
            );
            if (has_secondary) {
                test.expect_true(
                    result.cells[1U].row_text[0U] == static_cast<u8>('b') &&
                        result.cells[1U].row_text[1U] == 0U &&
                        result.cells[1U].row_text[2U] == static_cast<u8>('p') &&
                        result.cells[1U].row_text[18U] ==
                            static_cast<u8>('p') &&
                        result.cells[1U].row_text[19U] == 0U,
                    "the shorter group label retains the actual primary label tail after its first NUL"
                );
            }
        }
    }

    {
        Fixture fixture;
        for (u32 actor_index = 0U; actor_index < 3U; ++actor_index) {
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x79000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U + actor_index;
            list.next_resource_head_token = root;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                    {.token = root + 0x10U,
                     .next_token = root + 0x20U,
                     .resource_id = 0x0300U,
                     .secondary_quantity = static_cast<openswd3::compat::i16>(
                         actor_index == 1U ? 1U : 9U
                     ),
                     .name = actor_index == 1U ? "primary" : "unused",
                     .mode_flags = 1U},
                    {.token = root + 0x20U,
                     .resource_id = 0x0222U,
                     .secondary_quantity = static_cast<openswd3::compat::i16>(
                         actor_index == 2U ? 3U : 2U
                     ),
                     .name = actor_index == 2U ? "later" : "initial",
                     .category_mask = 0x08000000U,
                     .mode_flags = 1U},
                }
            );
        }

        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::configure_font_width) {
                fixture.queued_actor_code = 9U;
            } else if (
                call.call == Call::draw_text && call.arguments[1U] == 240U &&
                call.arguments[2U] == 170U
            ) {
                fixture.queued_actor_code = 10U;
            }
        };

        auto selected = request();
        selected.selected_cell = 0U;
        const auto result = draw_legacy_battle_mode_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, selected
        );
        test.expect_true(
            result.status == LegacyBattleModeGridFrameStatus::completed &&
                fixture.state.primary_count == 1U &&
                fixture.state.secondary_count == 2U &&
                fixture.state.group_slot_count == 3U &&
                fixture.panel_row_limit == 3U &&
                fixture.party[0U].actor_list.resource_head_token ==
                    0xDEAD0000U &&
                fixture.party[1U].actor_list.resource_head_token ==
                    0x79000100U &&
                fixture.party[2U].actor_list.resource_head_token ==
                    0x79000220U &&
                fixture.party[2U].actor_list.selected_resource_token ==
                    0x79000220U &&
                starts_with(
                    result.cells[1U].row_text,
                    std::array<u8, 6>{'l', 'a', 't', 'e', 'r', 0U}
                ),
            "the font and first-row predecessors change the actual queued party consumed by primary and subsequent group queries"
        );
    }

    {
        Fixture fixture;
        prepare_success(fixture);
        openswd3::test::inventory_node(fixture.party[0U].actor_list, 1U)
            .quantity_a = 1;
        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::draw_text && call.arguments[1U] == 240U &&
                call.arguments[2U] == 170U) {
                fixture.queued_actor_code = 7U;
            }
        };

        auto selected = request();
        selected.selected_cell = 0U;
        const auto result = draw_legacy_battle_mode_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, selected
        );
        test.expect_true(
            result.status ==
                    LegacyBattleModeGridFrameStatus::group_a_actor_typed_stop &&
                fixture.panel_row_limit == 1U &&
                fixture.state.primary_count == 1U &&
                fixture.state.group_slot_count == 0U &&
                fixture.party[0U].actor_list.resource_head_token ==
                    0x76000000U &&
                result.text_draw_calls == 2U && result.font_calls == 4U &&
                result.cells[1U].queried_secondary &&
                fixture.selection_input_gate == 0U,
            "an invalid live party stops at its group access and retains the actual primary and first-row prefix"
        );
    }

    {
        Fixture fixture;
        prepare_success(fixture);
        auto& list = fixture.party[0U].actor_list;
        list.selected_resource_token = 0xCAFEBABEU;
        openswd3::test::inventory_node(list, 1U)
            .definition_snapshot[0x46U - 0x0CU] = 0U;
        openswd3::test::set_inventory_name(
            openswd3::test::inventory_node(list, 1U), std::string(100U, 'x')
        );
        openswd3::test::inventory_node(list, 2U).quantity_a = 0;
        openswd3::test::inventory_node(list, 3U).quantity_a = 0;
        auto selected = request();
        selected.selected_cell = 0U;
        const auto result = draw_legacy_battle_mode_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, selected
        );
        test.expect_true(
            result.status == LegacyBattleModeGridFrameStatus::completed &&
                fixture.state.primary_count == 0U &&
                fixture.panel_row_limit == 0U &&
                result.text_copy_calls == 10U &&
                result.text_draw_calls == 11U &&
                list.resource_head_token == 0x76000000U &&
                list.selected_resource_token == 0xCAFEBABEU,
            "a fixed-ID success with closed output does not inspect its long name and still proceeds through counts and ten missing cells"
        );
    }

    for (const bool close_output_gate : {false, true}) {
        Fixture fixture;
        auto& list = fixture.party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        fixture.inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 0x0222U,
                 .secondary_quantity = static_cast<openswd3::compat::i16>(
                     close_output_gate ? 3U : 2U
                 ),
                 .name = "b",
                 .category_mask = 0x08000000U,
                 .mode_flags = 1U},
            }
        );

        fixture.port.on_call = [&fixture,
                                close_output_gate](const Request& call) {
            if (call.call == Call::draw_text && call.arguments[1U] == 240U &&
                call.arguments[2U] == 170U) {
                auto& node = openswd3::test::inventory_node(
                    fixture.party[0U].actor_list, 1U
                );
                if (close_output_gate) {
                    node.definition_snapshot[0x46U - 0x0CU] = 2U;
                    openswd3::test::set_inventory_name(
                        node, std::string(30U, 'x')
                    );
                } else {
                    openswd3::test::inventory_category(node) = 0U;
                }
            }
        };

        auto selected = request();
        selected.selected_cell = 0U;
        const auto result = draw_legacy_battle_mode_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, selected
        );
        const u16 count = close_output_gate ? 3U : 2U;
        test.expect_true(
            result.status == LegacyBattleModeGridFrameStatus::completed &&
                result.resource_query_failure ==
                    LegacyBattleActorListQueryStatus::completed &&
                fixture.state.primary_count == 0U &&
                fixture.state.group_slot_count == count &&
                fixture.panel_row_limit == count &&
                list.resource_head_token ==
                    (close_output_gate ? 0x76000010U : 0U) &&
                list.selected_resource_token == 0x76000010U &&
                result.cells[1U].row_text[0U] == static_cast<u8>('b') &&
                result.cells[1U].row_text[1U] == 0U &&
                result.cells[count - 1U].page == 2U &&
                result.cells[count - 1U].group_index == 0U &&
                result.text_draw_calls == 11U,
            "closed group output and normal exhaustion retain old capacity and name while the parent advances by the original signed count"
        );
    }

    for (const bool failing_primary : {false, true}) {
        Fixture fixture;
        prepare_success(fixture);
        auto& list = fixture.party[0U].actor_list;
        list.selected_resource_token = 0xCAFEBABEU;
        openswd3::test::inventory_node(list, 1U).quantity_a = 1;
        openswd3::test::inventory_node(list, 2U).quantity_a = 1;
        openswd3::test::inventory_node(list, 3U).quantity_a = 0;
        openswd3::test::set_inventory_name(
            openswd3::test::inventory_node(list, failing_primary ? 1U : 2U),
            std::string(20U, 'x')
        );
        auto selected = request();
        selected.selected_cell = 0U;
        const auto result = draw_legacy_battle_mode_grid_frame(
            fixture.state, fixture.bindings(), fixture.port, selected
        );
        test.expect_true(
            result.status ==
                    LegacyBattleModeGridFrameStatus::group_a_actor_typed_stop &&
                result.resource_query_failure ==
                    LegacyBattleActorListQueryStatus::list_text_typed_stop &&
                fixture.state.primary_count == (failing_primary ? 0U : 1U) &&
                fixture.state.group_slot_count == 0U &&
                fixture.panel_row_limit == (failing_primary ? 0xFFFFU : 1U) &&
                list.resource_head_token ==
                    (failing_primary ? 0x76000010U : 0x76000020U) &&
                list.selected_resource_token ==
                    (failing_primary ? 0xCAFEBABEU : 0x76000020U) &&
                result.text_draw_calls == (failing_primary ? 1U : 2U) &&
                result.font_calls == (failing_primary ? 3U : 4U) &&
                fixture.state.secondary_count == (failing_primary ? 0U : 1U) &&
                fixture.selection_input_gate == 0U,
            "real primary and group text capacity failures keep their published resource, count and drawing prefixes"
        );
    }
}

void test_mode_grid_resource_count(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (u32 gate_case = 0U; gate_case < 4U; ++gate_case) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x85000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                    {.token = root + 0x10U,
                     .next_token = root + 0x20U,
                     .resource_id = 0x0300U,
                     .secondary_quantity = 2,
                     .name = "PRIMARY",
                     .category_mask = 0x08000000U,
                     .mode_flags = 1U},
                    {.token = root + 0x20U,
                     .resource_id =
                         static_cast<u16>(gate_case == 1U ? 0x0300U : 0x0222U),
                     .secondary_quantity = 1,
                     .tertiary_quantity = 2,
                     .name = "GROUP",
                     .category_mask = gate_case == 0U ? 0U : 0x88000000U,
                     .mode_flags = static_cast<u8>(gate_case == 2U ? 2U : 4U)},
                }
            );

            const auto* original_storage = openswd3::test::inventory_data(list);
            auto selected = request();
            selected.selected_cell = 0U;
            const auto result = draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, selected
            );
            const bool included = gate_case == 3U;
            const auto queried_rows =
                std::ranges::count_if(result.cells, [](const auto& cell) {
                    return cell.queried_secondary;
                });
            test.expect_true(
                result.status == LegacyBattleModeGridFrameStatus::completed &&
                    result.resource_query_failure ==
                        LegacyBattleActorListQueryStatus::completed &&
                    fixture.state.primary_count == 2U &&
                    fixture.state.secondary_count == (included ? 3U : 0U) &&
                    fixture.panel_row_limit == (included ? 5U : 2U) &&
                    queried_rows == (included ? 3 : 0) &&
                    fixture.state.group_slot_count == (included ? 3U : 0U) &&
                    list.resource_head_token ==
                        (included ? root + 0x20U : root) &&
                    list.selected_resource_token ==
                        (included ? root + 0x20U : 0xCAFEBABEU) &&
                    list.next_resource_head_token == root &&
                    openswd3::test::inventory_data(list) == original_storage &&
                    openswd3::test::inventory_nodes(list).size() == 3U &&
                    std::ranges::all_of(
                        fixture.port.calls,
                        [](const Request& call) {
                            return call.call == Call::configure_font_mode ||
                                call.call == Call::configure_font_style ||
                                call.call == Call::configure_font_width ||
                                call.call == Call::draw_text;
                        }
                    ),
                "every actual party slot uses real count eligibility for secondary rows and the final panel WORD with no query forwarded to its font port"
            );
        }

        for (const bool partial_count : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            fixture.selection_input_gate = 0xFACE0001U;
            fixture.target_argument = 0xABCDEF12U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x86000000U + actor_index * 0x100U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                    {.token = root + 0x10U,
                     .next_token = partial_count ? root + 0x20U : 0xFEDCBA98U,
                     .resource_id = 0x0300U,
                     .secondary_quantity = 1,
                     .name = "PRIMARY",
                     .category_mask = 0xFFFFFFFFU,
                     .mode_flags = 1U},
                    {.token = root + 0x20U,
                     .next_token = 0xFEDCBA98U,
                     .resource_id = 0x0222U,
                     .secondary_quantity = 3,
                     .tertiary_quantity = 4,
                     .name = std::string(100U, 'n'),
                     .category_mask = 0x08000000U,
                     .mode_flags = 4U},
                }
            );

            const auto* original_storage = openswd3::test::inventory_data(list);
            const auto result = draw_legacy_battle_mode_grid_frame(
                fixture.state, fixture.bindings(), fixture.port, request()
            );
            test.expect_true(
                result.status ==
                        LegacyBattleModeGridFrameStatus::
                            group_a_actor_typed_stop &&
                    result.resource_query_failure ==
                        LegacyBattleActorListQueryStatus::
                            resource_node_typed_stop &&
                    fixture.state.primary_count == 1U &&
                    fixture.state.secondary_count ==
                        (partial_count ? 7U : 0U) &&
                    fixture.state.group_slot_count == 0U &&
                    fixture.state.row_text == text("PRIMARY") &&
                    fixture.panel_row_limit == 1U &&
                    list.resource_head_token == 0xFEDCBA98U &&
                    list.next_resource_head_token == root &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    openswd3::test::inventory_data(list) == original_storage &&
                    openswd3::test::inventory_nodes(list).size() == 3U &&
                    result.text_draw_calls == 1U && result.font_calls == 3U &&
                    result.selection_rectangle_calls == 0U &&
                    fixture.selection_input_gate == 0xFACE0001U &&
                    fixture.target_argument == 0xABCDEF12U &&
                    std::ranges::all_of(
                        result.cells,
                        [](const auto& cell) {
                            return cell.cell == 0U && !cell.queried_secondary;
                        }
                    ),
                "each real count failure retains the primary output and partial WORD while blocking root republication, row drawing and target changes"
            );
        }
    }
}

}  // namespace

void test_battle_mode_grid_frame(openswd3::test::Context& test) {
    test_mode_grid_resource_display(test);
    test_mode_grid_resource_count(test);
    for (u32 actor_index = 0U; actor_index < 10U; ++actor_index) {
        for (const bool has_secondary_quantity : {false, true}) {
            Fixture fixture;
            fixture.queued_actor_code = actor_index + 8U;
            auto& list = fixture.party[actor_index].actor_list;
            const u32 root = 0x80000000U + actor_index * 0x100U;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token = root;
            list.selected_resource_token = 0xCAFEBABEU;
            fixture.inventories.assign(
                list,
                {
                    {.token = root, .next_token = root + 0x10U, .name = {}},
                    {.token = root + 0x10U,
                     .next_token = root + 0x20U,
                     .resource_id = 0x0300U,
                     .secondary_quantity = 1,
                     .name = "primary",
                     .mode_flags = 0x01U},
                    {.token = root + 0x20U,
                     .resource_id = 0x0222U,
                     .secondary_quantity = static_cast<openswd3::compat::i16>(
                         has_secondary_quantity ? 1 : 0
                     ),
                     .name = "secondary",
                     .category_mask = 0x08000000U,
                     .mode_flags = 0x01U},
                }
            );

            auto bindings = fixture.bindings();
            auto selected = request();
            selected.selected_cell = 0U;
            const auto result =
                openswd3::battle::draw_legacy_battle_mode_grid_frame(
                    fixture.state, bindings, fixture.port, selected
                );
            test.expect_true(
                result.status ==
                        openswd3::battle::LegacyBattleModeGridFrameStatus::
                            completed &&
                    fixture.state.primary_count == 1U &&
                    fixture.state.secondary_count ==
                        (has_secondary_quantity ? 1U : 0U) &&
                    list.resource_head_token ==
                        (has_secondary_quantity ? root + 0x20U : root) &&
                    list.next_resource_head_token == root &&
                    list.selected_resource_token ==
                        (has_secondary_quantity ? root + 0x20U : 0xCAFEBABEU) &&
                    openswd3::test::inventory_nodes(list).size() == 3U,
                "each mode-grid actor republishes its root between real counts and retains the later selected secondary node"
            );
        }
    }

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
                result.return_edx == 0x33333333U &&
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
        const auto draws = fixture.port.calls_of(Call::draw_text);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                result.panel_action_update_calls == 1U &&
                result.panel_rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U && result.font_calls == 15U &&
                fixture.state.secondary_count == 4U &&
                result.text_copy_calls == 4U && result.text_draw_calls == 12U &&
                result.selection_rectangle_calls == 1U &&
                fixture.panel_row_limit == 6U &&
                fixture.selection_input_gate == 1U &&
                fixture.target_argument == 2U && result.selected_page == 2U &&
                result.selected_group_index == 1U,
            "mode grid completes its panel, ten cells, selected overlay and total-count publication"
        );
        test.expect_true(
            fixture.state.primary_count == 2U &&
                fixture.state.group_slot_count == 2U &&
                result.cells[2U].row_text == result.cells[3U].row_text &&
                result.cells[4U].row_text == result.cells[5U].row_text &&
                fixture.party[0U].actor_list.resource_head_token ==
                    0x76000030U &&
                fixture.party[0U].actor_list.selected_resource_token ==
                    0x76000030U,
            "mode grid repeats each actual group name for its quantity and retains the last queried node"
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
                fixture.panel_row_limit == 0xFFFFU &&
                fixture.state.primary_count == 0U &&
                fixture.state.row_text[0U] == 0xFFU,
            "mode grid invalid group-A code stops at the first primary query after the full panel prefix"
        );
    }

    {
        Fixture fixture;
        auto& list = fixture.party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        fixture.inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 0x0300U,
                 .secondary_quantity = 1,
                 .name = "ONE",
                 .mode_flags = 0x01U},
            }
        );

        fixture.port.on_call = [&fixture](const Request& call) {
            if (call.call == Call::configure_font_width) {
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
        auto& list = fixture.party[0U].actor_list;
        list.next_resource_head_token = 0x76000000U;
        fixture.inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 0x0222U,
                 .secondary_quantity = 1,
                 .name = "SECOND",
                 .category_mask = 0x08000000U,
                 .mode_flags = 0x01U},
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
        auto& actor_list = fixture.party[0U].actor_list;
        actor_list.next_resource_head_token = 0x78000000U;
        fixture.inventories.assign(
            actor_list,
            {
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
            }
        );
        const auto result =
            openswd3::battle::draw_legacy_battle_mode_grid_frame(
                fixture.state, bindings, fixture.port, request()
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleModeGridFrameStatus::
                        completed &&
                fixture.state.primary_count == 2U &&
                result.resource_query_failure ==
                    openswd3::battle::LegacyBattleActorListQueryStatus::
                        completed &&
                fixture.state.secondary_count == 2U &&
                fixture.state.group_slot_count == 2U &&
                actor_list.resource_head_token == 0x78000020U &&
                fixture.panel_row_limit == 4U,
            "mode grid production queries and resource-head refreshes use the startup party typed owner"
        );
        const auto primary_text = text("PRIMARY");
        const auto group_text = text("GROUP");
        test.expect_true(
            starts_with(result.cells[0U].row_text, primary_text) &&
                starts_with(result.cells[1U].row_text, primary_text) &&
                starts_with(
                    result.cells[2U].row_text,
                    std::span<const u8>(group_text).first(6U)
                ) &&
                starts_with(
                    result.cells[3U].row_text,
                    std::span<const u8>(group_text).first(6U)
                ) &&
                result.cells[2U].row_text[6U] == static_cast<u8>('Y') &&
                actor_list.selected_resource_token == 0x78000020U,
            "mode grid production preserves primary rows and repeats the typed group label for its returned slot count"
        );
    }
}
