#include "fixed_node_memory_resource.hpp"
#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_fixed_count_chain.hpp"
#include "test.hpp"

#include <array>

namespace {

using openswd3::battle::LegacyBattleFixedCountPath;
using openswd3::battle::LegacyBattleFixedCountStatus;
using openswd3::battle::LegacyBattleFixedCurveX87StackState;
using openswd3::battle::LegacyBattleFixedObjectState;
using openswd3::compat::u16;
using openswd3::compat::u32;

using DefinitionCurvePort = openswd3::test::LegacyBattleMonDatabaseFixture;

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

void test_node_lifetime_and_reuse(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    {
        LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
        const auto first =
            openswd3::battle::set_legacy_battle_fixed_count(state, 10U, 3U);
        auto& first_node = state.fixed_count_nodes.front();
        const u32 first_token = first_node.legacy_token;
        memory.allocation_enabled = false;
        const auto updated =
            openswd3::battle::set_legacy_battle_fixed_count(state, 10U, 4U);
        test.expect_true(
            first.status == LegacyBattleFixedCountStatus::completed &&
                updated.status == LegacyBattleFixedCountStatus::completed &&
                updated.matched_token == first_token &&
                state.fixed_count_nodes.size() == 1U &&
                memory.outstanding_blocks == 1U &&
                count(first_node.words[1U]) == 4U,
            "an existing owned node remains writable when new allocations fail"
        );

        memory.allocation_enabled = true;
        const auto second =
            openswd3::battle::set_legacy_battle_fixed_count(state, 11U, 5U);
        const auto& second_node = state.fixed_count_nodes.back();
        test.expect_true(
            second.status == LegacyBattleFixedCountStatus::completed &&
                second_node.legacy_token != first_token &&
                first_node.words[0U] == second_node.legacy_token &&
                count(first_node.words[1U]) == 4U &&
                key(second_node.words[1U]) == 11U &&
                count(second_node.words[1U]) == 5U &&
                key(state.object_words[0U][1U]) == 2U &&
                memory.outstanding_blocks == 2U,
            "appending a distinct guest identity preserves existing node references and values"
        );
    }

    test.expect_true(
        memory.outstanding_blocks == 0U,
        "destroying the shared state releases every owned node allocation"
    );
}

void test_allocate_and_update(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};

    const auto created = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
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
            state.fixed_count_nodes.size() == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.count_writes == 1U &&
            created.key_writes == 1U && created.root_key_increments == 1U &&
            created.return_edx == 0U && root[0U] == node.legacy_token &&
            key(root[1U]) == 1U && count(root[1U]) == 0U &&
            node.legacy_token != 0U && node.words[0U] == 0U &&
            key(node.words[1U]) == 0x1234U && count(node.words[1U]) == 1U &&
            node.words[2U] == 0U && node.words[3U] == 0U &&
            node.words[4U] == 0U,
        "missing key links the allocated record before clearing five dwords and publishing key, count, and root key"
    );

    const auto updated = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
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
            updated.matched_token == node.legacy_token &&
            updated.count_reads == 1U && updated.count_writes == 1U &&
            updated.return_ecx == 0x00010002U &&
            updated.return_edx == 0xCCCCCCCCU && count(node.words[1U]) == 3U,
        "existing dynamic record writes the low word of the quantity plus the full dword delta"
    );

    auto& mutable_node = state.fixed_count_nodes.front();
    mutable_node.words[1U] =
        (mutable_node.words[1U] & 0x0000FFFFU) | (0x14U << 16U);
    const auto capped = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
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
            capped.count_writes == 0U && capped.return_ecx == 0x33334444U &&
            capped.return_edx == 0x55556666U &&
            count(mutable_node.words[1U]) == 0x14U,
        "unsigned count twenty returns before loading the delta or writing the record"
    );
}

void test_root_match_and_new_delta_width(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    auto& root = state.object_words[0U];
    root[1U] = (0x13U << 16U) | 7U;

    const auto root_update =
        openswd3::battle::accumulate_legacy_battle_fixed_count(
            state,
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
    const auto created = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
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
            created.return_edx == 0U && key(node.words[1U]) == 0x2222U &&
            count(node.words[1U]) == 2U,
        "new-record path stores only the low delta word in the owned node"
    );
}

void test_allocation_write_stops(openswd3::test::Context& test) {
    for (const bool existing_tail : {false, true}) {
        openswd3::test::FixedNodeMemoryResource memory;
        LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
        if (existing_tail) {
            state.fixed_count_nodes.push_back({
                .legacy_token = 0x73000000U,
                .words = {0U, (3U << 16U) | 7U, 0U, 0U, 0U},
            });
            state.object_words[0U][0U] = 0x73000000U;
            state.object_words[0U][1U] = 1U;
        }

        memory.allocation_enabled = false;
        const auto failed =
            openswd3::battle::accumulate_legacy_battle_fixed_count(
                state, {.key = 9U, .delta = 9U}
            );
        test.expect_true(
            failed.status ==
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop &&
                failed.stopped_token == 0U && failed.stopped_offset == 0U &&
                failed.link_writes == 1U && failed.dword_zero_writes == 0U &&
                failed.count_writes == 0U && failed.root_key_increments == 0U &&
                state.fixed_count_nodes.size() ==
                    static_cast<std::size_t>(existing_tail) &&
                memory.outstanding_blocks == state.fixed_count_nodes.size() &&
                state.object_words[0U][0U] ==
                    (existing_tail ? 0x73000000U : 0U) &&
                key(state.object_words[0U][1U]) == (existing_tail ? 1U : 0U),
            "actual allocation failure publishes the null tail link without changing existing records or the root count"
        );
        if (existing_tail) {
            const auto& tail = state.fixed_count_nodes.front();
            test.expect_true(
                tail.words[0U] == 0U && key(tail.words[1U]) == 7U &&
                    count(tail.words[1U]) == 3U,
                "failed append retains the previously owned tail and its quantity"
            );
        }
    }
}

void test_unmapped_chain_record_stop(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    state.object_words[0U][0U] = 0x74000000U;

    const auto result = openswd3::battle::accumulate_legacy_battle_fixed_count(
        state,
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
            result.return_edx == 0x55556666U,
        "unmapped successor stops at its first original key read after publishing EAX from the predecessor link"
    );
}

void test_set_existing_records(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    auto& root = state.object_words[0U];
    root[1U] = (9U << 16U) | 7U;

    const auto root_set = openswd3::battle::set_legacy_battle_fixed_count(
        state, 7U, static_cast<u16>(0xABCD0015U)
    );
    test.expect_true(
        root_set.status == LegacyBattleFixedCountStatus::completed &&
            root_set.path == LegacyBattleFixedCountPath::existing_root &&
            key(root[1U]) == 7U && count(root[1U]) == 20U,
        "existing root receives the raw low word before unsigned values above twenty are overwritten with twenty"
    );

    state.fixed_count_nodes.push_back({
        .legacy_token = 0x75001234U,
        .words = {0U, (3U << 16U) | 8U, 0U, 0U, 0U},
        .accessible_bytes = 0x14U,
    });
    root[0U] = 0x75001234U;
    const auto node_set = openswd3::battle::set_legacy_battle_fixed_count(
        state, 8U, static_cast<u16>(0x12340014U)
    );
    test.expect_true(
        node_set.status == LegacyBattleFixedCountStatus::completed &&
            node_set.path == LegacyBattleFixedCountPath::existing_node &&
            node_set.matched_token == 0x75001234U &&
            count(state.fixed_count_nodes.front().words[1U]) == 20U,
        "existing dynamic record receives an exact quantity of twenty"
    );
}

void test_set_allocate_and_clamp(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};

    const auto result = openswd3::battle::set_legacy_battle_fixed_count(
        state, static_cast<u16>(0xFFFF3456U), static_cast<u16>(0xABCD0019U)
    );
    const auto& root = state.object_words[0U];
    const auto& node = state.fixed_count_nodes.front();
    test.expect_true(
        result.status == LegacyBattleFixedCountStatus::completed &&
            result.path == LegacyBattleFixedCountPath::allocated_node &&
            state.fixed_count_nodes.size() == 1U &&
            result.matched_token == node.legacy_token &&
            root[0U] == node.legacy_token && key(root[1U]) == 1U &&
            key(node.words[1U]) == 0x3456U && count(node.words[1U]) == 20U &&
            node.words[0U] == 0U && node.words[2U] == 0U &&
            node.words[3U] == 0U && node.words[4U] == 0U,
        "missing key links and clears one shared node, writes key then raw count, clamps, and increments the root word"
    );
}

void test_set_allocation_write_stops(openswd3::test::Context& test) {
    for (const bool existing_tail : {false, true}) {
        openswd3::test::FixedNodeMemoryResource memory;
        LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
        if (existing_tail) {
            state.fixed_count_nodes.push_back({
                .legacy_token = 0x73000000U,
                .words = {0U, (3U << 16U) | 7U, 0U, 0U, 0U},
            });
            state.object_words[0U][0U] = 0x73000000U;
            state.object_words[0U][1U] = 1U;
        }

        memory.allocation_enabled = false;
        const auto failed =
            openswd3::battle::set_legacy_battle_fixed_count(state, 9U, 9U);
        test.expect_true(
            failed.status ==
                    LegacyBattleFixedCountStatus::
                        allocation_record_access_typed_stop &&
                failed.stopped_token == 0U && failed.stopped_offset == 0U &&
                state.fixed_count_nodes.size() ==
                    static_cast<std::size_t>(existing_tail) &&
                memory.outstanding_blocks == state.fixed_count_nodes.size() &&
                state.object_words[0U][0U] ==
                    (existing_tail ? 0x73000000U : 0U) &&
                key(state.object_words[0U][1U]) == (existing_tail ? 1U : 0U),
            "actual allocation failure publishes the null tail link without changing existing records or the root count"
        );
        if (existing_tail) {
            const auto& tail = state.fixed_count_nodes.front();
            test.expect_true(
                tail.words[0U] == 0U && key(tail.words[1U]) == 7U &&
                    count(tail.words[1U]) == 3U,
                "failed append retains the previously owned tail and its quantity"
            );
        }
    }
}

void test_set_record_access_stops(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    state.object_words[0U][0U] = 0x78000000U;
    const auto unmapped =
        openswd3::battle::set_legacy_battle_fixed_count(state, 1U, 2U);
    test.expect_true(
        unmapped.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            unmapped.stopped_token == 0x78000000U &&
            unmapped.stopped_offset == 4U,
        "set path reports the unmapped successor at its first key access"
    );

    state = {};
    state.object_words[0U][0U] = 0x78000010U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x78000010U,
        .words = {0U, (3U << 16U) | 9U, 0U, 0U, 0U},
        .accessible_bytes = 7U,
    });
    const auto count_stop = openswd3::battle::set_legacy_battle_fixed_count(
        state, 9U, static_cast<u16>(0x1234000AU)
    );
    test.expect_true(
        count_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            count_stop.path == LegacyBattleFixedCountPath::existing_node &&
            count_stop.stopped_token == 0x78000010U &&
            count_stop.stopped_offset == 6U &&
            count(state.fixed_count_nodes.front().words[1U]) == 3U,
        "existing set path reports the inaccessible quantity write and leaves the previous quantity intact"
    );
}

void test_lookup_records_and_missing(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    state.object_words[0U][1U] = (12U << 16U) | 3U;
    const auto root = openswd3::battle::lookup_legacy_battle_fixed_count(
        state, 3U
    );
    test.expect_true(
        root.status == LegacyBattleFixedCountStatus::completed &&
            root.path == LegacyBattleFixedCountPath::existing_root &&
            root.matched_token == 0x004B9F00U && root.quantity == 12U,
        "lookup compares the root first and returns its quantity word"
    );

    state.object_words[0U][0U] = 0x79000000U;
    state.object_words[0U][1U] = (1U << 16U) | 2U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x79000000U,
        .words = {0U, (17U << 16U) | 9U, 0U, 0U, 0U},
    });
    const auto node = openswd3::battle::lookup_legacy_battle_fixed_count(
        state, 9U
    );
    const auto missing = openswd3::battle::lookup_legacy_battle_fixed_count(
        state, 10U
    );
    test.expect_true(
        node.status == LegacyBattleFixedCountStatus::completed &&
            node.path == LegacyBattleFixedCountPath::existing_node &&
            node.matched_token == 0x79000000U && node.quantity == 17U &&
            missing.status == LegacyBattleFixedCountStatus::completed &&
            missing.path == LegacyBattleFixedCountPath::none &&
            missing.matched_token == 0U && missing.quantity == 0U,
        "lookup scans mapped successors in order and returns zero when the key is absent"
    );
}

void test_lookup_record_access_stops(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    state.object_words[0U][0U] = 0x79000010U;
    state.object_words[0U][1U] = 2U;
    const auto unmapped = openswd3::battle::lookup_legacy_battle_fixed_count(
        state, 9U
    );
    test.expect_true(
        unmapped.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            unmapped.stopped_token == 0x79000010U &&
            unmapped.stopped_offset == 4U,
        "lookup reports the unmapped successor key read"
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
        state, 9U
    );
    const auto owner_stop = openswd3::battle::lookup_legacy_battle_fixed_count(
        state, 9U, 0x79000030U
    );
    test.expect_true(
        count_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            count_stop.path == LegacyBattleFixedCountPath::existing_node &&
            count_stop.matched_token == 0x79000020U &&
            count_stop.stopped_token == 0x79000020U &&
            count_stop.stopped_offset == 6U &&
            owner_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            owner_stop.stopped_token == 0x79000030U &&
            owner_stop.stopped_offset == 4U,
        "lookup reports the exact inaccessible quantity or root key"
    );
}

void test_curve_existing_and_missing(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    auto& root = state.object_words[1U];
    root[1U] = (4U << 16U) | 7U;
    root[2U] = 0xABCD4321U;

    const auto existing = openswd3::battle::advance_legacy_battle_fixed_curve(
        state,
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
            root[2U] == 0xABCD0064U,
        "existing fixed curve increments then unsigned-clamps the count and preserves the scale dword high word"
    );

    root[1U] = (0xFFFFU << 16U) | 7U;
    const auto wrapped = openswd3::battle::advance_legacy_battle_fixed_curve(
        state, {.key = 7U, .maximum = 2U, .multiplier = 20U}
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
    const auto created = openswd3::battle::advance_legacy_battle_fixed_curve(
        state,
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
            state.fixed_count_nodes.size() == 1U &&
            created.x87_stack == LegacyBattleFixedCurveX87StackState::empty &&
            created.link_writes == 1U && created.dword_zero_writes == 5U &&
            created.key_writes == 1U && created.count_writes == 1U &&
            created.scale_writes == 1U && created.root_key_increments == 1U &&
            created.truncate_calls == 2U && created.count == 1U &&
            created.scale == 33U && created.return_eax == 33U &&
            created.return_ecx == 0U && created.return_edx == 0U &&
            created_root[0U] == node.legacy_token &&
            key(created_root[1U]) == 1U && key(node.words[1U]) == 9U &&
            count(node.words[1U]) == 1U && key(node.words[2U]) == 33U,
        "missing fixed curve links and clears one node before publishing key count scale and the root key total"
    );

    state = {};
    const auto zero_maximum =
        openswd3::battle::advance_legacy_battle_fixed_curve(
            state, {.key = 0U, .maximum = 0U, .multiplier = 5U}
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
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
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
        state, {.key = 9U, .maximum = 2U, .multiplier = 6U}
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
    memory.allocation_enabled = false;
    const auto allocation_stop =
        openswd3::battle::advance_legacy_battle_fixed_curve(
            state, {.key = 9U, .maximum = 4U, .multiplier = 12U}
        );
    test.expect_true(
        allocation_stop.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            allocation_stop.stopped_token == 0U &&
            allocation_stop.stopped_offset == 0U &&
            allocation_stop.link_writes == 1U &&
            allocation_stop.dword_zero_writes == 0U &&
            state.object_words[1U][0U] == 0U &&
            key(state.object_words[1U][1U]) == 0U &&
            state.fixed_count_nodes.empty() && memory.outstanding_blocks == 0U,
        "actual node allocation failure retains the null published link and skips initialization"
    );
}

void test_curve_set_existing_and_missing(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    auto& root = state.object_words[1U];
    root[1U] = (4U << 16U) | 7U;
    root[2U] = 0xABCD4321U;

    const auto existing = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
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
            root[2U] == 0xABCD0064U,
        "existing fixed curve set writes the requested word before the inclusive maximum clamp and percentage"
    );

    state = {};
    state.object_words[1U][1U] = 0xFFFFU;
    const auto created = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
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
            state.fixed_count_nodes.size() == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.key_writes == 1U &&
            created.count_writes == 1U && created.clamp_writes == 0U &&
            created.scale_writes == 1U && created.root_key_increments == 1U &&
            created.truncate_calls == 1U && created.count == 1U &&
            created.scale == 33U && created.return_eax == 33U &&
            created.return_ecx == 1U && created.return_edx == 0U &&
            created_root[0U] == node.legacy_token &&
            key(created_root[1U]) == 0U && key(node.words[1U]) == 9U &&
            count(node.words[1U]) == 1U && key(node.words[2U]) == 33U &&
            node.words[3U] == 0U && node.words[4U] == 0U,
        "missing fixed curve set links and clears one node before publishing key count percentage and the wrapped root total"
    );

    node.words[2U] = 0xABCD0021U;
    const auto existing_node = openswd3::battle::set_legacy_battle_fixed_curve(
        state, {.key = 9U, .maximum = 4U, .count = 2U}
    );
    test.expect_true(
        existing_node.status == LegacyBattleFixedCountStatus::completed &&
            existing_node.path == LegacyBattleFixedCountPath::existing_node &&
            existing_node.chain_link_reads == 1U && existing_node.count == 2U &&
            existing_node.scale == 50U && existing_node.return_eax == 50U &&
            existing_node.return_ecx == 2U && existing_node.return_edx == 0U &&
            key(created_root[1U]) == 0U && count(node.words[1U]) == 2U &&
            node.words[2U] == 0xABCD0032U,
        "existing dynamic fixed curve set follows one next link without allocating or incrementing the root"
    );

    state = {};
    const auto zero_maximum = openswd3::battle::set_legacy_battle_fixed_curve(
        state, {.key = 0U, .maximum = 0U, .count = 9U}
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
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    auto& root = state.object_words[1U];
    root[1U] = 0xAAAA1234U;
    root[2U] = 0xBBBB5678U;

    const auto root_hit = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state, 0x1234U
    );
    test.expect_true(
        root_hit.status == LegacyBattleFixedCountStatus::completed &&
            root_hit.value == 0x5678U &&
            root_hit.matched_token == 0x004ACBA8U,
        "fixed curve lookup compares word keys and reads the value word from the root"
    );

    root[0U] = 0x7E001234U;
    root[1U] = 1U;
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7E001234U,
        .words = {0U, 0xBBBB2345U, 0xCCCC4321U, 0U, 0U},
        .accessible_bytes = 0x14U,
    });
    const auto node_hit = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state, 0x2345U
    );
    test.expect_true(
        node_hit.status == LegacyBattleFixedCountStatus::completed &&
            node_hit.value == 0x4321U &&
            node_hit.matched_token == 0x7E001234U,
        "fixed curve lookup follows the actual link and returns the matched node value"
    );

    const auto missing = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state, 0x7777U
    );
    test.expect_true(
        missing.status == LegacyBattleFixedCountStatus::completed &&
            missing.value == 0U && missing.matched_token == 0U,
        "missing fixed curve lookup returns zero only after scanning the existing chain"
    );
}

void test_curve_lookup_access_stops(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7F001234U,
        .words = {0U, 0x11112222U, 0x33334444U, 0U, 0U},
        .accessible_bytes = 5U,
    });
    const auto key_stop = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state, 0x2222U, 0x7F001234U
    );
    test.expect_true(
        key_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            key_stop.stopped_token == 0x7F001234U &&
            key_stop.stopped_offset == 4U,
        "fixed curve lookup reports the first inaccessible owner key"
    );

    state.fixed_count_nodes.front().accessible_bytes = 8U;
    const auto value_stop = openswd3::battle::lookup_legacy_battle_fixed_curve(
        state, 0x2222U, 0x7F001234U
    );
    test.expect_true(
        value_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            value_stop.stopped_token == 0x7F001234U &&
            value_stop.stopped_offset == 8U,
        "fixed curve lookup reports an inaccessible matching value"
    );

    state = {};
    state.object_words[1U][0U] = 0x7F00ABCDU;
    state.object_words[1U][1U] = 1U;
    const auto next_key_stop =
        openswd3::battle::lookup_legacy_battle_fixed_curve(state, 2U);
    test.expect_true(
        next_key_stop.status ==
                LegacyBattleFixedCountStatus::record_access_typed_stop &&
            next_key_stop.stopped_token == 0x7F00ABCDU &&
            next_key_stop.stopped_offset == 4U,
        "fixed curve lookup stops at the real key read of an unmapped linked token"
    );
}

void test_definition_curve_existing_and_locked(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
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
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    DefinitionCurvePort port;
    set_definition_word(port, 0x44U, 3U);

    const auto created =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state,
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
            state.fixed_count_nodes.size() == 1U && created.link_writes == 1U &&
            created.dword_zero_writes == 5U && created.key_writes == 1U &&
            created.count_writes == 2U && created.clamp_writes == 1U &&
            created.scale_writes == 1U && created.root_count_increments == 1U &&
            created.maximum == 3U && created.count == 3U &&
            created.scale == 100U && created.return_eax == 1U &&
            created.return_ecx == 3U && created.return_edx == 0U &&
            root[0U] == node.legacy_token && key(root[1U]) == 1U &&
            node.legacy_token != 0U && node.words[0U] == 0U &&
            key(node.words[1U]) == 9U && count(node.words[1U]) == 3U &&
            key(node.words[2U]) == 100U && node.words[3U] == 0U &&
            node.words[4U] == 0U,
        "a missing definition-backed key allocates, links, clears five dwords, writes key and raw count, clamps to the definition maximum, scales, then increments the root count"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    set_definition_word(port, 0x44U, 0U);
    state.object_words[2U][1U] = 0U;
    const auto zero_maximum =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, {.key = 0U, .count = 9U}
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
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    DefinitionCurvePort port;
    const auto owner_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state,
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
            owner_stop.return_edx == 0x33333333U && port.opened_path.empty() &&
            port.read_sizes.empty(),
        "an inaccessible owner stops at the initial root count read before loading a definition"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    state.object_words[2U][0U] = 0x7F300010U;
    state.object_words[2U][1U] = 1U;
    const auto next_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, {.key = 2U, .count = 3U}
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
            state, port, {.key = 0U, .count = 9U}
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
            state, port, {.key = 3U, .count = 4U}
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
            definition_stop.key_reads == 0U && port.released_streams.empty(),
        "a MON stream zero stops inside the closed definition loader before cleanup or chain search"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    port.allocation_succeeds = true;
    set_definition_word(port, 0x44U, 10U);
    memory.allocation_enabled = false;
    const auto allocation_stop =
        openswd3::battle::set_legacy_battle_fixed_definition_curve(
            state, port, {.key = 5U, .count = 7U}
        );
    test.expect_true(
        allocation_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveSetStatus::
                    allocation_record_access_typed_stop &&
            allocation_stop.stopped_token == 0U &&
            allocation_stop.stopped_offset == 0U &&
            allocation_stop.link_writes == 1U &&
            allocation_stop.dword_zero_writes == 0U &&
            state.object_words[2U][0U] == 0U &&
            key(state.object_words[2U][1U]) == 0U &&
            state.fixed_count_nodes.empty() && memory.outstanding_blocks == 0U,
        "actual node allocation failure retains the null published link and skips initialization"
    );
}

void test_definition_curve_lookup_hit_and_miss(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    DefinitionCurvePort port;
    set_definition_word(port, 0x44U, 0x5678U);
    port.definition_description = {'x', 0U};
    auto& root = state.object_words[2U];
    root[1U] = (0x1234U << 16U) | 7U;
    u16 maximum = 0xAAAAU;
    u16 current = 0xBBBBU;
    const auto hit =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, &maximum, &current
        );
    test.expect_true(
        hit.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    completed &&
            hit.path == LegacyBattleFixedCountPath::existing_root &&
            hit.matched_token ==
                openswd3::battle::kLegacyBattleFixedDefinitionCurveOwnerToken &&
            hit.maximum == 0x5678U && hit.count == 0x1234U &&
            maximum == 0x5678U && current == 0x1234U &&
            port.requested_definition_ids == std::vector<u32>{7U} &&
            port.definition_text_release_calls == 1U,
        "definition curve lookup searches the root before loading MON, releases transient text, then writes maximum before the matched count"
    );

    port.reset_mon_session();
    port.clear_definition();
    set_definition_word(port, 0x44U, 3U);
    maximum = 0xAAAAU;
    current = 0xBBBBU;
    const auto missing =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 8U, &maximum, &current
        );
    test.expect_true(
        missing.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    completed &&
            missing.path == LegacyBattleFixedCountPath::none &&
            missing.matched_token == 0U && missing.maximum == 3U &&
            missing.count == 0U && maximum == 3U && current == 0U &&
            port.requested_definition_ids == std::vector<u32>{8U},
        "a missing definition curve key still loads and cleans MON, then writes the maximum and a zero count"
    );

    port.reset_mon_session();
    port.clear_definition();
    port.open_succeeds = false;
    maximum = 0xAAAAU;
    current = 0xBBBBU;
    const auto open_failed =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 8U, &maximum, &current
        );
    test.expect_true(
        open_failed.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    completed &&
            open_failed.definition_load.status ==
                openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                    open_failed &&
            open_failed.maximum == 0U && maximum == 0U && current == 0U,
        "a normal MON open failure still cleans the scratch and publishes zero maximum and count on the missing path"
    );
}

void test_definition_curve_lookup_typed_stops(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    DefinitionCurvePort port;
    u16 maximum = 0xAAAAU;
    u16 current = 0xBBBBU;
    const auto owner_stop =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, &maximum, &current, 0x7F400000U
        );
    test.expect_true(
        owner_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    record_access_typed_stop &&
            owner_stop.stopped_token == 0x7F400000U &&
            owner_stop.stopped_offset == 4U &&
            maximum == 0xAAAAU &&
            current == 0xBBBBU && port.opened_path.empty() &&
            port.read_sizes.empty(),
        "an inaccessible lookup owner stops at the first root key read before MON or output effects"
    );

    state = {};
    port.reset_mon_session();
    state.object_words[2U][0U] = 0x7F400010U;
    state.object_words[2U][1U] = 1U;
    const auto link_stop =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, &maximum, &current
        );
    test.expect_true(
        link_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    record_access_typed_stop &&
            link_stop.stopped_token == 0x7F400010U &&
            link_stop.stopped_offset == 4U &&
            maximum == 0xAAAAU && current == 0xBBBBU &&
            port.opened_path.empty(),
        "an unmapped successor stops at its key read before loading MON or writing outputs"
    );

    state = {};
    port.reset_mon_session();
    port.open_succeeds = true;
    port.allocation_succeeds = false;
    state.object_words[2U][1U] = 7U;
    const auto definition_stop =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, &maximum, &current
        );
    test.expect_true(
        definition_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    definition_load_typed_stop &&
            definition_stop.path == LegacyBattleFixedCountPath::existing_root &&
            definition_stop.definition_load.status ==
                openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                    stream_zero_typed_stop &&
            maximum == 0xAAAAU && current == 0xBBBBU,
        "a MON typed stop occurs after the chain match but before cleanup and either output write"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    port.allocation_succeeds = true;
    set_definition_word(port, 0x44U, 9U);
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7F400020U,
        .words = {0U, (4U << 16U) | 7U, 0U, 0U, 0U},
        .accessible_bytes = 7U,
    });
    maximum = 0xAAAAU;
    current = 0xBBBBU;
    const auto count_read_stop =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, &maximum, &current, 0x7F400020U
        );
    test.expect_true(
        count_read_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    record_access_typed_stop &&
            count_read_stop.stopped_token == 0x7F400020U &&
            count_read_stop.stopped_offset == 6U &&
            maximum == 9U && current == 0xBBBBU,
        "a short matched record preserves the completed maximum write before stopping at the original count read"
    );

    state = {};
    port.reset_mon_session();
    port.clear_definition();
    set_definition_word(port, 0x44U, 5U);
    state.object_words[2U][1U] = (3U << 16U) | 7U;
    maximum = 0xAAAAU;
    const auto count_output_stop =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, &maximum, nullptr
        );
    test.expect_true(
        count_output_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    count_output_typed_stop &&
            maximum == 5U && count_output_stop.count == 3U,
        "a matched count output stop keeps the maximum write and count read prefixes"
    );

    state.object_words[2U][1U] = 8U;
    const auto missing_maximum_stop =
        openswd3::battle::lookup_legacy_battle_fixed_definition_curve(
            state, port, 7U, nullptr, &current
        );
    test.expect_true(
        missing_maximum_stop.status ==
                openswd3::battle::LegacyBattleFixedDefinitionCurveLookupStatus::
                    maximum_output_typed_stop &&
            missing_maximum_stop.maximum == 5U && current == 0xBBBBU,
        "the missing path reads the maximum but leaves count untouched when the first output is inaccessible"
    );
}

void test_curve_set_access_stops(openswd3::test::Context& test) {
    openswd3::test::FixedNodeMemoryResource memory;
    LegacyBattleFixedObjectState state{.fixed_count_nodes{&memory}};
    state.fixed_count_nodes.push_back({
        .legacy_token = 0x7D000000U,
        .words = {0U, 7U, 0U, 0U, 0U},
        .accessible_bytes = 7U,
    });
    auto& root = state.fixed_count_nodes.front();
    const auto count_stop = openswd3::battle::set_legacy_battle_fixed_curve(
        state,
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
    memory.allocation_enabled = false;
    const auto allocation_stop =
        openswd3::battle::set_legacy_battle_fixed_curve(
            state, {.key = 9U, .maximum = 5U, .count = 7U}
        );
    test.expect_true(
        allocation_stop.status ==
                LegacyBattleFixedCountStatus::
                    allocation_record_access_typed_stop &&
            allocation_stop.stopped_token == 0U &&
            allocation_stop.stopped_offset == 0U &&
            allocation_stop.link_writes == 1U &&
            allocation_stop.dword_zero_writes == 0U &&
            state.object_words[1U][0U] == 0U &&
            key(state.object_words[1U][1U]) == 0U &&
            state.fixed_count_nodes.empty() && memory.outstanding_blocks == 0U,
        "actual node allocation failure retains the null published link and skips initialization"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_node_lifetime_and_reuse(test);
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
    test_definition_curve_lookup_hit_and_miss(test);
    test_definition_curve_lookup_typed_stops(test);
    test_curve_set_access_stops(test);
    return test.exit_code();
}
