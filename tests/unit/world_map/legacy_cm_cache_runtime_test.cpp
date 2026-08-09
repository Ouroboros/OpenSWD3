#include "test.hpp"

#include "openswd3/world_map/legacy_cm_cache_runtime.hpp"

#include <array>

namespace {

using openswd3::compat::u32;
using openswd3::world_map::age_and_find_legacy_cm_cache_record;
using openswd3::world_map::kLegacyCmCacheInvalidMap;
using openswd3::world_map::LegacyCmCacheRecord;
using openswd3::world_map::prepare_legacy_cm_cache_miss;

void test_age_lookup_and_first_match(openswd3::test::Context& test) {
    std::array records{
        LegacyCmCacheRecord{.map_id = 10U, .byte_size = 5U,
                            .use_counter = 0xFFFFFFFFU, .stored_slot = 7U},
        LegacyCmCacheRecord{.map_id = 20U, .byte_size = 8U,
                            .use_counter = 4U, .stored_slot = 8U},
        LegacyCmCacheRecord{.map_id = 20U, .byte_size = 13U,
                            .use_counter = 9U, .stored_slot = 9U},
    };

    const auto lookup = age_and_find_legacy_cm_cache_record(records, 20U);
    test.expect_true(
        lookup.found && lookup.record_index == 1U &&
            lookup.prefix_byte_size == 5U && lookup.record.stored_slot == 8U,
        "first matching record and prefix size are retained"
    );
    test.expect_true(
        records[0].use_counter == 0U &&
            records[1].use_counter == 0U &&
            records[2].use_counter == 10U,
        "all counters increment with u32 wrap before the hit resets"
    );
}

void test_miss_ages_all_and_wraps_prefix(openswd3::test::Context& test) {
    std::array records{
        LegacyCmCacheRecord{.map_id = 1U, .byte_size = 0xFFFFFFFEU,
                            .use_counter = 1U, .stored_slot = 0U},
        LegacyCmCacheRecord{.map_id = kLegacyCmCacheInvalidMap,
                            .byte_size = 5U, .use_counter = 2U,
                            .stored_slot = 1U},
    };
    const auto lookup = age_and_find_legacy_cm_cache_record(records, 2U);
    test.expect_true(
        !lookup.found && lookup.record_index == 2U &&
            lookup.prefix_byte_size == 3U,
        "miss scans every record and preserves dword size wrap"
    );
    test.expect_true(
        records[0].use_counter == 2U && records[1].use_counter == 3U,
        "miss leaves every incremented counter in memory"
    );
}

void test_miss_plan_uses_free_record_without_eviction(
    openswd3::test::Context& test
) {
    std::array records{
        LegacyCmCacheRecord{.map_id = 10U, .byte_size = 100U,
                            .use_counter = 3U, .stored_slot = 9U},
        LegacyCmCacheRecord{.map_id = kLegacyCmCacheInvalidMap,
                            .byte_size = 0U, .use_counter = 4U,
                            .stored_slot = 7U},
    };
    const auto plan = prepare_legacy_cm_cache_miss(
        records,
        20U,
        200U,
        1U
    );
    test.expect_true(
        plan.ready && plan.insertion_record_index == 1U &&
            plan.evictions.empty() && plan.total_size_before == 100U &&
            plan.total_size_after == 100U,
        "an existing free record bypasses eviction below the byte limit"
    );
    test.expect_true(
        records[1].map_id == 20U && records[1].byte_size == 200U &&
            records[1].use_counter == 0U && records[1].stored_slot == 1U,
        "miss insertion normalizes the stored slot to the record index"
    );
}

void test_miss_plan_eviction_order(openswd3::test::Context& test) {
    constexpr u32 one_megabyte = 1U << 20U;
    std::array records{
        LegacyCmCacheRecord{.map_id = 10U, .byte_size = one_megabyte,
                            .use_counter = 2U, .stored_slot = 9U},
        LegacyCmCacheRecord{.map_id = 11U, .byte_size = 1U,
                            .use_counter = 100U, .stored_slot = 8U},
    };
    const auto plan = prepare_legacy_cm_cache_miss(
        records,
        20U,
        3U * one_megabyte,
        4U
    );
    test.expect_true(
        plan.ready && plan.evictions.size() == 1U &&
            plan.evictions[0].record_index == 0U &&
            plan.evictions[0].stored_slot == 9U &&
            plan.evictions[0].byte_size == one_megabyte,
        "first record older than cache_limit/4 wins before later records"
    );
    test.expect_true(
        records[0].map_id == 20U && records[0].stored_slot == 0U,
        "evicted record is immediately reused by the requested map"
    );
}

void test_miss_plan_smallest_fallback_and_safety(
    openswd3::test::Context& test
) {
    std::array records{
        LegacyCmCacheRecord{.map_id = 10U, .byte_size = 8U,
                            .use_counter = 0U, .stored_slot = 4U},
        LegacyCmCacheRecord{.map_id = 11U, .byte_size = 3U,
                            .use_counter = 0U, .stored_slot = 6U},
        LegacyCmCacheRecord{.map_id = 12U, .byte_size = 3U,
                            .use_counter = 0U, .stored_slot = 7U},
    };
    const auto plan = prepare_legacy_cm_cache_miss(
        records,
        20U,
        1U,
        60U
    );
    test.expect_true(
        plan.ready && plan.evictions.size() == 1U &&
            plan.evictions[0].record_index == 1U &&
            plan.evictions[0].stored_slot == 6U,
        "a full directory evicts the first strictly smallest record"
    );

    std::array free_records{
        LegacyCmCacheRecord{.map_id = kLegacyCmCacheInvalidMap,
                            .byte_size = 0U, .use_counter = 0U,
                            .stored_slot = 0U},
    };
    const auto impossible = prepare_legacy_cm_cache_miss(
        free_records,
        20U,
        2U << 20U,
        1U
    );
    test.expect_true(
        !impossible.ready && impossible.evictions.empty(),
        "oversized map with no active eviction candidate stops at safety boundary"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_age_lookup_and_first_match(test);
    test_miss_ages_all_and_wraps_prefix(test);
    test_miss_plan_uses_free_record_without_eviction(test);
    test_miss_plan_eviction_order(test);
    test_miss_plan_smallest_fallback_and_safety(test);
    return test.exit_code();
}
