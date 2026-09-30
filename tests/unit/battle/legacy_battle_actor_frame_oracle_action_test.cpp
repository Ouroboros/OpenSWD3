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

constexpr ObservedActionRecord kObservedRecords[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-00479850/observed-records.inc"
};

static_assert(std::size(kObservedRecords) == 17U);
static_assert(std::endian::native == std::endian::little);

}  // namespace

void test_battle_actor_frame_original_action_data(
    openswd3::test::Context& test
) {
#ifdef OPENSWD3_REAL_ACT_ROOT
    using namespace openswd3::asset_runtime;
    // The original global stream-cache setting was not captured. Run both
    // production provider paths; neither setting is asserted as observed.
    for (const u32 cache_mode : {0U, 1U}) {
        LegacyActRuntime runtime{OPENSWD3_REAL_ACT_ROOT};
        runtime.set_cache_limit(0x00080000U);
        LegacyActActionStreamProvider provider{runtime};
        LegacyActionUpdater updater{provider};
        updater.set_stream_cache_mode(cache_mode);
        for (const auto& row : kObservedRecords) {
            LegacyActionRecord record;
            std::memcpy(&record, row.before.data(), sizeof(record));
            // These are the two parent writes at 0x00479887/0x00479889.
            // No child replies, stream bytes, or missing register state are
            // synthesized. This is an action-data diff, not a parent replay.
            record.action_id = row.profile;
            record.base_variant = 0x24U;
            const auto result = updater.update(record);
            const std::string sample =
                "original final_group_b action data seq " +
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
        }
    }

    std::cout << "WP316 original action-data diff: 17 samples, "
              << "both ACT cache settings, 148 bytes and stream availability "
              << "per sample; not complete sub_479850 replay.\n";
#else
    static_cast<void>(test);
    std::cout << "WP316 original action-data diff not run: "
              << "real ACT archives unavailable.\n";
#endif
}
