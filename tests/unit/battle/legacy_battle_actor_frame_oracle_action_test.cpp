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

struct ObservedActionCpu {
    u32 sequence;
    u32 loader_eax;
    u32 loader_ecx;
    u32 loader_edx;
    u32 loader_eflags;
    u32 updater_eax;
    u32 updater_ecx;
    u32 updater_edx;
    u32 updater_eflags;
};

struct ObservedSampleIndex {
    u32 sequence;
    std::size_t pair_index;
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

constexpr ObservedActionCpu kObservedV4NestedCpu[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-v4-00479850/observed-cpu.inc"
};

constexpr ObservedActionRecord kObservedV4bUniqueRecords[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-v4b-00479850/observed-records.inc"
};

constexpr ObservedSampleIndex kObservedV4bMap[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-v4b-00479850/sample-map.inc"
};

constexpr ObservedActionCpu kObservedV4bCpu[]{
#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-action-v4b-00479850/observed-cpu.inc"
};

static_assert(std::size(kObservedV2Records) == 17U);
static_assert(std::size(kObservedV3Records) == 34U);
static_assert(std::size(kObservedV4NestedRecords) == 18U);
static_assert(std::size(kObservedV4NestedCpu) == 18U);
static_assert(std::size(kObservedV4bUniqueRecords) == 46U);
static_assert(std::size(kObservedV4bMap) == 78U);
static_assert(std::size(kObservedV4bCpu) == 78U);
static_assert([] {
    for (const auto& sample : kObservedV4bMap) {
        if (sample.pair_index >= std::size(kObservedV4bUniqueRecords)) {
            return false;
        }
    }
    return true;
}());
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
                                        const bool already_prepared,
                                        const u32 observed_sequence) {
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
                std::to_string(observed_sequence) + " cache " +
                std::to_string(cache_mode);
            test.expect_equal(
                result.status,
                LegacyActionUpdateStatus::completed,
                sample + " actual ACT provider completes"
            );
            test.expect_equal(
                result.return_value, 1U, sample + " updater succeeds"
            );
            if (already_prepared) {
                // V2 preloaded 408/36; V3 preloaded 407/36 in the same
                // runtime. Both v4 runs match the warmed logical hit arm,
                // not either original cache node or guest pointer.
                test.expect_equal(
                    result.cache_hit, true, sample + " local ACT cache hit"
                );
            }
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
            compare_record(row, "original v2", false, row.sequence);
        }

        if (cache_mode == 1U) {
            for (const auto& row : kObservedV3Records) {
                compare_record(row, "original v3", false, row.sequence);
            }

            for (const auto& row : kObservedV4NestedRecords) {
                compare_record(row, "original v4 nested", true, row.sequence);
            }

            for (const auto& sample : kObservedV4bMap) {
                compare_record(
                    kObservedV4bUniqueRecords[sample.pair_index],
                    "original v4b nested", true, sample.sequence
                );
            }
        }
    }

    std::cout
        << "WP316 original action-data diff: 17 samples, "
        << "both ACT cache settings, 148 bytes and stream availability "
        << "per sample; v3 profile407 34 samples with observed "
        << "entry/leave cache setting 1; v4 profile408 18 nested "
        << "input/output samples with observed callsite cache setting 1 "
        << "and 18 locally warmed ACT cache hits; v4b profile407 "
        << "78 nested samples from 46 exact raw pairs with 78 local "
        << "ACT cache hits; not CPU replies or complete sub_479850 replay.\n";
#else
    static_cast<void>(test);
    std::cout << "WP316 original action-data diff not run: "
              << "real ACT archives unavailable.\n";
#endif
}

void test_battle_actor_frame_original_action_cpu_branches(
    openswd3::test::Context& test
) {
    // LST 432B97/432B9C: a nonzero stream pointer with ZF=1 can only
    // return from the cache-hit arm of sub_432A50. The miss arm's final
    // AND would set ZF=0 for a nonzero pointer.
    constexpr u32 kArithmeticFlags = 0x0CD5U;  // CF/PF/AF/ZF/SF/DF/OF.
    u32 wait_returns = 0U;
    u32 command_returns = 0U;
    for (std::size_t i = 0U; i < std::size(kObservedV4NestedCpu); ++i) {
        const auto& cpu = kObservedV4NestedCpu[i];
        const auto& record = kObservedV4NestedRecords[i];
        const std::string sample = "original v4 nested CPU seq " +
            std::to_string(cpu.sequence);
        test.expect_equal(cpu.sequence, record.sequence, sample + " record identity");
        test.expect_equal(cpu.loader_eax != 0U, true, sample + " stream exists");
        test.expect_equal(cpu.loader_eax, record.after[0x54U / 4U],
                          sample + " original pointer publication");
        test.expect_equal(cpu.loader_ecx, (record.before[0] % 10U) * 3U,
                          sample + " cache bucket ECX at 432E72");
        test.expect_equal(cpu.loader_eflags & kArithmeticFlags, 0x44U,
                          sample + " cache-hit CMP return flags");
        test.expect_equal(cpu.updater_eax, 1U, sample + " updater return EAX");

        const u32 wait_before = record.before[0x44U / 4U] & 0xFFFFU;
        const u32 wait_after = record.after[0x44U / 4U] & 0xFFFFU;
        if (wait_before != 0U) {
            ++wait_returns;
            // 432450 MOV AX,wait; 432454 CMP AX,0; 432A0B DEC EAX;
            // the upper half of EAX still comes from the loader pointer.
            const u32 before_dec =
                (cpu.loader_eax & 0xFFFF0000U) | wait_before;
            const u32 after_dec = before_dec - 1U;
            const u32 expected_flags =
                (cpu.loader_eflags & 0x400U) |
                ((std::popcount(after_dec & 0xFFU) & 1) == 0 ? 0x04U : 0U) |
                ((before_dec & 0x0FU) == 0U ? 0x10U : 0U) |
                (after_dec == 0U ? 0x40U : 0U) |
                ((after_dec & 0x80000000U) != 0U ? 0x80U : 0U) |
                (before_dec == 0x80000000U ? 0x800U : 0U);
            test.expect_equal(wait_after, wait_before - 1U,
                              sample + " 432A0B wait countdown");
            test.expect_equal(cpu.updater_ecx, cpu.loader_ecx,
                              sample + " wait ECX from loader");
            test.expect_equal(cpu.updater_edx, cpu.loader_edx,
                              sample + " wait EDX from loader");
            test.expect_equal(cpu.updater_eflags & kArithmeticFlags,
                              expected_flags, sample + " DEC EAX flags");
        } else {
            ++command_returns;
            // 432479 recognizes DE (0x4544); 4329B4 compares external
            // mode 0 against EBP=1 before jumping to the shared return.
            // The first update also reads the default wait from the ACT
            // stream before reaching DE; entry can still have default 0.
            const u32 wait_default_after =
                record.after[0x44U / 4U] >> 16U;
            const u32 cursor_after = record.after[0x42U / 4U] >> 16U;
            test.expect_equal(record.before[0x90U / 4U], 0U,
                              sample + " original external mode");
            test.expect_equal(wait_after, wait_default_after,
                              sample + " loaded default wait on DE");
            test.expect_equal(cpu.updater_ecx, 0x4544U,
                              sample + " DE command word");
            test.expect_equal(cpu.updater_edx, cursor_after,
                              sample + " DE command cursor");
            test.expect_equal(cpu.updater_eflags & kArithmeticFlags, 0x95U,
                              sample + " CMP external mode flags");
        }
    }

    test.expect_equal(wait_returns, 14U, "v4 observed wait-return count");
    test.expect_equal(command_returns, 4U, "v4 observed DE-return count");
    std::cout << "WP316 original v4 nested CPU/LST restricted suffix: "
              << "18 cache-hit loader returns, 14 wait decrements, "
              << "4 DE commands; not production CPU reply or parent replay.\n";

    u32 v4b_wait_returns = 0U;
    u32 v4b_de_returns = 0U;
    u32 v4b_vo_returns = 0U;
    for (std::size_t i = 0U; i < std::size(kObservedV4bCpu); ++i) {
        const auto& cpu = kObservedV4bCpu[i];
        const auto& sample = kObservedV4bMap[i];
        const auto& record = kObservedV4bUniqueRecords[sample.pair_index];
        const std::string label = "original v4b nested CPU seq " +
            std::to_string(sample.sequence);
        test.expect_equal(cpu.sequence, sample.sequence,
                          label + " source sequence");
        test.expect_equal(cpu.loader_eax != 0U, true,
                          label + " original stream exists");
        test.expect_equal(cpu.loader_eax, record.after[0x54U / 4U],
                          label + " original pointer publication");
        test.expect_equal(cpu.loader_ecx, (record.before[0] % 10U) * 3U,
                          label + " original cache bucket");
        test.expect_equal(cpu.loader_eflags & kArithmeticFlags, 0x44U,
                          label + " original cache-hit flags");
        test.expect_equal(cpu.updater_eax, 1U,
                          label + " original updater EAX");
        const u32 before_wait = record.before[0x44U / 4U] & 0xFFFFU;
        const u32 after_wait = record.after[0x44U / 4U] & 0xFFFFU;
        if (before_wait != 0U) {
            ++v4b_wait_returns;
            const u32 before_dec =
                (cpu.loader_eax & 0xFFFF0000U) | before_wait;
            const u32 after_dec = before_dec - 1U;
            const u32 flags =
                (cpu.loader_eflags & 0x400U) |
                ((std::popcount(after_dec & 0xFFU) & 1) == 0 ? 0x04U : 0U) |
                ((before_dec & 0x0FU) == 0U ? 0x10U : 0U) |
                (after_dec == 0U ? 0x40U : 0U) |
                ((after_dec & 0x80000000U) != 0U ? 0x80U : 0U) |
                (before_dec == 0x80000000U ? 0x800U : 0U);
            test.expect_equal(after_wait, before_wait - 1U,
                              label + " original wait countdown");
            test.expect_equal(cpu.updater_ecx, cpu.loader_ecx,
                              label + " wait ECX from loader");
            test.expect_equal(cpu.updater_edx, cpu.loader_edx,
                              label + " wait EDX from loader");
            test.expect_equal(cpu.updater_eflags & kArithmeticFlags,
                              flags, label + " original DEC EAX flags");
        } else {
            test.expect_equal(record.before[0x90U / 4U], 0U,
                              label + " original external mode");
            test.expect_equal(after_wait,
                              record.after[0x44U / 4U] >> 16U,
                              label + " loaded default wait");
            test.expect_equal(cpu.updater_eflags & kArithmeticFlags, 0x95U,
                              label + " original mode CMP flags");
            if (cpu.updater_ecx == 0x4544U) {
                ++v4b_de_returns;
                test.expect_equal(cpu.updater_edx,
                                  record.after[0x42U / 4U] >> 16U,
                                  label + " DE command cursor");
            } else {
                ++v4b_vo_returns;
                // 432484 recognizes VO; 4329C8 compares external mode,
                // then 4329DC resets the record cursor without DEC EDX.
                test.expect_equal(cpu.updater_ecx, 0x4F56U,
                                  label + " VO command");
                test.expect_equal(record.after[0x42U / 4U] >> 16U, 0U,
                                  label + " VO reset cursor");
                test.expect_equal(cpu.updater_edx, 0x20U,
                                  label + " observed VO next cursor");
            }
        }
    }
    test.expect_equal(v4b_wait_returns, 49U, "v4b wait-return count");
    test.expect_equal(v4b_de_returns, 24U, "v4b DE-return count");
    test.expect_equal(v4b_vo_returns, 5U, "v4b VO-return count");
    std::cout << "WP316 original v4b nested CPU/LST restricted suffix: "
              << "78 cache-hit loader returns, 49 wait decrements, "
              << "24 DE and 5 VO commands; not production CPU reply "
              << "or parent replay.\n";
}
