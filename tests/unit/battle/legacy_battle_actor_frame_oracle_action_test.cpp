#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u32;
using openswd3::asset_runtime::kLegacyActionRecordSize;

struct ObservedActionRecord {
    u32 sequence;
    u32 profile;
    std::array<u32, kLegacyActionRecordSize / sizeof(u32)> before;
    std::array<u32, kLegacyActionRecordSize / sizeof(u32)> after;
};

constexpr ObservedActionRecord kObservedV2Records[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-00479850/observed-records.inc"
};

constexpr ObservedActionRecord kObservedV3Records[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-v3-00479850/observed-records.inc"
};

constexpr ObservedActionRecord kObservedV4NestedRecords[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-v4-00479850/observed-records.inc"
};

static_assert(std::size(kObservedV2Records) == 17U);
static_assert(std::size(kObservedV3Records) == 34U);
static_assert(std::size(kObservedV4NestedRecords) == 18U);
static_assert(std::endian::native == std::endian::little);

}  // namespace

void test_battle_actor_frame_original_action_data(
    openswd3::test::Context& test
) {
#ifdef OPENSWD3_REAL_ACT_ROOT
    using namespace openswd3::asset_runtime;
    // V2 did not capture the original cache setting; test both provider paths.
    // V3 observed setting 1 only at entry/leave, not at the inner callsite.
    // V4 observed setting 1 at each nested call and return; only that path
    // is compared with its already-prepared action-record input.
    for (const u32 cache_mode : {0U, 1U}) {
        LegacyActRuntime runtime{OPENSWD3_REAL_ACT_ROOT};
        runtime.set_cache_limit(0x00080000U);
        LegacyActActionStreamProvider provider{runtime};
        LegacyActionUpdater updater{provider};
        updater.set_stream_cache_mode(cache_mode);
        const auto compare_record = [&](const ObservedActionRecord& row,
                                        const char* const capture,
                                        const bool already_prepared) {
            LegacyActionRecord record;
            std::memcpy(&record, row.before.data(), sizeof(record));
            // Only v2/v3 inputs precede these two parent writes. V4 inputs
            // are captured at the actual nested call, after both writes.
            // No CPU reply or missing register state is synthesized.
            if (!already_prepared) {
                record.action_id = row.profile;
                record.base_variant = 0x24U;
            }
            const auto result = updater.update(record);
            const std::string sample = std::string(capture) +
                " final_group_b action data seq " +
                std::to_string(row.sequence) + " cache " +
                std::to_string(cache_mode);
            test.expect_equal(
                result.status,
                LegacyActionUpdateStatus::completed,
                sample + " actual ACT provider completes"
            );
            test.expect_equal(
                result.return_value, 1U, sample + " updater succeeds"
            );
            const auto* const actual = reinterpret_cast<const u8*>(&record);
            const auto* const expected =
                reinterpret_cast<const u8*>(row.after.data());
            constexpr std::size_t kStreamPointer =
                offsetof(LegacyActionRecord, stream_pointer_32);
            for (std::size_t offset = 0U; offset < sizeof(record); ++offset) {
                if (offset >= kStreamPointer &&
                    offset < kStreamPointer + sizeof(u32)) {
                    continue;
                }

                test.expect_equal(
                    actual[offset],
                    expected[offset],
                    sample + " record byte " + std::to_string(offset)
                );
            }

            // The approved 64-bit adapter keeps only zero/nonzero in +0x54;
            // original guest pointer identity is deliberately not compared.
            test.expect_equal(
                record.stream_pointer_32,
                row.after[kStreamPointer / sizeof(u32)] != 0U ? 1U : 0U,
                sample + " stream availability"
            );
        };

        for (const auto& row : kObservedV2Records) {
            compare_record(row, "original v2", false);
        }

        if (cache_mode == 1U) {
            for (const auto& row : kObservedV3Records) {
                compare_record(row, "original v3", false);
            }

            for (const auto& row : kObservedV4NestedRecords) {
                compare_record(row, "original v4 nested", true);
            }
        }
    }

    std::cout
        << "WP316 original action-data diff: 17 samples, "
        << "both ACT cache settings, 148 bytes and stream availability "
        << "per sample; v3 profile407 34 samples with observed "
        << "entry/leave cache setting 1; v4 profile408 18 nested "
        << "input/output samples with observed callsite cache setting 1; "
        << "not CPU replies or complete sub_479850 replay.\n";
#else
    static_cast<void>(test);
    std::cout << "WP316 original action-data diff not run: "
              << "real ACT archives unavailable.\n";
#endif
}
