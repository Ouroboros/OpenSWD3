#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_party_item_definition.hpp"
#include "test.hpp"

#include <array>
#include <utility>
#include <vector>

namespace {

using openswd3::battle::LegacyBattlePartyItemDefinitionCall;
using openswd3::battle::LegacyBattlePartyItemDefinitionCallReply;
using openswd3::battle::LegacyBattlePartyItemDefinitionCallRequest;
using openswd3::battle::LegacyBattlePartyItemDefinitionPath;
using openswd3::battle::LegacyBattlePartyItemDefinitionPort;
using openswd3::battle::LegacyBattlePartyItemDefinitionStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldItemListState;
using openswd3::world_map::LegacyWorldItemNode;

class Port final : public LegacyBattlePartyItemDefinitionPort,
                   public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattlePartyItemDefinitionCallReply
    invoke(const LegacyBattlePartyItemDefinitionCallRequest& request) override {
        calls.push_back(request);
        auto reply = LegacyBattlePartyItemDefinitionCallReply{
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        };
        if (request.call ==
            LegacyBattlePartyItemDefinitionCall::allocate_item_node) {
            reply.eax = allocation_token;
            reply.allocation_words = allocation_words;
            reply.allocation_accessible_bytes = allocation_accessible_bytes;
        } else if (
            request.call == LegacyBattlePartyItemDefinitionCall::copy_caption
        ) {
            reply.eax = copy_eax;
            reply.ecx = copy_ecx;
            reply.edx = copy_edx;
        } else {
            reply.eax = diagnostic_eax;
            reply.ecx = diagnostic_ecx;
            reply.edx = diagnostic_edx;
        }
        reply.typed_stop =
            typed_stop_enabled && request.call == typed_stop_call;
        return reply;
    }

    u32 allocation_token{0x73000000U};
    u32 allocation_accessible_bytes{
        openswd3::world_map::kLegacyWorldItemNodeBytes
    };
    openswd3::battle::LegacyBattlePartyItemAllocationWords allocation_words{};
    u32 copy_eax{0x0053C154U};
    u32 copy_ecx{0xC0DEC0DEU};
    u32 copy_edx{0xD00DD00DU};
    u32 diagnostic_eax{0x11111111U};
    u32 diagnostic_ecx{0x22222222U};
    u32 diagnostic_edx{0x33333333U};
    bool typed_stop_enabled{};
    LegacyBattlePartyItemDefinitionCall typed_stop_call{
        LegacyBattlePartyItemDefinitionCall::report_zero_item
    };
    std::vector<LegacyBattlePartyItemDefinitionCallRequest> calls;
};

[[nodiscard]] LegacyWorldItemNode& append_node(
    LegacyWorldItemListState& state,
    const std::size_t party,
    const u32 token,
    const u16 item_id
) {
    auto& list = *state.party_item_lists[party];
    LegacyWorldItemNode node;
    node.item_id = item_id;
    node.legacy_token = token;
    list.nodes.push_back(std::move(node));
    return list.nodes.back();
}

void test_existing_head_and_successor(openswd3::test::Context& test) {
    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[0U];
        list.sentinel.item_id = 41U;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {
                    .party_index = 0x12340000U,
                    .item_id = 0xAAAA0029U,
                    .entry_eax = 0x10101010U,
                    .entry_ecx = 0x20202020U,
                    .entry_edx = 0x30303030U,
                }
            );
        test.expect_true(
            result.status == LegacyBattlePartyItemDefinitionStatus::completed &&
                result.path ==
                    LegacyBattlePartyItemDefinitionPath::existing_head &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x30303030U &&
                result.matched_token == list.sentinel.legacy_token &&
                result.item_key_reads == 1U && result.head_writes == 0U &&
                result.head_restored && port.calls.empty(),
            "0x477BD0 compares the low-word key against the current zero-based party head before traversing"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[0U];
        auto& first = append_node(state, 0U, 0x71000100U, 7U);
        auto& second = append_node(state, 0U, 0x71000200U, 9U);
        list.sentinel.legacy_next_token = first.legacy_token;
        first.legacy_next_token = second.legacy_token;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {
                    .party_index = 0U,
                    .item_id = 9U,
                    .entry_edx = 0xA5A5A5A5U,
                }
            );
        test.expect_true(
            result.status == LegacyBattlePartyItemDefinitionStatus::completed &&
                result.path ==
                    LegacyBattlePartyItemDefinitionPath::existing_successor &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == first.legacy_token &&
                result.traversed_nodes == 2U && result.head_writes == 2U &&
                !result.head_restored &&
                list.legacy_head_token == second.legacy_token &&
                port.calls.empty(),
            "an existing successor returns zero without executing the 0x477C84 head restoration"
        );
    }
}

void test_reserved_miss_and_append(openswd3::test::Context& test) {
    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[1U];
        auto& tail = append_node(state, 1U, 0x71001000U, 5U);
        list.sentinel.legacy_next_token = tail.legacy_token;
        const u32 original = list.legacy_head_token;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {.party_index = 1U, .item_id = 0x8000U}
            );
        test.expect_true(
            result.status == LegacyBattlePartyItemDefinitionStatus::completed &&
                result.path ==
                    LegacyBattlePartyItemDefinitionPath::missing_reserved &&
                result.return_eax == 1U && result.return_ecx == 1U &&
                result.return_edx == tail.legacy_token &&
                result.head_restore_writes == 1U && result.head_restored &&
                list.legacy_head_token == original && port.calls.empty(),
            "the signed 0x8000 missing key skips allocation and restores the original party head"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        port.definition[0U] = 0x41U;
        port.definition[1U] = 0x42U;
        port.definition[2U] = 0U;
        port.definition_description = {0x58U, 0U};
        auto& list = *state.party_item_lists[2U];
        const u32 original = list.legacy_head_token;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {
                    .party_index = 2U,
                    .item_id = 1501U,
                    .entry_eax = 0xAAAAAAAAU,
                    .entry_ecx = 0xBBBBBBBBU,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        const auto& node = list.nodes.back();
        test.expect_true(
            result.status == LegacyBattlePartyItemDefinitionStatus::completed &&
                result.path == LegacyBattlePartyItemDefinitionPath::appended &&
                result.return_eax == 1U && result.return_ecx == port.copy_ecx &&
                result.return_edx == port.copy_edx &&
                result.allocation_calls == 1U &&
                result.definition_load_calls == 1U &&
                result.caption_copy_calls == 1U &&
                result.cleared_dwords == 44U &&
                result.chain_link_writes == 1U &&
                result.head_restore_writes == 1U && result.head_restored &&
                list.legacy_head_token == original && list.nodes.size() == 1U &&
                list.sentinel.legacy_next_token == port.allocation_token &&
                node.legacy_token == port.allocation_token &&
                node.item_id == 1501U &&
                node.definition_snapshot[0U] == 0x41U &&
                node.definition_snapshot[1U] == 0x42U &&
                node.legacy_description_token != 0U && caption[0U] == 0x41U &&
                caption[1U] == 0x42U && caption[2U] == 0U &&
                port.requested_definition_ids == std::vector<u32>{1501U} &&
                port.calls.size() == 2U &&
                port.calls[0U].call ==
                    LegacyBattlePartyItemDefinitionCall::allocate_item_node &&
                port.calls[1U].call ==
                    LegacyBattlePartyItemDefinitionCall::copy_caption,
            "a missing ordinary item is linked, cleared, loaded from MON, copied to the shared caption, then restores the head"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        caption.fill(0xCCU);
        port.open_succeeds = false;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 77U}
            );
        test.expect_true(
            result.status == LegacyBattlePartyItemDefinitionStatus::completed &&
                result.definition_load.status ==
                    openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                        open_failed &&
                result.return_eax == 1U && result.head_restored &&
                caption[0U] == 0U,
            "MON open failure remains a normal append path and copies the cleared name"
        );
    }
}

void test_diagnostic_and_owner_access_stops(openswd3::test::Context& test) {
    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        port.typed_stop_enabled = true;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {
                    .party_index = 0U,
                    .item_id = 0U,
                    .window_token = 0x44556677U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        diagnostic_typed_stop &&
                result.diagnostic_calls == 1U && result.item_key_reads == 0U &&
                port.calls.size() == 1U &&
                port.calls[0U].window_token == 0x44556677U &&
                port.calls[0U].text_token == 0x004A7D38U &&
                port.calls[0U].source_line == 0x50AU,
            "zero item reporting stops before the party head array when the diagnostic traps"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {.party_index = 4U, .item_id = 1U, .entry_eax = 0x12345678U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        party_index_typed_stop &&
                result.stopped_address == 0x004A94A0U &&
                result.return_eax == 0x12345678U && result.return_ecx == 4U,
            "an out-of-range low-word party index stops at the original array load"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        state.party_item_lists[0U].reset();
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 1U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        item_node_access_typed_stop &&
                result.stopped_token == 0U && result.stopped_offset == 4U &&
                result.return_eax == 0U,
            "a null party head stops at the unconditional initial plus-four key access"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[0U];
        list.sentinel.legacy_next_token = 0x7BAD0000U;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 1U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        item_node_access_typed_stop &&
                result.stopped_token == 0x7BAD0000U &&
                result.stopped_offset == 4U &&
                list.legacy_head_token == 0x7BAD0000U && !result.head_restored,
            "an unmapped successor stops only after publishing it as the global party head"
        );
    }
}

void test_external_call_and_host_stops(openswd3::test::Context& test) {
    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        port.definition[0U] = 0x5AU;
        port.definition[1U] = 0U;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 0U}
            );
        test.expect_true(
            result.status == LegacyBattlePartyItemDefinitionStatus::completed &&
                result.path == LegacyBattlePartyItemDefinitionPath::appended &&
                result.diagnostic_calls == 1U &&
                result.allocation_calls == 1U &&
                result.caption_copy_calls == 1U && result.return_eax == 1U &&
                result.head_restored && port.calls.size() == 3U &&
                port.calls[0U].call ==
                    LegacyBattlePartyItemDefinitionCall::report_zero_item &&
                port.calls[1U].call ==
                    LegacyBattlePartyItemDefinitionCall::allocate_item_node &&
                port.calls[2U].call ==
                    LegacyBattlePartyItemDefinitionCall::copy_caption &&
                port.requested_definition_ids == std::vector<u32>{0U} &&
                caption[0U] == 0x5AU && caption[1U] == 0U,
            "zero item reporting normally returns and the original path continues through allocation, MON, and caption copy"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        port.typed_stop_enabled = true;
        port.typed_stop_call =
            LegacyBattlePartyItemDefinitionCall::allocate_item_node;
        auto& list = *state.party_item_lists[0U];
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 12U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        allocation_call_typed_stop &&
                result.allocation_calls == 1U &&
                result.chain_link_writes == 0U && list.nodes.empty() &&
                list.sentinel.legacy_next_token == 0U && result.head_restored,
            "an allocator trap stops before the tail link write"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[0U];
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {
                    .party_index = 0U,
                    .item_id = 13U,
                    .host_item_node_allocation_succeeds = false,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        host_item_allocation_typed_stop &&
                result.chain_link_writes == 1U &&
                list.sentinel.legacy_next_token == port.allocation_token &&
                list.nodes.empty() && result.cleared_dwords == 0U &&
                result.head_restored,
            "host node mapping failure is isolated after the original physical tail link publication"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        caption.fill(0xCCU);
        port.definition[0U] = 0x41U;
        port.definition[1U] = 0U;
        port.typed_stop_enabled = true;
        port.typed_stop_call =
            LegacyBattlePartyItemDefinitionCall::copy_caption;
        auto& list = *state.party_item_lists[0U];
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 14U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        caption_call_typed_stop &&
                result.caption_copy_calls == 1U &&
                result.caption_bytes_copied == 0U && caption[0U] == 0xCCU &&
                list.legacy_head_token == port.allocation_token &&
                !result.head_restored,
            "a string-copy trap preserves the loaded new head and blocks every destination byte and restoration"
        );
    }
}

void test_allocation_and_callee_stops(openswd3::test::Context& test) {
    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[0U];
        auto& tail = append_node(state, 0U, 0x71002000U, 3U);
        list.sentinel.legacy_next_token = tail.legacy_token;
        port.allocation_token = 0U;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 4U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        allocation_node_access_typed_stop &&
                result.stopped_token == 0U && result.stopped_offset == 0U &&
                result.return_eax == 0U && result.return_ecx == 44U &&
                result.return_edx == tail.legacy_token &&
                result.chain_link_writes == 1U && result.cleared_dwords == 0U &&
                list.legacy_head_token == tail.legacy_token &&
                !result.head_restored,
            "zero allocation publishes the null tail link and stops at the first rep-stosd store without restoring the head"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        auto& list = *state.party_item_lists[0U];
        port.allocation_accessible_bytes = 8U;
        port.allocation_words[2U] = 0xAABBCCDDU;
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 6U}
            );
        const auto& node = list.nodes.back();
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        allocation_node_access_typed_stop &&
                result.stopped_token == port.allocation_token &&
                result.stopped_offset == 8U && result.return_eax == 0U &&
                result.return_ecx == 42U && result.cleared_dwords == 2U &&
                node.legacy_next_token == 0U && node.item_id == 0U &&
                node.selected_count == 0U && node.quantity_a == 0xCCDDU &&
                node.quantity_b == 0xAABBU &&
                list.legacy_head_token == list.sentinel.legacy_token,
            "partial allocation accessibility preserves two completed clears and the untouched third dword"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        port.allocation_succeeds = false;
        auto& list = *state.party_item_lists[0U];
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state, caption, port, port, {.party_index = 0U, .item_id = 33U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        definition_load_typed_stop &&
                result.definition_load.status ==
                    openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                        stream_zero_typed_stop &&
                result.cleared_dwords == 44U && result.appended_nodes == 1U &&
                list.legacy_head_token == port.allocation_token &&
                list.sentinel.legacy_next_token == port.allocation_token &&
                list.nodes.back().item_id == 33U && !result.head_restored &&
                result.caption_copy_calls == 0U,
            "a MON loader stop preserves the linked and selected new node while blocking copy and restoration"
        );
    }

    {
        LegacyWorldItemListState state;
        Port port;
        std::array<u8, 24U> caption{};
        caption.fill(0xCCU);
        port.definition[0U] = 0x41U;
        port.definition[1U] = 0x42U;
        port.definition[2U] = 0x43U;
        port.definition[3U] = 0U;
        auto& list = *state.party_item_lists[0U];
        const auto result =
            openswd3::battle::prepare_legacy_battle_party_item_definition(
                state,
                caption,
                port,
                port,
                {
                    .party_index = 0U,
                    .item_id = 44U,
                    .caption_accessible_bytes = 2U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattlePartyItemDefinitionStatus::
                        caption_destination_typed_stop &&
                result.stopped_token == 0x0053C154U &&
                result.stopped_offset == 2U &&
                result.caption_bytes_copied == 2U && caption[0U] == 0x41U &&
                caption[1U] == 0x42U && caption[2U] == 0xCCU &&
                list.legacy_head_token == port.allocation_token &&
                !result.head_restored,
            "caption overflow stops after the reached byte prefix and before the final party-head restoration"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_existing_head_and_successor(test);
    test_reserved_miss_and_append(test);
    test_diagnostic_and_owner_access_stops(test);
    test_external_call_and_host_stops(test);
    test_allocation_and_callee_stops(test);
    return test.exit_code();
}
