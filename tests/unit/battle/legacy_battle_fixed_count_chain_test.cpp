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
using openswd3::battle::LegacyBattleFixedObjectState;
using openswd3::compat::u16;
using openswd3::compat::u32;

class AllocationPort final : public LegacyBattleFixedCountAllocationPort {
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

}  // namespace

int main() {
    openswd3::test::Context test;
    test_allocate_and_update(test);
    test_root_match_and_new_delta_width(test);
    test_allocation_write_stops(test);
    test_unmapped_chain_record_stop(test);
    return test.exit_code();
}
