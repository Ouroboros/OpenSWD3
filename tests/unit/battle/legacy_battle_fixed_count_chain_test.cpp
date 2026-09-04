#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"
#include "test.hpp"

#include <array>
#include <deque>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleFixedCountAllocationPort;
using openswd3::battle::LegacyBattleFixedCountAllocationReply;
using openswd3::battle::LegacyBattleFixedCountAllocationRequest;
using openswd3::battle::LegacyBattleFixedCountPath;
using openswd3::battle::LegacyBattleFixedCountStatus;
using openswd3::battle::LegacyBattleFixedCurveX87StackState;
using openswd3::battle::LegacyBattleFixedObjectState;
using openswd3::compat::u16;
using openswd3::compat::u32;

class AllocationPort : public virtual LegacyBattleFixedCountAllocationPort {
public:
    [[nodiscard]] LegacyBattleFixedCountAllocationReply
    allocate_legacy_battle_fixed_count_node(
        const LegacyBattleFixedCountAllocationRequest& request
    ) override {
        requests.push_back(request);
        if (replies.empty()) {
            return {};
        }
        const auto reply = replies.front();
        replies.pop_front();
        return reply;
    }

    std::deque<LegacyBattleFixedCountAllocationReply> replies;
    std::vector<LegacyBattleFixedCountAllocationRequest> requests;
};

class DefinitionCurvePort final
    : public AllocationPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {};

void set_definition_word(
    DefinitionCurvePort& port, const std::size_t offset, const u16 value
) noexcept {
    port.definition[offset] = static_cast<openswd3::compat::u8>(value);
    port.definition[offset + 1U] =
        static_cast<openswd3::compat::u8>(value >> 8U);
}

[[nodiscard]] u16 key(const u32 packed) noexcept {
    return static_cast<u16>(packed);
}

[[nodiscard]] u16 count(const u32 packed) noexcept {
    return static_cast<u16>(packed >> 16U);
}

void test_allocate_and_update(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    port.replies.push_back({
        .eax = 0x71001234U,
        .ecx = 0xA1A2A3A4U,
        .edx = 0xB1B2B3B4U,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 0x14U,
    });

    const auto created = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0x1234U,
            .delta = 1U,
            .entry_eax = 0xCAFEBABEU,
            .entry_ecx = 0x10203040U,
            .entry_edx = 0x50607080U,
        }
    );
    const auto& root = state.object_words[0U];
    const auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        created.status == LegacyBattleFixedCountStatus::completed &&
            created.path == LegacyBattleFixedCountPath::allocated_node &&
            created.allocation_calls == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.count_writes == 1U &&
            created.key_writes == 1U && created.root_key_increments == 1U &&
            created.return_eax == 0x71000001U &&
            created.return_ecx == 0xA1A2A3A4U && created.return_edx == 0U &&
            root[0U] == 0x71001234U && key(root[1U]) == 1U &&
            count(root[1U]) == 0U && node.legacy_token == 0x71001234U &&
            node.words[0U] == 0U && key(node.words[1U]) == 0x1234U &&
            count(node.words[1U]) == 1U && node.words[2U] == 0U &&
            node.words[3U] == 0U && node.words[4U] == 0U &&
            port.requests.size() == 1U &&
            port.requests[0U].allocation_size == 0x14U &&
            port.requests[0U].eax == 0U &&
            port.requests[0U].ecx == 0x10203040U &&
            port.requests[0U].edx == 0x50607080U,
        "missing key links the allocated record before clearing five dwords and publishing key, count, and root key"
    );

    const auto updated = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0xFFFF1234U,
            .delta = 0x00010002U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0xCCCCCCCCU,
        }
    );
    test.expect_true(
        updated.status == LegacyBattleFixedCountStatus::completed &&
            updated.path == LegacyBattleFixedCountPath::existing_node &&
            updated.matched_token == 0x71001234U &&
            updated.allocation_calls == 0U && updated.count_reads == 1U &&
            updated.count_writes == 1U && updated.return_eax == 0x71010003U &&
            updated.return_ecx == 0x00010002U &&
            updated.return_edx == 0xCCCCCCCCU && count(node.words[1U]) == 3U,
        "existing dynamic record adds the full dword delta to EAX and writes only the resulting low word count"
    );

    auto& mutable_node = state.fixed_count_nodes.front();
    mutable_node.words[1U] =
        (mutable_node.words[1U] & 0x0000FFFFU) | (0x14U << 16U);
    const auto capped = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0x1234U,
            .delta = 0xFFFFFFFFU,
            .entry_eax = 0x11112222U,
            .entry_ecx = 0x33334444U,
            .entry_edx = 0x55556666U,
        }
    );
    test.expect_true(
        capped.status == LegacyBattleFixedCountStatus::completed &&
            capped.path == LegacyBattleFixedCountPath::existing_node &&
            capped.count_writes == 0U && capped.return_eax == 0x71000014U &&
            capped.return_ecx == 0x33334444U &&
            capped.return_edx == 0x55556666U &&
            count(mutable_node.words[1U]) == 0x14U,
        "unsigned count twenty returns before loading the delta or writing the record"
    );
}

void test_root_match_and_new_delta_width(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    auto& root = state.object_words[0U];
    root[1U] = (0x13U << 16U) | 7U;

    const auto root_update =
        openswd3::battle::accumulate_legacy_battle_fixed_count(
            state,
            port,
            {
                .key = 7U,
                .delta = 1U,
                .entry_eax = 0xAABBCCDDU,
                .entry_ecx = 0x12345678U,
                .entry_edx = 0x87654321U,
            }
        );
    test.expect_true(
        root_update.status == LegacyBattleFixedCountStatus::completed &&
            root_update.path == LegacyBattleFixedCountPath::existing_root &&
            root_update.return_eax == 0xAABB0014U &&
            root_update.return_ecx == 1U &&
            root_update.return_edx == 0x87654321U && count(root[1U]) == 0x14U,
        "root participates in the first key comparison and preserves entry EAX high word on an existing match"
    );

    state = {};
    port.replies.push_back({
        .eax = 0x72004321U,
        .ecx = 0x13572468U,
        .edx = 0xFFFFFFFFU,
        .accessible_bytes = 0x14U,
    });
    const auto created = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0x2222U,
            .delta = 0xABCD0002U,
            .entry_eax = 0x11111111U,
            .entry_ecx = 0x22222222U,
            .entry_edx = 0x33333333U,
        }
    );
    const auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        created.status == LegacyBattleFixedCountStatus::completed &&
            created.return_eax == 0x72000002U &&
            created.return_ecx == 0x13572468U && created.return_edx == 0U &&
            key(node.words[1U]) == 0x2222U && count(node.words[1U]) == 2U,
        "new-record path loads only the low delta word into AX after allocator register publication"
    );
}

void test_allocation_write_stops(openswd3::test::Context& test) {
    for (u32 accessible_bytes = 0U; accessible_bytes < 0x14U;
         accessible_bytes += 4U) {
        LegacyBattleFixedObjectState state;
        AllocationPort port;
        const std::array<u32, 5> stale{
            0x11111111U,
            0x22222222U,
            0x33333333U,
            0x44444444U,
            0x55555555U,
        };
        const u32 token = 0x73000000U + accessible_bytes;
        port.replies.push_back({
            .eax = token,
            .ecx = 0x12345678U,
            .edx = 0x87654321U,
            .initial_words = stale,
            .accessible_bytes = accessible_bytes,
        });

        const auto result =
            openswd3::battle::accumulate_legacy_battle_fixed_count(
                state,
                port,
                {
                    .key = 0x3333U,
                    .delta = 1U,
                    .entry_eax = 0xAAAAAAAAU,
                    .entry_ecx = 0xBBBBBBBBU,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        const auto& root = state.object_words[0U];
        const auto& node = state.fixed_count_nodes.front();
        bool prefix_matches = true;
        const u32 cleared_words = accessible_bytes / 4U;
        for (u32 index = 0U; index < stale.size(); ++index) {
            const u32 expected = index < cleared_words ? 0U : stale[index];
            prefix_matches = prefix_matches && node.words[index] == expected;
        }
        test.expect_true(
            result.status ==
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop &&
                result.path == LegacyBattleFixedCountPath::none &&
                result.stopped_token == token &&
                result.stopped_offset == accessible_bytes &&
                result.link_writes == 1U &&
                result.dword_zero_writes == cleared_words &&
                result.return_eax == token &&
                result.return_ecx == 0x12345678U && result.return_edx == 0U &&
                root[0U] == token && key(root[1U]) == 0U && prefix_matches,
            "allocated record stops at each original dword clear after preserving the predecessor link and completed clear prefix"
        );
    }

    LegacyBattleFixedObjectState state;
    AllocationPort port;
    const auto failed = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0x4444U,
            .delta = 1U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0xCCCCCCCCU,
        }
    );
    test.expect_true(
        failed.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            failed.stopped_token == 0U && failed.stopped_offset == 0U &&
            failed.link_writes == 1U && failed.dword_zero_writes == 0U &&
            failed.return_eax == 0U && failed.return_ecx == 0U &&
            failed.return_edx == 0U && state.object_words[0U][0U] == 0U,
        "zero allocation return is linked first and stops only at the following original node write"
    );
}

void test_unmapped_chain_record_stop(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    state.object_words[0U][0U] = 0x74000000U;

    const auto result = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 1U,
            .entry_eax = 0x11112222U,
            .entry_ecx = 0x33334444U,
            .entry_edx = 0x55556666U,
        }
    );
    test.expect_true(
        result.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            result.stopped_token == 0x74000000U &&
            result.stopped_offset == 4U && result.chain_link_reads == 1U &&
            result.return_eax == 0x74000000U &&
            result.return_ecx == 0x33334444U &&
            result.return_edx == 0x55556666U && port.requests.empty(),
        "unmapped successor stops at its first original key read after publishing EAX from the predecessor link"
    );
}

void test_set_existing_records(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    auto& root = state.object_words[0U];
    root[1U] = (9U << 16U) | 7U;

    const auto root_set = openswd3::battle::set_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 7U,
            .count = 0xABCD0015U,
            .entry_eax = 0xAABBCCDDU,
            .entry_ecx = 0x11223344U,
            .entry_edx = 0x55667788U,
        }
    );
    test.expect_true(
        root_set.status == LegacyBattleFixedCountStatus::completed &&
            root_set.path == LegacyBattleFixedCountPath::existing_root &&
            root_set.count_writes == 2U && root_set.clamp_writes == 1U &&
            root_set.return_eax == 0xAABB0015U &&
            root_set.return_ecx == 0x11223344U &&
            root_set.return_edx == 0x55667788U && key(root[1U]) == 7U &&
            count(root[1U]) == 20U,
        "existing root receives the raw low word before unsigned values above twenty are overwritten with twenty"
    );

    state.fixed_count_nodes.push_back({
        .legacy_token = 0x75001234U,
        .words = {0U, (3U << 16U) | 8U, 0U, 0U, 0U},
        .accessible_bytes = 0x14U,
    });
    root[0U] = 0x75001234U;
    const auto node_set = openswd3::battle::set_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 8U,
            .count = 0x12340014U,
            .entry_eax = 0xDEADBEEFU,
            .entry_ecx = 0xCAFEBABEU,
            .entry_edx = 0x10203040U,
        }
    );
    test.expect_true(
        node_set.status == LegacyBattleFixedCountStatus::completed &&
            node_set.path == LegacyBattleFixedCountPath::existing_node &&
            node_set.chain_link_reads == 1U && node_set.count_writes == 1U &&
            node_set.clamp_writes == 0U && node_set.return_eax == 0x75000014U &&
            node_set.return_ecx == 0xCAFEBABEU &&
            node_set.return_edx == 0x10203040U &&
            count(state.fixed_count_nodes.front().words[1U]) == 20U &&
            port.requests.empty(),
        "existing dynamic record uses the successor token high word and writes an exact count of twenty without clamping"
    );
}

void test_set_allocate_and_clamp(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    port.replies.push_back({
        .eax = 0x76004321U,
        .ecx = 0xA1A2A3A4U,
        .edx = 0xB1B2B3B4U,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 0x14U,
    });

    const auto result = openswd3::battle::set_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0xFFFF3456U,
            .count = 0xABCD0019U,
            .entry_eax = 0x11112222U,
            .entry_ecx = 0x33334444U,
            .entry_edx = 0x55556666U,
        }
    );
    const auto& root = state.object_words[0U];
    const auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        result.status == LegacyBattleFixedCountStatus::completed &&
            result.path == LegacyBattleFixedCountPath::allocated_node &&
            result.allocation_calls == 1U && result.link_writes == 1U &&
            result.dword_zero_writes == 5U && result.key_writes == 1U &&
            result.count_writes == 2U && result.clamp_writes == 1U &&
            result.root_key_increments == 1U &&
            result.return_eax == 0x76000019U && result.return_ecx == 0U &&
            result.return_edx == 0xB1B2B3B4U && root[0U] == 0x76004321U &&
            key(root[1U]) == 1U && key(node.words[1U]) == 0x3456U &&
            count(node.words[1U]) == 20U && port.requests.size() == 1U &&
            port.requests[0U].eax == 0U &&
            port.requests[0U].ecx == 0x33334444U &&
            port.requests[0U].edx == 0x55556666U,
        "missing key links and clears one shared node, writes key then raw count, clamps, and increments the root word"
    );
}

void test_set_allocation_write_stops(openswd3::test::Context& test) {
    for (u32 accessible_bytes = 0U; accessible_bytes < 0x14U;
         accessible_bytes += 4U) {
        LegacyBattleFixedObjectState state;
        AllocationPort port;
        const std::array<u32, 5> stale{
            0x11111111U,
            0x22222222U,
            0x33333333U,
            0x44444444U,
            0x55555555U,
        };
        const u32 token = 0x77000000U + accessible_bytes;
        port.replies.push_back({
            .eax = token,
            .ecx = 0x12345678U,
            .edx = 0x87654321U,
            .initial_words = stale,
            .accessible_bytes = accessible_bytes,
        });

        const auto result = openswd3::battle::set_legacy_battle_fixed_count(
            state,
            port,
            {
                .key = 0x1111U,
                .count = 7U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0xBBBBBBBBU,
                .entry_edx = 0xCCCCCCCCU,
            }
        );
        const auto& node = state.fixed_count_nodes.front();
        bool prefix_matches = true;
        const u32 cleared_words = accessible_bytes / 4U;
        for (u32 index = 0U; index < stale.size(); ++index) {
            const u32 expected = index < cleared_words ? 0U : stale[index];
            prefix_matches = prefix_matches && node.words[index] == expected;
        }
        test.expect_true(
            result.status ==
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop &&
                result.stopped_token == token &&
                result.stopped_offset == accessible_bytes &&
                result.link_writes == 1U &&
                result.dword_zero_writes == cleared_words &&
                result.return_eax == token && result.return_ecx == 0U &&
                result.return_edx == 0x87654321U &&
                state.object_words[0U][0U] == token && prefix_matches,
            "set path preserves the predecessor link and each completed clear before the original allocation write fault"
        );
    }

    LegacyBattleFixedObjectState state;
    AllocationPort port;
    const auto failed = openswd3::battle::set_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 0x2222U,
            .count = 9U,
            .entry_eax = 0x11111111U,
            .entry_ecx = 0x22222222U,
            .entry_edx = 0x33333333U,
        }
    );
    test.expect_true(
        failed.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            failed.stopped_token == 0U && failed.stopped_offset == 0U &&
            failed.link_writes == 1U && failed.return_eax == 0U &&
            failed.return_ecx == 0U && failed.return_edx == 0U,
        "zero allocation is linked before the set path stops at the first new-record clear"
    );
}

void test_set_record_access_stops(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    state.object_words[0U][0U] = 0x78000000U;
    const auto unmapped = openswd3::battle::set_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 1U,
            .count = 2U,
            .entry_eax = 0x11112222U,
            .entry_ecx = 0x33334444U,
            .entry_edx = 0x55556666U,
        }
    );
    test.expect_true(
        unmapped.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            unmapped.stopped_token == 0x78000000U &&
            unmapped.stopped_offset == 4U &&
            unmapped.return_eax == 0x78000000U && port.requests.empty(),
        "set path stops at an unmapped successor key read after preserving the loaded token in EAX"
    );

    state = {};
    state.object_words[0U][0U] = 0x78000010U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x78000010U,
        .words = {0U, (3U << 16U) | 9U, 0U, 0U, 0U},
        .accessible_bytes = 7U,
    });
    const auto count_stop = openswd3::battle::set_legacy_battle_fixed_count(
        state,
        port,
        {
            .key = 9U,
            .count = 0x1234000AU,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0xCCCCCCCCU,
        }
    );
    test.expect_true(
        count_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            count_stop.path == LegacyBattleFixedCountPath::existing_node &&
            count_stop.stopped_token == 0x78000010U &&
            count_stop.stopped_offset == 6U &&
            count_stop.return_eax == 0x7800000AU &&
            count_stop.return_ecx == 0xBBBBBBBBU &&
            count_stop.return_edx == 0xCCCCCCCCU &&
            count(state.fixed_count_nodes.front().words[1U]) == 3U,
        "existing set path stops at the original count write after the key comparison and AX load"
    );
}

void test_lookup_records_and_missing(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    state.object_words[0U][1U] = (12U << 16U) | 3U;
    const auto root = openswd3::battle::lookup_legacy_battle_fixed_count(
        state,
        {
            .key = 0x12340003U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0xCAFE5678U,
        }
    );
    test.expect_true(
        root.status == LegacyBattleFixedCountStatus::completed &&
            root.path == LegacyBattleFixedCountPath::existing_root &&
            root.matched_token == 0x004B9F00U && root.key_reads == 1U &&
            root.chain_link_reads == 0U && root.count_reads == 1U &&
            root.return_eax == 12U && root.return_ecx == 0x004B9F00U &&
            root.return_edx == 0xCAFE0003U,
        "lookup compares the root first, clears EAX, replaces DX with the key, and returns the root count word"
    );

    state.object_words[0U][0U] = 0x79000000U;
    state.object_words[0U][1U] = (1U << 16U) | 2U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x79000000U,
        .words = {0U, (17U << 16U) | 9U, 0U, 0U, 0U},
    });
    const auto node = openswd3::battle::lookup_legacy_battle_fixed_count(
        state,
        {
            .key = 9U,
            .entry_eax = 0xFFFFFFFFU,
            .entry_ecx = 0xEEEEEEEEU,
            .entry_edx = 0xFACE1234U,
        }
    );
    const auto missing = openswd3::battle::lookup_legacy_battle_fixed_count(
        state,
        {
            .key = 10U,
            .entry_eax = 0xFFFFFFFFU,
            .entry_ecx = 0xEEEEEEEEU,
            .entry_edx = 0xABCD1234U,
        }
    );
    test.expect_true(
        node.status == LegacyBattleFixedCountStatus::completed &&
            node.path == LegacyBattleFixedCountPath::existing_node &&
            node.matched_token == 0x79000000U && node.key_reads == 2U &&
            node.chain_link_reads == 1U && node.count_reads == 1U &&
            node.return_eax == 17U && node.return_ecx == 0x79000000U &&
            node.return_edx == 0xFACE0009U &&
            missing.status == LegacyBattleFixedCountStatus::completed &&
            missing.path == LegacyBattleFixedCountPath::none &&
            missing.matched_token == 0U && missing.key_reads == 2U &&
            missing.chain_link_reads == 2U && missing.count_reads == 0U &&
            missing.return_eax == 0U && missing.return_ecx == 0U &&
            missing.return_edx == 0xABCD000AU,
        "lookup scans mapped successors in order and returns zero with ECX cleared when the key is absent"
    );
}

void test_lookup_record_access_stops(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    state.object_words[0U][0U] = 0x79000010U;
    state.object_words[0U][1U] = 2U;
    const auto unmapped = openswd3::battle::lookup_legacy_battle_fixed_count(
        state,
        {
            .key = 9U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0xCDEF1234U,
        }
    );
    test.expect_true(
        unmapped.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            unmapped.stopped_token == 0x79000010U &&
            unmapped.stopped_offset == 4U && unmapped.key_reads == 1U &&
            unmapped.chain_link_reads == 1U && unmapped.return_eax == 0U &&
            unmapped.return_ecx == 0x79000010U &&
            unmapped.return_edx == 0xCDEF0009U,
        "lookup stops at an unmapped successor key read after MOV ECX publishes the token"
    );

    state = {};
    state.object_words[0U][0U] = 0x79000020U;
    state.object_words[0U][1U] = 2U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x79000020U,
        .words = {0U, (3U << 16U) | 9U, 0U, 0U, 0U},
        .accessible_bytes = 7U,
    });
    const auto count_stop = openswd3::battle::lookup_legacy_battle_fixed_count(
        state,
        {
            .key = 9U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0x13572468U,
        }
    );
    const auto owner_stop = openswd3::battle::lookup_legacy_battle_fixed_count(
        state,
        {
            .owner_token = 0x79000030U,
            .key = 9U,
            .entry_eax = 0xAAAAAAAAU,
            .entry_ecx = 0xBBBBBBBBU,
            .entry_edx = 0x24681357U,
        }
    );
    test.expect_true(
        count_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            count_stop.path == LegacyBattleFixedCountPath::existing_node &&
            count_stop.matched_token == 0x79000020U &&
            count_stop.stopped_token == 0x79000020U &&
            count_stop.stopped_offset == 6U && count_stop.key_reads == 2U &&
            count_stop.chain_link_reads == 1U && count_stop.count_reads == 0U &&
            count_stop.return_eax == 0U &&
            count_stop.return_ecx == 0x79000020U &&
            count_stop.return_edx == 0x13570009U &&
            owner_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            owner_stop.stopped_token == 0x79000030U &&
            owner_stop.stopped_offset == 4U && owner_stop.key_reads == 0U &&
            owner_stop.return_eax == 0U &&
            owner_stop.return_ecx == 0x79000030U &&
            owner_stop.return_edx == 0x24680009U,
        "lookup stops at the exact count or root access after preserving the register prefix"
    );
}

void test_curve_existing_and_missing(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    auto& root = state.object_words[1U];
    root[1U] = (4U << 16U) | 7U;
    root[2U] = 0xABCD4321U;

    const auto existing = openswd3::battle::advance_legacy_battle_fixed_curve(
        state,
        port,
        {
            .key = 0xFFFF0007U,
            .maximum = 0xAAAA0005U,
            .multiplier = 0xBBBB0014U,
            .entry_eax = 0xBBBB0014U,
            .entry_ecx = 0xAAAA0005U,
            .entry_edx = 0xCCCC0007U,
        }
    );
    test.expect_true(
        existing.status == LegacyBattleFixedCountStatus::completed &&
            existing.path == LegacyBattleFixedCountPath::existing_root &&
            existing.x87_stack == LegacyBattleFixedCurveX87StackState::empty &&
            existing.matched_token == 0x004ACBA8U &&
            existing.count_writes == 2U && existing.clamp_writes == 1U &&
            existing.scale_writes == 1U && existing.truncate_calls == 2U &&
            existing.count == 5U && existing.scale == 100U &&
            existing.return_eax == 20U && existing.return_ecx == 5U &&
            existing.return_edx == 0U && count(root[1U]) == 5U &&
            root[2U] == 0xABCD0064U && port.requests.empty(),
        "existing fixed curve increments then unsigned-clamps the count and preserves the scale dword high word"
    );

    root[1U] = (0xFFFFU << 16U) | 7U;
    const auto wrapped = openswd3::battle::advance_legacy_battle_fixed_curve(
        state, port, {.key = 7U, .maximum = 2U, .multiplier = 20U}
    );
    test.expect_true(
        wrapped.status == LegacyBattleFixedCountStatus::completed &&
            wrapped.count == 0U && wrapped.scale == 0U &&
            wrapped.count_writes == 1U && wrapped.clamp_writes == 0U &&
            wrapped.return_eax == 0U && wrapped.return_ecx == 0U &&
            wrapped.return_edx == 0U && count(root[1U]) == 0U,
        "existing fixed curve preserves the original word increment wrap before the unsigned maximum comparison"
    );

    state = {};
    port.replies.push_back({
        .eax = 0x7B001234U,
        .ecx = 0x11111111U,
        .edx = 0x22222222U,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 0x14U,
    });
    const auto created = openswd3::battle::advance_legacy_battle_fixed_curve(
        state,
        port,
        {
            .key = 0xCCCC0009U,
            .maximum = 0xBBBB0003U,
            .multiplier = 0xAAAA0064U,
            .entry_eax = 0xAAAA0064U,
            .entry_ecx = 0xBBBB0003U,
            .entry_edx = 0xCCCC0009U,
        }
    );
    const auto& created_root = state.object_words[1U];
    const auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        created.status == LegacyBattleFixedCountStatus::completed &&
            created.path == LegacyBattleFixedCountPath::allocated_node &&
            created.x87_stack == LegacyBattleFixedCurveX87StackState::empty &&
            created.allocation_calls == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.key_writes == 1U &&
            created.count_writes == 1U && created.scale_writes == 1U &&
            created.root_key_increments == 1U && created.truncate_calls == 2U &&
            created.count == 1U && created.scale == 33U &&
            created.return_eax == 33U && created.return_ecx == 0U &&
            created.return_edx == 0U && created_root[0U] == 0x7B001234U &&
            key(created_root[1U]) == 1U && key(node.words[1U]) == 9U &&
            count(node.words[1U]) == 1U && key(node.words[2U]) == 33U &&
            port.requests.back().eax == 0U &&
            port.requests.back().ecx == 0xBBBB0003U &&
            port.requests.back().edx == 0xCCCC0009U,
        "missing fixed curve links and clears one node before publishing key count scale and the root key total"
    );

    state = {};
    const auto zero_maximum =
        openswd3::battle::advance_legacy_battle_fixed_curve(
            state, port, {.key = 0U, .maximum = 0U, .multiplier = 5U}
        );
    test.expect_true(
        zero_maximum.status == LegacyBattleFixedCountStatus::completed &&
            zero_maximum.path == LegacyBattleFixedCountPath::existing_root &&
            zero_maximum.count == 0U && zero_maximum.scale == 0U &&
            zero_maximum.return_eax == 0U &&
            zero_maximum.return_edx == 0x80000000U,
        "zero maximum keeps the original zero-over-zero x87 indefinite high dword while AX remains zero"
    );
}

void test_curve_access_stops(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    auto& root = state.object_words[1U];
    root[0U] = 0x7C000000U;
    root[1U] = 8U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7C000000U,
        .words = {0U, 9U, 0xAABBCCDDU, 0U, 0U},
        .accessible_bytes = 7U,
    });
    const auto count_stop = openswd3::battle::advance_legacy_battle_fixed_curve(
        state,
        port,
        {
            .key = 9U,
            .maximum = 2U,
            .multiplier = 6U,
            .entry_eax = 0xAAAA0006U,
            .entry_ecx = 0xBBBB0002U,
            .entry_edx = 0xCCCC0009U,
        }
    );
    test.expect_true(
        count_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            count_stop.path == LegacyBattleFixedCountPath::existing_node &&
            count_stop.stopped_token == 0x7C000000U &&
            count_stop.stopped_offset == 6U &&
            count_stop.return_eax == 0x7C000000U &&
            count_stop.return_ecx == 0xBBBB0002U &&
            count_stop.return_edx == 0xCCCC0009U,
        "fixed curve stops before the first inaccessible existing count increment"
    );

    state.fixed_count_nodes.front().accessible_bytes = 9U;
    const auto scale_stop = openswd3::battle::advance_legacy_battle_fixed_curve(
        state, port, {.key = 9U, .maximum = 2U, .multiplier = 6U}
    );
    test.expect_true(
        scale_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            scale_stop.path == LegacyBattleFixedCountPath::existing_node &&
            scale_stop.stopped_token == 0x7C000000U &&
            scale_stop.stopped_offset == 8U && scale_stop.count == 1U &&
            scale_stop.x87_stack ==
                LegacyBattleFixedCurveX87StackState::ratio &&
            scale_stop.truncate_calls == 1U && scale_stop.return_eax == 50U &&
            scale_stop.return_ecx == 1U && scale_stop.return_edx == 0U &&
            count(state.fixed_count_nodes.front().words[1U]) == 1U &&
            state.fixed_count_nodes.front().words[2U] == 0xAABBCCDDU,
        "fixed curve preserves the increment clamp and first x87 conversion before an inaccessible scale write"
    );

    state = {};
    port.replies.push_back({
        .eax = 0x7C001000U,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 8U,
    });
    const auto allocation_stop =
        openswd3::battle::advance_legacy_battle_fixed_curve(
            state, port, {.key = 9U, .maximum = 4U, .multiplier = 12U}
        );
    const auto& partial = state.fixed_count_nodes.front();
    test.expect_true(
        allocation_stop.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            allocation_stop.path == LegacyBattleFixedCountPath::none &&
            allocation_stop.stopped_token == 0x7C001000U &&
            allocation_stop.stopped_offset == 8U &&
            allocation_stop.link_writes == 1U &&
            allocation_stop.dword_zero_writes == 2U &&
            allocation_stop.x87_stack ==
                LegacyBattleFixedCurveX87StackState::maximum &&
            allocation_stop.return_eax == 0x7C001000U &&
            allocation_stop.return_ecx == 0U &&
            allocation_stop.return_edx == 4U &&
            state.object_words[1U][0U] == 0x7C001000U &&
            partial.words[0U] == 0U && partial.words[1U] == 0U &&
            partial.words[2U] == 0x33333333U,
        "fixed curve stops at the first inaccessible allocation clear after linking and loading the x87 maximum"
    );

    state = {};
    port.replies.push_back({
        .eax = 0x7C002000U,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 12U,
    });
    const auto post_divide_stop =
        openswd3::battle::advance_legacy_battle_fixed_curve(
            state, port, {.key = 9U, .maximum = 4U, .multiplier = 12U}
        );
    test.expect_true(
        post_divide_stop.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            post_divide_stop.stopped_token == 0x7C002000U &&
            post_divide_stop.stopped_offset == 12U &&
            post_divide_stop.dword_zero_writes == 3U &&
            post_divide_stop.x87_stack ==
                LegacyBattleFixedCurveX87StackState::ratio &&
            post_divide_stop.return_eax == 0x7C002000U &&
            post_divide_stop.return_ecx == 0U &&
            post_divide_stop.return_edx == 4U &&
            state.fixed_count_nodes.front().words[2U] == 0U &&
            state.fixed_count_nodes.front().words[3U] == 0x44444444U,
        "fixed curve preserves the completed one-over-maximum division before the inaccessible plus-twelve clear"
    );
}

void test_curve_set_existing_and_missing(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    auto& root = state.object_words[1U];
    root[1U] = (4U << 16U) | 7U;
    root[2U] = 0xABCD4321U;

    const auto existing = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
        port,
        {
            .key = 0xFFFF0007U,
            .maximum = 0xAAAA0005U,
            .count = 0xBBBB0005U,
            .entry_eax = 0xAAAA0005U,
            .entry_ecx = 0xCCCC0005U,
            .entry_edx = 0xDDDD0007U,
        }
    );
    test.expect_true(
        existing.status == LegacyBattleFixedCountStatus::completed &&
            existing.path == LegacyBattleFixedCountPath::existing_root &&
            existing.matched_token == 0x004ACBA8U &&
            existing.count_writes == 2U && existing.clamp_writes == 1U &&
            existing.scale_writes == 1U && existing.truncate_calls == 1U &&
            existing.count == 5U && existing.scale == 100U &&
            existing.return_eax == 100U && existing.return_ecx == 5U &&
            existing.return_edx == 0U && count(root[1U]) == 5U &&
            root[2U] == 0xABCD0064U && port.requests.empty(),
        "existing fixed curve set writes the requested word before the inclusive maximum clamp and percentage"
    );

    state = {};
    state.object_words[1U][1U] = 0xFFFFU;
    port.replies.push_back({
        .eax = 0x7D001234U,
        .ecx = 0xABCD1234U,
        .edx = 0xEEEEEEEEU,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 0x14U,
    });
    const auto created = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
        port,
        {
            .key = 0xEEEE0009U,
            .maximum = 0xAAAA0003U,
            .count = 0xBBBB0001U,
            .entry_eax = 0xAAAA0003U,
            .entry_ecx = 0xBBBB0001U,
            .entry_edx = 0xEEEE0009U,
        }
    );
    const auto& created_root = state.object_words[1U];
    auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        created.status == LegacyBattleFixedCountStatus::completed &&
            created.path == LegacyBattleFixedCountPath::allocated_node &&
            created.allocation_calls == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.key_writes == 1U &&
            created.count_writes == 1U && created.clamp_writes == 0U &&
            created.scale_writes == 1U && created.root_key_increments == 1U &&
            created.truncate_calls == 1U && created.count == 1U &&
            created.scale == 33U && created.return_eax == 33U &&
            created.return_ecx == 1U && created.return_edx == 0U &&
            created_root[0U] == 0x7D001234U && key(created_root[1U]) == 0U &&
            key(node.words[1U]) == 9U && count(node.words[1U]) == 1U &&
            key(node.words[2U]) == 33U && node.words[3U] == 0U &&
            node.words[4U] == 0U && port.requests.back().eax == 0U &&
            port.requests.back().ecx == 0xBBBB0001U &&
            port.requests.back().edx == 0xEEEE0009U,
        "missing fixed curve set links and clears one node before publishing key count percentage and the wrapped root total"
    );

    node.words[2U] = 0xABCD0021U;
    const auto existing_node = openswd3::battle::set_legacy_battle_fixed_curve(
        state, port, {.key = 9U, .maximum = 4U, .count = 2U}
    );
    test.expect_true(
        existing_node.status == LegacyBattleFixedCountStatus::completed &&
            existing_node.path == LegacyBattleFixedCountPath::existing_node &&
            existing_node.chain_link_reads == 1U &&
            existing_node.allocation_calls == 0U && existing_node.count == 2U &&
            existing_node.scale == 50U && existing_node.return_eax == 50U &&
            existing_node.return_ecx == 2U && existing_node.return_edx == 0U &&
            key(created_root[1U]) == 0U && count(node.words[1U]) == 2U &&
            node.words[2U] == 0xABCD0032U && port.requests.size() == 1U,
        "existing dynamic fixed curve set follows one next link without allocating or incrementing the root"
    );

    state = {};
    const auto zero_maximum = openswd3::battle::set_legacy_battle_fixed_curve(
        state, port, {.key = 0U, .maximum = 0U, .count = 9U}
    );
    test.expect_true(
        zero_maximum.status == LegacyBattleFixedCountStatus::completed &&
            zero_maximum.path == LegacyBattleFixedCountPath::existing_root &&
            zero_maximum.count == 0U && zero_maximum.scale == 0U &&
            zero_maximum.count_writes == 2U &&
            zero_maximum.clamp_writes == 1U && zero_maximum.return_eax == 0U &&
            zero_maximum.return_ecx == 0U &&
            zero_maximum.return_edx == 0x80000000U,
        "zero maximum keeps the set then clamp and zero-over-zero x87 integer indefinite"
    );
}

void test_curve_lookup_records_and_missing(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    auto& root = state.object_words[1U];
    root[1U] = 0xAAAA1234U;
    root[2U] = 0xBBBB5678U;

    const auto root_hit = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state,
        {
            .key = 0xDEAD1234U,
            .entry_eax = 0x11111111U,
            .entry_ecx = 0xCCCCFFFFU,
            .entry_edx = 0xDDDDDDDDU,
        }
    );
    test.expect_true(
        root_hit.status == LegacyBattleFixedCountStatus::completed &&
            root_hit.value == 0x5678U &&
            root_hit.matched_token == 0x004ACBA8U && root_hit.key_reads == 1U &&
            root_hit.chain_link_reads == 0U && root_hit.value_reads == 1U &&
            root_hit.return_eax == 0x004A5678U &&
            root_hit.return_ecx == 0xCCCC1234U &&
            root_hit.return_edx == 0xDDDDDDDDU,
        "fixed curve lookup truncates the key and preserves the root token high word on a root hit"
    );

    root[0U] = 0x7E001234U;
    root[1U] = 1U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7E001234U,
        .words = {0U, 0xBBBB2345U, 0xCCCC4321U, 0U, 0U},
        .accessible_bytes = 0x14U,
    });
    const auto node_hit = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state,
        {
            .key = 0xFFFF2345U,
            .entry_ecx = 0xABCD0000U,
            .entry_edx = 0x12345678U,
        }
    );
    test.expect_true(
        node_hit.status == LegacyBattleFixedCountStatus::completed &&
            node_hit.value == 0x4321U &&
            node_hit.matched_token == 0x7E001234U && node_hit.key_reads == 2U &&
            node_hit.chain_link_reads == 1U && node_hit.value_reads == 1U &&
            node_hit.return_eax == 0x7E004321U &&
            node_hit.return_ecx == 0xABCD2345U &&
            node_hit.return_edx == 0x12345678U,
        "fixed curve lookup follows one link and preserves the matched dynamic token high word"
    );

    const auto missing = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state,
        {
            .key = 0xAAAA7777U,
            .entry_eax = 0xFFFFFFFFU,
            .entry_ecx = 0xBCDE1111U,
            .entry_edx = 0x87654321U,
        }
    );
    test.expect_true(
        missing.status == LegacyBattleFixedCountStatus::completed &&
            missing.value == 0U && missing.matched_token == 0U &&
            missing.key_reads == 2U && missing.chain_link_reads == 2U &&
            missing.value_reads == 0U && missing.return_eax == 0U &&
            missing.return_ecx == 0xBCDE7777U &&
            missing.return_edx == 0x87654321U,
        "missing fixed curve lookup returns zero only after scanning the existing chain"
    );
}

void test_curve_lookup_access_stops(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7F001234U,
        .words = {0U, 0x11112222U, 0x33334444U, 0U, 0U},
        .accessible_bytes = 5U,
    });
    const auto key_stop = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state,
        {
            .owner_token = 0x7F001234U,
            .key = 0xAAAA2222U,
            .entry_eax = 0xBBBBBBBBU,
            .entry_ecx = 0xCCCC0000U,
            .entry_edx = 0xDDDDDDDDU,
        }
    );
    test.expect_true(
        key_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            key_stop.stopped_token == 0x7F001234U &&
            key_stop.stopped_offset == 4U && key_stop.key_reads == 0U &&
            key_stop.chain_link_reads == 0U && key_stop.value_reads == 0U &&
            key_stop.return_eax == 0x7F001234U &&
            key_stop.return_ecx == 0xCCCC2222U &&
            key_stop.return_edx == 0xDDDDDDDDU,
        "fixed curve lookup stops at the first inaccessible owner key read after loading EAX and CX"
    );

    state.fixed_count_nodes.front().accessible_bytes = 8U;
    const auto value_stop = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state,
        {
            .owner_token = 0x7F001234U,
            .key = 0x2222U,
            .entry_ecx = 0xEEEE0000U,
            .entry_edx = 0xFFFFFFFFU,
        }
    );
    test.expect_true(
        value_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            value_stop.stopped_token == 0x7F001234U &&
            value_stop.stopped_offset == 8U && value_stop.key_reads == 1U &&
            value_stop.chain_link_reads == 0U && value_stop.value_reads == 0U &&
            value_stop.return_eax == 0x7F001234U &&
            value_stop.return_ecx == 0xEEEE2222U &&
            value_stop.return_edx == 0xFFFFFFFFU,
        "fixed curve lookup stops at the inaccessible hit value before replacing AX"
    );

    state = {};
    state.object_words[1U][0U] = 0x7F00ABCDU;
    state.object_words[1U][1U] = 1U;
    const auto next_key_stop =
        openswd3::battle::lookup_legacy_battle_fixed_curve(
            state,
            {
                .key = 2U,
                .entry_ecx = 0x12340000U,
                .entry_edx = 0x56789ABCU,
            }
        );
    test.expect_true(
        next_key_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            next_key_stop.stopped_token == 0x7F00ABCDU &&
            next_key_stop.stopped_offset == 4U &&
            next_key_stop.key_reads == 1U &&
            next_key_stop.chain_link_reads == 1U &&
            next_key_stop.return_eax == 0x7F00ABCDU &&
            next_key_stop.return_ecx == 0x12340002U &&
            next_key_stop.return_edx == 0x56789ABCU,
        "fixed curve lookup stops at the real key read of an unmapped linked token"
    );
}

void test_definition_curve_existing_and_locked(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    DefinitionCurvePort port;
    set_definition_word(port, 0x44U, 10U);
    port.definition_description = {'x', 0U};

    auto& root = state.object_words[2U];
    root[1U] = 0xAAAA0000U;
    root[2U] = 0x00001234U;
    const auto root_set =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state,
            port,
            port,
            {
                .key = 0U,
                .count = 5U,
                .entry_eax = 0x11111111U,
                .entry_ecx = 0x22222222U,
                .entry_edx = 0x33333333U,
            }
        );
    const auto& scratch = port.legacy_battle_mon_definition_scratch();
    test.expect_true(
        root_set.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    completed &&
            root_set.path == LegacyBattleFixedCountPath::existing_root &&
            root_set.definition_load_calls == 1U &&
            root_set.definition_cleanup_calls == 1U &&
            root_set.definition_text_release_calls == 1U &&
            root_set.root_count_reads == 1U && root_set.key_reads == 1U &&
            root_set.lock_reads == 1U && !root_set.locked &&
            root_set.maximum == 10U && root_set.count == 5U &&
            root_set.scale == 50U && root_set.count_writes == 1U &&
            root_set.scale_writes == 1U && root_set.return_eax == 1U &&
            root_set.return_ecx == 5U && root_set.return_edx == 0U &&
            key(root[1U]) == 0U && count(root[1U]) == 5U &&
            root[2U] == 0x00000032U &&
            port.requested_definition_ids == std::vector<u32>{0U} &&
            port.definition_text_release_calls == 1U &&
            port.legacy_battle_mon_database_state()
                    .definition_text_allocation_bytes == 2U &&
            scratch[0xA0U] == 0U && scratch[0xA1U] == 0U &&
            scratch[0xA2U] == 0U && scratch[0xA3U] == 0U &&
            port.legacy_battle_mon_definition_scratch_description().empty(),
        "definition-backed curve loads and releases transient text, preserves its allocation counter bug, and updates the empty root key-zero alias"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    set_definition_word(port, 0x44U, 20U);
    state.object_words[2U][0U] = 0x7F100000U;
    state.object_words[2U][1U] = 1U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7F100000U,
        .words = {0U, (7U << 16U) | 9U, 0x00011234U, 0U, 0U},
        .accessible_bytes = 0x14U,
    });
    const auto locked =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state,
            port,
            port,
            {
                .key = 9U,
                .count = 17U,
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0xBBBBBBBBU,
                .entry_edx = 0xCCCCCCCCU,
            }
        );
    const auto& locked_node = state.fixed_count_nodes.front();
    test.expect_true(
        locked.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    completed &&
            locked.path == LegacyBattleFixedCountPath::existing_node &&
            locked.matched_token == 0x7F100000U && locked.locked &&
            locked.root_count_reads == 1U && locked.chain_link_reads == 1U &&
            locked.key_reads == 1U && locked.lock_reads == 1U &&
            locked.count_writes == 0U && locked.scale_writes == 0U &&
            locked.maximum == 0U && locked.return_eax == 1U &&
            count(locked_node.words[1U]) == 7U &&
            locked_node.words[2U] == 0x00011234U,
        "a nonempty root starts at its first node and a nonzero plus-ten word returns one after definition cleanup without reading the maximum"
    );
}

void test_definition_curve_allocate_and_clamp(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    DefinitionCurvePort port;
    set_definition_word(port, 0x44U, 3U);
    port.replies.push_back({
        .eax = 0x7F200000U,
        .ecx = 0xAABBCCDDU,
        .edx = 0x11223344U,
        .initial_words =
            {0x11111111U, 0x22222222U, 0x33333333U, 0x44444444U, 0x55555555U},
        .accessible_bytes = 0x14U,
    });

    const auto created =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state,
            port,
            port,
            {
                .key = 0xFFFF0009U,
                .count = 0xAAAA0007U,
                .entry_eax = 0xAAAA0007U,
                .entry_ecx = 0xFFFF0009U,
                .entry_edx = 0x12340002U,
            }
        );
    const auto& root = state.object_words[2U];
    const auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        created.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    completed &&
            created.path == LegacyBattleFixedCountPath::allocated_node &&
            created.allocation_calls == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.key_writes == 1U &&
            created.count_writes == 2U && created.clamp_writes == 1U &&
            created.scale_writes == 1U && created.root_count_increments == 1U &&
            created.maximum == 3U && created.count == 3U &&
            created.scale == 100U && created.return_eax == 1U &&
            created.return_ecx == 3U && created.return_edx == 0U &&
            root[0U] == 0x7F200000U && key(root[1U]) == 1U &&
            node.legacy_token == 0x7F200000U && node.words[0U] == 0U &&
            key(node.words[1U]) == 9U && count(node.words[1U]) == 3U &&
            key(node.words[2U]) == 100U && node.words[3U] == 0U &&
            node.words[4U] == 0U && port.requests.size() == 1U &&
            port.requests[0U].allocation_size == 0x14U &&
            port.requests[0U].eax == 0U,
        "a missing definition-backed key allocates, links, clears five dwords, writes key and raw count, clamps to the definition maximum, scales, then increments the root count"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    set_definition_word(port, 0x44U, 0U);
    state.object_words[2U][1U] = 0U;
    const auto zero_maximum =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, port, {.key = 0U, .count = 9U}
        );
    test.expect_true(
        zero_maximum.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    completed &&
            zero_maximum.path == LegacyBattleFixedCountPath::existing_root &&
            zero_maximum.count_writes == 2U &&
            zero_maximum.clamp_writes == 1U && zero_maximum.maximum == 0U &&
            zero_maximum.count == 0U && zero_maximum.scale == 0U &&
            zero_maximum.return_eax == 1U && zero_maximum.return_ecx == 0U &&
            zero_maximum.return_edx == 0x80000000U,
        "a zero definition maximum preserves the raw write, inclusive zero clamp, zero-over-zero integer indefinite, and final EAX one"
    );
}

void test_definition_curve_typed_stops(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    DefinitionCurvePort port;
    const auto owner_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state,
            port,
            port,
            {
                .owner_token = 0x7F300000U,
                .key = 1U,
                .count = 2U,
                .entry_eax = 0x11111111U,
                .entry_ecx = 0x22222222U,
                .entry_edx = 0x33333333U,
            }
        );
    test.expect_true(
        owner_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop &&
            owner_stop.stopped_token == 0x7F300000U &&
            owner_stop.stopped_offset == 4U &&
            owner_stop.definition_load_calls == 0U &&
            owner_stop.return_eax == 0x11111111U &&
            owner_stop.return_ecx == 0x22222222U &&
            owner_stop.return_edx == 0x33333333U && port.calls.empty(),
        "an inaccessible owner stops at the initial root count read before loading a definition"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    state.object_words[2U][0U] = 0x7F300010U;
    state.object_words[2U][1U] = 1U;
    const auto next_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, port, {.key = 2U, .count = 3U}
        );
    test.expect_true(
        next_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    record_access_typed_stop &&
            next_stop.stopped_token == 0x7F300010U &&
            next_stop.stopped_offset == 4U &&
            next_stop.definition_load_calls == 1U &&
            next_stop.definition_cleanup_calls == 1U &&
            next_stop.key_reads == 0U && next_stop.return_eax == 0U,
        "a nonempty root publishes its link before loading and cleaning the definition, then stops at the linked key read"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    port.open_succeeds = false;
    const auto open_failed =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, port, {.key = 0U, .count = 9U}
        );
    test.expect_true(
        open_failed.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    completed &&
            open_failed.definition_load.status ==
                openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                    open_failed &&
            open_failed.definition_load_calls == 1U &&
            open_failed.definition_cleanup_calls == 1U &&
            open_failed.path == LegacyBattleFixedCountPath::existing_root &&
            open_failed.maximum == 0U && open_failed.count == 0U &&
            open_failed.scale == 0U && open_failed.return_eax == 1U &&
            open_failed.return_ecx == 0U &&
            open_failed.return_edx == 0x80000000U,
        "a normal MON open failure returns zero from the loader, still runs definition cleanup, and continues the original zero-maximum curve update"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    port.open_succeeds = true;
    port.allocation_succeeds = false;
    const auto definition_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, port, {.key = 3U, .count = 4U}
        );
    test.expect_true(
        definition_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    definition_load_typed_stop &&
            definition_stop.definition_load.status ==
                openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                    stream_zero_typed_stop &&
            definition_stop.definition_load_calls == 1U &&
            definition_stop.definition_cleanup_calls == 0U &&
            definition_stop.key_reads == 0U &&
            definition_stop.return_eax == 0U &&
            definition_stop.return_ecx == 0x100U,
        "a MON stream zero stops inside the closed definition loader before cleanup or chain search"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    port.allocation_succeeds = true;
    set_definition_word(port, 0x44U, 10U);
    port.replies.push_back({
        .eax = 0x7F300020U,
        .ecx = 0xAABBCCDDU,
        .edx = 0x11223344U,
        .initial_words =
            {0x11111111U, 0x22222222U, 0x33333333U, 0x44444444U, 0x55555555U},
        .accessible_bytes = 4U,
    });
    const auto clear_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, port, {.key = 5U, .count = 0x12340007U}
        );
    const auto& partial = state.fixed_count_nodes.front();
    test.expect_true(
        clear_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    allocation_record_access_typed_stop &&
            clear_stop.stopped_token == 0x7F300020U &&
            clear_stop.stopped_offset == 4U && clear_stop.link_writes == 1U &&
            clear_stop.dword_zero_writes == 1U &&
            clear_stop.return_eax == 0x7F300020U &&
            clear_stop.return_ecx == 0xAABB0007U &&
            clear_stop.return_edx == 0U &&
            state.object_words[2U][0U] == 0x7F300020U &&
            partial.words[0U] == 0U && partial.words[1U] == 0x22222222U,
        "the allocation path links and clears plus zero, replaces CX with the count, then stops at the inaccessible plus-four clear"
    );
}

void test_curve_set_access_stops(openswd3::test::Context& test) {
    LegacyBattleFixedObjectState state;
    AllocationPort port;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7D000000U,
        .words = {0U, 7U, 0U, 0U, 0U},
        .accessible_bytes = 7U,
    });
    auto& root = state.fixed_count_nodes.front();
    const auto count_stop = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
        port,
        {
            .owner_token = 0x7D000000U,
            .key = 7U,
            .maximum = 0xAAAA0002U,
            .count = 0xBBBB0001U,
            .entry_eax = 0xCCCC0002U,
            .entry_ecx = 0xDDDD0001U,
            .entry_edx = 0xEEEE0007U,
        }
    );
    test.expect_true(
        count_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            count_stop.path == LegacyBattleFixedCountPath::existing_root &&
            count_stop.stopped_token == 0x7D000000U &&
            count_stop.stopped_offset == 6U &&
            count_stop.return_eax == 0xAAAA0002U &&
            count_stop.return_ecx == 0xDDDD0001U &&
            count_stop.return_edx == 0xEEEE0007U && count(root.words[1U]) == 0U,
        "fixed curve set stops at the existing count write after loading the argument words into EAX and CX"
    );

    root.accessible_bytes = 9U;
    const auto scale_stop = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
        port,
        {
            .owner_token = 0x7D000000U,
            .key = 7U,
            .maximum = 2U,
            .count = 1U,
        }
    );
    test.expect_true(
        scale_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            scale_stop.stopped_offset == 8U && scale_stop.count == 1U &&
            scale_stop.truncate_calls == 1U && scale_stop.return_eax == 50U &&
            scale_stop.return_ecx == 1U && scale_stop.return_edx == 0U &&
            count(root.words[1U]) == 1U && root.words[2U] == 0U,
        "fixed curve set preserves the count and completed x87 conversion before an inaccessible scale write"
    );

    state = {};
    port.replies.push_back({
        .eax = 0x7D002000U,
        .ecx = 0xABCD1234U,
        .edx = 0xFFFFFFFFU,
        .initial_words =
            {
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x44444444U,
                0x55555555U,
            },
        .accessible_bytes = 4U,
    });
    const auto allocation_stop =
        openswd3::battle::set_legacy_battle_fixed_curve(
            state,
            port,
            {
                .key = 9U,
                .maximum = 5U,
                .count = 7U,
                .entry_ecx = 0xEEEE0007U,
                .entry_edx = 0xFFFF0009U,
            }
        );
    test.expect_true(
        allocation_stop.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            allocation_stop.stopped_token == 0x7D002000U &&
            allocation_stop.stopped_offset == 4U &&
            allocation_stop.link_writes == 1U &&
            allocation_stop.dword_zero_writes == 1U &&
            allocation_stop.return_eax == 0x7D002000U &&
            allocation_stop.return_ecx == 0xABCD0007U &&
            allocation_stop.return_edx == 0U &&
            state.object_words[1U][0U] == 0x7D002000U &&
            state.fixed_count_nodes.front().words[0U] == 0U &&
            state.fixed_count_nodes.front().words[1U] == 0x22222222U,
        "fixed curve set preserves the allocator ECX high word and first clear before the inaccessible plus-four clear"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_allocate_and_update(test);
    test_root_match_and_new_delta_width(test);
    test_allocation_write_stops(test);
    test_unmapped_chain_record_stop(test);
    test_set_existing_records(test);
    test_set_allocate_and_clamp(test);
    test_set_allocation_write_stops(test);
    test_set_record_access_stops(test);
    test_lookup_records_and_missing(test);
    test_lookup_record_access_stops(test);
    test_curve_existing_and_missing(test);
    test_curve_access_stops(test);
    test_curve_lookup_records_and_missing(test);
    test_curve_lookup_access_stops(test);
    test_curve_set_existing_and_missing(test);
    test_definition_curve_existing_and_locked(test);
    test_definition_curve_allocate_and_clamp(test);
    test_definition_curve_typed_stops(test);
    test_curve_set_access_stops(test);
    return test.exit_code();
}
